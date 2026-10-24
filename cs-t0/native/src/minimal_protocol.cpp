#include "isaac_cs_t0/minimal_protocol.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <tuple>

namespace isaac_cs_t0::protocol {
namespace {
constexpr std::size_t kHelloSize=136,kInputPrefix=16,kAckSize=216,kStatePrefix=96,kValueSize=40;
bool Nonzero(const Id& id) noexcept {return std::any_of(id.begin(),id.end(),[](auto b){return b!=0;});}
bool ValidSequence(std::uint64_t value) noexcept {return value!=0 && value!=UINT64_MAX;}
bool ValidText(const Text& value) noexcept {
    bool tail=false,nonempty=false;
    for(auto b:value) {if(!b) {tail=true;continue;}if(tail || b<33 || b>126) return false;nonempty=true;}
    return tail && nonempty;
}
bool ValidRole(Role role) noexcept {return role==Role::Authority || role==Role::Remote;}
bool ValidKind(Kind kind) noexcept {return kind>=Kind::Hello && kind<=Kind::StateHash;}
Role Other(Role role) noexcept {return role==Role::Authority?Role::Remote:Role::Authority;}
std::size_t Ordinal(Role role) noexcept {return role==Role::Authority?0:1;}
void Put(std::uint8_t* out,std::uint64_t value,unsigned width) noexcept {
    for(unsigned i=0;i<width;++i) out[i]=static_cast<std::uint8_t>(value>>(i*8));
}
std::uint64_t Get(const std::uint8_t* in,unsigned width) noexcept {
    std::uint64_t result=0;for(unsigned i=0;i<width;++i) result|=std::uint64_t(in[i])<<(i*8);return result;
}
template<std::size_t N> void Store(std::uint8_t* to,const std::array<std::uint8_t,N>& from) noexcept {std::copy(from.begin(),from.end(),to);}
template<std::size_t N> void Read(const std::uint8_t* from,std::array<std::uint8_t,N>& to) noexcept {std::copy_n(from,N,to.begin());}
bool HelloValid(const Hello& h) noexcept {
    return ValidText(h.build) && ValidText(h.domain) && Nonzero(h.local_nonce) && Nonzero(h.peer_nonce) &&
        h.local_nonce!=h.peer_nonce && h.input_schema && h.state_schema && h.protocol==kVersion && ValidRole(h.role) &&
        h.capabilities && h.maximum_packet>=kHeaderSize+kAckSize && h.maximum_packet<=kMaximumPacket &&
        h.clients[0] && h.clients[1] && h.clients[0]!=h.clients[1];
}
void WriteHello(std::uint8_t* p,const Hello& h) noexcept {
    Store(p,h.build);Store(p+32,h.domain);Store(p+64,h.local_nonce);Store(p+80,h.peer_nonce);
    Put(p+96,h.input_schema,2);Put(p+98,h.state_schema,2);Put(p+100,h.protocol,2);Put(p+102,static_cast<unsigned>(h.role),2);
    Put(p+104,h.capabilities,8);Put(p+112,h.maximum_packet,4);Put(p+120,h.clients[0],8);Put(p+128,h.clients[1],8);
}
Error ReadHello(const std::uint8_t* p,Hello& h) noexcept {
    if(Get(p+116,4)) return Error::Reserved;
    Read(p,h.build);Read(p+32,h.domain);Read(p+64,h.local_nonce);Read(p+80,h.peer_nonce);
    h.input_schema=static_cast<std::uint16_t>(Get(p+96,2));h.state_schema=static_cast<std::uint16_t>(Get(p+98,2));
    h.protocol=static_cast<std::uint16_t>(Get(p+100,2));h.role=static_cast<Role>(Get(p+102,2));
    h.capabilities=Get(p+104,8);h.maximum_packet=static_cast<std::uint32_t>(Get(p+112,4));
    h.clients={Get(p+120,8),Get(p+128,8)};return Error::None;
}
std::size_t PayloadSize(const Message& m) noexcept {
    switch(m.header.kind) {
    case Kind::Hello:return kHelloSize;
    case Kind::Input:return kInputPrefix+64*(1+m.input.redundancy_count);
    case Kind::Ack:return kAckSize;
    case Kind::StateHash:return kStatePrefix+kValueSize*m.state.count;
    }return 0;
}
bool ValidAck(const Ack& a) noexcept {
    if(!a.client_id || a.client_id!=a.agreement.clients[1] || !HelloValid(a.agreement) || a.agreement.role!=Role::Authority ||
       a.server_tick==UINT64_MAX || a.accepted==UINT64_MAX || a.processed==UINT64_MAX || a.processed_source==UINT64_MAX ||
       a.processed>a.accepted || a.processed_source>a.processed || a.processed_tick>a.server_tick ||
       static_cast<unsigned>(a.session_state)>static_cast<unsigned>(coop_active::SessionState::Failed) ||
       (a.rejection_mask&~0x7ff8u) || a.rejected_count<static_cast<std::uint64_t>(std::popcount(a.rejection_mask))) return false;
    // A successful missing-input tick publishes source=0 while the historical
    // processed high-water remains nonzero. It must not re-ack the previous source.
    if((a.processed==0 && (a.processed_source!=0 || a.processed_tick!=0)) ||
       (a.processed!=0 && (!a.processed_tick || !a.generation)) ||
       ((a.rejected_count==0)!=(a.rejection_mask==0))) return false;
    if(a.purpose==AckPurpose::First) return ValidSequence(a.hello_sequence) &&
        a.session_state==coop_active::SessionState::Warmup && a.server_tick==0 && a.accepted==0 && a.processed==0 &&
        a.generation==0 && a.rejected_count==0 && a.rejection_mask==0;
    return a.purpose==AckPurpose::Progress && a.hello_sequence==0 &&
        a.session_state>=coop_active::SessionState::Active;
}
bool ValidObservation(const StateObservation& s) noexcept {
    if(!ValidText(s.domain) || !s.schema || !s.tick || s.tick==UINT64_MAX || !s.run || !s.room ||
       !s.attempt || !s.source_token || !s.reason || s.reason>kValuesNotExported || s.count>kMaximumValues ||
       (s.phase!=CapturePhase::PostStandardPreTopExit && s.phase!=CapturePhase::OddOnly)) return false;
    if(s.status==StateStatus::Gap) return s.count==0 &&
        ((s.phase==CapturePhase::OddOnly)==(s.reason==2));
    if(s.status!=StateStatus::PartialValues || s.phase!=CapturePhase::PostStandardPreTopExit || s.reason==2 || !s.capture || !s.count) return false;
    for(std::size_t i=0;i<s.count;++i) {
        const auto& v=s.values[i];
        if(!v.entity || !v.use || !v.field || !v.width || v.width>16 ||
           (v.encoding!=ValueEncoding::Raw && v.encoding!=ValueEncoding::F32 && v.encoding!=ValueEncoding::Derived)) return false;
        for(std::size_t b=v.width;b<v.raw.size();++b) if(v.raw[b]) return false;
        if(v.encoding==ValueEncoding::F32) {
            if(v.width!=4) return false;
            const auto bits=static_cast<std::uint32_t>(Get(v.raw.data(),4));
            if(!std::isfinite(std::bit_cast<float>(bits))) return false;
        }
        if(i && std::tie(s.values[i-1].entity,s.values[i-1].use,s.values[i-1].field)>=std::tie(v.entity,v.use,v.field)) return false;
    }return true;
}
Error Validate(const Message& m) noexcept {
    const auto& h=m.header;
    if(!ValidKind(h.kind)) return Error::Kind;
    if(!ValidRole(h.role)) return Error::Role;
    if(!Nonzero(h.session) || !h.sender) return Error::Identity;
    if(!ValidSequence(h.sequence)) return Error::Sequence;
    if(h.kind==Kind::Hello) {
        if(!HelloValid(m.hello)) return Error::Payload;
        if(m.hello.role!=h.role || h.sender!=m.hello.clients[Ordinal(h.role)]) return Error::Identity;
    } else if(h.kind==Kind::Input) {
        if(h.role!=Role::Remote) return Error::Role;
        if(m.input.redundancy_count>=kMaximumInputRecords) return Error::Payload;
        if(m.input.last_ack==UINT64_MAX) return Error::Sequence;
        std::uint64_t previous=0;
        for(std::size_t i=0;i<=m.input.redundancy_count;++i) {
            const auto& c=i==m.input.redundancy_count?m.input.command:m.input.redundant[i];
            if(c.session_id!=h.session || c.client_id!=h.sender) return Error::Identity;
            if(!ValidSequence(c.sequence) || c.sequence<=previous) return Error::Sequence;
            if(!c.target_tick || c.target_tick==UINT64_MAX || !coop_active::Valid(c.state)) return Error::Payload;
            previous=c.sequence;
        }
    } else if(h.kind==Kind::Ack) {
        if(h.role!=Role::Authority) return Error::Role;
        if(!ValidAck(m.ack)) return Error::Payload;
        if(h.sender!=m.ack.agreement.clients[0]) return Error::Identity;
    } else {
        if(h.role!=Role::Authority) return Error::Role;
        if(!ValidObservation(m.state)) return Error::Payload;
    }
    return Error::None;
}
bool SamePacket(const Packet& a,std::span<const std::uint8_t> b) noexcept {
    return a.size==b.size() && std::equal(b.begin(),b.end(),a.bytes.begin());
}
}
const char* ErrorName(Error error) noexcept {
    switch(error) {
#define T0_ERROR(name) case Error::name:return #name
    T0_ERROR(None);T0_ERROR(Duplicate);T0_ERROR(Configuration);T0_ERROR(Length);T0_ERROR(Magic);T0_ERROR(Version);
    T0_ERROR(Header);T0_ERROR(Flags);T0_ERROR(Reserved);T0_ERROR(Crc);T0_ERROR(Kind);T0_ERROR(Role);T0_ERROR(Identity);
    T0_ERROR(Sequence);T0_ERROR(Payload);T0_ERROR(Incompatible);T0_ERROR(Handshake);T0_ERROR(Phase);T0_ERROR(Terminal);
    T0_ERROR(Pending);T0_ERROR(PendingMismatch);
#undef T0_ERROR
    }return "UNKNOWN";
}
std::uint32_t PacketCrc(std::span<const std::uint8_t> bytes) noexcept {
    std::uint32_t crc=0xffffffffu;
    for(std::size_t i=0;i<bytes.size();++i) {
        crc^=i>=56 && i<60?0:bytes[i];
        for(unsigned bit=0;bit<8;++bit) crc=(crc>>1)^((crc&1)?0xedb88320u:0);
    }return crc^0xffffffffu;
}
Error Encode(const Message& m,Packet& output) noexcept {
    const auto validation=Validate(m);if(validation!=Error::None) return validation;
    const auto size=PayloadSize(m);if(size>kMaximumPacket-kHeaderSize) return Error::Length;
    Packet packet{};packet.size=kHeaderSize+size;auto* p=packet.bytes.data();
    Store(p,kMagic);Put(p+4,kVersion,2);Put(p+6,static_cast<unsigned>(m.header.kind),2);Put(p+8,kHeaderSize,2);
    Put(p+10,static_cast<unsigned>(m.header.role),2);Store(p+16,m.header.session);Put(p+32,m.header.sender,8);
    Put(p+40,m.header.sequence,8);Put(p+48,size,4);p+=kHeaderSize;
    if(m.header.kind==Kind::Hello) WriteHello(p,m.hello);
    else if(m.header.kind==Kind::Input) {
        Put(p,1+m.input.redundancy_count,2);Put(p+8,m.input.last_ack,8);
        for(std::size_t i=0;i<=m.input.redundancy_count;++i) {
            coop_active::EncodedInputCmdV2 command{};
            if(!coop_active::Encode(i==m.input.redundancy_count?m.input.command:m.input.redundant[i],command)) return Error::Payload;
            Store(p+kInputPrefix+64*i,command);
        }
    } else if(m.header.kind==Kind::Ack) {
        const auto& a=m.ack;
        const std::array<std::uint64_t,8> words={a.server_tick,a.client_id,a.accepted,a.processed,a.processed_tick,a.processed_source,a.generation,a.rejected_count};
        for(std::size_t i=0;i<words.size();++i) Put(p+8*i,words[i],8);
        Put(p+64,a.rejection_mask,4);Put(p+68,static_cast<unsigned>(a.session_state),2);Put(p+70,static_cast<unsigned>(a.purpose),2);
        Put(p+72,a.hello_sequence,8);WriteHello(p+80,a.agreement);
    } else {
        const auto& s=m.state;Store(p,s.domain);
        const std::array<std::uint64_t,5> words={s.tick,s.run,s.room,s.attempt,s.capture};
        for(std::size_t i=0;i<words.size();++i) Put(p+32+8*i,words[i],8);
        Put(p+72,s.schema,2);Put(p+74,static_cast<unsigned>(s.phase),2);Put(p+76,static_cast<unsigned>(s.status),2);
        Put(p+78,s.reason,2);Put(p+80,s.count,2);Put(p+88,s.source_token,8);
        for(std::size_t i=0;i<s.count;++i) {
            auto* q=p+kStatePrefix+kValueSize*i;const auto& v=s.values[i];
            Put(q,v.entity,8);Put(q+8,v.use,8);Put(q+16,v.field,2);q[18]=0; // FieldStatus::Value only.
            q[19]=static_cast<std::uint8_t>(v.encoding);Put(q+20,v.width,2);Store(q+24,v.raw);
        }
    }
    Put(packet.bytes.data()+56,PacketCrc(packet.View()),4);output=packet;return Error::None;
}
Error Decode(std::span<const std::uint8_t> bytes,Message& output) noexcept {
    if(bytes.size()<kHeaderSize || bytes.size()>kMaximumPacket || !bytes.data()) return Error::Length;
    const auto* p=bytes.data();
    if(!std::equal(kMagic.begin(),kMagic.end(),p)) return Error::Magic;
    if(Get(p+4,2)!=kVersion) return Error::Version;
    if(Get(p+8,2)!=kHeaderSize) return Error::Header;
    if(Get(p+12,4)) return Error::Flags;
    if(Get(p+52,4) || Get(p+60,4)) return Error::Reserved;
    if(Get(p+48,4)!=bytes.size()-kHeaderSize) return Error::Length;
    if(Get(p+56,4)!=PacketCrc(bytes)) return Error::Crc;
    Message m{};m.header.kind=static_cast<Kind>(Get(p+6,2));m.header.role=static_cast<Role>(Get(p+10,2));
    Read(p+16,m.header.session);m.header.sender=Get(p+32,8);m.header.sequence=Get(p+40,8);
    if(!ValidKind(m.header.kind)) return Error::Kind;
    const auto size=bytes.size()-kHeaderSize;p+=kHeaderSize;
    if(m.header.kind==Kind::Hello) {
        if(size!=kHelloSize) return Error::Length;
        const auto r=ReadHello(p,m.hello);if(r!=Error::None) return r;
    } else if(m.header.kind==Kind::Input) {
        if(size<kInputPrefix) return Error::Length;
        const auto count=Get(p,2);
        if(!count || count>kMaximumInputRecords || size!=kInputPrefix+64*count) return Error::Length;
        if(Get(p+2,2) || Get(p+4,4)) return Error::Reserved;
        m.input.redundancy_count=static_cast<std::uint16_t>(count-1);m.input.last_ack=Get(p+8,8);
        for(std::size_t i=0;i<count;++i) {
            coop_active::EncodedInputCmdV2 command{};Read(p+kInputPrefix+64*i,command);
            auto& target=i==count-1?m.input.command:m.input.redundant[i];
            if(!coop_active::Decode(command,target)) return Error::Payload;
        }
    } else if(m.header.kind==Kind::Ack) {
        if(size!=kAckSize) return Error::Length;
        auto& a=m.ack;a.server_tick=Get(p,8);a.client_id=Get(p+8,8);a.accepted=Get(p+16,8);a.processed=Get(p+24,8);
        a.processed_tick=Get(p+32,8);a.processed_source=Get(p+40,8);a.generation=Get(p+48,8);a.rejected_count=Get(p+56,8);
        a.rejection_mask=static_cast<std::uint32_t>(Get(p+64,4));a.session_state=static_cast<coop_active::SessionState>(Get(p+68,2));
        a.purpose=static_cast<AckPurpose>(Get(p+70,2));a.hello_sequence=Get(p+72,8);
        const auto r=ReadHello(p+80,a.agreement);if(r!=Error::None) return r;
    } else {
        if(size<kStatePrefix) return Error::Length;
        auto& s=m.state;Read(p,s.domain);s.tick=Get(p+32,8);s.run=Get(p+40,8);s.room=Get(p+48,8);s.attempt=Get(p+56,8);s.capture=Get(p+64,8);
        s.schema=static_cast<std::uint16_t>(Get(p+72,2));s.phase=static_cast<CapturePhase>(Get(p+74,2));
        s.status=static_cast<StateStatus>(Get(p+76,2));s.reason=static_cast<std::uint16_t>(Get(p+78,2));
        s.count=static_cast<std::uint16_t>(Get(p+80,2));s.source_token=Get(p+88,8);
        if(Get(p+82,2) || Get(p+84,4)) return Error::Reserved;
        if(s.count>kMaximumValues || size!=kStatePrefix+kValueSize*s.count) return Error::Length;
        for(std::size_t i=0;i<s.count;++i) {
            const auto* q=p+kStatePrefix+kValueSize*i;auto& v=s.values[i];
            if(q[18]!=0 || Get(q+22,2)) return Error::Reserved;
            v.entity=Get(q,8);v.use=Get(q+8,8);v.field=static_cast<std::uint16_t>(Get(q+16,2));
            v.encoding=static_cast<ValueEncoding>(q[19]);v.width=static_cast<std::uint16_t>(Get(q+20,2));Read(q+24,v.raw);
        }
    }
    const auto validation=Validate(m);if(validation!=Error::None) return validation;
    output=m;return Error::None;
}
Hello Endpoint::LocalHello() const noexcept {
    Hello h{};h.build=config_.build;h.domain=config_.domain;h.local_nonce=config_.local_nonce;h.peer_nonce=config_.peer_nonce;
    h.role=config_.role;h.input_schema=config_.input_schema;h.state_schema=config_.state_schema;
    h.capabilities=config_.capabilities;h.maximum_packet=config_.maximum_packet;h.clients=config_.clients;return h;
}
Hello Endpoint::PeerHello() const noexcept {
    auto h=LocalHello();h.role=Other(h.role);std::swap(h.local_nonce,h.peer_nonce);return h;
}
Error Endpoint::Start(const Config& c) noexcept {
    if(state_!=PeerState::Dormant) return Error::Phase;
    config_=c;
    if(!Nonzero(c.session) || !HelloValid(LocalHello())) return Fail(Error::Configuration);
    state_=PeerState::Negotiating;return Error::None;
}
Error Endpoint::PrepareHello(Packet& output) noexcept {
    if(state_!=PeerState::Negotiating || hello_sent_) return Error::Phase;
    Message m{};m.header.kind=Kind::Hello;m.hello=LocalHello();return Prepare(m,output);
}
Error Endpoint::PrepareFirstAck(Packet& output) noexcept {
    if(config_.role!=Role::Authority || state_!=PeerState::Negotiating || !hello_sent_ || !hello_received_) return Error::Phase;
    Message m{};m.header.kind=Kind::Ack;m.ack.purpose=AckPurpose::First;m.ack.client_id=config_.clients[1];
    m.ack.hello_sequence=peer_hello_sequence_;return Prepare(m,output);
}
Error Endpoint::Prepare(const Message& content,Packet& output) noexcept {
    if(state_==PeerState::Failed || state_==PeerState::Retired) return Error::Terminal;
    if(state_==PeerState::Dormant) return Error::Phase;
    if(pending_valid_) return Error::Pending;
    if(!ValidSequence(next_send_)) return Fail(Error::Sequence);
    auto m=content;m.header.session=config_.session;m.header.role=config_.role;m.header.sender=config_.clients[Ordinal(config_.role)];m.header.sequence=next_send_;
    if(m.header.kind==Kind::Hello) {
        if(state_!=PeerState::Negotiating || hello_sent_) return Error::Phase;
        m.hello=LocalHello();
    } else if(m.header.kind==Kind::Ack && m.ack.purpose==AckPurpose::First) {
        if(config_.role!=Role::Authority) return Error::Role;
        if(state_!=PeerState::Negotiating || !hello_sent_ || !hello_received_ || m.ack.hello_sequence!=peer_hello_sequence_) return Error::Phase;
        m.ack.agreement=LocalHello();
    } else {
        if(state_!=PeerState::Established) return Error::Phase;
        if(m.header.kind==Kind::Ack) m.ack.agreement=LocalHello();
        if(m.header.kind==Kind::Input && m.input.last_ack>received_sequences_[2]) return Error::Sequence;
        if(m.header.kind==Kind::StateHash && (m.state.domain!=config_.domain || m.state.schema!=config_.state_schema)) return Error::Incompatible;
    }
    Packet packet{};const auto r=Encode(m,packet);if(r!=Error::None) return r;
    if(packet.size>config_.maximum_packet) return Error::Length;
    pending_=packet;pending_valid_=true;output=packet;return Error::None;
}
Error Endpoint::CommitSent(const Packet& packet) noexcept {
    if(state_==PeerState::Failed || state_==PeerState::Retired) return Error::Terminal;
    if(!pending_valid_ || packet.size>kMaximumPacket || !SamePacket(pending_,packet.View())) return Error::PendingMismatch;
    const auto kind=static_cast<Kind>(Get(pending_.bytes.data()+6,2));
    if(kind==Kind::Hello) {hello_sent_=true;local_hello_sequence_=next_send_;}
    else if(kind==Kind::Ack) {
        last_ack_sent_=next_send_;
        if(state_==PeerState::Negotiating) state_=PeerState::Established;
    }
    ++next_send_;pending_valid_=false;return Error::None;
}
Error Endpoint::Receive(std::span<const std::uint8_t> bytes,Message& output) noexcept {
    if(state_==PeerState::Failed || state_==PeerState::Retired) return Error::Terminal;
    if(state_==PeerState::Dormant) return Error::Phase;
    Message m{};const auto r=Decode(bytes,m);if(r!=Error::None) return r;
    if(bytes.size()>config_.maximum_packet) return Error::Length;
    if(m.header.session!=config_.session || m.header.role!=Other(config_.role) || m.header.sender!=config_.clients[Ordinal(Other(config_.role))]) return Error::Identity;
    const auto index=static_cast<std::size_t>(m.header.kind)-1;
    if(m.header.sequence<=received_sequences_[index]) return SamePacket(received_[index],bytes)?Error::Duplicate:Error::Sequence;
    if(m.header.kind==Kind::Hello) {
        if(state_!=PeerState::Negotiating || hello_received_) return Error::Phase;
        if(m.hello!=PeerHello() || m.hello.input_schema!=2 || m.hello.state_schema!=1 ||
           m.hello.domain!=kStateDomain || m.hello.capabilities!=kCapabilities) return Fail(Error::Incompatible);
    } else if(m.header.kind==Kind::Ack) {
        if(m.ack.agreement!=PeerHello()) return Fail(Error::Incompatible);
        if(m.ack.purpose==AckPurpose::First) {
            if(config_.role!=Role::Remote || state_!=PeerState::Negotiating || !hello_sent_ || !hello_received_) return Error::Phase;
            if(m.ack.hello_sequence!=local_hello_sequence_ || m.header.sequence<=peer_hello_sequence_) return Fail(Error::Handshake);
        } else {
            if(state_!=PeerState::Established) return Error::Phase;
            Message previous{};
            if(Latest(Kind::Ack,previous)==Error::None &&
               (m.ack.server_tick<previous.ack.server_tick || m.ack.accepted<previous.ack.accepted ||
                m.ack.processed<previous.ack.processed || m.ack.processed_tick<previous.ack.processed_tick || m.ack.generation<previous.ack.generation ||
                m.ack.rejected_count<previous.ack.rejected_count || m.ack.session_state<previous.ack.session_state ||
                (m.ack.rejection_mask&previous.ack.rejection_mask)!=previous.ack.rejection_mask)) return Error::Sequence;
        }
    } else {
        if(state_!=PeerState::Established) return Error::Phase;
        if(m.header.kind==Kind::Input && m.input.last_ack>last_ack_sent_) return Error::Sequence;
        if(m.header.kind==Kind::StateHash && (m.state.domain!=config_.domain || m.state.schema!=config_.state_schema)) return Error::Incompatible;
        if(m.header.kind==Kind::StateHash) {
            Message previous{};
            if(Latest(Kind::StateHash,previous)==Error::None && m.state.tick<previous.state.tick) return Error::Sequence;
        }
    }
    Packet copy{};copy.size=bytes.size();std::copy(bytes.begin(),bytes.end(),copy.bytes.begin());
    received_[index]=copy;received_sequences_[index]=m.header.sequence;
    if(m.header.kind==Kind::Hello) {hello_received_=true;peer_hello_sequence_=m.header.sequence;}
    else if(m.header.kind==Kind::Ack && m.ack.purpose==AckPurpose::First) state_=PeerState::Established;
    output=m;return Error::None;
}
Error Endpoint::Latest(Kind kind,Message& output) const noexcept {
    if(!ValidKind(kind)) return Error::Kind;
    const auto index=static_cast<std::size_t>(kind)-1;
    if(!received_sequences_[index]) return Error::Phase;
    return Decode(received_[index].View(),output);
}
} // namespace isaac_cs_t0::protocol
