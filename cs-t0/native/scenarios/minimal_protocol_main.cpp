// Acceptance scenarios authored before the production T0-08 implementation.
// Owned in-memory endpoints only; never touches a game, socket, Hook or Lua.
#include "isaac_cs_t0/minimal_protocol.hpp"
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <string>

using namespace isaac_cs_t0::protocol;
namespace {
struct Evidence {
    std::ofstream checks,packets;
    unsigned total=0,failed=0;
    Evidence(const char* a,const char* b):checks(a),packets(b) {}
    void Check(const std::string& name,bool ok) {
        ++total;if(!ok) ++failed;
        checks<<"{\"name\":\""<<name<<"\",\"passed\":"<<(ok?"true":"false")<<"}\n";
    }
    void PacketRow(const std::string& name,const Packet& p,const Endpoint& a,const Endpoint& b,Error error) {
        packets<<"{\"name\":\""<<name<<"\",\"authority\":"<<static_cast<unsigned>(a.State())
               <<",\"remote\":"<<static_cast<unsigned>(b.State())<<",\"result\":\""<<ErrorName(error)<<"\",\"bytes\":\"";
        for(std::size_t i=0;i<p.size;++i) packets<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(p.bytes[i]);
        packets<<std::dec<<"\"}\n";
    }
};
Config Configuration(Role role) {
    Config c{};c.role=role;c.session[0]=99;c.clients={101,202};
    c.build=TextId("isaac-ng-69e6e3a7-unpacked");
    c.local_nonce[0]=role==Role::Authority?11:22;c.peer_nonce[0]=role==Role::Authority?22:11;
    return c;
}
struct Pair {
    Endpoint a,b;Message decoded{};Packet ah,bh,ack;
    Error Start() {auto r=a.Start(Configuration(Role::Authority));return r==Error::None?b.Start(Configuration(Role::Remote)):r;}
    bool Hello(Evidence& e,const std::string& n) {
        if(Start()!=Error::None || a.PrepareHello(ah)!=Error::None || b.PrepareHello(bh)!=Error::None) return false;
        if(a.CommitSent(ah)!=Error::None || b.CommitSent(bh)!=Error::None) return false;
        auto x=a.Receive(bh.View(),decoded);e.PacketRow(n+".hello-to-authority",bh,a,b,x);
        auto y=b.Receive(ah.View(),decoded);e.PacketRow(n+".hello-to-remote",ah,a,b,y);
        return x==Error::None && y==Error::None;
    }
    bool Establish(Evidence& e,const std::string& n) {
        if(!Hello(e,n) || a.PrepareFirstAck(ack)!=Error::None) return false;
        if(a.CommitSent(ack)!=Error::None) return false;
        auto r=b.Receive(ack.View(),decoded);e.PacketRow(n+".first-ack",ack,a,b,r);
        return r==Error::None && a.State()==PeerState::Established && b.State()==PeerState::Established;
    }
};
Message InputMessage() {
    Message m{};m.header.kind=Kind::Input;m.input.command.session_id=Configuration(Role::Remote).session;
    m.input.command.client_id=202;m.input.command.sequence=1;m.input.command.target_tick=2;
    m.input.command.state.move_x=32767;return m;
}
Message ProgressMessage() {
    Message m{};m.header.kind=Kind::Ack;m.ack.client_id=202;m.ack.server_tick=2;
    m.ack.accepted=1;m.ack.processed=1;m.ack.processed_tick=2;m.ack.processed_source=1;
    m.ack.generation=1;m.ack.session_state=isaac_cs_t0::coop_active::SessionState::Active;
    return m;
}
Message StateMessage() {
    Message m{};m.header.kind=Kind::StateHash;m.state.tick=2;m.state.run=1;m.state.room=1;
    m.state.attempt=1;m.state.capture=1;m.state.source_token=1;m.state.reason=15;
    m.state.count=1;auto& v=m.state.values[0];v.entity=1;v.use=1;v.field=11;
    v.encoding=ValueEncoding::F32;v.width=4;v.raw[2]=0x80;v.raw[3]=0x3f;
    return m;
}
bool SamePacket(const Packet& x,const Packet& y) {return x.size==y.size && x.bytes==y.bytes;}
void RepairCrc(Packet& p) {
    for(unsigned i=0;i<4;++i) p.bytes[56+i]=0;
    const auto crc=PacketCrc(p.View());for(unsigned i=0;i<4;++i) p.bytes[56+i]=static_cast<std::uint8_t>(crc>>(8*i));
}
}
int main(int argc,char** argv) {
    if(argc!=3) return 2;Evidence e(argv[1],argv[2]);if(!e.checks || !e.packets) return 2;
    {
        auto p=std::make_unique<Pair>();e.Check("lifecycle.both-hello",p->Hello(e,"lifecycle"));
        e.Check("lifecycle.no-early-establishment",p->a.State()==PeerState::Negotiating && p->b.State()==PeerState::Negotiating);
        e.Check("lifecycle.prepare-first-ack",p->a.PrepareFirstAck(p->ack)==Error::None);
        e.Check("lifecycle.prepare-is-not-delivery",p->a.State()==PeerState::Negotiating);
        auto changed=p->ack;changed.bytes[0]^=1;
        e.Check("lifecycle.changed-pending-rejected",p->a.CommitSent(changed)==Error::PendingMismatch);
        e.Check("lifecycle.ack-commit",p->a.CommitSent(p->ack)==Error::None && p->a.State()==PeerState::Established && p->b.State()==PeerState::Negotiating);
        e.Check("lifecycle.remote-first-ack",p->b.Receive(p->ack.View(),p->decoded)==Error::None && p->b.State()==PeerState::Established);
        e.PacketRow("lifecycle.first-ack",p->ack,p->a,p->b,Error::None);
        auto m=InputMessage();Packet input{};e.Check("lifecycle.prepare-input",p->b.Prepare(m,input)==Error::None);
        e.Check("lifecycle.input-commit",p->b.CommitSent(input)==Error::None);
        e.Check("lifecycle.input-delivery",p->a.Receive(input.View(),p->decoded)==Error::None && p->decoded.input.command.state.move_x==32767);
        e.PacketRow("lifecycle.input",input,p->a,p->b,Error::None);
        Packet encoded{};e.Check("lifecycle.input-byte-stability",Encode(p->decoded,encoded)==Error::None && SamePacket(input,encoded));
        e.Check("lifecycle.duplicate-input",p->a.Receive(input.View(),p->decoded)==Error::Duplicate);
        // The next native tick is neutral: source=0, retained processed high-water=1.
        auto ack=ProgressMessage();ack.ack.server_tick=3;ack.ack.processed_tick=3;ack.ack.processed_source=0;
        Packet progress{};e.Check("lifecycle.progress-encode",p->a.Prepare(ack,progress)==Error::None && p->a.CommitSent(progress)==Error::None);
        e.Check("lifecycle.progress-delivery",p->b.Receive(progress.View(),p->decoded)==Error::None && p->decoded.ack.processed_source==0 && p->decoded.ack.processed==1 && p->decoded.ack.processed_tick==3);
        e.PacketRow("lifecycle.progress-ack",progress,p->a,p->b,Error::None);
        auto state=StateMessage();Packet values{};e.Check("lifecycle.values-encode",p->a.Prepare(state,values)==Error::None && p->a.CommitSent(values)==Error::None);
        e.Check("lifecycle.values-delivery",p->b.Receive(values.View(),p->decoded)==Error::None && p->decoded.state.count==1);
        e.PacketRow("lifecycle.partial-values",values,p->a,p->b,Error::None);
        e.Check("lifecycle.values-byte-stability",Encode(p->decoded,encoded)==Error::None && SamePacket(values,encoded));
        state.state.status=StateStatus::Gap;state.state.count=0;state.state.phase=CapturePhase::OddOnly;state.state.reason=2;
        Packet gap{};e.Check("lifecycle.explicit-odd-gap",p->a.Prepare(state,gap)==Error::None && p->a.CommitSent(gap)==Error::None && p->b.Receive(gap.View(),p->decoded)==Error::None);
        e.PacketRow("lifecycle.odd-gap",gap,p->a,p->b,Error::None);
        p->a.Retire();p->b.Retire();e.Check("lifecycle.no-resurrection",p->a.Receive(p->bh.View(),p->decoded)==Error::Terminal && p->b.Start(Configuration(Role::Remote))==Error::Phase);
    }
    {
        Pair p;e.Check("pending.start",p.Start()==Error::None);Packet wire{};
        e.Check("pending.no-ack-before-hello",p.a.PrepareFirstAck(wire)==Error::Phase);
        e.Check("pending.no-input-before-established",p.b.Prepare(InputMessage(),wire)==Error::Phase);
        e.Check("pending.cancel-prepare",p.a.PrepareHello(wire)==Error::None);const auto first=wire;
        e.Check("pending.second-prepare-bounded",p.a.PrepareHello(wire)==Error::Pending);
        p.a.CancelPending();e.Check("pending.cancel-preserves-sequence",p.a.PrepareHello(wire)==Error::None && SamePacket(first,wire));
        e.Check("pending.single-commit",p.a.CommitSent(wire)==Error::None && p.a.CommitSent(wire)==Error::PendingMismatch);
    }
    for(unsigned which=0;which<9;++which) {
        Pair p;auto a=Configuration(Role::Authority),b=Configuration(Role::Remote);
        if(which==0) b.build[0]='x';if(which==1) b.input_schema=3;if(which==2) b.state_schema=2;
        if(which==3) b.domain[0]='x';if(which==4) b.capabilities^=8;if(which==5) b.local_nonce[1]=1;
        if(which==6) b.peer_nonce[1]=1;if(which==7) b.maximum_packet=1100;if(which==8) b.clients[0]=303;
        e.Check("mismatch.start."+std::to_string(which),p.a.Start(a)==Error::None && p.b.Start(b)==Error::None);
        Packet wire{};auto r=p.b.PrepareHello(wire);if(r==Error::None) r=p.b.CommitSent(wire);
        if(r==Error::None) r=p.a.Receive(wire.View(),p.decoded);
        e.Check("mismatch.failed-before-established."+std::to_string(which),r==Error::Incompatible && p.a.State()==PeerState::Failed);
        e.Check("mismatch.failed-stays-terminal."+std::to_string(which),p.a.Receive(wire.View(),p.decoded)==Error::Terminal);
        e.PacketRow("mismatch."+std::to_string(which),wire,p.a,p.b,r);
    }
    for(unsigned which=0;which<8;++which) {
        Endpoint endpoint;auto c=Configuration(Role::Authority);
        if(which==0) c.session={};if(which==1) c.clients[1]=101;if(which==2) c.local_nonce={};
        if(which==3) c.peer_nonce=c.local_nonce;if(which==4) c.build={};if(which==5) c.domain={};
        if(which==6) c.role=static_cast<Role>(9);if(which==7) c.maximum_packet=1201;
        e.Check("config.invalid."+std::to_string(which),endpoint.Start(c)==Error::Configuration && endpoint.State()==PeerState::Failed);
    }
    {
        Pair p;e.Check("boundary.handshake",p.Establish(e,"boundary"));
        Message hello{};e.Check("boundary.decode-hello",Decode(p.bh.View(),hello)==Error::None);
        Packet encoded{};e.Check("boundary.hello-byte-stability",Encode(hello,encoded)==Error::None && SamePacket(p.bh,encoded));
        for(std::size_t n=0;n<p.bh.size;++n) {
            Message out{};out.header.sequence=987;const auto r=Decode({p.bh.bytes.data(),n},out);
            e.Check("boundary.truncation."+std::to_string(n),r!=Error::None && out.header.sequence==987);
        }
        auto damaged=p.bh;damaged.bytes[80]^=1;
        e.Check("boundary.crc",Decode(damaged.View(),hello)==Error::Crc);
        auto extended=p.bh;extended.bytes[extended.size++]=0;e.Check("boundary.trailing",Decode(extended.View(),hello)==Error::Length);
        const std::array<unsigned,9> offsets={0,4,6,8,10,12,52,60,48};
        for(auto offset:offsets) {auto bad=p.bh;bad.bytes[offset]=0xff;RepairCrc(bad);
            e.Check("boundary.header."+std::to_string(offset),Decode(bad.View(),hello)!=Error::None);
        }
        auto old=p.bh;old.bytes[0]='I';old.bytes[1]='S';old.bytes[2]='C';old.bytes[3]='S';RepairCrc(old);
        e.Check("boundary.reject-iscs",Decode(old.View(),hello)==Error::Magic);
        Packet before=p.bh;hello.header.kind=static_cast<Kind>(99);
        e.Check("boundary.encode-output-atomic",Encode(hello,before)!=Error::None && SamePacket(before,p.bh));
        e.Check("boundary.duplicate-hello",p.a.Receive(p.bh.View(),p.decoded)==Error::Duplicate);
        e.Check("boundary.duplicate-first-ack",p.b.Receive(p.ack.View(),p.decoded)==Error::Duplicate);
    }
    {
        Pair p;e.Check("identity.hello",p.Hello(e,"identity"));p.a.PrepareFirstAck(p.ack);p.a.CommitSent(p.ack);
        Message m{};Decode(p.ack.View(),m);m.ack.hello_sequence++;
        Packet wrong{};Encode(m,wrong);e.Check("identity.wrong-echo",p.b.Receive(wrong.View(),p.decoded)==Error::Handshake && p.b.State()==PeerState::Failed);
        Pair q;e.Check("identity.fresh",q.Establish(e,"identity-fresh"));auto in=InputMessage();Packet wire{};
        q.b.Prepare(in,wire);q.b.CommitSent(wire);Message decoded{};Decode(wire.View(),decoded);
        decoded.header.session[0]++;decoded.input.command.session_id=decoded.header.session;
        const auto wrong_session_encoding=Encode(decoded,wrong);
        e.Check("identity.wrong-session-no-mutation",wrong_session_encoding==Error::None && q.a.Receive(wrong.View(),q.decoded)==Error::Identity && q.a.State()==PeerState::Established);
        e.PacketRow("identity.wrong-session",wrong,q.a,q.b,Error::Identity);
        Decode(wire.View(),decoded);decoded.header.sender=101;decoded.input.command.client_id=101;
        const auto wrong_sender_encoding=Encode(decoded,wrong);
        e.Check("identity.sender-cannot-route",wrong_sender_encoding==Error::None && q.a.Receive(wrong.View(),q.decoded)==Error::Identity);
        e.PacketRow("identity.wrong-sender",wrong,q.a,q.b,Error::Identity);
        e.Check("identity.valid-after-reject",q.a.Receive(wire.View(),q.decoded)==Error::None);
        Decode(wire.View(),decoded);decoded.input.command.target_tick++;Encode(decoded,wrong);
        e.Check("identity.same-seq-different-packet",q.a.Receive(wrong.View(),q.decoded)==Error::Sequence);
        decoded.header.sequence=UINT64_MAX;e.Check("identity.no-sequence-wrap",Encode(decoded,wrong)==Error::Sequence);
        decoded.header.sequence=1;decoded.input.command.client_id=101;
        e.Check("identity.input-envelope-binding",Encode(decoded,wrong)==Error::Identity);
    }
    {
        Pair p;e.Check("messages.establish",p.Establish(e,"messages"));
        auto input=InputMessage();Packet packet{};
        input.input.command.state.version=1;e.Check("messages.reject-v1",p.b.Prepare(input,packet)==Error::Payload);
        input=InputMessage();input.input.command.state.reserved=1;e.Check("messages.reject-canonical-reserved",p.b.Prepare(input,packet)==Error::Payload);
        input=InputMessage();input.input.command.sequence=0;e.Check("messages.reject-zero-input-seq",p.b.Prepare(input,packet)==Error::Sequence);
        input=InputMessage();input.input.last_ack=UINT64_MAX;e.Check("messages.reject-ack-wrap",p.b.Prepare(input,packet)==Error::Sequence);
        auto ack=ProgressMessage();ack.ack.processed=2;e.Check("messages.processed-beyond-accepted",p.a.Prepare(ack,packet)==Error::Payload);
        ack=ProgressMessage();ack.ack.processed_tick=3;e.Check("messages.processed-beyond-tick",p.a.Prepare(ack,packet)==Error::Payload);
        auto state=StateMessage();state.state.count=kMaximumValues+1;e.Check("messages.state-budget",p.a.Prepare(state,packet)==Error::Payload);
        state=StateMessage();state.state.reason=0;e.Check("messages.no-complete-world-claim",p.a.Prepare(state,packet)==Error::Payload);
        state=StateMessage();state.state.values[0].raw[2]=0x80;state.state.values[0].raw[3]=0x7f;
        e.Check("messages.nonfinite",p.a.Prepare(state,packet)==Error::Payload);
        state=StateMessage();state.state.values[0].raw[4]=1;e.Check("messages.raw-tail-zero",p.a.Prepare(state,packet)==Error::Payload);
        state=StateMessage();state.state.count=2;state.state.values[1]=state.state.values[0];
        e.Check("messages.duplicate-field",p.a.Prepare(state,packet)==Error::Payload);
        state=StateMessage();state.state.status=StateStatus::Gap;e.Check("messages.gap-cannot-have-values",p.a.Prepare(state,packet)==Error::Payload);
        state=StateMessage();state.state.phase=CapturePhase::OddOnly;e.Check("messages.odd-cannot-have-values",p.a.Prepare(state,packet)==Error::Payload);
        e.Check("messages.remote-cannot-send-state",p.b.Prepare(StateMessage(),packet)==Error::Role);
        e.Check("messages.authority-cannot-send-input",p.a.Prepare(InputMessage(),packet)==Error::Role);
        state=StateMessage();state.state.count=kMaximumValues;
        for(unsigned i=0;i<kMaximumValues;++i) {state.state.values[i]=state.state.values[0];state.state.values[i].entity=i+1;}
        e.Check("messages.maximum-values",p.a.Prepare(state,packet)==Error::None && packet.size<=kMaximumPacket && p.a.CommitSent(packet)==Error::None && p.b.Receive(packet.View(),p.decoded)==Error::None && p.decoded.state.count==kMaximumValues);
        e.PacketRow("messages.maximum-values",packet,p.a,p.b,Error::None);
    }
    e.checks.flush();e.packets.flush();return e.failed || !e.checks || !e.packets?1:0;
}
