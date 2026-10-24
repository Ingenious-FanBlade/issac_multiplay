#pragma once
#include "runtime.h"
#include "HookSystem.h"

// Only our pending table is touched during static initialization. The public
// FunctionHook constructor is called later by ModInit, after group resolution.
#define CS_HOOK_COMMON(id, group, def, cls, method, pri, rv, pat, own, nargs, flags, owner, conv, sig, ret, stat, typ) \
 namespace H_##id { \
 static void* internalSuper = nullptr; \
 struct Wrapper : cls { stat auto conv hook sig noexcept -> ret; stat auto conv super sig -> ret; }; \
 static cs::Hook record{#id,cs::Group::group,def,&typeid(typ),nullptr,&internalSuper,rv,pat,own,nargs,flags,pri,cs::Owner::owner}; \
 struct Pending { Pending() noexcept { auto member=&Wrapper::hook; static_assert(sizeof(member)==sizeof(void*)); \
     memcpy(&record.entry,&member,sizeof(void*)); cs::AddPending(&record); } }; \
 static Pending pending; \
 auto __declspec(naked) conv Wrapper::super sig -> ret { __asm jmp internalSuper } \
 } \
 auto conv H_##id::Wrapper::hook sig noexcept -> ret

#define CS_HOOK_METHOD(id,group,def,cls,method,pri,rv,pat,own,nargs,flags,owner,conv,sig,ret) \
 CS_HOOK_COMMON(id,group,def,cls,method,pri,rv,pat,own,nargs,flags,owner,conv,sig,ret,,auto (cls::*) sig -> ret)
#define CS_HOOK_STATIC(id,group,def,cls,method,pri,rv,pat,own,nargs,flags,owner,conv,sig,ret) \
 CS_HOOK_COMMON(id,group,def,cls,method,pri,rv,pat,own,nargs,flags,owner,conv,sig,ret,static,auto (*) sig -> ret)
