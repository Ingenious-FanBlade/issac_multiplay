#pragma once

#include <Windows.h>

namespace isaac_cs_t0::mode {

void SetModule(HMODULE module) noexcept;
void ProcessDetach() noexcept;

extern "C" DWORD WINAPI IsaacCsT0Initialize(const wchar_t* mapping_name);
extern "C" DWORD WINAPI IsaacCsT0Shutdown(void* unused);

} // namespace isaac_cs_t0::mode
