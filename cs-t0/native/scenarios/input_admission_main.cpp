// Offline end-to-end admission lifecycles; expectations precede the production change.
#include "isaac_cs_t0/authority_session.hpp"
#include "isaac_cs_t0/authority_tick.hpp"
#include "isaac_cs_t0/coop_input.hpp"
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <iomanip>
#include <span>

using namespace isaac_cs_t0::coop_active;
namespace {
const char* DecisionName(std::size_t decision) {
    constexpr const char* names[]{"None","Accepted","Duplicate","WrongSession","WrongClient","Invalid","Stale",
        "Expired","TooFar","Full","EdgeConflict","SequenceConflict","WrongGeneration","Retired","SessionInactive"};
    return decision<15?names[decision]:"UNKNOWN";
}
struct Domain {
    AuthoritySession session{};AuthorityTickClock clock;
    std::array<Participant,2> players{};SessionIdentity identity{};
    std::array<InputCmdDraftV2,2> generated{};std::array<int,6> objects{};
    std::ostream& admissions;std::string scenario;std::uint64_t arrival=0;
    Domain(std::uint64_t initial,std::ostream& log,const std::string& name):clock(initial),admissions(log),scenario(name) {
        identity.session[0]=29;identity.clients={101,202};
        for(std::uint32_t i=0;i<2;++i) players[i].Initialize(identity.session,identity.clients[i],i,&session);
    }
    bool Bind(std::uint32_t ordinal) {
        const auto offset=ordinal*3;
        return players[ordinal].Bind({&objects[offset],&objects[offset+1],&objects[offset+2],static_cast<std::int32_t>(ordinal)});
    }
    Decision Deliver(std::uint32_t ordinal,const InputCmdDraftV2& command,std::uint64_t generation,
                     bool typed=false,const std::string& mode="") {
        std::array<std::uint8_t,65> wire{};std::memcpy(wire.data(),&command,sizeof(command));
        const std::size_t length=mode=="short"?63:mode=="header-short"?39:mode=="empty"?0:mode=="trailing"?65:64;
        const auto decision=typed?players[ordinal].AcceptInput(command,generation,clock.Completed()):
            players[ordinal].AcceptInput(std::span<const std::uint8_t>(wire.data(),length),generation,clock.Completed());
        admissions<<"{\"scenario\":\""<<scenario<<"\",\"arrival\":\""<<++arrival
            <<"\",\"ordinal\":"<<ordinal<<",\"transport\":\""<<(typed?"local-loopback":"remote-bytes")
            <<"\",\"generation\":\""<<generation<<"\",\"completed\":\""<<clock.Completed()
            <<"\",\"sequence\":\""<<command.sequence<<"\",\"target\":\""<<command.target_tick
            <<"\",\"length\":"<<length<<",\"bytes_le\":\"";
        for(std::size_t i=0;i<length;++i)admissions<<std::hex<<std::setfill('0')<<std::setw(2)<<static_cast<unsigned>(wire[i]);
        admissions<<std::dec<<"\",\"decision\":\""<<DecisionName(static_cast<std::size_t>(decision))<<"\"}\n";
        return decision;
    }
};
TickBoundary Boundary(bool odd,std::uint64_t loop) {
    TickBoundary b{};b.kind=odd?TickKind::OddPlayer:TickKind::StandardRoom;b.loop=loop;
    b.thread=7;b.main_thread=7;b.owner=100;b.return_rva=odd?0x555246u:0x2FBA52u;b.player_return_rva=odd?0x2FD60Cu:0;
    return b;
}
void Snapshot(std::ostream& out,const ClientInputProgress& p) {
    out<<"{\"client\":\""<<p.client<<"\",\"generation\":\""<<p.generation<<"\",\"ordinal\":"<<p.ordinal
        <<",\"next_sequence\":\""<<p.next_sequence<<"\",\"last_accepted_sequence\":\""<<p.last_accepted_sequence
        <<"\",\"last_accepted_target_tick\":\""<<p.last_accepted_target_tick
        <<"\",\"last_received_tick\":\""<<p.last_received_tick<<"\",\"last_received_sequence\":\""<<p.last_received_sequence
        <<"\",\"last_received_target_tick\":\""<<p.last_received_target_tick<<"\",\"last_received_generation\":\""<<p.last_received_generation
        <<"\",\"last_received_decision\":\""<<DecisionName(p.last_received_decision)
        <<"\",\"last_resolved_tick\":\""<<p.last_resolved_tick<<"\",\"last_resolved_sequence\":\""<<p.last_resolved_sequence
        <<"\",\"last_processed_tick\":\""<<p.last_processed_tick<<"\",\"last_processed_sequence\":\""<<p.last_processed_sequence
        <<"\",\"last_processed_source_sequence\":\""<<p.last_processed_source_sequence
        <<"\",\"last_state_tick\":\""<<p.last_state_tick<<"\",\"last_state_generation\":\""<<p.last_state_generation
        <<"\",\"future_first_tick\":\""<<p.future_first_tick<<"\",\"future_last_tick\":\""<<p.future_last_tick
        <<"\",\"submit_count\":\""<<p.submit_count<<"\",\"received_count\":\""<<p.received_count
        <<"\",\"accepted_count\":\""<<p.accepted_count<<"\",\"duplicate_count\":\""<<p.duplicate_count
        <<"\",\"rejected_count\":\""<<p.rejected_count<<"\",\"expired_before_resolve\":\""<<p.expired_before_resolve
        <<"\",\"discarded_on_invalidate\":\""<<p.discarded_on_invalidate
        <<"\",\"current_valid\":"<<(p.current_valid?"true":"false")<<",\"last_state_valid\":"<<(p.last_state_valid?"true":"false")
        <<",\"future_window_valid\":"<<(p.future_window_valid?"true":"false")<<",\"scope_pending\":"<<(p.scope_pending?"true":"false")
        <<",\"has_received\":"<<(p.has_received?"true":"false")<<",\"retired\":"<<(p.retired?"true":"false")
        <<",\"counters_saturated\":"<<(p.counters_saturated?"true":"false")
        <<",\"last_state\":{\"move_x\":"<<p.last_state.move_x<<",\"held_flags\":"<<p.last_state.held_flags<<",\"edge_flags\":"<<p.last_state.edge_flags<<"}"
        <<",\"current_state\":{\"move_x\":"<<p.current_state.move_x<<",\"edge_flags\":"<<p.current_state.edge_flags<<"}"
        <<",\"resolved_contributors\":"<<p.resolved_contributors<<",\"processed_contributors\":"<<p.processed_contributors
        <<",\"rejections\":{";
    for(std::size_t i=1;i<p.rejections.size();++i) out<<(i==1?"":",")<<'"'<<DecisionName(i)<<"\":\""<<p.rejections[i]<<'"';
    out<<"},\"edges\":[";
    for(std::size_t i=0;i<p.edges.size();++i) {
        const auto& e=p.edges[i];if(i)out<<',';
        out<<"{\"accepted_sequence\":\""<<e.accepted_sequence<<"\",\"accepted_tick\":\""<<e.accepted_tick<<"\",\"accepted_generation\":\""<<e.accepted_generation
            <<"\",\"resolved_sequence\":\""<<e.resolved_sequence<<"\",\"resolved_tick\":\""<<e.resolved_tick<<"\",\"resolved_generation\":\""<<e.resolved_generation
            <<"\",\"processed_sequence\":\""<<e.processed_sequence<<"\",\"processed_tick\":\""<<e.processed_tick<<"\",\"processed_generation\":\""<<e.processed_generation
            <<"\",\"processed_count\":\""<<e.processed_count<<"\"}";
    }
    out<<"]}";
}
}
int main(int argc,char** argv) {
    if(argc!=5) return 2;
    std::ifstream input(argv[1]);std::ofstream out(argv[2]),batches(argv[3]),admissions(argv[4]);if(!input || !out || !batches || !admissions) return 2;
    std::unique_ptr<Domain> domain;std::string line,name;std::size_t step=0;
    while(std::getline(input,line)) {
        std::istringstream row(line);std::string op;row>>op;
        if(op=="scenario") {std::uint64_t initial=0;row>>name>>initial;domain=std::make_unique<Domain>(initial,admissions,name);step=0;continue;}
        if(!domain) return 2;
        auto& d=*domain;bool ok=true,unchanged=true;float value=0;auto decision=Decision::Accepted;
        auto tick_decision=TickDecision::Idle;std::uint64_t made_sequence=0,made_target=0;
        if(op=="warmup") {ok=d.session.Start(d.identity) && d.session.Prepared();ok=d.Bind(0) && d.Bind(1) && ok;}
        else if(op=="activate") ok=d.session.Activate(d.players[0].BoundValid(),d.players[1].BoundValid());
        else if(op=="fail-session") ok=d.session.Fail(SessionCause::RuntimeFailure);
        else if(op=="retire-session") ok=d.session.BeginRetirement(SessionCause::Stop);
        else if(op=="status") {}
        else if(op=="begin" || op=="return" || op=="abort") {
            std::string kind;std::uint64_t loop=0;row>>kind>>loop;const auto boundary=Boundary(kind=="odd",loop);
            if(op=="begin") {
                tick_decision=d.clock.Begin(boundary,d.session.AcceptsInput(),false);ok=tick_decision==TickDecision::Started;
                if(ok)for(auto& player:d.players)if(!player.Retired() && !player.Begin(d.clock.Current())) return 3;
            } else {
                tick_decision=d.clock.Complete(boundary,op=="return",d.session.AcceptsInput());ok=tick_decision==TickDecision::Committed;
                for(auto& player:d.players)player.End(ok);
            }
            if(d.clock.Closed() && tick_decision!=TickDecision::Cancelled)d.session.Fail(SessionCause::RuntimeFailure);
        } else {
            std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1)return 2;auto& player=d.players[ordinal];
            if(op=="packet" || op=="local-packet" || op=="make") {
                std::uint64_t sequence=0,target=0;std::uint32_t edges=0;std::int32_t move=0;std::string mode;
                if(op!="make")row>>sequence;
                row>>target>>edges>>move>>mode;
                InputStateDraftV2 state{};state.move_x=static_cast<std::int16_t>(move);state.edge_flags=edges;
                if(mode=="held-item")state.held_flags=kHeldItem;
                if(op=="make") {d.generated[ordinal]=player.MakeCommand(state,target);made_sequence=d.generated[ordinal].sequence;made_target=d.generated[ordinal].target_tick;ok=made_sequence!=0;}
                else {
                    InputCmdDraftV2 command{};command.session_id=d.identity.session;command.client_id=d.identity.clients[ordinal];
                    command.sequence=sequence;command.target_tick=target;command.state=state;
                    if(mode=="wrong-session")command.session_id[0]^=128;
                    if(mode=="wrong-client")command.client_id=d.identity.clients[1-ordinal];
                    if(mode=="malformed")command.state.reserved=1;
                    if(mode=="version")command.state.version=1;
                    if(mode=="encoded-length")command.state.encoded_length=25;
                    if(mode=="held-flags")command.state.held_flags=16;
                    if(mode=="edge-flags")command.state.edge_flags=16;
                    if(mode=="axis-min")command.state.move_x=-32768;
                    if(mode=="aim-mismatch")command.state.held_flags=1;
                    decision=d.Deliver(ordinal,command,player.Generation()-(mode=="old-generation"?1:0),op=="local-packet",mode);
                    ok=decision==Decision::Accepted;
                }
            } else if(op=="submit-made" || op=="submit-remote") {decision=d.Deliver(ordinal,d.generated[ordinal],player.Generation(),op=="submit-made");ok=decision==Decision::Accepted;}
            else if(op=="invalidate")player.Invalidate();
            else if(op=="rebind")ok=d.Bind(ordinal);
            else if(op=="retire")player.Retire();
            else if(op=="end-again")player.End(true);
            // Fault injection into the production API: a client missed an earlier
            // resolve while its accepted queue survived. It is not a native tick.
            else if(op=="resolve-only") {std::uint64_t tick=0;row>>tick;ok=player.Begin(tick);}
            else if(op=="read") {
                std::uint32_t action=0;row>>action;
                auto before=std::make_unique<std::uint8_t[]>(sizeof(Participant));std::memcpy(before.get(),&player,sizeof(Participant));
                const auto consumer=action<4?Consumer::Movement:action==8?Consumer::Bomb:action==9?Consumer::Item:action==10?Consumer::Pill:Consumer::Drop;
                const auto query=action<4?Query::Scalar:Query::Triggered;
                value=player.Read(action,query,consumer,player.Bound(),d.clock.Current());
                const auto again=player.Read(action,query,consumer,player.Bound(),d.clock.Current());
                const auto snapshot=player.Progress(d.clock.Completed());(void)snapshot;
                unchanged=value==again && std::memcmp(before.get(),&player,sizeof(Participant))==0;
            } else if(op=="fill" || op=="fill-local" || op=="history-cycle" || op=="outage") {
                std::uint32_t count=0;row>>count;if(count>5000)return 2;
                for(std::uint32_t i=1;i<=count;++i) {
                    InputCmdDraftV2 command{};command.session_id=d.identity.session;command.client_id=d.identity.clients[ordinal];
                    command.sequence=i;command.target_tick=d.clock.Completed()+1;
                    decision=d.Deliver(ordinal,command,player.Generation(),op=="fill-local" || op=="outage");
                    if(op=="history-cycle" || op=="outage") {
                        const auto boundary=Boundary((i&1)!=0,i);
                        if(d.clock.Begin(boundary,true,false)!=TickDecision::Started)return 3;
                        for(auto& participant:d.players)if(!participant.Retired() && !participant.Begin(d.clock.Current()))return 3;
                        tick_decision=d.clock.Complete(boundary,true,true);if(tick_decision!=TickDecision::Committed)return 3;
                        for(auto& participant:d.players)participant.End(true);
                    }
                    batches<<"{\"scenario\":\""<<name<<"\",\"sequence\":\""<<i<<"\",\"target\":\""<<command.target_tick
                        <<"\",\"decision\":\""<<DecisionName(static_cast<std::size_t>(decision))<<"\",\"completed\":\""<<d.clock.Completed()<<"\"}\n";
                }
                ok=decision==Decision::Accepted;
            } else return 2;
        }
        out<<"{\"scenario\":\""<<name<<"\",\"step\":"<<++step<<",\"ok\":"<<(ok?"true":"false")
            <<",\"unchanged\":"<<(unchanged?"true":"false")<<",\"value\":"<<value<<",\"decision\":\""<<DecisionName(static_cast<std::size_t>(decision))
            <<"\",\"tick_decision\":\""<<TickDecisionName(tick_decision)<<"\",\"session\":\""<<SessionStateName(d.session.Status().state)
            <<"\",\"completed\":\""<<d.clock.Completed()<<"\",\"current\":\""<<d.clock.Current()
            <<"\",\"generated_sequence\":\""<<made_sequence<<"\",\"generated_target\":\""<<made_target<<"\",\"a\":";
        Snapshot(out,d.players[0].Progress(d.clock.Completed()));out<<",\"b\":";Snapshot(out,d.players[1].Progress(d.clock.Completed()));out<<"}\n";
        if(!out || !batches || !admissions)return 4;
    }
    out.flush();batches.flush();admissions.flush();return out && batches && admissions?0:4;
}
