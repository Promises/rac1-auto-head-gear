// SPDX-License-Identifier: MIT
//
// KeepHead on a plain buffer standing in for game memory, with the game's side played as the
// decompilation shows it: the Items menu and the save write the head's equipped entry, every level
// start clears it, and the game's own O2 Mask fills the head slot without touching the entry.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>

#include "fake_memory.hpp"
#include "keep_head.hpp"
#include "layout.hpp"

namespace {

using namespace head_gear;
using head_gear_test::Buffer;
using head_gear_test::View;
using Kind = Restored::Kind;

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

// hero_switch_item's mark for an empty slot set aside.
constexpr std::int32_t kEmpty = 0x26;

struct Game {
    Buffer buffer;
    View memory{&buffer};
    Disc disc;
    // The item set aside while the game's mask is on (Hero.saved_item[2]); 0 when none.
    std::int32_t set_aside = 0;

    explicit Game(bool ntsc = false) : disc(disc_for(ntsc)) {}

    std::uint32_t entry_address() const { return disc.equipped + kEquippedHead; }
    std::uint32_t slot_address() const { return disc.hero + kHeadSlot + kSlotItem; }

    std::int32_t entry() const { return buffer.read<std::int32_t>(entry_address()); }
    std::int32_t worn() const { return buffer.read<std::int32_t>(slot_address()); }

    void own(std::int32_t item) { buffer.write(disc.owned + static_cast<std::uint32_t>(item), std::uint8_t{1}); }

    // The Items menu: close_pause_menu asks for the item (0x26 for none), hero_switch_item puts it in
    // the slot and the entry.
    void choose(std::int32_t item) {
        buffer.write(entry_address(), item);
        buffer.write(slot_address(), item);
    }

    // Load Game: the save's equipped items restored (memcard_RestoreData), then the mod told.
    void load_save(KeepHead& keep, std::int32_t head) {
        buffer.write(entry_address(), head);
        keep.loaded(memory);
    }

    // The game's own O2 Mask (hero_switch_item, slot 2, swimming or where there is no air): the worn
    // item set aside, the mask in the slot, the entry left as it is.
    void force_mask() {
        set_aside = worn() != 0 ? worn() : kEmpty;
        buffer.write(slot_address(), kO2Mask);
    }

    // The need ends: what was set aside goes back to the slot and the entry.
    void end_force() {
        const std::int32_t back = set_aside == kEmpty ? 0 : set_aside;
        buffer.write(entry_address(), back);
        buffer.write(slot_address(), back);
        set_aside = 0;
    }

    // A level start with the mod around it: LoadLevelGameplay's initialize_hero clears the hero (the
    // slot) and reset_hero_to_idle the entry. `set_by_start` stands for a start that puts something
    // there itself, which the game does not.
    Restored level_start(KeepHead& keep, std::int32_t set_by_start = 0) {
        const std::int32_t chosen = keep.chosen(memory);
        buffer.write(slot_address(), std::int32_t{0});
        buffer.write(entry_address(), set_by_start);
        set_aside = 0;
        return keep.restore(memory, chosen);
    }
};

void the_choice_is_recorded() {
    Game g;
    KeepHead keep(g.disc, true);
    check(!keep.recorded(g.memory), "record: the entry as found is no choice");
    g.choose(kPilotsHelmet);
    const auto first = keep.recorded(g.memory);
    check(first && *first == kPilotsHelmet, "record: the helmet chosen");
    check(!keep.recorded(g.memory), "record: once");
    g.choose(0);
    const auto second = keep.recorded(g.memory);
    check(second && *second == 0, "record: taken off");
}

void restored_after_a_death() {
    Game g;
    KeepHead keep(g.disc, true);
    g.own(kPilotsHelmet);
    g.choose(kPilotsHelmet);
    keep.recorded(g.memory);
    const Restored done = g.level_start(keep);
    check(head_gear::reason_for(0, false) == Reason::Death, "death: the reason");
    check(done.kind == Kind::Worn && done.item == kPilotsHelmet, "death: put back");
    check(g.entry() == kPilotsHelmet, "death: the entry holds the helmet");
    check(!keep.recorded(g.memory), "death: the restore is no new choice");
}

void restored_after_a_planet_change() {
    Game g;
    KeepHead keep(g.disc, true);
    g.own(kSonicSummoner);
    g.choose(kSonicSummoner);
    const Restored done = g.level_start(keep);
    check(head_gear::reason_for(1, false) == Reason::PlanetChange, "planet: the reason");
    check(done.kind == Kind::Worn && g.entry() == kSonicSummoner, "planet: put back");
}

void restored_after_a_load() {
    Game g;
    KeepHead keep(g.disc, true);
    g.own(kPilotsHelmet);
    g.own(kO2Mask);
    g.choose(kPilotsHelmet);
    keep.recorded(g.memory);
    g.load_save(keep, kO2Mask);  // the save was made wearing the mask
    check(!keep.recorded(g.memory), "load: the save's entry is no new choice");
    const Restored done = g.level_start(keep);
    check(head_gear::reason_for(1, true) == Reason::GameLoaded, "load: the reason");
    check(head_gear::reason_for(0, true) == Reason::GameLoaded, "load: the reason, in place");
    check(done.kind == Kind::Worn && g.entry() == kO2Mask, "load: the save's choice put back");

    g.load_save(keep, 0);  // a save made with nothing on
    const Restored bare = g.level_start(keep);
    check(bare.kind == Kind::Bare && g.entry() == 0, "load: a bare save stays bare");
}

void the_games_mask_is_left_to_it_and_the_choice_comes_back() {
    Game g;
    KeepHead keep(g.disc, true);
    g.own(kO2Mask);
    g.own(kPilotsHelmet);
    g.choose(kPilotsHelmet);
    keep.recorded(g.memory);

    g.force_mask();
    check(!keep.recorded(g.memory), "forced: no choice recorded");
    check(g.worn() == kO2Mask && g.entry() == kPilotsHelmet, "forced: the mask worn, the choice kept");
    g.end_force();
    check(!keep.recorded(g.memory), "forced: its end is no new choice");
    check(g.worn() == kPilotsHelmet, "forced: the helmet back after");

    g.force_mask();  // drowned with the mask forced on: the level starts again
    const Restored done = g.level_start(keep);
    check(done.kind == Kind::Worn && g.entry() == kPilotsHelmet, "forced: a death restores the helmet, not the mask");
}

void nothing_is_restored_as_nothing() {
    Game g;
    KeepHead keep(g.disc, true);
    g.own(kO2Mask);
    g.own(kPilotsHelmet);
    g.choose(kPilotsHelmet);
    g.choose(0);
    keep.recorded(g.memory);
    const Restored done = g.level_start(keep);
    check(done.kind == Kind::Bare && g.entry() == 0, "nothing: stays bare");

    g.force_mask();
    g.end_force();
    check(g.worn() == 0 && g.entry() == 0, "nothing: bare again after the game's mask");
    g.force_mask();
    const Restored after = g.level_start(keep);
    check(after.kind == Kind::Bare && g.entry() == 0, "nothing: a death under the game's mask leaves it bare");
}

void an_item_not_owned_is_not_restored() {
    Game g;
    KeepHead keep(g.disc, true);
    g.load_save(keep, kPilotsHelmet);  // an entry for an item this game does not own
    const Restored done = g.level_start(keep);
    check(done.kind == Kind::NotOwned && done.item == kPilotsHelmet, "not owned: said so");
    check(g.entry() == 0, "not owned: the head stays bare");
}

void the_setting_off_restores_nothing() {
    Game g;
    KeepHead keep(g.disc, false);
    g.own(kPilotsHelmet);
    g.choose(kPilotsHelmet);
    check(!keep.recorded(g.memory), "off: no record");
    const Restored done = g.level_start(keep);
    check(done.kind == Kind::Off, "off: nothing done");
    check(g.entry() == 0, "off: the head comes up bare as the game has it");
}

void what_the_start_sets_or_no_head_item_is_left_alone() {
    Game g;
    KeepHead keep(g.disc, true);
    g.own(kPilotsHelmet);
    g.own(kO2Mask);
    g.choose(kPilotsHelmet);
    const Restored done = g.level_start(keep, kO2Mask);
    check(done.kind == Kind::LeftAlone && g.entry() == kO2Mask, "start's own: kept");

    g.load_save(keep, 0x1C);  // not a head item (the Magneboots' id)
    const Restored odd = g.level_start(keep);
    check(odd.kind == Kind::LeftAlone && g.entry() == 0, "no head item: not written");
}

void both_discs_write_their_own_entry() {
    for (const bool ntsc : {false, true}) {
        Game g(ntsc);
        KeepHead keep(g.disc, true);
        g.own(kSonicSummoner);
        g.choose(kSonicSummoner);
        g.level_start(keep);
        check(g.buffer.read<std::int32_t>(ntsc ? 0x00141668u : 0x00141768u) == kSonicSummoner,
              ntsc ? "NTSC: the entry at 0x141668" : "PAL: the entry at 0x141768");
    }
}

}  // namespace

int main() {
    the_choice_is_recorded();
    restored_after_a_death();
    restored_after_a_planet_change();
    restored_after_a_load();
    the_games_mask_is_left_to_it_and_the_choice_comes_back();
    nothing_is_restored_as_nothing();
    an_item_not_owned_is_not_restored();
    the_setting_off_restores_nothing();
    what_the_start_sets_or_no_head_item_is_left_alone();
    both_discs_write_their_own_entry();
    if (failures == 0) {
        std::printf("keep_head: all passed\n");
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
