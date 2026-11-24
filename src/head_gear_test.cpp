// SPDX-License-Identifier: MIT
//
// AsWorn on a plain buffer standing in for game memory: when it changes the head slot, what it
// puts there, and that it puts back exactly what was there.
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "fake_memory.hpp"
#include "head_gear.hpp"
#include "layout.hpp"

namespace {

using namespace head_gear;
using head_gear_test::Buffer;
using head_gear_test::View;

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

constexpr std::uint32_t kRatchet = 0x00150000u;
constexpr std::uint32_t kMaskModel = 0x00150100u;

struct Game {
    Buffer buffer;
    View memory{&buffer};
    Disc disc = disc_for(true);

    Game() { buffer.write(disc.hero + kHeroMoby, kRatchet); }

    void own(std::int32_t item) { buffer.write(disc.owned + static_cast<std::uint32_t>(item), std::uint8_t{1}); }

    void wear(std::int32_t item, std::uint32_t model) {
        buffer.write(disc.hero + kHeadSlot + kSlotItem, item);
        buffer.write(disc.hero + kHeadSlot + kSlotModel, model);
    }

    std::int32_t item() const { return buffer.read<std::int32_t>(disc.hero + kHeadSlot + kSlotItem); }
    std::uint32_t model() const { return buffer.read<std::uint32_t>(disc.hero + kHeadSlot + kSlotModel); }
};

void not_owned_changes_nothing() {
    Game g;
    {
        const AsWorn<View> worn(g.memory, g.disc, kSonicSummoner, true);
        check(!worn.active(), "not owned: inactive");
        check(g.item() == 0 && g.model() == 0, "not owned: slot untouched");
    }
    check(g.item() == 0 && g.model() == 0, "not owned: slot untouched after");
}

void owned_bare_head_reads_as_worn_with_a_stand_in() {
    Game g;
    g.own(kSonicSummoner);
    {
        const AsWorn<View> worn(g.memory, g.disc, kSonicSummoner, true);
        check(worn.active(), "bare head: active");
        check(g.item() == kSonicSummoner, "bare head: reads as the summoner");
        check(g.model() == kRatchet, "bare head: Ratchet's moby stands in");
    }
    check(g.item() == 0 && g.model() == 0, "bare head: put back");
}

void another_item_worn_keeps_its_model() {
    Game g;
    g.own(kSonicSummoner);
    g.wear(kO2Mask, kMaskModel);
    {
        const AsWorn<View> worn(g.memory, g.disc, kSonicSummoner, true);
        check(g.item() == kSonicSummoner, "mask worn: reads as the summoner");
        check(g.model() == kMaskModel, "mask worn: its model stays");
    }
    check(g.item() == kO2Mask && g.model() == kMaskModel, "mask worn: put back");
}

void without_model_only_the_item_changes() {
    Game g;
    g.own(kO2Mask);
    {
        const AsWorn<View> worn(g.memory, g.disc, kO2Mask, false);
        check(g.item() == kO2Mask, "gas: reads as the mask");
        check(g.model() == 0, "gas: no stand-in");
    }
    check(g.item() == 0 && g.model() == 0, "gas: put back");
}

void already_worn_changes_nothing() {
    Game g;
    g.own(kO2Mask);
    g.wear(kO2Mask, kMaskModel);
    {
        const AsWorn<View> worn(g.memory, g.disc, kO2Mask, false);
        check(!worn.active(), "worn: inactive");
    }
    check(g.item() == kO2Mask && g.model() == kMaskModel, "worn: untouched");
}

void a_change_by_the_game_meanwhile_stays() {
    Game g;
    g.own(kSonicSummoner);
    {
        const AsWorn<View> worn(g.memory, g.disc, kSonicSummoner, true);
        g.wear(kO2Mask, kMaskModel);  // the game swaps the mask on inside the update
    }
    check(g.item() == kO2Mask && g.model() == kMaskModel, "game's change kept");
}

}  // namespace

int main() {
    not_owned_changes_nothing();
    owned_bare_head_reads_as_worn_with_a_stand_in();
    another_item_worn_keeps_its_model();
    without_model_only_the_item_changes();
    already_worn_changes_nothing();
    a_change_by_the_game_meanwhile_stays();
    if (failures == 0) {
        std::printf("head_gear: all passed\n");
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
