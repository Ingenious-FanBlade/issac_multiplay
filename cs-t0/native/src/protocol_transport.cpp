#include "isaac_cs_t0/protocol_transport.hpp"
#include <algorithm>
#include <tuple>

namespace isaac_cs_t0::protocol {
bool ValidFaultConfig(const FaultConfig& c) noexcept {
    return c.version==1 && c.size==sizeof(FaultConfig) && c.seed && c.delay_ticks<=8 && c.jitter_ticks<=8 &&
        c.loss_per_mille<=1000 && c.duplicate_per_mille<=1000 && c.reorder_per_mille<=1000 &&
        !c.reserved && !c.reserved_tail && c.outage_start<=UINT64_MAX-c.outage_ticks;
}
bool FaultInjectionTransport::Initialize(const FaultConfig& c,TraceFunction trace,void* context) noexcept {
    if(initialized_ || !ValidFaultConfig(c)) return false;
    config_=c;rng_=c.seed;trace_=trace;context_=context;initialized_=true;return true;
}
std::uint64_t FaultInjectionTransport::Random() noexcept {rng_^=rng_<<13;rng_^=rng_>>7;rng_^=rng_<<17;return rng_;}
bool FaultInjectionTransport::Emit(TraceAction action,Direction dir,const Packet& p,std::uint64_t now,
    std::uint64_t generation,std::uint64_t token,std::uint32_t decision) noexcept {
    if(!trace_)return healthy_;
    TraceRecord r{};r.action=action;r.direction=dir;r.packet=p;r.tick=now;r.generation=generation;r.token=token;r.decision=decision;
    if(!trace_(r,context_))healthy_=false;return healthy_;
}
TransportResult FaultInjectionTransport::Send(Direction dir,const Packet& packet,std::uint64_t generation,std::uint64_t now,bool reliable) noexcept {
    if(!initialized_ || closed_ || !healthy_)return TransportResult::Closed;
    if(packet.size<kHeaderSize || packet.size>kMaximumPacket || now>UINT64_MAX-32 || token_==UINT64_MAX ||
       (dir!=Direction::ToAuthority && dir!=Direction::ToRemote))return TransportResult::Invalid;
    const auto reserve=!reliable && config_.duplicate_per_mille?2u:1u;
    if(count_+reserve>slots_.size()) {Emit(TraceAction::Full,dir,packet,now,generation,0);return TransportResult::Full;}
    const auto token=++token_;if(!Emit(TraceAction::Send,dir,packet,now,generation,token,reliable?1u:0u))return TransportResult::Closed;
    // Faults begin only after HELLO/first ACK. The handshake uses reliable owned slots.
    const bool outage=!reliable && config_.outage_ticks && now>=config_.outage_start && now-config_.outage_start<config_.outage_ticks;
    if(outage || (!reliable && Random()%1000<config_.loss_per_mille)) {
        Emit(TraceAction::Drop,dir,packet,now,generation,token,outage?2u:1u);return TransportResult::Lost;
    }
    const bool duplicate=!reliable && Random()%1000<config_.duplicate_per_mille;
    const auto jitter=reliable?0:Random()%(1+config_.jitter_ticks);
    const bool reorder=!reliable && Random()%1000<config_.reorder_per_mille;
    const auto delay=reliable?0:config_.delay_ticks+jitter+(reorder?4u:0u);
    for(unsigned copy=0;copy<(duplicate?2u:1u);++copy) {
        const auto slot=std::find_if(slots_.begin(),slots_.end(),[](const Slot& s){return !s.used;});
        if(slot==slots_.end()) {healthy_=false;return TransportResult::Full;}
        slot->packet=packet;slot->generation=generation;slot->token=token;slot->direction=dir;slot->copy=copy;slot->due=now+delay+copy;slot->used=true;++count_;
        if(!Emit(TraceAction::Enqueue,dir,packet,now,generation,token,static_cast<std::uint32_t>(delay+copy)))return TransportResult::Closed;
    }return TransportResult::Queued;
}
bool FaultInjectionTransport::Pop(Direction dir,std::uint64_t now,Delivery& output) noexcept {
    if(!healthy_ || closed_)return false;
    Slot* best=nullptr;
    for(auto& s:slots_)if(s.used && s.direction==dir && s.due<=now &&
        (!best || std::tie(s.due,s.token,s.copy)<std::tie(best->due,best->token,best->copy)))best=&s;
    if(!best)return false;
    Delivery d{best->packet,best->generation,best->token};best->used=false;--count_;
    if(!Emit(TraceAction::Deliver,dir,d.packet,now,d.generation,d.token))return false;
    output=d;return true;
}
void FaultInjectionTransport::Clear(Direction dir,std::uint64_t now) noexcept {
    for(auto& slot:slots_)if(slot.used && slot.direction==dir) {
        Emit(TraceAction::Drop,dir,slot.packet,now,slot.generation,slot.token,3);slot.used=false;--count_;
    }
}
void FaultInjectionTransport::Close(std::uint64_t now) noexcept {
    if(closed_)return;Clear(Direction::ToAuthority,now);Clear(Direction::ToRemote,now);
    Packet empty{};Emit(TraceAction::Closed,Direction::ToAuthority,empty,now,0,0);closed_=true;
}
bool ProtocolLink::SendPrepared(Endpoint& from,Direction dir,const Packet& p,std::uint64_t generation,std::uint64_t now,bool reliable) noexcept {
    const auto sent=transport_.Send(dir,p,generation,now,reliable);
    if(sent!=TransportResult::Queued && sent!=TransportResult::Lost) {from.CancelPending();healthy_=false;return false;}
    if(from.CommitSent(p)!=Error::None)healthy_=false;return Healthy();
}
bool ProtocolLink::Send(Endpoint& from,Direction dir,const Message& message,std::uint64_t generation,std::uint64_t now,bool reliable) noexcept {
    Packet packet{};if(from.Prepare(message,packet)!=Error::None) {healthy_=false;return false;}
    return SendPrepared(from,dir,packet,generation,now,reliable);
}
bool ProtocolLink::Receive(Endpoint& endpoint,Direction dir,const Delivery& d,std::uint64_t now,Message& message,Error& result) noexcept {
    result=endpoint.Receive(d.packet.View(),message);TraceRecord r{};r.tick=now;r.token=d.token;r.generation=d.generation;
    r.direction=dir;r.packet=d.packet;r.decision=static_cast<unsigned>(result);r.action=result==Error::None?TraceAction::Receive:TraceAction::Rejected;
    if(trace_ && !trace_(r,context_))healthy_=false;
    if(endpoint.State()==PeerState::Failed)healthy_=false;return Healthy();
}
bool ProtocolLink::Initialize(const Config& config,const FaultConfig& faults,TraceFunction trace,void* context) noexcept {
    if(initialized_ || config.role!=Role::Authority)return false;
    initialized_=true;trace_=trace;context_=context;
    auto peer=config;peer.role=Role::Remote;std::swap(peer.local_nonce,peer.peer_nonce);
    if(authority_.Start(config)!=Error::None || remote_.Start(peer)!=Error::None || !transport_.Initialize(faults,trace,context))return healthy_=false;
    Packet packet{};Delivery d{};Message message{};Error result{};
    if(authority_.PrepareHello(packet)!=Error::None || !SendPrepared(authority_,Direction::ToRemote,packet,0,0,true) ||
       remote_.PrepareHello(packet)!=Error::None || !SendPrepared(remote_,Direction::ToAuthority,packet,0,0,true))return healthy_=false;
    if(!transport_.Pop(Direction::ToAuthority,0,d) || !Receive(authority_,Direction::ToAuthority,d,0,message,result) || result!=Error::None ||
       !transport_.Pop(Direction::ToRemote,0,d) || !Receive(remote_,Direction::ToRemote,d,0,message,result) || result!=Error::None)return healthy_=false;
    if(authority_.PrepareFirstAck(packet)!=Error::None || !SendPrepared(authority_,Direction::ToRemote,packet,0,0,true) || !PumpRemote(0))return healthy_=false;
    return authority_.State()==PeerState::Established && remote_.State()==PeerState::Established;
}
bool ProtocolLink::SendInput(const coop_active::InputCmdDraftV2& command,std::uint64_t generation,std::uint64_t now) noexcept {
    if(!initialized_ || closed_ || !Healthy() || !generation)return false;
    if(generation_!=generation) {transport_.Clear(Direction::ToAuthority,now);history_count_=0;generation_=generation;}
    if(history_count_ && command.sequence<=history_[history_count_-1].sequence)return false;
    Message m{};m.header.kind=Kind::Input;m.input.command=command;m.input.last_ack=remote_.LastAckSequence();
    m.input.redundancy_count=static_cast<std::uint16_t>(history_count_);m.input.redundant=history_;
    if(!Send(remote_,Direction::ToAuthority,m,generation,now))return false;
    if(history_count_==history_.size()) {std::move(history_.begin()+1,history_.end(),history_.begin());--history_count_;}
    history_[history_count_++]=command;return true;
}
bool ProtocolLink::PumpRemote(std::uint64_t now) noexcept {
    Delivery d{};Message message{};Error result{};
    for(unsigned n=0;n<64 && transport_.Pop(Direction::ToRemote,now,d);++n)
        if(!Receive(remote_,Direction::ToRemote,d,now,message,result))return false;
    return Healthy();
}
bool ProtocolLink::PumpInputs(std::uint64_t now,std::uint32_t budget,AdmitFunction admit,void* context,std::uint64_t expected_generation) noexcept {
    if(!initialized_ || closed_ || !Healthy() || !admit)return false;
    if(!PumpRemote(now))return false;
    Delivery d{};Message m{};Error result{};
    for(unsigned n=0;n<32 && budget>=kMaximumInputRecords && transport_.Pop(Direction::ToAuthority,now,d);++n) {
        if(d.generation!=(expected_generation?expected_generation:generation_)) {
            TraceRecord r{};r.tick=now;r.token=d.token;r.generation=d.generation;r.packet=d.packet;r.action=TraceAction::Rejected;
            r.decision=static_cast<unsigned>(Error::Identity);if(trace_ && !trace_(r,context_))return healthy_=false;
            continue;
        }
        if(!Receive(authority_,Direction::ToAuthority,d,now,m,result))return false;
        if(result!=Error::None)continue;
        if(m.header.kind!=Kind::Input)return healthy_=false;
        for(std::size_t i=0;i<=m.input.redundancy_count;++i) {
            const auto& c=i==m.input.redundancy_count?m.input.command:m.input.redundant[i];
            const auto decision=admit(c,d.generation,context);--budget;
            TraceRecord r{};r.tick=now;r.token=d.token;r.generation=d.generation;r.action=TraceAction::Admission;
            r.decision=decision;coop_active::EncodedInputCmdV2 bytes{};
            if(!coop_active::Encode(c,bytes))return healthy_=false;
            r.packet.size=bytes.size();std::copy(bytes.begin(),bytes.end(),r.packet.bytes.begin());
            if(trace_ && !trace_(r,context_))return healthy_=false;
        }
    }return Healthy();
}
bool ProtocolLink::Publish(const Ack& ack,const StateObservation& state,std::uint64_t now) noexcept {
    if(!initialized_ || closed_ || !Healthy())return false;
    Message m{};m.header.kind=Kind::Ack;m.ack=ack;if(!Send(authority_,Direction::ToRemote,m,0,now))return false;
    m={};m.header.kind=Kind::StateHash;m.state=state;
    return Send(authority_,Direction::ToRemote,m,0,now) && PumpRemote(now);
}
void ProtocolLink::Retire(std::uint64_t now) noexcept {if(closed_)return;transport_.Close(now);authority_.Retire();remote_.Retire();closed_=true;}
} // namespace isaac_cs_t0::protocol
