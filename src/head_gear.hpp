// SPDX-License-Identifier: MIT
//
// The head slot, read as worn: for the length of one game function, an item the player owns reads
// as the one on Ratchet's head, then the slot is put back as it was.
//
// The game decides a few things by the item worn on the head (g_hero.items[2]) where owning it
// should do: the Sonic Summoner calls the sand mice only while worn, and the O2 Mask keeps out
// Blarg's toxic gas and Rilgar's extermination robots only while worn. The checks are inside the
// update functions of those mobys, so the mod wraps each update in an AsWorn.
//
// Memory is anything with read<T>(address) and write(address, value), openrac::Memory in the
// game and a plain buffer in the tests.
#pragma once

#include <cstdint>

#include "layout.hpp"

namespace head_gear {

template <class Memory>
class AsWorn {
public:
    // Makes the head slot read as `item` when the player owns it and wears something else.
    // `with_model`: the checks also want a model on the head (the Sonic Summoner's); with none
    // there, Ratchet's own moby stands in. The game only touches that model after checking it is
    // a Sonic Summoner, which Ratchet's moby is not, so nothing is drawn or moved differently.
    AsWorn(const Memory& memory, const Disc& disc, std::int32_t item, bool with_model)
        : m_memory(memory), m_slot(disc.hero + kHeadSlot) {
        if (memory.template read<std::uint8_t>(disc.owned + static_cast<std::uint32_t>(item)) == 0) {
            return;
        }
        m_item = memory.template read<std::int32_t>(m_slot + kSlotItem);
        if (m_item == item) {
            return;
        }
        const bool bare = memory.template read<std::uint32_t>(m_slot + kSlotModel) == 0;
        m_stand_in = with_model && bare ? memory.template read<std::uint32_t>(disc.hero + kHeroMoby) : 0;
        memory.write(m_slot + kSlotItem, item);
        if (m_stand_in != 0) {
            memory.write(m_slot + kSlotModel, m_stand_in);
        }
        m_as = item;
    }

    // Puts the slot back, unless the game changed it meanwhile: then its change stays.
    ~AsWorn() {
        if (m_as == 0) {
            return;
        }
        if (m_memory.template read<std::int32_t>(m_slot + kSlotItem) == m_as) {
            m_memory.write(m_slot + kSlotItem, m_item);
        }
        if (m_stand_in != 0 && m_memory.template read<std::uint32_t>(m_slot + kSlotModel) == m_stand_in) {
            m_memory.write(m_slot + kSlotModel, std::uint32_t{0});
        }
    }

    AsWorn(const AsWorn&) = delete;
    AsWorn& operator=(const AsWorn&) = delete;

    // Whether the slot reads as another item than the one worn, until this ends.
    bool active() const { return m_as != 0; }

private:
    const Memory& m_memory;
    std::uint32_t m_slot;
    // The item it reads as while active; 0 when it changed nothing.
    std::int32_t m_as = 0;
    // The item the slot held.
    std::int32_t m_item = 0;
    // Ratchet's moby, put in as the model; 0 when none was put in.
    std::uint32_t m_stand_in = 0;
};

}  // namespace head_gear
