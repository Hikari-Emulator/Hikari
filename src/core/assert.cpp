#include "core/assert.hpp"

#include <cstdio>
#include <cstdlib>

namespace hikari::core::detail {

void abort_with_diagnostic(const char* expr, const char* file, int line) noexcept {
    std::fputs("hikari: invariant failed (", stderr);
    std::fputs(expr != nullptr ? expr : "?", stderr);
    std::fputs(") at ", stderr);
    std::fputs(file != nullptr ? file : "?", stderr);
    std::fputs("\n", stderr);
    (void)line;
    std::fflush(stderr);
    std::abort();
}

} // namespace hikari::core::detail
