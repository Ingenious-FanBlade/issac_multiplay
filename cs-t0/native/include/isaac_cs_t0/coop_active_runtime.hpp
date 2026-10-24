#pragma once
#include <Windows.h>
#include <cstdint>
namespace isaac_cs_t0::coop_active {
#if defined(ISAAC_CS_WORLD_E2E)
// Main-thread caller-owned output. Copies only existing companion state.
void CopyWorldE2EWindowFacts(std::uint32_t* six_words) noexcept;
#endif
void SetModule(HMODULE module) noexcept;
void ProcessDetach() noexcept;
extern "C" DWORD WINAPI IsaacCsT0CoopActiveInitialize(const wchar_t* mapping);
extern "C" DWORD WINAPI IsaacCsT0CoopActiveShutdown(void*);
}
