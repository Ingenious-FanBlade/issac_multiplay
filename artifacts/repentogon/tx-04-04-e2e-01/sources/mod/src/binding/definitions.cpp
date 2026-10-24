#include "runtime.h"
#include "HookSystem_private.h"
#include "binding/offsets.h"
#include "IsaacRepentance.h"
#include <cctype>
namespace cs {
bool PatternMatches(uintptr_t rva, const char* pattern) noexcept {
    // Validate readable image bounds before inspecting instruction shapes.
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(Base);
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(Base+dos->e_lfanew);
    const auto* bytes=reinterpret_cast<const unsigned char*>(Base+rva);
    size_t i=0;
    while (*pattern) {
        if (*pattern==' ') { ++pattern; continue; }
        if (rva+i>=nt->OptionalHeader.SizeOfImage) return false;
        MEMORY_BASIC_INFORMATION memory{};
        if (!VirtualQuery(bytes+i,&memory,sizeof(memory)) || memory.State!=MEM_COMMIT ||
            (memory.Protect&(PAGE_NOACCESS|PAGE_GUARD))) return false;
        if (*pattern=='?') { pattern+=2; ++i; continue; }
        char pair[3]{pattern[0],pattern[1],0};
        if (bytes[i++]!=strtoul(pair,nullptr,16)) return false;
        pattern+=2;
    }
    return true;
}
bool PrepareBindings() {
    Capabilities.fill(true);
    size_t n=0; auto hooks=Hooks(n);
    for (size_t i=0;i<n;++i) {
        auto& h=*hooks[i]; bool ok;
        if (h.own) ok=PatternMatches(h.rva,h.pattern);
        else {
            auto* d=FunctionDefinition::Find(h.name,*h.type);
            ok=d && d->CanHook() && uintptr_t(d->GetAddress())==Base+h.rva;
        }
        if (!ok) { Capabilities[size_t(h.group)]=false; CapabilityReasons[size_t(h.group)]+=std::string(h.id)+":binding_mismatch;"; Trace("binding_error","%s rva=%x",h.id,unsigned(h.rva)); }
    }
    // Console::Print is used by the bridge, so its definition must also exist.
    auto* print=FunctionDefinition::Find("Console::Print",typeid(void(Console::*)(const std_string&,unsigned int,unsigned int)));
    if (!print || uintptr_t(print->GetAddress())!=Base+0x28DC60) { Capabilities[size_t(Group::Control)]=false;CapabilityReasons[size_t(Group::Control)]+="Console::Print:binding_mismatch;"; }
    struct Dependency { uintptr_t rva; const char* pattern; };
    const Dependency device[]={
#include "binding_v1_9_7_12.inc"
    };
    for (auto& d:device) if (!PatternMatches(d.rva,d.pattern)) { Capabilities[size_t(Group::Device)]=false;CapabilityReasons[size_t(Group::Device)]+="native_device_dependency;"; }
    // Address definitions and ArgData are permanent: ZHL retains these pointers.
    for (size_t i=0;i<n;++i) {
        auto& h=*hooks[i]; if (!h.own || !Capabilities[size_t(h.group)]) continue;
        auto* args=new HookSystem::ArgData[h.args]{};
        for (int a=0;a<h.args;++a) args[a]={HookSystem::NoRegister{},1};
        char internal[HookSystem::FUNCTION_INTERNAL_NAME_MAX_LEN]{};
        // InitRepentogon may call Definition::Init again after our ModInit.
        // Load() always dereferences outfunc, including for address definitions.
        auto* resolved=new void*(reinterpret_cast<void*>(Base+h.rva));
        new FunctionDefinition(h.name,internal,*h.type,*resolved,args,h.args,h.flags,resolved,true);
    }
    for (size_t i=0;i<Capabilities.size();++i) Trace("capability","%s ok=%u",GroupNames[i],Capabilities[i]);
    return Capabilities[size_t(Group::Loop)] && Capabilities[size_t(Group::Control)];
}
std::string MenuStatus() {
    if(!g_MenuManager) return "menu_id=-1\nselected_save=-1\n";
    return "menu_id="+std::to_string(g_MenuManager->_selectedMenuID)+"\nselected_save="+
        std::to_string(g_MenuManager->GetMenuSave()->SelectedSave)+"\nmenu_game_selection="+
        std::to_string(g_MenuManager->GetMenuGame()->SelectedElement)+"\n";
}
}
