#include "game/native_room_grid.h"
#include "game/world_bridge.h"
#include "replica/native_room_grid.h"
#include "replica/native_entity.h"
#include "runtime.h"
#include <memory>
#include <sstream>

namespace cs::game {
// 先由15的域增量语义重建完整T；后续仍从本地最后成功的C调和，绝不直接patch到C。
std::string ReconstructNativeRoomGridTarget(const std::string &baseline,const std::string &target){
  auto b=replica::PrepareNativeRoomGrid(baseline),t=replica::PrepareNativeRoomGrid(target);
  const replica::Limits limits{32u<<20,4194304,64,2u<<20};std::vector<std::string> domains{"grid","room"};
  if(t.model["components"].HasMember("entity"))domains.insert(domains.begin(),"entity");
  auto delta=replica::Difference(b.model,t.model,limits,domains);auto rebuilt=replica::Reconstruct(b.model,delta,limits,domains);
  return replica::Text(rebuilt);
}
NativeRoomGridTransaction PrepareNativeRoomGridTransaction(const std::string &current,
    const std::string &text,bool healthy){
  auto target=replica::PrepareNativeRoomGrid(text);
  std::unique_ptr<replica::NativeRoomGridTarget> previous;
  if(!current.empty())previous=std::make_unique<replica::NativeRoomGridTarget>(replica::PrepareNativeRoomGrid(current));
  const auto plan=replica::PlanNativeRoomGrid(previous.get(),target,healthy);
  NativeRoomGridTransaction out;out.model=text;
  out.connection=target.scope.connection;out.session=target.scope.session;out.run=target.scope.run;
  out.floor=target.scope.floor;out.dimension=target.scope.dimension;out.room=target.scope.room;
  out.scene=target.scope.scene;out.revision=target.revision;out.shape=target.shape;out.grids=unsigned(target.grids.size());
  out.duplicate=plan.disposition==replica::NativeRoomGridDisposition::Duplicate;
  out.stale=plan.disposition==replica::NativeRoomGridDisposition::Stale;out.reset=plan.reset_room;
  out.remove=plan.remove;out.create=plan.create;out.change=plan.change;out.rebuild=plan.rebuild;
  out.has_entities=target.model["components"].HasMember("entity");
  if(out.has_entities){
    auto entities=replica::PrepareNativeEntity(target.model);
    std::unique_ptr<replica::NativeEntityTarget> old;
    if(previous&&previous->model["components"].HasMember("entity"))old=std::make_unique<replica::NativeEntityTarget>(replica::PrepareNativeEntity(previous->model));
    auto e=replica::PlanNativeEntity(old.get(),entities,healthy&&!plan.reset_room);
    if(!old&&plan.disposition==replica::NativeRoomGridDisposition::Duplicate)out.duplicate=false;
    if(e.disposition==replica::NativeEntityDisposition::Apply)out.duplicate=false;
    if(e.disposition==replica::NativeEntityDisposition::Stale)out.stale=true;
    out.entity_reset=e.reset;out.entity_remove=std::move(e.remove);out.entity_create=std::move(e.create);
    out.entity_change=std::move(e.change);out.entity_rebuild=std::move(e.rebuild);
    out.entity_order=entities.creation_order;out.entity_player_dependencies.assign(entities.player_dependencies.begin(),entities.player_dependencies.end());
    out.entity_render_order=entities.render_order;
    out.entity_player_roster=entities.player_roster;for(unsigned i=0;i<4;++i)out.entity_player_esau[i]=entities.player_esau[i];
    // Remove/Morph使用原生Room+私有池的完整退休，避免手工改未知查询缓存；逻辑key仍保留。
    bool rebuild_scene=!out.entity_remove.empty()||!out.entity_rebuild.empty();
    if(old){if(old->player_dependencies!=entities.player_dependencies||old->player_roster!=entities.player_roster||old->player_esau!=entities.player_esau)rebuild_scene=true;
      for(auto id:entities.player_dependencies){const auto before=old->objects.find(id);
        if(before==old->objects.end()||before->second.shape!=entities.objects.at(id).shape)rebuild_scene=true;}}
    if(rebuild_scene){out.reset=out.entity_reset=true;auto full=replica::PlanNativeRoomGrid(previous.get(),target,false);
      out.remove.clear();out.change.clear();out.rebuild.clear();out.create=std::move(full.create);
      auto all=replica::PlanNativeEntity(old.get(),entities,false);out.entity_remove.clear();out.entity_change.clear();out.entity_rebuild.clear();out.entity_create=std::move(all.create);}
  }else if(previous&&previous->model["components"].HasMember("entity")){
    // 两域包没有Entity成员声明，不能把缺席解释为完整空Entity目标。
    throw std::runtime_error("native_scene_entity_component_required");
  }
  out.external_dependencies.assign(target.external_dependencies.begin(),target.external_dependencies.end());return out;
}
namespace {
// 身份Registry只属于正式观测代次，连续采集不会因测试命令或录制开关换ID。
std::unique_ptr<replica::Registry> source_registry;
std::uint64_t source_session=0,source_epoch=0,source_run=0,source_revision=0,source_scene=0,source_room=0;
// 原生run token可以随观测服务重建从1开始；网络run独立单调分配，旧key不能复活。
std::uint64_t source_replica_run=0;
std::string source_error;
}
std::string CaptureSceneSource(bool entity){
  try{
    const auto c=CurrentWorldRecording();
    if(!IsMain()||!c||!c->formal_consumer||!c->collector)throw std::runtime_error("native_source_inactive");
    const auto &r=c->latest_room,&g=c->latest_grid,&e=c->latest_entity;using namespace world::observation;
    const auto run=c->collector->CurrentRunToken(),room=c->collector->CurrentRoomToken(),mutation=c->collector->CurrentMutation();
    if(!run||!room||r.result.coverage!=Coverage::Complete||g.result.coverage!=Coverage::Complete||
       r.context.authority_tick.presence!=Presence::Known||g.context.authority_tick.presence!=Presence::Known||
       r.context.authority_tick.value!=g.context.authority_tick.value||r.context.mutation_after!=mutation||g.context.mutation_after!=mutation||
       r.context.floor.presence!=Presence::Known||r.context.dimension.presence!=Presence::Known||r.context.room.presence!=Presence::Known)
      throw std::runtime_error("native_source_incomplete_window");
    if(entity&&(e.result.coverage!=Coverage::Complete||e.context.authority_tick.presence!=Presence::Known||
      e.context.authority_tick.value!=r.context.authority_tick.value||e.context.mutation_after!=mutation))
      throw std::runtime_error("native_source_incomplete_entity_window");
    const ObservationEnvelope *sources[]={&r,&g,entity?&e:&g};
    for(const auto *value:sources){const auto &context=value->context;
      if(context.phase!=Phase::OrdinaryPost||context.run.presence!=Presence::Known||context.run.value.local!=run||
        context.room.presence!=Presence::Known||context.room.value.local!=room||context.mutation_before!=mutation||
        context.session!=c->session_key||context.epoch!=c->epoch||context.outer!=r.context.outer||context.owner!=r.context.owner)
        throw std::runtime_error("native_source_scope_mismatch");
    }
    if(source_session!=c->session_key||source_epoch!=c->epoch||source_run!=run){
      if(source_replica_run==UINT64_MAX)throw std::runtime_error("native_source_run_exhausted");
      auto next_registry=std::make_unique<replica::Registry>(c->session_key,source_replica_run+1);
      source_session=c->session_key;source_epoch=c->epoch;source_run=run;source_revision=source_scene=source_room=0;
      ++source_replica_run;source_registry=std::move(next_registry);
    }
    if(source_revision==UINT64_MAX||source_scene==UINT64_MAX)throw std::runtime_error("native_source_revision_exhausted");
    if(source_room!=room){++source_scene;source_room=room;}
    replica::Scope scope{source_session,1,source_replica_run,r.context.floor.value.local,r.context.dimension.value.local,r.context.room.value.local,source_scene};
    std::vector<ObservationEnvelope> values;
    // 正式来源只复制已经发布的同窗口纯值；两个入口使用同一个Registry及scope计数器。
    if(entity)values.push_back(e);values.push_back(g);values.push_back(r);
    auto target=replica::Capture(scope,++source_revision,values,*source_registry,replica::Limits{32u<<20,4194304,64,2u<<20});
    source_error.clear();return replica::Text(target);
  }catch(const std::exception &e){source_error=e.what();throw;}
}
std::string CaptureNativeRoomGridSource(){return CaptureSceneSource(false);}
std::string CaptureNativeSceneSource(){return CaptureSceneSource(true);}
std::string NativeRoomGridSourceFields(){
  std::ostringstream o;o<<"native.source.error="<<source_error<<"\nnative.source.revision="<<source_revision
    <<"\nnative.source.run="<<source_replica_run<<"\nnative.source.epoch="<<source_epoch<<'\n';
  const auto c=CurrentWorldRecording();if(c){o<<"native.source.grid="<<world::observation::CoverageName(c->latest_grid.result.coverage)
    <<"\nnative.source.room="<<world::observation::CoverageName(c->latest_room.result.coverage)
    <<"\nnative.source.entity="<<world::observation::CoverageName(c->latest_entity.result.coverage)<<'\n';}return o.str();
}
} // namespace cs::game
