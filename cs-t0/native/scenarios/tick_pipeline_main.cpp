// Complete offline production-chain lifecycles; authored before implementation.
#include "isaac_cs_t0/authority_tick_pipeline.hpp"
#include <cstring>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
using namespace isaac_cs_t0::coop_active;
namespace {
struct Domain {
 AuthoritySession session{};AuthorityTickClock clock;std::array<Participant,2> players{};
 AuthorityTickPipeline flow;SessionIdentity identity{};std::array<int,6> objects{};
 std::array<InputCmdDraftV2,2> commands{};std::uint64_t body_calls=0,publications=0;
 TickBoundary boundary{};
 Domain(std::uint64_t initial,std::uint32_t budget):clock(initial),flow(session,clock,players,budget) {
  identity.session[0]=37;identity.clients={101,202};
  for(std::uint32_t i=0;i<2;++i)players[i].Initialize(identity.session,identity.clients[i],i,&session);
 }
 bool Bind(std::uint32_t i) {return players[i].Bind({&objects[i*3],&objects[i*3+1],&objects[i*3+2],static_cast<std::int32_t>(i)});}
};
TickBoundary Boundary(const std::string& kind,std::uint64_t loop) {
 TickBoundary b{};b.loop=loop;b.thread=7;b.main_thread=7;b.owner=100;
 b.kind=kind=="odd"?TickKind::OddPlayer:TickKind::StandardRoom;
 b.return_rva=kind=="odd"?0x555246u:0x2FBA52u;b.player_return_rva=kind=="odd"?0x2FD60Cu:0;
 if(kind=="wrong-thread")b.thread=8;
 if(kind=="wrong-source")b.return_rva=1;
 if(kind=="wrong-owner")b.owner=200;
 return b;
}
void Player(std::ostream& out,const Participant& p,std::uint64_t completed) {
 const auto v=p.Progress(completed);
 out<<"{\"accepted\":\""<<v.last_accepted_sequence<<"\",\"processed\":\""<<v.last_processed_sequence
 <<"\",\"processed_tick\":\""<<v.last_processed_tick<<"\",\"source\":\""<<v.last_processed_source_sequence
 <<"\",\"generation\":\""<<v.generation<<"\",\"submit\":\""<<v.submit_count<<"\",\"duplicates\":\""<<v.duplicate_count
 <<"\",\"rejected\":\""<<v.rejected_count<<"\",\"edge_count\":\""<<v.edges[0].processed_count
 <<"\",\"depth\":"<<p.Depth()<<",\"visible\":"<<(v.current_valid?"true":"false")
 <<",\"last_valid\":"<<(v.last_state_valid?"true":"false")<<",\"move\":"<<v.current_state.move_x
 <<",\"retired\":"<<(v.retired?"true":"false")<<"}";
}
void Snapshot(std::ostream& out,const Domain& d) {
 const auto s=d.flow.Status();const auto& r=d.flow.Receipt();
 out<<"\"phase\":\""<<TickPipelinePhaseName(s.phase)<<"\",\"completed\":\""<<d.clock.Completed()
 <<"\",\"current\":\""<<d.clock.Current()<<"\",\"simulation\":\""<<d.flow.SimulationTick()
 <<"\",\"candidate\":\""<<s.candidate<<"\",\"drained\":"<<s.drained<<",\"budget\":"<<s.budget
 <<",\"session\":\""<<SessionStateName(d.session.Status().state)<<"\",\"body_calls\":\""<<d.body_calls
 <<"\",\"publications\":\""<<d.publications<<"\",\"receipt\":{\"tick\":\""<<r.tick
 <<"\",\"post\":"<<static_cast<std::uint32_t>(r.post.status)<<",\"reason\":"<<r.post.reason
 <<",\"capture\":\""<<r.post.capture<<"\",\"published\":"<<(r.published?"true":"false")<<"},\"a\":";
 Player(out,d.players[0],d.clock.Completed());out<<",\"b\":";Player(out,d.players[1],d.clock.Completed());
}
}
int main(int argc,char** argv) {
 if(argc!=4)return 2;
 std::ifstream input(argv[1]);std::ofstream output(argv[2]),batches(argv[3]);if(!input || !output || !batches)return 2;
 std::unique_ptr<Domain> domain;std::string line,name;std::size_t step=0;
 while(std::getline(input,line)) {
  std::istringstream row(line);std::string op;row>>op;
  if(op=="scenario") {std::uint64_t initial=0;std::uint32_t budget=kTickDrainAttemptBudget;row>>name>>initial>>budget;domain=std::make_unique<Domain>(initial,budget);step=0;continue;}
  if(!domain)return 2;auto& d=*domain;auto decision=TickDecision::Idle;auto admission=Decision::Accepted;bool ok=true;float value=0;
  if(op=="warmup") {ok=d.session.Start(d.identity) && d.session.Prepared();ok=d.Bind(0) && d.Bind(1) && ok;}
  else if(op=="activate")ok=d.session.Activate(d.players[0].BoundValid(),d.players[1].BoundValid());
  else if(op=="fail-session")ok=d.session.Fail(SessionCause::RuntimeFailure);
  else if(op=="retire-session")ok=d.session.BeginRetirement(SessionCause::Stop);
  else if(op=="drain") {std::string kind,mode;std::uint64_t loop=0;row>>kind>>loop>>mode;d.boundary=Boundary(kind,loop);decision=d.flow.BeginDrain(d.boundary,mode=="blocked");ok=decision==TickDecision::Started;}
  else if(op=="resolve") {decision=d.flow.BeginSimulation();ok=decision==TickDecision::Started;}
  else if(op=="returned") {std::string kind,mode;std::uint64_t loop=0;row>>kind>>loop>>mode;if(d.flow.SimulationTick())++d.body_calls;decision=d.flow.NativeReturned(Boundary(kind,loop),mode!="abnormal");ok=decision==TickDecision::Committed;}
  else if(op=="post") {std::uint32_t status=0;std::string fault;row>>status>>fault;TickPostState post{};post.status=static_cast<PostStateStatus>(status);
   post.tick=status==0?0:d.flow.Status().candidate;post.attempt=status==0?0:700;post.capture=status==1?501:0;post.reason=status==2?2u:status==3?7u:0u;
   if(fault=="wrong-tick")++post.tick;
   decision=d.flow.FinishPostState(d.boundary.loop+(fault=="wrong-loop"?1:0),post);ok=decision==TickDecision::Committed;
  } else if(op=="publish") {unsigned success=0;row>>success;ok=d.flow.PublishCompleted(success!=0);if(ok)++d.publications;}
  else if(op=="outer") {decision=d.flow.EndOuter(d.boundary.loop);ok=decision==TickDecision::Idle;}
  else if(op=="cycle") {
   std::uint32_t count=0;row>>count;if(count>5000)return 2;
   for(std::uint32_t i=1;i<=count;++i) {
    d.boundary=Boundary((i&1)?"normal":"odd",i);
    if(d.flow.BeginDrain(d.boundary,false)!=TickDecision::Started)return 3;
    InputStateDraftV2 state{};state.move_x=32767;state.edge_flags=i==1?1u:0u;
    const auto command=d.players[0].MakeCommand(state,d.clock.Completed()+1);
    admission=d.flow.AcceptInput(0,command,d.players[0].Generation());
    if(d.flow.BeginSimulation()!=TickDecision::Started)return 3;
    ++d.body_calls;if(d.flow.NativeReturned(d.boundary,true)!=TickDecision::Committed)return 3;
    if(d.flow.FinishPostState(i,TickPostState{})!=TickDecision::Committed)return 3;
    if(!d.flow.PublishCompleted(true))return 3;++d.publications;
    batches<<"{\"scenario\":\""<<name<<"\",\"index\":"<<i<<',';Snapshot(batches,d);batches<<"}\n";
   }
  } else if(op=="status") {}
  else {
   std::uint32_t ordinal=2;row>>ordinal;if(ordinal>1)return 2;auto& player=d.players[ordinal];
   if(op=="local" || op=="remote") {
    std::uint64_t sequence=0,target=0;std::uint32_t edges=0;std::int32_t move=0;std::string mode;row>>sequence>>target>>edges>>move>>mode;
    InputCmdDraftV2 command{};command.session_id=d.identity.session;command.client_id=d.identity.clients[ordinal];command.sequence=sequence;command.target_tick=target;command.state.edge_flags=edges;command.state.move_x=static_cast<std::int16_t>(move);
    if(mode=="malformed")command.state.reserved=1;
    if(mode=="wrong-client")command.client_id=d.identity.clients[1-ordinal];
    if(mode=="wrong-session")command.session_id[0]^=128;
    const auto generation=player.Generation()-(mode=="old-generation"?1:0);
    if(op=="local")admission=d.flow.AcceptInput(ordinal,command,generation);
    else {EncodedInputCmdV2 bytes{};std::memcpy(bytes.data(),&command,bytes.size());admission=d.flow.AcceptInput(ordinal,std::span<const std::uint8_t>(bytes),generation);}
    ok=admission==Decision::Accepted;
   } else if(op=="invalidate")player.Invalidate();
   else if(op=="rebind")ok=d.Bind(ordinal);
   else if(op=="retire")player.Retire();
   else if(op=="read") {std::uint32_t action=0;row>>action;const auto consumer=action==8?Consumer::Bomb:Consumer::Movement;value=player.Read(action,action==8?Query::Triggered:Query::Scalar,consumer,player.Bound(),d.flow.SimulationTick());}
   else return 2;
  }
  output<<"{\"scenario\":\""<<name<<"\",\"step\":"<<++step<<",\"ok\":"<<(ok?"true":"false")<<",\"decision\":\""<<TickDecisionName(decision)<<"\",\"admission\":"<<static_cast<std::uint32_t>(admission)<<",\"value\":"<<value<<',';
  Snapshot(output,d);output<<"}\n";if(!output || !batches)return 4;
 }
 output.flush();batches.flush();return output && batches?0:4;
}
