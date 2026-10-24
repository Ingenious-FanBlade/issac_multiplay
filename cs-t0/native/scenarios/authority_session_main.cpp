// Offline CLI for complete, scripted session/input lifecycles. No game APIs.
// Scenarios and their expected outcomes were specified before the implementation.
#include "isaac_cs_t0/authority_session.hpp"
#include "isaac_cs_t0/coop_input.hpp"
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

using namespace isaac_cs_t0::coop_active;
namespace {
struct Domain {
    AuthoritySession session{};
    std::array<Participant,2> players{};
    std::array<InputCmdDraftV2,2> commands{};
    SessionIdentity identity{};
    std::array<int,6> objects{}; // Opaque binding metadata; never dereferenced.
    Domain() {
        identity.session[0]=17;identity.clients={101,202};
        for(std::uint32_t i=0;i<2;++i)
            players[i].Initialize(identity.session,identity.clients[i],i,&session);
    }
};
void Write(std::ostream& output,const std::string& name,std::size_t step,
    const Domain& domain,bool ok,std::uint64_t sequence,std::uint32_t decision,float value) {
    const auto status=domain.session.Status();
    const auto& identity=domain.session.Identity();
    output<<"{\"scenario\":\""<<name<<"\",\"step\":"<<step
        <<",\"state\":\""<<SessionStateName(status.state)<<"\",\"cause\":\""
        <<SessionCauseName(status.cause)<<"\",\"ok\":"<<(ok?"true":"false")
        <<",\"input_open\":"<<(domain.session.AcceptsInput()?"true":"false")
        <<",\"sequence\":"<<sequence<<",\"decision\":"<<decision<<",\"value\":"<<value
        <<",\"session_byte\":"<<static_cast<unsigned>(identity.session[0])
        <<",\"clients\":["<<identity.clients[0]<<","<<identity.clients[1]<<"]"
        <<",\"depth\":["<<domain.players[0].Depth()<<","<<domain.players[1].Depth()<<"]"
        <<",\"bound\":["<<(domain.players[0].BoundValid()?"true":"false")<<","<<(domain.players[1].BoundValid()?"true":"false")<<"]"
        <<",\"previous\":["<<(domain.players[0].PreviousComparable()?"true":"false")<<","<<(domain.players[1].PreviousComparable()?"true":"false")<<"]}\n";
}
}
int main(int argc,char** argv) {
    if(argc!=3) return 2;
    std::ifstream input(argv[1]);std::ofstream output(argv[2],std::ios::out|std::ios::trunc);
    if(!input || !output) return 2;
    std::unique_ptr<Domain> domain;std::string line,name;std::size_t step=0;
    while(std::getline(input,line)) {
        std::istringstream row(line);std::string op;row>>op;
        if(op=="scenario") {row>>name;domain=std::make_unique<Domain>();step=0;continue;}
        if(!domain) return 2;
        auto& d=*domain;bool ok=true;std::uint64_t sequence=0;std::uint32_t decision=0;float value=0;
        if(op=="start" || op=="restart" || op=="zero-session" || op=="zero-client" || op=="same-client") {
            auto id=d.identity;
            if(op=="restart") {id.session[0]=99;id.clients={303,404};}
            if(op=="zero-session") id.session={};
            if(op=="zero-client") id.clients[0]=0;
            if(op=="same-client") id.clients[1]=id.clients[0];
            ok=d.session.Start(id);
        } else if(op=="prepared") ok=d.session.Prepared();
        else if(op=="activate") ok=d.session.Activate(d.players[0].BoundValid(),d.players[1].BoundValid());
        else if(op=="retire" || op=="teardown") ok=d.session.BeginRetirement(op=="retire"?SessionCause::Stop:SessionCause::RunTeardown);
        else if(op=="finish") {unsigned restored=0;row>>restored;ok=d.session.CompleteRetirement(restored!=0);}
        else if(op=="fail" || op=="controller-lost" || op=="restore-fail") {
            const auto cause=op=="controller-lost"?SessionCause::ControllerLost:
                op=="restore-fail"?SessionCause::RestoreFailure:SessionCause::RuntimeFailure;
            ok=d.session.Fail(cause);
        } else if(op=="race") {
            std::atomic<bool> go=false;
            std::thread failure([&]{while(!go.load(std::memory_order_acquire)) std::this_thread::yield();d.session.Fail(SessionCause::RuntimeFailure);});
            std::thread stop([&]{while(!go.load(std::memory_order_acquire)) std::this_thread::yield();d.session.BeginRetirement(SessionCause::Stop);});
            go.store(true,std::memory_order_release);failure.join();stop.join();
        } else if(op=="status") {}
        else {
            std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1) return 2;
            auto& player=d.players[ordinal];
            if(op=="bind") {
                const auto offset=ordinal*3;
                ok=player.Bind({&d.objects[offset],&d.objects[offset+1],&d.objects[offset+2],static_cast<std::int32_t>(ordinal)});
            } else if(op=="make") {
                InputStateDraftV2 state{};state.move_x=32767;state.edge_flags=1;
                d.commands[ordinal]=player.MakeCommand(state,1);sequence=d.commands[ordinal].sequence;ok=sequence!=0;
            } else if(op=="packet" || op=="wrong-session" || op=="wrong-client" || op=="old-generation") {
                InputCmdDraftV2 command{};command.session_id=d.identity.session;
                command.client_id=d.identity.clients[ordinal];command.sequence=1;command.target_tick=1;
                command.state.move_x=32767;command.state.edge_flags=1;
                if(op=="wrong-session") command.session_id[0]=99;
                if(op=="wrong-client") command.client_id=ordinal?101:202;
                EncodedInputCmdV2 bytes{};InputCmdDraftV2 decoded{};
                if(!Encode(command,bytes) || !Decode(bytes,decoded)) return 2;
                const auto generation=player.Generation()-(op=="old-generation"?1:0);
                decision=static_cast<std::uint32_t>(player.AcceptInput(decoded,generation,0));
                ok=decision==static_cast<std::uint32_t>(Decision::Accepted);
            } else if(op=="submit") {
                EncodedInputCmdV2 bytes{};InputCmdDraftV2 decoded{};
                if(!Encode(d.commands[ordinal],bytes) || !Decode(bytes,decoded)) return 2;
                decision=static_cast<std::uint32_t>(player.AcceptInput(decoded,player.Generation(),0));
                ok=decision==static_cast<std::uint32_t>(Decision::Accepted);
            } else if(op=="resolve") {
                std::uint64_t tick=0;row>>tick;ok=player.Begin(tick);
                value=player.Read(1,Query::Scalar,Consumer::Movement,player.Bound(),tick);
            } else if(op=="read") {
                std::uint64_t tick=0;row>>tick;
                value=player.Read(1,Query::Scalar,Consumer::Movement,player.Bound(),tick);
            } else if(op=="edge") {
                std::uint64_t tick=0;row>>tick;
                value=player.Read(8,Query::Triggered,Consumer::Bomb,player.Bound(),tick);
            } else if(op=="end") player.End(true);
            else if(op=="invalidate") player.Invalidate();
            else if(op=="retire-player") player.Retire();
            else if(op=="reinitialize") player.Initialize(d.identity.session,d.identity.clients[ordinal],ordinal,&d.session);
            else return 2;
        }
        Write(output,name,++step,d,ok,sequence,decision,value);
        if(!output) return 3;
    }
    output.flush();return output?0:3;
}
