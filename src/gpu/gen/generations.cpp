#include "gpu/gen/generations.hpp"

#include "core/result.hpp"

namespace hikari::gpu::gen {

auto MaxwellGen::decode(const CommandWord* word, CaptureSink& sink) noexcept
    -> core::Result<DecodeResult> {
    if (word == nullptr) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "gpu::gen::MaxwellGen::decode null command word");
    }
    ++sink.accepted_ops;
    return DecodeResult{.decoded_ops = 1U};
}

auto AmpereGen::decode(const CommandWord* /*word*/, CaptureSink& /*sink*/) noexcept
    -> core::Result<DecodeResult> {
    return core::make_error(core::ErrorCode::kNotImplemented,
                            "gpu::gen::AmpereGen::decode not implemented");
}

} // namespace hikari::gpu::gen
