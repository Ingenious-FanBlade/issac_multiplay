#include "isaac_cs_t0/active_runtime.hpp"
#include "isaac_cs_t0/boundary_runtime.hpp"
#include "isaac_cs_t0/coop_passive_runtime.hpp"
#include "isaac_cs_t0/coop_active_runtime.hpp"
#include "isaac_cs_t0/input_runtime.hpp"
#include "isaac_cs_t0/loop_runtime.hpp"
#include "isaac_cs_t0/offline_attestation_runtime.hpp"
#include "isaac_cs_t0/runtime.hpp"

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
        isaac_cs_t0::mode::SetModule(instance);
        isaac_cs_t0::boundary::SetModule(instance);
        isaac_cs_t0::input::SetModule(instance);
        isaac_cs_t0::loop::SetModule(instance);
        isaac_cs_t0::offline_attestation::SetModule(instance);
        isaac_cs_t0::active::SetModule(instance);
        isaac_cs_t0::coop_passive::SetModule(instance);
        isaac_cs_t0::coop_active::SetModule(instance);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        isaac_cs_t0::mode::ProcessDetach();
        isaac_cs_t0::boundary::ProcessDetach();
        isaac_cs_t0::input::ProcessDetach();
        isaac_cs_t0::loop::ProcessDetach();
        isaac_cs_t0::offline_attestation::ProcessDetach();
        isaac_cs_t0::active::ProcessDetach();
        isaac_cs_t0::coop_passive::ProcessDetach();
        isaac_cs_t0::coop_active::ProcessDetach();
    }
    return TRUE;
}
