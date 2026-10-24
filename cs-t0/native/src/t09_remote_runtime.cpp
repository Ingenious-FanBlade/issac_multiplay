#include "isaac_cs_t0/t09_remote_runtime.hpp"
#include "isaac_cs_t0/coop_input.hpp"
#include "isaac_cs_t0/minimal_protocol.hpp"
#include "isaac_cs_t0/t09_wire_ipc.hpp"
#include "isaac_cs_t0/target_image.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace isaac_cs_t0::t09 {
namespace {
constexpr std::uint64_t kRemoteLeadTicks=8;
std::atomic<DWORD> g_remote_error{0};
HANDLE g_remote_thread=nullptr;
std::atomic<bool> g_remote_initialized{false};
std::atomic<bool> g_remote_ever_initialized{false};
volatile LONG g_remote_stop=0;

bool PublishHello(protocol::Endpoint& endpoint,WireIpc& wire) noexcept {
    protocol::Packet packet{};
    protocol::Message hello{};hello.header.kind=protocol::Kind::Hello;
    if(endpoint.Prepare(hello,packet)!=protocol::Error::None)return false;
    if(!wire.Write(Direction::ToAuthority,packet.View(),1,0,false))return false;
    return endpoint.CommitSent(packet)==protocol::Error::None;
}
bool SendInput(protocol::Endpoint& endpoint,WireIpc& wire,
    const coop_active::InputCmdDraftV2& command) noexcept {
    protocol::Packet packet{};
    protocol::Message message{};message.header.kind=protocol::Kind::Input;
    message.input.command=command;message.input.last_ack=endpoint.LastAckSequence();
    if(endpoint.Prepare(message,packet)!=protocol::Error::None)return false;
    if(!wire.Write(Direction::ToAuthority,packet.View(),1,wire.HeartbeatTick(),false))return false;
    return endpoint.CommitSent(packet)==protocol::Error::None;
}
bool DrainInbound(protocol::Endpoint& endpoint,WireIpc& wire) noexcept {
    for(unsigned packets=0;packets<64;++packets) {
        std::vector<std::uint8_t> bytes;std::uint64_t generation=0,tick=0;
        const auto read=wire.Read(Direction::ToRemote,bytes,generation,tick,false);
        if(read==ReadResult::Empty)return true;
        if(read==ReadResult::Error)return false;
        protocol::Message message{};
        const auto result=endpoint.Receive(bytes,message);
        if(result==protocol::Error::Duplicate)continue;
        if(result!=protocol::Error::None)return false;
        if(message.header.kind==protocol::Kind::Input)return false;
    }
    return true;
}
DWORD WINAPI RemoteLoop(void*) noexcept {
    WireIpc wire;OpenConfig discovered{};
    // The controller already validated the exact manifest. The role-open path
    // validates the complete structural identity and rejects malformed roles.
    // The mapping name itself is passed only by the trusted local controller.
    // (The actual open is repeated here because the export initializer opens the
    // same mapping for its first handshake packet before the thread starts.)
    return 0;
}
}

extern "C" DWORD WINAPI IsaacCsT0T09RemoteInitialize(const wchar_t* wire_mapping) noexcept {
    if(g_remote_ever_initialized.exchange(true))return 36;
    if(g_remote_initialized.exchange(true))return 36;
    if(!target_image::IsApprovedT09(reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr)))) {
        g_remote_initialized=false;return 31;
    }
    if(!wire_mapping || !*wire_mapping) {g_remote_initialized=false;return 32;}
    // Keep the mapping/endpoint alive in a dedicated worker. This path owns no
    // game object and is therefore independent of the game's render main thread.
    struct Context final {
        std::wstring name;
        WireIpc wire;
        OpenConfig discovered;
        protocol::Endpoint endpoint;
        coop_active::Participant producer;
    };
    auto* context=new Context{};
    context->name=wire_mapping;
    if(context->wire.OpenForRole(wire_mapping,2,context->discovered)!=OpenResult::Accepted) {
        delete context;g_remote_initialized=false;return 33;
    }
    context->wire.SetDriverPid(GetCurrentProcessId());
    context->wire.SetDriverState(kDriverOpen|kDriverHandshake);
    protocol::Config config{};
    config.role=protocol::Role::Remote;
    config.session=context->discovered.session;
    config.clients=context->discovered.clients;
    config.build=context->discovered.build;
    config.local_nonce=context->discovered.remote_nonce;
    config.peer_nonce=context->discovered.authority_nonce;
    if(context->endpoint.Start(config)!=protocol::Error::None ||
       !PublishHello(context->endpoint,context->wire)) {
        context->wire.SetDriverError(0xE401);delete context;g_remote_initialized=false;return 34;
    }
    context->producer.Initialize(context->discovered.session,context->discovered.clients[1],1,nullptr);
    InterlockedExchange(&g_remote_stop,0);g_remote_error.store(0);
    g_remote_thread=CreateThread(nullptr,0,[](void* parameter)->DWORD {
        auto& context=*static_cast<Context*>(parameter);
        bool established=false;
        std::uint64_t last_heartbeat=0;
        const auto finish=[&](DWORD code) noexcept ->DWORD {
            if(code) {
                context.wire.SetDriverError(code);
                g_remote_error.store(code);
            }
            delete &context;
            return code;
        };
        for(;;) {
            if(InterlockedCompareExchange(&g_remote_stop,0,0)!=0)break;
            if(context.wire.Closed())break;
            if(!DrainInbound(context.endpoint,context.wire))return finish(0xE402);
            if(context.endpoint.State()==protocol::PeerState::Established && !established) {
                established=true;
                context.wire.SetDriverState(kDriverEstablished|kDriverReady);
            }
            const auto run_state=context.wire.RunState();
            if(run_state==kRunStopRequested||run_state==kRunStopped)break;
            if(established && run_state==kRunRunning) {
                const auto heartbeat=context.wire.ControllerHeartbeatTick();
                if(heartbeat>last_heartbeat) {
                    last_heartbeat=heartbeat;
                    const auto target=heartbeat+kRemoteLeadTicks;
                    coop_active::InputStateDraftV2 state{};
                    if((heartbeat/100)%2==0)state.move_x=4096;
                    else state.move_y=4096;
                    auto command=context.producer.MakeCommand(state,target);
                    if(command.sequence==100)command.state.edge_flags=8;
                    if(!command.client_id||!command.sequence||!command.target_tick)
                        return finish(0xE403);
                    if(!SendInput(context.endpoint,context.wire,command))return finish(0xE404);
                    context.wire.SetHeartbeat(heartbeat,0);
                    context.wire.SetDriverState(kDriverRunning);
                }
            }
            Sleep(1);
        }
        context.endpoint.Retire();
        context.wire.SetDriverFlags(kDriverScriptComplete|kDriverRestoreComplete);
        context.wire.SetDriverState(kDriverStopped);
        context.wire.SetRunState(kRunStopped);
        context.wire.MarkClosed();
        return finish(0);
    },context,0,nullptr);
    if(!g_remote_thread) {
        context->wire.SetDriverError(0xE405);delete context;g_remote_initialized=false;return 35;
    }
    return 0;
}

extern "C" DWORD WINAPI IsaacCsT0T09RemoteShutdown(void*) noexcept {
    if(!g_remote_initialized.load())return 0;
    InterlockedExchange(&g_remote_stop,1);
    if(g_remote_thread && WaitForSingleObject(g_remote_thread,10000)!=WAIT_OBJECT_0) {
        g_remote_error.store(0xE406);
        return 8;
    }
    if(g_remote_thread) {CloseHandle(g_remote_thread);g_remote_thread=nullptr;}
    const auto error=g_remote_error.load();
    g_remote_initialized.store(false);
    return error?static_cast<DWORD>(error):0;
}
} // namespace isaac_cs_t0::t09
