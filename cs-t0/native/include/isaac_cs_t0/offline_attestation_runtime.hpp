#pragma once

#include <Windows.h>

namespace isaac_cs_t0::offline_attestation {

void SetModule(HMODULE module) noexcept;
void ProcessDetach() noexcept;
extern "C" DWORD WINAPI IsaacCsT0OfflineAttestationInitialize(const wchar_t* mapping_name);
extern "C" DWORD WINAPI IsaacCsT0OfflineAttestationShutdown(void* unused);

} // namespace isaac_cs_t0::offline_attestation
