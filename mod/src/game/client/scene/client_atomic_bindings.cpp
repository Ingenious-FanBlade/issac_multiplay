#include "game/client/scene/client_atomic_bindings.h"
#include "game/client/scene/client_atomic_bindings_native.h"
#include "game/client/display/client_display_memory.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "game/common/binary_effect_e2e.h"
#include "state/client_memory_resource.h"
#include <algorithm>
#include <cstring>
#include "game/client/scene/client_exact_index.h"
#include <tuple>

namespace cs::game {
namespace {
namespace S=state;
#include "state_host_fields.inc"
namespace F=host_field;
using D=S::ClientInstallDomain;
using K=ClientBinarySpecial;
constexpr auto none=S::kClientPlanNone;
S::ClientStateNativeStep Result(bool ok,bool written,const char *why) noexcept {return {ok,written,!ok,why};}
template<class T>T Read(std::span<const std::byte> bytes) {
  if(bytes.size()!=sizeof(T))throw S::ClientShadowError(S::StateReadiness::InvalidEncoding,"binary_binding_value_width");
  T value;std::memcpy(&value,bytes.data(),sizeof(value));return value;
}
bool Layer(std::string_view h) noexcept {
  return h=="read_anm2_layer_states"||h=="read_grid_rock_layers"||h=="read_room_wave_sprite_layers";
}
bool Plus(std::string_view h) noexcept {return h.starts_with("read_existing_")||h=="player_existing_spoofs_members";}
// 每个特殊字段明确选择原生ABI，或由已定界的结构／显示描述消费。
K ClassifyKind(const S::Field &f) noexcept {
  const std::string_view h=f.handler;
  if(S::FindLayout(f.layout)->protocol)return K::Metadata;
  // 普通Copy／Nested字段没有具名handler。保留两项非handler例外后直接分派，
  // 避免每个标量在覆盖、排序和封存时重复走整张特殊函数名称表。
  if(h.empty()){
    if(std::string_view(f.target)=="item-config"&&f.source==S::FieldSource::Object)return K::Config;
    if((f.layout==276&&f.index==2)||(f.source==S::FieldSource::Synthetic&&!f.native_size))return K::Metadata;
    return K::None;
  }
  if(Plus(h))return K::Companion;
  if(Layer(h))return K::LayerCount;
  if(h=="read_layer_frame_selectors")return K::LayerFrames;
  if(h=="read_null_layer_frame_selectors")return K::NullFrames;
  if(h=="anm2_selected_file_to_resource")return K::File;
  if(h=="anm2_animation_to_index")return K::Animation;
  if(h=="room_wave_sprite_animation_read")return K::WaveAnimation;
  if(h=="layer_spritesheet_path_to_resource")return K::LayerPath;
  if(h=="image_base_name_to_resource")return K::LayerImage;
  if(h=="f_x_image_read"||h=="ray_image_read")return K::Image;
  if(h=="entity_selected_config_to_resource"||(std::string_view(f.target)=="item-config"&&f.source==S::FieldSource::Object))return K::Config;
  if(h=="room_descriptor_data_read"||h=="room_descriptor_override_data_read")return K::DescriptorConfig;
  if(h=="familiar_player_to_member"||h=="player__backupPlayer_to_member")return K::BorrowedPlayer;
  if(h=="player__heldEntity_to_member")return K::BorrowedEntity;
  if(h=="grid_rock_rubble_to_resource")return K::Rubble;
  if(h=="read_text_bytes")return K::Text;
  if(h=="read_anm2_default_name")return K::DefaultName;
  if(h=="room_descriptor_dimension_read"||h=="room_descriptor_visited_count_read"||h=="room_descriptor_allowed_doors_read")return K::Widen;
  if(h=="room_descriptor_flags_read")return K::DescriptorFlags;
  if(h=="read_grid_door_light_bit")return K::DoorLight;
  if(h=="read_grid_door_draw_enabled")return K::DoorAlpha;
  if(h=="f_x_color_condition_read")return K::FXCondition;
  if(h=="f_x_mask_backdrop_read")return K::FXBackdrop;
  if(h=="vein_children_read")return K::VeinChildren;
  if(h=="run_daily_display_read")return K::Daily;
  if(h=="run_ascent_display_read")return K::Ascent;
  if(h=="run_time_counter_anchor_read")return K::Time;
  if(h=="f_x_poses_read")return K::FXCount;
  if(h=="room_descriptor_read"||h=="room_door_slots_read"||h=="level_game_prefix_room_offset_read"||
     h=="level_game_prefix_current_dimension_read"||(f.layout==276&&f.index==2)||
     h=="entity_index_to_draw_order"||h=="npc_controller_to_ui_mode"||h=="incubus_weapon_to_charge_bar_amount"||
     h=="display_value_read"||h=="room_descriptor_map_icons_read"||h=="final_saved_map_icons_payload_read"||
     (f.source==S::FieldSource::Synthetic&&!f.native_size))return K::Metadata;
  return K::None;
}
// 特殊分类及晚绑定资格只依赖不可变生成定义；不含原生地址或窗口有效性。
struct DefinitionFacts {K kind;bool late;};
DefinitionFacts ClassifyFacts(const S::Field &f) noexcept {
  return {ClassifyKind(f),Layer(f.handler)||Plus(f.handler)||
    f.nested_layout==F::OverlayDisplaySet::kLayout||f.nested_layout==F::OverlayDisplayState::kLayout};
}
// 不可变生成表只冷绑定一次；可信计划的整批字段直接借用同一张分类表。
const std::vector<DefinitionFacts> &DefinitionIndex() {
  struct Index {
    memory::Lease charge;
    std::vector<DefinitionFacts> facts;
    Index():charge(memory::Reserve(memory::Pool::Values,128+S::Fields().size()*sizeof(DefinitionFacts),"client_special_kind_index")),facts(S::Fields().size()) {
      for(std::size_t i=0;i<facts.size();++i)facts[i]=ClassifyFacts(S::Fields()[i]);
      charge->Resize(sizeof(Index)+128+facts.capacity()*sizeof(DefinitionFacts));
    }
  };
  static const Index index;return index.facts;
}
// 外部或复制字段仍核地址范围与元素边界；不把任意引用当作生成表下标。
DefinitionFacts Facts(const S::Field &f) noexcept {
  try {
    const auto definitions=S::Fields();
    const auto first=reinterpret_cast<std::uintptr_t>(definitions.data());
    const auto address=reinterpret_cast<std::uintptr_t>(&f);
    if(address<first||address-first>=definitions.size_bytes()||(address-first)%sizeof(S::Field))return ClassifyFacts(f);
    return DefinitionIndex()[(address-first)/sizeof(S::Field)];
  }catch(...){return ClassifyFacts(f);}
}
K Kind(const S::Field &f) noexcept {return Facts(f).kind;}
unsigned Category(const S::ClientInstallEntity &e,std::uint16_t layout) noexcept {
  if(layout==147)return 12;
  switch(e.type){case 1:return 0;case 2:return 2;case 3:return 8;case 4:return 4;case 5:return 7;case 6:return 10;
    case 7:return 5;case 8:return 6;case 9:return 3;case 1000:return 9;case 9001:return 11;default:return 1;}
}
using FieldKey=std::tuple<std::uint32_t,std::uint32_t,std::uint16_t,std::uint16_t>;
template<class T>using Vector=S::ClientStateVector<T>;
// 一个实例保管一个事务；同一准备owner只有一份已提交world成员，body更新借用它。
class NativeBindings final:public ClientAtomicInstallBindings {
  struct Special {
    ClientBinarySpecialWrite write;
    ClientBinarySpecialUndo undo;
    std::uint32_t first=0,size=0;
    std::uint32_t target=none;
    std::size_t charge=0;
  };
  ClientSceneMainPreparation &preparation_;
  // 每个安装器只借用一次不可变分类表，热循环按已验证的定义下标访问。
  // 冷分配失败仍走原分类；不缓存原生地址、对象资格或窗口。
  const std::vector<DefinitionFacts> *definition_facts_=nullptr;
  K DefinitionKind(std::uint32_t index)const noexcept {
    return definition_facts_?(*definition_facts_)[index].kind:Kind(S::Fields()[index]);
  }
  std::shared_ptr<S::ClientStateMemory> memory_;
  ClientNativeRoots roots_{};
  void *vm_=nullptr;
  const S::ClientStateApplyBatch *batch_=nullptr;
  const ClientSceneInstallPlan *published_scene_=nullptr;
  std::uint64_t candidate_window_=0;
  Vector<ClientBinaryInit> init_;
  Vector<ClientBinaryEntityMember> members_,previous_members_;
  Vector<void *> previous_addresses_;
  // 普通 World 的目标 render 身份集合，按本地地址排序。真实列表的 Z 次序由显示推进维护。
  Vector<void *> render_members_;
  Vector<std::pair<std::uintptr_t,std::uint32_t>> draw_order_;
  Vector<Special> special_;
  Vector<std::uint32_t> special_index_;
  Vector<std::byte> values_;
  Vector<std::uint32_t> execution_;
  Vector<std::uint32_t> fx_counts_,companion_fields_,layer_images_;
  ClientBinaryRoomMembers room_;
  // HUD 的四个参与者槽由同批控制组决定；Player.ControllerIndex 始终保持客机 -1。
  std::array<std::uint8_t,256> roster_ordinals_{};
  ClientBinaryRoomPrevious previous_room_,initial_room_;
  NativeBindings *prior_=nullptr;
  static thread_local NativeBindings *world_,*candidate_;
  bool described_=false,world_target_=false,geometry_=false,members_entered_=false,members_done_=false,
       committed_=false,invalid_=false,uncertain_=false,restore_baseline_=false,frame_installed_=false,restore_procedural_=false;
  bool same_members_=false,borrowed_procedural_=false,companions_checked_=false;
  std::uint32_t previous_frame_=0,initial_frame_=0,initial_time_=0;
  std::size_t retirement_=0;
  const char *reason_="binary_native_bindings_preflight";
  struct Overlay final {
    void *owner=nullptr,*player=nullptr,*previous_player=nullptr;
    std::uint64_t generation=0;
    int state=0,id=0,delay=0,controller=-1;
    bool entered=false;
  };
  std::array<Overlay,65> overlays_{};
  unsigned overlay_count_=0;
  std::size_t overlay_charge_=0;
  Vector<ClientBinaryProceduralDefinition> procedural_definitions_;
  std::array<void *,2> procedural_atlases_{};
  ClientBinaryProcedural *procedural_=nullptr;

  static void *Object(const ClientAtomicInputs &in,std::uint32_t n) noexcept {return n<in.objects.size()?in.objects[n].address:nullptr;}
  static void *Resource(const ClientAtomicInputs &in,S::ResourceKey key) noexcept {
    const auto found=std::lower_bound(in.resources.begin(),in.resources.end(),key,[](const auto &entry,const auto &value){return entry.key<value;});
    return found!=in.resources.end()&&found->key==key?found->address:nullptr;
  }
  const char *DescribeMembers(const ClientAtomicInputs &in);
  bool DescribeFields(const ClientAtomicInputs &in);
  bool CanBorrowProcedural() const noexcept;
  bool ShowOverlay(const Overlay &) const noexcept;
  bool Undo(Special &) noexcept;
  static const NativeBindings *Visible() noexcept {
    if(candidate_){
      if(candidate_->candidate_window_==OwnerToken(Owner::Manager))return candidate_;
      return nullptr;
    }
    return world_;
  }
  bool Displayable()const noexcept {return !invalid_&&!batch_->cancelled.load(std::memory_order_acquire);}
public:
  NativeBindings(const std::shared_ptr<S::ClientStateMemory> &m,ClientSceneMainPreparation &p):
    preparation_(p),memory_(m),init_(S::ClientStateAllocator<ClientBinaryInit>(m)),
    members_(S::ClientStateAllocator<ClientBinaryEntityMember>(m)),previous_members_(S::ClientStateAllocator<ClientBinaryEntityMember>(m)),
    previous_addresses_(S::ClientStateAllocator<void *>(m)),render_members_(S::ClientStateAllocator<void *>(m)),
    draw_order_(S::ClientStateAllocator<std::pair<std::uintptr_t,std::uint32_t>>(m)),
    special_(S::ClientStateAllocator<Special>(m)),special_index_(S::ClientStateAllocator<std::uint32_t>(m)),
    values_(S::ClientStateAllocator<std::byte>(m)),execution_(S::ClientStateAllocator<std::uint32_t>(m)),
    fx_counts_(S::ClientStateAllocator<std::uint32_t>(m)),companion_fields_(S::ClientStateAllocator<std::uint32_t>(m)),
    layer_images_(S::ClientStateAllocator<std::uint32_t>(m)),
    procedural_definitions_(S::ClientStateAllocator<ClientBinaryProceduralDefinition>(m)){
      try {definition_facts_=&DefinitionIndex();}catch(...){/* 冷表不可用时保留完整分类。 */}
    }
  bool Current(const S::ClientStateApplyBatch &b)const noexcept override {
    return !invalid_&&!uncertain_&&(!batch_||batch_==&b);
  }
  S::ClientStateNativeStep Preflight(const S::ClientStateApplyBatch &b,const ClientAtomicInputs &in) noexcept override;
  bool WorldMembersRetained() const noexcept override;
  bool LatePath(const S::Field &f)const noexcept override {
    return Facts(f).late;
  }
  ClientAtomicFieldUse DescribeField(const ClientAtomicInputs &in,std::uint32_t n)const noexcept override {
    const auto &f=in.scene.State().Field(n);
    const auto kind=DefinitionKind(f.definition);
    const auto layout=S::Fields()[f.definition].layout;
    const bool covered=kind!=K::None&&kind!=K::FXCount;
    return {std::uint8_t(f.present&&kind!=K::Metadata?kind:K::None),covered,
      covered&&f.present&&kind!=K::Metadata&&kind!=K::Companion,
      f.present&&layout>=F::OverlayDisplayState::kLayout&&layout<=F::ProceduralDisplaySet::kLayout};
  }
  // 静止记录可省的具名类别：Sprite 文件／动画／图层／图像／配置／文本及门、FX 显示位。
  // 这些值由原生对象自己持有，已提交批次退休时只释放保存的旧副本，不回写活对象。
  // 引用（BorrowedEntity／BorrowedPlayer）须留给退休拆边；FXCount 由成员安装逐批写入；
  // Time／Daily／Ascent 随帧换算，房间描述类不在静止放行域，一律保留。
  bool QuietSkippable(const ClientAtomicFieldUse &use)const noexcept override {
    if(use.display)return false;
    switch(K(use.kind)){
      case K::None:case K::Metadata:case K::Companion:case K::File:case K::Animation:case K::WaveAnimation:
      case K::LayerCount:case K::LayerFrames:case K::NullFrames:case K::LayerPath:case K::Image:case K::LayerImage:
      case K::Config:case K::Rubble:case K::Text:case K::DefaultName:case K::DoorLight:case K::DoorAlpha:
      case K::FXCondition:case K::FXBackdrop:case K::VeinChildren:
        return true;
      default:return false;
    }
  }
  bool FrameField(const ClientAtomicFieldUse &use)const noexcept override {return K(use.kind)==K::Time;}
  // 快速帧：本绑定必须仍是已提交的当前 World 发布者。帧号直接写入（已提交批次不再撤销帧号），
  // 渲染钩子随后读取同形的新计划。
  bool AdvancePublished(const ClientSceneInstallPlan &scene,std::uint32_t frame) noexcept override {
    if(!committed_||!world_target_||world_!=this||invalid_||uncertain_||!published_scene_)return false;
    std::uint32_t previous=0;
    if(!ClientBinaryInstallFrame(roots_,frame,previous))return false;
    published_scene_=&scene;return true;
  }
  unsigned ValueOrder(const S::Field &f)const noexcept override {
    // Room／Level的标量前缀由原生实体Init读取。完整姿态和动态容器保持后置。
    if((f.layout==276||f.layout==270)&&f.operation==S::Operation::Copy)return 0;
    if(f.layout==F::RoomDescriptor::kLayout&&f.index==F::RoomDescriptor::spawn_seed)return 0;
    if(f.layout==F::Room::kLayout&&(f.index==F::Room::bounds||f.index==F::Room::door_grid_positions))return 0;
    // 未被具名处理器接管的字段只可能为None或FXCount，两者原Order均为4。
    // 不改变原生执行优先级，也不重新查询普通叶不需要的特殊分类表。
    return 4;
  }
  unsigned Order(const S::Field &f)const noexcept override {
    if(ValueOrder(f)==0)return 0;
    switch(Kind(f)){case K::DescriptorConfig:return 0;case K::File:return 1;
      case K::Widen:case K::DescriptorFlags:return f.layout==270?0:4;
      case K::Animation:case K::WaveAnimation:return 2;
      case K::LayerCount:case K::Companion:return 3;
      case K::DoorAlpha:return 6;
      case K::BorrowedEntity:case K::BorrowedPlayer:return 7;
      default:return 4;}
  }
  bool Resolve(const ClientAtomicInputs &,std::uint32_t,std::uint32_t,void *,void *&,std::size_t &) noexcept override;
  S::ClientStateNativeStep Initialize(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  S::ClientStateNativeStep PrepareFieldOwners(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  S::ClientStateNativeStep InstallField(const S::ClientStateApplyBatch &,const ClientAtomicInputs &,
      std::uint32_t) noexcept override;
  S::ClientStateNativeStep FinishFields(const ClientAtomicInputs &,std::uint8_t streams) noexcept override;
  ClientAtomicReadback ReadField(const ClientAtomicInputs &,std::uint32_t) noexcept override;
  S::ClientStateNativeStep InstallMembers(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  S::ClientStateNativeStep Publish(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  void Committed(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  void Invalidate(const S::ClientStateApplyBatch &,const char *) noexcept override {
    invalid_=true;if(candidate_==this)candidate_=nullptr;
  }
  bool Quiesce(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  bool ForgetMembers(std::span<const ClientLiveRetirement>) noexcept override;
  bool UnbindMemberOverlays(std::span<const ClientLiveRetirement>) noexcept override;
  bool ClearReferenceTo(const ClientAtomicInputs &,std::uint32_t,std::span<const ClientLiveRetirement>) noexcept override;
  bool UndoField(const S::ClientStateApplyBatch &,const ClientAtomicInputs &,std::uint32_t) noexcept override;
  bool RetirementCurrent(const S::ClientStateApplyBatch &b,const ClientAtomicInputs &)const noexcept override {
    return batch_==&b&&!uncertain_&&ClientAtomicInstallWindow(roots_,vm_);
  }
  bool Retire(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  static bool GridWaterClip(const void *,std::uint32_t &) noexcept;
  static bool DrawOrder(const void *,std::uint32_t &) noexcept;
  static bool HudOrdinal(const void *,unsigned &) noexcept;
  static void RenderOverlays(const void *) noexcept;
  static const ClientSceneInstallPlan *PublishedWorld() noexcept {
    return world_&&world_->Displayable()?world_->published_scene_:nullptr;
  }
};
thread_local NativeBindings *NativeBindings::world_=nullptr;
thread_local NativeBindings *NativeBindings::candidate_=nullptr;

// 只读预检返回首个失败条件，供同一票据记录，避免把预检失败误记成写入失败。
const char *NativeBindings::DescribeMembers(const ClientAtomicInputs &in) {
  const auto &plan=in.scene.State();world_target_=in.scene.Rooms().has_value();prior_=world_;
  if(prior_&&(prior_->roots_.game!=roots_.game||prior_->roots_.room!=roots_.room||&prior_->preparation_!=&preparation_))return "binary_native_member_prior_roots";
  if(!world_target_)return prior_&&prior_->Displayable()?nullptr:"binary_native_member_body_world";
  const auto &world=plan.World();const auto level=in.scene.Rooms()->Level();
  if(!in.scene.Rooms()->Room().Descriptor().Values().spawn_seed)return "binary_native_member_spawn_seed";
  room_.descriptor=Object(in,world.current_descriptor);room_.width=world.grid_width;room_.height=world.grid_height;
  room_.type=in.scene.Rooms()->Room().Values().type;
  room_.dimension=world.dimension;room_.entry=world.current_entry;room_.descriptor_count=level.OrdinaryCount();
  const auto source_slot=in.scene.Rooms()->Room().Descriptor().SourceSlot();
  room_.last_boss=level.Values().current_is_last_boss?int(source_slot):-1;
  for(const auto &descriptor:plan.Descriptors()){
    const auto d=level.DescriptorBySourceSlot(descriptor.source_slot);
    if(!d||!Object(in,descriptor.object))return "binary_native_member_room_descriptor";
    room_.descriptors[descriptor.source_slot]=true;
    if(d->Values().greed_treasure_icon){
      if(room_.greed_treasure!=-1)return "binary_native_member_greed_treasure";room_.greed_treasure=d->Values().grid;
    }
  }
  for(std::size_t i=0;i<room_.occupancy.size();++i)room_.occupancy[i]=world.occupancy[i]==S::kNullIndex?UINT32_MAX:world.occupancy[i];
  for(const auto &grid:plan.Grids()){
    room_.grids[grid.index]=Object(in,grid.object);
    if(!room_.grids[grid.index]&&in.objects[grid.object].failure==ClientMemberFailure::None)
      return "binary_native_member_grid_object";
  }
  for(const auto &grid:in.scene.Grids()){
    if(!room_.grids[grid.Index()])continue;
    room_.water_clip[grid.Index()]=grid.WaterClipFlags();
  }
  for(std::size_t i=0;i<room_.doors.size();++i)if(world.doors[i]!=none){
    room_.doors[i]=Object(in,world.doors[i]);
    if(!room_.doors[i]&&in.objects[world.doors[i]].failure==ClientMemberFailure::None)
      return "binary_native_member_door_object";
  }
  decltype(room_.roster) roster{};decltype(roster_ordinals_) ordinals{};
  for(const auto &p:plan.Players())if(p.roster!=S::kNullIndex&&Object(in,p.object)){
    const auto group=std::find_if(plan.Groups().begin(),plan.Groups().end(),[&](const auto &g){return g.object==p.group;});
    if(group==plan.Groups().end())return "binary_native_member_roster_group";
    if(p.roster>=roster.size())return "binary_native_member_roster_slot";
    roster[p.roster]=Object(in,p.object);ordinals[p.roster]=group->ordinal;
  }
  for(std::size_t n=0;n<roster.size();++n)if(roster[n]){
    room_.roster[room_.roster_count]=roster[n];roster_ordinals_[room_.roster_count++]=ordinals[n];
  }

  // 没有主玩家的场景不能进入原生 Render，额外生命与相机查询都会解引用 roster。
  if(!room_.roster_count)return "binary_native_no_player_roster";
  init_.resize(plan.Objects().size());
  Vector<std::pair<unsigned,std::uint32_t>> ordered{S::ClientStateAllocator<std::pair<unsigned,std::uint32_t>>(memory_)};
  const auto shadow=plan.Effective(S::BodyStream::World),body=plan.Effective(S::BodyStream::Body);
  const auto &objects=plan.Objects();
  ClientNativeReadWindow member_reads;
  for(const auto &entity:plan.Entities()){
    const auto n=entity.object;const auto &obj=objects[n];auto &init=init_[n];
    init.object=Object(in,n);
    if(!init.object&&in.objects[n].failure!=ClientMemberFailure::None)continue;
    init.type=entity.type;init.variant=entity.variant;init.subtype=entity.subtype;
    init.seed=entity.init_seed;init.category=Category(entity,obj.member.identity.layout);
    init.fresh=!in.objects[n].borrowed;init.config=Resource(in,entity.configuration);
    if(entity.configuration.index!=S::kNullIndex&&!init.config)return "binary_native_member_entity_config";
    if(const auto *why=ClientBinaryInitFailure(init,&member_reads)){
      if(in.reject&&in.reject(in.locator,n,ClientMemberFailure::Native,why))continue;
      return why;
    }
    const auto r=shadow->Find(obj.member.identity);if(!r)return "binary_native_member_entity_record";
    auto &m=members_.emplace_back();m.object=init.object;m.type=init.type;m.variant=init.variant;m.subtype=init.subtype;
    const auto *flags=r->Field(15);if(!flags||!flags->present)return "binary_native_member_entity_flags";
    m.add_update=m.update=(Read<std::uint64_t>(flags->bytes)&8)==0;
    if(obj.domain==D::Player){
      const auto p=std::find_if(plan.Players().begin(),plan.Players().end(),[n](const auto &v){return v.object==n;});if(p==plan.Players().end())return "binary_native_member_player_record";
      init.player_type=p->player_type;m.valid=p->membership&1;m.room=p->membership&2;m.render=p->membership&4;m.update=p->membership&8;
    }
    ordered.emplace_back(entity.order,std::uint32_t(members_.size()-1));
    draw_order_.emplace_back(reinterpret_cast<std::uintptr_t>(init.object),entity.order);
    if(init.fresh){
      const auto pose=obj.domain==D::Player?(body?body->Find(obj.member.identity):std::nullopt):r;
      // 普通 World 可以先于新身体的 Body 抵达。初始姿态用中性值，首份 Body 补写。
      if(pose){
        const auto *position=pose->Field(10),*velocity=pose->Field(12);
        if(!position||!velocity||!position->present||!velocity->present)return "binary_native_member_entity_motion";
        init.position=Read<std::array<float,2>>(position->bytes);init.velocity=Read<std::array<float,2>>(velocity->bytes);
      }
      const auto *spawner_type=r->Field(4),*spawner_variant=r->Field(5);
      if(!spawner_type||!spawner_variant||!spawner_type->present||!spawner_variant->present)return "binary_native_member_entity_spawner";
      init.spawner_type=Read<std::uint32_t>(spawner_type->bytes);init.spawner_variant=Read<std::uint32_t>(spawner_variant->bytes);
    }
  }
  std::sort(ordered.begin(),ordered.end());
  std::sort(draw_order_.begin(),draw_order_.end());
  if(std::adjacent_find(draw_order_.begin(),draw_order_.end(),[](const auto &a,const auto &b){return a.first==b.first;})!=draw_order_.end())return "binary_native_member_duplicate_object";
  Vector<ClientBinaryEntityMember> sorted{S::ClientStateAllocator<ClientBinaryEntityMember>(memory_)};
  sorted.reserve(ordered.size());
  for(const auto &[order,index]:ordered)sorted.push_back(members_[index]);
  members_.swap(sorted);
  for(std::uint32_t i=0;i<plan.BindingCount();++i){
    const auto link=plan.Binding(i);
    if(link.is_resource||link.is_null||link.object>=in.objects.size()||!in.objects[link.object].borrowed)continue;
    const auto &value=plan.Value(link.value);const auto &record=plan.Records()[value.record];
    if(record.object>=init_.size()||!init_[record.object].fresh)continue;
    const auto &path=plan.Paths()[value.path];if(path.parent!=none)continue;
    if(path.step.field==36)init_[record.object].parent=Object(in,link.object);
    if(path.step.field==39)init_[record.object].spawner=Object(in,link.object);
  }
  if(prior_){
    previous_members_=prior_->members_;
    previous_addresses_.reserve(previous_members_.size());
    for(const auto &member:previous_members_)previous_addresses_.push_back(member.object);
    std::sort(previous_addresses_.begin(),previous_addresses_.end(),std::less<const void *>{});
    const auto &order=prior_->draw_order_;
    for(auto &member:members_){
      const auto key=reinterpret_cast<std::uintptr_t>(member.object);
      const auto at=std::lower_bound(order.begin(),order.end(),key,[](const auto &e,auto k){return e.first<k;});
      if(at!=order.end()&&at->first==key)member.index=ClientBinaryLocalIndex(member.object);
    }
    // 列表次序和实际索引都是结构。只在完整成员元组及共享容器内容相同后复用。
    const auto key=[](const auto &x){return std::tie(x.object,x.type,x.variant,x.subtype,x.index,
      x.valid,x.room,x.render,x.update,x.add_update);};
    same_members_=draw_order_==prior_->draw_order_&&room_.roster_count==prior_->room_.roster_count&&
      room_.roster==prior_->room_.roster&&room_.grids==prior_->room_.grids&&room_.doors==prior_->room_.doors&&
      std::equal(members_.begin(),members_.end(),prior_->members_.begin(),prior_->members_.end(),
        [&](const auto &a,const auto &b){return key(a)==key(b);});
    // 转场和暂停显示分支不执行普通 Sort，沿用原有原生列表次序检查。
    if(same_members_&&S::StateTargetStreams(plan.Target())==1&&plan.Target().sections[0].base_version&&
        in.scene.Display()&&in.scene.Display()->Metadata().branch==ClientDisplayBranch::Normal){
      render_members_.reserve(members_.size());
      unsigned last_order=0;
      for(std::size_t n=0;n<members_.size();++n){
        const auto &member=members_[n];
        if(!member.room||!member.render)continue;
        const auto order=ordered[n].first;
        // 同 Z 且同 Host 顺序键可能保留输入次序，遇到相同键时沿用原检查和登记。
        if(!render_members_.empty()&&order==last_order){render_members_.clear();break;}
        render_members_.push_back(member.object);last_order=order;
      }
      std::sort(render_members_.begin(),render_members_.end(),std::less<const void *>{});
    }
  }
  if(!ClientBinaryCaptureRoom(roots_,previous_room_))return "binary_native_member_room_snapshot";
  if(prior_){previous_room_.members.roster=prior_->room_.roster;previous_room_.members.roster_count=prior_->room_.roster_count;
    previous_room_.members.grids=prior_->room_.grids;previous_room_.members.doors=prior_->room_.doors;}
  initial_room_=prior_?prior_->initial_room_:previous_room_;
  const auto *clock=static_cast<const std::byte *>(ClientBinaryRunClock(roots_));
  if(!clock)return "binary_native_member_run_clock";
  initial_time_=prior_?prior_->initial_time_:Read<std::uint32_t>({clock,4});
  return ClientBinaryMemberFailure(roots_,previous_addresses_.data(),previous_addresses_.size(),members_.data(),members_.size(),
      prior_?&prior_->room_:nullptr,&room_);
}
bool NativeBindings::DescribeFields(const ClientAtomicInputs &in) {
  const auto &plan=in.scene.State();const auto definitions=S::Fields();
  special_index_.resize(plan.FieldCount(),none);
  // 描述期的两份索引同寿命，按实际arena块计费，离开此函数即归还全部容量。
  S::ClientStateMemoryResource upstream(memory_,memory::Pool::Values);
  std::pmr::monotonic_buffer_resource arena(8192,&upstream);
  ClientExactIndex<FieldKey> index{&arena},display_index{&arena};
  // Describe 已封存具名处理器的整数索引。这里只接合当前资源和原生承载。
  std::size_t special_count=0,display_count=0,companion_count=0,layer_image_count=0;
  for(const auto &entry:in.binding_fields){
    const auto kind=K(entry.kind);
    if(world_target_&&entry.display)++display_count;
    if(kind==K::None)continue;
    if(kind==K::Companion)++companion_count;else ++special_count;
    if(kind==K::LayerImage)++layer_image_count;
  }
  index.reserve(special_count);display_index.reserve(display_count);
  special_.reserve(special_count);companion_fields_.reserve(companion_count);
  layer_images_.reserve(layer_image_count);
  for(const auto &entry:in.binding_fields){
    const auto n=entry.field;
    const auto &f=plan.Field(n);
    const auto &d=definitions[f.definition];const auto kind=K(entry.kind);
    const auto &path=plan.Paths()[f.path];
    if(world_target_&&entry.display)
      display_index.emplace(FieldKey{f.record,path.parent,d.layout,d.index},n);
    if(kind==K::None)continue;
    // Plus父承载只检查、不写入；由FinishFields在独立纯读片内统一检查。
    if(kind==K::Companion){companion_fields_.push_back(n);continue;}
    const auto slot=std::uint32_t(special_.size());special_index_[n]=slot;
    auto &op=special_.emplace_back();
    op.write.kind=kind;op.write.offset=d.native_offset;op.write.size=d.native_size;op.write.count=f.elements;
    const auto &record=plan.Records()[f.record];
    if(!world_target_&&record.layout==F::EntityPlayer::kLayout&&(kind==K::File||kind==K::Animation)){
      auto root=f.path;
      while(plan.Paths()[root].parent!=none)root=plan.Paths()[root].parent;
      const auto field=plan.Paths()[root].step.field;
      op.write.shared_player_sprite=field==F::Entity::sprite_0||field==F::Entity::sprite_1;
    }
    if(kind==K::FXCount)fx_counts_.push_back(n);
    if(kind==K::LayerImage)layer_images_.push_back(n);
    if(kind==K::LayerFrames||kind==K::NullFrames){
      op.first=std::uint32_t(values_.size());op.size=f.elements*4;values_.resize(values_.size()+op.size);
    }else if(kind==K::Text||kind==K::DefaultName){
      const auto &input=in.fields[n].Input();op.write.bytes=input.data;op.write.length=input.bytes;
    }else if(kind==K::Widen||kind==K::DescriptorFlags||kind==K::DoorLight||kind==K::DoorAlpha||kind==K::FXCondition||
      kind==K::FXBackdrop||kind==K::VeinChildren||kind==K::Daily||kind==K::Ascent||kind==K::Time){
      const auto bytes=plan.Bytes(f.bytes);op.write.bytes=bytes.data();op.write.length=bytes.size();
      op.write.frame=plan.Records()[f.record].context.game_frame;
    }
    index.emplace(FieldKey{f.record,path.parent,d.layout,d.index},slot);
  }
  if(world_target_){
    const auto display_field=[&](const S::ClientInstallValue &v,unsigned n){
      const auto found=display_index.find({v.record,v.path,v.layout,std::uint16_t(n)});
      return found==display_index.end()?std::span<const std::byte>{}:plan.Bytes(plan.Field(found->second).bytes);
    };
    // 负ID严格按原生 ~id 下标排列，所有配置及图集先解析为本地资源。
    // 数字计划登记这三个显示布局节点并保留原值序，勿再扫描所有实体标量。
    for(const auto n:plan.NativeDisplayValues()){const auto &v=plan.Value(n);
      if(v.layout!=F::ProceduralDisplayDefinition::kLayout&&v.layout!=F::ProceduralDisplaySet::kLayout)continue;
      const auto field=[&](unsigned n){return display_field(v,n);};
      const auto resource=[&](unsigned n){
        S::BodySourceFieldInfo format;
        if(!S::DescribeBodySourceField(v.layout,n,format))return static_cast<void *>(nullptr);
        const auto key=Read<std::uint16_t>(field(n));
        return key==S::kNullIndex?nullptr:Resource(in,{format.resource_table,key});
      };
      if(v.layout==F::ProceduralDisplaySet::kLayout){
        procedural_atlases_={resource(F::ProceduralDisplaySet::atlas),resource(F::ProceduralDisplaySet::small_atlas)};
        continue;
      }
      const auto text=[&](unsigned n){
        const auto bytes=field(n);
        if(bytes.size()<2||Read<std::uint16_t>(bytes.first(2))!=bytes.size()-2)throw S::ClientShadowError(S::StateReadiness::InvalidEncoding,"binary_procedural_text");
        const std::string_view str(reinterpret_cast<const char *>(bytes.data()+2),bytes.size()-2);
        if(str.find('\0')!=str.npos)throw S::ClientShadowError(S::StateReadiness::InvalidEncoding,"binary_procedural_text_nul");
        return str;
      };
      using namespace F::ProceduralDisplayDefinition;
      const int number=Read<int>(field(id));auto *config=resource(configuration);
      if(number!=~int(procedural_definitions_.size())||!config)return false;
      procedural_definitions_.push_back({config,number,text(name),text(description),text(gfx)});
    }
    if(procedural_definitions_.empty()?(procedural_atlases_[0]||procedural_atlases_[1]):(!procedural_atlases_[0]||!procedural_atlases_[1]))return false;
    bool global=false;
    for(const auto n:plan.NativeDisplayValues()){const auto &v=plan.Value(n);
      if(v.layout!=F::OverlayDisplayState::kLayout)continue;
      const auto element=plan.Paths()[v.path].step.element;
      if(element!=overlay_count_||overlay_count_==overlays_.size())return false;
      auto &overlay=overlays_[overlay_count_++];
      const auto field=[&](unsigned n){return display_field(v,n);};
      using namespace F::OverlayDisplayState;
      overlay.controller=Read<int>(field(controller));
      if(overlay.controller==-1){if(global)return false;global=true;overlay.owner=ClientBinaryGlobalOverlay(roots_);if(!overlay.owner)return false;}
      overlay.generation=Read<std::uint64_t>(field(generation));
      overlay.state=Read<int>(field(F::OverlayDisplayState::state));overlay.id=Read<int>(field(id));overlay.delay=Read<int>(field(delay));
      const auto index=Read<std::uint16_t>(field(player));
      if(index!=S::kNullIndex){
        const auto *member=S::FindStateMember(plan.Records()[v.record].members->Directory(),index);
        if(!member)return false;
        const auto &objects=plan.Objects();
        const auto found=std::lower_bound(objects.begin(),objects.end(),member->identity,[](const auto &o,const auto &m){return o.member.identity<m;});
        if(found==objects.end()||found->member.identity!=member->identity)return false;
        overlay.player=Object(in,std::uint32_t(found-objects.begin()));
        if(!overlay.player){
          if(in.objects[found-objects.begin()].failure==ClientMemberFailure::None)return false;
          overlay.state=0;
        }
      }
      if(overlay.state&&(overlay.id<0||overlay.id>46))return false;
    }
    if(!global)return false;
  }
  execution_.reserve(special_.size());
  Vector<unsigned char> seen{S::ClientStateAllocator<unsigned char>(memory_)};seen.resize(special_.size());
  Vector<unsigned> counts{S::ClientStateAllocator<unsigned>(memory_)};counts.resize(special_.size());
  // 数字下标按 Values 原序封存；引用只在本记录的区间内精确查找。
  for(const auto i:plan.NativeBindingValues()){
    const auto &value=plan.Value(i);if(value.layout||value.path==none)continue;
    const auto &path=plan.Paths()[value.path];
    // 该循环只消费资源／实体引用以及 AnimationState 的两组选择器。
    // 其它标量由完整字段或通用原子写入器消费，无须逐叶查询特殊字段索引。
    const bool selectors=path.step.layout==F::AnimationState::kLayout&&
      (path.step.field==F::AnimationState::layer_frames||path.step.field==F::AnimationState::null_layer_frames);
    if(!selectors&&value.operation!=S::Operation::Resource&&value.operation!=S::Operation::Reference)continue;
    const auto at=index.find({value.record,path.parent,path.step.layout,path.step.field});if(at==index.end())continue;
    auto &op=special_[at->second];const auto kind=op.write.kind;
    if(kind==K::LayerFrames||kind==K::NullFrames){
      const auto b=plan.Bytes(value.bytes);if(path.step.element>=op.write.count||b.size()!=4)return false;
      std::memcpy(values_.data()+op.first+path.step.element*4,b.data(),4);++counts[at->second];continue;
    }
    if(value.operation!=S::Operation::Resource&&value.operation!=S::Operation::Reference)continue;
    const auto link_at=plan.BindingForValue(value.record,i-plan.Records()[value.record].first_value);
    if(!link_at||seen[at->second]++)return false;
    const auto *link=&*link_at;
    if(!link->is_null){
      op.write.resource=link->is_resource?Resource(in,link->resource):Object(in,link->object);
      if(!link->is_resource)op.target=link->object;
      if(link->is_resource&&!op.write.resource)return false;
    }
  }
  for(std::size_t n=0;n<special_.size();++n){
    auto &op=special_[n];
    switch(op.write.kind){
      case K::LayerFrames:case K::NullFrames:
        if(counts[n]!=op.write.count)return false;
        op.write.bytes=op.size?values_.data()+op.first:nullptr;op.write.length=op.size;break;
      case K::File:case K::Animation:case K::WaveAnimation:case K::Image:case K::LayerImage:case K::LayerPath:case K::Rubble:
      case K::Config:case K::DescriptorConfig:case K::BorrowedEntity:case K::BorrowedPlayer:if(!seen[n])return false;break;
      default:break;
    }
  }
  return true;
}
S::ClientStateNativeStep NativeBindings::Preflight(const S::ClientStateApplyBatch &b,const ClientAtomicInputs &in) noexcept {
  if(described_)return Result(batch_==&b,false,"binary_native_preflight_current");
  batch_=&b;roots_=ClientPreparationRoots();vm_=ClientLuaStateIdentity();
  try {
    if(!ClientAtomicInstallWindow(roots_,vm_))return Result(false,false,"binary_native_members_window");
    if(const auto *why=DescribeMembers(in))return Result(false,false,why);
    if(!DescribeFields(in))return Result(false,false,"binary_native_special_preflight");
    described_=true;return Result(true,false,"binary_native_preflight_ready");
  }catch(...){return Result(false,false,"binary_native_binding_preflight_exception");}
}
bool NativeBindings::Resolve(const ClientAtomicInputs &in,std::uint32_t,std::uint32_t n,void *parent,void *&address,std::size_t &extent) noexcept {
  const auto &path=in.scene.State().Paths()[n];
  const auto *field=S::FindField(path.step.layout,path.step.field);if(!field)return false;
  const auto *layout=S::FindLayout(field->nested_layout);if(!layout)return false;
  if(Layer(field->handler))address=ClientBinaryLayer(parent,path.step.element);
  else if(Plus(field->handler))address=ClientBinaryPlus(parent,layout->id,in.reads);
  else if(layout->id==F::OverlayDisplaySet::kLayout||layout->id==F::OverlayDisplayState::kLayout){
    if(layout->id==F::OverlayDisplaySet::kLayout){address=overlays_.data();extent=sizeof(overlays_);return true;}
    if(path.step.element>=overlay_count_)return false;
    address=overlays_[path.step.element].owner;extent=layout->native_size;
    return address&&extent;
  }
  else return false;
  extent=layout->native_size;return address&&extent;
}
// 与 Initialize 共用同一个配置表借用判定，不能把配置原生装载当成纯局部写。
bool NativeBindings::CanBorrowProcedural() const noexcept {
  const auto definition=[](const auto &d){return std::tie(d.configuration,d.id,d.name,d.description,d.gfx);};
  return prior_&&procedural_atlases_==prior_->procedural_atlases_&&
    std::equal(procedural_definitions_.begin(),procedural_definitions_.end(),prior_->procedural_definitions_.begin(),
      prior_->procedural_definitions_.end(),[&](const auto &a,const auto &b){return definition(a)==definition(b);})&&
    ClientBinaryCheckProcedural(roots_,prior_->procedural_);
}
bool NativeBindings::ShowOverlay(const Overlay &o) const noexcept {
  if(o.controller!=-1||!o.state)return false;
  if(prior_)for(const auto &previous:prior_->overlays_)
    if(previous.owner&&previous.controller==-1)return o.generation!=previous.generation;
  return o.generation!=0;
}
bool NativeBindings::WorldMembersRetained() const noexcept {
  return same_members_&&ClientBinaryMemberSetsCurrent(roots_,members_.data(),members_.size(),room_,
    render_members_.data(),render_members_.size());
}
S::ClientStateNativeStep NativeBindings::Initialize(const S::ClientStateApplyBatch &,const ClientAtomicInputs &in) noexcept {
  if(prior_!=world_)return Result(false,false,"binary_native_prior_world_changed");
  bool written=false;
  if(world_target_){
    if(CanBorrowProcedural()){
      // 发布成功才转交原生配置表；失败批次只归还借用。
      procedural_=prior_->procedural_;borrowed_procedural_=true;
    }else{
      if(!ClientBinarySetProcedural(roots_,procedural_,prior_?prior_->procedural_:nullptr,procedural_definitions_.data(),procedural_definitions_.size(),
           procedural_atlases_[0],procedural_atlases_[1]))return Result(false,true,"binary_procedural_owner_install");
      written=true;
    }
    // 描述配置已先写入。原生Init读取本轮房间和维度，此时所有新成员仍私有。
    geometry_=true;
    if(!prior_||!ClientBinaryRoomGeometryCurrent(roots_,room_)){
      if(!ClientBinarySetRoom(roots_,room_))return Result(false,true,"binary_native_room_init_prefix");
      written=true;
    }
  }
  ClientBinaryInitPoolSnapshot before_init;
  const bool has_init=std::any_of(init_.begin(),init_.end(),[](const auto &init){return init.fresh;});
  if(has_init&&!ClientBinaryRememberInitPools(roots_,before_init))return Result(false,written,"binary_native_init_pool_snapshot");
  // Player基类先就绪，之后的派生Init才能读真实的Player前置。后置环统一由EntityPtr接合。
  const char *init_failure=nullptr;
  for(unsigned pass=0;pass<2&&!init_failure;++pass)for(std::uint32_t n=0;n<init_.size();++n){
    auto &init=init_[n];
    if(!init.fresh||bool(init.category)!=bool(pass)||in.objects[n].failure!=ClientMemberFailure::None)continue;
    const bool ok=ClientBinaryInitApply(init,reason_);written|=init.entered;
    if(!ok&&(!in.reject||!in.reject(in.locator,n,ClientMemberFailure::Native,reason_))){
      uncertain_|=init.entered&&!init.returned;init_failure=reason_;break;
    }
  }
  const bool auxiliaries_clean=!has_init||ClientBinaryCleanupInitPools(roots_,before_init,this,[](const void *context,const void *object) noexcept {
    const auto &order=static_cast<const NativeBindings *>(context)->draw_order_;
    const auto key=reinterpret_cast<std::uintptr_t>(object);
    const auto at=std::lower_bound(order.begin(),order.end(),key,[](const auto &entry,auto address){return entry.first<address;});
    return at!=order.end()&&at->first==key;
  },written);
  if(init_failure)return Result(false,written,init_failure);
  if(!auxiliaries_clean)return Result(false,written,"binary_native_init_auxiliary_cleanup");
  const auto &plan=in.scene.State();
  for(const auto &g:plan.Grids())if(!in.objects[g.object].borrowed&&
      in.objects[g.object].failure==ClientMemberFailure::None){
    if(!ClientBinaryGridInit(Object(in,g.object),g.index)&&
        (!in.reject||!in.reject(in.locator,g.object,ClientMemberFailure::Native,"binary_native_grid_init")))
      return Result(false,written,"binary_native_grid_init");
    written=true;
  }
  return Result(true,written,"binary_native_init_complete");
}
S::ClientStateNativeStep NativeBindings::PrepareFieldOwners(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept {
  if(!world_target_)return Result(true,false,"binary_overlay_world_inherited");
  for(unsigned n=0;n<overlay_count_;++n){
    auto &o=overlays_[n];
    if(o.controller!=-1){
      const auto size=S::FindLayout(F::OverlayDisplayState::kLayout)->native_size;
      try{memory_->ChargeNative(size);}catch(...){return Result(false,n!=0,"binary_overlay_budget");}
      if(!ClientBinaryCreateOverlay(o.owner)){
        if(!o.owner)memory_->UnchargeNative(size);else overlay_charge_+=size;
        uncertain_=o.owner!=nullptr;return Result(false,true,"binary_overlay_native_constructor");
      }
      overlay_charge_+=size;
    }
    o.previous_player=ClientBinaryOverlayCurrentPlayer(o.owner);o.entered=true;
    // 私有承载的两个 Sprite 由原子字段安装。只有 Game 全局承载使用本地定义表调用 Show。
    const bool show=ShowOverlay(o);
    const bool ok=show?ClientBinaryShowOverlay(o.owner,o.id,o.delay,o.player):
      ClientBinaryOverlayPlayer(o.owner,o.state?o.player:nullptr);
    if(!ok){uncertain_=true;return Result(false,true,"binary_overlay_native_show");}
  }
  return Result(true,true,"binary_overlay_owner_ready");
}
S::ClientStateNativeStep NativeBindings::InstallField(const S::ClientStateApplyBatch &,const ClientAtomicInputs &in,
    std::uint32_t n) noexcept {
  const auto slot=special_index_[n];if(slot==none)return Result(true,false,"binary_native_display_or_absent");
  auto &op=special_[slot];
  const auto &plan=in.scene.State();const auto &f=plan.Field(n);const auto &path=plan.Paths()[f.path];
  try {
    if(op.target<in.objects.size()&&(in.objects[op.target].failure!=ClientMemberFailure::None||
        in.objects[op.target].unbound))op.write.resource=nullptr;
    void *parent=nullptr;std::size_t extent=0;
    if(!in.locate(in.locator,f.record,path.parent,parent,extent))return Result(false,false,"binary_native_special_parent");
    op.write.parent=parent;
    if(op.write.kind==K::Companion){
      const auto &d=S::Fields()[f.definition];
      return Result(ClientBinaryPlus(parent,d.nested_layout)!=nullptr,false,"binary_native_plus_owner");
    }
    if(op.write.kind==K::VeinChildren){
      const auto &array_path=plan.Paths()[path.parent];
      if(array_path.step.element>=10||array_path.parent==none)return Result(false,false,"binary_native_vein_path");
      void *owner=nullptr;std::size_t owner_size=0;
      if(!in.locate(in.locator,f.record,array_path.parent,owner,owner_size)||owner_size<192+10*52)return Result(false,false,"binary_native_vein_owner");
      op.write.resource=static_cast<std::byte *>(owner)+192;
    }
    if(op.write.size&&(op.write.offset>extent||op.write.size>extent-op.write.offset))return Result(false,false,"binary_native_special_extent");
    const auto charge=ClientBinarySpecialCharge(op.write);
    if(charge){memory_->ChargeNative(charge);op.charge=charge;}
    execution_.push_back(slot);
    const bool ok=ClientBinaryApplySpecial(roots_,op.write,op.undo);uncertain_|=op.undo.uncertain;
    if(op.charge&&!op.undo.storage){memory_->UnchargeNative(op.charge);op.charge=0;}
    return Result(ok,op.undo.applied||op.undo.uncertain,ok?"binary_native_special_installed":"binary_native_special_write");
  }catch(...){return Result(false,false,"binary_native_special_exception");}
}
S::ClientStateNativeStep NativeBindings::FinishFields(const ClientAtomicInputs &in,std::uint8_t streams) noexcept {
  bool written=false;
  try {
    ClientDisplayMemoryScope memory(memory_);
    const auto &plan=in.scene.State();
    if(!companions_checked_){
      // 有子字段的Plus已在晚绑定写入前核定；包括无子字段的所有Plus还须在发布前
      // 逐一核真实holder及动态类型。只在这一轮纯读中共享页见证，不进入原生回调。
      // 与原InstallField一样，每批执行一次；漂移修复仍按各字段的当前父链重新定位。
      ClientNativeReadWindow reads;
      for(auto n:companion_fields_){
        const auto &f=plan.Field(n);const auto &d=S::Fields()[f.definition];
        const auto object=plan.Records()[f.record].object;
        if(in.objects[object].failure!=ClientMemberFailure::None||in.objects[object].unbound)continue;
        void *parent=nullptr;std::size_t extent=0;
        if(!in.locate(in.locator,f.record,plan.Paths()[f.path].parent,parent,extent)||
            !ClientBinaryPlus(parent,d.nested_layout,&reads)){
          if(in.reject&&in.reject(in.locator,object,ClientMemberFailure::Native,"binary_native_plus_owner"))continue;
          return Result(false,false,"binary_native_plus_owner");
        }
      }
      companions_checked_=true;
    }
    // 第二遍只读核对：同一缓存图像若被互不兼容的图层共享，必须在 Render 前拒绝，不能最后写者胜出。
    // 两遍都只访问描述期登记的LayerImage字段，仍逐项核同一流和真实父承载。
    for(unsigned pass=0;pass<2;++pass)for(const auto n:layer_images_){
      const auto slot=special_index_[n];if(slot==none)continue;
      const auto &op=special_[slot];
      if(op.write.kind!=K::LayerImage||!op.undo.entered)continue;
      const auto &f=plan.Field(n);
      const auto object=plan.Records()[f.record].object;
      if(object>=in.objects.size()||in.objects[object].unbound||in.objects[object].failure!=ClientMemberFailure::None)continue;
      // 在定位原生地址之前按流筛选；保留的 joint 安装器仅能代表仍发布的那一流。
      const auto stream=unsigned(plan.Records()[f.record].stream);
      if(stream>1||!(streams&(1u<<stream)))continue;
      const auto &path=plan.Paths()[f.path];
      void *parent=nullptr;std::size_t extent=0;
      if(!in.locate(in.locator,f.record,path.parent,parent,extent)||parent!=op.write.parent){
        if(in.reject&&in.reject(in.locator,object,ClientMemberFailure::Native,"binary_layer_graphics_owner"))continue;
        return Result(false,written,"binary_layer_graphics_owner");
      }
      bool changed=false;
      const bool ok=ClientBinaryLayerGraphics(parent,pass==0,changed);written|=changed;
      if(!ok){
        const char *why=pass?"binary_layer_graphics_conflict":"binary_layer_graphics_format";
        if(!changed&&in.reject&&in.reject(in.locator,object,ClientMemberFailure::Native,why))continue;
        uncertain_|=changed;return Result(false,written,why);
      }
    }
    memory.Complete();
    return Result(true,written,"binary_layer_graphics_ready");
  }catch(...){uncertain_|=written;return Result(false,written,"binary_layer_graphics_exception");}
}
S::ClientStateNativeStep NativeBindings::InstallMembers(const S::ClientStateApplyBatch &,const ClientAtomicInputs &in) noexcept {
  if(prior_!=world_)return Result(false,false,"binary_native_prior_world_changed");
  if(!world_target_)return Result(prior_==world_&&PublishedWorld(),false,"binary_native_body_world_current");
  for(const auto &object:in.objects)if(object.address&&object.failure!=ClientMemberFailure::None){
    const ClientLiveRetirement entry{object.identity,object.address,object.instance};
    if(!ForgetMembers({&entry,1}))return Result(false,false,"binary_native_member_filter");
    same_members_=false;
  }
  // 局部失败仍可跳过单个对象。玩家全部失效时拒绝候选，避免向原生显示发布空 roster。
  if(!room_.roster_count)return Result(false,false,"binary_native_no_player_roster_after_apply");
  // FX vector仍由通用字段接合，原生绘制另外读取可见前缀的count。
  for(const auto n:fx_counts_){
    const auto result=InstallField(*batch_,in,n);if(!result.ready)return result;
  }
  if(const auto *why=ClientBinaryMemberFailure(roots_,previous_addresses_.data(),previous_addresses_.size(),members_.data(),members_.size(),
      prior_?&prior_->room_:nullptr,&room_))return Result(false,false,why);
  members_done_=WorldMembersRetained();
  if(!members_done_){
    members_entered_=true;members_done_=ClientBinarySetMembers(roots_,previous_members_.data(),previous_members_.size(),members_.data(),members_.size(),room_);
  }
  if(!members_done_)return Result(false,true,"binary_native_member_install");
  frame_installed_=ClientBinaryInstallFrame(roots_,in.scene.Display()->Metadata().frame,previous_frame_);
  initial_frame_=prior_?prior_->initial_frame_:previous_frame_;
  if(frame_installed_){candidate_window_=OwnerToken(Owner::Manager);candidate_=this;}
  return Result(frame_installed_,true,"binary_native_frame_install");
}
ClientAtomicReadback NativeBindings::ReadField(const ClientAtomicInputs &in,std::uint32_t n) noexcept {
  using R=ClientAtomicReadback;
  const auto &plan=in.scene.State();const auto &f=plan.Field(n);const auto &d=S::Fields()[f.definition];
  const auto &path=plan.Paths()[f.path];
  if(d.layout==F::GameRun::kLayout&&d.index==F::GameRun::procedural_display)
    return ClientBinaryCheckProcedural(roots_,procedural_)?R::Match:R::Unsafe;
  if(d.layout==F::ProceduralDisplayDefinition::kLayout&&d.index==F::ProceduralDisplayDefinition::id){
    const auto index=plan.Paths()[path.parent].step.element;
    return index<procedural_definitions_.size()&&ClientBinaryCheckProceduralEntry(procedural_,index,procedural_definitions_[index])?R::Match:R::Unsafe;
  }
  if(d.layout==F::OverlayDisplayState::kLayout&&d.index==F::OverlayDisplayState::player){
    const auto element=plan.Paths()[path.parent].step.element;
    if(element>=overlay_count_)return R::Unsafe;
    const auto &o=overlays_[element];
    // 原生登记槽被外部拆除时，完整恢复重新核身体use，再建立登记。
    return ClientBinaryOverlayCurrentPlayer(o.owner)==(o.state?o.player:nullptr)?R::Match:R::Unsafe;
  }
  const auto slot=special_index_[n];if(slot==none)return R::Untracked;
  const auto &op=special_[slot];if(!op.undo.entered)return R::Untracked;
  try {
    void *parent=nullptr;std::size_t extent=0;
    if(!in.locate(in.locator,f.record,path.parent,parent,extent)||parent!=op.write.parent)return R::Unsafe;
    if(op.write.size&&(op.write.offset>extent||op.write.size>extent-op.write.offset))return R::Unsafe;
    return static_cast<R>(ClientBinaryReadSpecial(roots_,op.write));
  }catch(...){return R::Unsafe;}
}
S::ClientStateNativeStep NativeBindings::Publish(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept {
  // 原生写入口负责对象和附件资格，发布只确认展示 owner 仍属于当前场景。
  return Result(prior_==world_&&(world_target_?members_done_&&frame_installed_:PublishedWorld()!=nullptr),
    false,"binary_native_publish_owner");
}
void NativeBindings::Committed(const S::ClientStateApplyBatch &,const ClientAtomicInputs &in) noexcept {
  committed_=true;for(auto n:execution_)special_[n].undo.committed=true;
  if(borrowed_procedural_){prior_->procedural_=nullptr;borrowed_procedural_=false;}
  else ClientBinaryCommitProcedural(procedural_);
#ifdef ISAACCS_E2E
  if(world_target_&&replica::stream::E2ETraceEnabled()){
    BinaryBloodEffects effects,previous;
    for(const auto &member:members_)effects.Observe(member.type,member.variant);
    if(prior_)for(const auto &member:prior_->members_)previous.Observe(member.type,member.variant);
    effects.TraceChange("client",in.scene.Display()->Metadata().frame,previous);
  }
#endif
  if(world_target_){published_scene_=&in.scene;world_=this;}
  if(candidate_==this)candidate_=nullptr;
  prior_=nullptr;previous_members_.clear();previous_addresses_.clear();
}
bool NativeBindings::Quiesce(const S::ClientStateApplyBatch &b,const ClientAtomicInputs &in) noexcept {
  if(!RetirementCurrent(b,in))return false;
  for(auto &o:overlays_)if(o.entered&&(o.controller!=-1||!committed_||world_==this)){
    if(!ClientBinaryOverlayPlayer(o.owner,o.controller==-1&&!committed_?o.previous_player:nullptr)){uncertain_=true;return false;}
    if(committed_)ClientBinaryHideOverlay(o.owner);
  }
  if(!committed_&&frame_installed_&&!ClientBinaryRestoreFrame(roots_,previous_frame_)){uncertain_=true;return false;}
  if(!committed_&&(geometry_||members_entered_)){
    // 失败时先撤候选集合，再恢复旧room与成员。原生异常保留前缀，避免重复登记。
    if(!ClientBinaryRestoreRoom(roots_,previous_room_)){uncertain_=true;return false;}
    if(members_entered_&&!ClientBinarySetMembers(roots_,members_.data(),members_.size(),previous_members_.data(),previous_members_.size(),previous_room_.members)){
      uncertain_=true;return false;
    }
  }
  if(world_==this){
    // 当前world失去发布资格时撤掉它自己的集合。实际根可能仍由其它批次借用。
    if(!ClientBinaryUnpublishWorld(roots_,members_.data(),members_.size(),room_)){uncertain_=true;return false;}
    world_=nullptr;restore_baseline_=restore_procedural_=true;
  }
  const auto &plan=in.scene.State();
  for(std::size_t n=0;n<in.objects.size();++n)if(in.objects[n].retire_root){
    const auto domain=plan.Objects()[n].domain;
    if(!ClientBinaryDetachMember(roots_,in.objects[n].address,domain==D::Grid)){uncertain_=true;return false;}
  }
  return true;
}
bool NativeBindings::UnbindMemberOverlays(std::span<const ClientLiveRetirement> leaving) noexcept {
  const auto retiring=[&](const void *address){
    return address&&std::any_of(leaving.begin(),leaving.end(),[&](const auto &entry){return entry.address==address;});
  };
  for(auto &overlay:overlays_){
    if(retiring(overlay.previous_player))overlay.previous_player=nullptr;
    if(!retiring(overlay.player))continue;
    if(overlay.entered&&ClientBinaryOverlayCurrentPlayer(overlay.owner)==overlay.player&&
        !ClientBinaryOverlayPlayer(overlay.owner,nullptr))return false;
    overlay.player=nullptr;
  }
  return true;
}
// 原生摘表成功后同步当前账目。后续预检不能把新占用同一地址的实例误认为老成员。
bool NativeBindings::ForgetMembers(std::span<const ClientLiveRetirement> leaving) noexcept {
  if(!world_target_)return true;
  for(const auto &entry:leaving){
    const auto address=entry.address;
    std::erase_if(members_,[&](const auto &member){return member.object==address;});
    // 故障后同一批可能遇到全局撤显，回滚表也不能重新暴露已释放的地址。
    if(members_entered_){
      std::erase_if(previous_members_,[&](const auto &member){return member.object==address;});
      std::erase(previous_addresses_,address);
      for(auto &grid:previous_room_.members.grids)if(grid==address)grid=nullptr;
      for(auto &door:previous_room_.members.doors)if(door==address)door=nullptr;
      auto &previous=previous_room_.members;
      const auto old_end=previous.roster.begin()+previous.roster_count;
      const auto old_slot=std::find(previous.roster.begin(),old_end,address);
      if(old_slot!=old_end){std::move(old_slot+1,old_end,old_slot);previous.roster[--previous.roster_count]=nullptr;}
    }
    std::erase(render_members_,address);
    std::erase_if(draw_order_,[&](const auto &order){return order.first==reinterpret_cast<std::uintptr_t>(address);});
    for(auto &grid:room_.grids)if(grid==address)grid=nullptr;
    for(auto &door:room_.doors)if(door==address)door=nullptr;
    const auto end=room_.roster.begin()+room_.roster_count;
    const auto found=std::find(room_.roster.begin(),end,address);
    if(found!=end){
      const auto slot=found-room_.roster.begin();
      std::move(found+1,end,found);room_.roster[--room_.roster_count]=nullptr;
      std::move(roster_ordinals_.begin()+slot+1,roster_ordinals_.begin()+room_.roster_count+1,roster_ordinals_.begin()+slot);
    }
  }
  return true;
}
bool NativeBindings::ClearReferenceTo(const ClientAtomicInputs &in,std::uint32_t n,
    std::span<const ClientLiveRetirement> leaving) noexcept {
  const auto slot=special_index_[n];if(slot==none)return true;
  auto &op=special_[slot];
  if(op.write.kind!=K::BorrowedEntity&&op.write.kind!=K::BorrowedPlayer)return true;
  const auto retiring=[&](const void *address){
    return address&&std::any_of(leaving.begin(),leaving.end(),[&](const auto &entry){return entry.address==address;});
  };
  if(op.undo.original_size==sizeof(void *)){
    void *previous=nullptr;std::memcpy(&previous,op.undo.original.data(),sizeof(previous));
    if(retiring(previous)){previous=nullptr;std::memcpy(op.undo.original.data(),&previous,sizeof(previous));}
  }
  if(!retiring(op.write.resource))return true;
  const auto &plan=in.scene.State();const auto &field=plan.Field(n);
  try {
    void *parent=nullptr;std::size_t extent=0;
    if(in.locate(in.locator,field.record,plan.Paths()[field.path].parent,parent,extent)&&parent==op.write.parent&&
        op.write.offset<=extent&&sizeof(void *)<=extent-op.write.offset&&
        !ClientAtomicClearRetiringReference(static_cast<std::byte *>(parent)+op.write.offset,op.write.resource,false))return false;
    op.write.resource=nullptr;
    return true;
  }catch(...){return false;}
}
bool NativeBindings::Retire(const S::ClientStateApplyBatch &b,const ClientAtomicInputs &in) noexcept {
  if(!RetirementCurrent(b,in))return false;
  for(auto i=execution_.rbegin();i!=execution_.rend();++i){auto &op=special_[*i];
    if(!Undo(op))return false;
  }
  if(restore_baseline_){
    if(!ClientBinaryRestoreRoom(roots_,initial_room_)||!ClientBinaryRestoreFrame(roots_,initial_frame_)){uncertain_=true;return false;}
    auto *clock=ClientBinaryRunClock(roots_);if(!clock)return false;
    std::memcpy(clock,&initial_time_,sizeof(initial_time_));
    restore_baseline_=false;
  }
  const auto &objects=in.scene.State().Objects();
  while(retirement_<objects.size()){
    const auto i=retirement_++;
    if(!in.objects[i].retire_root||!in.objects[i].address)continue;
    const auto domain=objects[i].domain;
    if(domain!=D::Player&&domain!=D::Entity)continue;
    if(!committed_&&i<init_.size()&&!init_[i].returned)continue;
    const auto category=i<init_.size()&&init_[i].object?init_[i].category:
      domain==D::Player?0u:ClientBinaryRetireCategory(in.objects[i].address);
    if(!ClientBinaryEntityRetire(roots_,in.objects[i].address,category)){uncertain_=true;return false;}
  }
  if(borrowed_procedural_){procedural_=nullptr;borrowed_procedural_=false;}
  if(!ClientBinaryRetireProcedural(roots_,procedural_,!committed_||restore_procedural_,committed_)){uncertain_=true;return false;}
  for(auto &o:overlays_)if(o.controller!=-1&&!ClientBinaryDestroyOverlay(o.owner)){uncertain_=true;return false;}
  if(overlay_charge_){memory_->UnchargeNative(overlay_charge_);overlay_charge_=0;}
  return true;
}
bool NativeBindings::UndoField(const S::ClientStateApplyBatch &b,const ClientAtomicInputs &in,std::uint32_t n) noexcept {
  const auto slot=special_index_[n];
  if(slot==none)return true;
  return RetirementCurrent(b,in)&&Undo(special_[slot]);
}
bool NativeBindings::Undo(Special &op) noexcept {
  if(!ClientBinaryRetireSpecial(op.write,op.undo)){uncertain_=true;return false;}
  if(op.charge){memory_->UnchargeNative(op.charge);op.charge=0;}
  return true;
}
bool NativeBindings::GridWaterClip(const void *object,std::uint32_t &value) noexcept {
  // 同一Manager窗口的换房surface使用候选格；窗口外仅借用已提交成员。
  const auto *owner=Visible();
  return owner&&owner->Displayable()&&ClientBinaryGridClip(owner->roots_,owner->room_,object,value);
}
bool NativeBindings::DrawOrder(const void *object,std::uint32_t &value) noexcept {
  const auto *owner=Visible();
  if(!owner||!owner->Displayable())return false;
  const auto key=reinterpret_cast<std::uintptr_t>(object);
  const auto &order=owner->draw_order_;
  const auto found=std::lower_bound(order.begin(),order.end(),key,[](const auto &entry,auto k){return entry.first<k;});
  if(found==order.end()||found->first!=key)return false;
  value=found->second;return true;
}
bool NativeBindings::HudOrdinal(const void *object,unsigned &value) noexcept {
  const auto *owner=Visible();
  if(!owner||!owner->Displayable())return false;
  for(std::size_t i=0;i<owner->room_.roster_count;++i)if(owner->room_.roster[i]==object){
    value=owner->roster_ordinals_[i];return value<4;
  }
  return false;
}
} // namespace
bool ClientBinaryGridWaterClip(const void *object,std::uint32_t &value) noexcept {return NativeBindings::GridWaterClip(object,value);}
bool ClientBinaryDrawOrder(const void *object,std::uint32_t &value) noexcept {return NativeBindings::DrawOrder(object,value);}
bool ClientBinaryHudOrdinal(const void *object,unsigned &value) noexcept {return NativeBindings::HudOrdinal(object,value);}
void NativeBindings::RenderOverlays(const void *global) noexcept {
  const auto *owner=Visible();
  if(!owner||!owner->Displayable()||global!=ClientBinaryGlobalOverlay(owner->roots_))return;
  for(const auto &overlay:owner->overlays_)
    if(overlay.controller>=0&&overlay.state==2)ClientBinaryRenderOverlay(overlay.owner);
}
void ClientBinaryRenderOverlays(const void *global) noexcept {NativeBindings::RenderOverlays(global);}
const ClientSceneInstallPlan *ClientBinaryPublishedWorld() noexcept {return NativeBindings::PublishedWorld();}
std::unique_ptr<ClientAtomicInstallBindings> ClientMakeAtomicInstallBindings(
    const std::shared_ptr<S::ClientStateMemory> &memory,ClientSceneMainPreparation &preparation) {
  return std::make_unique<NativeBindings>(memory,preparation);
}
} // namespace cs::game
