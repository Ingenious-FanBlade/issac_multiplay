#pragma once

#include "isaac_cs_t0/hooks.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace isaac_cs_t0::active {

inline constexpr std::size_t kActiveRelativeCount = 6;
inline constexpr std::size_t kActivePointerCount = 7;

enum class TargetFailureStage : std::uint32_t {
    None = 0,
    InvalidArguments = 1,
    RelativeBytes = 2,
    RelativeTarget = 3,
    PointerTarget = 4,
    PointerOwner = 5,
    VtableCol = 6,
};

struct TargetFailure {
    TargetFailureStage stage = TargetFailureStage::None;
    std::uint32_t index = UINT32_MAX;
};

struct ActiveReplacements {
    std::array<void*, kActiveRelativeCount> relative{};
    std::array<void*, kActivePointerCount> pointers{};
};

// Performs all frozen-image validation and prepares the caller-owned patch descriptors.
// Originals are bound to frozen RVAs before the suspended transaction begins, so wrappers
// never depend on a post-resume captured-original copy-back.
bool PrepareActiveTargets(
    std::uint8_t* image_base, const ActiveReplacements& replacements,
    std::array<isaac_cs_t0::mode::RelativeCallPatch, kActiveRelativeCount>& relative,
    std::array<isaac_cs_t0::mode::PointerPatch, kActivePointerCount>& pointers,
    TargetFailure& failure) noexcept;

bool VerifyActiveTargetsRestored(
    const std::array<isaac_cs_t0::mode::RelativeCallPatch, kActiveRelativeCount>& relative,
    const std::array<isaac_cs_t0::mode::PointerPatch, kActivePointerCount>& pointers) noexcept;

} // namespace isaac_cs_t0::active
