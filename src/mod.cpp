// SPDX-License-Identifier: MIT
//
// Auto Head Gear: the Sonic Summoner and the O2 Mask work when owned, as if worn, the way later
// Ratchet & Clank games treat them. Nothing is put on Ratchet's head: the head slot reads as the
// owned item only inside the update of each moby that asks (head_gear.hpp), and is put back after.
//
// Where the game asks (PAL games/rac1/decomp, NTSC games/rac1/ntsc-decomp):
//   Sonic Summoner, item 5:
//     sand_mouse (437)       sand_mouse_update: follows Ratchet; wants the summoner's model too
//     sandmouse (1818)       sandmouse_update: the same for the other sand mice; wants the model
//     sandmouse_house (604)  sandmouse_house_update: the house turns toward Ratchet
//   O2 Mask, item 6:
//     toxic_gas_vent (911)   check_oscillating/timed_toxic_gas_vent_collisions: gas hurts without it
//     extermibot (623)       extermibot_update: its strike hurts without it
//     extermitank (625)      process_extermitank_particle_hits: its spray pushes without it
// The hook points are the SDK's moby updates, marked experimental (from the behaviour catalogue).
//
// And the headgear chosen in the Items menu stays on, like the boots (keep_head.hpp): read before
// each level start (rac1.level.load_gameplay: a death, a planet change, a load) and put back after.
// rac1.save.restore_data says a save was loaded; a tick's end only reads, to log a new choice.
#include <cstdint>
#include <string>

#include "openrac/games/rac1.hpp"
#include "openrac/mod.hpp"

#include "head_gear.hpp"
#include "keep_head.hpp"
#include "layout.hpp"

namespace {

using head_gear::AsWorn;
using head_gear::KeepHead;
using head_gear::Reason;
using head_gear::Restored;

struct State {
    openrac::Mod* mod = nullptr;
    head_gear::Disc disc{};
    KeepHead keep{head_gear::Disc{}, false};
    // A save was restored (Load Game, or a new game's start) since the last level start.
    bool game_loaded = false;
};

State& state() {
    static State s;
    return s;
}

// The update `original` of one moby, run with the head slot reading as `item` when owned.
template <class Original>
void as_worn(Original original, std::uint32_t moby, std::int32_t item, bool with_model) {
    const openrac::Memory memory = state().mod->memory();
    const AsWorn<openrac::Memory> worn(memory, state().disc, item, with_model);
    original(moby);
}

std::string item_name(std::int32_t item) {
    switch (item) {
        case 0: return "nothing";
        case head_gear::kSonicSummoner: return "the Sonic Summoner";
        case head_gear::kO2Mask: return "the O2 Mask";
        case head_gear::kPilotsHelmet: return "the Pilot's Helmet";
        default: return "item " + std::to_string(item);
    }
}

const char* reason_text(Reason reason) {
    switch (reason) {
        case Reason::Death: return "after a death";
        case Reason::PlanetChange: return "after a planet change";
        case Reason::GameLoaded: return "after loading a game";
    }
    return "";
}

void log_restored(const Restored& done, Reason reason) {
    const openrac::Mod& mod = *state().mod;
    const std::string why = reason_text(reason);
    switch (done.kind) {
        case Restored::Kind::Worn:
            mod.info("headgear restored " + why + ": " + item_name(done.item));
            break;
        case Restored::Kind::Bare:
            mod.info("headgear left off " + why + ", as chosen");
            break;
        case Restored::Kind::NotOwned:
            mod.info("headgear not restored " + why + ": " + item_name(done.item) + " is not owned");
            break;
        case Restored::Kind::LeftAlone:
            mod.info("headgear left as the level start set it " + why);
            break;
        case Restored::Kind::Off:
            break;
    }
}

// Each level start: the choice read before the game clears it, put back after.
template <class Original>
std::uint32_t keep_through_level_start(Original original, std::int32_t full) {
    State& s = state();
    const openrac::Memory memory = s.mod->memory();
    const std::int32_t chosen = s.keep.chosen(memory);
    const std::uint32_t result = original(full);
    const Reason reason = head_gear::reason_for(full, s.game_loaded);
    s.game_loaded = false;
    log_restored(s.keep.restore(memory, chosen), reason);
    return result;
}

void load(openrac::Mod& mod) {
    if (mod.game() != "rac1") {
        throw openrac::Error("this mod is for Ratchet & Clank");
    }
    const bool ntsc = mod.version() == "ntsc";
    if (!ntsc && mod.version() != "pal") {
        throw openrac::Error("this mod knows the PAL and NTSC-U discs only");
    }
    state().mod = &mod;
    state().disc = head_gear::disc_for(ntsc);

    namespace rac1 = openrac::rac1;
    using head_gear::kO2Mask;
    using head_gear::kSonicSummoner;
    const bool summoner = mod.setting<bool>("sonic_summoner");
    const bool mask = mod.setting<bool>("o2_mask");
    const bool keep = mod.setting<bool>("keep_head_gear");

    if (summoner) {
        mod.hook<rac1::Moby_SandMouse_Update>(
            [](auto original, std::uint32_t moby) { as_worn(original, moby, kSonicSummoner, true); });
        mod.hook<rac1::Moby_Sandmouse_Update>(
            [](auto original, std::uint32_t moby) { as_worn(original, moby, kSonicSummoner, true); });
        mod.hook<rac1::Moby_SandmouseHouse_Update>(
            [](auto original, std::uint32_t moby) { as_worn(original, moby, kSonicSummoner, false); });
    }
    if (mask) {
        mod.hook<rac1::Moby_ToxicGasVent_Update>(
            [](auto original, std::uint32_t moby) { as_worn(original, moby, kO2Mask, false); });
        mod.hook<rac1::Moby_Extermibot_Update>(
            [](auto original, std::uint32_t moby) { as_worn(original, moby, kO2Mask, false); });
        mod.hook<rac1::Moby_Extermitank_Update>(
            [](auto original, std::uint32_t moby) { as_worn(original, moby, kO2Mask, false); });
    }
    if (keep) {
        state().keep = KeepHead(state().disc, true);
        mod.hook<rac1::Level_LoadGameplay>(
            [](auto original, std::int32_t full) { return keep_through_level_start(original, full); });
        mod.hook<rac1::Save_RestoreData>([](auto original, std::uint32_t src, std::int32_t slot, std::uint32_t table) {
            const std::int32_t took = original(src, slot, table);
            State& s = state();
            s.game_loaded = true;
            s.keep.loaded(s.mod->memory());
            return took;
        });
        mod.on_frame([] {
            State& s = state();
            if (const auto chosen = s.keep.recorded(s.mod->memory())) {
                s.mod->info("headgear choice recorded: " + item_name(*chosen));
            }
        });
    }
    mod.info(std::string(summoner && mask ? "Sonic Summoner and O2 Mask work when owned"
                         : summoner       ? "Sonic Summoner works when owned"
                         : mask           ? "O2 Mask works when owned"
                                          : "Sonic Summoner and O2 Mask switched off")
             + (keep ? "; the chosen headgear stays on" : "; the chosen headgear is not kept"));
}

}  // namespace

OPENRAC_MOD_CPP("auto-head-gear", "1.1.0", load)
