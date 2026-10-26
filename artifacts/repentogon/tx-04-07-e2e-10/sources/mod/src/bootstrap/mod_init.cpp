#include "binding/offsets.h"
#include "libzhl.h"
#include "runtime.h"
namespace cs {
DWORD MainThread = 0;
uintptr_t Base = 0;
std::array<bool, size_t(Group::Count)> Capabilities{};
std::array<std::string, size_t(Group::Count)> CapabilityReasons{};
std::string RepentogonVersion = "unknown";
Config Settings;
bool IsMain() noexcept { return GetCurrentThreadId() == MainThread; }
void RefreshVersion() noexcept {
  try {
    if (auto mod = GetModuleHandleW(L"zhlREPENTOGON.dll")) {
      auto version = reinterpret_cast<const char **>(
          GetProcAddress(mod, "__REPENTOGON_VERSION"));
      if (version && *version)
        RepentogonVersion = *version;
    }
  } catch (...) {
    Trace("version_error", "exception");
  }
}
void *GamePointer() noexcept {
  return Base ? *reinterpret_cast<void **>(Base + offset::Game) : nullptr;
}
} // namespace cs
BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved) {
  // 加载阶段只关闭线程通知；进程终止时只用 Win32 句柄尽力补写 trace。
  if (reason == DLL_PROCESS_ATTACH)
    DisableThreadLibraryCalls(module);
  if (reason == DLL_PROCESS_DETACH && reserved)
    cs::FinishTraceOnProcessExit();
  return TRUE;
}
// 先读取配置与能力组，再注册常驻 Hook；任何初始化失败都返回 0。
MOD_EXPORT int ModInit() noexcept {
  using namespace cs;
  try {
    MainThread = GetCurrentThreadId();
    Base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    Settings = ReadConfig();
    StartTrace(Settings.trace_dir);
    RefreshVersion();
    auto dos = reinterpret_cast<IMAGE_DOS_HEADER *>(Base);
    auto nt = reinterpret_cast<IMAGE_NT_HEADERS *>(Base + dos->e_lfanew);
    Trace("boot",
          "version=%s timestamp=%08X base=%p repentogon=%s thread=%lu "
          "enabled=%u registered=0 "
          "pointers=0 reason=%s",
          Version, nt->FileHeader.TimeDateStamp, reinterpret_cast<void *>(Base),
          RepentogonVersion.c_str(), MainThread, Settings.enabled,
          Settings.reason.c_str());
    if (!Settings.enabled) {
      FinishTrace();
      return 0;
    }
    Trace("config",
          "schema=1 enabled=1 control=%u port=%u host_port=%u log_level=%s",
          Settings.control, Settings.control_port, Settings.host_port,
          Settings.log_level.c_str());
    PrepareBindings();
    RegisterHooks();
    PublishStatus();
    if (Settings.control && Capabilities[size_t(Group::Loop)] &&
        Capabilities[size_t(Group::Control)])
      StartControlPort();
  } catch (const std::exception &e) {
    Capabilities.fill(false);
    Trace("bootstrap_error", "%s", e.what());
  } catch (...) {
    Capabilities.fill(false);
    Trace("bootstrap_error", "unknown_exception");
  }
  // LoadMods 收到非零会中止加载，本 DLL 的失败不能中止 REPENTOGON。
  return 0;
}
