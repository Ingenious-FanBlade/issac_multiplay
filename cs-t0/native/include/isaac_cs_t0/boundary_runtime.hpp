#pragma once

#include <Windows.h>

namespace isaac_cs_t0::boundary {

void SetModule(HMODULE module) noexcept;
void ProcessDetach() noexcept;

extern "C" DWORD WINAPI IsaacCsT0BoundaryInitialize(const wchar_t* mapping_name);
extern "C" DWORD WINAPI IsaacCsT0BoundaryShutdown(void* unused);

} // namespace isaac_cs_t0::boundary
