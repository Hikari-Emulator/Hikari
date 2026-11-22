#include "core/hash.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#define XXH_INLINE_ALL
#include "picosha2/picosha2.h"
#include "xxhash/xxhash.h"

namespace hikari::core {

namespace {

static_assert(sizeof(XXH3_state_t) <= 576);
static_assert(alignof(XXH3_state_t) <= 64);

constexpr auto hex_digit(std::uint8_t nibble) noexcept -> char {
    const auto v = static_cast<std::uint8_t>(nibble & 0xFU);
    return v < 10U ? static_cast<char>('0' + v) : static_cast<char>('a' + (v - 10U));
}

void write_u64_hex_be(std::uint64_t value, std::span<char, 16> out) noexcept {
    std::size_t idx = 0;
    for (char& ch : out) {
        const auto shift = static_cast<unsigned>((15U - idx) * 4U);
        ch = hex_digit(static_cast<std::uint8_t>(value >> shift));
        ++idx;
    }
}

auto as_xxh3_state(std::byte* storage) noexcept -> XXH3_state_t* {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) opaque C state
    return reinterpret_cast<XXH3_state_t*>(storage);
}

auto as_xxh3_state(const std::byte* storage) noexcept -> const XXH3_state_t* {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) opaque C state
    return reinterpret_cast<const XXH3_state_t*>(storage);
}

} // namespace

auto xxh3_128(std::span<const std::byte> data) noexcept -> Hash128 {
    const XXH128_hash_t result = XXH3_128bits(data.data(), data.size());
    return Hash128{.low = result.low64, .high = result.high64};
}

auto sha256(std::span<const std::byte> data) noexcept -> Sha256Digest {
    picosha2::hash256_one_by_one hasher;
    hasher.init();
    for (const std::byte b : data) {
        const auto u = std::to_integer<std::uint8_t>(b);
        hasher.process(std::span<const std::uint8_t, 1>{&u, 1}.begin(),
                       std::span<const std::uint8_t, 1>{&u, 1}.end());
    }
    hasher.finish();
    Sha256Digest digest{};
    hasher.get_hash_bytes(digest.begin(), digest.end());
    return digest;
}

auto format_hash128(Hash128 hash) noexcept -> std::array<char, 33> {
    std::array<char, 33> out{};
    const std::span<char, 33> out_span{out};
    write_u64_hex_be(hash.high, out_span.subspan<0, 16>());
    write_u64_hex_be(hash.low, out_span.subspan<16, 16>());
    out.back() = '\0';
    return out;
}

auto format_sha256(const Sha256Digest& digest) noexcept -> std::array<char, 65> {
    std::array<char, 65> out{};
    std::size_t idx = 0;
    for (const std::uint8_t byte : digest) {
        out.at(idx++) = hex_digit(static_cast<std::uint8_t>(byte >> 4U));
        out.at(idx++) = hex_digit(byte);
    }
    out.back() = '\0';
    return out;
}

Xxh3Stream::Xxh3Stream() noexcept {
    reset();
}

void Xxh3Stream::reset() noexcept {
    XXH3_INITSTATE(as_xxh3_state(state_.data()));
    XXH3_128bits_reset(as_xxh3_state(state_.data()));
}

void Xxh3Stream::update(std::span<const std::byte> data) noexcept {
    if (!data.empty()) {
        XXH3_128bits_update(as_xxh3_state(state_.data()), data.data(), data.size());
    }
}

auto Xxh3Stream::finalize() const noexcept -> Hash128 {
    const XXH128_hash_t result = XXH3_128bits_digest(as_xxh3_state(state_.data()));
    return Hash128{.low = result.low64, .high = result.high64};
}

} // namespace hikari::core
