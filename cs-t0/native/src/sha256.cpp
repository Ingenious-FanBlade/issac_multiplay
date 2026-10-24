#include "isaac_cs_t0/sha256.hpp"

#include <array>
#include <cstring>

namespace isaac_cs_t0::mode {
namespace {

constexpr std::array<std::uint32_t, 64> kRoundConstants{{
    0x428A2F98u, 0x71374491u, 0xB5C0FBCFu, 0xE9B5DBA5u, 0x3956C25Bu, 0x59F111F1u,
    0x923F82A4u, 0xAB1C5ED5u, 0xD807AA98u, 0x12835B01u, 0x243185BEu, 0x550C7DC3u,
    0x72BE5D74u, 0x80DEB1FEu, 0x9BDC06A7u, 0xC19BF174u, 0xE49B69C1u, 0xEFBE4786u,
    0x0FC19DC6u, 0x240CA1CCu, 0x2DE92C6Fu, 0x4A7484AAu, 0x5CB0A9DCu, 0x76F988DAu,
    0x983E5152u, 0xA831C66Du, 0xB00327C8u, 0xBF597FC7u, 0xC6E00BF3u, 0xD5A79147u,
    0x06CA6351u, 0x14292967u, 0x27B70A85u, 0x2E1B2138u, 0x4D2C6DFCu, 0x53380D13u,
    0x650A7354u, 0x766A0ABBu, 0x81C2C92Eu, 0x92722C85u, 0xA2BFE8A1u, 0xA81A664Bu,
    0xC24B8B70u, 0xC76C51A3u, 0xD192E819u, 0xD6990624u, 0xF40E3585u, 0x106AA070u,
    0x19A4C116u, 0x1E376C08u, 0x2748774Cu, 0x34B0BCB5u, 0x391C0CB3u, 0x4ED8AA4Au,
    0x5B9CCA4Fu, 0x682E6FF3u, 0x748F82EEu, 0x78A5636Fu, 0x84C87814u, 0x8CC70208u,
    0x90BEFFFAu, 0xA4506CEBu, 0xBEF9A3F7u, 0xC67178F2u,
}};

constexpr std::uint32_t RotateRight(std::uint32_t value, std::uint32_t bits) noexcept
{
    return (value >> bits) | (value << (32u - bits));
}

class Sha256State final {
public:
    void Update(const void* data, std::size_t size) noexcept
    {
        if (!data || !size)
            return;
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        total_size_ += size;
        while (size)
        {
            const std::size_t take = (std::min)(size, block_.size() - block_size_);
            std::memcpy(block_.data() + block_size_, bytes, take);
            block_size_ += take;
            bytes += take;
            size -= take;
            if (block_size_ == block_.size())
            {
                Transform(block_.data());
                block_size_ = 0;
            }
        }
    }

    Sha256Digest Final() noexcept
    {
        const std::uint64_t bit_size = static_cast<std::uint64_t>(total_size_) * 8u;
        block_[block_size_++] = 0x80u;
        if (block_size_ > 56)
        {
            while (block_size_ < block_.size())
                block_[block_size_++] = 0;
            Transform(block_.data());
            block_size_ = 0;
        }
        while (block_size_ < 56)
            block_[block_size_++] = 0;
        for (std::size_t index = 0; index < 8; ++index)
            block_[63 - index] = static_cast<std::uint8_t>(bit_size >> (index * 8));
        Transform(block_.data());

        Sha256Digest result{};
        for (std::size_t index = 0; index < state_.size(); ++index)
        {
            result[index * 4] = static_cast<std::uint8_t>(state_[index] >> 24);
            result[index * 4 + 1] = static_cast<std::uint8_t>(state_[index] >> 16);
            result[index * 4 + 2] = static_cast<std::uint8_t>(state_[index] >> 8);
            result[index * 4 + 3] = static_cast<std::uint8_t>(state_[index]);
        }
        return result;
    }

private:
    void Transform(const std::uint8_t* block) noexcept
    {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t index = 0; index < 16; ++index)
        {
            const std::size_t offset = index * 4;
            words[index] = (static_cast<std::uint32_t>(block[offset]) << 24) |
                           (static_cast<std::uint32_t>(block[offset + 1]) << 16) |
                           (static_cast<std::uint32_t>(block[offset + 2]) << 8) |
                           static_cast<std::uint32_t>(block[offset + 3]);
        }
        for (std::size_t index = 16; index < words.size(); ++index)
        {
            const std::uint32_t s0 = RotateRight(words[index - 15], 7) ^
                                     RotateRight(words[index - 15], 18) ^
                                     (words[index - 15] >> 3);
            const std::uint32_t s1 = RotateRight(words[index - 2], 17) ^
                                     RotateRight(words[index - 2], 19) ^
                                     (words[index - 2] >> 10);
            words[index] = words[index - 16] + s0 + words[index - 7] + s1;
        }
        std::uint32_t a = state_[0];
        std::uint32_t b = state_[1];
        std::uint32_t c = state_[2];
        std::uint32_t d = state_[3];
        std::uint32_t e = state_[4];
        std::uint32_t f = state_[5];
        std::uint32_t g = state_[6];
        std::uint32_t h = state_[7];
        for (std::size_t index = 0; index < words.size(); ++index)
        {
            const std::uint32_t s1 = RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
            const std::uint32_t choose = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = h + s1 + choose + kRoundConstants[index] + words[index];
            const std::uint32_t s0 = RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + majority;
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    std::array<std::uint32_t, 8> state_{{
        0x6A09E667u, 0xBB67AE85u, 0x3C6EF372u, 0xA54FF53Au,
        0x510E527Fu, 0x9B05688Cu, 0x1F83D9ABu, 0x5BE0CD19u,
    }};
    std::array<std::uint8_t, 64> block_{};
    std::size_t block_size_ = 0;
    std::size_t total_size_ = 0;
};

} // namespace

Sha256Digest Sha256(const void* data, std::size_t size) noexcept
{
    Sha256State state;
    state.Update(data, size);
    return state.Final();
}

Sha256Digest Sha256Pair(const void* first, std::size_t first_size,
                        const void* second, std::size_t second_size) noexcept
{
    Sha256State state;
    state.Update(first, first_size);
    state.Update(second, second_size);
    return state.Final();
}

} // namespace isaac_cs_t0::mode
