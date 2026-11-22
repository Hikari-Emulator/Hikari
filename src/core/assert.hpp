#pragma once

namespace hikari::core::detail {

[[noreturn]] void abort_with_diagnostic(const char* expr, const char* file, int line) noexcept;

} // namespace hikari::core::detail

#define HIKARI_CHECK(cond)                                                                         \
    ((cond) ? static_cast<void>(0)                                                                 \
            : ::hikari::core::detail::abort_with_diagnostic(#cond, __FILE__, __LINE__))

#ifndef NDEBUG
#define HIKARI_ASSERT(cond) HIKARI_CHECK(cond)
#else
#define HIKARI_ASSERT(cond) ((void)sizeof(cond))
#endif

#define HIKARI_UNREACHABLE()                                                                       \
    ::hikari::core::detail::abort_with_diagnostic("unreachable", __FILE__, __LINE__)
