#include "game/native_room_grid.h"
#include "game/world_bridge.h"
#include "replica/native_room_grid.h"
#include "runtime.h"
#include <memory>
#include <sstream>

namespace cs::game {
// 先由15的域增量语义重建完整T；后续仍从本地最后成功的C调和，绝不直接patch到C。
std::string ReconstructNativeRoomGridTarget(const std::string &baseline,const std::string &target){
  auto b=replica::PrepareNativeRoomGrid(baseline),t=replica::PrepareNativeRoomGrid(target);
  const replica::Limits limits{32u<<20,4194304,64,2u<<20};const std::vector<std::string> domains{"grid","room"};
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
  out.external_dependencies.assign(target.external_dependencies.begin(),target.external_dependencies.end());return out;
}
namespace {
// 身份Registry只属于正式观测代次，连续采集不会因测试命令或录制开关换ID。
std::unique_ptr<replica::Registry> source_registry;
std::uint64_t source_session=0,source_epoch=0,source_run=0,source_revision=0,source_scene=0,source_room=0;
std::string source_error;
}
std::string CaptureNativeRoomGridSource(){
  try{
    const auto c=CurrentWorldRecording();
    if(!IsMain()||!c||!c->formal_consumer||!c->collector)throw std::runtime_error("native_source_inactive");
    const auto &r=c->latest_room,&g=c->latest_grid;using namespace world::observation;
    const auto run=c->collector->CurrentRunToken(),room=c->collector->CurrentRoomToken(),mutation=c->collector->CurrentMutation();
    if(!run||!room||r.result.coverage!=Coverage::Complete||g.result.coverage!=Coverage::Complete||
       r.context.authority_tick.presence!=Presence::Known||g.context.authority_tick.presence!=Presence::Known||
       r.context.authority_tick.value!=g.context.authority_tick.value||r.context.mutation_after!=mutation||g.context.mutation_after!=mutation||
       r.context.floor.presence!=Presence::Known||r.context.dimension.presence!=Presence::Known||r.context.room.presence!=Presence::Known)
      throw std::runtime_error("native_source_incomplete_window");
    if(source_session!=c->session_key||source_epoch!=c->epoch||source_run!=run){
      source_session=c->session_key;source_epoch=c->epoch;source_run=run;source_revision=source_scene=source_room=0;
      source_registry=std::make_unique<replica::Registry>(source_session,run);
    }
    if(source_revision==UINT64_MAX||source_scene==UINT64_MAX)throw std::runtime_error("native_source_revision_exhausted");
    if(source_room!=room){++source_scene;source_room=room;}
    replica::Scope scope{source_session,1,run,r.context.floor.value.local,r.context.dimension.value.local,r.context.room.value.local,source_scene};
    auto target=replica::Capture(scope,++source_revision,{g,r},*source_registry,replica::Limits{32u<<20,4194304,64,2u<<20});
    source_error.clear();return replica::Text(target);
  }catch(const std::exception &e){source_error=e.what();throw;}
}
std::string NativeRoomGridSourceFields(){
  std::ostringstream o;o<<"native.source.error="<<source_error<<"\nnative.source.revision="<<source_revision<<'\n';
  const auto c=CurrentWorldRecording();if(c){o<<"native.source.grid="<<world::observation::CoverageName(c->latest_grid.result.coverage)
    <<"\nnative.source.room="<<world::observation::CoverageName(c->latest_room.result.coverage)<<'\n';}return o.str();
}
} // namespace cs::game
