#include "isaac_cs_t0/t09_authority_link.hpp"
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace isaac_cs_t0::t09 {
namespace {
OpenConfig ExpectedWire(const protocol::Config& config,const std::array<std::uint8_t,16>& run_nonce) {
    OpenConfig expected{};
    expected.role=1;
    expected.run_nonce=run_nonce;
    expected.session=config.session;
    expected.authority_nonce=config.local_nonce;
    expected.remote_nonce=config.peer_nonce;
    expected.clients=config.clients;
    expected.build=config.build;
    return expected;
}
std::wstring WireName(const std::array<std::uint8_t,16>& run_nonce) {
    constexpr char digits[]="0123456789abcdef";
    std::wstring name=L"Local\\IsaacCsT0.Wire.";
    for(auto byte:run_nonce) {name.push_back(static_cast<wchar_t>(digits[byte>>4]));name.push_back(static_cast<wchar_t>(digits[byte&15]));}
    name+=L".A";
    return name;
}
bool Fail(AuthorityWireLink& link,WireIpc&) noexcept {
    link.FailInitialization(0xE301);
    return false;
}
}
bool AuthorityWireLink::Initialize(const protocol::Config& config,const wchar_t* wire_name,
    const std::array<std::uint8_t,16>& run_nonce,std::uint64_t run_ticks) noexcept {
    if(initialized_ || closed_ || config.role!=protocol::Role::Authority || !wire_name ||
       !*wire_name || !run_ticks) return false;
    initialized_=true;generation_=1;run_ticks_=run_ticks;
    const auto expected=ExpectedWire(config,run_nonce);
    if(wire_.Open(wire_name,expected)!=OpenResult::Accepted) {
        healthy_=false;return false;
    }
    wire_.SetDriverPid(GetCurrentProcessId());
    wire_.SetDriverState(kDriverOpen|kDriverHandshake);
    if(endpoint_.Start(config)!=protocol::Error::None) {
        wire_.SetDriverError(0xE301);healthy_=false;return false;
    }
    protocol::Message hello{};hello.header.kind=protocol::Kind::Hello;
    if(!Send(hello,0)) {
        wire_.SetDriverError(0xE302);return false;
    }
    hello_sent_=true;
    return true;
}
bool AuthorityWireLink::Send(const protocol::Message& content,std::uint64_t tick) noexcept {
    if(!healthy_ || closed_ || !wire_.IsOpen()) return false;
    protocol::Packet packet{};
    if(endpoint_.Prepare(content,packet)!=protocol::Error::None)return Fail(*this,wire_);
    if(!wire_.Write(Direction::ToRemote,packet.View(),generation_,tick,false))return Fail(*this,wire_);
    if(endpoint_.CommitSent(packet)!=protocol::Error::None)return Fail(*this,wire_);
    return true;
}
bool AuthorityWireLink::SendPrepared(const protocol::Packet& packet,std::uint64_t tick) noexcept {
    if(!healthy_ || closed_ || !wire_.IsOpen()) return false;
    if(!wire_.Write(Direction::ToRemote,packet.View(),generation_,tick,false))return Fail(*this,wire_);
    if(endpoint_.CommitSent(packet)!=protocol::Error::None)return Fail(*this,wire_);
    return true;
}
bool AuthorityWireLink::DrainInbound(std::uint64_t now,std::uint32_t admission_budget,
    AdmitFunction admit,void* context,std::uint64_t expected_generation) noexcept {
    for(unsigned packets=0;packets<64;++packets) {
        std::vector<std::uint8_t> bytes;std::uint64_t slot_generation=0,slot_tick=0;
        const auto read=wire_.Read(Direction::ToAuthority,bytes,slot_generation,slot_tick,false);
        if(read==ReadResult::Empty)break;
        if(read==ReadResult::Error)return Fail(*this,wire_);
        protocol::Message message{};
        const auto result=endpoint_.Receive(bytes,message);
        if(result==protocol::Error::Duplicate)continue;
        if(result!=protocol::Error::None)return Fail(*this,wire_);
        if(message.header.kind==protocol::Kind::Hello) {
            if(first_ack_sent_)continue;
            protocol::Packet ack{};
            if(endpoint_.PrepareFirstAck(ack)!=protocol::Error::None ||
               !SendPrepared(ack,now)) return Fail(*this,wire_);
            first_ack_sent_=true;
            continue;
        }
        if(message.header.kind!=protocol::Kind::Input)return Fail(*this,wire_);
        if(!established_ || wire_.RunState()!=kRunRunning)
            return Fail(*this,wire_);
        // Keep the inbound ring bounded while waiting for STOP, but do not
        // change admission progress after the final ACK has been published.
        if(publication_complete_)continue;
        const auto generation=expected_generation?expected_generation:wire_.LocalGeneration();
        for(std::size_t i=0;i<=message.input.redundancy_count;++i) {
            const auto& command=i==message.input.redundancy_count?
                message.input.command:message.input.redundant[i];
            if(run_started_ && command.target_tick>end_tick_)continue;
            (void)admit(command,generation,context);
            --admission_budget;
        }
    }
    if(endpoint_.State()==protocol::PeerState::Established && !established_) {
        established_=true;
        wire_.SetDriverState(kDriverEstablished);
        if(ready_marked_)wire_.SetDriverState(kDriverReady);
    }
    return healthy_;
}
bool AuthorityWireLink::PumpInputs(std::uint64_t now,std::uint32_t admission_budget,
    AdmitFunction admit,void* context,std::uint64_t expected_generation) noexcept {
    if(!initialized_ || closed_ || !healthy_ || !admit || !wire_.IsOpen())return false;
    if(wire_.Closed()) {closed_=true;return false;}
    if(admission_budget<protocol::kMaximumInputRecords)return healthy_;
    if(!DrainInbound(now,admission_budget,admit,context,expected_generation))return false;
    if(established_ && wire_.RunState()==kRunRunning)
        wire_.SetDriverState(kDriverRunning);
    return healthy_;
}
bool AuthorityWireLink::Publish(const protocol::Ack& ack,
    const protocol::StateObservation& state,std::uint64_t tick) noexcept {
    if(!initialized_ || closed_ || !healthy_ || !wire_.IsOpen())return false;
    if(wire_.Closed()) {closed_=true;return false;}
    // Completing the wire window does not stop the game or retire its owner.
    // Later simulation ticks must succeed without advancing packets/heartbeat.
    if(publication_complete_)return true;
    // The game keeps ticking while the remote handshake/control barrier is
    // pending. Those pre-RUN_START ticks are not C/S publications and must not
    // fail the existing authority pipeline.
    if(!established_ || wire_.RunState()!=kRunRunning)return true;
    if(!tick || tick==UINT64_MAX || ack.server_tick!=tick || state.tick!=tick)
        return Fail(*this,wire_);
    if(!run_started_) {
        const auto start=tick-1;
        // UINT64_MAX itself is not a valid protocol observation tick.
        if(run_ticks_>=UINT64_MAX-start)return Fail(*this,wire_);
        run_started_=true;run_start_tick_=start;end_tick_=start+run_ticks_;
        wire_.SetRunStartTick(run_start_tick_);
        wire_.SetTargetTick(end_tick_);
    } else if(tick!=last_heartbeat_+1 || tick>end_tick_)return Fail(*this,wire_);
    protocol::Message ack_message{};ack_message.header.kind=protocol::Kind::Ack;ack_message.ack=ack;
    if(!Send(ack_message,tick))return false;
    protocol::Message state_message{};state_message.header.kind=protocol::Kind::StateHash;
    state_message.state=state;
    if(!Send(state_message,tick))return false;
    last_heartbeat_=tick;
    wire_.SetHeartbeat(tick,0);
    wire_.SetDriverState(kDriverRunning);
    if(tick==end_tick_) {
        // Both packets have been queued and committed before this latch/flag.
        publication_complete_=true;
        wire_.SetDriverFlags(kDriverTargetReached|kDriverScriptComplete);
    }
    return healthy_;
}
void AuthorityWireLink::MarkGameReady() noexcept {
    ready_marked_=true;
    if(established_)wire_.SetDriverState(kDriverReady);
}
void AuthorityWireLink::FailInitialization(std::uint32_t code) noexcept {
    healthy_=false;
    if(wire_.IsOpen())wire_.SetDriverError(code?code:0xE303);
}
void AuthorityWireLink::Retire(std::uint64_t now) noexcept {
    if(closed_)return;
    closed_=true;
    endpoint_.Retire();
    wire_.SetDriverFlags(kDriverRestoreComplete);
    wire_.SetDriverState(kDriverStopped);
    wire_.SetRunState(kRunStopped);
    wire_.MarkClosed();
    (void)now;
}
} // namespace isaac_cs_t0::t09
