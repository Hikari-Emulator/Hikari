#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace hikari::core {

enum class ErrorCode : std::uint32_t {
    kInvalidArgument = 1,
    kUnexpectedEof = 2,
    kInvalidMagic = 3,
    kUnsupportedVersion = 4,
    kIntegrityMismatch = 5,
    kIoError = 6,
    kOverflow = 7,
};

struct Error {
    ErrorCode code;
    std::string_view message;
};

template <class T> using Result = std::expected<T, Error>;

[[nodiscard]] constexpr auto make_error(ErrorCode code, std::string_view message) noexcept
    -> std::unexpected<Error> {
    return std::unexpected<Error>{Error{.code = code, .message = message}};
}

} // namespace hikari::core
