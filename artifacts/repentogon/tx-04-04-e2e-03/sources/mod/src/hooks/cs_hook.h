#pragma once
#include "HookSystem.h"
#include "runtime.h"

// 静态初始化只填本 DLL 的待注册表；ModInit 判定能力组后再调用 ZHL。
// wrapper 保持异常中立，原函数异常必须继续交给原生上层处理。
#define CS_HOOK_COMMON(id, group, def, cls, method, pri, rv, pat, own, nargs, flags, owner, conv,  \
                       sig, ret, stat, typ)                                                        \
  namespace H_##id {                                                                               \
    static void *internalSuper = nullptr;                                                          \
    struct Wrapper : cls {                                                                         \
      stat auto conv hook sig->ret;                                                                \
      stat auto conv super sig->ret;                                                               \
    };                                                                                             \
    static cs::Hook record{#id,                                                                    \
                           cs::Group::group,                                                       \
                           def,                                                                    \
                           &typeid(typ),                                                           \
                           nullptr,                                                                \
                           &internalSuper,                                                         \
                           rv,                                                                     \
                           pat,                                                                    \
                           own,                                                                    \
                           nargs,                                                                  \
                           flags,                                                                  \
                           pri,                                                                    \
                           cs::Owner::owner};                                                      \
    struct Pending {                                                                               \
      Pending() noexcept {                                                                         \
        auto member = &Wrapper::hook;                                                              \
        static_assert(sizeof(member) == sizeof(void *));                                           \
        memcpy(&record.entry, &member, sizeof(void *));                                            \
        cs::AddPending(&record);                                                                   \
      }                                                                                            \
    };                                                                                             \
    static Pending pending;                                                                        \
    auto __declspec(naked) conv Wrapper::super sig->ret {                                          \
      __asm jmp internalSuper                                                                      \
    }                                                                                              \
  }                                                                                                \
  auto conv H_##id::Wrapper::hook sig->ret

#define CS_HOOK_METHOD(id, group, def, cls, method, pri, rv, pat, own, nargs, flags, owner, conv,  \
                       sig, ret)                                                                   \
  CS_HOOK_COMMON(id, group, def, cls, method, pri, rv, pat, own, nargs, flags, owner, conv, sig,   \
                 ret, , auto(cls::*) sig->ret)
#define CS_HOOK_STATIC(id, group, def, cls, method, pri, rv, pat, own, nargs, flags, owner, conv,  \
                       sig, ret)                                                                   \
  CS_HOOK_COMMON(id, group, def, cls, method, pri, rv, pat, own, nargs, flags, owner, conv, sig,   \
                 ret, static, auto(*) sig->ret)
