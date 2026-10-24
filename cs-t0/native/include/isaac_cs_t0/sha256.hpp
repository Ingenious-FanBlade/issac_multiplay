#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace isaac_cs_t0::mode {

using Sha256Digest = std::array<std::uint8_t, 32>;

Sha256Digest Sha256(const void* data, std::size_t size) noexcept;
Sha256Digest Sha256Pair(const void* first, std::size_t first_size,
                        const void* second, std::size_t second_size) noexcept;

} // namespace isaac_cs_t0::mode
