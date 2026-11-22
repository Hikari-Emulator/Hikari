#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>

namespace hikari::core {

template <class T, std::size_t Capacity> class SpscQueue {
    static_assert(Capacity >= 2, "SpscQueue capacity must be at least 2");

public:
    [[nodiscard]] auto try_push(const T& item) noexcept -> bool {
        const std::size_t w = write_idx_.load(std::memory_order_relaxed);
        const std::size_t next_w = (w + 1U) % Capacity;
        if (next_w == read_idx_.load(std::memory_order_acquire)) {
            return false;
        }
        buffer_.at(w) = item;
        write_idx_.store(next_w, std::memory_order_release);
        return true;
    }

    [[nodiscard]] auto try_pop() noexcept -> std::optional<T> {
        const std::size_t r = read_idx_.load(std::memory_order_relaxed);
        if (r == write_idx_.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        T item = buffer_.at(r);
        read_idx_.store((r + 1U) % Capacity, std::memory_order_release);
        return item;
    }

private:
    alignas(64) std::atomic<std::size_t> write_idx_{0};
    alignas(64) std::atomic<std::size_t> read_idx_{0};
    std::array<T, Capacity> buffer_{};
};

} // namespace hikari::core
