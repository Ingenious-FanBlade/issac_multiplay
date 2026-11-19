#include "game/client/display/client_display.h"
#include "state/client_description_memory.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace cs::game {
namespace {
#include "state_host_fields.inc"
namespace F=host_field;
struct DisplayCopyPlan final {DisplayRoot root;std::uint16_t layout,field;std::uint32_t offset,size;};
#include "state_display_install.inc"
[[noreturn]] void Fail(const char *reason){throw state::Gap("client_display",reason);}
template<class T> T Exact(std::span<const std::byte> bytes) {
  if(bytes.size()!=sizeof(T))Fail("display_scalar_size");T v;std::memcpy(&v,bytes.data(),sizeof(v));return v;
}
std::span<const std::byte> Field(const state::ClientShadowRecordView &r,std::uint16_t field) {
  const auto *v=r.Field(field);if(!v||!v->present)Fail("display_required_field");
  const auto *definition=state::FindField(r.Identity().layout,field);
  if(!definition)Fail("display_field_plan");
  // 嵌套、链接与容器已经由完整影子递归核定，不能将其编码误当作标量数组再校验。
  if(!definition->nested_layout&&(definition->operation==state::Operation::Copy||definition->operation==state::Operation::Convert)&&
     !state::ValidBodySourceScalars(r.Identity().layout,field,v->bytes))Fail("display_invalid_scalar");return v->bytes;
}
state::ClientShadowRecordView Record(const state::ClientShadowState &s,const state::PresentationMember &key) {
  auto r=s.Find(key);if(!r)Fail("display_missing_record");return *r;
}
}
struct ClientDisplayStorage final {
  state::ClientShadowStateRef world;
  ClientDisplaySelection selection;
  ClientRoomLevelDescription rooms;
  std::array<ClientDisplayCopy,64> copies{};
  std::array<std::int32_t,80> effects{};
  std::uint16_t copy_count=0,effect_count=0;
  std::uint32_t time_base=0;
  bool paused=false;
  std::uint8_t branch=0;
  float sfx=1;
  ClientDisplayMetadata metadata;
  ClientDisplayInjections injections;
  std::vector<std::uint64_t,state::ClientDescriptionAllocator<std::uint64_t>> map_facts;
  memory::Lease charge;
  ClientDisplayStorage(state::ClientShadowStateRef w,const ClientDisplaySelection &s,ClientRoomLevelDescription r,const std::shared_ptr<state::ClientStateMemory> &m)
    :world(std::move(w)),selection(s),rooms(std::move(r)),map_facts(state::ClientDescriptionAllocator<std::uint64_t>(m)) {}
  void Copy(DisplayRoot root,std::uint32_t offset,std::span<const std::byte> bytes) {
    if(copy_count==copies.size()||bytes.empty()||bytes.size()>88)Fail("display_copy_budget");
    copies[copy_count++]={root,offset,bytes.data(),std::uint32_t(bytes.size())};
  }
};
ClientDisplayWorld::ClientDisplayWorld(std::shared_ptr<const ClientDisplayStorage> s):storage_(std::move(s)){}
const state::PresentationBatchContext &ClientDisplayWorld::Context() const noexcept{return storage_->world->Context();}
const ClientDisplaySelection &ClientDisplayWorld::Selection() const noexcept{return storage_->selection;}
const ClientRoomLevelDescription &ClientDisplayWorld::Rooms() const noexcept{return storage_->rooms;}
std::span<const ClientDisplayCopy> ClientDisplayWorld::Copies() const noexcept{return {storage_->copies.data(),storage_->copy_count};}
std::uint32_t ClientDisplayWorld::TimeBase() const noexcept{return storage_->time_base;}
bool ClientDisplayWorld::Paused() const noexcept{return storage_->paused;}
std::uint8_t ClientDisplayWorld::Branch() const noexcept{return storage_->branch;}
float ClientDisplayWorld::SfxModifier() const noexcept{return storage_->sfx;}
const ClientDisplayMetadata &ClientDisplayWorld::Metadata() const noexcept{return storage_->metadata;}
const ClientDisplayInjections &ClientDisplayWorld::Inputs() const noexcept{return storage_->injections;}
std::span<const std::uint64_t> ClientDisplayWorld::MapFacts() const noexcept{return storage_->map_facts;}
long ClientDisplayWorld::StorageReferences()const noexcept{return storage_.use_count();}
ClientDisplayMemoryRef ClientDisplayWorld::Memory()const noexcept{return storage_->map_facts.get_allocator().Memory();}
ClientDisplayMemoryRef ClientDisplayMemory(const ClientDisplayWorldRef &w) noexcept{return w?w->Memory():nullptr;}
ClientDisplayMetadata ClientDisplayMeta(const ClientDisplayWorldRef &w) noexcept {
  return w->Metadata();
}
ClientDisplayInjections ClientDisplayInputs(const ClientDisplayWorldRef &w) noexcept {return w->Inputs();}
ClientDisplayArray<ClientDisplayCopy> ClientDisplayCopies(const ClientDisplayWorldRef &w) noexcept {
  const auto values=w->Copies();return {values.data(),values.size()};
}
unsigned ClientDisplayMapCount(const ClientDisplayWorldRef &w) noexcept{return w->Rooms().DescriptorCount();}
ClientDisplayMapValue ClientDisplayMapAt(const ClientDisplayWorldRef &w,unsigned i) {
  const auto d=w->Rooms().DescriptorAt(std::uint16_t(i));const auto icons=d.SavedIcons();ClientDisplayMapValue v;
  const auto values=d.Values();v.grid=values.grid;v.dimension=values.dimension;v.display_flags=values.display_flags;
  v.spawn_seed=values.spawn_seed;v.spawn_seed_present=values.spawn_seed_present;v.visited=values.visited;v.display_bits=values.display_bits;
  v.slot=d.SourceSlot();v.mask=icons.mask;v.present=icons.present;std::copy(icons.frames.begin(),icons.frames.end(),v.frames);return v;
}
bool ClientDisplaySameMap(const ClientDisplayWorldRef &a,const ClientDisplayWorldRef &b) noexcept {
  return a&&b&&std::ranges::equal(a->MapFacts(),b->MapFacts());
}
ClientDisplayWorldRef ClientDescribeDisplayWorld(state::ClientShadowStateRef world,
    const ClientDisplaySelection &selection,ClientRoomLevelDescription rooms,std::shared_ptr<state::ClientStateMemory> peer) {
  if(!world)Fail("display_world_missing");const auto &c=world->Context();const auto &r=rooms.Context();
  if(c.stream!=state::BodyStream::World||c.layout_version!=state::LayoutVersion()||!c.target_version||
     r.layout_version!=c.layout_version||r.target_version!=c.target_version||r.game_frame!=c.game_frame||
     r.world_revision!=c.source_revision||r.member_epoch!=c.member_epoch||r.resource_epoch!=c.resource_epoch||
     r.room.run!=c.scope.run||r.room.id!=c.scope.room||r.floor.id!=c.scope.floor)Fail("display_context_mismatch");
  const auto same=[](const ClientWorldShadowIdentity &a,const state::PresentationMember &b){return a.key.id==b.id&&
    a.key.space==b.space&&a.layout==b.layout&&a.use==b.use&&a.shape==b.shape;};
  if(!same(rooms.Room().Identity(),selection.room)||!same(rooms.Level().Identity(),selection.level)||
     !same(rooms.Room().Descriptor().Identity(),selection.descriptor)||selection.game.layout!=kDisplayGameRun||
     selection.room.layout!=kDisplayRoom||selection.sfx.layout!=kDisplaySFXVolumeState)Fail("display_root_identity");
  auto charge=peer?memory::Lease{}:memory::Reserve(memory::Pool::Values,sizeof(ClientDisplayStorage)+sizeof(ClientDisplayWorld)+128,"client_display_target");
  auto s=peer?std::allocate_shared<ClientDisplayStorage>(state::ClientStateAllocator<ClientDisplayStorage>(peer),std::move(world),selection,std::move(rooms),peer):
    std::make_shared<ClientDisplayStorage>(std::move(world),selection,std::move(rooms),peer);s->charge=std::move(charge);
  const auto game=Record(*s->world,selection.game),room=Record(*s->world,selection.room),
    level=Record(*s->world,selection.level),descriptor=Record(*s->world,selection.descriptor),sfx=Record(*s->world,selection.sfx);
  for(const auto &p:kDisplayCopies){const auto &source=p.layout==game.Identity().layout?game:
      p.layout==room.Identity().layout?room:p.layout==level.Identity().layout?level:descriptor;
    const auto bytes=Field(source,p.field);if(bytes.size()!=p.size)Fail("display_copy_size");s->Copy(p.root,p.offset,bytes);}
  // 保留线上 Room.render_scroll，交给新相机初始化。每帧显示标量回写由独立表驱动。
  s->injections.camera_initial_scroll=Exact<std::array<float,2>>(Field(room,kDisplayRoom_render_scroll));
  s->time_base=Exact<std::uint32_t>(Field(game,kDisplayGameRun_time_counter_base));
  s->paused=Exact<std::uint8_t>(Field(game,kDisplayGameRun_host_paused))!=0;
  s->branch=Exact<std::uint8_t>(Field(game,kDisplayGameRun_display_branch));
  s->sfx=Exact<float>(Field(sfx,kDisplaySFXVolumeState_volume_modifier));
  if(!std::isfinite(s->sfx)||s->branch>12)Fail("display_pause_branch_or_sfx");
  // Seeds只解码小型纯值树摘要，不执行原生构造／增删效果／SetStartSeed。
  std::array<state::ProjectionFact,8> facts{};std::array<state::BodyFieldView,4> fields{};
  state::BodyCodecScratch scratch{facts,0,fields};
  const auto seeds=state::DecodeValue(kDisplayRunSeeds,Field(game,kDisplayGameRun_seeds),scratch);
  const auto ids=std::find_if(seeds.fields.begin(),seeds.fields.end(),[](const auto &v){return v.field==kDisplayRunSeeds_effect_tree_head;});
  if(ids==seeds.fields.end()||!ids->present||ids->bytes.size()<2)Fail("display_seed_ids_missing");
  s->effect_count=Exact<std::uint16_t>(ids->bytes.first(2));
  if(s->effect_count>80||ids->bytes.size()!=2+4u*s->effect_count)Fail("display_seed_ids_size");
  std::int32_t previous=-1;
  for(unsigned i=0;i<s->effect_count;++i){const auto id=Exact<std::int32_t>(ids->bytes.subspan(2+4*i,4));
    if(id<0||id>=80||id<=previous)Fail("display_seed_ids_order");s->effects[i]=previous=id;}
  // Shockwave完整28B均为具名纯值；不复制本地指针、surface或模拟payload。
  for(const auto index:{kDisplayRoom_shockwave_a,kDisplayRoom_shockwave_b}){
    const auto *f=state::FindField(room.Identity().layout,index);const auto bytes=Field(room,index);
    if(!f||bytes.size()!=28||f->native_size!=28||f->nested_layout!=kDisplayRoomShockwave)Fail("display_shockwave_plan");
    s->Copy(DisplayRoot::Room,f->native_offset,bytes);}
  // HellBackdrop的两个条件标量位于可变Sprite之后的固定尾部；ANM2仍由原资源owner安装。
  const auto hell=Field(room,kDisplayRoom_hell_backdrop);
  if(hell.empty()||std::to_integer<unsigned>(hell.front())>1)Fail("display_hell_presence");
  if(hell.front()!=std::byte{0}){
    if(hell.size()<9)Fail("display_hell_tail");
    const auto *parent=state::FindField(kDisplayRoom,kDisplayRoom_hell_backdrop);
    for(const auto [field,tail]:{std::pair{kDisplayRoomHellBackdrop_display_offset,8u},std::pair{kDisplayRoomHellBackdrop_accumulated_offset,4u}}){
      const auto *f=state::FindField(kDisplayRoomHellBackdrop,field);const auto bytes=hell.subspan(hell.size()-tail,4);
      if(!f||!state::ValidBodySourceScalars(kDisplayRoomHellBackdrop,field,bytes))Fail("display_hell_value");
      s->Copy(DisplayRoot::Room,parent->native_offset+f->native_offset,bytes);}}
  const auto current=s->rooms.Room().Descriptor().Values();auto &meta=s->metadata;
  meta.scope={c.scope.session,c.scope.run,c.scope.floor,c.scope.dimension,c.scope.room,c.scope.scene};
#ifdef ISAACCS_E2E
  meta.source_revision=c.source_revision;
#endif
  meta.frame=c.game_frame;meta.time_base=s->time_base;meta.spawn_seed=current.spawn_seed;meta.grid=current.grid;meta.dimension=current.dimension;
  meta.paused=s->paused;meta.branch=static_cast<ClientDisplayBranch>(s->branch);meta.sfx=s->sfx;meta.room_use=selection.room.use;meta.room_shape=selection.room.shape;
  meta.challenge=Exact<std::uint32_t>(Field(game,kDisplayGameRun_challenge));
  {
    const auto input=Field(game,kDisplayGameRun_display_injections);
    std::array<state::ProjectionFact,64> focus_facts{};
    std::array<state::BodyFieldView,24> focus_fields{};
    state::BodyCodecScratch focus_scratch{focus_facts,0,focus_fields};
    const auto value=state::DecodeValue(F::DisplayInjectionState::kLayout,input,focus_scratch);
    using namespace F::DisplayInjectionState;
    for(const auto &f:value.fields)if(f.present){
      switch(f.field){
        case devil_total:s->injections.devil=Exact<float>(f.bytes);break;
        case angel_share:s->injections.angel=Exact<float>(f.bytes);break;
        case planetarium:s->injections.planetarium=Exact<float>(f.bytes);break;
        case deal_qualified:s->injections.deal_qualified=Exact<std::uint8_t>(f.bytes)!=0;break;
        case progress_blocked:s->injections.progress_blocked=Exact<std::uint8_t>(f.bytes)!=0;break;
        case pill_effects:s->injections.pill_effects=Exact<std::array<std::int32_t,15>>(f.bytes);break;
        case pill_identified:s->injections.pill_identified=Exact<std::array<std::uint8_t,15>>(f.bytes);break;
        case water_current:s->injections.water=Exact<std::array<float,2>>(f.bytes);break;
        case map_focus:meta.map_focus[0]=Exact<std::int32_t>(f.bytes);break;
        case map_focus_large:meta.map_focus[1]=Exact<std::int32_t>(f.bytes);break;
        case backdrop_wall_style:s->injections.backdrop_wall_style=Exact<std::int32_t>(f.bytes);break;
        case backdrop_tutorial:s->injections.backdrop_tutorial=Exact<std::uint8_t>(f.bytes)!=0;break;
        case room_graphics_flags:s->injections.room_graphics_flags=Exact<std::uint8_t>(f.bytes);break;
        case camera_entry_door:s->injections.camera_entry_door=Exact<std::int32_t>(f.bytes);break;
        case camera_previous_entry:s->injections.camera_previous_entry=Exact<std::int32_t>(f.bytes);break;
        case camera_return_focus:s->injections.camera_return_focus=Exact<std::uint8_t>(f.bytes)!=0;break;
        case camera_return_position:s->injections.camera_return_position=Exact<std::array<float,2>>(f.bytes);break;
        case poop_fx_countdown:s->injections.poop_fx_countdown=Exact<std::int32_t>(f.bytes);break;
        case poop_fx_color:s->injections.poop_fx_color=Exact<std::array<float,11>>(f.bytes);break;
      }
    }
    if(meta.challenge==32)for(const int focus:meta.map_focus)if(focus<0||focus>=169)Fail("display_challenge32_map_focus_missing");
  }
  meta.transition_mode=Exact<std::int32_t>(Field(game,kDisplayGameRun_transition_mode));
  meta.transition_direction=Exact<std::int32_t>(Field(game,kDisplayGameRun_transition_direction));
  meta.transition_animation=Exact<std::int32_t>(Field(game,kDisplayGameRun_transition_animation));
  meta.transition_boss=Exact<std::int32_t>(Field(game,kDisplayGameRun_transition_boss_marker));
  // 保存明确输入值用于比较；这是有界向量比较，不生成或校验任何哈希。
  auto &map=s->map_facts;const auto map_capacity=32+80+s->rooms.DescriptorCount()*26u;
  if(s->charge)s->charge->Resize(sizeof(ClientDisplayStorage)+sizeof(ClientDisplayWorld)+128+map_capacity*sizeof(std::uint64_t));map.reserve(map_capacity);
  const auto level_values=s->rooms.Level().Values();
  meta.entry=level_values.current_entry;const auto config=s->rooms.Room().Descriptor().Configuration();
  if(!config)Fail("display_current_room_config_missing");meta.width=config->width;meta.height=config->height;
  map={c.scope.run,c.scope.floor,c.scope.dimension,c.scope.room,c.scope.scene,level_values.stage,level_values.stage_type,
    level_values.raw_curses,std::uint32_t(level_values.current_entry),std::uint32_t(meta.map_focus[0]),std::uint32_t(meta.map_focus[1])};
  for(const auto index:{kDisplayGameRun_temporary_curses,kDisplayGameRun_challenge,kDisplayGameRun_difficulty})map.push_back(Exact<std::uint32_t>(Field(game,index)));
  for(unsigned i=0;i<s->effect_count;++i)map.push_back(std::uint32_t(s->effects[i]));map.push_back(UINT64_MAX);
  for(unsigned i=0;i<s->rooms.DescriptorCount();++i){const auto d=s->rooms.DescriptorAt(std::uint16_t(i));const auto v=d.Values();const auto icons=d.SavedIcons();
    for(auto value:{std::uint64_t(d.SourceSlot()),d.Identity().key.id,d.Identity().use,d.Identity().shape,
        std::uint64_t(v.display_flags),std::uint64_t(v.visited),std::uint64_t(v.display_bits),std::uint64_t(v.greed_treasure_icon),
        std::uint64_t(icons.present),std::uint64_t(icons.mask)})map.push_back(value);
    for(auto value:icons.frames)map.push_back(value);}
  if(peer)return std::allocate_shared<ClientDisplayWorld>(state::ClientStateAllocator<ClientDisplayWorld>(peer),std::move(s));
  return ClientDisplayWorldRef(new ClientDisplayWorld(std::move(s)));
}
} // namespace cs::game
