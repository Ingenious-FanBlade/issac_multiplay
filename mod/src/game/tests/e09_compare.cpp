#include "game/tests/e09_compare.h"
#include "game/client/scene/client_preparation.h"
#include "game/participants/topology.h"
#include "replica/stream_e2e.h"
#include "state/host_publish.h"
#include "state_host_fields.inc"
#include <algorithm>
#include <atomic>
#include <cstring>

namespace cs::game {
namespace {
namespace S=state;
using replica::stream::E09Field;
std::atomic<bool> enabled{false};
std::array<std::uint64_t,2> last_render{};

// J273 独立原生读取位置，对应 Lua 公共字段。只选现行合约中的标量，不把权威玩法字段当作客机差异。
struct Field {const char *name;std::uint16_t offset;};
constexpr Field entity[]={
  {"type",0x28},{"variant",0x2c},{"subtype",0x30},{"spawner_type",0x34},{"spawner_variant",0x38},
  {"visible",0x171},{"exists",0x172},{"dead",0x173},{"grid_collision_class",0x184},{"entity_collision_class",0x188},
  {"position.x",0x33c},{"position.y",0x340},{"position_offset.x",0x34c},{"position_offset.y",0x350},
  {"velocity.x",0x360},{"velocity.y",0x364},{"size",0x370},{"size_multi.x",0x374},{"size_multi.y",0x378},
  {"hit_points",0x380},{"max_hit_points",0x384},{"init_seed",0x3ec},
  {"_maxHearts",0x1340},{"_redHearts",0x1344},{"_soulHearts",0x134c},
  {"_numKeys",0x135c},{"_numBombs",0x1364},{"_numCoins",0x1368},{"_playerType",0x13bc}
};
constexpr Field grid[]={{"desc.type",4},{"desc.variant",8},{"desc.state",12},{"grid_index",0x24}};

bool Sample(std::uint64_t revision,std::uint32_t frame) noexcept {
  return revision&&frame%3==0&&enabled.load(std::memory_order_relaxed);
}

void Record(const char *role,const S::StateScope &scope,std::uint64_t revision,std::uint32_t frame,std::uint64_t tick,
            unsigned stream,const S::PresentationMember &id,std::span<const std::byte> bytes,
            const void *address) noexcept {
  if((id.space!=1&&id.space!=2)||bytes.empty())return;
  const auto *layout=S::FindFlatLayout(id.layout);
  if(!layout)return;
  const auto selected=id.space==1?std::span<const Field>(entity):std::span<const Field>(grid);
  std::array<E09Field,std::size(entity)> values{};
  std::size_t size=0;
  for(const auto &field:selected){
    if(field.offset>=0x1000&&id.layout!=host_field::EntityPlayer::kLayout)continue;
    const auto found=std::find_if(layout->fields.begin(),layout->fields.end(),
                                 [&](const auto &v){return v.name==field.name;});
    if(found==layout->fields.end()||!S::FlatFieldInStream(*layout,std::uint16_t(found-layout->fields.begin()),stream)||
       found->size>4||found->offset+found->size>bytes.size())continue;
    auto &result=values[size++];
    result.name=field.name;
    std::memcpy(&result.wire,bytes.data()+found->offset,found->size);
    result.read=address&&ReadMemory(static_cast<const std::byte *>(address)+field.offset,&result.native,found->size);
  }
  if(size)replica::stream::E2ECompareFields(role,stream,scope,revision,frame,tick,
      {id.space,id.id,id.use,id.shape},std::span(values).first(size));
}
}

void E09Enable(bool on) noexcept {
  if(on)last_render={};
  enabled.store(on,std::memory_order_relaxed);
}
void E09Host(const S::HostStateSnapshot &snapshot,const S::CapturePacket &packet) noexcept {
  const auto &w=snapshot.witness;
  if(!Sample(w.revision,w.game_frame))return;
  const bool ordinary=w.phase==S::CapturePhase::Ordinary;
  for(const auto &record:snapshot.records){
    if(!record->flat||record->flat_suspended||(!ordinary&&record->stream!=S::BodyStream::Body))continue;
    const auto retained=std::any_of(snapshot.retained_members.begin(),snapshot.retained_members.end(),
                                   [&](const auto &entry){return entry.member==record->member.index;});
    const auto &id=record->member.identity;
    // Body 快照也携带其他身体的上一份值，只把本次 PlayerPost 实际更新的根交给同 tick Lua 对照。
    const bool updated=!ordinary&&std::any_of(packet.records.begin(),packet.records.end(),[&](const auto &root){
      const auto &s=root.subject;
      return !root.retained&&s.space==id.space&&s.local==id.id&&s.use==id.use;
    });
    if(!retained)Record("host",w.scope,w.revision,w.game_frame,updated?w.tick:0,unsigned(record->stream),id,
                        record->flat->Bytes(),nullptr);
  }
}
void E09Client(const S::ClientStateBlockTable &table,const ClientSceneMainPreparation &preparation,unsigned mask) noexcept {
  if(!enabled.load(std::memory_order_relaxed))return;
  for(unsigned stream=0;stream<2;++stream){
    if(!(mask&(1u<<stream))||!table.streams[stream])continue;
    const auto &state=*table.streams[stream];
    const auto &ctx=state.Context();
    if(!Sample(ctx.source_revision,ctx.game_frame))continue;
    for(std::size_t i=0;i<state.RecordCount();++i){
      const auto record=state.Record(i);
      if(!record.Flat()||state.FlatFault(record.Identity()))continue;
      if(record.Identity().space!=1&&record.Identity().space!=2)continue;
      std::uint64_t instance=0;
      const auto *object=stream==0?preparation.FindWorldMemberForReadback(record.Identity()):
        preparation.FindLiveMember(record.Identity(),instance);
      const char *role=state.DeferredResource(record.Member())?"client-retained":"client";
      Record(role,ctx.scope,ctx.source_revision,ctx.game_frame,0,stream,record.Identity(),record.Flat()->Bytes(),object);
    }
  }
}
void E09ClientRender(const S::ClientStateBlockTable &table) noexcept {
  if(!enabled.load(std::memory_order_relaxed))return;
  // 同一发布版本可能被多次绘制；只记成功返回的首次可比版本。
  for(unsigned stream=0;stream<2;++stream){
    if(!table.streams[stream])continue;
    const auto &ctx=table.streams[stream]->Context();
    if(Sample(ctx.source_revision,ctx.game_frame)&&last_render[stream]!=ctx.source_revision){
      last_render[stream]=ctx.source_revision;
      replica::stream::E2ECompareRendered(stream,ctx.source_revision,ctx.game_frame);
    }
  }
}
}
