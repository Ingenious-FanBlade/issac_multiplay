#pragma once
#include <Windows.h>
namespace isaac_cs_t0::t09 {
// Loaded resident into the B game process. The exported worker owns only a
// Remote protocol::Endpoint and Wire IPC; it never touches Player/Lua/world.
extern "C" DWORD WINAPI IsaacCsT0T09RemoteInitialize(const wchar_t* wire_mapping) noexcept;
extern "C" DWORD WINAPI IsaacCsT0T09RemoteShutdown(void*) noexcept;
}
