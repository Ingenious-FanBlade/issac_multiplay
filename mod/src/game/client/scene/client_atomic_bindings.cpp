#include "game/client/scene/client_atomic_bindings.h"
#include "game/client/scene/client_atomic_bindings_native.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include <algorithm>
#include <cstring>
#include <map>
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
K Kind(const S::Field &f) noexcept {
  const std::string_view h=f.handler;
  if(S::FindLayout(f.layout)->protocol)return K::Metadata;
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
unsigned Category(const S::ClientInstallEntity &e,std::uint16_t layout) noexcept {
  if(layout==147)return 12;
  switch(e.type){case 1:return 0;case 2:return 2;case 3:return 8;case 4:return 4;case 5:return 7;case 6:return 10;
    case 7:return 5;case 8:return 6;case 9:return 3;case 1000:return 9;case 9001:return 11;default:return 1;}
}
bool Fresh(const S::ClientInstallObject &o) noexcept {
  return o.action==S::ClientInstallAction::Join||o.action==S::ClientInstallAction::Reshape;
}
using FieldKey=std::tuple<std::uint32_t,std::uint32_t,std::uint16_t,std::uint16_t>;
template<class T>using Vector=S::ClientStateVector<T>;
// 一个实例保管一个事务；同一准备owner只有一份已提交world成员，body更新借用它。
class NativeBindings final:public ClientAtomicInstallBindings {
  struct Special {
    ClientBinarySpecialWrite write;
    ClientBinarySpecialUndo undo;
    std::uint32_t first=0,size=0;
    std::size_t charge=0;
  };
  ClientSceneMainPreparation &preparation_;
  std::shared_ptr<S::ClientStateMemory> memory_;
  ClientNativeRoots roots_{};
  void *vm_=nullptr;
  const S::ClientStateApplyBatch *batch_=nullptr;
  const ClientSceneInstallPlan *published_scene_=nullptr;
  std::uint64_t candidate_window_=0;
  Vector<ClientBinaryInit> init_;
  Vector<ClientBinaryEntityMember> members_,previous_members_;
  Vector<void *> previous_addresses_;
  Vector<std::pair<std::uintptr_t,std::uint32_t>> draw_order_;
  Vector<Special> special_;
  Vector<std::uint32_t> special_index_;
  Vector<std::byte> values_;
  Vector<std::uint32_t> execution_;
  Vector<std::uint32_t> fx_counts_;
  ClientBinaryRoomMembers room_;
  ClientBinaryRoomPrevious previous_room_,initial_room_;
  NativeBindings *prior_=nullptr;
  static thread_local NativeBindings *world_,*candidate_;
  bool described_=false,world_target_=false,geometry_=false,members_entered_=false,members_done_=false,
       committed_=false,invalid_=false,uncertain_=false,restore_baseline_=false,frame_installed_=false,restore_procedural_=false;
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
  static const S::ClientInstallBinding *Link(const S::ClientStateInstallPlan &plan,std::size_t n) noexcept {
    const auto at=std::lower_bound(plan.Bindings().begin(),plan.Bindings().end(),n,[](const auto &v,std::size_t i){return v.value<i;});
    return at!=plan.Bindings().end()&&at->value==n?&*at:nullptr;
  }
  bool DescribeMembers(const ClientAtomicInputs &in);
  bool DescribeFields(const ClientAtomicInputs &in);
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
    previous_addresses_(S::ClientStateAllocator<void *>(m)),
    draw_order_(S::ClientStateAllocator<std::pair<std::uintptr_t,std::uint32_t>>(m)),
    special_(S::ClientStateAllocator<Special>(m)),special_index_(S::ClientStateAllocator<std::uint32_t>(m)),
    values_(S::ClientStateAllocator<std::byte>(m)),execution_(S::ClientStateAllocator<std::uint32_t>(m)),
    fx_counts_(S::ClientStateAllocator<std::uint32_t>(m)),procedural_definitions_(S::ClientStateAllocator<ClientBinaryProceduralDefinition>(m)){}
  bool Current(const S::ClientStateApplyBatch &b)const noexcept override {
    return !invalid_&&!uncertain_&&(!batch_||batch_==&b)&&preparation_.Current(b)&&
      (!described_||ClientAtomicInstallWindow(roots_,vm_));
  }
  S::ClientStateNativeStep Preflight(const S::ClientStateApplyBatch &b,const ClientAtomicInputs &in) noexcept override;
  bool LatePath(const S::Field &f)const noexcept override {
    return Layer(f.handler)||Plus(f.handler)||f.nested_layout==F::OverlayDisplaySet::kLayout||f.nested_layout==F::OverlayDisplayState::kLayout;
  }
  bool Covers(const ClientAtomicInputs &in,std::uint32_t n)const noexcept override {
    const auto kind=Kind(S::Fields()[in.scene.State().Fields()[n].definition]);
    return kind!=K::None&&kind!=K::FXCount;
  }
  unsigned Order(const S::Field &f)const noexcept override {
    // Room／Level的标量前缀由原生实体Init读取。完整姿态和动态容器保持后置。
    if((f.layout==276||f.layout==270)&&f.operation==S::Operation::Copy)return 0;
    if(f.layout==F::RoomDescriptor::kLayout&&f.index==F::RoomDescriptor::spawn_seed)return 0;
    if(f.layout==F::Room::kLayout&&(f.index==F::Room::bounds||f.index==F::Room::door_grid_positions))return 0;
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
  S::ClientStateNativeStep InstallField(const S::ClientStateApplyBatch &,const ClientAtomicInputs &,std::uint32_t) noexcept override;
  ClientAtomicReadback ReadField(const ClientAtomicInputs &,std::uint32_t,bool) noexcept override;
  S::ClientStateNativeStep InstallMembers(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  S::ClientStateNativeStep Publish(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  void Committed(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  void Invalidate(const S::ClientStateApplyBatch &,const char *) noexcept override {
    invalid_=true;if(candidate_==this)candidate_=nullptr;
  }
  bool Quiesce(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  bool UndoField(const S::ClientStateApplyBatch &,const ClientAtomicInputs &,std::uint32_t) noexcept override;
  bool RetirementCurrent(const S::ClientStateApplyBatch &b,const ClientAtomicInputs &)const noexcept override {
    return batch_==&b&&!uncertain_&&ClientAtomicInstallWindow(roots_,vm_);
  }
  bool Retire(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept override;
  static bool GridWaterClip(const void *,std::uint32_t &) noexcept;
  static bool DrawOrder(const void *,std::uint32_t &) noexcept;
  static void RenderOverlays(const void *) noexcept;
  static const ClientSceneInstallPlan *PublishedWorld() noexcept {
    return world_&&world_->Displayable()?world_->published_scene_:nullptr;
  }
};
thread_local NativeBindings *NativeBindings::world_=nullptr;
thread_local NativeBindings *NativeBindings::candidate_=nullptr;

bool NativeBindings::DescribeMembers(const ClientAtomicInputs &in) {
  const auto &plan=in.scene.State();world_target_=in.scene.Rooms().has_value();prior_=world_;
  if(prior_&&(prior_->roots_.game!=roots_.game||prior_->roots_.room!=roots_.room||&prior_->preparation_!=&preparation_))return false;
  if(!world_target_)return prior_&&prior_->Displayable();
  const auto &world=plan.World();const auto level=in.scene.Rooms()->Level();
  if(!in.scene.Rooms()->Room().Descriptor().Values().spawn_seed)return false;
  room_.descriptor=Object(in,world.current_descriptor);room_.width=world.grid_width;room_.height=world.grid_height;
  room_.type=in.scene.Rooms()->Room().Values().type;
  room_.dimension=world.dimension;room_.entry=world.current_entry;room_.descriptor_count=level.OrdinaryCount();
  const auto source_slot=in.scene.Rooms()->Room().Descriptor().SourceSlot();
  room_.last_boss=level.Values().current_is_last_boss?int(source_slot):-1;
  for(const auto &descriptor:plan.Descriptors()){
    const auto d=level.DescriptorBySourceSlot(descriptor.source_slot);
    if(!d||!Object(in,descriptor.object))return false;
    room_.descriptors[descriptor.source_slot]=true;
    if(d->Values().greed_treasure_icon){
      if(room_.greed_treasure!=-1)return false;room_.greed_treasure=d->Values().grid;
    }
  }
  for(std::size_t i=0;i<room_.occupancy.size();++i)room_.occupancy[i]=world.occupancy[i]==S::kNullIndex?UINT32_MAX:world.occupancy[i];
  for(const auto &grid:plan.Grids())if(!(room_.grids[grid.index]=Object(in,grid.object)))return false;
  for(const auto &grid:in.scene.Grids()){
    if(!room_.grids[grid.Index()])return false;
    room_.water_clip[grid.Index()]=grid.WaterClipFlags();
  }
  for(std::size_t i=0;i<room_.doors.size();++i)if(world.doors[i]!=none&&!(room_.doors[i]=Object(in,world.doors[i])))return false;
  for(const auto &p:plan.Players())if(p.roster!=S::kNullIndex){room_.roster[p.roster]=Object(in,p.object);++room_.roster_count;}

  init_.resize(plan.Objects().size());
  Vector<std::pair<unsigned,std::uint32_t>> ordered{S::ClientStateAllocator<std::pair<unsigned,std::uint32_t>>(memory_)};
  const auto shadow=plan.Effective(S::BodyStream::World),body=plan.Effective(S::BodyStream::Body);
  const auto &objects=plan.Objects();
  for(const auto &entity:plan.Entities()){
    const auto n=entity.object;const auto &obj=objects[n];auto &init=init_[n];
    init.object=Object(in,n);init.type=entity.type;init.variant=entity.variant;init.subtype=entity.subtype;
    init.seed=entity.init_seed;init.category=Category(entity,obj.member.identity.layout);
    init.fresh=Fresh(obj);init.config=Resource(in,entity.configuration);
    if(entity.configuration.index!=S::kNullIndex&&!init.config)return false;
    if(!ClientBinaryInitCheck(init))return false;
    const auto r=shadow->Find(obj.member.identity);if(!r)return false;
    auto &m=members_.emplace_back();m.object=init.object;m.type=init.type;m.variant=init.variant;m.subtype=init.subtype;
    const auto *flags=r->Field(15);if(!flags||!flags->present)return false;
    m.add_update=m.update=(Read<std::uint64_t>(flags->bytes)&8)==0;
    if(obj.domain==D::Player){
      const auto p=std::find_if(plan.Players().begin(),plan.Players().end(),[n](const auto &v){return v.object==n;});if(p==plan.Players().end())return false;
      init.player_type=p->player_type;m.valid=p->membership&1;m.room=p->membership&2;m.render=p->membership&4;m.update=p->membership&8;
    }
    ordered.emplace_back(entity.order,std::uint32_t(members_.size()-1));
    draw_order_.emplace_back(reinterpret_cast<std::uintptr_t>(init.object),entity.order);
    if(init.fresh){
      const auto pose=obj.domain==D::Player?(body?body->Find(obj.member.identity):std::nullopt):r;
      if(!pose)return false;
      const auto *position=pose->Field(10),*velocity=pose->Field(12);
      if(!position||!velocity||!position->present||!velocity->present)return false;
      init.position=Read<std::array<float,2>>(position->bytes);init.velocity=Read<std::array<float,2>>(velocity->bytes);
      const auto *spawner_type=r->Field(4),*spawner_variant=r->Field(5);
      if(!spawner_type||!spawner_variant||!spawner_type->present||!spawner_variant->present)return false;
      init.spawner_type=Read<std::uint32_t>(spawner_type->bytes);init.spawner_variant=Read<std::uint32_t>(spawner_variant->bytes);
    }
  }
  std::sort(ordered.begin(),ordered.end());
  std::sort(draw_order_.begin(),draw_order_.end());
  if(std::adjacent_find(draw_order_.begin(),draw_order_.end(),[](const auto &a,const auto &b){return a.first==b.first;})!=draw_order_.end())return false;
  Vector<ClientBinaryEntityMember> sorted{S::ClientStateAllocator<ClientBinaryEntityMember>(memory_)};
  sorted.reserve(ordered.size());
  for(const auto &[order,index]:ordered)sorted.push_back(members_[index]);
  members_.swap(sorted);
  for(const auto &link:plan.Bindings()){
    if(link.is_resource||link.is_null||link.object==none)continue;
    const auto &value=plan.Values()[link.value];const auto &record=plan.Records()[value.record];
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
  }
  if(!ClientBinaryCaptureRoom(roots_,previous_room_))return false;
  if(prior_){previous_room_.members.roster=prior_->room_.roster;previous_room_.members.roster_count=prior_->room_.roster_count;
    previous_room_.members.grids=prior_->room_.grids;previous_room_.members.doors=prior_->room_.doors;}
  initial_room_=prior_?prior_->initial_room_:previous_room_;
  const auto *clock=static_cast<const std::byte *>(ClientBinaryRunClock(roots_));
  if(!clock)return false;
  initial_time_=prior_?prior_->initial_time_:Read<std::uint32_t>({clock,4});
  return ClientBinaryCheckMembers(roots_,previous_addresses_.data(),previous_addresses_.size(),members_.data(),members_.size(),
      prior_?&prior_->room_:nullptr,&room_);
}
bool NativeBindings::DescribeFields(const ClientAtomicInputs &in) {
  const auto &plan=in.scene.State();const auto definitions=S::Fields();
  special_index_.resize(plan.Fields().size(),none);
  using Entry=std::pair<const FieldKey,std::uint32_t>;
  std::map<FieldKey,std::uint32_t,std::less<FieldKey>,S::ClientStateAllocator<Entry>> index{S::ClientStateAllocator<Entry>(memory_)};
  decltype(index) display_index{S::ClientStateAllocator<Entry>(memory_)};
  for(std::uint32_t n=0;n<plan.Fields().size();++n){
    const auto &f=plan.Fields()[n];if(plan.Records()[f.record].event!=none)continue;
    const auto &d=definitions[f.definition];const auto kind=Kind(d);
    const auto &path=plan.Paths()[f.path];
    if(world_target_&&f.present&&d.layout>=F::OverlayDisplayState::kLayout&&d.layout<=F::ProceduralDisplaySet::kLayout)
      display_index.emplace(FieldKey{f.record,path.parent,d.layout,d.index},n);
    if(kind==K::None||kind==K::Metadata||!f.present)continue;
    const auto slot=std::uint32_t(special_.size());special_index_[n]=slot;
    auto &op=special_.emplace_back();
    op.write.kind=kind;op.write.offset=d.native_offset;op.write.size=d.native_size;op.write.count=f.elements;
    if(kind==K::FXCount)fx_counts_.push_back(n);
    if(kind==K::LayerFrames||kind==K::NullFrames){
      op.first=std::uint32_t(values_.size());op.size=f.elements*4;values_.resize(values_.size()+op.size);
    }else if(kind==K::Text||kind==K::DefaultName){
      const auto &input=in.fields[n].input;op.write.bytes=input.data;op.write.length=input.bytes;
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
      return found==display_index.end()?std::span<const std::byte>{}:plan.Bytes(plan.Fields()[found->second].bytes);
    };
    // 负ID严格按原生 ~id 下标排列，所有配置及图集先解析为本地资源。
    for(const auto &v:plan.Values())if(v.layout==F::ProceduralDisplayDefinition::kLayout||v.layout==F::ProceduralDisplaySet::kLayout){
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
    for(const auto &v:plan.Values())if(v.layout==F::OverlayDisplayState::kLayout){
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
        if(!overlay.player)return false;
      }
      if(overlay.state&&(overlay.id<0||overlay.id>46))return false;
    }
    if(!global)return false;
  }
  execution_.reserve(special_.size());
  Vector<unsigned char> seen{S::ClientStateAllocator<unsigned char>(memory_)};seen.resize(special_.size());
  Vector<unsigned> counts{S::ClientStateAllocator<unsigned>(memory_)};counts.resize(special_.size());
  for(std::size_t i=0;i<plan.Values().size();++i){
    const auto &value=plan.Values()[i];if(value.layout||value.path==none)continue;
    const auto &path=plan.Paths()[value.path];const auto at=index.find({value.record,path.parent,path.step.layout,path.step.field});if(at==index.end())continue;
    auto &op=special_[at->second];const auto kind=op.write.kind;
    if(kind==K::LayerFrames||kind==K::NullFrames){
      const auto b=plan.Bytes(value.bytes);if(path.step.element>=op.write.count||b.size()!=4)return false;
      std::memcpy(values_.data()+op.first+path.step.element*4,b.data(),4);++counts[at->second];continue;
    }
    if(value.operation!=S::Operation::Resource&&value.operation!=S::Operation::Reference)continue;
    const auto *link=Link(plan,i);if(!link||seen[at->second]++)return false;
    if(!link->is_null){
      op.write.resource=link->is_resource?Resource(in,link->resource):Object(in,link->object);
      if(!op.write.resource)return false;
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
    if(!ClientAtomicInstallWindow(roots_,vm_)||!DescribeMembers(in))return Result(false,false,"binary_native_members_preflight");
    if(!DescribeFields(in))return Result(false,false,"binary_native_special_preflight");
    described_=true;return Result(true,false,"binary_native_preflight_ready");
  }catch(...){return Result(false,false,"binary_native_binding_preflight_exception");}
}
bool NativeBindings::Resolve(const ClientAtomicInputs &in,std::uint32_t,std::uint32_t n,void *parent,void *&address,std::size_t &extent) noexcept {
  const auto &path=in.scene.State().Paths()[n];
  const auto *field=S::FindField(path.step.layout,path.step.field);if(!field)return false;
  const auto *layout=S::FindLayout(field->nested_layout);if(!layout)return false;
  if(Layer(field->handler))address=ClientBinaryLayer(parent,path.step.element);
  else if(Plus(field->handler))address=ClientBinaryPlus(parent,layout->id);
  else if(layout->id==F::OverlayDisplaySet::kLayout||layout->id==F::OverlayDisplayState::kLayout){
    if(layout->id==F::OverlayDisplaySet::kLayout){address=overlays_.data();extent=sizeof(overlays_);return true;}
    if(path.step.element>=overlay_count_)return false;
    address=overlays_[path.step.element].owner;extent=layout->native_size;
    return address&&extent;
  }
  else return false;
  extent=layout->native_size;return address&&extent;
}
S::ClientStateNativeStep NativeBindings::Initialize(const S::ClientStateApplyBatch &,const ClientAtomicInputs &in) noexcept {
  if(prior_!=world_)return Result(false,false,"binary_native_prior_world_changed");
  bool written=false;
  if(world_target_){
    if(!ClientBinarySetProcedural(roots_,procedural_,prior_?prior_->procedural_:nullptr,procedural_definitions_.data(),procedural_definitions_.size(),
         procedural_atlases_[0],procedural_atlases_[1]))return Result(false,true,"binary_procedural_owner_install");
    // 描述配置已先写入。原生Init读取本轮房间和维度，此时所有新成员仍私有。
    geometry_=true;
    if(!ClientBinarySetRoom(roots_,room_))return Result(false,true,"binary_native_room_init_prefix");
    written=true;
  }
  // Player基类先就绪，之后的派生Init才能读真实的Player前置。后置环统一由EntityPtr接合。
  for(unsigned pass=0;pass<2;++pass)for(auto &init:init_)if(init.fresh&&bool(init.category)==bool(pass)){
    const bool ok=ClientBinaryInitApply(init,reason_);written|=init.entered;
    if(!ok){uncertain_|=init.entered&&!init.returned;return Result(false,written,reason_);}
  }
  const auto &plan=in.scene.State();
  for(const auto &g:plan.Grids())if(Fresh(plan.Objects()[g.object])){
    if(!ClientBinaryGridInit(Object(in,g.object),g.index))return Result(false,written,"binary_native_grid_init");
    written=true;
  }
  return Result(true,written,"binary_native_init_complete");
}
S::ClientStateNativeStep NativeBindings::PrepareFieldOwners(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept {
  if(!world_target_)return Result(true,false,"binary_overlay_world_inherited");
  std::uint64_t previous_generation=0;
  if(prior_)for(const auto &previous:prior_->overlays_)if(previous.owner&&previous.controller==-1)previous_generation=previous.generation;
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
    const bool show=o.controller==-1&&o.state&&o.generation!=previous_generation;
    const bool ok=show?ClientBinaryShowOverlay(o.owner,o.id,o.delay,o.player):
      ClientBinaryOverlayPlayer(o.owner,o.state?o.player:nullptr);
    if(!ok){uncertain_=true;return Result(false,true,"binary_overlay_native_show");}
  }
  return Result(true,true,"binary_overlay_owner_ready");
}
S::ClientStateNativeStep NativeBindings::InstallField(const S::ClientStateApplyBatch &,const ClientAtomicInputs &in,std::uint32_t n) noexcept {
  const auto slot=special_index_[n];if(slot==none)return Result(true,false,"binary_native_display_or_absent");
  auto &op=special_[slot];
  const auto &plan=in.scene.State();const auto &f=plan.Fields()[n];const auto &path=plan.Paths()[f.path];
  try {
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
S::ClientStateNativeStep NativeBindings::InstallMembers(const S::ClientStateApplyBatch &,const ClientAtomicInputs &in) noexcept {
  if(prior_!=world_)return Result(false,false,"binary_native_prior_world_changed");
  if(!world_target_)return Result(prior_==world_&&PublishedWorld(),false,"binary_native_body_world_current");
  // FX vector仍由通用字段接合，原生绘制另外读取可见前缀的count。
  for(const auto n:fx_counts_){
    const auto result=InstallField(*batch_,in,n);if(!result.ready)return result;
  }
  if(!ClientBinaryCheckMembers(roots_,previous_addresses_.data(),previous_addresses_.size(),members_.data(),members_.size(),
      prior_?&prior_->room_:nullptr,&room_))return Result(false,false,"binary_native_members_changed_after_init");
  members_entered_=true;members_done_=ClientBinarySetMembers(roots_,previous_members_.data(),previous_members_.size(),members_.data(),members_.size(),room_);
  if(!members_done_)return Result(false,true,"binary_native_member_install");
  frame_installed_=ClientBinaryInstallFrame(roots_,in.scene.Display()->Metadata().frame,previous_frame_);
  initial_frame_=prior_?prior_->initial_frame_:previous_frame_;
  if(frame_installed_){candidate_window_=OwnerToken(Owner::Manager);candidate_=this;}
  return Result(frame_installed_,true,"binary_native_frame_install");
}
ClientAtomicReadback NativeBindings::ReadField(const ClientAtomicInputs &in,std::uint32_t n,bool repair) noexcept {
  using R=ClientAtomicReadback;
  const auto &plan=in.scene.State();const auto &f=plan.Fields()[n];const auto &d=S::Fields()[f.definition];
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
    return static_cast<R>(ClientBinaryReadSpecial(roots_,op.write,repair));
  }catch(...){return R::Unsafe;}
}
S::ClientStateNativeStep NativeBindings::Publish(const S::ClientStateApplyBatch &,const ClientAtomicInputs &) noexcept {
  const bool valid=prior_==world_&&(world_target_?members_done_&&ClientBinaryCheckProcedural(roots_,procedural_)&&
    ClientBinaryCheckPublished(roots_,members_.data(),members_.size(),room_)&&
    std::all_of(overlays_.begin(),overlays_.begin()+overlay_count_,[](const auto &o){
      return o.entered&&ClientBinaryOverlayCurrentPlayer(o.owner)==(o.state?o.player:nullptr);
    }):PublishedWorld()!=nullptr);
  return Result(valid,false,"binary_native_members_postcondition");
}
void NativeBindings::Committed(const S::ClientStateApplyBatch &,const ClientAtomicInputs &in) noexcept {
  committed_=true;for(auto n:execution_)special_[n].undo.committed=true;
  ClientBinaryCommitProcedural(procedural_);
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
  if(!committed_&&geometry_){
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
    if(!ClientBinaryEntityRetire(in.objects[i].address,category)){uncertain_=true;return false;}
  }
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
} // namespace
bool ClientBinaryGridWaterClip(const void *object,std::uint32_t &value) noexcept {return NativeBindings::GridWaterClip(object,value);}
bool ClientBinaryDrawOrder(const void *object,std::uint32_t &value) noexcept {return NativeBindings::DrawOrder(object,value);}
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
