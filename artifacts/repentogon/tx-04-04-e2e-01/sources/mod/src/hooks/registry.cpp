#include "runtime.h"
#include "HookSystem.h"
namespace cs {
namespace { Hook* pending[64]{}; size_t used=0; }
void AddPending(Hook* h) noexcept { if (used<64) pending[used++]=h; }
Hook* const* Hooks(size_t& n) noexcept { n=used; return pending; }
void RegisterHooks() {
    // The ZHL constructor is the public registration API in tag 1.1.3. It
    // copies metadata; these process-lifetime objects must never be deleted.
    for (size_t i=0;i<used;++i) {
        auto& h=*pending[i]; if (!Capabilities[size_t(h.group)]) continue;
        new FunctionHook(h.name,*h.type,h.entry,h.super,h.priority);
        h.registered=true;
        Trace("hook_registered","%s group=%s priority=%d",h.id,GroupNames[size_t(h.group)],h.priority);
    }
}
}
