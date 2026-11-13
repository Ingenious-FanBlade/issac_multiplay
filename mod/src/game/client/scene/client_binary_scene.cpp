#include "game/client/scene/client_binary_scene.h"
#include "game/client/scene/client_atomic_install.h"
#include "game/client/scene/client_scene.h"
#include "game/common/context.h"
#include "net/live.h"
#include "runtime.h"
#include <algorithm>

namespace cs::game {
namespace {
namespace S=state;
S::ClientStateNativeStep Ready(const char *why,bool written=false) noexcept {return {true,written,false,why};}
S::ClientStateNativeStep Failed(const char *why,bool written=false) noexcept {return {false,written,true,why};}
// 后台仅使用封存计划和表中的资源结果，实际原生资格由 Manager 重新检查。
class Readiness final:public ClientSceneInstallReadiness {
public:
  bool Available()const noexcept override {return true;}
  S::StateReadiness Inspect(const ClientSceneInstallPlan &scene,const S::ClientStateTables &tables)const override {
    const auto &plan=scene.State();
    for(const auto &need:plan.Resources()){
      switch(tables.ResourceState(need.resources->Directory().epoch,need.key)){
        case S::ClientResourceState::Ready:break;
        case S::ClientResourceState::Failed:
        case S::ClientResourceState::MissingLocalResource:return S::StateReadiness::MissingLocalResource;
        default:return S::StateReadiness::ResourcePending;
      }
    }
    return S::StateReadiness::Ready;
  }
};
}
struct ClientBinaryScene::Impl final:S::ClientStateNativeOwner,S::ClientStateApplyResults {
  struct Batch final {
    S::ClientStateApplyBatchRef ticket;
    std::unique_ptr<ClientSceneAtomicInstaller> writer;
    bool display_entered=false;
    bool Drain(bool final_scene=false) noexcept {
      if(!ticket)return true;
      if(writer&&!writer->Drain(*ticket,final_scene))return false;
      *this={};return true;
    }
    const ClientSceneInstallPlan &Plan()const noexcept {return static_cast<const ClientSceneInstallPlan &>(*ticket->native);}
  };
  ClientScene &scene;
  net::live::Client &transport;
  std::shared_ptr<S::ClientStateMemory> memory;
  ClientSceneMainPreparation preparation;
  S::ClientStateApplyCoordinator coordinator;
  std::shared_ptr<ClientSceneAtomicInstaller::RootScalars> root_scalars;
  S::StateRoute route{};
  Batch pending,world,body,retired_world,retired_body;
  std::array<std::size_t,4> readback_cursor{};
  // 最新提交批次持有完整costume及两流资源租约；不为配对投影保留历史安装器链。
  bool costumes_in_body=false;
  bool opened=false,closing=false,resetting=false,display_detached=false,preserve_room=false;
  std::optional<S::StateResyncReason> request;
  std::array<ClientDisplaySound,512> sounds{};
  std::array<void *,512> event_players{},event_resources{};
  bool BindDisplay() noexcept {
    const auto values=pending.Plan().Sounds();
    const auto sound_links=pending.Plan().SoundLinks();
    const auto links=pending.Plan().EventLinks();
    if(values.size()>sounds.size()||links.size()>event_players.size())return false;
    std::copy(values.begin(),values.end(),sounds.begin());
    std::fill_n(event_players.begin(),links.size(),nullptr);
    std::fill_n(event_resources.begin(),links.size(),nullptr);
    if(!preparation.VisitResources(*pending.ticket,this,[](void *p,S::ResourceKey key,void *value)noexcept{
      auto &s=*static_cast<Impl *>(p);
      const auto sound_links=s.pending.Plan().SoundLinks(),event_links=s.pending.Plan().EventLinks();
      for(std::size_t i=0;i<sound_links.size();++i)if(sound_links[i].resource==key)s.sounds[i].asset=value;
      for(std::size_t i=0;i<event_links.size();++i)if(event_links[i].resource==key)s.event_resources[i]=value;
      return true;
    }))return false;
    for(std::size_t i=0;i<sound_links.size();++i)if(!ClientDisplaySoundReady(sounds[i]))return false;
    if(!links.empty()&&!preparation.VisitObjects(*pending.ticket,this,[](void *p,std::uint32_t n,void *value,const S::PresentationMember &)noexcept{
      auto &s=*static_cast<Impl *>(p);
      const auto links=s.pending.Plan().EventLinks();
      for(std::size_t i=0;i<links.size();++i)if(links[i].player==n)s.event_players[i]=value;
      return true;
    }))return false;
    for(std::size_t i=0;i<links.size();++i){
      if((links[i].player!=S::kClientPlanNone&&!event_players[i])||(links[i].resource.index!=S::kNullIndex&&!event_resources[i]))return false;
      auto event=pending.Plan().Events()[i];
      if(event.kind==unsigned(S::StateEventKind::Sfx)){
        event.sound.asset=event_resources[i];
        if(!ClientDisplaySoundReady(event.sound))return false;
      }
    }
    return true;
  }
  Impl(ClientScene &s,net::live::Client &t,std::shared_ptr<S::ClientStateMemory> m):
    scene(s),transport(t),memory(std::move(m)),preparation(memory),coordinator(*this,*this),
    root_scalars(ClientBinaryRootScalars(memory)) {}
  bool Current(const S::ClientStateApplyBatch &b)const noexcept override {
    return !closing&&!resetting&&pending.ticket.get()==&b&&opened&&preparation.Current(b);
  }
  S::ClientStateNativeStep Prepare(const S::ClientStateApplyBatch &) noexcept override {
    return preparation.Prepare(pending.ticket);
  }
  S::ClientStateNativeStep Stage(const S::ClientStateApplyBatch &) noexcept override {
    if(!pending.writer)try{pending.writer=std::make_unique<ClientSceneAtomicInstaller>(memory,preparation,root_scalars);}
    catch(...){return Failed("binary_install_owner_allocation");}
    return pending.writer->Stage(pending.ticket);
  }
  S::ClientStateNativeStep Install(const S::ClientStateApplyBatch &b) noexcept override {
    if(!scene.DestructiveReady())return {false,false,false,"binary_install_waiting_for_borrows"};
    bool written=false;
    if(const auto &display=pending.Plan().Display()){
      if(!BindDisplay())return Failed("binary_display_resource_or_event_binding");
      const auto step=scene.PrepareBinaryDisplay(display);
      if(!step.done)return Failed(step.reason,step.written);
      pending.display_entered=true;written=step.written;
    }
    const auto step=pending.writer->Install(b);
    if(!step.ready){
      if(!written&&!step.written)scene.AbortBinaryDisplay();
      return Failed(step.reason,written||step.written);
    }
    return Ready(step.reason,written||step.written);
  }
  S::ClientStateNativeStep Display(const S::ClientStateApplyBatch &) noexcept override {
    const auto &display=pending.Plan().Display();
    if(!display)return Ready("binary_body_display_inherited");
    const auto step=scene.InstallBinaryDisplay(display);
    if(!step.done)return Failed(step.reason,step.written);
    const auto boss=scene.RestoreBinaryBoss(pending.Plan().Boss());
    if(!boss.done)return Failed(boss.reason,true);
    const auto audio=scene.InstallBinaryAudio(pending.Plan().Music(),{sounds.data(),pending.Plan().Sounds().size()});
    return audio.done?Ready(audio.reason,true):Failed(audio.reason,true);
  }
  S::ClientStateNativeStep Replay(const S::ClientStateApplyBatch &,std::size_t index) noexcept override {
    if(index>=pending.Plan().Events().size())return Failed("binary_display_event_index");
    auto event=pending.Plan().Events()[index];
    event.player=event_players[index];event.configuration=event_resources[index];event.sound.asset=event.configuration;
    const auto step=scene.ReplayBinaryEvent(event);
    return step.done?Ready(step.reason,step.written):Failed(step.reason,step.written);
  }
  S::ClientStateNativeStep Publish(const S::ClientStateApplyBatch &b) noexcept override {
    if(pending.Plan().Display()&&!scene.BinaryDisplayComplete())return Failed("binary_display_publication_incomplete",true);
    const auto step=pending.writer->Publish(b);
    if(step.ready){
      const auto next=b.target.candidate->State(S::BodyStream::World);
      scene.PublishBinary(next?next->Context().target_version:scene.AppliedRevision());
    }
    return step;
  }
  void Invalidate(const S::ClientStateApplyBatch &b,const char *why) noexcept override {
    if(pending.ticket.get()!=&b)return;
    Trace("client","event=binary_native_invalidate route=%llu serial=%llu reason=%s",
      b.ticket.route.route,b.ticket.serial,why);
    scene.SuspendBinaryDisplay();resetting=true;preserve_room=false;
    if(pending.writer)pending.writer->Invalidate(b,why);
  }
  bool Complete(const S::ClientStateApplyCompletion &done) noexcept override {
    if(!transport.CompleteBinary(done))return false;
    if(done.outcome==S::StateApplyOutcome::FailedBeforeWrites&&!closing)
      Trace("client","event=binary_native_rejected route=%llu serial=%llu reason=%s",done.ticket.route.route,done.ticket.serial,coordinator.Reason());
    return true;
  }
  bool DetachDisplay() noexcept {
    if(display_detached)return true;
    if(!scene.RetireBinaryDisplay(preserve_room))return false;
    display_detached=true;return true;
  }
  bool RetireInstalled() noexcept {
    return retired_body.Drain()&&retired_world.Drain()&&body.Drain()&&world.Drain(true);
  }
  bool Drain(const S::ClientStateApplyBatch &b) noexcept override {
    if(pending.ticket.get()!=&b)return true;
    const bool retire_scene=resetting||closing;
    // 音频和显示先退出当前对象栈，再回滚或退休字段附件及原生成员。
    if(retire_scene&&!DetachDisplay())return false;
    if(!retire_scene&&pending.display_entered)scene.AbortBinaryDisplay();
    if(pending.writer&&!pending.writer->Drain(*pending.ticket,retire_scene&&!world.ticket))return false;
    if(!pending.writer&&!preparation.DrainBatch(*pending.ticket))return false;
    pending.writer.reset();
    pending.display_entered=false;
    if(!resetting&&!closing)return true;
    if(!DetachDisplay()||!RetireInstalled()||!scene.ReleaseBinaryDisplayRoots(preserve_room))return false;
    resetting=false;display_detached=false;return true;
  }
  void CommitPending() noexcept {
    const auto streams=S::StateTargetStreams(pending.ticket->target.candidate->Target());
    costumes_in_body=!(streams&1);
    // 联合批次独占一份安装器。单流更新保留另一流仍借用的批次。
    if(streams&1){
      retired_world=std::move(world);world=std::move(pending);
      if(streams&2)retired_body=std::move(body);
      else if(!body.ticket&&retired_world.ticket&&retired_world.ticket->target.candidate->State(S::BodyStream::Body))
        body=std::move(retired_world);
    }else {retired_body=std::move(body);body=std::move(pending);}
  }
  bool RepairInstalled() noexcept {
    if(!scene.DisplayReady()||!scene.DestructiveReady())return true;
    const auto run=[&](Batch &batch,S::BodyStream stream,bool costumes=false){
      if(!batch.ticket)return true;
      if(batch.ticket->resolution.load(std::memory_order_acquire)!=S::ClientStateApplyResolution::Committed)return false;
      const auto step=batch.writer->Repair(*batch.ticket,stream,readback_cursor[unsigned(stream)+(costumes?2:0)],costumes);
      if(!step.failed)return true;
      Trace("client","event=binary_native_readback_failed stream=%u reason=%s",unsigned(stream),step.reason);
      return false;
    };
    auto &costumes=costumes_in_body?body:world;
    return run(world,S::BodyStream::World)&&run(body.ticket?body:world,S::BodyStream::Body)&&
      run(costumes,S::BodyStream::World,true)&&run(costumes,S::BodyStream::Body,true);
  }
};
ClientBinaryScene::ClientBinaryScene(ClientScene &s,net::live::Client &t,std::shared_ptr<S::ClientStateMemory> m):
  impl_(std::make_unique<Impl>(s,t,std::move(m))) {}
ClientBinaryScene::~ClientBinaryScene()=default;
std::shared_ptr<const S::ClientStatePreparation> ClientBinaryScene::Preparation(){
  return std::make_shared<ClientSceneStatePreparation>(std::make_shared<Readiness>());
}
bool ClientBinaryScene::Open(const S::StateRoute &route) noexcept {
  auto &s=*impl_;if(s.route.connection||!route.connection||!route.route)return false;
  s.route=route;s.readback_cursor={};s.costumes_in_body=false;s.scene.BeginBinary();return true;
}
void ClientBinaryScene::Close(bool preserve_room) noexcept {
  auto &s=*impl_;
  if(!s.route.connection){
    if(!preserve_room)s.closing=true;
    return;
  }
  if(!s.closing&&preserve_room&&s.scene.DisplayReady())s.preserve_room=s.scene.PreserveBinaryRoom();
  if(!preserve_room)s.preserve_room=false;
  s.closing=true;s.scene.SuspendBinaryDisplay();s.coordinator.Cancel();s.preparation.Close();
}
bool ClientBinaryScene::Drained()const noexcept {return !impl_->route.connection&&!impl_->closing;}
void ClientBinaryScene::Recover(S::StateResyncReason reason) noexcept {
  auto &s=*impl_;
  if(!s.route.connection||s.closing||s.resetting)return;
  s.scene.SuspendBinaryDisplay();s.resetting=true;s.request=reason;
  s.coordinator.Cancel();
}
void ClientBinaryScene::Pump() noexcept {
  auto &s=*impl_;
  if(!IsMain()||TopOwner()!=Owner::Manager)return;
  if(!s.route.connection){
    if(s.closing&&s.scene.DiscardBinaryHistory())s.closing=false;
    return;
  }
  if(!s.opened&&!s.closing&&ClientPreparationRoots().game)s.opened=s.preparation.Open(s.route,s.route.route);
  if(s.opened||s.closing)s.preparation.PumpResources(s.transport);
  if(s.request&&s.transport.RecoverBinary(s.route,*s.request))s.request.reset();
  if(s.coordinator.Busy())s.coordinator.Pump();
  if(!s.coordinator.Busy()&&s.pending.ticket){
    if(s.pending.ticket->resolution.load(std::memory_order_acquire)==S::ClientStateApplyResolution::Committed)s.CommitPending();
    else if(!s.pending.Drain())return;
  }
  if(!s.retired_body.Drain()||!s.retired_world.Drain())return;
  if(s.resetting&&!s.coordinator.Busy()&&!s.request){
    if(!s.DetachDisplay()||!s.RetireInstalled()||!s.scene.ReleaseBinaryDisplayRoots())return;
    s.resetting=s.display_detached=false;
  }
  if(s.closing&&!s.coordinator.Busy()){
    if(!s.DetachDisplay()||!s.RetireInstalled()||!s.scene.ReleaseBinaryDisplayRoots(s.preserve_room))return;
    s.preparation.Close();s.preparation.PumpResources(s.transport);
    // worker 可能仍有撤销的批次在邮箱，必须回收原 ticket 后才能交还路由。
    S::ClientStateApplyBatchRef cancelled;
    if(s.transport.PollBinary(cancelled)){s.pending.ticket=cancelled;if(s.coordinator.Offer(std::move(cancelled))){s.coordinator.Cancel();s.coordinator.Pump();}return;}
    if(!s.preparation.Drain())return;
    s.route={};s.request.reset();s.opened=s.closing=s.resetting=s.display_detached=s.preserve_room=false;s.scene.FinishBinary();return;
  }
  if(s.opened&&!s.closing&&!s.coordinator.Busy()&&!s.pending.ticket){
    if(s.resetting)return;
    if(!s.RepairInstalled()){Recover(S::StateResyncReason::Apply);return;}
    S::ClientStateApplyBatchRef next;
    if(s.transport.PollBinary(next)){
      s.pending.ticket=next;
      if(s.coordinator.Offer(std::move(next)))s.coordinator.Pump();
    }
  }
}
ClientPlayerControlView ClientBinaryScene::Control(unsigned ordinal)const noexcept {
  const auto &s=*impl_;
  if(!s.scene.DisplayReady())return {};
  const auto *current=s.pending.ticket&&s.pending.writer&&s.pending.Plan().Display()&&
    s.scene.AppliedRevision()==s.pending.Plan().Display()->Context().target_version?&s.pending:&s.world;
  if(!current->ticket)return {};
  const auto &plan=current->Plan().State();
  for(const auto &g:plan.Groups())if(g.ordinal==ordinal){
    const auto &participant=plan.Objects()[g.participant].member.identity;
    const auto &group=plan.Objects()[g.object].member.identity;
    ClientPlayerControlView view;view.present=true;view.ordinal=ordinal;view.control_session=g.session;view.binding_revision=g.binding;
    view.participant={plan.Target().route.scope.run,participant.id,participant.space};
    view.group={plan.Target().route.scope.run,group.id,group.space};view.revision=s.scene.AppliedRevision();
    for(const auto &p:plan.Players())if(p.group==g.object){++view.bodies;view.active+=bool(p.control&1);view.consumers+=bool(p.control&2);}
    return view;
  }
  return {};
}
}
