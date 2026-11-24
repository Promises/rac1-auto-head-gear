// SPDX-License-Identifier: MIT
//
// Game memory for the tests: a plain buffer, read and written as openrac::Memory is.
#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

namespace head_gear_test {

struct Buffer {
    // Up to past the hero (0x0013F450 + 0x2080 + 4 on PAL) and the equipped items, on the heap.
    std::vector<std::uint8_t> bytes = std::vector<std::uint8_t>(0x00142000);

    template <class T>
    T read(std::uint32_t address) const {
        static_assert(std::is_trivially_copyable_v<T>);
        T value;
        std::memcpy(&value, bytes.data() + address, sizeof value);
        return value;
    }

    template <class T>
    void write(std::uint32_t address, const T& value) {
        std::memcpy(bytes.data() + address, &value, sizeof value);
    }
};

// openrac::Memory's write is const (a view); the buffer is the test's own.
struct View {
    Buffer* buffer;

    template <class T>
    T read(std::uint32_t address) const {
        return buffer->read<T>(address);
    }

    template <class T>
    void write(std::uint32_t address, const T& value) const {
        buffer->write(address, value);
    }
};

}  // namespace head_gear_test
