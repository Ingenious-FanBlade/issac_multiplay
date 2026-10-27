#pragma once
#include "HookSystem.h"
#include "runtime.h"
#include "world/observation.h"

// 静态初始化只填本 DLL 的待注册表；ModInit 判定能力组后再调用 ZHL。
// wrapper 保持异常中立，原函数异常必须继续交给原生上层处理。
#define CS_HOOK_COMMON(id, group, def, cls, method, pri, rv, pat, own, nargs,  \
                       flags, owner, conv, sig, ret, stat, typ)                \
  namespace H_##id {                                                           \
    static void *internalSuper = nullptr;                                      \
    struct Wrapper : cls {                                                     \
      stat auto conv hook sig->ret;                                            \
      stat auto conv super sig->ret;                                           \
    };                                                                         \
    static cs::Hook record{#id,                                                \
                           cs::Group::group,                                   \
                           def,                                                \
                           &typeid(typ),                                       \
                           nullptr,                                            \
                           &internalSuper,                                     \
                           rv,                                                 \
                           pat,                                                \
                           own,                                                \
                           nargs,                                              \
                           flags,                                              \
                           pri,                                                \
                           cs::Owner::owner};                                  \
    struct Pending {                                                           \
      Pending() noexcept {                                                     \
        auto member = &Wrapper::hook;                                          \
        static_assert(sizeof(member) == sizeof(void *));                       \
        memcpy(&record.entry, &member, sizeof(void *));                        \
        cs::AddPending(&record);                                               \
      }                                                                        \
    };                                                                         \
    static Pending pending;                                                    \
    /* super 只做一次 jmp，形参不在本体内引用；用 __pragma 精确抑制这一处 */    \
    __pragma(warning(suppress : 4100))                                         \
    auto __declspec(naked) conv Wrapper::super sig->ret {                      \
      __asm jmp internalSuper                                                  \
    }                                                                          \
  }                                                                            \
  auto conv H_##id::Wrapper::hook sig->ret

#define CS_HOOK_METHOD(id, group, def, cls, method, pri, rv, pat, own, nargs,  \
                       flags, owner, conv, sig, ret)                           \
  CS_HOOK_COMMON(id, group, def, cls, method, pri, rv, pat, own, nargs, flags, \
                 owner, conv, sig, ret, , auto(cls::*) sig->ret)
#define CS_HOOK_STATIC(id, group, def, cls, method, pri, rv, pat, own, nargs,  \
                       flags, owner, conv, sig, ret)                           \
  CS_HOOK_COMMON(id, group, def, cls, method, pri, rv, pat, own, nargs, flags, \
                 owner, conv, sig, ret, static, auto(*) sig->ret)

// tx-04-08：world 观察入口（迁移设计 4.1/4.2、失败方式 F4）。
//
// 与 CS_HOOK_* 的区别：
//   1. group 固定 World，记录里带 Observation 编号、ZHL 定义名与 arg_spec，
//      供能力组、清单与状态报告使用；owner 不进入会话 owner 栈；
//   2. 不使用 cs::CallScope：非主线程不调用 game::ForeignCall、原生异常不
//      调用 game::ContextFault，改由 cs::world::ObservationScope 只记 world
//      自有故障（主线程桥在下一个收束点读取并收口）；
//   3. 自有定义的 ArgData 来自显式 HookArg 表，可表达 ECX/EDX 等寄存器参数，
//      不再按逗号数假定“每参一个栈 word”。
#define CS_WORLD_HOOK_COMMON(id, def, cls, method, pri, rv, pat, own, nargs,   \
                             argspec, flags, obs, conv, sig, ret, stat, typ)   \
  namespace HW_##id {                                                         \
    static void *internalSuper = nullptr;                                     \
    struct Wrapper : cls {                                                    \
      stat auto conv hook sig->ret;                                           \
      stat auto conv super sig->ret;                                          \
    };                                                                        \
    static cs::Hook record{#id,                                               \
                           cs::Group::World,                                  \
                           def,                                               \
                           &typeid(typ),                                      \
                           nullptr,                                           \
                           &internalSuper,                                    \
                           rv,                                                \
                           pat,                                               \
                           own,                                               \
                           nargs,                                             \
                           flags,                                             \
                           pri,                                               \
                           cs::Owner::None,                                   \
                           argspec,                                           \
                           static_cast<std::uint32_t>(                        \
                               cs::world::Observation::obs),                  \
                           true};                                             \
    struct Pending {                                                          \
      Pending() noexcept {                                                    \
        auto member = &Wrapper::hook;                                         \
        static_assert(sizeof(member) == sizeof(void *));                      \
        memcpy(&record.entry, &member, sizeof(void *));                       \
        cs::AddPending(&record);                                              \
      }                                                                       \
    };                                                                        \
    static Pending pending;                                                   \
    /* super 只做一次 jmp，形参不在本体内引用；用 __pragma 精确抑制这一处 */   \
    __pragma(warning(suppress : 4100))                                        \
    auto __declspec(naked) conv Wrapper::super sig->ret {                     \
      __asm jmp internalSuper                                                 \
    }                                                                         \
  }                                                                           \
  auto conv HW_##id::Wrapper::hook sig->ret

#define CS_WORLD_HOOK_METHOD(id, def, cls, method, pri, rv, pat, own, nargs,   \
                             argspec, flags, obs, conv, sig, ret)              \
  CS_WORLD_HOOK_COMMON(id, def, cls, method, pri, rv, pat, own, nargs, argspec, \
                       flags, obs, conv, sig, ret, , auto(cls::*) sig->ret)
#define CS_WORLD_HOOK_STATIC(id, def, cls, method, pri, rv, pat, own, nargs,   \
                             argspec, flags, obs, conv, sig, ret)              \
  CS_WORLD_HOOK_COMMON(id, def, cls, method, pri, rv, pat, own, nargs, argspec, \
                       flags, obs, conv, sig, ret, static, auto(*) sig->ret)
