#include "world/binary/entity_source_internal.h"
#include <limits>

namespace cs::world::binary {
namespace {
// 这是同源handler的操作绑定，不是逐字段白名单；所有直接字段仍由生成计划解释。
struct RecipeBinding {std::string_view name;EntityRead read;};
constexpr RecipeBinding recipes[]={
  {"entity_index_to_draw_order",EntityRead::Order},{"ExpireFrame",EntityRead::Expire},
  {"entity_plus_flying_to_value",EntityRead::Flying},{"entity_plus_water_to_value",EntityRead::Water},
  {"anm2_selected_file_to_resource",EntityRead::File},{"layer_spritesheet_path_to_resource",EntityRead::ImagePath},
  {"image_base_name_to_resource",EntityRead::Image},{"anm2_animation_to_index",EntityRead::Animation},
  {"temporary_effect_config_to_index",EntityRead::Item},{"item_config_to_resource",EntityRead::Item},
  {"costumespritedesc_item_resource",EntityRead::Item},{"playertemporaryeffect_item_resource",EntityRead::Item},
  {"entity_selected_config_to_resource",EntityRead::Config},
  {"queueitemdata_item_read",EntityRead::QueueItem},{"queueitemdata_flags_read",EntityRead::QueueFlags},
  {"playercostumemap_index_read",EntityRead::CostumeIndex},{"playercostumemap_layer_id_read",EntityRead::Raw},
  {"npc_controller_to_ui_mode",EntityRead::NpcControl},{"incubus_weapon_to_charge_bar_amount",EntityRead::Incubus},
  {"entityplayer_render_topology_revision_read",EntityRead::Topology},{"entityplayer_costume_topology_revision_read",EntityRead::Topology},
  {"entityplayer_native_membership_read",EntityRead::Membership},{"entityplayer_roster_rank_read",EntityRead::Roster},
  {"entityplayer_control_group_read",EntityRead::Group},{"entityplayer_body_role_read",EntityRead::Role},
  {"entityplayer_control_flags_read",EntityRead::Control},{"optionaldrawsequence_has_draw_record_read",EntityRead::DrawPresent},
  {"entityplayer_inventory_suppressed_read",EntityRead::Bool},
  {"playeractiveitemdesc_charge_read",EntityRead::Raw},{"playeractiveitemdesc_battery_charge_read",EntityRead::Raw},
  {"playeractiveitemdesc_sub_charge_read",EntityRead::Raw},{"playeractiveitemdesc_partial_charge_read",EntityRead::Raw},
  {"playeractiveitemdesc_var_data_read",EntityRead::Raw},{"playertemporaryeffect_cooldown_read",EntityRead::Raw},
  {"drawrecordnode_main_animation_frame_read",EntityRead::Raw},{"drawrecordnode_second_animation_frame_read",EntityRead::DrawSecond},
  {"playerweapontears_modifiers_read",EntityRead::Mask},{"playerweaponbrimstone_modifiers_read",EntityRead::Mask},
  {"playerweaponlaser_modifiers_read",EntityRead::Mask},{"playerweaponknife_modifiers_read",EntityRead::Mask},
  {"playerweaponmonstroslung_modifiers_read",EntityRead::Mask},{"playerweapontechx_modifiers_read",EntityRead::Mask},
  {"playerweaponboneaxe_modifiers_read",EntityRead::Mask},{"playerweaponspiritsword_modifiers_read",EntityRead::Mask},
  {"playerweaponfetus_modifiers_read",EntityRead::Mask},
  {"read_native_vector",EntityRead::Vector},{"read_player_temporary_effect_entries",EntityRead::Vector},
  {"playertemporaryeffects_entries_read",EntityRead::Vector},{"entityplayer__costumeSpriteDescs_read",EntityRead::Vector},
  {"entityplayer__collectibleCounts_read",EntityRead::Vector},{"entityplayer__smeltedTrinkets_read",EntityRead::Vector},
  {"entityplayer_extra_collectible_counts_read",EntityRead::Vector},{"player_extra_trinket_presence_read",EntityRead::TrinketPresence},
  {"read_anm2_default_name",EntityRead::String},{"entityplayer__headAnim_read",EntityRead::String},{"read_text_bytes",EntityRead::String},
  {"entityplayer_inline_sprites_read",EntityRead::Fixed},{"entityplayer__playerCostumeMap_read",EntityRead::Fixed},
  {"entityplayer__activeDesc_read",EntityRead::Fixed},{"entityplayer_pocket_items_read",EntityRead::Fixed},
  {"read_anm2_layer_states",EntityRead::Layers},{"read_layer_frame_selectors",EntityRead::LayerFrames},
  {"read_null_layer_frame_selectors",EntityRead::NullFrames},
  {"read_npc_composite_references",EntityRead::Deque},{"read_npc_secondary_path",EntityRead::Deque},
  {"read_npc_primary_path",EntityRead::Deque},{"entityplayer__afterImageFrames_read",EntityRead::Deque},
  {"read_umbilical_rope_nodes",EntityRead::Rope},{"read_effect_trail_nodes",EntityRead::Trail},
  {"read_effect_ghost_active_sprites",EntityRead::Ghost},
  {"read_familiar_IsaacsBody",EntityRead::Shared},{"read_familiar_AbyssLocust",EntityRead::Shared},
  {"read_familiar_UmbilicalBaby",EntityRead::Shared},{"read_effect_MrMe_owner",EntityRead::Shared},
  {"read_effect_SpriteTrail_owner",EntityRead::Shared},{"read_effect_PickupGhost_owner",EntityRead::Shared},
  {"read_existing_entity_plus",EntityRead::Plus},{"read_existing_EntityFamiliarPlus",EntityRead::Plus},
  {"read_existing_EntityKnifePlus",EntityRead::Plus},{"read_existing_EntityTearPlus",EntityRead::Plus},
  {"read_existing_EntityPickupPlus",EntityRead::Plus},{"read_existing_EntityLaserPlus",EntityRead::Plus},
  {"player_existing_spoofs_members",EntityRead::Spoofs},{"entityplayer__itemWispsList_read",EntityRead::Tree},
  {"itemspoof_blocked_read",EntityRead::Blocked},{"itemspoof_innate_groups_read",EntityRead::Innate},
  {"player_weapon_slots_read",EntityRead::Weapon},{"player_current_record_node",EntityRead::DrawNode},
  {"player_current_record_sprite",EntityRead::DrawSprite}
};
std::uint32_t Word(std::span<const std::byte> bytes,std::size_t at) noexcept {
  std::uint32_t v=0;std::memcpy(&v,bytes.data()+at,4);return v;
}
// 冷读取PE静态元数据；不扫描对象、不校验哈希，虚表只绑定已登记的真实TD。
std::vector<CaptureVtableBinding> ReadVtables(MemoryReader memory,std::span<const std::uint8_t> supported) {
  if(!memory.copy || !memory.image_base || memory.image_size<4096)throw std::invalid_argument("entity_capture_image");
  auto temp=memory::Reserve(memory::Pool::Workspace,memory.image_size,"entity_capture_rtti_image");
  std::vector<std::byte> image(memory.image_size);
  // PE映像可能含不可读空洞，只复制头和各已映射可读节，而非跨整幅映像盲读。
  if(!memory.copy(memory.image_base,image.data(),4096))throw std::runtime_error("entity_capture_pe_header");
  const auto nt=Word(image,0x3c);
  if(nt>3500 || Word(image,nt)!=0x4550)throw std::runtime_error("entity_capture_pe_header");
  const auto sections=std::to_integer<unsigned>(image[nt+6])|(std::to_integer<unsigned>(image[nt+7])<<8);
  const auto optional=std::to_integer<unsigned>(image[nt+20])|(std::to_integer<unsigned>(image[nt+21])<<8);
  const auto first=nt+24+optional;
  if(!sections || sections>32 || first+sections*40>4096)throw std::runtime_error("entity_capture_pe_sections");
  for(unsigned i=0;i<sections;++i){const auto at=first+i*40,rva=Word(image,at+12),size=Word(image,at+8),flags=Word(image,at+36);
    if(!(flags&0x40000000) || !size)continue;
    if(rva>image.size() || size>image.size()-rva || !memory.copy(memory.image_base+rva,image.data()+rva,size))
      throw std::runtime_error("entity_capture_pe_section_read");
  }
  const auto inside=[&](std::uint32_t p,std::size_t n){return p>=memory.image_base && n<=image.size() && p-memory.image_base<=image.size()-n;};
  std::vector<CaptureVtableBinding> result;
  for(std::uint32_t off=4096;off+8<=image.size();off+=4){const auto col=Word(image,off),entry=Word(image,off+4);
    // Hook常驻可替换首个虚函数；身份依据是原生COL/TD，不要求函数仍落在游戏PE。
    if(!inside(col,20) || !entry)continue;const auto locator=col-memory.image_base;
    if(Word(image,locator) || Word(image,locator+4) || Word(image,locator+8))continue;
    const auto td=Word(image,locator+12),chd=Word(image,locator+16);
    if(!inside(td,8) || !inside(chd,16))continue;
    const auto hierarchy=chd-memory.image_base,bases=Word(image,hierarchy+8),array=Word(image,hierarchy+12);
    if(Word(image,hierarchy) || !bases || bases>32 || !inside(array,bases*4))continue;
    const auto descriptor=Word(image,array-memory.image_base);
    if(!inside(descriptor,24) || Word(image,descriptor-memory.image_base)!=td)continue;
    for(const auto &layout:Layouts())if(layout.id<supported.size() && supported[layout.id] && layout.rtti_rva && td-memory.image_base==layout.rtti_rva){
      bool arm=false;for(const auto &v:Variants())if(v.arm==layout.id && v.arm!=v.base){arm=true;break;}
      if(!arm)result.push_back({memory.image_base+off+4,layout.id});break;
    }
  }
  std::sort(result.begin(),result.end(),[](auto a,auto b){return a.vtable<b.vtable;});
  result.erase(std::unique(result.begin(),result.end(),[](auto a,auto b){return a.vtable==b.vtable;}),result.end());
  if(result.empty())throw std::runtime_error("entity_capture_rtti_missing");return result;
}
}

// 冷准备构造全部绑定、arena、资源目录与拓扑保留区，逐帧不调用账本。
EntitySource::Impl::Impl(MemoryReader m,std::thread::id t,EntityServices s,EntitySourceCapacity c)
    :memory(m),thread(t),services(s),capacity(c) {
  if(t==std::thread::id{} || !s.current || !s.reference || !c.scratch_bytes || !c.witnesses || !c.resources ||
     c.resources>65535 || !c.resource_bytes || !c.animation_layers || !c.topology_bodies || c.topology_bodies>256 || !c.topology_bytes_per_body)
    throw std::invalid_argument("entity_capture_source_capacity");
  const auto retained=sizeof(*this)+512+Fields().size()*sizeof(EntityRead)+c.scratch_bytes+
    std::uint64_t(c.witnesses)*sizeof(NativeReadWitness)+std::uint64_t(c.resources)*(sizeof(NativeResource)+sizeof(NativeConfigOwner))+c.resource_bytes+
    std::uint64_t(c.animation_layers)*sizeof(NativeAnimationLayer)+std::uint64_t(c.topology_bodies)*(sizeof(NativeTopology)+c.topology_bytes_per_body)+65536u*sizeof(NativeSpriteDependency);
  memory_charge=memory::Reserve(memory::Pool::Values,retained,"entity_capture_native_sources");
  reads.resize(Fields().size());scratch.resize(c.scratch_bytes);witnesses.resize(c.witnesses);resources.resize(c.resources);
  resource_bytes.resize(c.resource_bytes);animation_layers.resize(c.animation_layers);topologies.resize(c.topology_bodies);
  topology_bytes.resize(std::size_t(c.topology_bodies)*c.topology_bytes_per_body);
  sprite_dependencies.resize(65536);
  config_owners.resize(c.resources);
  std::vector<std::uint8_t> supported(65536);
  for(unsigned id=437;id<=453;++id)supported[id]=1;
  for(const auto &layout:Layouts())if((layout.id>=310 && layout.id<=383) || (layout.id>=422 && layout.id<=424) || layout.id==435 || layout.id==436)supported[layout.id]=1;
  for(const auto &layout:Layouts()){auto parent=&layout;while(parent && parent->id!=100 && parent->id!=199 && parent->id!=239 && parent->id!=270 && parent->id!=276 && parent->id!=237)parent=FindLayout(parent->parent);
    if(parent)supported[layout.id]=1;}
  bool changed=true;while(changed){changed=false;for(const auto &f:Fields())if(supported[f.layout] && f.nested_layout && !supported[f.nested_layout]){supported[f.nested_layout]=1;changed=true;}
    for(const auto &row:LayoutChoices())if(supported[row.layout] && !supported[row.choice]){supported[row.choice]=1;changed=true;}}
  vtables=ReadVtables(m,supported);
  std::vector<CaptureHandlerBinding> leaves;std::vector<CaptureTraversalBinding> traversals;
  for(std::size_t i=0;i<Fields().size();++i){const auto &f=Fields()[i];if(!supported[f.layout])continue;
    auto recipe=EntityRead::None;const std::string_view name=f.handler?f.handler:"";
    if(f.operation==Operation::Reference)recipe=EntityRead::Reference;
    for(const auto &binding:recipes)if(binding.name==name){recipe=binding.read;break;}
    if(name=="room_door_slots_read")recipe=EntityRead::Scene;
    if(name=="display_value_read")recipe=EntityRead::Display;
    if(recipe==EntityRead::None)recipe=GridRecipe(name);
    if(recipe==EntityRead::None)recipe=SceneRecipe(name);
    if(recipe==EntityRead::None)recipe=RunRecipe(name);
    reads[i]=recipe;
    if(name.empty() || recipe==EntityRead::None)continue;
    bool seen=false;for(const auto &b:leaves)if(b.name==name && b.operation==f.operation){seen=true;break;}
    if(seen)continue;
    leaves.push_back({name,f.operation,[](const CaptureFieldAccess &a,CaptureValueWriter &w,CaptureDiagnostic &d) noexcept {
      return static_cast<Impl *>(a.window.context)->Leaf(a,w,d);}});
    if(f.nested_layout || f.operation==Operation::Container || f.operation==Operation::Choice)
      traversals.push_back({name,f.operation,
        [](const CaptureFieldAccess &a,CaptureTraversalSource &v,CaptureDiagnostic &d) noexcept {return static_cast<Impl *>(a.window.context)->Open(a,v,d);},
        [](const CaptureFieldAccess &a,const CaptureTraversalSource &v,std::uint16_t i,CaptureRoot &r,CaptureDiagnostic &d) noexcept {return static_cast<Impl *>(a.window.context)->Element(a,v,i,r,d);},
        [](const CaptureFieldAccess &a,const CaptureTraversalSource &v,CaptureDiagnostic &d) noexcept {return static_cast<Impl *>(a.window.context)->Close(a,v,d);}});
  }
  const auto qualified=[](const CaptureFieldAccess &a) noexcept {auto &self=*static_cast<Impl *>(a.window.context);
    return self.Current(a.window.witness,a.root) && self.facts.ready;};
  const auto player=[](const CaptureFieldAccess &a) noexcept {auto &self=*static_cast<Impl *>(a.window.context);
    return self.Current(a.window.witness,a.root) && self.facts.ready && self.facts.player_ready;};
  const auto text=[](const CaptureFieldAccess &a) noexcept {auto &self=*static_cast<Impl *>(a.window.context);
    return self.Current(a.window.witness,a.root) && self.facts.ready && self.facts.text_ready;};
  const auto incubus=[](const CaptureFieldAccess &a) noexcept {auto &self=*static_cast<Impl *>(a.window.context);std::int32_t value=0;CaptureDiagnostic d;
    return self.Current(a.window.witness,a.root) && self.facts.ready && self.Incubus(a.root.address,value,d);};
  // 名字只在准备期消解；各资格回调本身不读取Lua、Recorder或其他身体证言。
  const CaptureQualificationBinding qualifications[]={
    {"entity-current-window",qualified},{"current_body_field_witness_same_use_and_binding",player},
    {"player-equipment-current-window",player},{"text-current-use-writer",text},
    {"entity-current-window-and-stable-incubus-weapon",incubus},
    {"grid-current-window",qualified},{"qualified-main-thread-stable-world-owner-and-current-use",qualified},
    {"qualified-current-use-and-immutable-resource-epoch",qualified},
    {"active-run-stable-seeds-owner",qualified},
    {"host-final-score-cache-same-world",[](const CaptureFieldAccess &a) noexcept {auto &s=*static_cast<Impl *>(a.window.context);return s.facts.score_ready&&s.Current(a.window.witness,a.root);}}};
  plan=std::make_unique<CapturePlan>(qualifications,leaves,vtables);
  executor=std::make_unique<CaptureExecutor>(*plan,traversals,t,CaptureExecutorCapacity{},CaptureExecutionContext{this,
    [](void *p,const CaptureWitness &w,std::uint8_t table,CaptureLink link,CodecItemConfig &out) noexcept {
      auto &self=*static_cast<Impl *>(p);if(!self.active || self.window.witness!=w || table!=3 || !link.handle || link.handle>self.resource_used)return false;
      const auto &r=self.resources[std::size_t(link.handle-1)];if(r.table!=3 || r.kind!=7)return false;
      out={r.values[0],r.values[1],std::bit_cast<std::int32_t>(r.values[2])};return true;}});
  memory_charge->Resize(retained+vtables.capacity()*sizeof(CaptureVtableBinding));
}
bool EntitySource::Impl::Current(const CaptureWitness &w,const CaptureRoot &root) noexcept {
  return active && std::this_thread::get_id()==thread && w==window.witness && root.subject==facts.subject && services.current(services.owner,w,facts);
}
// 任何地址都只在已核主线程root窗口内借用，所有乘加先用u64检查。
bool EntitySource::Impl::Read(std::uint64_t address,std::span<std::byte> out,CaptureDiagnostic &d) noexcept {
  const CaptureRoot root{facts.subject,facts.address,0,0};
  if(!Current(window.witness,root))return FailCapture(d,CaptureFailure::InvalidWindow,root);
  if(!address || address>UINT32_MAX || out.size()>(std::uint64_t{1}<<32)-address)return FailCapture(d,CaptureFailure::AddressRange,root);
  if(!memory.copy || !memory.copy(std::uint32_t(address),out.data(),out.size()))return FailCapture(d,CaptureFailure::ReadFailed,root);
  return Current(window.witness,root) || FailCapture(d,CaptureFailure::InvalidWindow,root);
}
bool EntitySource::Impl::Track(std::uint64_t address,std::span<std::byte> out,CaptureDiagnostic &d) noexcept {
  if(out.size()>32 || witness_used==witnesses.size())return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
  if(!Read(address,out,d))return false;auto &w=witnesses[witness_used++];w.address=std::uint32_t(address);w.size=std::uint8_t(out.size());
  std::copy(out.begin(),out.end(),w.bytes.begin());return true;
}
std::span<std::byte> EntitySource::Impl::Allocate(std::uint64_t size,unsigned align,CaptureDiagnostic &d) noexcept {
  const auto first=(std::uint64_t(scratch_used)+align-1)&~std::uint64_t(align-1);
  if(!align || (align&(align-1)) || first>scratch.size() || size>scratch.size()-first){FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});return {};}
  scratch_used=std::uint32_t(first+size);return std::span(scratch).subspan(std::size_t(first),std::size_t(size));
}
// MSVC Release string先核宽长度和SSO，再读有效前缀；没有逐帧std::string构造。
bool EntitySource::Impl::StringAt(std::uint32_t address,std::span<std::byte> out,std::uint32_t &size,CaptureDiagnostic &d,bool track) noexcept {
  std::array<std::uint32_t,2> header{},after{};
  if(!(track?Tracked(std::uint64_t(address)+16,header,d):Value(std::uint64_t(address)+16,header,d)))return false;
  size=header[0];const auto reserved=header[1];std::uint32_t data=address;
  if(size>out.size() || size>reserved || (reserved<16?reserved!=15:reserved==UINT32_MAX))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  if(reserved>=16 && (!(track?Tracked(address,data,d):Value(address,data,d)) || !data))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  // 只读有效字节，不碰SSO未使用槽或堆容量尾部。
  if(size && !Read(data,out.first(size),d))return false;
  if(!Value(std::uint64_t(address)+16,after,d) || after!=header)return FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});return true;
}
bool EntitySource::Impl::CStringAt(std::uint32_t address,std::uint32_t limit,std::span<std::byte> out,std::uint32_t &size,CaptureDiagnostic &d,bool track) noexcept {
  if(limit>out.size())return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
  for(size=0;size<limit;++size){if(!Read(std::uint64_t(address)+size,out.subspan(size,1),d))return false;if(out[size]!=std::byte{})continue;
    std::array<std::byte,32> check{};
    for(std::uint32_t i=0;i<=size;){const auto n=std::min(32u,size+1-i);auto block=std::span(check).first(n);
      if(!(track?Track(std::uint64_t(address)+i,block,d):Read(std::uint64_t(address)+i,block,d)) ||
         !std::equal(block.begin(),block.end(),out.begin()+i))return FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});i+=n;}
    return true;}
  return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
}
EntitySource::EntitySource(MemoryReader m,std::thread::id t,EntityServices s,EntitySourceCapacity c):impl_(std::make_unique<Impl>(m,t,s,c)) {}
EntitySource::~EntitySource()=default;
// 每根同步执行独立暂存；资源只追加，失败根不会把未齐备的记录交后台。
bool EntitySource::Capture(const CaptureWitness &w,EntityFacts f,BodyStream stream,CaptureValueWriter &writer,CaptureDiagnostic &d,bool joint) noexcept {
  auto &s=*impl_;if(s.active || std::this_thread::get_id()!=s.thread){FailCapture(d,s.active?CaptureFailure::Reentrant:CaptureFailure::WrongThread,{f.subject,f.address});return writer.Fail(d);}
  s.facts=f;s.scratch_used=s.witness_used=0;s.topology_revision=0;s.map_ready=false;s.active=true;
  struct End {bool &active;~End(){active=false;}} end{s.active};
  s.window={s.thread,w,&s,
    [](void *p,const CaptureWitness &v,const CaptureRoot &r) noexcept {return static_cast<Impl *>(p)->Current(v,r);},
    [](void *p,std::uint32_t address,std::span<std::byte> out) noexcept {auto &self=*static_cast<Impl *>(p);CaptureDiagnostic ignored;return self.Read(address,out,ignored);}};
  CaptureRoot root{f.subject,f.address,UINT32_MAX-f.address,f.layout};
  if(!f.ready || !s.window.Check(root,d) || (!f.layout && !s.plan->Identify(s.window,root,d)))return writer.Fail(d);
  root.extent=FindLayout(root.layout)->native_size;
  if(!root.address || root.extent>(std::uint64_t{1}<<32)-root.address){FailCapture(d,CaptureFailure::AddressRange,root);return writer.Fail(d);}
  s.root_layout=root.layout;
  if(root.layout==197){
    if(!s.PrepareTopology(d))return writer.Fail(d);
    s.facts.subject.topology=s.topology_revision;root.subject=s.facts.subject;
    const auto index=s.FindTopology(f.subject);
    if(stream==BodyStream::Body && !joint && (index==UINT32_MAX || s.topologies[index].ordinary!=s.topology_revision)){
      FailCapture(d,CaptureFailure::NeedsOrdinaryJoint,root);return writer.Fail(d);}
  }
  // 非附属Effect不能带不明payload；不把未登记的实际owner吞成空。
  auto *base=FindLayout(root.layout);while(base && base->id!=101)base=FindLayout(base->parent);
  if(base && root.layout!=104 && root.layout!=105 && root.layout!=106){std::array<std::uint32_t,2> holder{};
    if(!s.Value(std::uint64_t(f.address)+0x4b4,holder,d))return writer.Fail(d);
    if(holder[0] || holder[1]){FailCapture(d,CaptureFailure::UnknownLayout,root);return writer.Fail(d);}}
  if(!s.executor->Execute(s.window,root,stream,writer,d,CaptureExecutionLimits{joint?32u<<20:65536u,262144,32,joint}))return false;
  std::array<std::byte,32> after{};
  for(std::uint32_t i=0;i<s.witness_used;++i){const auto &mark=s.witnesses[i];
    if(!s.Read(mark.address,std::span(after).first(mark.size),d) || !std::equal(after.begin(),after.begin()+mark.size,mark.bytes.begin())){
      FailCapture(d,CaptureFailure::SourceChanged,root);return writer.Fail(d);}}
  return true;
}
std::uint32_t EntitySource::Impl::FindTopology(CaptureSubject subject) const noexcept {
  for(std::uint32_t i=0;i<topology_used;++i)if(topologies[i].subject.space==subject.space && topologies[i].subject.local==subject.local &&
      topologies[i].subject.use==subject.use)return i;return UINT32_MAX;
}
void EntitySource::OrdinaryCommitted(CaptureSubject subject) noexcept {auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return;
  const auto i=s.FindTopology(subject);if(i!=UINT32_MAX && s.topologies[i].subject==subject)s.topologies[i].ordinary=s.topologies[i].revision;}
void EntitySource::Invalidate() noexcept {auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return;for(std::uint32_t i=0;i<s.topology_used;++i)s.topologies[i].ordinary=0;}
// 物理退休才回收拓扑槽，普通绑定撤销仍保留该身体的结构版本。
void EntitySource::Retire(CaptureSubject subject) noexcept {auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return;
  const auto i=s.FindTopology(subject);if(i!=UINT32_MAX)s.topologies[i]={};
  const auto end=s.sprite_dependencies.begin()+s.sprite_dependency_used;
  s.sprite_dependency_used=std::uint32_t(std::remove_if(s.sprite_dependencies.begin(),end,[subject](const auto &entry){
    return entry.space==subject.space && entry.local==subject.local && entry.use==subject.use;})-s.sprite_dependencies.begin());}
NativeResourceView EntitySource::Resources() const noexcept {const auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return {};
  return {std::span(s.resources).first(s.resource_used),std::span(s.resource_bytes).first(s.resource_bytes_used),std::span(s.animation_layers).first(s.layer_used)};}
NativeResourceView EntitySource::PublishedResources(CaptureCatalogStamp stamp) const noexcept {const auto &s=*impl_;
  if(stamp.resources>s.resources.size() || stamp.resource_bytes>s.resource_bytes.size() || stamp.resource_layers>s.animation_layers.size())return {};
  return {std::span(s.resources).first(stamp.resources),std::span(s.resource_bytes).first(stamp.resource_bytes),std::span(s.animation_layers).first(stamp.resource_layers)};}
std::uint32_t EntitySource::Topology(CaptureSubject subject) const noexcept {const auto &s=*impl_;const auto i=s.FindTopology(subject);return i==UINT32_MAX?0:s.topologies[i].revision;}
bool EntitySource::UsesSprite(std::uint32_t address) const noexcept {const auto &s=*impl_;
  const auto end=s.sprite_dependencies.begin()+s.sprite_dependency_used;
  const auto at=std::lower_bound(s.sprite_dependencies.begin(),end,address,[](const auto &entry,std::uint32_t p){return entry.address<p;});
  return at!=end && at->address==address;}
std::uint64_t EntitySource::RetainedBytes() const noexcept {const auto &s=*impl_;
  return s.memory_charge->Bytes()+s.plan->RetainedBytes()+s.executor->RetainedBytes();}
}
