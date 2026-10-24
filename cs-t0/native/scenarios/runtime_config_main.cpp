// T0-07 operation 7 offline production-chain runtime configuration acceptance.
// Scenarios and failure expectations were authored before production plumbing.
#include "isaac_cs_t0/authority_tick_pipeline.hpp"
#include "isaac_cs_t0/coop_active_ipc.hpp"
#include <array>
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
using namespace isaac_cs_t0::coop_active;

namespace {
InputRuntimeConfig Config(std::uint64_t lead,std::uint64_t future,std::uint64_t max_age,
    std::uint32_t script,std::uint32_t admission)
{
    InputRuntimeConfig config{};
    config.input_lead_ticks=lead;config.future_window_ticks=future;
    config.max_input_age_ticks=max_age;config.script_drain_budget=script;
    config.admission_attempt_budget=admission;return config;
}
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
    Domain(std::uint64_t initial,const InputRuntimeConfig& requested):
        clock(initial),flow(session,clock,players),config(requested)
    {
        identity.session[0]=37;identity.clients={101,202};
        config_ok=ValidateInputRuntimeConfig(config)==InputConfigDecision::Accepted;
        if(!clock.ConfigureFutureWindow(config.future_window_ticks)) config_ok=false;
        if(!flow.Configure(config)) config_ok=false;
        for(std::uint32_t i=0;i<2;++i) {
            if(!players[i].Configure(config)) config_ok=false;
            players[i].Initialize(identity.session,identity.clients[i],i,&session);
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
        if(flow.Status().phase==TickPipelinePhase::Draining && flow.BeginSimulation()!=TickDecision::Started) return false;
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
void ConfigFields(std::ostream& out,const InputRuntimeConfig& config)
{
    out<<"{\"input_lead_ticks\":\""<<config.input_lead_ticks
       <<"\",\"future_window_ticks\":\""<<config.future_window_ticks
       <<"\",\"max_input_age_ticks\":\""<<config.max_input_age_ticks
       <<"\",\"script_drain_budget\":"<<config.script_drain_budget
       <<",\"admission_attempt_budget\":"<<config.admission_attempt_budget<<"}";
}
void Snapshot(std::ostream& out,const Domain& domain)
{
    const auto status=domain.flow.Status();
    const auto progress=domain.players[0].Progress(domain.clock.Completed());
    out<<"\"config_ok\":"<<Bool(domain.config_ok)
       <<",\"effective_config\":";
    ConfigFields(out,domain.players[0].RuntimeConfig());
    out<<",\"clock_future\":\""<<domain.clock.FutureWindow()
       <<"\",\"pipeline_budget\":"<<status.budget
       <<",\"phase\":\""<<TickPipelinePhaseName(status.phase)
       <<"\",\"completed\":\""<<domain.clock.Completed()
       <<"\",\"current\":\""<<domain.clock.Current()
       <<"\",\"candidate\":\""<<status.candidate
       <<"\",\"drained\":"<<status.drained
       <<",\"session\":\""<<SessionStateName(domain.session.Status().state)
       <<"\",\"body_calls\":\""<<domain.body_calls
       <<"\",\"publications\":\""<<domain.publications
       <<"\",\"a_visible\":"<<Bool(domain.players[0].Visible())
       <<",\"a_retired\":"<<Bool(domain.players[0].Retired())
       <<",\"b_visible\":"<<Bool(domain.players[1].Visible())
       <<",\"b_retired\":"<<Bool(domain.players[1].Retired())
       <<",\"future_first_tick\":\""<<progress.future_first_tick
       <<"\",\"future_last_tick\":\""<<progress.future_last_tick
       <<"\",\"future_window_valid\":"<<Bool(progress.future_window_valid);
}
Decision SubmitConfigCommand(Domain& domain,std::uint32_t ordinal,std::istream& row,bool bytes)
{
    std::uint64_t sequence=0,target=0;std::uint32_t edges=0,held=0;std::int32_t move=0;std::string mode;
    row>>sequence>>target>>edges>>held>>move>>mode;
    auto& player=domain.players[ordinal];InputCmdDraftV2 command{};
    command.session_id=domain.identity.session;command.client_id=domain.identity.clients[ordinal];
    command.sequence=sequence;command.target_tick=target;command.state.edge_flags=edges;
    command.state.held_flags=held;command.state.move_x=static_cast<std::int16_t>(move);
    if(mode=="malformed")command.state.reserved=1;
    if(bytes) {
        EncodedInputCmdV2 encoded{};std::memcpy(encoded.data(),&command,sizeof(command));
        return domain.flow.AcceptInput(ordinal,std::span<const std::uint8_t>(encoded.data(),encoded.size()),
            player.Generation());
    }
    return domain.flow.AcceptInput(ordinal,command,player.Generation());
}
void MappingResult(std::ostream& out,InputConfigDecision decision,const InputRuntimeConfig& config)
{
    out<<",\"mapping_decision\":\""<<InputConfigDecisionName(decision)<<"\",\"mapping_config\":";
    ConfigFields(out,config);
}
}
int main(int argc,char** argv)
{
    if(argc!=3)return 2;
    std::ifstream input(argv[1]);std::ofstream output(argv[2]);if(!input||!output)return 2;
    std::unique_ptr<Domain> domain;std::string line,name;std::size_t step=0;
    while(std::getline(input,line)) {
        std::istringstream row(line);std::string op;row>>op;
        if(op=="scenario") {
            std::uint64_t initial=0,lead=0,future=0,max_age=0;std::uint32_t script=0,admission=0;
            row>>name>>initial>>lead>>future>>max_age>>script>>admission;
            domain=std::make_unique<Domain>(initial,Config(lead,future,max_age,script,admission));
            step=0;continue;
        }
        if(!domain)return 2;auto& d=*domain;
        auto decision=TickDecision::Idle;auto admission=Decision::Accepted;bool ok=true;float value=0.0F;
        bool has_validation=false,has_target=false,has_script=false,has_reconfigure=false,has_mapping=false;
        InputConfigDecision validation=InputConfigDecision::Accepted;
        bool target_valid=false;std::uint64_t next_target=0;
        bool script_allowed=false,reconfigure_rejected=false;
        bool has_remaining=false,has_event=false;std::uint32_t remaining_attempts=0;
        Event packed_event{};
        InputConfigDecision mapping_decision=InputConfigDecision::Accepted;InputRuntimeConfig mapping_config{};
        if(op=="warmup") {
            ok=d.session.Start(d.identity)&&d.session.Prepared();
            ok=d.Bind(0)&&d.Bind(1)&&ok;
        } else if(op=="activate") {
            ok=d.session.Activate(d.players[0].BoundValid(),d.players[1].BoundValid());
        } else if(op=="begin") {
            std::string kind;row>>kind;decision=d.StartTick(kind);ok=decision==TickDecision::Started;
        } else if(op=="finish") {
            ok=d.FinishTick();decision=ok?TickDecision::Committed:TickDecision::Reentry;
        } else if(op=="local"||op=="remote") {
            std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1)return 2;
            admission=SubmitConfigCommand(d,ordinal,row,op=="remote");ok=admission==Decision::Accepted;
        } else if(op=="retire-session") {
            ok=d.session.BeginRetirement(SessionCause::Stop);
        } else if(op=="fail-session") {
            ok=d.session.Fail(SessionCause::RuntimeFailure);
        } else if(op=="validate") {
            std::uint64_t lead=0,future=0,max_age=0,version=1,size=sizeof(InputRuntimeConfig),reserved=0;
            std::uint32_t script=0,admission_budget=0;
            row>>lead>>future>>max_age>>script>>admission_budget>>version>>size>>reserved;
            auto candidate=Config(lead,future,max_age,script,admission_budget);
            candidate.version=static_cast<std::uint32_t>(version);
            candidate.size=static_cast<std::uint32_t>(size);candidate.reserved=reserved;
            validation=ValidateInputRuntimeConfig(candidate);has_validation=true;ok=validation==InputConfigDecision::Accepted;
        } else if(op=="next-target") {
            std::uint64_t completed=0;row>>completed;
            target_valid=NextInputTarget(completed,d.config,next_target);has_target=true;ok=target_valid;
        } else if(op=="next-startup") {
            std::uint64_t completed=0;row>>completed;
            target_valid=NextStartupTarget(completed,next_target);has_target=true;ok=target_valid;
        } else if(op=="script-limit") {
            std::uint64_t drained=0;row>>drained;
            script_allowed=ScriptDrainAllowed(drained,d.config);has_script=true;ok=script_allowed;
        } else if(op=="reconfigure") {
            const bool player_ok=d.players[0].Configure(d.config);
            const bool pipeline_ok=d.flow.Configure(d.config);
            const bool clock_ok=d.clock.ConfigureFutureWindow(d.config.future_window_ticks);
            reconfigure_rejected=!player_ok&&!pipeline_ok&&!clock_ok;
            has_reconfigure=true;ok=reconfigure_rejected;
        } else if(op=="shared-v3") {
            auto memory=std::make_unique<std::uint8_t[]>(sizeof(SharedV5));
            auto* shared=reinterpret_cast<Shared*>(memory.get());
            shared->version=3;shared->size=static_cast<std::uint32_t>(sizeof(Shared));
            mapping_decision=ReadInputRuntimeConfig(shared,shared->version,shared->size,mapping_config);
            has_mapping=true;ok=mapping_decision==InputConfigDecision::Accepted;
        } else if(op=="shared-v4") {
            auto memory=std::make_unique<std::uint8_t[]>(sizeof(SharedV5));
            auto* shared=reinterpret_cast<Shared*>(memory.get());
            shared->version=4;shared->size=static_cast<std::uint32_t>(sizeof(SharedV4));
            mapping_decision=ReadInputRuntimeConfig(shared,shared->version,shared->size,mapping_config);
            has_mapping=true;ok=mapping_decision==InputConfigDecision::Accepted;
        } else if(op=="shared-v5") {
            auto memory=std::make_unique<std::uint8_t[]>(sizeof(SharedV5));
            auto* shared=reinterpret_cast<Shared*>(memory.get());
            auto* extended=reinterpret_cast<SharedV5*>(memory.get());
            shared->version=5;shared->size=static_cast<std::uint32_t>(sizeof(SharedV5));extended->config=d.config;
            mapping_decision=ReadInputRuntimeConfig(shared,shared->version,shared->size,mapping_config);
            has_mapping=true;ok=mapping_decision==InputConfigDecision::Accepted;
        } else if(op=="shared-v5-invalid") {
            auto memory=std::make_unique<std::uint8_t[]>(sizeof(SharedV5));
            auto* shared=reinterpret_cast<Shared*>(memory.get());
            auto* extended=reinterpret_cast<SharedV5*>(memory.get());
            shared->version=5;shared->size=static_cast<std::uint32_t>(sizeof(SharedV5));
            extended->config=d.config;extended->config.max_input_age_ticks=1;
            mapping_decision=ReadInputRuntimeConfig(shared,shared->version,shared->size,mapping_config);
            has_mapping=true;ok=mapping_decision==InputConfigDecision::Accepted;
        } else if(op=="shared-wrong-size") {
            auto memory=std::make_unique<std::uint8_t[]>(sizeof(SharedV5));
            auto* shared=reinterpret_cast<Shared*>(memory.get());
            shared->version=5;shared->size=static_cast<std::uint32_t>(sizeof(SharedV4));
            mapping_decision=ReadInputRuntimeConfig(shared,shared->version,shared->size,mapping_config);
            has_mapping=true;ok=mapping_decision==InputConfigDecision::Accepted;
        } else if(op=="shared-wrong-version") {
            auto memory=std::make_unique<std::uint8_t[]>(sizeof(SharedV5));
            auto* shared=reinterpret_cast<Shared*>(memory.get());
            shared->version=6;shared->size=static_cast<std::uint32_t>(sizeof(SharedV5));
            mapping_decision=ReadInputRuntimeConfig(shared,shared->version,shared->size,mapping_config);
            has_mapping=true;ok=mapping_decision==InputConfigDecision::Accepted;
        } else if(op=="remaining") {
            remaining_attempts=d.flow.RemainingAdmissionAttempts();has_remaining=true;ok=true;
        } else if(op=="config-event") {
            FillInputRuntimeConfigEvent(packed_event,d.config,5);has_event=true;ok=true;
        } else if(op=="status") {
        } else return 2;
        output<<"{\"scenario\":\""<<name<<"\",\"step\":"<<++step<<",\"ok\":"<<Bool(ok)
              <<",\"decision\":\""<<TickDecisionName(decision)<<"\",\"admission\":\""<<AdmissionName(admission)
              <<"\",\"value\":"<<value<<',';
        Snapshot(output,d);
        if(has_validation) output<<",\"validate_decision\":\""<<InputConfigDecisionName(validation)<<"\"";
        if(has_target) output<<",\"next_target_valid\":"<<Bool(target_valid)
                              <<",\"next_target\":\""<<next_target<<"\"";
        if(has_script) output<<",\"script_allowed\":"<<Bool(script_allowed);
        if(has_reconfigure) output<<",\"reconfigure_rejected\":"<<Bool(reconfigure_rejected);
        if(has_mapping) MappingResult(output,mapping_decision,mapping_config);
        if(has_remaining) output<<",\"remaining_attempts\":"<<remaining_attempts;
        if(has_event) output<<",\"event_kind\":"<<static_cast<std::uint32_t>(packed_event.kind)
            <<",\"event_lead\":\""<<packed_event.command
            <<"\",\"event_future\":\""<<packed_event.target
            <<"\",\"event_max_age\":\""<<packed_event.generation
            <<"\",\"event_script\":"<<packed_event.action
            <<",\"event_admission\":"<<packed_event.decision
            <<",\"event_mapping\":"<<packed_event.query
            <<",\"event_config_version\":"<<packed_event.detail
            <<",\"event_boolean\":"<<packed_event.boolean
            <<",\"event_before0\":"<<packed_event.before[0]
            <<",\"event_before1\":"<<packed_event.before[1];
        output<<"}\n";if(!output)return 4;
    }
    output.flush();return output?0:4;
}
