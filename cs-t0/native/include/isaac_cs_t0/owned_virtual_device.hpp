#pragma once
#include <Windows.h>
#include "isaac_cs_t0/coop_input.hpp"

namespace isaac_cs_t0::coop_active {
enum class DeviceState : LONG { Empty, RawAllocated, BaseConstructed, Initialized,
    ControlReady, Registered, Retiring, Unregistered };
using OwnedRead = float (*)(void*,std::uint32_t,Query) noexcept;
struct DevicePair { void* object = nullptr; void* control = nullptr; };
static_assert(sizeof(DevicePair) == 8);

class OwnedVirtualDevice final {
public:
    bool Create(std::uint8_t* image, HMODULE module, DWORD main_thread,
                OwnedRead reader, void* context) noexcept;
    bool Register() noexcept;
    void Neutralize() noexcept;
    bool Remove() noexcept;
    bool ExactMember() const noexcept;
    bool Healthy() const noexcept;
    bool Registered() const noexcept { return state_ == DeviceState::Registered; }
    bool Detached() const noexcept { return state_ == DeviceState::Unregistered; }
    DeviceState State() const noexcept { return state_; }
    const DevicePair& Pair() const noexcept { return pair_; }
    std::int32_t Key() const noexcept { return key_; }
    std::uint32_t Error() const noexcept { return error_; }
    bool Pinned() const noexcept { return pinned_; }
    std::uint32_t Strong() const noexcept;
private:
    bool MemberCount(std::uint32_t& exact, std::uint32_t& same_key) const noexcept;
    std::uint8_t* image_ = nullptr;
    DWORD main_thread_ = 0;
    DevicePair pair_{};
    DeviceState state_ = DeviceState::Empty;
    std::int32_t key_ = -1;
    std::uint32_t error_ = 0;
    bool attempted_ = false, removal_attempted_ = false, pinned_ = false;
};
} // namespace isaac_cs_t0::coop_active
