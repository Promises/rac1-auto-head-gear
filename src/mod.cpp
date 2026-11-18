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
#include <cstdint>

#include "openrac/games/rac1.hpp"
#include "openrac/mod.hpp"

#include "head_gear.hpp"
#include "layout.hpp"

namespace {

using head_gear::AsWorn;

struct State {
    openrac::Mod* mod = nullptr;
    head_gear::Disc disc{};
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
    mod.info(summoner && mask ? "Sonic Summoner and O2 Mask work when owned"
             : summoner       ? "Sonic Summoner works when owned"
             : mask           ? "O2 Mask works when owned"
                              : "both switched off in its settings: changes nothing");
}

}  // namespace

OPENRAC_MOD_CPP("auto-head-gear", "1.0.0", load)
