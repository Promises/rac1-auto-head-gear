// SPDX-License-Identifier: MIT
//
// The headgear the player chose, kept on like the boots.
//
// The player's choice is the head's entry of the equipped items (layout.hpp, kEquippedHead): the
// Items menu puts the item chosen there (close_pause_menu asks for it, hero_switch_item writes it),
// the game's save keeps it, and the head slot is made from it at a level's start. Every level start
// clears it (reset_hero_to_idle, inside LoadLevelGameplay), and a death, a planet change and a load
// all start the level again: so the head comes up bare, while the boots' entry, never cleared, keeps
// them on. KeepHead reads the entry before the level start and puts it back after.
//
// The game's own O2 Mask (underwater, drowning, and where there is no air: Blarg's lift, Orxon,
// Gemlik's surface) never writes the entry: hero_switch_item puts the mask in the head slot with the
// worn item set aside and gives that back when the need ends, the entry untouched. So the entry is
// the player's choice throughout, KeepHead writes nothing while the game plays, and the forced mask
// is the game's alone.
//
// Memory is anything with read<T>(address) and write(address, value), as for AsWorn.
#pragma once

#include <cstdint>
#include <optional>

#include "layout.hpp"

namespace head_gear {

// What started the level again.
enum class Reason {
    Death,         // the level made again in place (LoadLevelGameplay with full 0)
    PlanetChange,  // a level started anew
    GameLoaded,    // a save loaded, or a new game, since the last level start
};

inline Reason reason_for(std::int32_t full, bool game_loaded) {
    if (game_loaded) {
        return Reason::GameLoaded;
    }
    return full == 0 ? Reason::Death : Reason::PlanetChange;
}

// What KeepHead did with the head after a level start.
struct Restored {
    enum class Kind {
        Off,       // the setting is off: nothing done
        Worn,      // the chosen item put back on
        Bare,      // nothing chosen: the head stays bare, as the level start leaves it
        NotOwned,  // the chosen item is not owned (any more): the head stays bare
        LeftAlone, // the level start put something there itself, or the entry held no head item
    };
    Kind kind = Kind::Off;
    // The item chosen: the one put back, or the one not owned.
    std::int32_t item = 0;
};

class KeepHead {
public:
    KeepHead(const Disc& disc, bool enabled)
        : m_entry(disc.equipped + kEquippedHead), m_owned(disc.owned), m_enabled(enabled) {}

    // The choice now: read it before the level start clears it.
    template <class Memory>
    std::int32_t chosen(const Memory& memory) const {
        return memory.template read<std::int32_t>(m_entry);
    }

    // After the level start: puts `chosen` back when it is a head item still owned and the start
    // left the entry empty. Writes at most once, and only the entry the game makes the slot from.
    template <class Memory>
    Restored restore(const Memory& memory, std::int32_t chosen) {
        Restored done;
        if (!m_enabled) {
            return done;
        }
        done.item = chosen;
        if (memory.template read<std::int32_t>(m_entry) != 0 || (chosen != 0 && !is_head_item(chosen))) {
            done.kind = Restored::Kind::LeftAlone;
        } else if (chosen == 0) {
            done.kind = Restored::Kind::Bare;
        } else if (memory.template read<std::uint8_t>(m_owned + static_cast<std::uint32_t>(chosen)) == 0) {
            done.kind = Restored::Kind::NotOwned;
        } else {
            memory.write(m_entry, chosen);
            done.kind = Restored::Kind::Worn;
        }
        see(memory);
        return done;
    }

    // Once a tick: the new choice when the game changed the entry since (the Items menu), else none.
    // Only reads.
    template <class Memory>
    std::optional<std::int32_t> recorded(const Memory& memory) {
        if (!m_enabled) {
            return std::nullopt;
        }
        const std::int32_t now = memory.template read<std::int32_t>(m_entry);
        if (!m_seen_once) {
            see(memory);  // the entry as the mod found it: not a change
            return std::nullopt;
        }
        if (now == m_seen) {
            return std::nullopt;
        }
        m_seen = now;
        return now;
    }

    // A save restored the entry: not a choice made in this game, so not recorded.
    template <class Memory>
    void loaded(const Memory& memory) {
        see(memory);
    }

private:
    template <class Memory>
    void see(const Memory& memory) {
        m_seen = memory.template read<std::int32_t>(m_entry);
        m_seen_once = true;
    }

    std::uint32_t m_entry;
    std::uint32_t m_owned;
    bool m_enabled;
    // The entry as last seen, so that only the game's changes of it are recorded.
    std::int32_t m_seen = 0;
    bool m_seen_once = false;
};

}  // namespace head_gear
