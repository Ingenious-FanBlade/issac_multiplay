#include "HookSystem_private.h"
#include "IsaacRepentance.h"
#include "binding/offsets.h"
#include "hook_capacity.h"
#include "runtime.h"
#include <cctype>
namespace cs {
bool PatternMatches(uintptr_t rva, const char *pattern) noexcept {
  // 检查映像边界及内存可读性后再检查指令形状。
  auto dos = reinterpret_cast<IMAGE_DOS_HEADER *>(Base);
  auto nt = reinterpret_cast<IMAGE_NT_HEADERS *>(Base + dos->e_lfanew);
  const auto *bytes = reinterpret_cast<const unsigned char *>(Base + rva);
  size_t i = 0;
  while (*pattern) {
    if (*pattern == ' ') {
      ++pattern;
      continue;
    }
    if (rva + i >= nt->OptionalHeader.SizeOfImage)
      return false;
    MEMORY_BASIC_INFORMATION memory{};
    if (!VirtualQuery(bytes + i, &memory, sizeof(memory)) ||
        memory.State != MEM_COMMIT ||
        (memory.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
      return false;
    if (*pattern == '?') {
      pattern += 2;
      ++i;
      continue;
    }
    char pair[3]{pattern[0], pattern[1], 0};
    if (bytes[i++] != strtoul(pair, nullptr, 16))
      return false;
    pattern += 2;
  }
  return true;
}
// 逐组解析地址；组内任一点位不符便停用该组，不猜测 ABI。
bool PrepareBindings() {
  Capabilities.fill(true);
  size_t n = 0;
  auto hooks = Hooks(n);
  for (size_t i = 0; i < n; ++i) {
    auto &h = *hooks[i];
    bool ok;
    if (h.own)
      ok = PatternMatches(h.rva, h.pattern);
    else {
      auto *d = FunctionDefinition::Find(h.name, *h.type);
      ok = d && d->CanHook() && uintptr_t(d->GetAddress()) == Base + h.rva;
    }
    if (!ok) {
      Capabilities[size_t(h.group)] = false;
      CapabilityReasons[size_t(h.group)] +=
          std::string(h.id) + ":binding_mismatch;";
      Trace("binding_error", "%s rva=%x", h.id, unsigned(h.rva));
    }
  }
  // 清单容量：生成期已保证行数不超限；运行时如仍出现拒收，必须显式停用
  // 受影响的能力组，不能像旧实现那样静默丢弃。
  size_t rejected_count = 0;
  auto rejected = RejectedHooks(rejected_count);
  for (size_t i = 0; i < rejected_count; ++i) {
    auto &h = *rejected[i];
    Capabilities[size_t(h.group)] = false;
    CapabilityReasons[size_t(h.group)] += std::string(h.id) + ":capacity_exceeded;";
    Trace("hook_capacity_error", "%s group=%s", h.id,
          GroupNames[size_t(h.group)]);
  }
  const auto capacity = PendingHookCapacity();
  if (capacity.rejected) {
    Trace("hook_capacity", "capacity=%u rows=%u used=%u rejected=%u",
          unsigned(capacity.capacity), unsigned(kHookManifestRows),
          unsigned(capacity.used), unsigned(capacity.rejected));
  }
  // 命令桥调用 Print，必须同时确认其定义。
  auto *print = FunctionDefinition::Find(
      "Console::Print", typeid(void (Console::*)(const std_string &,
                                                 unsigned int, unsigned int)));
  if (!print || uintptr_t(print->GetAddress()) != Base + 0x28DC60) {
    Capabilities[size_t(Group::Control)] = false;
    CapabilityReasons[size_t(Group::Control)] +=
        "Console::Print:binding_mismatch;";
  }
  struct Dependency {
    uintptr_t rva;
    const char *pattern;
  };
  const Dependency device[] = {
#include "binding_v1_9_7_12.inc"
  };
  for (auto &d : device)
    if (!PatternMatches(d.rva, d.pattern)) {
      Capabilities[size_t(Group::Device)] = false;
      CapabilityReasons[size_t(Group::Device)] += "native_device_dependency;";
    }
  // ZHL 保留这些指针，地址定义和 ArgData 必须常驻。
  // 自有定义的每个参数按清单的 HookArg 表构造真实 ArgData：栈参数给 word 数，
  // 寄存器参数给寄存器编号（tx-04-08；旧实现一律 NoRegister,1）。
  for (size_t i = 0; i < n; ++i) {
    auto &h = *hooks[i];
    if (!h.own || !Capabilities[size_t(h.group)])
      continue;
    auto *args = new HookSystem::ArgData[h.args]{};
    for (int a = 0; a < h.args; ++a) {
      if (!h.arg_spec) {
        args[a] = {HookSystem::NoRegister{}, 1};
        continue;
      }
      const auto &spec = h.arg_spec[a];
      switch (spec.kind) {
      case HookArg::Kind::GpRegister:
        args[a] = {HookSystem::GPRegisters(spec.reg), 0};
        break;
      case HookArg::Kind::XmmRegister:
        args[a] = {HookSystem::XMMRegisters(spec.reg), 0};
        break;
      case HookArg::Kind::Stack:
      default:
        args[a] = {HookSystem::NoRegister{},
                   static_cast<size_t>(spec.size ? spec.size : 1)};
        break;
      }
    }
    char internal[HookSystem::FUNCTION_INTERNAL_NAME_MAX_LEN]{};
    // REPENTOGON 可能再次初始化定义，Load 总会解引用 outfunc。
    auto *resolved = new void *(reinterpret_cast<void *>(Base + h.rva));
    new FunctionDefinition(h.name, internal, *h.type, *resolved, args, h.args,
                           h.flags, resolved, true);
  }
  for (size_t i = 0; i < Capabilities.size(); ++i)
    Trace("capability", "%s ok=%u", GroupNames[i], Capabilities[i]);
  return Capabilities[size_t(Group::Loop)] &&
         Capabilities[size_t(Group::Control)];
}
// 主线程仅复制菜单状态；暂停菜单的编号已在目标 v1.9.7.12 原生分支复核。
std::string MenuStatus(bool run_active) {
  std::string status = "menu_id=-1\nselected_save=-1\nmenu_game_selection=-1\n";
  if (g_MenuManager)
    status = "menu_id=" + std::to_string(g_MenuManager->_selectedMenuID) +
             "\nselected_save=" +
             std::to_string(g_MenuManager->GetMenuSave()->SelectedSave) +
             "\nmenu_game_selection=" +
             std::to_string(g_MenuManager->GetMenuGame()->SelectedElement) +
             "\n";
  // 非 run 不解引用暂停对象；网络线程只接收 PublishStatus 已复制的字符串。
  auto *game = static_cast<Game *>(GamePointer());
  auto *pause = run_active && game ? game->GetPauseMenu() : nullptr;
  status += "pause_state=" + std::to_string(pause ? pause->state : -1) +
            "\npause_selection=" +
            std::to_string(pause ? pause->selectedelement : -1) + "\n";
  return status;
}
} // namespace cs
