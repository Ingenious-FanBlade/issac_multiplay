// T0-07 operation 8 offline production fault and boundary matrix.
// Scenarios and expectations are authored before production test-support changes.
#include "isaac_cs_t0/authority_tick_pipeline.hpp"
#include <array>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
using namespace isaac_cs_t0::coop_active;

namespace {
const char* Bool(bool value) noexcept {return value?"true":"false";}
const char* AdmissionName(Decision decision) noexcept
{
    switch(decision) {
    case Decision::Accepted:return "Accepted";
    case Decision::Duplicate:return "Duplicate";
    case Decision::WrongSession:return "WrongSession";
    case Decision::WrongClient:return "WrongClient";
    case Decision::Invalid:return "Invalid";
    case Decision::Stale:return "Stale";
    case Decision::Expired:return "Expired";
    case Decision::TooFar:return "TooFar";
    case Decision::Full:return "Full";
    case Decision::EdgeConflict:return "EdgeConflict";
    case Decision::SequenceConflict:return "SequenceConflict";
    case Decision::WrongGeneration:return "WrongGeneration";
    case Decision::Retired:return "Retired";
    case Decision::SessionInactive:return "SessionInactive";
    }
    return "UNKNOWN";
}
TickBoundary Boundary(const std::string& kind,std::uint64_t loop)
{
    TickBoundary boundary{};boundary.loop=loop;boundary.thread=7;boundary.main_thread=7;boundary.owner=100;
    if(kind=="odd") {boundary.kind=TickKind::OddPlayer;boundary.return_rva=0x555246u;boundary.player_return_rva=0x2FD60Cu;}
    else {boundary.kind=TickKind::StandardRoom;boundary.return_rva=0x2FBA52u;}
    return boundary;
}
struct Domain {
    AuthoritySession session{};AuthorityTickClock clock;std::array<Participant,2> players{};
    AuthorityTickPipeline flow;SessionIdentity identity{};std::array<int,6> objects{};
    InputRuntimeConfig config{};bool config_ok=false;
    std::uint64_t body_calls=0,publications=0,next_loop=1;TickBoundary boundary{};
    Domain(std::uint64_t initial,std::uint64_t source_sequence_start):
        clock(initial),flow(session,clock,players)
    {
        identity.session[0]=41;identity.clients={101,202};
        config_ok=clock.ConfigureFutureWindow(config.future_window_ticks);
        config_ok=flow.Configure(config)&&config_ok;
        for(std::uint32_t i=0;i<2;++i) {
            config_ok=players[i].Configure(config)&&config_ok;
            players[i].Initialize(identity.session,identity.clients[i],i,&session,
                i==0?source_sequence_start:1);
        }
    }
    bool Bind(std::uint32_t ordinal)
    {
        return players[ordinal].Bind({&objects[ordinal*3],&objects[ordinal*3+1],
            &objects[ordinal*3+2],static_cast<std::int32_t>(ordinal)});
    }
    TickDecision StartTick(const std::string& kind)
    {
        boundary=Boundary(kind,next_loop++);
        return flow.BeginDrain(boundary,false);
    }
    bool FinishTick()
    {
        if(flow.Status().phase==TickPipelinePhase::Draining &&
           flow.BeginSimulation()!=TickDecision::Started) return false;
        if(flow.Status().phase!=TickPipelinePhase::Simulating) return false;
        ++body_calls;
        if(flow.NativeReturned(boundary,true)!=TickDecision::Committed) return false;
        TickPostState post{};post.status=PostStateStatus::Complete;post.tick=flow.Status().candidate;
        post.attempt=700;post.capture=500;
        if(flow.FinishPostState(boundary.loop,post)!=TickDecision::Committed) return false;
        if(!flow.PublishCompleted(true)) return false;
        ++publications;return flow.EndOuter(boundary.loop)==TickDecision::Idle;
    }
};
void Player(std::ostream& out,const Participant& player,std::uint64_t completed)
{
    const auto value=player.Progress(completed);
    out<<"{\"generation\":\""<<value.generation
       <<"\",\"next_sequence\":\""<<value.next_sequence
       <<"\",\"last_accepted\":\""<<value.last_accepted_sequence
       <<"\",\"accepted_count\":\""<<value.accepted_count
       <<"\",\"received_count\":\""<<value.received_count
       <<"\",\"duplicates\":\""<<value.duplicate_count
       <<"\",\"rejected\":\""<<value.rejected_count
       <<"\",\"wrong_session\":\""<<value.rejections[3]
       <<"\",\"wrong_client\":\""<<value.rejections[4]
       <<"\",\"invalid\":\""<<value.rejections[5]
       <<"\",\"stale\":\""<<value.rejections[6]
       <<"\",\"expired\":\""<<value.rejections[7]
       <<"\",\"too_far\":\""<<value.rejections[8]
       <<"\",\"full\":\""<<value.rejections[9]
       <<"\",\"edge_conflict\":\""<<value.rejections[10]
       <<"\",\"sequence_conflict\":\""<<value.rejections[11]
       <<"\",\"wrong_generation\":\""<<value.rejections[12]
       <<"\",\"last_received_decision\":\""<<AdmissionName(static_cast<Decision>(value.last_received_decision))
       <<"\",\"last_processed_tick\":\""<<value.last_processed_tick
       <<"\",\"last_processed_sequence\":\""<<value.last_processed_sequence
       <<"\",\"last_processed_source\":\""<<value.last_processed_source_sequence
       <<"\",\"visible\":"<<Bool(value.current_valid)
       <<",\"last_state_valid\":"<<Bool(value.last_state_valid)
       <<",\"retired\":"<<Bool(value.retired)
       <<",\"depth\":"<<player.Depth()
       <<R"(,"edge0_accepted_tick":")"<<value.edges[0].accepted_tick
       <<R"(","edge0_processed":")"<<value.edges[0].processed_count
       <<R"(","edge0_processed_tick":")"<<value.edges[0].processed_tick
       <<"\",\"availability\":{\"source\":\""<<SourceActivityName(value.availability.source)
       <<"\",\"silent_ticks\":\""<<value.availability.silent_ticks
       <<"\",\"timeout_entries\":\""<<value.availability.timeout_entries
       <<"\",\"timeout_recoveries\":\""<<value.availability.timeout_recoveries
       <<"\",\"valid_arrival_count\":\""<<value.availability.valid_arrival_count
       <<"\",\"missing_reason\":\""<<MissingInputReasonName(value.availability.missing_reason)
       <<"\"}}";
}
void Snapshot(std::ostream& out,const Domain& domain)
{
    const auto status=domain.flow.Status();const auto& receipt=domain.flow.Receipt();
    const auto session=domain.session.Status();
    out<<"\"config_ok\":"<<Bool(domain.config_ok)
       <<",\"phase\":\""<<TickPipelinePhaseName(status.phase)
       <<"\",\"completed\":\""<<domain.clock.Completed()
       <<"\",\"current\":\""<<domain.clock.Current()
       <<"\",\"candidate\":\""<<status.candidate
       <<"\",\"drained\":"<<status.drained
       <<",\"budget\":"<<status.budget
       <<",\"session\":\""<<SessionStateName(session.state)
       <<"\",\"session_cause\":\""<<SessionCauseName(session.cause)
       <<"\",\"body_calls\":\""<<domain.body_calls
       <<"\",\"publications\":\""<<domain.publications
       <<"\",\"receipt\":{\"tick\":\""<<receipt.tick
       <<"\",\"kind\":"<<static_cast<std::uint32_t>(receipt.kind)
       <<",\"post\":"<<static_cast<std::uint32_t>(receipt.post.status)
       <<",\"reason\":"<<receipt.post.reason
       <<",\"capture\":\""<<receipt.post.capture
       <<"\",\"published\":"<<Bool(receipt.published)
       <<"},\"a\":";
    Player(out,domain.players[0],domain.clock.Completed());out<<",\"b\":";
    Player(out,domain.players[1],domain.clock.Completed());
}
InputCmdDraftV2 BuildCommand(const Domain& domain,std::uint32_t ordinal,std::uint64_t sequence,
    std::uint64_t target,std::uint32_t edges,std::uint32_t held,std::int32_t move,const std::string& mode)
{
    InputCmdDraftV2 command{};
    command.session_id=domain.identity.session;command.client_id=domain.identity.clients[ordinal];
    command.sequence=sequence;command.target_tick=target;
    command.state.edge_flags=edges;command.state.held_flags=held;
    command.state.move_x=static_cast<std::int16_t>(move);
    if(mode=="malformed")command.state.reserved=1;
    else if(mode=="bad-version")command.state.version=3;
    else if(mode=="bad-length")command.state.encoded_length=23;
    else if(mode=="bad-edge")command.state.edge_flags=16;
    else if(mode=="wrong-client")command.client_id=domain.identity.clients[1-ordinal];
    else if(mode=="wrong-session")command.session_id[0]^=0x80;
    return command;
}
}
int main(int argc,char** argv)
{
    if(argc!=3)return 2;
    std::ifstream input(argv[1]);std::ofstream output(argv[2]);
    if(!input||!output)return 2;
    std::unique_ptr<Domain> domain;std::string line,name;std::size_t step=0;
    while(std::getline(input,line)) {
        std::istringstream row(line);std::string op;row>>op;
        if(op=="scenario") {
            std::uint64_t initial=0,sequence_start=1;
            row>>name>>initial>>sequence_start;
            domain=std::make_unique<Domain>(initial,sequence_start);step=0;continue;
        }
        if(!domain)return 2;auto& d=*domain;
        auto decision=TickDecision::Idle;auto admission=Decision::Accepted;bool ok=true;float value=0.0F;
        std::uint64_t generated_sequence=0,generated_target=0;
        bool source_retired=false,has_source=false;
        std::uint64_t fill_attempts=0,fill_accepted=0;auto fill_last=Decision::Accepted;bool has_fill=false;
        std::uint64_t history_count=0,history_last_accepted=0;auto history_stale=Decision::Accepted,history_new=Decision::Accepted;bool has_history=false;
        std::uint32_t remaining_attempts=0;bool has_remaining=false;
        if(op=="warmup") {
            ok=d.session.Start(d.identity)&&d.session.Prepared();
            ok=d.Bind(0)&&d.Bind(1)&&ok;
        } else if(op=="activate") {
            ok=d.session.Activate(d.players[0].BoundValid(),d.players[1].BoundValid());
        } else if(op=="begin") {
            std::string kind;row>>kind;decision=d.StartTick(kind);ok=decision==TickDecision::Started;
        } else if(op=="resolve") {
            decision=d.flow.BeginSimulation();ok=decision==TickDecision::Started;
        } else if(op=="returned") {
            std::string mode;row>>mode;
            if(d.flow.SimulationTick()) ++d.body_calls;
            TickBoundary returned=d.boundary;
            if(mode=="wrong") ++returned.loop;
            decision=d.flow.NativeReturned(returned,mode!="abnormal");
            ok=mode=="normal"?decision==TickDecision::Committed:decision!=TickDecision::Committed;
        } else if(op=="post") {
            std::uint32_t status=0;std::string fault;row>>status>>fault;
            TickPostState post{};post.status=static_cast<PostStateStatus>(status);
            post.tick=status==0?0:d.flow.Status().candidate;
            post.attempt=status==0?0:700;post.capture=status==1?500:0;
            post.reason=status==2?2u:status==3?7u:status==4?9u:0u;
            if(fault=="wrong-tick")++post.tick;
            decision=d.flow.FinishPostState(d.boundary.loop+(fault=="wrong-loop"?1:0),post);
            ok=decision==TickDecision::Committed;
        } else if(op=="publish") {
            unsigned success=0;row>>success;
            const bool published=d.flow.PublishCompleted(success!=0);
            if(published)++d.publications;
            ok=published;decision=published?TickDecision::Committed:TickDecision::Cancelled;
        } else if(op=="outer") {
            decision=d.flow.EndOuter(d.boundary.loop);ok=decision==TickDecision::Idle;
        } else if(op=="finish") {
            ok=d.FinishTick();decision=ok?TickDecision::Committed:TickDecision::Reentry;
        } else if(op=="timeout-a-live") {
            std::uint32_t count=0;row>>count;
            if(count>5000)return 2;
            for(std::uint32_t i=0;i<count;++i) {
                if(d.StartTick("normal")!=TickDecision::Started) {ok=false;break;}
                const auto command=d.players[0].MakeCommand(InputStateDraftV2{},d.flow.Status().candidate);
                if(command.sequence==0 ||
                   d.flow.AcceptInput(0,command,d.players[0].Generation())!=Decision::Accepted ||
                   !d.FinishTick()) {ok=false;break;}
            }
            decision=ok?TickDecision::Committed:TickDecision::Reentry;
        } else if(op=="remaining") {
            remaining_attempts=d.flow.RemainingAdmissionAttempts();has_remaining=true;ok=true;
        } else if(op=="cycle") {
            std::uint32_t count=0;row>>count;
            if(count>5000)return 2;
            for(std::uint32_t i=0;i<count;++i) {
                const auto started=d.StartTick("normal");
                if(started!=TickDecision::Started||!d.FinishTick()) {ok=false;break;}
            }
            decision=ok?TickDecision::Committed:TickDecision::Reentry;
        } else if(op=="local"||op=="remote"||op=="direct-local"||op=="direct-remote") {
            std::uint32_t ordinal=2;std::uint64_t sequence=0,target=0;std::uint32_t edges=0,held=0;std::int32_t move=0;
            std::string mode;row>>ordinal>>sequence>>target>>edges>>held>>move>>mode;
            if(ordinal>1)return 2;
            auto command=BuildCommand(d,ordinal,sequence,target,edges,held,move,mode);
            std::uint64_t generation=d.players[ordinal].Generation();
            if(mode=="old-generation"&&generation)--generation;
            const bool direct=op=="direct-local"||op=="direct-remote";
            const bool bytes=op=="remote"||op=="direct-remote";
            if(direct&&!bytes) admission=d.players[ordinal].AcceptInput(command,generation,d.clock.Completed());
            else {
                EncodedInputCmdV2 encoded{};std::memcpy(encoded.data(),&command,sizeof(command));
                if(mode=="short") {
                    const auto wire=std::span<const std::uint8_t>(encoded.data(),40);
                    admission=direct?d.players[ordinal].AcceptInput(wire,generation,d.clock.Completed()):
                        d.flow.AcceptInput(ordinal,wire,generation);
                } else if(mode=="long") {
                    std::array<std::uint8_t,65> wire{};std::memcpy(wire.data(),encoded.data(),encoded.size());
                    const auto span=std::span<const std::uint8_t>(wire.data(),wire.size());
                    admission=direct?d.players[ordinal].AcceptInput(span,generation,d.clock.Completed()):
                        d.flow.AcceptInput(ordinal,span,generation);
                } else {
                    const auto wire=std::span<const std::uint8_t>(encoded.data(),encoded.size());
                    admission=direct?d.players[ordinal].AcceptInput(wire,generation,d.clock.Completed()):
                        d.flow.AcceptInput(ordinal,wire,generation);
                }
            }
            ok=admission==Decision::Accepted;
        } else if(op=="fill-queue") {
            std::uint32_t ordinal=2,attempts=0;std::uint64_t target=0;
            row>>ordinal>>attempts>>target;if(ordinal>1||attempts>2000)return 2;
            auto& player=d.players[ordinal];has_fill=true;fill_attempts=attempts;
            for(std::uint32_t i=0;i<attempts;++i) {
                const auto command=player.MakeCommand(InputStateDraftV2{},target);
                fill_last=player.AcceptInput(command,player.Generation(),d.clock.Completed());
                if(fill_last==Decision::Accepted)++fill_accepted;
            }
            ok=fill_last!=Decision::Accepted;
        } else if(op=="history-evict") {
            std::uint32_t ordinal=2,count=0;row>>ordinal>>count;if(ordinal>1||count>5000)return 2;
            auto& player=d.players[ordinal];has_history=true;history_count=count;
            std::uint64_t tick=1;bool loop_ok=true;
            for(std::uint32_t i=0;i<count;++i) {
                const auto command=player.MakeCommand(InputStateDraftV2{},tick);
                const auto accepted=player.AcceptInput(command,player.Generation(),tick-1);
                if(command.sequence==0||accepted!=Decision::Accepted||!player.Begin(tick)) {loop_ok=false;break;}
                player.End(true);++tick;
            }
            history_last_accepted=player.Progress(tick-1).last_accepted_sequence;
            InputCmdDraftV2 stale{};
            stale.session_id=d.identity.session;stale.client_id=d.identity.clients[ordinal];
            stale.sequence=1;stale.target_tick=tick;stale.state={};
            history_stale=player.AcceptInput(stale,player.Generation(),tick-1);
            const auto fresh=player.MakeCommand(InputStateDraftV2{},tick);
            history_new=player.AcceptInput(fresh,player.Generation(),tick-1);
            ok=loop_ok&&history_stale==Decision::Stale&&history_new==Decision::Accepted;
        } else if(op=="source-next") {
            std::uint32_t ordinal=2;std::uint64_t target=0;row>>ordinal>>target;if(ordinal>1)return 2;
            const auto command=d.players[ordinal].MakeCommand(InputStateDraftV2{},target);
            has_source=true;generated_sequence=command.sequence;generated_target=command.target_tick;
            source_retired=d.players[ordinal].Retired();ok=generated_sequence!=0;
        } else if(op=="invalidate") {
            std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1)return 2;d.players[ordinal].Invalidate();ok=true;
        } else if(op=="rebind") {
            std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1)return 2;ok=d.Bind(ordinal);
        } else if(op=="retire") {
            std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1)return 2;d.players[ordinal].Retire();ok=true;
        } else if(op=="retire-session") {
            ok=d.session.BeginRetirement(SessionCause::Stop);
        } else if(op=="fail-session") {
            ok=d.session.Fail(SessionCause::RuntimeFailure);
        } else if(op=="status") {
        } else return 2;
        output<<"{\"scenario\":\""<<name<<"\",\"step\":"<<++step<<",\"ok\":"<<Bool(ok)
              <<",\"decision\":\""<<TickDecisionName(decision)<<"\",\"admission\":\""<<AdmissionName(admission)
              <<"\",\"value\":"<<value<<',';
        Snapshot(output,d);
        if(has_source) output<<",\"generated_sequence\":\""<<generated_sequence
            <<"\",\"generated_target\":\""<<generated_target
            <<"\",\"source_retired\":"<<Bool(source_retired);
        if(has_fill) output<<",\"fill_attempts\":\""<<fill_attempts
            <<"\",\"fill_accepted\":\""<<fill_accepted
            <<"\",\"fill_last_decision\":\""<<AdmissionName(fill_last)<<"\"";
        if(has_history) output<<",\"history_count\":\""<<history_count
            <<"\",\"history_last_accepted\":\""<<history_last_accepted
            <<"\",\"history_stale_decision\":\""<<AdmissionName(history_stale)
            <<"\",\"history_new_decision\":\""<<AdmissionName(history_new)<<"\"";
        if(has_remaining) output<<",\"remaining_attempts\":"<<remaining_attempts;
        output<<"}\n";if(!output)return 4;
    }
    output.flush();return output?0:4;
}
