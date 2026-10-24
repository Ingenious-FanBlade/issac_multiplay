#if defined(ISAAC_CS_WORLD_E2E)
struct AuditFrame {
    std::array<std::uint32_t,9> entry{};
    std::array<std::uint8_t,512> fx{};
    std::uint32_t originals=0;
    bool first=false;
};
thread_local std::array<AuditFrame,64> g_audit{};
std::array<bool,19> g_audit_sampled{};
volatile LONG g_fault_workers=0;
bool SameFx(const std::uint8_t* a,const std::uint8_t* b) noexcept
{
    // FXSAVE reserved bytes are not architectural state and may be unwritten.
    if(std::memcmp(a,b,32) || std::memcmp(a+160,b+160,128))return false;
    for(std::uint32_t i=0;i<8;++i)if(std::memcmp(a+32+i*16,b+32+i*16,10))return false;
    return true;
}
extern "C" void __cdecl WorldAuditOriginal(std::uint32_t hook,std::uint32_t* regs,std::uint8_t* fx) noexcept
{
    if(!g_depth || g_frames[g_depth-1].hook!=hook)return;
    auto& frame=g_frames[g_depth-1];auto& audit=g_audit[g_depth-1];
    std::uint32_t errors=0;
    for(std::uint32_t i=0;i<9;++i)if(i!=3 && regs[i]!=audit.entry[i])errors|=1u<<i;
    const auto stack=Address(regs+9);
    if(frame.stack-stack!=704 || ((frame.stack^stack)&15u))errors|=1u<<9;
    for(std::uint32_t i=0;i<kArgumentWords[hook];++i)if(regs[10+i]!=frame.args[i])errors|=1u<<10;
    if(!SameFx(audit.fx.data(),fx))errors|=1u<<11;
    if(++audit.originals!=1)errors|=1u<<12;
    std::uint32_t words[14]{};std::memcpy(words,regs,9*4);
    words[9]=errors;words[10]=stack;words[11]=frame.stack;words[12]=kArgumentWords[hook];words[13]=audit.originals;
    e2e::Record(e2e::Kind::Original,hook,words,14);
    e2e::Record(e2e::Kind::Arguments,hook,regs+10,kArgumentWords[hook],1);
    if(audit.first)e2e::Fx(hook,1,fx);
}
DWORD WINAPI WrongThreadFault(void*) noexcept
{
    const auto context=loop::CurrentObservationContext();
    const bool accepted=MainThread(); // Stops before game-memory access; exercises the real thread check.
    e2e::Worker(context.main_thread_id,context.thread_id,accepted);
    InterlockedDecrement(&g_fault_workers);return 0;
}

extern "C" __declspec(naked) void WorldBridge0()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 0
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+0*4]
    }
}

extern "C" __declspec(naked) void WorldBridge1()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 1
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+1*4]
    }
}

extern "C" __declspec(naked) void WorldBridge2()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 2
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+2*4]
    }
}

extern "C" __declspec(naked) void WorldBridge3()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 3
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+3*4]
    }
}

extern "C" __declspec(naked) void WorldBridge4()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 4
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+4*4]
    }
}

extern "C" __declspec(naked) void WorldBridge5()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 5
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+5*4]
    }
}

extern "C" __declspec(naked) void WorldBridge6()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 6
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+6*4]
    }
}

extern "C" __declspec(naked) void WorldBridge7()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 7
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+7*4]
    }
}

extern "C" __declspec(naked) void WorldBridge8()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 8
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+8*4]
    }
}

extern "C" __declspec(naked) void WorldBridge9()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 9
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+9*4]
    }
}

extern "C" __declspec(naked) void WorldBridge10()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 10
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+10*4]
    }
}

extern "C" __declspec(naked) void WorldBridge11()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 11
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+11*4]
    }
}

extern "C" __declspec(naked) void WorldBridge12()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 12
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+12*4]
    }
}

extern "C" __declspec(naked) void WorldBridge13()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 13
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+13*4]
    }
}

extern "C" __declspec(naked) void WorldBridge14()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 14
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+14*4]
    }
}

extern "C" __declspec(naked) void WorldBridge15()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 15
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+15*4]
    }
}

extern "C" __declspec(naked) void WorldBridge16()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 16
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+16*4]
    }
}

extern "C" __declspec(naked) void WorldBridge17()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 17
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+17*4]
    }
}

extern "C" __declspec(naked) void WorldBridge18()
{
    __asm {
        pushfd
        pushad
        sub esp,544
        lea eax,[esp+15]
        and eax,0FFFFFFF0h
        fxsave [eax]
        mov [esp+528],eax
        lea eax,[esp+544]
        push dword ptr [esp+528]
        push eax
        push 18
        call WorldAuditOriginal
        add esp,12
        mov eax,[esp+528]
        fxrstor [eax]
        lea esp,[esp+544]
        popad
        popfd
        jmp dword ptr [g_world_trampolines+18*4]
    }
}
extern "C" {void* g_world_calls[19]{reinterpret_cast<void*>(WorldBridge0),reinterpret_cast<void*>(WorldBridge1),reinterpret_cast<void*>(WorldBridge2),reinterpret_cast<void*>(WorldBridge3),reinterpret_cast<void*>(WorldBridge4),reinterpret_cast<void*>(WorldBridge5),reinterpret_cast<void*>(WorldBridge6),reinterpret_cast<void*>(WorldBridge7),reinterpret_cast<void*>(WorldBridge8),reinterpret_cast<void*>(WorldBridge9),reinterpret_cast<void*>(WorldBridge10),reinterpret_cast<void*>(WorldBridge11),reinterpret_cast<void*>(WorldBridge12),reinterpret_cast<void*>(WorldBridge13),reinterpret_cast<void*>(WorldBridge14),reinterpret_cast<void*>(WorldBridge15),reinterpret_cast<void*>(WorldBridge16),reinterpret_cast<void*>(WorldBridge17),reinterpret_cast<void*>(WorldBridge18)};}
#else
#define g_world_calls g_world_trampolines
#endif
