#pragma once

#include <Windows.h>

#include <cstdint>

namespace isaac_cs_t0::input {

namespace detail {

enum class EmbeddedShutdownResult : std::uint32_t {
    Complete = 0,
    InputRestoreFailed = 1,
    InputDrainFailed = 2,
    LoopStopFailed = 3,
};

inline std::uint32_t BoundedTargetRva(const void* image_base, std::uint32_t image_size,
                                      const void* address) noexcept
{
    const auto base = reinterpret_cast<std::uintptr_t>(image_base);
    const auto value = reinterpret_cast<std::uintptr_t>(address);
    if (!base || !value || value < base)
        return 0;
    const auto difference = value - base;
    return difference != 0 && difference < image_size
               ? static_cast<std::uint32_t>(difference)
               : 0;
}

// Kept as a small shared policy primitive so the harness exercises the exact fail-stop ordering
// used by InputShutdown.  A failed phase never invokes any later phase.
template <typename RestoreInput, typename DrainInput, typename StopLoop>
inline EmbeddedShutdownResult RunEmbeddedShutdownStages(
    RestoreInput&& restore_input, DrainInput&& drain_input, StopLoop&& stop_loop) noexcept
{
    if (!restore_input())
        return EmbeddedShutdownResult::InputRestoreFailed;
    if (!drain_input())
        return EmbeddedShutdownResult::InputDrainFailed;
    if (!stop_loop())
        return EmbeddedShutdownResult::LoopStopFailed;
    return EmbeddedShutdownResult::Complete;
}

} // namespace detail

void SetModule(HMODULE module) noexcept;
void ProcessDetach() noexcept;

extern "C" DWORD WINAPI IsaacCsT0InputInitialize(const wchar_t* mapping_name);
extern "C" DWORD WINAPI IsaacCsT0InputShutdown(void* unused);

} // namespace isaac_cs_t0::input
