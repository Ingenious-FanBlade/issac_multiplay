// T0-07 operation 6 offline production-chain missing-input lifecycles.
// Scenarios were authored before the production diagnostics were implemented.
#include "isaac_cs_t0/authority_tick_pipeline.hpp"
#include <array>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
using namespace isaac_cs_t0::coop_active;

namespace {
const char* Bool(bool value) noexcept {return value ? "true" : "false";}
const char* AdmissionName(Decision decision) noexcept
{
    switch (decision) {
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
TickBoundary Boundary(const std::string& kind, std::uint64_t loop)
{
    TickBoundary boundary{};
    boundary.loop=loop;boundary.thread=7;boundary.main_thread=7;boundary.owner=100;
    if(kind=="odd") {
        boundary.kind=TickKind::OddPlayer;
        boundary.return_rva=0x555246u;boundary.player_return_rva=0x2FD60Cu;
    } else {
        boundary.kind=TickKind::StandardRoom;boundary.return_rva=0x2FBA52u;
    }
    return boundary;
}
struct Domain {
    AuthoritySession session{};
    AuthorityTickClock clock;
    std::array<Participant,2> players{};
    AuthorityTickPipeline flow;
    SessionIdentity identity{};
    std::array<int,6> objects{};
    std::uint64_t body_calls=0,publications=0,next_loop=1,batch_sequence=0;
    TickBoundary boundary{};
    Domain(std::uint64_t initial,std::uint32_t budget):clock(initial),flow(session,clock,players,budget)
    {
        identity.session[0]=37;identity.clients={101,202};
        for(std::uint32_t i=0;i<2;++i)
            players[i].Initialize(identity.session,identity.clients[i],i,&session);
    }
    bool Bind(std::uint32_t ordinal)
    {
        return players[ordinal].Bind({&objects[ordinal*3],&objects[ordinal*3+1],
            &objects[ordinal*3+2],static_cast<std::int32_t>(ordinal)});
    }
    TickDecision StartTick(const std::string& kind,bool blocked)
    {
        boundary=Boundary(kind,next_loop++);
        return flow.BeginDrain(boundary,blocked);
    }
    bool FinishTick()
    {
        if(flow.Status().phase==TickPipelinePhase::Draining &&
           flow.BeginSimulation()!=TickDecision::Started) return false;
        if(flow.Status().phase!=TickPipelinePhase::Simulating) return false;
        ++body_calls;
        if(flow.NativeReturned(boundary,true)!=TickDecision::Committed) return false;
        TickPostState post{};post.status=PostStateStatus::Complete;post.tick=flow.Status().candidate;
        post.attempt=900;post.capture=600;
        if(flow.FinishPostState(boundary.loop,post)!=TickDecision::Committed) return false;
        if(!flow.PublishCompleted(true)) return false;
        ++publications;
        return flow.EndOuter(boundary.loop)==TickDecision::Idle;
    }
};
void Availability(std::ostream& out,const ClientInputAvailability& value)
{
    out<<"{\"observed_tick\":\""<<value.observed_tick
       <<"\",\"effective_available\":"<<Bool(value.effective_available)
       <<",\"neutral_applied\":"<<Bool(value.neutral_applied)
       <<",\"missing_reason\":\""<<MissingInputReasonName(value.missing_reason)
       <<"\",\"source\":\""<<SourceActivityName(value.source)
       <<"\",\"effective_source_tick\":\""<<value.effective_source_tick
       <<"\",\"effective_source_sequence\":\""<<value.effective_source_sequence
       <<"\",\"effective_age_ticks\":\""<<value.effective_age_ticks
       <<"\",\"has_valid_arrival\":"<<Bool(value.has_valid_arrival)
       <<",\"last_valid_arrival_tick\":\""<<value.last_valid_arrival_tick
       <<"\",\"last_valid_arrival_sequence\":\""<<value.last_valid_arrival_sequence
       <<"\",\"last_valid_arrival_target_tick\":\""<<value.last_valid_arrival_target_tick
       <<"\",\"last_valid_arrival_generation\":\""<<value.last_valid_arrival_generation
       <<"\",\"valid_arrival_count\":\""<<value.valid_arrival_count
       <<"\",\"silent_ticks\":\""<<value.silent_ticks
       <<"\",\"silent_entries\":\""<<value.silent_entries
       <<"\",\"silence_recoveries\":\""<<value.silence_recoveries
       <<"\",\"timeout_entries\":\""<<value.timeout_entries
       <<"\",\"timeout_recoveries\":\""<<value.timeout_recoveries<<"\"}";
}
void Player(std::ostream& out,const Participant& player,std::uint64_t completed)
{
    const auto value=player.Progress(completed);
    out<<"{\"accepted\":\""<<value.last_accepted_sequence
       <<"\",\"processed\":\""<<value.last_processed_sequence
       <<"\",\"processed_tick\":\""<<value.last_processed_tick
       <<"\",\"source\":\""<<value.last_processed_source_sequence
       <<"\",\"generation\":\""<<value.generation
       <<"\",\"submit\":\""<<value.submit_count
       <<"\",\"received\":\""<<value.received_count
       <<"\",\"duplicates\":\""<<value.duplicate_count
       <<"\",\"rejected\":\""<<value.rejected_count
       <<"\",\"edge_counts\":["<<value.edges[0].processed_count<<','<<value.edges[1].processed_count
       <<','<<value.edges[2].processed_count<<','<<value.edges[3].processed_count
       <<"],\"visible\":"<<Bool(value.current_valid)
       <<",\"last_valid\":"<<Bool(value.last_state_valid)
       <<",\"current_move\":\""<<value.current_state.move_x
       <<"\",\"last_move\":\""<<value.last_state.move_x
       <<"\",\"last_held_flags\":\""<<value.last_state.held_flags
       <<"\",\"last_edge_flags\":\""<<value.last_state.edge_flags
       <<"\",\"retired\":"<<Bool(value.retired)
       <<",\"bound\":"<<Bool(player.BoundValid())
       <<",\"depth\":"<<player.Depth()
       <<",\"availability\":";
    Availability(out,value.availability);
    out<<'}';
}
void ReceiptClient(std::ostream& out,const TickClientReceipt& value)
{
    out<<"{\"client\":\""<<value.client
       <<"\",\"generation\":\""<<value.generation
       <<"\",\"processed_generation\":\""<<value.processed_generation
       <<"\",\"accepted\":\""<<value.accepted
       <<"\",\"processed\":\""<<value.processed
       <<"\",\"processed_tick\":\""<<value.processed_tick
       <<"\",\"source\":\""<<value.source
       <<"\",\"current_binding\":"<<Bool(value.current_binding)
       <<",\"availability\":";
    Availability(out,value.availability);
    out<<'}';
}
void Snapshot(std::ostream& out,const Domain& domain)
{
    const auto status=domain.flow.Status();const auto& receipt=domain.flow.Receipt();
    out<<"\"phase\":\""<<TickPipelinePhaseName(status.phase)
       <<"\",\"completed\":\""<<domain.clock.Completed()
       <<"\",\"current\":\""<<domain.clock.Current()
       <<"\",\"simulation\":\""<<domain.flow.SimulationTick()
       <<"\",\"candidate\":\""<<status.candidate
       <<"\",\"drained\":"<<status.drained
       <<",\"budget\":"<<status.budget
       <<",\"session\":\""<<SessionStateName(domain.session.Status().state)
       <<"\",\"body_calls\":\""<<domain.body_calls
       <<"\",\"publications\":\""<<domain.publications
       <<"\",\"policy\":{\"max_input_age_ticks\":\""<<kMaximumInputAgeTicks
       <<"\",\"client_silence_timeout_ticks\":\""<<kClientSilenceTimeoutTicks
       <<"\"},\"receipt\":{\"tick\":\""<<receipt.tick
       <<"\",\"post\":"<<static_cast<std::uint32_t>(receipt.post.status)
       <<",\"reason\":"<<receipt.post.reason
       <<",\"capture\":\""<<receipt.post.capture
       <<"\",\"published\":"<<Bool(receipt.published)<<",\"a\":";
    ReceiptClient(out,receipt.clients[0]);out<<",\"b\":";
    ReceiptClient(out,receipt.clients[1]);out<<"},\"a\":";
    Player(out,domain.players[0],domain.clock.Completed());out<<",\"b\":";
    Player(out,domain.players[1],domain.clock.Completed());
}
Decision SubmitCommand(Domain& domain,std::uint32_t ordinal,std::istream& row,bool direct)
{
    std::uint64_t sequence=0,target=0;std::uint32_t edges=0,held=0;std::int32_t move=0;
    std::string mode;
    row>>sequence>>target>>edges>>held>>move>>mode;
    auto& player=domain.players[ordinal];
    InputCmdDraftV2 command{};
    command.session_id=domain.identity.session;command.client_id=domain.identity.clients[ordinal];
    command.sequence=sequence;command.target_tick=target;
    command.state.edge_flags=edges;command.state.held_flags=held;
    command.state.move_x=static_cast<std::int16_t>(move);
    if(mode=="malformed") command.state.reserved=1;
    else if(mode=="wrong-client") command.client_id=domain.identity.clients[1-ordinal];
    else if(mode=="wrong-session") command.session_id[0]^=0x80;
    std::uint64_t generation=player.Generation();
    if(mode=="old-generation" && generation) --generation;
    if(direct) return player.AcceptInput(command,generation,domain.clock.Completed());
    return domain.flow.AcceptInput(ordinal,command,generation);
}
Decision SubmitBytes(Domain& domain,std::uint32_t ordinal,std::istream& row,bool direct)
{
    std::uint64_t sequence=0,target=0;std::uint32_t edges=0,held=0;std::int32_t move=0;
    std::string mode;
    row>>sequence>>target>>edges>>held>>move>>mode;
    auto& player=domain.players[ordinal];
    InputCmdDraftV2 command{};
    command.session_id=domain.identity.session;command.client_id=domain.identity.clients[ordinal];
    command.sequence=sequence;command.target_tick=target;
    command.state.edge_flags=edges;command.state.held_flags=held;
    command.state.move_x=static_cast<std::int16_t>(move);
    if(mode=="malformed") command.state.reserved=1;
    else if(mode=="wrong-client") command.client_id=domain.identity.clients[1-ordinal];
    else if(mode=="wrong-session") command.session_id[0]^=0x80;
    std::uint64_t generation=player.Generation();
    if(mode=="old-generation" && generation) --generation;
    EncodedInputCmdV2 bytes{};std::memcpy(bytes.data(),&command,bytes.size());
    if(direct)
        return player.AcceptInput(std::span<const std::uint8_t>(bytes.data(),bytes.size()),
                                  generation,domain.clock.Completed());
    return domain.flow.AcceptInput(ordinal,
        std::span<const std::uint8_t>(bytes.data(),bytes.size()),generation);
}
float ReadValue(Domain& domain,std::uint32_t ordinal,std::uint32_t action)
{
    auto& player=domain.players[ordinal];const auto tick=domain.flow.SimulationTick();
    if(action<4) return player.Read(action,Query::Scalar,Consumer::Movement,player.Bound(),tick);
    if(action<8) return player.Read(action,Query::Scalar,Consumer::Shooting,player.Bound(),tick);
    if(action==8) return player.Read(action,Query::Triggered,Consumer::Bomb,player.Bound(),tick);
    if(action==9) return player.Read(action,Query::Triggered,Consumer::Item,player.Bound(),tick);
    if(action==10) return player.Read(action,Query::Triggered,Consumer::Pill,player.Bound(),tick);
    if(action==11) return player.Read(action,Query::Triggered,Consumer::Drop,player.Bound(),tick);
    return 0.0F;
}
}
int main(int argc,char** argv)
{
    if(argc!=4) return 2;
    std::ifstream input(argv[1]);std::ofstream output(argv[2]),batches(argv[3]);
    if(!input || !output || !batches) return 2;
    std::unique_ptr<Domain> domain;std::string line,name;
    std::size_t step=0;
    while(std::getline(input,line)) {
        std::istringstream row(line);std::string op;row>>op;
        if(op=="scenario") {
            std::uint64_t initial=0;std::uint32_t budget=kTickDrainAttemptBudget;
            row>>name>>initial>>budget;
            domain=std::make_unique<Domain>(initial,budget);step=0;continue;
        }
        if(!domain) return 2;
        auto& d=*domain;
        auto decision=TickDecision::Idle;
        auto admission=Decision::Accepted;
        bool ok=true;float value=0.0F;
        if(op=="warmup") {
            ok=d.session.Start(d.identity) && d.session.Prepared();
            ok=d.Bind(0) && d.Bind(1) && ok;
        } else if(op=="activate") {
            ok=d.session.Activate(d.players[0].BoundValid(),d.players[1].BoundValid());
        } else if(op=="fail-session") {
            ok=d.session.Fail(SessionCause::RuntimeFailure);
        } else if(op=="retire-session") {
            ok=d.session.BeginRetirement(SessionCause::Stop);
        } else if(op=="begin") {
            std::string kind,mode;row>>kind>>mode;
            const bool blocked=mode=="blocked";
            decision=d.StartTick(kind,blocked);
            ok=decision==(blocked?TickDecision::Blocked:TickDecision::Started);
        } else if(op=="resolve") {
            decision=d.flow.BeginSimulation();ok=decision==TickDecision::Started;
        } else if(op=="returned") {
            std::string mode;row>>mode;
            if(d.flow.SimulationTick()) ++d.body_calls;
            decision=d.flow.NativeReturned(d.boundary,mode!="abnormal");
            ok=decision==(mode=="abnormal"?TickDecision::AbnormalReturn:TickDecision::Committed);
        } else if(op=="post") {
            std::uint32_t status=0;std::string fault;row>>status>>fault;
            TickPostState post{};post.status=static_cast<PostStateStatus>(status);
            post.tick=status==0?0:d.flow.Status().candidate;
            post.attempt=status==0?0:900;post.capture=status==1?600:0;
            post.reason=status==2?2u:status==3?7u:0u;
            if(fault=="wrong-tick") ++post.tick;
            decision=d.flow.FinishPostState(d.boundary.loop+(fault=="wrong-loop"?1:0),post);
            ok=decision==TickDecision::Committed;
        } else if(op=="publish") {
            unsigned success=0;row>>success;
            ok=d.flow.PublishCompleted(success!=0);
            if(ok) ++d.publications;
            decision=ok?TickDecision::Committed:TickDecision::Cancelled;
        } else if(op=="outer") {
            decision=d.flow.EndOuter(d.boundary.loop);ok=decision==TickDecision::Idle;
        } else if(op=="finish") {
            ok=d.FinishTick();decision=ok?TickDecision::Committed:TickDecision::Reentry;
        } else if(op=="cycle") {
            std::uint32_t count=0;row>>count;
            if(count>5000) return 2;
            for(std::uint32_t index=1;index<=count;++index) {
                d.boundary=Boundary("normal",d.next_loop++);
                if(d.flow.BeginDrain(d.boundary,false)!=TickDecision::Started) return 3;
                if(!d.FinishTick()) return 3;
                batches<<"{\"scenario\":\""<<name<<"\",\"sequence\":"
                       <<++d.batch_sequence<<",\"index\":"<<index<<',';
                Snapshot(batches,d);batches<<"}\n";
            }
            ok=true;decision=TickDecision::Committed;
        } else if(op=="invalidate") {
            std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1) return 2;
            d.players[ordinal].Invalidate();ok=true;
        } else if(op=="rebind") {
            std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1) return 2;
            ok=d.Bind(ordinal);
        } else if(op=="retire") {
            std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1) return 2;
            d.players[ordinal].Retire();ok=true;
        } else if(op=="read") {
            std::uint32_t ordinal=2,action=0;row>>ordinal>>action;
            if(ordinal>1 || action>11) return 2;
            value=ReadValue(d,ordinal,action);
        } else if(op=="direct" || op=="direct-bytes" || op=="local" || op=="remote") {
            std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1) return 2;
            const bool direct=op.rfind("direct",0)==0;
            const bool bytes=op=="remote" || op=="direct-bytes";
            admission=bytes?SubmitBytes(d,ordinal,row,direct):SubmitCommand(d,ordinal,row,direct);
            ok=admission==Decision::Accepted;
        } else if(op=="status") {
        } else {
            return 2;
        }
        output<<"{\"scenario\":\""<<name<<"\",\"step\":"<<++step
              <<",\"ok\":"<<Bool(ok)<<",\"decision\":\""<<TickDecisionName(decision)
              <<"\",\"admission\":\""<<AdmissionName(admission)
              <<"\",\"value\":"<<value<<',';
        Snapshot(output,d);output<<"}\n";
        if(!output || !batches) return 4;
    }
    output.flush();batches.flush();
    return output && batches ? 0 : 4;
}
