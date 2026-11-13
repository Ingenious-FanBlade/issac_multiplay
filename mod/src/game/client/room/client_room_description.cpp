#include "game/client/room/client_room_description.h"
#include "core/replication_memory.h"
#include "state/nested.h"
#include "state/client_description_memory.h"
#include "world/common/contracts.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>
#include <vector>

namespace cs::game {
namespace {
constexpr auto kRoles=std::size_t(ClientRoomFieldRole::Count);
constexpr std::size_t kDescriptorSlots=527,kGridCells=448;
using Space=world::observation::IdentitySpace;
// 生成表完整覆盖四个正式根；可选嵌套负载仍是一个有界只读投影，不构建字段树。
struct WorldDescriptionPlan final {
  ClientWorldDescriptionKind kind;
  std::uint16_t layout;
  std::uint32_t required_blocks;
  std::array<std::uint16_t,kRoles> fields;
};
struct SavedIconFramePlan final {std::uint8_t bit,maximum;};
#include "state_room_view.inc"
static_assert(kWorldDescriptionRoles==kRoles);
[[noreturn]] void Fail(const char *reason){throw state::Gap("room_level_description",reason);}
const WorldDescriptionPlan &Plan(ClientWorldDescriptionKind kind) {
  for(const auto &p:kWorldDescriptionPlans)if(p.kind==kind)return p;
  Fail("unregistered_root_layout");
}
bool Key(const ClientObjectKey &key,Space space,std::uint64_t run) noexcept {
  return key.run==run&&run&&key.id&&key.space==unsigned(space);
}
bool PhysicalSlot(std::uint16_t slot) noexcept {return slot<kDescriptorSlots&&slot!=525;}
// Slice仅指向描述拥有的连续字节；版本保留最后变化时刻，而非强行改成target_version。
struct Slice final {const state::Field *field=nullptr;std::uint32_t offset=0,size=0,version=0;};
struct OwnedRecord final {
  ClientWorldShadowIdentity identity;
  const WorldDescriptionPlan *plan=nullptr;
  std::array<Slice,kWorldDescriptionMaxFields> fields{};
};
struct OwnedDescriptor final {
  std::uint16_t source_slot=UINT16_MAX,ordinary_ordinal=UINT16_MAX;
  OwnedRecord record;
};
}
// 所有描述／投影共享一次Values租约；无原生对象、输入span或借用资源表地址。
struct ClientRoomLevelStorage final {
  ClientRoomLevelContext context;
  OwnedRecord room,level;
  std::vector<OwnedDescriptor,state::ClientDescriptionAllocator<OwnedDescriptor>> descriptors;
  std::vector<ClientRoomConfiguration,state::ClientDescriptionAllocator<ClientRoomConfiguration>> configurations;
  std::vector<std::uint8_t,state::ClientDescriptionAllocator<std::uint8_t>> bytes;
  std::array<std::uint16_t,kDescriptorSlots> by_slot{},ordinary{};
  std::array<std::uint16_t,507> occupancy{};
  std::array<std::optional<ClientRoomGridMember>,8> doors{};
  std::uint16_t current_descriptor=UINT16_MAX,ordinary_count=0;
  memory::Lease charge;
  explicit ClientRoomLevelStorage(const std::shared_ptr<state::ClientStateMemory> &m)
    :descriptors(state::ClientDescriptionAllocator<OwnedDescriptor>(m)),configurations(state::ClientDescriptionAllocator<ClientRoomConfiguration>(m)),bytes(state::ClientDescriptionAllocator<std::uint8_t>(m)) {}
};
namespace {
const OwnedRecord &Record(const ClientRoomLevelStorage &s,ClientWorldDescriptionKind kind,std::uint16_t index=0) noexcept {
  if(kind==ClientWorldDescriptionKind::Room)return s.room;
  if(kind==ClientWorldDescriptionKind::Level)return s.level;
  return s.descriptors[index].record;
}
const Slice *FindSlice(const OwnedRecord &r,ClientRoomFieldRole role) noexcept {
  if(std::size_t(role)>=kRoles)return nullptr;
  const auto index=r.plan->fields[std::size_t(role)];
  return index==UINT16_MAX?nullptr:&r.fields[index];
}
const Slice &Required(const OwnedRecord &r,ClientRoomFieldRole role) {
  const auto *slice=FindSlice(r,role);if(!slice||!slice->field)Fail("field_not_applicable");return *slice;
}
ClientWorldBytes Bytes(const ClientRoomLevelStorage &s,const Slice &slice) noexcept {
  return {slice.size?s.bytes.data()+slice.offset:nullptr,slice.size};
}
template<class T> T Exact(ClientWorldBytes b) {
  if(b.size!=sizeof(T)||!b.data)Fail("scalar_or_array_size");
  T v{};std::memcpy(&v,b.data,sizeof(v));return v;
}
template<class T> T Value(const ClientRoomLevelStorage &s,const OwnedRecord &r,ClientRoomFieldRole role) {
  return Exact<T>(Bytes(s,Required(r,role)));
}
template<std::size_t N> std::array<float,N> Floats(const ClientRoomLevelStorage &s,const OwnedRecord &r,ClientRoomFieldRole role) {
  auto v=Value<std::array<float,N>>(s,r,role);for(float x:v)if(!std::isfinite(x))Fail("nonfinite_value");return v;
}
float Float(const ClientRoomLevelStorage &s,const OwnedRecord &r,ClientRoomFieldRole role) {
  const auto v=Value<float>(s,r,role);if(!std::isfinite(v))Fail("nonfinite_value");return v;
}
bool Boolean(const ClientRoomLevelStorage &s,const OwnedRecord &r,ClientRoomFieldRole role) {
  const auto v=Value<std::uint8_t>(s,r,role);if(v>1)Fail("boolean_value");return v!=0;
}
ClientRoomValues RoomValues(const ClientRoomLevelStorage &s) {
  const auto &r=s.room;using R=ClientRoomFieldRole;ClientRoomValues v;
  v.descriptor_slot=Value<std::int32_t>(s,r,R::DescriptorSlot);v.dimension=Value<std::int32_t>(s,r,R::Dimension);
  v.type=Value<std::uint32_t>(s,r,R::RoomType);v.grid_width=Value<std::uint32_t>(s,r,R::GridWidth);
  v.grid_height=Value<std::uint32_t>(s,r,R::GridHeight);v.bounds=Floats<4>(s,r,R::Bounds);
  v.frame_baseline=Value<std::uint32_t>(s,r,R::FrameBaseline);v.greed_timer=Value<std::int32_t>(s,r,R::GreedTimer);
  v.floor_color=Floats<11>(s,r,R::FloorColor);v.wall_color=Floats<11>(s,r,R::WallColor);
  v.water_amount=Float(s,r,R::WaterAmount);v.lava_intensity=Float(s,r,R::LavaIntensity);
  v.render_scroll=Floats<2>(s,r,R::RenderScroll);v.door_members=Value<std::array<std::uint16_t,8>>(s,r,R::DoorMembers);
  v.door_positions=Value<std::array<std::int32_t,8>>(s,r,R::DoorPositions);return v;
}
ClientLevelValues LevelValues(const ClientRoomLevelStorage &s) {
  const auto &r=s.level;using R=ClientRoomFieldRole;ClientLevelValues v;
  v.stage=Value<std::uint32_t>(s,r,R::Stage);v.stage_type=Value<std::uint32_t>(s,r,R::StageType);
  v.raw_curses=Value<std::uint32_t>(s,r,R::RawCurses);v.current_entry=Value<std::int32_t>(s,r,R::CurrentEntry);
  v.dimension=Value<std::uint8_t>(s,r,R::CurrentDimension);v.greed_wave=Value<std::int32_t>(s,r,R::GreedWave);
  v.current_is_last_boss=Boolean(s,r,R::CurrentIsLastBoss);return v;
}
ClientRoomDescriptorValues DescriptorValues(const ClientRoomLevelStorage &s,std::uint16_t index) {
  const auto &r=s.descriptors[index].record;using R=ClientRoomFieldRole;ClientRoomDescriptorValues v;
  v.grid=Value<std::int32_t>(s,r,R::GridIndex);v.safe_grid=Value<std::int32_t>(s,r,R::SafeGridIndex);
  v.dimension=Value<std::uint8_t>(s,r,R::DescriptorDimension);v.allowed_doors=Value<std::uint8_t>(s,r,R::AllowedDoors);
  v.doors=Value<std::array<std::int32_t,8>>(s,r,R::DoorTargets);v.display_flags=Value<std::uint32_t>(s,r,R::DisplayFlags);
  v.visited=Boolean(s,r,R::Visited);v.display_bits=Value<std::uint8_t>(s,r,R::DisplayBits);
  v.greed_treasure_icon=Boolean(s,r,R::GreedTreasureIcon);const auto *seed=FindSlice(r,R::SpawnSeed);
  v.spawn_seed_present=seed&&seed->field&&seed->size==4;if(v.spawn_seed_present)v.spawn_seed=Exact<std::uint32_t>(Bytes(s,*seed));return v;
}
// 这里只解析已登记L2值；动态帧次序／上限来自同源表，不恢复原ID、数量或保存vector。
ClientSavedMapIcons SavedIcons(const ClientRoomLevelStorage &s,std::uint16_t index) {
  auto b=Bytes(s,Required(s.descriptors[index].record,ClientRoomFieldRole::SavedIcons));ClientSavedMapIcons v;
  if(b.size<1||!b.data||b.data[kSavedIconOffsets[0]]>1)Fail("saved_icon_presence");
  v.present=b.data[kSavedIconOffsets[0]]!=0;
  if(!v.present){if(b.size!=1)Fail("absent_saved_icon_payload");return v;}
  const auto prefix=kSavedIconOffsets[1]+kSavedIconOffsets[2];
  if(prefix>b.size||b.size-prefix<sizeof(v.mask))Fail("saved_icon_mask");
  std::memcpy(&v.mask,b.data+prefix,sizeof(v.mask));auto at=prefix+std::uint32_t(sizeof(v.mask));
  for(const auto &frame:kSavedIconFrames)if(v.mask&(std::uint16_t(1)<<frame.bit)){
    if(at>=b.size||b.data[at]>frame.maximum)Fail("saved_icon_frame");v.frames[frame.bit]=b.data[at++];
  }
  if(at!=b.size)Fail("saved_icon_trailing_bytes");return v;
}
const ClientRoomConfiguration *Configuration(const ClientRoomLevelStorage &s,std::uint16_t resource) {
  if(resource==UINT16_MAX)return nullptr;
  const auto i=std::lower_bound(s.configurations.begin(),s.configurations.end(),resource,
    [](const ClientRoomConfiguration &c,std::uint16_t n){return c.resource<n;});
  if(i==s.configurations.end()||i->resource!=resource)Fail("configuration_dependency_missing");return &*i;
}
// 完整根检查允许未变化块复用旧版本；prefix类型只接受明确登记的Level根，不接受任意片段。
std::uint32_t RecordSize(const ClientWorldShadowRecordView &source,ClientWorldDescriptionKind kind,
                         const ClientRoomLevelContext &context) {
  const auto &id=source.identity;const auto &plan=Plan(kind);const auto *layout=state::FindLayout(id.layout);
  const auto space=kind==ClientWorldDescriptionKind::Room?Space::RoomInstance:
    kind==ClientWorldDescriptionKind::Level?Space::Floor:Space::RoomDescriptor;
  if(!(id.context==context)||!Key(id.key,space,context.floor.run)||!id.use||!id.shape||id.member==UINT16_MAX||
     !layout||id.layout!=plan.layout||layout->block_count>32)Fail("record_identity_or_layout");
  if((kind==ClientWorldDescriptionKind::Room&&!(id.key==context.room))||
     (kind==ClientWorldDescriptionKind::Level&&!(id.key==context.floor)))Fail("root_owner");
  std::uint32_t present=0,total=0;
  for(unsigned i=0;i<source.blocks.size();++i){const auto &b=source.blocks[i];
    if(!b.present){if(b.data||b.size)Fail("absent_block_payload");continue;}
    if(i>=layout->block_count)Fail("undefined_block");const auto &definition=state::Blocks()[layout->first_block+i];
    if(definition.id!=i||b.layout!=id.layout||!(b.key==id.key)||b.use!=id.use||b.shape!=id.shape)Fail("block_identity");
    if(b.size<definition.min_size||b.size>definition.max_size||(b.size&&!b.data)||
       (definition.max_size&&!b.version)||b.version>context.target_version)Fail("block_bounds_or_version");
    if(b.size>(32u<<20)-total)Fail("record_byte_budget");total+=b.size;present|=std::uint32_t(1)<<i;
  }
  if((present&plan.required_blocks)!=plan.required_blocks)Fail("incomplete_shadow_record");
  for(const auto &limit:state::LayoutBudgets())if(limit.layout==id.layout&&total>limit.record_bytes)Fail("layout_byte_budget");
  return total;
}
// 一次性复制根块并按可信固定起点切片；变长内部留在该投影中，由第12/13步递归消费。
OwnedRecord CopyRecord(ClientRoomLevelStorage &s,const ClientWorldShadowRecordView &source,
                        ClientWorldDescriptionKind kind,std::uint32_t &at) {
  OwnedRecord r;r.identity=source.identity;r.plan=&Plan(kind);std::array<std::uint32_t,32> offsets{};
  for(unsigned i=0;i<source.blocks.size();++i){const auto &b=source.blocks[i];offsets[i]=at;
    if(b.present&&b.size){std::memcpy(s.bytes.data()+at,b.data,b.size);at+=b.size;}}
  const auto *layout=state::FindLayout(r.identity.layout);
  for(std::uint16_t i=0;i<layout->field_count;++i){const auto *f=state::FindField(layout->id,i);
    if(!f||i>=r.fields.size()||f->wire_offset==UINT32_MAX||f->block>=layout->block_count)Fail("field_plan");
    const auto &b=source.blocks[f->block];auto end=b.size;
    if(std::uint32_t(i)+1u<layout->field_count){const auto *next=state::FindField(layout->id,std::uint16_t(i+1));if(next->block==f->block)end=next->wire_offset;}
    if(f->wire_offset>end||end>b.size||end-f->wire_offset<f->min_size||end-f->wire_offset>f->max_size)Fail("field_bounds");
    r.fields[i]={f,offsets[f->block]+f->wire_offset,end-f->wire_offset,b.version};
  }
  return r;
}
template<class T> T ConfigurationValue(ClientWorldBytes bytes,ClientRoomFieldRole role) {
  const auto &p=Plan(ClientWorldDescriptionKind::Configuration);const auto i=p.fields[std::size_t(role)];
  const auto *f=state::FindField(p.layout,i);
  if(!f||f->wire_offset==UINT32_MAX||f->min_size!=sizeof(T)||f->max_size!=sizeof(T)||
     f->wire_offset>bytes.size||sizeof(T)>bytes.size-f->wire_offset)Fail("configuration_field_plan");
  return Exact<T>({bytes.data+f->wire_offset,std::uint32_t(sizeof(T))});
}
ClientRoomConfiguration CopyConfiguration(const ClientRoomConfigShadowView &source,const ClientRoomLevelContext &context) {
  const auto &p=Plan(ClientWorldDescriptionKind::Configuration);const auto *layout=state::FindLayout(p.layout);
  if(source.resource==UINT16_MAX||source.resource_epoch!=context.resource_epoch||source.layout!=p.layout||
     source.layout_version!=context.layout_version||!source.data||layout->block_count!=1)Fail("configuration_context");
  const auto &b=state::Blocks()[layout->first_block];if(b.min_size!=b.max_size||source.size!=b.min_size)Fail("configuration_size");
  ClientRoomConfiguration c;c.resource=source.resource;const ClientWorldBytes bytes{source.data,source.size};using R=ClientRoomFieldRole;
  c.stage_id=ConfigurationValue<std::int32_t>(bytes,R::ConfigStageId);c.type=ConfigurationValue<std::int32_t>(bytes,R::ConfigType);
  c.subtype=ConfigurationValue<std::int32_t>(bytes,R::ConfigSubtype);c.width=ConfigurationValue<std::int8_t>(bytes,R::ConfigWidth);
  c.height=ConfigurationValue<std::int8_t>(bytes,R::ConfigHeight);c.shape=ConfigurationValue<std::uint8_t>(bytes,R::ConfigShape);
  if(c.shape<1||c.shape>12||c.width<=0||c.height<=0)Fail("configuration_geometry");return c;
}
std::uint64_t StorageSize(const ClientRoomLevelStorage &s) noexcept {
  return sizeof(s)+64ull+s.bytes.capacity()+s.descriptors.capacity()*sizeof(OwnedDescriptor)+
    s.configurations.capacity()*sizeof(ClientRoomConfiguration);
}
// 结构关系在生成时核定一次，后续查询只查自有索引；不用重复扫描原生描述／保存图。
void BindWorld(ClientRoomLevelStorage &s,std::span<const ClientRoomGridMember> grids) {
  using R=ClientRoomFieldRole;const auto room=RoomValues(s);const auto level=LevelValues(s);
  if(room.dimension<0||room.dimension>2||level.dimension>2||room.dimension!=level.dimension||room.descriptor_slot<0||
     room.descriptor_slot>=std::int32_t(kDescriptorSlots)||!PhysicalSlot(std::uint16_t(room.descriptor_slot)))Fail("current_room_dimension_or_slot");
  if(!room.grid_width||!room.grid_height||room.grid_width>kGridCells||room.grid_height>kGridCells||
     std::uint64_t(room.grid_width)*room.grid_height>kGridCells)Fail("room_grid_extent");
  s.by_slot.fill(UINT16_MAX);s.ordinary.fill(UINT16_MAX);std::array<std::uint64_t,kDescriptorSlots> keys{};
  for(std::uint16_t i=0;i<s.descriptors.size();++i){const auto &d=s.descriptors[i];
    if(!PhysicalSlot(d.source_slot)||s.by_slot[d.source_slot]!=UINT16_MAX)Fail("descriptor_source_slot");s.by_slot[d.source_slot]=i;
    if(d.source_slot<507){if(d.ordinary_ordinal>=507||s.ordinary[d.ordinary_ordinal]!=UINT16_MAX)Fail("ordinary_descriptor_order");
      s.ordinary[d.ordinary_ordinal]=i;++s.ordinary_count;
    }else if(d.ordinary_ordinal!=UINT16_MAX)Fail("off_grid_ordinary_ordinal");
    keys[i]=d.record.identity.key.id;const auto v=DescriptorValues(s,i);
    if(v.dimension>2||v.display_bits>31)Fail("descriptor_display_value");
    (void)SavedIcons(s,i);
    (void)Configuration(s,Value<std::uint16_t>(s,d.record,R::Data));
    (void)Configuration(s,Value<std::uint16_t>(s,d.record,R::OverrideData));
  }
  std::sort(keys.begin(),keys.begin()+s.descriptors.size());
  if(std::adjacent_find(keys.begin(),keys.begin()+s.descriptors.size())!=keys.begin()+s.descriptors.size())Fail("duplicate_descriptor_identity");
  for(std::uint16_t i=0;i<s.ordinary_count;++i)if(s.ordinary[i]==UINT16_MAX)Fail("ordinary_descriptor_gap");
  s.current_descriptor=s.by_slot[std::size_t(room.descriptor_slot)];
  if(s.current_descriptor==UINT16_MAX||DescriptorValues(s,s.current_descriptor).dimension!=level.dimension)Fail("current_descriptor_missing");
  s.occupancy=Value<std::array<std::uint16_t,507>>(s,s.level,R::Occupancy);
  for(std::size_t i=0;i<s.occupancy.size();++i){const auto slot=s.occupancy[i];if(slot==UINT16_MAX)continue;
    if(!PhysicalSlot(slot)||s.by_slot[slot]==UINT16_MAX||DescriptorValues(s,s.by_slot[slot]).dimension!=i/169)Fail("occupancy_dependency");}
  if(level.current_entry>=0){if(level.current_entry>=169||
      s.occupancy[std::size_t(level.dimension)*169+std::size_t(level.current_entry)]!=room.descriptor_slot)Fail("entry_quadrant_mapping");
  }else if(level.current_entry< -20||level.current_entry== -19||506-level.current_entry!=room.descriptor_slot)Fail("special_entry_mapping");
  // presence与整个ANM2负载一致；不把已分配、loaded或本地资源ready当成世界显示资格。
  const auto wave=Boolean(s,s.room,R::WaveSpritePresent);const auto &sprite=Required(s.room,R::WaveSprite);
  if(wave!=(sprite.size!=0))Fail("wave_sprite_presence");
  const auto extent=room.grid_width*room.grid_height;
  std::array<std::uint16_t,kGridCells> indexes{};std::array<std::uint64_t,kGridCells> grid_keys{};
  for(std::size_t i=0;i<grids.size();++i){const auto &g=grids[i];const auto *layout=state::FindLayout(g.layout);
    if(!Key(g.key,Space::Grid,s.context.floor.run)||!(g.room==s.context.room)||!g.use||!g.shape||
       g.member==UINT16_MAX||g.index>=extent||!layout||
       !std::binary_search(kWorldGridLayouts.begin(),kWorldGridLayouts.end(),g.layout))Fail("grid_member_context");
    indexes[i]=g.index;grid_keys[i]=g.key.id;
  }
  std::sort(indexes.begin(),indexes.begin()+grids.size());std::sort(grid_keys.begin(),grid_keys.begin()+grids.size());
  if(std::adjacent_find(indexes.begin(),indexes.begin()+grids.size())!=indexes.begin()+grids.size()||
     std::adjacent_find(grid_keys.begin(),grid_keys.begin()+grids.size())!=grid_keys.begin()+grids.size())Fail("duplicate_grid_identity_or_index");
  for(std::size_t i=0;i<room.door_members.size();++i){const auto member=room.door_members[i];if(member==UINT16_MAX)continue;
    const auto g=std::find_if(grids.begin(),grids.end(),[member](const ClientRoomGridMember &x){return x.member==member;});
    if(g==grids.end()||g->layout!=kWorldDoorLayout||room.door_positions[i]<0||std::uint32_t(room.door_positions[i])>=extent)Fail("door_member_dependency");
    s.doors[i]=*g; // 多个槽允许共享同一成员；不强迫候选位置等于Door主GridIndex。
  }
}
}

ClientRoomProjection::ClientRoomProjection(std::shared_ptr<const ClientRoomLevelStorage> s,ClientWorldDescriptionKind k,
    std::uint16_t i,ClientRoomFieldRole role) noexcept:storage_(std::move(s)),kind_(k),record_(i),role_(role){}
ClientWorldBytes ClientRoomProjection::Bytes() const noexcept {
  if(!storage_)return {};const auto *f=FindSlice(Record(*storage_,kind_,record_),role_);return f?cs::game::Bytes(*storage_,*f):ClientWorldBytes{};
}
std::uint16_t ClientRoomProjection::RootLayout() const noexcept {return storage_?Record(*storage_,kind_,record_).identity.layout:0;}
std::uint16_t ClientRoomProjection::FieldIndex() const noexcept {return storage_?Required(Record(*storage_,kind_,record_),role_).field->index:UINT16_MAX;}
std::uint16_t ClientRoomProjection::NestedLayout() const noexcept {return storage_?Required(Record(*storage_,kind_,record_),role_).field->nested_layout:0;}
std::uint8_t ClientRoomProjection::NestedBlock() const noexcept {return storage_?Required(Record(*storage_,kind_,record_),role_).field->nested_block:UINT8_MAX;}
std::uint32_t ClientRoomProjection::Version() const noexcept {return storage_?Required(Record(*storage_,kind_,record_),role_).version:0;}

ClientRoomDescriptorDescription::ClientRoomDescriptorDescription(std::shared_ptr<const ClientRoomLevelStorage> s,std::uint16_t i) noexcept:storage_(std::move(s)),record_(i){}
const ClientWorldShadowIdentity &ClientRoomDescriptorDescription::Identity() const noexcept {return storage_->descriptors[record_].record.identity;}
std::uint16_t ClientRoomDescriptorDescription::SourceSlot() const noexcept {return storage_->descriptors[record_].source_slot;}
std::optional<std::uint16_t> ClientRoomDescriptorDescription::OrdinaryOrdinal() const noexcept {
  const auto n=storage_->descriptors[record_].ordinary_ordinal;return n==UINT16_MAX?std::nullopt:std::optional<std::uint16_t>(n);
}
ClientRoomDescriptorValues ClientRoomDescriptorDescription::Values() const {return DescriptorValues(*storage_,record_);}
std::optional<ClientRoomConfiguration> ClientRoomDescriptorDescription::Configuration(bool override_data) const {
  const auto resource=Value<std::uint16_t>(*storage_,storage_->descriptors[record_].record,
    override_data?ClientRoomFieldRole::OverrideData:ClientRoomFieldRole::Data);
  const auto *c=cs::game::Configuration(*storage_,resource);return c?std::optional<ClientRoomConfiguration>(*c):std::nullopt;
}
ClientSavedMapIcons ClientRoomDescriptorDescription::SavedIcons() const {return cs::game::SavedIcons(*storage_,record_);}
std::optional<ClientRoomProjection> ClientRoomDescriptorDescription::Field(ClientRoomFieldRole role) const noexcept {
  if(!FindSlice(storage_->descriptors[record_].record,role))return std::nullopt;
  return ClientRoomProjection(storage_,ClientWorldDescriptionKind::Descriptor,record_,role);
}
ClientRoomDescription::ClientRoomDescription(std::shared_ptr<const ClientRoomLevelStorage> s) noexcept:storage_(std::move(s)){}
const ClientWorldShadowIdentity &ClientRoomDescription::Identity() const noexcept {return storage_->room.identity;}
ClientRoomValues ClientRoomDescription::Values() const {return RoomValues(*storage_);}
ClientRoomDescriptorDescription ClientRoomDescription::Descriptor() const noexcept {return ClientRoomDescriptorDescription(storage_,storage_->current_descriptor);}
std::optional<ClientRoomGridMember> ClientRoomDescription::Door(std::uint8_t slot) const {if(slot>=8)Fail("door_slot");return storage_->doors[slot];}
std::optional<ClientRoomProjection> ClientRoomDescription::Field(ClientRoomFieldRole role) const noexcept {
  if(!FindSlice(storage_->room,role))return std::nullopt;return ClientRoomProjection(storage_,ClientWorldDescriptionKind::Room,0,role);
}
ClientLevelDescription::ClientLevelDescription(std::shared_ptr<const ClientRoomLevelStorage> s) noexcept:storage_(std::move(s)){}
const ClientWorldShadowIdentity &ClientLevelDescription::Identity() const noexcept {return storage_->level.identity;}
ClientLevelValues ClientLevelDescription::Values() const {return LevelValues(*storage_);}
std::uint16_t ClientLevelDescription::OrdinaryCount() const noexcept {return storage_->ordinary_count;}
ClientRoomDescriptorDescription ClientLevelDescription::OrdinaryAt(std::uint16_t ordinal) const {
  if(ordinal>=storage_->ordinary_count)Fail("ordinary_ordinal");return ClientRoomDescriptorDescription(storage_,storage_->ordinary[ordinal]);
}
std::optional<ClientRoomDescriptorDescription> ClientLevelDescription::DescriptorBySourceSlot(std::uint16_t slot) const {
  if(!PhysicalSlot(slot))Fail("physical_source_slot");const auto i=storage_->by_slot[slot];
  return i==UINT16_MAX?std::nullopt:std::optional<ClientRoomDescriptorDescription>(ClientRoomDescriptorDescription(storage_,i));
}
std::optional<ClientRoomDescriptorDescription> ClientLevelDescription::DescriptorAt(std::uint8_t dimension,std::uint16_t grid) const {
  if(dimension>2||grid>=169)Fail("level_grid_coordinate");const auto slot=storage_->occupancy[std::size_t(dimension)*169+grid];
  return slot==UINT16_MAX?std::nullopt:DescriptorBySourceSlot(slot);
}
std::optional<ClientRoomProjection> ClientLevelDescription::Field(ClientRoomFieldRole role) const noexcept {
  if(!FindSlice(storage_->level,role))return std::nullopt;return ClientRoomProjection(storage_,ClientWorldDescriptionKind::Level,0,role);
}
ClientRoomLevelDescription::ClientRoomLevelDescription(std::shared_ptr<const ClientRoomLevelStorage> s) noexcept:storage_(std::move(s)){}
const ClientRoomLevelContext &ClientRoomLevelDescription::Context() const noexcept {return storage_->context;}
ClientRoomDescription ClientRoomLevelDescription::Room() const noexcept {return ClientRoomDescription(storage_);}
ClientLevelDescription ClientRoomLevelDescription::Level() const noexcept {return ClientLevelDescription(storage_);}
std::uint16_t ClientRoomLevelDescription::DescriptorCount() const noexcept {return std::uint16_t(storage_->descriptors.size());}
ClientRoomDescriptorDescription ClientRoomLevelDescription::DescriptorAt(std::uint16_t index) const {
  if(index>=storage_->descriptors.size())Fail("descriptor_index");return ClientRoomDescriptorDescription(storage_,index);
}
const void *ClientRoomLevelDescription::StorageOwner() const noexcept {return storage_.get();}
std::uint64_t ClientRoomLevelDescription::StorageBytes() const noexcept {return StorageSize(*storage_);}
long ClientRoomLevelDescription::StorageReferences()const noexcept{return storage_.use_count();}

ClientRoomLevelDescription ClientDescribeRoomLevel(const ClientRoomLevelShadowView &source,std::uint32_t byte_limit,std::shared_ptr<state::ClientStateMemory> peer) {
  const auto &context=source.room.identity.context;
  if(context.layout_version!=state::LayoutVersion()||!context.target_version||!context.member_epoch||
     !context.world_revision||!context.presentation_revision||!Key(context.floor,Space::Floor,context.floor.run)||
     !Key(context.room,Space::RoomInstance,context.floor.run))Fail("world_context");
  if(!byte_limit||byte_limit>(32u<<20)||source.descriptors.empty()||source.descriptors.size()>526||
     source.configurations.size()>2*source.descriptors.size()||source.grids.size()>kGridCells)Fail("input_budget");
  std::uint32_t total=0;auto add=[&](std::uint32_t n){if(n>byte_limit-total)Fail("world_byte_budget");total+=n;};
  add(RecordSize(source.room,ClientWorldDescriptionKind::Room,context));add(RecordSize(source.level,ClientWorldDescriptionKind::Level,context));
  std::array<std::uint16_t,2+526+kGridCells> members{};std::size_t member_count=0;
  members[member_count++]=source.room.identity.member;members[member_count++]=source.level.identity.member;
  for(const auto &d:source.descriptors){add(RecordSize(d.record,ClientWorldDescriptionKind::Descriptor,context));members[member_count++]=d.record.identity.member;}
  const auto payload_bytes=total;
  for(const auto &c:source.configurations)add(c.size);
  for(const auto &g:source.grids)members[member_count++]=g.member;
  std::sort(members.begin(),members.begin()+member_count);
  if(std::adjacent_find(members.begin(),members.begin()+member_count)!=members.begin()+member_count)Fail("duplicate_member_number");
  auto charge=peer?memory::Lease{}:memory::Reserve(memory::Pool::Values,sizeof(ClientRoomLevelStorage)+64ull+payload_bytes+
    source.descriptors.size()*sizeof(OwnedDescriptor)+source.configurations.size()*sizeof(ClientRoomConfiguration),"room_level_shadow_description");
  auto storage=peer?std::allocate_shared<ClientRoomLevelStorage>(state::ClientStateAllocator<ClientRoomLevelStorage>(peer),peer):std::make_shared<ClientRoomLevelStorage>(peer);
  storage->charge=std::move(charge);storage->context=context;
  storage->bytes.resize(payload_bytes);storage->descriptors.resize(source.descriptors.size());storage->configurations.resize(source.configurations.size());
  if(storage->charge)storage->charge->Resize(StorageSize(*storage));std::uint32_t at=0;
  storage->room=CopyRecord(*storage,source.room,ClientWorldDescriptionKind::Room,at);
  storage->level=CopyRecord(*storage,source.level,ClientWorldDescriptionKind::Level,at);
  for(std::size_t i=0;i<source.descriptors.size();++i){const auto &d=source.descriptors[i];
    storage->descriptors[i]={d.source_slot,d.ordinary_ordinal,CopyRecord(*storage,d.record,ClientWorldDescriptionKind::Descriptor,at)};}
  for(std::size_t i=0;i<source.configurations.size();++i)storage->configurations[i]=CopyConfiguration(source.configurations[i],context);
  // 配置已变成具名标量，只计其实际容器；不再复制一份15B原始配置或保留输入指针。
  if(at!=payload_bytes)Fail("owned_payload_size");
  std::sort(storage->configurations.begin(),storage->configurations.end(),[](const auto &a,const auto &b){return a.resource<b.resource;});
  for(std::size_t i=1;i<storage->configurations.size();++i)
    if(storage->configurations[i-1].resource==storage->configurations[i].resource)Fail("duplicate_configuration_resource");
  BindWorld(*storage,source.grids);if(storage->charge)storage->charge->Resize(StorageSize(*storage));return ClientRoomLevelDescription(std::move(storage));
}
} // namespace cs::game
