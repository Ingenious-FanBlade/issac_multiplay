#include "game/room_grid_original.h"
#include "game/world_bridge.h"
#include "replica/model.h"
#include "runtime.h"
#include <memory>
#include <sstream>
#include <filesystem>
#include <fstream>

namespace cs::game {
namespace {
// Registry只属于正式Source的观测代次，和录制文件/测试控制口没有生命周期关联。
std::unique_ptr<replica::Registry> registry;
std::uint64_t session=0,epoch=0,run=0,revision=0,scene=0,last_room=0;
std::string source_error;
// 只有显式E2E的失败现场暂存原始纯值，按room去重，不参与正式资源采集。
void FailedSource(){static std::uint64_t saved_room=0;auto c=CurrentWorldRecording();if(Settings.world_e2e_token.empty()||!c||!c->collector)return;const auto room=c->collector->CurrentRoomToken();if(room==saved_room)return;saved_room=room;
 const auto path=std::filesystem::u8path(Settings.world_output_dir)/"original";std::filesystem::create_directories(path);
 std::ofstream(path/"source-error-grid.json",std::ios::binary)<<world::observation::EncodeEnvelope(c->latest_grid);
 std::ofstream(path/"source-error-room.json",std::ios::binary)<<world::observation::EncodeEnvelope(c->latest_room);
}
}
bool ReadOriginalRoomSource(OriginalRoomSource &out) noexcept {
  try {
    auto c=CurrentWorldRecording();if(!IsMain()||!c||!c->formal_consumer||!c->collector)return false;
    const auto &g=c->latest_grid,&r=c->latest_room;
    using namespace world::observation;
    if(g.result.coverage!=Coverage::Complete||r.result.coverage!=Coverage::Complete||
       g.context.authority_tick.presence!=Presence::Known||r.context.authority_tick.presence!=Presence::Known||
       g.context.authority_tick.value!=r.context.authority_tick.value||r.context.room.presence!=Presence::Known||
       r.context.floor.presence!=Presence::Known||r.context.dimension.presence!=Presence::Known)return false;
    const auto current_run=c->collector->CurrentRunToken(),current_room=c->collector->CurrentRoomToken(),mutation=c->collector->CurrentMutation();
    if(!current_run||!current_room||g.context.mutation_after!=mutation||r.context.mutation_after!=mutation)return false;
    if(session!=c->session_key||epoch!=c->epoch||run!=current_run){
      session=c->session_key;epoch=c->epoch;run=current_run;revision=0;last_room=0;
      registry=std::make_unique<replica::Registry>(session,run);
    }
    if(revision==UINT64_MAX||scene==UINT64_MAX)return false;
    if(last_room!=current_room){++scene;last_room=current_room;}
    replica::Scope scope{session,1,run,r.context.floor.value.local,r.context.dimension.value.local,r.context.room.value.local,scene};
    // 原版2x2房的完整Grid可超过百万节点，专用域在32MiB内允许四百万节点。
    auto model=replica::Capture(scope,++revision,{g,r},*registry,replica::Limits{32u<<20,4194304,64,2u<<20});
    out={replica::Text(model),session,epoch,current_run,current_room,mutation,revision};source_error.clear();return true;
  }catch(const std::exception &e){source_error=e.what();try{FailedSource();}catch(...){}return false;}catch(...){source_error="source_copy_exception";return false;}
}
// 正式源不完整时公开原因，等待窗口不能被误报告为采集已完成。
std::string OriginalRoomSourceFields(){std::ostringstream s;s<<"original.source.error="<<source_error<<"\n";auto c=CurrentWorldRecording();if(!c)return s.str();
 for(const auto &entry:{std::make_pair("grid",&c->latest_grid),std::make_pair("room",&c->latest_room)}){const auto &e=*entry.second;s<<"original.source."<<entry.first<<".coverage="<<world::observation::CoverageName(e.result.coverage)<<"\noriginal.source."<<entry.first<<".tick="<<e.context.authority_tick.value<<"\noriginal.source."<<entry.first<<".mutation="<<e.context.mutation_after<<"\noriginal.source."<<entry.first<<".bytes="<<e.payload.size()<<"\noriginal.source."<<entry.first<<".reason=";for(const auto &r:e.result.reasons)s<<r<<",";s<<"\n";}
 if(c->collector)s<<"original.source.mutation="<<c->collector->CurrentMutation()<<"\n";return s.str();}
bool OriginalRoomSourceStable(const OriginalRoomSource &source) noexcept {
  auto c=CurrentWorldRecording();return IsMain()&&c&&c->collector&&c->session_key==source.session&&c->epoch==source.epoch&&
    c->collector->CurrentRunToken()==source.run&&c->collector->CurrentRoomToken()==source.room&&c->collector->CurrentMutation()==source.mutation;
}
std::string OriginalGridKey(const void *grid) noexcept {
  try {
    auto c=CurrentWorldRecording();if(!IsMain()||!c||!registry||session!=c->session_key||epoch!=c->epoch)return {};
    const auto *id=c->identities.Find(world::ObjectDomain::Grid,std::uint32_t(reinterpret_cast<std::uintptr_t>(grid)));
    // 当前正式Grid完整快照已核定公开读取资格；FIRST_OBSERVED不补造Init/出生历史。
    if(!id||id->Retired())return {};
    replica::Json scratch;return replica::Text(registry->Key(2,std::to_string(epoch)+"/"+std::to_string(id->local_id),std::to_string(id->use),scratch.GetAllocator()));
  }catch(...){return {};}
}
} // namespace cs::game
