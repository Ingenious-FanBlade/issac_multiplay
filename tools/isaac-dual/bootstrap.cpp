#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstring>

#pragma comment(linker, "/EXPORT:PrepareDualTest=_PrepareDualTest")

namespace {
// 每个测试进程有独立的 profile。原版和 REPENTOGON 共享同一个查询结果。
char profile[MAX_PATH];
DWORD profileLength;

BOOL WINAPI TestProfile(HANDLE, LPSTR buffer, LPDWORD size) {
  if (!size) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  const DWORD required = profileLength + 1;
  if (!buffer || *size < required) {
    *size = required;
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
  }
  std::memcpy(buffer, profile, required);
  *size = required;
  return TRUE;
}

// 主线程仍挂起。仅变更已加载模块的私有页面，随后归还原页权限。
bool ReplaceCode(void* target, const void* bytes, SIZE_T count) {
  DWORD previous;
  if (!VirtualProtect(target, count, PAGE_EXECUTE_READWRITE, &previous)) {
    return false;
  }
  std::memcpy(target, bytes, count);
  DWORD ignored;
  const bool restored = VirtualProtect(target, count, previous, &ignored) != 0;
  const bool flushed = FlushInstructionCache(GetCurrentProcess(), target, count) != 0;
  return restored && flushed;
}

// 原版动态查询该导出；REPENTOGON 在 DLL 静态初始化中直接导入它。
// 先安装子进程内的跳转，随后才让 zhlLoader 加载 REPENTOGON，保留其正常加载顺序。
bool RedirectProfiles() {
  profileLength = GetEnvironmentVariableA("USERPROFILE", profile, sizeof(profile));
  if (!profileLength || profileLength >= sizeof(profile)) return false;

  auto* userenv = LoadLibraryExW(L"userenv.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (!userenv) return false;
  auto* target = GetProcAddress(userenv, "GetUserProfileDirectoryA");
  if (!target) return false;
  unsigned char jump[] = {0xe9, 0, 0, 0, 0};
  const DWORD displacement = reinterpret_cast<DWORD>(&TestProfile) -
                             reinterpret_cast<DWORD>(target) - sizeof(jump);
  std::memcpy(jump + 1, &displacement, sizeof(displacement));
  return ReplaceCode(reinterpret_cast<void*>(target), jump, sizeof(jump));
}

bool InterceptSteamRestart() {
  auto* steam = GetModuleHandleW(L"steam_api.dll");
  if (!steam) return false;
  auto* restart = GetProcAddress(steam, "SteamAPI_RestartAppIfNecessary");
  if (!restart) return false;
  // J273 测试副本的 32 位 cdecl 导出以此三字节开头。只让重启动查询返回 false。
  const unsigned char prologue[] = {0x55, 0x8b, 0xec};
  const unsigned char returnFalse[] = {0x31, 0xc0, 0xc3};
  auto* target = reinterpret_cast<void*>(restart);
  return std::memcmp(target, prologue, sizeof(prologue)) == 0 &&
         ReplaceCode(target, returnFalse, sizeof(returnFalse));
}
}  // namespace

// 返回值由父进程在恢复主线程前读取，失败时仅回收此次启动的子进程。
extern "C" DWORD __cdecl PrepareDualTest(void*) {
  if (!InterceptSteamRestart()) return 1;
  if (!RedirectProfiles()) return 2;
  return 0;
}
