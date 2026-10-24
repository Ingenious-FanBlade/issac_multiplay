#include "isaac_cs_t0/t09_wire_ipc.hpp"
#include <algorithm>
#include <cstring>
#include <limits>

namespace isaac_cs_t0::t09 {
namespace {
std::size_t RingIndex(Direction direction) noexcept {return static_cast<std::size_t>(direction);}
bool DirectionValid(Direction direction) noexcept {
    return direction==Direction::ToAuthority || direction==Direction::ToRemote;
}
bool AllZero(const std::uint8_t* bytes,std::size_t size) noexcept {
    return std::all_of(bytes,bytes+size,[](std::uint8_t value){return value==0;});
}
}
OpenResult WireIpc::Open(const wchar_t* name,const OpenConfig& expected) noexcept {
    if(!name || !*name)return OpenResult::Name;
    mapping_=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name);
    if(!mapping_)return OpenResult::Mapping;
    view_=static_cast<std::uint8_t*>(MapViewOfFile(mapping_,FILE_MAP_ALL_ACCESS,0,0,kWireMappingSize));
    if(!view_) {CloseHandle(mapping_);mapping_=nullptr;return OpenResult::View;}
    MEMORY_BASIC_INFORMATION region{};
    if(!VirtualQuery(view_,&region,sizeof(region)) || region.RegionSize<kWireMappingSize) {
        Close();return OpenResult::View;
    }
    header_=HeaderAtBase();
    if(header_->magic!=kWireMagic || header_->version!=kWireVersion ||
       header_->header_size!=kWireHeaderSize || header_->mapping_size!=kWireMappingSize) {
        Close();return OpenResult::Header;
    }
    role_=header_->role;
    if(role_!=1 && role_!=2) {Close();return OpenResult::Header;}
    if(header_->role!=expected.role ||
       std::memcmp(header_->run_nonce,expected.run_nonce.data(),16) ||
       std::memcmp(header_->session,expected.session.data(),16) ||
       std::memcmp(header_->authority_nonce,expected.authority_nonce.data(),16) ||
       std::memcmp(header_->remote_nonce,expected.remote_nonce.data(),16) ||
       header_->clients[0]!=expected.clients[0] || header_->clients[1]!=expected.clients[1]) {
        Close();return OpenResult::Identity;
    }
    if(std::memcmp(header_->build,expected.build.data(),32) ||
       header_->input_schema!=expected.input_schema || header_->state_schema!=expected.state_schema ||
       header_->wire_version!=expected.wire_version || header_->identity_reserved!=0) {
        Close();return OpenResult::Build;
    }
    if(!AllZero(header_->reserved,sizeof(header_->reserved))) {Close();return OpenResult::Reserved;}
    return OpenResult::Accepted;
}
OpenResult WireIpc::OpenForRole(const wchar_t* name,std::uint32_t expected_role,
    OpenConfig& discovered) noexcept {
    if(!name || !*name)return OpenResult::Name;
    mapping_=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name);
    if(!mapping_)return OpenResult::Mapping;
    view_=static_cast<std::uint8_t*>(MapViewOfFile(mapping_,FILE_MAP_ALL_ACCESS,0,0,kWireMappingSize));
    if(!view_) {CloseHandle(mapping_);mapping_=nullptr;return OpenResult::View;}
    MEMORY_BASIC_INFORMATION region{};
    if(!VirtualQuery(view_,&region,sizeof(region)) || region.RegionSize<kWireMappingSize) {
        Close();return OpenResult::View;
    }
    header_=HeaderAtBase();
    if(header_->magic!=kWireMagic || header_->version!=kWireVersion ||
       header_->header_size!=kWireHeaderSize || header_->mapping_size!=kWireMappingSize ||
       header_->role!=expected_role || (header_->role!=1 && header_->role!=2) ||
       header_->input_schema!=2 || header_->state_schema!=1 || header_->wire_version!=2 ||
       header_->identity_reserved!=0 || !AllZero(header_->reserved,sizeof(header_->reserved))) {
        Close();return OpenResult::Header;
    }
    role_=header_->role;
    std::memcpy(discovered.run_nonce.data(),header_->run_nonce,16);
    std::memcpy(discovered.session.data(),header_->session,16);
    std::memcpy(discovered.authority_nonce.data(),header_->authority_nonce,16);
    std::memcpy(discovered.remote_nonce.data(),header_->remote_nonce,16);
    discovered.clients={header_->clients[0],header_->clients[1]};
    std::memcpy(discovered.build.data(),header_->build,32);
    discovered.role=header_->role;
    discovered.input_schema=header_->input_schema;
    discovered.state_schema=header_->state_schema;
    discovered.wire_version=header_->wire_version;
    const auto nonzero=[](const auto& value){return std::any_of(value.begin(),value.end(),
        [](std::uint8_t byte){return byte!=0;});};
    bool build_tail=false,build_nonempty=false,build_valid=true;
    for(auto value:discovered.build) {
        if(!value){build_tail=true;continue;}
        if(build_tail||value<33||value>126){build_valid=false;break;}
        build_nonempty=true;
    }
    if(!nonzero(discovered.run_nonce) || !nonzero(discovered.session) ||
       !nonzero(discovered.authority_nonce) || !nonzero(discovered.remote_nonce) ||
       discovered.authority_nonce==discovered.remote_nonce ||
       !discovered.clients[0] || !discovered.clients[1] ||
       discovered.clients[0]==discovered.clients[1] || !build_valid || !build_nonempty) {
        Close();return OpenResult::Identity;
    }
    return OpenResult::Accepted;
}
void WireIpc::Close() noexcept {
    header_=nullptr;role_=0;
    if(view_) {UnmapViewOfFile(view_);view_=nullptr;}
    if(mapping_) {CloseHandle(mapping_);mapping_=nullptr;}
}
WireSlot* WireIpc::Slot(Direction direction,std::size_t index) noexcept {
    auto* base=view_+kWireHeaderSize+
        (RingIndex(direction)*kWireRingCapacity+index)*kWireSlotSize;
    return reinterpret_cast<WireSlot*>(base);
}
std::uint32_t* WireIpc::ErrorTarget(bool controller) noexcept {
    if(!header_)return nullptr;
    return controller?reinterpret_cast<std::uint32_t*>(const_cast<LONG*>(&header_->controller_error)):
        reinterpret_cast<std::uint32_t*>(const_cast<LONG*>(&header_->driver_error));
}
bool WireIpc::Write(Direction direction,std::span<const std::uint8_t> packet,
    std::uint64_t generation,std::uint64_t tick,bool controller_writer) noexcept {
    if(!header_ || !packet.data() || packet.size()<64 || packet.size()>1200 ||
       !DirectionValid(direction) || direction!=Outbound(role_)) return false;
    if(InterlockedCompareExchange(const_cast<LONG*>(&header_->closed),0,0))return false;
    const auto index=RingIndex(direction);
    const auto produced=static_cast<std::uint64_t>(InterlockedCompareExchange64(&header_->produced[index],0,0));
    const auto consumed=static_cast<std::uint64_t>(InterlockedCompareExchange64(&header_->consumed[index],0,0));
    if(produced<consumed || produced-consumed>=kWireRingCapacity ||
       produced==(std::numeric_limits<std::uint64_t>::max)()) {
        InterlockedIncrement64(&header_->overwritten[index]);
        auto* target=ErrorTarget(controller_writer);
        if(target)InterlockedExchange(reinterpret_cast<volatile LONG*>(target),0xE101);
        MarkClosed();return false;
    }
    const auto token=produced+1;
    auto* slot=Slot(direction,static_cast<std::size_t>((token-1)%kWireRingCapacity));
    InterlockedExchange64(&slot->published,0);
    slot->token=token;slot->generation=generation;slot->tick=tick;
    slot->direction=static_cast<std::uint32_t>(direction);slot->length=static_cast<std::uint32_t>(packet.size());
    std::memcpy(slot->packet,packet.data(),packet.size());
    std::memset(slot->reserved,0,sizeof(slot->reserved));
    MemoryBarrier();
    InterlockedExchange64(&slot->published,static_cast<LONG64>(token));
    InterlockedExchange64(&header_->produced[index],static_cast<LONG64>(token));
    return true;
}
ReadResult WireIpc::Read(Direction direction,std::vector<std::uint8_t>& packet,
    std::uint64_t& generation,std::uint64_t& tick,bool controller_reader) noexcept {
    if(!header_ || !DirectionValid(direction) || direction!=Inbound(role_))return ReadResult::Error;
    const auto index=RingIndex(direction);
    const auto produced=static_cast<std::uint64_t>(InterlockedCompareExchange64(&header_->produced[index],0,0));
    const auto consumed=static_cast<std::uint64_t>(InterlockedCompareExchange64(&header_->consumed[index],0,0));
    if(consumed>produced || produced-consumed>kWireRingCapacity) {
        auto* target=ErrorTarget(controller_reader);
        if(target)InterlockedExchange(reinterpret_cast<volatile LONG*>(target),0xE102);
        MarkClosed();return ReadResult::Error;
    }
    if(consumed==produced)return ReadResult::Empty;
    const auto expected=consumed+1;
    auto* slot=Slot(direction,static_cast<std::size_t>(consumed%kWireRingCapacity));
    const auto published=static_cast<std::uint64_t>(InterlockedCompareExchange64(&slot->published,0,0));
    if(published!=expected || slot->token!=expected ||
       slot->direction!=static_cast<std::uint32_t>(direction) || slot->length<64 || slot->length>1200 ||
       !AllZero(slot->reserved,sizeof(slot->reserved))) {
        auto* target=ErrorTarget(controller_reader);
        if(target)InterlockedExchange(reinterpret_cast<volatile LONG*>(target),0xE103);
        MarkClosed();return ReadResult::Error;
    }
    generation=slot->generation;tick=slot->tick;
    packet.assign(slot->packet,slot->packet+slot->length);
    MemoryBarrier();
    InterlockedExchange64(&header_->consumed[index],static_cast<LONG64>(expected));
    return ReadResult::Packet;
}
void WireIpc::SetDriverState(std::uint32_t bits) noexcept {
    if(header_)InterlockedOr(&header_->driver_state,static_cast<LONG>(bits));
}
void WireIpc::SetDriverFlags(std::uint32_t bits) noexcept {
    if(header_)InterlockedOr(&header_->driver_flags,static_cast<LONG>(bits));
}
void WireIpc::SetDriverError(std::uint32_t code) noexcept {
    if(!header_)return;
    InterlockedExchange(&header_->driver_error,static_cast<LONG>(code));
    InterlockedOr(&header_->driver_state,static_cast<LONG>(kDriverError));
    MarkClosed();
}
void WireIpc::SetDriverPid(std::uint32_t pid) noexcept {
    if(header_)InterlockedExchange(&header_->driver_pid,static_cast<LONG>(pid));
}
void WireIpc::SetHeartbeat(std::uint64_t tick,std::uint64_t qpc) noexcept {
    if(!header_)return;
    InterlockedExchange64(&header_->heartbeat_tick,static_cast<LONG64>(tick));
    InterlockedExchange64(&header_->driver_heartbeat_qpc,static_cast<LONG64>(qpc));
}
void WireIpc::SetRunState(std::uint32_t state) noexcept {
    if(header_)InterlockedExchange(&header_->run_state,static_cast<LONG>(state));
}
void WireIpc::SetRunStartTick(std::uint64_t tick) noexcept {
    if(header_)InterlockedExchange64(&header_->run_start_tick,static_cast<LONG64>(tick));
}
void WireIpc::SetTargetTick(std::uint64_t tick) noexcept {
    if(header_)InterlockedExchange64(&header_->target_tick,static_cast<LONG64>(tick));
}
void WireIpc::MarkClosed() noexcept {
    if(header_)InterlockedExchange(&header_->closed,1);
}
std::uint32_t WireIpc::Closed() const noexcept {
    return header_?static_cast<std::uint32_t>(InterlockedCompareExchange(
        const_cast<LONG*>(&header_->closed),0,0)):1;
}
std::uint32_t WireIpc::RunState() const noexcept {
    return header_?static_cast<std::uint32_t>(InterlockedCompareExchange(
        const_cast<LONG*>(&header_->run_state),0,0)):0;
}
std::uint32_t WireIpc::DriverState() const noexcept {
    return header_?static_cast<std::uint32_t>(InterlockedCompareExchange(
        const_cast<LONG*>(&header_->driver_state),0,0)):0;
}
std::uint32_t WireIpc::DriverError() const noexcept {
    return header_?static_cast<std::uint32_t>(InterlockedCompareExchange(
        const_cast<LONG*>(&header_->driver_error),0,0)):0;
}
std::uint32_t WireIpc::ControllerError() const noexcept {
    return header_?static_cast<std::uint32_t>(InterlockedCompareExchange(
        const_cast<LONG*>(&header_->controller_error),0,0)):0;
}
std::uint32_t WireIpc::DriverFlags() const noexcept {
    return header_?static_cast<std::uint32_t>(InterlockedCompareExchange(
        const_cast<LONG*>(&header_->driver_flags),0,0)):0;
}
std::uint64_t WireIpc::HeartbeatTick() const noexcept {
    return header_?static_cast<std::uint64_t>(InterlockedCompareExchange64(
        const_cast<LONG64*>(&header_->heartbeat_tick),0,0)):0;
}
std::uint64_t WireIpc::RunStartTick() const noexcept {
    return header_?static_cast<std::uint64_t>(InterlockedCompareExchange64(
        const_cast<LONG64*>(&header_->run_start_tick),0,0)):0;
}
std::uint64_t WireIpc::StopTick() const noexcept {
    return header_?static_cast<std::uint64_t>(InterlockedCompareExchange64(
        const_cast<LONG64*>(&header_->stop_tick),0,0)):0;
}
std::uint64_t WireIpc::LocalGeneration() const noexcept {
    return header_?static_cast<std::uint64_t>(InterlockedCompareExchange64(
        const_cast<LONG64*>(&header_->local_generation),0,0)):1;
}
std::uint64_t WireIpc::TargetTick() const noexcept {
    return header_?static_cast<std::uint64_t>(InterlockedCompareExchange64(
        const_cast<LONG64*>(&header_->target_tick),0,0)):0;
}
} // namespace isaac_cs_t0::t09

namespace isaac_cs_t0::t09 {
std::uint64_t WireIpc::ControllerHeartbeatTick() const noexcept {
    return header_?static_cast<std::uint64_t>(InterlockedCompareExchange64(
        const_cast<LONG64*>(&header_->controller_heartbeat_tick),0,0)):0;
}
} // namespace isaac_cs_t0::t09
