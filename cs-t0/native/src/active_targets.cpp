#include "isaac_cs_t0/active_targets.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace isaac_cs_t0::active {
namespace {

constexpr std::array<std::uint32_t, kActiveRelativeCount> kCallRvas{{
    0x00385BC2u, 0x00385BC9u, 0x00385BD2u,
    0x00385BD9u, 0x00385BE0u, 0x00385BE7u,
}};
constexpr std::array<std::uint32_t, kActiveRelativeCount> kCallTargetRvas{{
    0x00379B40u, 0x00380F40u, 0x00391A40u,
    0x003A3D30u, 0x0037B020u, 0x003A42B0u,
}};
constexpr std::array<std::array<std::uint8_t, 5>, kActiveRelativeCount> kCallBytes{{
    {{0xE8, 0x79, 0x3F, 0xFF, 0xFF}},
    {{0xE8, 0x72, 0xB3, 0xFF, 0xFF}},
    {{0xE8, 0x69, 0xBE, 0x00, 0x00}},
    {{0xE8, 0x52, 0xE1, 0x01, 0x00}},
    {{0xE8, 0x3B, 0x54, 0xFF, 0xFF}},
    {{0xE8, 0xC4, 0xE6, 0x01, 0x00}},
}};
constexpr std::array<std::uint32_t, kActivePointerCount> kPointerRvas{{
    0x007A16A8u, 0x007A16ACu, 0x007A16B0u,
    0x00782CB0u, 0x00782CB4u, 0x00782CB8u,
    0x0076BDDCu,
}};
constexpr std::array<std::uint32_t, kActivePointerCount> kPointerTargetRvas{{
    0x0061E8D0u, 0x0061E970u, 0x0061EA20u,
    0x0061E8D0u, 0x0061E970u, 0x0061EA20u,
    0x00382AF0u,
}};
constexpr std::uint32_t kKeyboardVtableRva = 0x007A1678u;
constexpr std::uint32_t kKeyboardColRva = 0x007B191Cu;
constexpr std::uint32_t kGamepadVtableRva = 0x00782C80u;
constexpr std::uint32_t kGamepadColRva = 0x007B0460u;

bool ValidateVtableCol(std::uint8_t* image_base, std::uint32_t vtable_rva,
                       std::uint32_t col_rva) noexcept
{
    __try
    {
        auto** const vtable = reinterpret_cast<void**>(image_base + vtable_rva);
        return vtable[-1] == image_base + col_rva;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
}

} // namespace

bool PrepareActiveTargets(
    std::uint8_t* image_base, const ActiveReplacements& replacements,
    std::array<isaac_cs_t0::mode::RelativeCallPatch, kActiveRelativeCount>& relative,
    std::array<isaac_cs_t0::mode::PointerPatch, kActivePointerCount>& pointers,
    TargetFailure& failure) noexcept
{
    failure = {};
    if (!image_base ||
        std::any_of(replacements.relative.begin(), replacements.relative.end(),
                    [](void* value) { return value == nullptr; }) ||
        std::any_of(replacements.pointers.begin(), replacements.pointers.end(),
                    [](void* value) { return value == nullptr; }))
    {
        failure = {TargetFailureStage::InvalidArguments, UINT32_MAX};
        return false;
    }
    __try
    {
        for (std::size_t index = 0; index < relative.size(); ++index)
        {
            auto* const call = image_base + kCallRvas[index];
            if (std::memcmp(call, kCallBytes[index].data(), kCallBytes[index].size()) != 0)
            {
                failure = {TargetFailureStage::RelativeBytes,
                           static_cast<std::uint32_t>(index)};
                return false;
            }
            const auto displacement = *reinterpret_cast<const std::int32_t*>(call + 1);
            if (call + 5 + displacement != image_base + kCallTargetRvas[index])
            {
                failure = {TargetFailureStage::RelativeTarget,
                           static_cast<std::uint32_t>(index)};
                return false;
            }
            auto& patch = relative[index];
            patch.call = call;
            std::copy(kCallBytes[index].begin(), kCallBytes[index].end(), patch.expected);
            patch.replacement = replacements.relative[index];
            patch.original = image_base + kCallTargetRvas[index];
        }
        for (std::size_t index = 0; index < pointers.size(); ++index)
        {
            auto** const slot = reinterpret_cast<void**>(image_base + kPointerRvas[index]);
            void* const expected = image_base + kPointerTargetRvas[index];
            if (*slot != expected)
            {
                failure = {TargetFailureStage::PointerTarget,
                           static_cast<std::uint32_t>(index)};
                return false;
            }
            if (!isaac_cs_t0::mode::IsAddressOwnedBy(*slot, L"isaac-ng.exe"))
            {
                failure = {TargetFailureStage::PointerOwner,
                           static_cast<std::uint32_t>(index)};
                return false;
            }
            pointers[index] = {slot, replacements.pointers[index], expected,
                               L"isaac-ng.exe", false, expected};
        }
        if (!ValidateVtableCol(image_base, kKeyboardVtableRva, kKeyboardColRva))
        {
            failure = {TargetFailureStage::VtableCol, 0};
            return false;
        }
        if (!ValidateVtableCol(image_base, kGamepadVtableRva, kGamepadColRva))
        {
            failure = {TargetFailureStage::VtableCol, 1};
            return false;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        failure = {TargetFailureStage::PointerTarget, UINT32_MAX};
        return false;
    }
    return true;
}

bool VerifyActiveTargetsRestored(
    const std::array<isaac_cs_t0::mode::RelativeCallPatch, kActiveRelativeCount>& relative,
    const std::array<isaac_cs_t0::mode::PointerPatch, kActivePointerCount>& pointers) noexcept
{
    __try
    {
        for (const auto& patch : relative)
        {
            if (patch.installed || !patch.call ||
                std::memcmp(patch.call, patch.expected, sizeof(patch.expected)) != 0)
                return false;
        }
        for (const auto& patch : pointers)
        {
            if (patch.installed || !patch.slot || !patch.expected_original ||
                *patch.slot != patch.expected_original)
                return false;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    return true;
}

} // namespace isaac_cs_t0::active
