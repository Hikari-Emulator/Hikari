#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace hikari::core {

struct Hash128 {
    std::uint64_t low = 0;
    std::uint64_t high = 0;

    [[nodiscard]] constexpr bool operator==(const Hash128&) const noexcept = default;
};

using Sha256Digest = std::array<std::uint8_t, 32>;

[[nodiscard]] auto xxh3_128(std::span<const std::byte> data) noexcept -> Hash128;
[[nodiscard]] auto sha256(std::span<const std::byte> data) noexcept -> Sha256Digest;

[[nodiscard]] auto format_hash128(Hash128 hash) noexcept -> std::array<char, 33>;
[[nodiscard]] auto format_sha256(const Sha256Digest& digest) noexcept -> std::array<char, 65>;

class Xxh3Stream {
public:
    Xxh3Stream() noexcept;

    void reset() noexcept;
    void update(std::span<const std::byte> data) noexcept;
    [[nodiscard]] auto finalize() const noexcept -> Hash128;

private:
    static constexpr std::size_t kStateStorageBytes = 576;
    alignas(64) std::array<std::byte, kStateStorageBytes> state_{};
};

} // namespace hikari::core
