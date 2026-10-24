#pragma once
#include <Windows.h>
#include "isaac_cs_t0/minimal_protocol.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace isaac_cs_t0::t09 {
inline constexpr std::uint32_t kWireMagic=0x49575439u; // little-endian "T9WI"
inline constexpr std::uint16_t kWireVersion=1;
inline constexpr std::size_t kWireHeaderSize=320,kWireSlotSize=1248,kWireRingCapacity=128;
inline constexpr std::size_t kWireMappingSize=kWireHeaderSize+2*kWireRingCapacity*kWireSlotSize;
inline constexpr std::uint32_t kRunCreated=0,kRunBootstrap=1,kRunRunning=2,kRunStopRequested=3,kRunStopped=4;
inline constexpr std::uint32_t kDriverOpen=1,kDriverHandshake=2,kDriverEstablished=4,kDriverReady=8,
    kDriverRunning=16,kDriverStopped=32,kDriverError=64;
inline constexpr std::uint32_t kDriverTargetReached=1,kDriverScriptComplete=2,kDriverRestoreComplete=4;
inline constexpr std::uint32_t kControllerMappingCreated=1,kControllerUdpReady=2,kControllerDriverSpawned=4,
    kControllerReadySent=8,kControllerArmedSent=16,kControllerStopReceived=32,kControllerStopped=64,
    kControllerError=128;

enum class Direction : std::uint32_t { ToAuthority=0,ToRemote=1 };
enum class ReadResult : std::uint32_t { Empty,Packet,Error };
enum class OpenResult : std::uint32_t { Accepted,Name,Mapping,View,Header,Identity,Build,Reserved };

struct alignas(8) WireHeader {
    std::uint32_t magic=kWireMagic;std::uint16_t version=kWireVersion,header_size=static_cast<std::uint16_t>(kWireHeaderSize);
    std::uint32_t mapping_size=static_cast<std::uint32_t>(kWireMappingSize),role=0;
    std::uint8_t run_nonce[16]{},session[16]{},authority_nonce[16]{},remote_nonce[16]{};
    std::uint64_t clients[2]{};
    std::uint8_t build[32]{};
    std::uint16_t input_schema=2,state_schema=1,wire_version=2,identity_reserved=0;
    volatile LONG run_state=kRunCreated,driver_state=0,driver_error=0,controller_error=0;
    volatile LONG64 produced[2]{},consumed[2]{},overwritten[2]{};
    volatile LONG64 heartbeat_tick=0,run_start_tick=0,stop_tick=0,local_generation=1,target_tick=0;
    volatile LONG driver_pid=0,driver_flags=0;
    volatile LONG64 driver_heartbeat_qpc=0;
    volatile LONG closed=0,controller_state=kControllerMappingCreated;
    volatile LONG64 controller_heartbeat_tick=0;
    std::uint8_t reserved[48]{};
};
static_assert(sizeof(WireHeader)==kWireHeaderSize);
static_assert(offsetof(WireHeader,session)==32);
static_assert(offsetof(WireHeader,clients)==80);
static_assert(offsetof(WireHeader,build)==96);
static_assert(offsetof(WireHeader,run_state)==136);
static_assert(offsetof(WireHeader,produced)==152);
static_assert(offsetof(WireHeader,overwritten)==184);
static_assert(offsetof(WireHeader,heartbeat_tick)==200);
static_assert(offsetof(WireHeader,target_tick)==232);
static_assert(offsetof(WireHeader,closed)==256);
static_assert(offsetof(WireHeader,controller_heartbeat_tick)==264);
static_assert(offsetof(WireHeader,reserved)==272);

struct alignas(8) WireSlot {
    volatile LONG64 published=0;
    std::uint64_t token=0,generation=0,tick=0;
    std::uint32_t direction=0,length=0;
    std::uint8_t packet[1200]{};
    std::uint8_t reserved[8]{};
};
static_assert(sizeof(WireSlot)==kWireSlotSize);
static_assert(offsetof(WireSlot,token)==8);
static_assert(offsetof(WireSlot,direction)==32);
static_assert(offsetof(WireSlot,packet)==40);
static_assert(offsetof(WireSlot,reserved)==1240);

struct OpenConfig {
    std::uint32_t role=0;
    std::array<std::uint8_t,16> run_nonce{},session{},authority_nonce{},remote_nonce{};
    std::array<std::uint64_t,2> clients{};
    std::array<std::uint8_t,32> build{};
    std::uint16_t input_schema=2,state_schema=1,wire_version=2;
};

// One endpoint's immutable packet bridge. The native/driver main thread is the
// only protocol decoder. The controller process performs the opposite ring
// operation. No game object, Lua state or canonical queue is touched here.
class WireIpc final {
public:
    WireIpc()=default;
    WireIpc(const WireIpc&)=delete;
    WireIpc& operator=(const WireIpc&)=delete;
    ~WireIpc(){Close();}
    OpenResult Open(const wchar_t* name,const OpenConfig& expected) noexcept;
    // Used by the remote driver after the trusted controller creates the
    // mapping. It validates the complete structural identity and returns it;
    // exact manifest equality remains the controller's/full-Open caller's job.
    OpenResult OpenForRole(const wchar_t* name,std::uint32_t expected_role,
                           OpenConfig& discovered) noexcept;
    void Close() noexcept;
    bool IsOpen() const noexcept {return view_!=nullptr;}
    bool Write(Direction direction,std::span<const std::uint8_t> packet,
               std::uint64_t generation,std::uint64_t tick,bool controller_writer) noexcept;
    ReadResult Read(Direction direction,std::vector<std::uint8_t>& packet,
                    std::uint64_t& generation,std::uint64_t& tick,bool controller_reader) noexcept;
    static Direction Outbound(std::uint32_t role) noexcept {return role==1?Direction::ToRemote:Direction::ToAuthority;}
    static Direction Inbound(std::uint32_t role) noexcept {return role==1?Direction::ToAuthority:Direction::ToRemote;}
    WireHeader* Header() noexcept {return header_;}
    const WireHeader* Header() const noexcept {return header_;}
    std::uint32_t Role() const noexcept {return role_;}
    void SetDriverState(std::uint32_t bits) noexcept;
    void SetDriverFlags(std::uint32_t bits) noexcept;
    void SetDriverError(std::uint32_t code) noexcept;
    void SetDriverPid(std::uint32_t pid) noexcept;
    void SetHeartbeat(std::uint64_t tick,std::uint64_t qpc) noexcept;
    void SetRunState(std::uint32_t state) noexcept;
    void SetRunStartTick(std::uint64_t tick) noexcept;
    void SetTargetTick(std::uint64_t tick) noexcept;
    void MarkClosed() noexcept;
    std::uint32_t Closed() const noexcept;
    std::uint32_t RunState() const noexcept;
    std::uint32_t DriverState() const noexcept;
    std::uint32_t DriverError() const noexcept;
    std::uint32_t ControllerError() const noexcept;
    std::uint32_t DriverFlags() const noexcept;
    std::uint64_t HeartbeatTick() const noexcept;
    std::uint64_t RunStartTick() const noexcept;
    std::uint64_t StopTick() const noexcept;
    std::uint64_t LocalGeneration() const noexcept;
    std::uint64_t TargetTick() const noexcept;
    std::uint64_t ControllerHeartbeatTick() const noexcept;
private:
    WireHeader* HeaderAtBase() noexcept {return reinterpret_cast<WireHeader*>(view_);}
    WireSlot* Slot(Direction direction,std::size_t index) noexcept;
    std::uint32_t* ErrorTarget(bool controller) noexcept;
    HANDLE mapping_=nullptr;
    std::uint8_t* view_=nullptr;
    WireHeader* header_=nullptr;
    std::uint32_t role_=0;
};
} // namespace isaac_cs_t0::t09
