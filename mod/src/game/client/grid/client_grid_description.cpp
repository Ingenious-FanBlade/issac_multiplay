#include "game/client/grid/client_grid_description.h"
#include "state/nested.h"
#include "core/replication_memory.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace cs::game {
namespace {
constexpr auto kRoles=std::size_t(ClientGridFieldRole::Count);
// 所有字段索引、Desc标量位置、必需块和工厂类映射均由现行正式源生成。
struct GridDescriptionPlan final {
  std::uint16_t layout;
  std::uint32_t required_blocks;
  std::array<std::uint16_t,kRoles> fields;
  std::array<std::uint32_t,4> descriptor_offsets;
};
struct GridFactoryType final { std::int32_t type; std::uint16_t layout; };
#include "state_grid_view.inc"
const GridDescriptionPlan *Plan(std::uint16_t layout) noexcept {
  const auto i=std::lower_bound(kGridDescriptionPlans.begin(),kGridDescriptionPlans.end(),layout,
    [](const GridDescriptionPlan &p,std::uint16_t n){return p.layout<n;});
  return i!=kGridDescriptionPlans.end()&&i->layout==layout?&*i:nullptr;
}
std::uint16_t FactoryLayout(std::int32_t type) noexcept {
  const auto i=std::lower_bound(kGridFactoryTypes.begin(),kGridFactoryTypes.end(),type,
    [](const GridFactoryType &p,std::int32_t n){return p.type<n;});
  return i!=kGridFactoryTypes.end()&&i->type==type?i->layout:0;
}
[[noreturn]] void Fail(const char *reason){throw state::Gap("grid_description",reason);}
template<class T> T Scalar(ClientGridBytes bytes,std::uint32_t offset=0) {
  if(!bytes.data||offset>bytes.size||sizeof(T)>bytes.size-offset)Fail("scalar_bounds");
  T value{};std::memcpy(&value,bytes.data+offset,sizeof(value));return value;
}
// 切片只指向当前描述拥有的字节；不允许来自网络的偏移或原生字段地址。
struct Slice final { const state::Field *field=nullptr;std::uint32_t offset=0,size=0,version=0; };
}
struct ClientGridShadowStorage final {
  ClientGridShadowIdentity identity;
  const GridDescriptionPlan *plan=nullptr;
  std::array<Slice,kRoles> slices{};
  std::vector<std::uint8_t> bytes;
  memory::Lease charge;
};
namespace {
const Slice &SliceFor(const ClientGridShadowStorage &s,ClientGridFieldRole role) {
  const auto n=std::size_t(role);
  if(n>=kRoles||!s.slices[n].field)Fail("field_not_applicable");
  return s.slices[n];
}
ClientGridBytes BytesFor(const ClientGridShadowStorage &s,ClientGridFieldRole role) {
  const auto &slice=SliceFor(s,role);
  return {slice.size?s.bytes.data()+slice.offset:nullptr,slice.size};
}
// 元素数和完整有效前缀必须一致，空列表与不适用保留不同含义；不读取capacity尾部。
std::uint16_t Count(const ClientGridShadowStorage &s,ClientGridFieldRole role,std::uint32_t stride) {
  const auto &slice=SliceFor(s,role);const auto bytes=BytesFor(s,role);
  const auto count=Scalar<std::uint16_t>(bytes);
  if(slice.field->operation!=state::Operation::Container||count>slice.field->limit||
     bytes.size!=2u+std::uint32_t(count)*stride)Fail("container_bounds");
  return count;
}
bool Boolean(const ClientGridShadowStorage &s,ClientGridFieldRole role) {
  const auto value=Scalar<std::uint8_t>(BytesFor(s,role));
  if(value>1)Fail("boolean_value");return value!=0;
}
}
ClientGridProjection::ClientGridProjection(std::shared_ptr<const ClientGridShadowStorage> s,ClientGridFieldRole r) noexcept
  :storage_(std::move(s)),role_(r){}
ClientGridBytes ClientGridProjection::Bytes() const noexcept {
  if(!storage_)return {};const auto &s=storage_->slices[std::size_t(role_)];
  return {s.size?storage_->bytes.data()+s.offset:nullptr,s.size};
}
std::uint16_t ClientGridProjection::RootLayout() const noexcept {return storage_?storage_->identity.layout:0;}
std::uint16_t ClientGridProjection::FieldIndex() const noexcept {return storage_?storage_->slices[std::size_t(role_)].field->index:UINT16_MAX;}
std::uint16_t ClientGridProjection::NestedLayout() const noexcept {return storage_?storage_->slices[std::size_t(role_)].field->nested_layout:0;}
std::uint8_t ClientGridProjection::NestedBlock() const noexcept {return storage_?storage_->slices[std::size_t(role_)].field->nested_block:UINT8_MAX;}
std::uint32_t ClientGridProjection::Version() const noexcept {return storage_?storage_->slices[std::size_t(role_)].version:0;}
bool ClientGridSpriteDescription::HasPayload() const noexcept {
  return std::any_of(projections.begin(),projections.end(),[](const ClientGridProjection &p){return p.Bytes().size!=0;});
}
ClientGridDescription::ClientGridDescription(std::shared_ptr<const ClientGridShadowStorage> s) noexcept:storage_(std::move(s)){}
const ClientGridShadowIdentity &ClientGridDescription::Identity() const noexcept {return storage_->identity;}
ClientGridDescriptor ClientGridDescription::Descriptor() const {
  const auto bytes=BytesFor(*storage_,ClientGridFieldRole::Descriptor);const auto &o=storage_->plan->descriptor_offsets;
  ClientGridDescriptor d;d.type=Scalar<std::int32_t>(bytes,o[0]);d.variant=Scalar<std::int32_t>(bytes,o[1]);d.state=Scalar<std::int32_t>(bytes,o[2]);
  if(o[3]!=UINT32_MAX)d.spawn_seed=Scalar<std::uint32_t>(bytes,o[3]);return d;
}
std::uint32_t ClientGridDescription::Index() const {return Scalar<std::uint32_t>(BytesFor(*storage_,ClientGridFieldRole::GridIndex));}
std::uint32_t ClientGridDescription::WaterClipFlags() const {return Scalar<std::uint32_t>(BytesFor(*storage_,ClientGridFieldRole::WaterClipFlags));}
std::optional<ClientGridProjection> ClientGridDescription::Field(ClientGridFieldRole role) const noexcept {
  if(std::size_t(role)>=kRoles||!storage_->slices[std::size_t(role)].field)return std::nullopt;
  return ClientGridProjection(storage_,role);
}
std::optional<ClientGridSpriteDescription> ClientGridDescription::Sprite(ClientGridSpriteRole role) const noexcept {
  // 三个投影按外观／动画／图层排列；角色是否存在取生成计划，不按variant猜资源有无。
  constexpr ClientGridFieldRole starts[]={ClientGridFieldRole::MainAppearance,ClientGridFieldRole::FireAppearance,
    ClientGridFieldRole::EmberAppearance,ClientGridFieldRole::TimerAppearance,ClientGridFieldRole::ExtraAppearance};
  if(unsigned(role)>=std::size(starts))return std::nullopt;
  ClientGridSpriteDescription sprite;
  for(unsigned i=0;i<3;++i){auto p=Field(ClientGridFieldRole(unsigned(starts[unsigned(role)])+i));
    if(!p)return std::nullopt;sprite.projections[i]=std::move(*p);}
  sprite.layout=sprite.projections[0].NestedLayout();return sprite;
}
std::optional<ClientGridDoorDescription> ClientGridDescription::Door() const {
  if(!Field(ClientGridFieldRole::TargetRoomType))return std::nullopt;
  return ClientGridDoorDescription{Scalar<std::int32_t>(BytesFor(*storage_,ClientGridFieldRole::TargetRoomType)),
    Scalar<std::int32_t>(BytesFor(*storage_,ClientGridFieldRole::TargetRoomIndex)),
    Boolean(*storage_,ClientGridFieldRole::ExtraVisible),Boolean(*storage_,ClientGridFieldRole::DoorLightBit)};
}
std::uint16_t ClientGridDescription::EmberCount() const {return Count(*storage_,ClientGridFieldRole::Embers,16);}
ClientGridEmberDescription ClientGridDescription::EmberAt(std::uint16_t index) const {
  if(index>=EmberCount())Fail("ember_index");const auto b=BytesFor(*storage_,ClientGridFieldRole::Embers);const auto n=2u+16u*index;
  return {Scalar<float>(b,n),Scalar<float>(b,n+4),Scalar<std::int32_t>(b,n+8),Scalar<std::int32_t>(b,n+12)};
}
std::uint16_t ClientGridDescription::RubbleCount() const {return Count(*storage_,ClientGridFieldRole::RubbleOffsets,8);}
ClientPoseVector ClientGridDescription::RubbleAt(std::uint16_t index) const {
  if(index>=RubbleCount())Fail("rubble_index");const auto b=BytesFor(*storage_,ClientGridFieldRole::RubbleOffsets);const auto n=2u+8u*index;
  return {Scalar<float>(b,n),Scalar<float>(b,n+4)};
}
const void *ClientGridDescription::StorageOwner() const noexcept {return storage_.get();}
std::uint64_t ClientGridDescription::StorageBytes() const noexcept {return storage_->charge->Bytes();}

ClientGridDescription ClientDescribeGrid(const ClientGridShadowRecordView &source,std::uint32_t byte_limit) {
  const auto &id=source.identity;const auto *plan=Plan(id.layout);const auto *layout=state::FindLayout(id.layout);
  if(id.layout_version!=state::LayoutVersion())Fail("layout_version");
  if(!plan||!layout||layout->base_prefix||layout->block_count>32)Fail("grid_layout");
  if(!id.key.run||!id.key.id||id.key.space!=2||!id.room.id||id.room.space!=7||id.key.run!=id.room.run||
     !id.use||!id.shape||id.member==UINT16_MAX||!id.target_version)Fail("identity");
  if(!byte_limit||byte_limit>(32u<<20))Fail("byte_budget");
  std::uint32_t total=0,present=0;std::array<std::uint32_t,32> offsets{};
  for(unsigned i=0;i<32;++i){const auto &input=source.blocks[i];offsets[i]=total;
    if(!input.present){if(input.size||input.data)Fail("absent_block_payload");continue;}
    if(i>=layout->block_count)Fail("undefined_block");
    const auto &block=state::Blocks()[layout->first_block+i];
    if(block.id!=i||input.layout!=id.layout||!(input.key==id.key)||input.use!=id.use||input.shape!=id.shape)Fail("block_identity");
    if(input.size<block.min_size||input.size>block.max_size||(input.size&&!input.data)||
       (block.max_size&&!input.version)||input.version>id.target_version)Fail("block_bounds_or_version");
    if(input.size>byte_limit-total)Fail("record_budget");total+=input.size;present|=std::uint32_t(1)<<i;
  }
  if((present&plan->required_blocks)!=plan->required_blocks)Fail("incomplete_shadow_record");
  // 描述一次性取得全部块的自有字节，之后只共享const owner，不保留输入指针或逐字段树。
  auto charge=memory::Reserve(memory::Pool::Values,sizeof(ClientGridShadowStorage)+32ull+total,"grid_shadow_description");
  auto storage=std::make_shared<ClientGridShadowStorage>();storage->charge=std::move(charge);storage->identity=id;storage->plan=plan;
  storage->bytes.resize(total);storage->charge->Resize(sizeof(ClientGridShadowStorage)+32ull+storage->bytes.capacity());
  for(unsigned i=0;i<32;++i)if(source.blocks[i].present&&source.blocks[i].size)
    std::memcpy(storage->bytes.data()+offsets[i],source.blocks[i].data,source.blocks[i].size);
  for(std::size_t r=0;r<kRoles;++r){const auto index=plan->fields[r];if(index==UINT16_MAX)continue;
    const auto *field=state::FindField(id.layout,index);if(!field||field->block>=layout->block_count||field->wire_offset==UINT32_MAX)Fail("field_plan");
    const auto &input=source.blocks[field->block];const auto start=field->wire_offset;
    auto end=input.size;
    for(std::uint16_t next=index+1;next<layout->field_count;++next){const auto *f=state::FindField(id.layout,next);
      if(f->block==field->block){end=f->wire_offset;break;}}
    if(start>end||end>input.size||end-start<field->min_size||end-start>field->max_size)Fail("field_bounds");
    storage->slices[r]={field,offsets[field->block]+start,end-start,input.version};
  }
  ClientGridDescription result(storage);const auto desc=result.Descriptor();
  if(FactoryLayout(desc.type)!=id.layout||result.Index()>447)Fail("factory_type_or_index");
  (void)result.Door(); // 保留负目标索引，只核对两个真正的布尔投影。
  if(result.Field(ClientGridFieldRole::Embers)){
    const auto count=result.EmberCount();if(desc.state>=4&&count)Fail("ember_state");
    for(std::uint16_t i=0;i<count;++i){const auto e=result.EmberAt(i);
      if(!std::isfinite(e.x)||!std::isfinite(e.y)||e.age<0||e.age>=e.max_life||e.max_life>29)Fail("ember_value");}
  }
  if(result.Field(ClientGridFieldRole::RubbleOffsets)){
    const auto count=result.RubbleCount();const auto animation=Scalar<std::uint16_t>(BytesFor(*storage,ClientGridFieldRole::RubbleAnimation));
    if(desc.state!=2&&(count||animation!=UINT16_MAX))Fail("rubble_state");
    if(count&&animation==UINT16_MAX)Fail("rubble_animation");
    for(std::uint32_t i=0;i<count;++i){const auto v=result.RubbleAt(std::uint16_t(i));
      if(!std::isfinite(v.x)||!std::isfinite(v.y))Fail("rubble_value");}
  }
  return result;
}
} // namespace cs::game
