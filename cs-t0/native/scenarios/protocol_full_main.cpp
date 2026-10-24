// Full protocol lifecycle/corpus acceptance, authored before operation 5-9 code.
#include "isaac_cs_t0/protocol_transport.hpp"
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
using namespace isaac_cs_t0::protocol;
using isaac_cs_t0::coop_active::InputCmdDraftV2;
namespace {
Config ConfigFor(Role role) {
    Config c{};c.role=role;c.session[0]=99;c.clients={101,202};c.build=TextId("isaac-t0-tx-02-00");
    c.local_nonce[0]=role==Role::Authority?11:22;c.peer_nonce[0]=role==Role::Authority?22:11;return c;
}
struct Evidence {
    std::ofstream rows,trace;unsigned failed=0;std::string scenario="initial";
    Evidence(const char* a,const char* b):rows(a),trace(b) {}
    void Check(const std::string& name,bool passed) {if(!passed)++failed;rows<<"{\"name\":\""<<name<<"\",\"passed\":"<<(passed?"true":"false")<<"}\n";}
    static bool Record(const TraceRecord& r,void* context) noexcept {
        auto& e=*static_cast<Evidence*>(context);e.trace<<"{\"scenario\":\""<<e.scenario<<"\",\"tick\":"<<r.tick<<",\"token\":"<<r.token<<",\"generation\":"<<r.generation
            <<",\"action\":"<<static_cast<unsigned>(r.action)<<",\"direction\":"<<static_cast<unsigned>(r.direction)<<",\"decision\":"<<r.decision<<",\"bytes\":\"";
        constexpr char digits[]="0123456789abcdef";
        for(auto b:r.packet.View())e.trace<<digits[b>>4]<<digits[b&15];e.trace<<"\"}\n";return bool(e.trace);
    }
};
struct Production {
    isaac_cs_t0::coop_active::AuthoritySession session;
    isaac_cs_t0::coop_active::Participant player;
    std::array<int,3> objects{};unsigned accepted=0,duplicates=0,edges=0;bool valid=true;
    Production() {auto c=ConfigFor(Role::Authority);session.Start({c.session,c.clients});session.Prepared();player.Initialize(c.session,202,1,&session);
        player.Bind({&objects[0],&objects[1],&objects[2],7});session.Activate(true,true);}
    static std::uint32_t Admit(const InputCmdDraftV2& c,std::uint64_t generation,void* context) noexcept {
        auto& p=*static_cast<Production*>(context);const auto d=p.player.AcceptInput(c,generation,p.now);
        if(d==isaac_cs_t0::coop_active::Decision::Accepted)++p.accepted;if(d==isaac_cs_t0::coop_active::Decision::Duplicate)++p.duplicates;
        return static_cast<std::uint32_t>(d);
    }
    std::uint64_t now=0;
};
InputCmdDraftV2 Command(std::uint64_t seq,std::uint64_t tick,bool edge=false) {
    InputCmdDraftV2 c{};c.session_id=ConfigFor(Role::Remote).session;c.client_id=202;c.sequence=seq;c.target_tick=tick;c.state.move_x=4096;c.state.edge_flags=edge?8:0;return c;
}
bool Establish(Endpoint& a,Endpoint& b) {
    Packet ah{},bh{},ack{};Message m{};
    return a.Start(ConfigFor(Role::Authority))==Error::None && b.Start(ConfigFor(Role::Remote))==Error::None &&
        a.PrepareHello(ah)==Error::None && b.PrepareHello(bh)==Error::None && a.CommitSent(ah)==Error::None && b.CommitSent(bh)==Error::None &&
        a.Receive(bh.View(),m)==Error::None && b.Receive(ah.View(),m)==Error::None && a.PrepareFirstAck(ack)==Error::None &&
        a.CommitSent(ack)==Error::None && b.Receive(ack.View(),m)==Error::None;
}
}
int main(int argc,char** argv) {
    if(argc!=5)return 2;Evidence e(argv[2],argv[3]);std::ifstream corpus(argv[1]);if(!corpus||!e.rows||!e.trace)return 2;
    std::string line;while(std::getline(corpus,line)) {
        std::istringstream row(line);std::string name,hex;bool valid=false;row>>name>>valid>>hex;
        std::vector<std::uint8_t> raw;if(hex!="-")for(std::size_t i=0;i<hex.size();i+=2)raw.push_back(static_cast<std::uint8_t>(std::stoul(hex.substr(i,2),nullptr,16)));
        Message out{};out.header.sequence=987;const auto result=Decode(raw,out);Packet encoded{};
        bool passed=valid?(result==Error::None && Encode(out,encoded)==Error::None && encoded.size==raw.size() && std::equal(raw.begin(),raw.end(),encoded.bytes.begin())):
            (result!=Error::None && out.header.sequence==987);
        e.Check(name,passed);
    }
    for(unsigned profile=0;profile<4;++profile) {
        auto link=std::make_unique<ProtocolLink>();Production p;FaultConfig f{};f.seed=0x20008+profile;
        f.delay_ticks=profile?1u:0u;f.jitter_ticks=profile?1u:0u;f.loss_per_mille=profile==1?120u:0u;
        f.duplicate_per_mille=profile==2?1000u:150u;f.reorder_per_mille=profile==2?800u:150u;
        f.outage_start=profile==3?40:0;f.outage_ticks=profile==3?20:0;
        const auto prefix="participant."+std::to_string(profile);
        e.scenario=prefix;
        e.Check(prefix+".handshake",link->Initialize(ConfigFor(Role::Authority),f,&Evidence::Record,&e));
        for(std::uint64_t tick=0;tick<140;++tick) {
            p.now=tick;if(tick<120)e.Check(prefix+".send."+std::to_string(tick),link->SendInput(Command(tick+1,tick+8,tick==10),p.player.Generation(),tick));
            e.Check(prefix+".pump."+std::to_string(tick),link->PumpInputs(tick,64,&Production::Admit,&p));
            p.player.Begin(tick+1);if(p.player.Current().edge_flags&8)++p.edges;p.player.End(true);
            const auto progress=p.player.Progress(tick+1);Ack ack{};ack.server_tick=tick+1;ack.client_id=202;ack.accepted=progress.last_accepted_sequence;
            ack.processed=progress.last_processed_sequence;ack.processed_tick=progress.last_processed_tick;ack.processed_source=progress.last_processed_source_sequence;
            ack.generation=progress.last_state_generation;ack.session_state=isaac_cs_t0::coop_active::SessionState::Active;
            StateObservation state{};state.tick=tick+1;state.run=1;state.room=1;state.attempt=tick+1;state.source_token=tick+1;state.status=StateStatus::Gap;state.reason=15;
            e.Check(prefix+".publication."+std::to_string(tick),link->Publish(ack,state,tick+1));
        }
        e.Check(prefix+".input-accepted",p.accepted>70);e.Check(prefix+".edge-at-most-once",p.edges<=1 && (profile?true:p.edges==1));
        link->Retire(140);e.Check(prefix+".retired",!link->SendInput(Command(200,150),p.player.Generation(),140));
    }
    // Both lanes use independent replay windows; same-lane regressions reject.
    auto link=std::make_unique<ProtocolLink>();FaultConfig f{};f.delay_ticks=0;f.jitter_ticks=0;f.loss_per_mille=0;f.duplicate_per_mille=0;f.reorder_per_mille=0;
    e.scenario="lanes";
    e.Check("lanes.handshake",link->Initialize(ConfigFor(Role::Authority),f,&Evidence::Record,&e));
    Ack ack{};ack.server_tick=10;ack.client_id=202;ack.session_state=isaac_cs_t0::coop_active::SessionState::Active;
    StateObservation state{};state.tick=9;state.run=1;state.room=1;state.attempt=1;state.source_token=1;state.status=StateStatus::Gap;state.reason=15;
    e.Check("lanes.cross-order",link->Publish(ack,state,10));Message latest{};
    e.Check("lanes.latest-ack",link->Remote().Latest(Kind::Ack,latest)==Error::None && latest.ack.server_tick==10);
    e.Check("lanes.latest-state",link->Remote().Latest(Kind::StateHash,latest)==Error::None && latest.state.tick==9);
    link->Retire(10);
    {
        Endpoint a,b;e.Check("progress.handshake",Establish(a,b));Message m{};m.header.kind=Kind::Ack;
        m.ack.client_id=202;m.ack.server_tick=10;m.ack.accepted=2;m.ack.processed=1;m.ack.processed_tick=10;m.ack.generation=2;
        m.ack.session_state=isaac_cs_t0::coop_active::SessionState::Active;m.ack.rejected_count=3;m.ack.rejection_mask=(1u<<7)|(1u<<8);
        Packet wire{};Message out{};
        e.Check("progress.initial",a.Prepare(m,wire)==Error::None && a.CommitSent(wire)==Error::None && b.Receive(wire.View(),out)==Error::None);
        for(unsigned field=0;field<7;++field) {
            auto bad=m;
            if(field==0){bad.ack.server_tick=9;bad.ack.processed_tick=9;}
            if(field==1)bad.ack.accepted=1;
            if(field==2){bad.ack.processed=0;bad.ack.processed_tick=0;}
            if(field==3)bad.ack.processed_tick=9;
            if(field==4)bad.ack.generation=1;
            if(field==5){bad.ack.rejected_count=2;}
            if(field==6){bad.ack.rejected_count=4;bad.ack.rejection_mask=1u<<7;}
            e.Check("progress.regression."+std::to_string(field),a.Prepare(bad,wire)==Error::None && a.CommitSent(wire)==Error::None && b.Receive(wire.View(),out)==Error::Sequence);
        }
        m.ack.server_tick=11;e.Check("progress.valid-after-reject",a.Prepare(m,wire)==Error::None && a.CommitSent(wire)==Error::None && b.Receive(wire.View(),out)==Error::None);
        Message s{};s.header.kind=Kind::StateHash;s.state=state;
        e.Check("progress.state-first",a.Prepare(s,wire)==Error::None && a.CommitSent(wire)==Error::None && b.Receive(wire.View(),out)==Error::None);
        s.state.tick=8;e.Check("progress.state-regression",a.Prepare(s,wire)==Error::None && a.CommitSent(wire)==Error::None && b.Receive(wire.View(),out)==Error::Sequence);
        e.Check("progress.state-unchanged",b.Latest(Kind::StateHash,out)==Error::None && out.state.tick==9);
    }
    {
        e.scenario="generation";auto p=std::make_unique<Production>();auto g=std::make_unique<ProtocolLink>();auto c=f;c.delay_ticks=2;
        e.Check("generation.handshake",g->Initialize(ConfigFor(Role::Authority),c,&Evidence::Record,&e));
        e.Check("generation.old-send",g->SendInput(Command(1,8,true),p->player.Generation(),0));p->player.Invalidate();
        p->player.Bind({&p->objects[0],&p->objects[1],&p->objects[2],7});p->now=3;
        e.Check("generation.old-rejected-before-admission",g->PumpInputs(3,64,&Production::Admit,p.get(),p->player.Generation()) && p->accepted==0 && p->player.Depth()==0);
        e.Check("generation.new-send",g->SendInput(Command(2,12),p->player.Generation(),4));p->now=7;
        e.Check("generation.new-accepted",g->PumpInputs(7,64,&Production::Admit,p.get(),p->player.Generation()) && p->accepted==1);
        g->Retire(8);
    }
    {
        e.scenario="budget";auto p=std::make_unique<Production>();auto g=std::make_unique<ProtocolLink>();auto c=f;c.delay_ticks=4;
        e.Check("budget.handshake",g->Initialize(ConfigFor(Role::Authority),c,&Evidence::Record,&e));
        for(std::uint64_t i=0;i<4;++i)e.Check("budget.source."+std::to_string(i),g->SendInput(Command(i+1,i+8),p->player.Generation(),i));
        p->now=4;e.Check("budget.no-partial-admission",g->PumpInputs(4,3,&Production::Admit,p.get()) && p->accepted==0);
        p->now=7;e.Check("budget.resume",g->PumpInputs(7,64,&Production::Admit,p.get()) && p->accepted==4);g->Retire(8);
    }
    {
        e.scenario="capacity";auto transport=std::make_unique<FaultInjectionTransport>();Endpoint sender;Packet wire{};
        e.Check("capacity.prepare",sender.Start(ConfigFor(Role::Remote))==Error::None && sender.PrepareHello(wire)==Error::None && transport->Initialize(f,&Evidence::Record,&e));
        bool filled=true;for(std::size_t i=0;i<FaultInjectionTransport::kCapacity;++i)filled=transport->Send(Direction::ToAuthority,wire,1,0,true)==TransportResult::Queued && filled;
        e.Check("capacity.bounded",filled && transport->Send(Direction::ToAuthority,wire,1,0,true)==TransportResult::Full);
        const auto before=wire;sender.CancelPending();
        e.Check("capacity.failed-send-no-sequence-consumption",sender.PrepareHello(wire)==Error::None && wire.size==before.size && wire.bytes==before.bytes);
        transport->Close(1);
    }
    e.Check("crc.standard",PacketCrc(std::span<const std::uint8_t>{reinterpret_cast<const std::uint8_t*>("123456789"),9})==0xcbf43926u);
    std::ofstream summary(argv[4]);summary<<"{\"failed\":"<<e.failed<<",\"game_access\":false}\n";
    return e.failed||!e.rows||!e.trace||!summary?1:0;
}
