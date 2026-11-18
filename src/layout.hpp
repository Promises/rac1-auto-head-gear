// SPDX-License-Identifier: MIT
//
// Where Ratchet & Clank keeps what this mod reads and writes, on both discs. The hero's field
// offsets are the same on PAL and NTSC; the bases differ, and NTSC is not a flat 0x100 below PAL
// (the same bases gadget-quick-use and weapon-leveling use).
//   games/rac1/decomp/include/structs/hero.h (PAL), games/rac1/ntsc-decomp/include/rnc/hero/hero.h
#pragma once

#include <cstdint>

namespace head_gear {

// g_hero.items[2], the head slot (struct HeroItemSlot, 0x50 bytes, items[] at hero + 0x1090).
constexpr std::uint32_t kHeadSlot = 0x1090u + 2u * 0x50u;
// HeroItemSlot.moby: the model worn on the head, 0 when nothing is.
constexpr std::uint32_t kSlotModel = 0x00u;
// HeroItemSlot.item_id: the item worn on the head.
constexpr std::uint32_t kSlotItem = 0x28u;
// Hero.moby: Ratchet's own moby.
constexpr std::uint32_t kHeroMoby = 0x2080u;

// Item ids: the index into the owned-items bytes and what the head slot's item_id holds.
constexpr std::int32_t kSonicSummoner = 5;
constexpr std::int32_t kO2Mask = 6;

struct Disc {
    // g_hero.
    std::uint32_t hero;
    // item_available[]: one byte per item id, nonzero when owned.
    std::uint32_t owned;
};

inline Disc disc_for(bool ntsc) {
    if (ntsc) {
        return {0x0013F350u, 0x0013D4C0u};
    }
    return {0x0013F450u, 0x0013D5C8u};
}

}  // namespace head_gear
