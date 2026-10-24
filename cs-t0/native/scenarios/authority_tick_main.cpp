// Scripted offline lifecycles through the production session/clock/codec/queue.
// Expected outcomes and failure cases precede the clock implementation.
#include "isaac_cs_t0/authority_session.hpp"
#include "isaac_cs_t0/authority_tick.hpp"
#include "isaac_cs_t0/coop_input.hpp"
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

using namespace isaac_cs_t0::coop_active;
namespace {
struct Domain {
    AuthoritySession session{};
    AuthorityTickClock clock;
    std::array<Participant,2> players{};
    SessionIdentity identity{};
    std::array<int,6> objects{};
    bool blocked=false;
    explicit Domain(std::uint64_t initial):clock(initial) {
        identity.session[0]=23;identity.clients={101,202};
        for(std::uint32_t i=0;i<2;++i) players[i].Initialize(identity.session,identity.clients[i],i,&session);
    }
    void CloseInput(bool normal) {for(auto& player:players) player.End(normal);}
};
TickBoundary Boundary(const std::string& source,std::uint64_t loop) {
    TickBoundary boundary{};boundary.loop=loop;boundary.thread=7;boundary.main_thread=7;boundary.owner=100;
    boundary.kind=source=="odd"?TickKind::OddPlayer:TickKind::StandardRoom;
    boundary.return_rva=source=="odd"?0x555246u:0x2FBA52u;
    boundary.player_return_rva=source=="odd"?0x2FD60Cu:0;
    if(source=="fsm") {boundary.kind=TickKind::OddPlayer;boundary.return_rva=0x555246;boundary.player_return_rva=0x2FD4A9;}
    if(source=="lifecycle") boundary.return_rva=0x52FC7C;
    if(source=="door") boundary.return_rva=0x2FE381;
    if(source=="wrong-thread") boundary.thread=8;
    if(source=="changed-thread") {boundary.thread=8;boundary.main_thread=8;}
    if(source=="wrong-owner") boundary.owner=200;
    if(source=="no-owner") boundary.owner=0;
    return boundary;
}
void Write(std::ostream& output,const std::string& scenario,std::size_t step,const Domain& d,
    bool ok,TickDecision decision,std::uint32_t input_decision,float value) {
    output<<"{\"scenario\":\""<<scenario<<"\",\"step\":"<<step<<",\"ok\":"<<(ok?"true":"false")
        <<",\"decision\":\""<<TickDecisionName(decision)<<"\",\"input_decision\":"<<input_decision
        <<",\"session\":\""<<SessionStateName(d.session.Status().state)<<"\",\"completed\":\""<<d.clock.Completed()
        <<"\",\"current\":\""<<d.clock.Current()<<"\",\"closed\":"<<(d.clock.Closed()?"true":"false")
        <<",\"value\":"<<value<<",\"depth\":["<<d.players[0].Depth()<<","<<d.players[1].Depth()<<"]"
        <<",\"previous\":["<<(d.players[0].PreviousComparable()?"true":"false")<<","<<(d.players[1].PreviousComparable()?"true":"false")<<"]}\n";
}
}
int main(int argc,char** argv) {
    if(argc!=3) return 2;
    std::ifstream input(argv[1]);std::ofstream output(argv[2]);if(!input || !output) return 2;
    std::unique_ptr<Domain> domain;std::string line,name;std::size_t step=0;
    while(std::getline(input,line)) {
        std::istringstream row(line);std::string op;row>>op;
        if(op=="scenario") {std::uint64_t initial=0;row>>name>>initial;domain=std::make_unique<Domain>(initial);step=0;continue;}
        if(!domain) return 2;
        auto& d=*domain;bool ok=true;auto decision=TickDecision::Idle;std::uint32_t input_decision=0;float value=0;
        if(op=="warmup") {
            ok=d.session.Start(d.identity) && d.session.Prepared();
            for(std::uint32_t i=0;i<2;++i) {
                const auto offset=i*3;
                ok=d.players[i].Bind({&d.objects[offset],&d.objects[offset+1],&d.objects[offset+2],static_cast<std::int32_t>(i)}) && ok;
            }
        } else if(op=="activate") ok=d.session.Activate(d.players[0].BoundValid(),d.players[1].BoundValid());
        else if(op=="pause" || op=="loading" || op=="menu") d.blocked=true;
        else if(op=="resume") d.blocked=false;
        else if(op=="stop") ok=d.session.BeginRetirement(SessionCause::Stop);
        else if(op=="fail") ok=d.session.Fail(SessionCause::RuntimeFailure);
        else if(op=="idle" || op=="render" || op=="physical-poll" || op=="early-odd-return") {}
        else if(op=="top") {
            std::uint64_t loop=0;row>>loop;decision=d.clock.EndOuter(loop,d.session.AcceptsInput());
            ok=decision==TickDecision::Idle;
            if(decision!=TickDecision::Idle) {
                d.CloseInput(false);
                if(decision!=TickDecision::Cancelled) d.session.Fail(SessionCause::RuntimeFailure);
            }
        } else if(op=="begin" || op=="return" || op=="abnormal-return") {
            std::string source;std::uint64_t loop=0;row>>source>>loop;const auto boundary=Boundary(source,loop);
            if(op=="begin") {
                decision=d.clock.Begin(boundary,d.session.AcceptsInput(),d.blocked);ok=decision==TickDecision::Started;
                if(ok) for(auto& player:d.players) if(!player.Retired() && !player.Begin(d.clock.Current())) return 3;
            } else {
                decision=d.clock.Complete(boundary,op=="return",d.session.AcceptsInput());
                ok=decision==TickDecision::Committed;d.CloseInput(ok);
            }
            if(d.clock.Closed()) {
                d.CloseInput(false);
                if(decision!=TickDecision::Cancelled) d.session.Fail(SessionCause::RuntimeFailure);
            }
        } else {
            std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1) return 2;auto& player=d.players[ordinal];
            if(op=="input") {
                InputStateDraftV2 state{};state.move_x=32767;state.edge_flags=1;
                const auto command=player.MakeCommand(state,d.clock.Completed()+1);
                EncodedInputCmdV2 bytes{};InputCmdDraftV2 decoded{};
                if(!Encode(command,bytes) || !Decode(bytes,decoded)) return 3;
                input_decision=static_cast<std::uint32_t>(player.AcceptInput(decoded,player.Generation(),d.clock.Completed()));
                ok=input_decision==static_cast<std::uint32_t>(Decision::Accepted);
            } else if(op=="read") value=player.Read(1,Query::Scalar,Consumer::Movement,player.Bound(),d.clock.Current());
            else if(op=="edge") value=player.Read(8,Query::Triggered,Consumer::Bomb,player.Bound(),d.clock.Current());
            else if(op=="generation-change") player.Invalidate();
            else if(op=="retire-player") player.Retire();
            else return 2;
        }
        Write(output,name,++step,d,ok,decision,input_decision,value);if(!output) return 4;
    }
    output.flush();return output?0:4;
}
