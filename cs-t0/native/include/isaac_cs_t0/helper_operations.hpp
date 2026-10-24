#pragma once

#include <array>
#include <cwchar>

namespace isaac_cs_t0::helper {

struct Operation {
    const wchar_t* argument;
    bool inject;
    const char* entry;
    bool resident = false;
};

inline constexpr std::array<Operation, 18> kOperations{{
    {L"--inject", true, "IsaacCsT0Initialize"},
    {L"--unload", false, "IsaacCsT0Shutdown"},
    {L"--inject-boundary", true, "IsaacCsT0BoundaryInitialize"},
    {L"--unload-boundary", false, "IsaacCsT0BoundaryShutdown"},
    {L"--inject-input", true, "IsaacCsT0InputInitialize"},
    {L"--unload-input", false, "IsaacCsT0InputShutdown"},
    {L"--inject-loop", true, "IsaacCsT0LoopInitialize"},
    {L"--unload-loop", false, "IsaacCsT0LoopShutdown"},
    {L"--inject-offline-attestation", true,
     "IsaacCsT0OfflineAttestationInitialize"},
    {L"--unload-offline-attestation", false,
     "IsaacCsT0OfflineAttestationShutdown"},
    {L"--inject-input-active", true, "IsaacCsT0InputActiveInitialize"},
    {L"--unload-input-active", false, "IsaacCsT0InputActiveShutdown"},
    {L"--inject-coop-passive", true, "IsaacCsT0CoopPassiveInitialize"},
    {L"--unload-coop-passive", false, "IsaacCsT0CoopPassiveShutdown"},
    {L"--inject-coop-active", true, "IsaacCsT0CoopActiveInitialize", true},
    {L"--stop-coop-active", false, "IsaacCsT0CoopActiveShutdown", true},
    {L"--inject-t09-remote", true, "IsaacCsT0T09RemoteInitialize", true},
    {L"--stop-t09-remote", false, "IsaacCsT0T09RemoteShutdown", true},
}};

inline const Operation* FindOperation(const wchar_t* argument) noexcept
{
    if (!argument)
        return nullptr;
    for (const auto& operation : kOperations)
        if (_wcsicmp(argument, operation.argument) == 0)
            return &operation;
    return nullptr;
}

} // namespace isaac_cs_t0::helper
