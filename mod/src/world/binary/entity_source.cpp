#include "world/binary/entity_source_internal.h"
#include "core/capture_cost.h"
#include "state_host_fields.inc"
#include <cstdio>
#include <limits>
#include <tuple>

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
  const auto copy_image=memory.copy_image?memory.copy_image:memory.copy;
  auto temp=memory::Reserve(memory::Pool::Workspace,memory.image_size,"entity_capture_rtti_image");
  std::vector<std::byte> image(memory.image_size);
  // PE映像可能含不可读空洞，只复制头和各已映射可读节，而非跨整幅映像盲读。
  if(!copy_image(memory.image_base,image.data(),4096))throw std::runtime_error("entity_capture_pe_header");
  const auto nt=Word(image,0x3c);
  if(nt>3500 || Word(image,nt)!=0x4550)throw std::runtime_error("entity_capture_pe_header");
  const auto sections=std::to_integer<unsigned>(image[nt+6])|(std::to_integer<unsigned>(image[nt+7])<<8);
  const auto optional=std::to_integer<unsigned>(image[nt+20])|(std::to_integer<unsigned>(image[nt+21])<<8);
  const auto first=nt+24+optional;
  if(!sections || sections>32 || first+sections*40>4096)throw std::runtime_error("entity_capture_pe_sections");
  for(unsigned i=0;i<sections;++i){const auto at=first+i*40,rva=Word(image,at+12),size=Word(image,at+8),flags=Word(image,at+36);
    // 可丢弃的重定位节可能已由 Windows 回收，RTTI 不在这些节里。
    if(!(flags&0x40000000) || (flags&0x02000000) || !size)continue;
    if(rva>image.size() || size>image.size()-rva || !copy_image(memory.image_base+rva,image.data()+rva,size))
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
EntityRead FindEntityRead(std::string_view name) noexcept {
  for(const auto &binding:recipes)if(binding.name==name)return binding.read;
  return EntityRead::None;
}

EntitySource::Impl::Impl(MemoryReader m,std::thread::id t,EntityServices s,EntitySourceCapacity c)
    :memory(m),thread(t),services(s),capacity(c) {
  if(t==std::thread::id{} || !s.current || !s.reference || !s.reads || !c.scratch_bytes || !c.witnesses || !c.resources ||
     c.resources>65535 || !c.resource_bytes || !c.animation_layers || !c.topology_bodies || c.topology_bodies>256 || !c.topology_bytes_per_body)
    throw std::invalid_argument("entity_capture_source_capacity");
  const auto retained=sizeof(*this)+512+Fields().size()*sizeof(EntityRead)+c.scratch_bytes+
    std::uint64_t(c.witnesses)*(sizeof(NativeReadWitness)+sizeof(std::uint32_t))+std::uint64_t(c.resources)*(sizeof(NativeResource)+sizeof(NativeConfigOwner)+sizeof(std::uint32_t))+c.resource_bytes+
    std::uint64_t(c.animation_layers)*sizeof(NativeAnimationLayer)+std::uint64_t(c.topology_bodies)*(sizeof(PlayerTopologyRevision)+c.topology_bytes_per_body)+
    65536u*sizeof(NativeSpriteDependency)+4096u*sizeof(NativeResourceMemo);
  memory_charge=memory::Reserve(memory::Pool::Values,retained,"entity_capture_native_sources");
  reads.resize(Fields().size());scratch.resize(c.scratch_bytes);witnesses.resize(c.witnesses);resources.resize(c.resources);
  resource_bytes.resize(c.resource_bytes);animation_layers.resize(c.animation_layers);topologies.resize(c.topology_bodies);
  topology_bytes.resize(std::size_t(c.topology_bodies)*c.topology_bytes_per_body);
  sprite_dependencies.resize(65536);
  config_owners.resize(c.resources);
  witness_order.resize(c.witnesses);resource_order.resize(c.resources);resource_memos.resize(4096);
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
    if(const auto named=FindEntityRead(name);named!=EntityRead::None)recipe=named;
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
        // 路径游标只来自本根自有值或 Track/Value，后两者保留读段和访问异常保护。
        [](const CaptureFieldAccess &a,const CaptureTraversalSource &v,CaptureDiagnostic &d) noexcept {return static_cast<Impl *>(a.window.context)->Close(a,v,d);},true});
  }
  const auto qualified=[](const CaptureFieldAccess &a) noexcept {auto &self=*static_cast<Impl *>(a.window.context);
    return self.facts.ready;};
  const auto player=[](const CaptureFieldAccess &a) noexcept {auto &self=*static_cast<Impl *>(a.window.context);
    return self.facts.ready && self.facts.player_ready;};
  const auto text=[](const CaptureFieldAccess &a) noexcept {auto &self=*static_cast<Impl *>(a.window.context);
    return self.facts.ready && self.facts.text_ready;};
  // 执行器先核当前窗口，再按根复用这些已固定的资格。Incubus 的武器证言仍由具名叶读取并复核。
  const CaptureQualificationBinding qualifications[]={
    {"entity-current-window",qualified,true},{"current_body_field_witness_same_use_and_binding",player,true},
    {"player-equipment-current-window",player,true},{"text-current-use-writer",text,true},
    {"entity-current-window-and-stable-incubus-weapon",qualified,true},
    {"grid-current-window",qualified,true},{"qualified-main-thread-stable-world-owner-and-current-use",qualified,true},
    {"qualified-current-use-and-immutable-resource-epoch",qualified,true},
    {"active-run-stable-seeds-owner",qualified,true}};
  plan=std::make_unique<CapturePlan>(qualifications,leaves,vtables);
  executor=std::make_unique<CaptureExecutor>(*plan,traversals,t,CaptureExecutorCapacity{},CaptureExecutionContext{this,
    [](void *p,const CaptureWitness &w,std::uint8_t table,CaptureLink link,CodecItemConfig &out) noexcept {
      auto &self=*static_cast<Impl *>(p);if(!self.active || self.window.witness!=w || table!=3 || !link.handle || link.handle>self.resource_used)return false;
      const auto &r=self.resources[std::size_t(link.handle-1)];if(r.table!=3 || r.kind!=7)return false;
      out={r.values[0],r.values[1],std::bit_cast<std::int32_t>(r.values[2])};return true;},capture::CaptureExecutionCounts});
  const auto flat_bytes=BindFlatTear();
  memory_charge->Resize(sizeof(*this)+512+flat_bytes+reads.capacity()*sizeof(EntityRead)+scratch.capacity()+
    witnesses.capacity()*sizeof(NativeReadWitness)+witness_order.capacity()*sizeof(std::uint32_t)+
    resources.capacity()*sizeof(NativeResource)+resource_order.capacity()*sizeof(std::uint32_t)+resource_bytes.capacity()+
    config_owners.capacity()*sizeof(NativeConfigOwner)+animation_layers.capacity()*sizeof(NativeAnimationLayer)+
    topologies.capacity()*sizeof(PlayerTopologyRevision)+topology_bytes.capacity()+sprite_dependencies.capacity()*sizeof(NativeSpriteDependency)+
    resource_memos.capacity()*sizeof(NativeResourceMemo)+vtables.capacity()*sizeof(CaptureVtableBinding));
}
bool EntitySource::Impl::ReadScopeCurrent() noexcept {
  if(!active)return false;
  const auto serial=services.reads->Serial();
  return serial&&(!player.read_serial||player.read_serial==serial)&&std::this_thread::get_id()==thread;
}
bool EntitySource::Impl::Current(const CaptureWitness &w,const CaptureRoot &root) noexcept {
  // Copy/Read 直接借用当前窗口的同一份证言，无需逐成员比较自身；外部证言仍完整比较。
  // 读段序号、线程、根身份及当前来源检查照常执行。
  return ReadScopeCurrent()&&
    (&w==&window.witness||w==window.witness)&&root.subject==facts.subject&&
    (root_qualified||services.current(services.owner,w,facts));
}
// 任何地址都只在已核主线程root窗口内借用，所有乘加先用u64检查。
bool EntitySource::Impl::Read(std::uint64_t address,std::span<std::byte> out,CaptureDiagnostic &d) noexcept {
  // 私有Read只消费本owner的facts和window，不必先复制整份subject再与自身比较。
  // 读段、线程和局部撤销在每次复制前检查；同步根在复读证言后核对外部写水位。
  if(!ReadScopeCurrent()||(!root_qualified&&!services.current(services.owner,window.witness,facts)))
    return FailCapture(d,CaptureFailure::InvalidWindow,{facts.subject,facts.address});
  if(!address || address>UINT32_MAX || out.size()>(std::uint64_t{1}<<32)-address)
    return FailCapture(d,CaptureFailure::AddressRange,{facts.subject,facts.address});
  if(!CopyInput(std::uint32_t(address),out))
    return FailCapture(d,CaptureFailure::ReadFailed,{facts.subject,facts.address});
  return true;
}
bool EntitySource::Impl::Track(std::uint64_t address,std::span<std::byte> out,CaptureDiagnostic &d,NativeEvidenceSource source) noexcept {
  if(out.empty())return true;
  if(out.size()>32||address>UINT32_MAX)return FailCapture(d,CaptureFailure::AddressRange,{facts.subject});
  const auto key=std::tuple(source.owner,address,out.size(),source.use);
  const auto end=witness_order.begin()+witness_used;
  const auto at=std::lower_bound(witness_order.begin(),end,key,[&](auto i,auto k){
    const auto &w=witnesses[i];return std::tuple(w.owner,std::uint64_t(w.address),std::size_t(w.size),w.use)<k;});
  if(at!=end){const auto &w=witnesses[*at];if(w.owner==source.owner&&w.address==address&&w.size==out.size()&&w.use==source.use){
    std::copy_n(w.bytes.begin(),w.size,out.begin());return true;}}
  // Read只执行当前读段检查和受保护复制，不插入或删除证言，复用本次未命中的位置。
  const auto position=std::uint32_t(at-witness_order.begin());
  return Read(address,out,d)&&InsertWitness(position,address,out,d,source);
}
// 一个根证言表的键包括子 owner、地址、宽度和用途；World／Body 合并时保留首次诊断位置。
bool EntitySource::Impl::Remember(std::uint64_t address,std::span<const std::byte> bytes,CaptureDiagnostic &d,NativeEvidenceSource source) noexcept {
  if(bytes.empty())return true;
  const auto key=std::tuple(source.owner,address,bytes.size(),source.use);const auto end=witness_order.begin()+witness_used;
  const auto at=std::lower_bound(witness_order.begin(),end,key,[&](auto i,auto k){
    const auto &w=witnesses[i];return std::tuple(w.owner,std::uint64_t(w.address),std::size_t(w.size),w.use)<k;});
  if(at!=end){const auto &w=witnesses[*at];if(w.owner==source.owner&&w.address==address&&w.size==bytes.size()&&w.use==source.use)
    return std::equal(bytes.begin(),bytes.end(),w.bytes.begin())||FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});}
  return InsertWitness(std::uint32_t(at-witness_order.begin()),address,bytes,d,source);
}
bool EntitySource::Impl::InsertWitness(std::uint32_t position,std::uint64_t address,std::span<const std::byte> bytes,
    CaptureDiagnostic &d,NativeEvidenceSource source) noexcept {
  if(!address||address>UINT32_MAX||bytes.size()>32||witness_used==witnesses.size()||position>witness_used)
    return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
  const auto at=witness_order.begin()+position,end=witness_order.begin()+witness_used;
  std::move_backward(at,end,end+1);*at=witness_used;
  auto &mark=witnesses[witness_used++];mark.address=std::uint32_t(address);mark.size=std::uint8_t(bytes.size());
  mark.owner=source.owner;mark.use=source.use;
  mark.layout=evidence_layout;mark.field=evidence_field;
  std::copy(bytes.begin(),bytes.end(),mark.bytes.begin());++stats.witnesses;return true;
}
// 完成证言复读或精确复用后，一次核当前根身份与整段外部资源写水位。
bool EntitySource::Impl::ReviewRoot() noexcept {
  const auto check=services.review?services.review:services.current;
  return ReadScopeCurrent()&&check(services.owner,window.witness,facts)&&services.reads->ReviewRoot();
}
bool EntitySource::Impl::Verify(const CaptureRoot &root,CaptureDiagnostic &d) noexcept {
  capture::Cost review(capture::CostPart::Review);
  if(player.shared_streams)++stats.shared_player_reviews;
  const auto check=services.review?services.review:services.current;
  // 完整根在入口已确认读取资格；独立来源仍在复读前先核自身窗口。
  if((!root_qualified||window.witness.phase==CapturePhase::Production)&&!check(services.owner,window.witness,facts))
    return FailCapture(d,CaptureFailure::InvalidWindow,root);
  std::array<std::byte,32> after{};
  for(std::uint32_t i=0;i<witness_used;++i){
    const auto &mark=witnesses[i];
    ++stats.rechecks;
    if(!Read(mark.address,std::span(after).first(mark.size),d)||!std::equal(after.begin(),after.begin()+mark.size,mark.bytes.begin())){
      FailCapture(d,CaptureFailure::SourceChanged,root,mark.field);
      if(mark.layout)d.layout=mark.layout;
      if(d.field==kNullIndex)d.field=mark.field;
      return false;
    }
  }
  return ReviewRoot()||FailCapture(d,CaptureFailure::InvalidWindow,root);
}
std::span<std::byte> EntitySource::Impl::Allocate(std::uint64_t size,unsigned align,CaptureDiagnostic &d) noexcept {
  const auto first=(std::uint64_t(scratch_used)+align-1)&~std::uint64_t(align-1);
  if(!align || (align&(align-1)) || first>scratch.size() || size>scratch.size()-first){FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});return {};}
  scratch_used=std::uint32_t(first+size);return std::span(scratch).subspan(std::size_t(first),std::size_t(size));
}
// MSVC Release string先核宽长度和SSO，再读有效前缀；没有逐帧std::string构造。
bool EntitySource::Impl::StringAt(std::uint32_t address,std::span<std::byte> out,std::uint32_t &size,CaptureDiagnostic &d,bool track,
    NativeEvidenceSource source) noexcept {
  std::array<std::uint32_t,2> header{},after{};
  if(!(track?Tracked(std::uint64_t(address)+16,header,d,source):Value(std::uint64_t(address)+16,header,d)))return false;
  size=header[0];const auto reserved=header[1];std::uint32_t data=address;
  if(size>out.size() || size>reserved || (reserved<16?reserved!=15:reserved==UINT32_MAX))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  if(reserved>=16 && (!(track?Tracked(address,data,d,source):Value(address,data,d)) || !data))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  // 只读有效字节，不碰SSO未使用槽或堆容量尾部。
  if(size && !Read(data,out.first(size),d))return false;
  if(track&&source.use==NativeEvidenceUse::FileName){
    for(std::uint32_t i=0;i<size;){const auto count=std::min(32u,size-i);
      if(!Remember(std::uint64_t(data)+i,out.subspan(i,count),d,source))return false;
      i+=count;
    }
  }
  if(!track&&(!Value(std::uint64_t(address)+16,after,d)||after!=header))return FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});
  return true;
}
bool EntitySource::Impl::CStringAt(std::uint32_t address,std::uint32_t limit,std::span<std::byte> out,std::uint32_t &size,CaptureDiagnostic &d,bool track,std::uint32_t extent) noexcept {
  if(limit>out.size())return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
  if(extent)limit=std::min(limit,extent);
  for(size=0;size<limit;){
    if(!Current(window.witness,{facts.subject,facts.address}))return FailCapture(d,CaptureFailure::InvalidWindow,{facts.subject});
    std::size_t copied=0;const auto block=out.subspan(size,extent?limit-size:1u);
    if(!services.reads->StringBlock(std::uint64_t(address)+size,block,copied))return FailCapture(d,CaptureFailure::ReadFailed,{facts.subject});
    const auto zero=std::find(block.begin(),block.begin()+copied,std::byte{});
    if(input_recorder){
      const auto consumed=std::size_t(zero-block.begin())+unsigned(zero!=block.begin()+copied);
      RecordInput(std::uint32_t(std::uint64_t(address)+size),block.first(consumed));
    }
    size+=std::uint32_t(zero-block.begin());
    if(zero==block.begin()+copied)continue;
    std::array<std::byte,32> check{};
    for(std::uint32_t i=0;i<=size;){const auto n=std::min(32u,size+1-i);auto verified=std::span(check).first(n);
      if(track){if(!Remember(std::uint64_t(address)+i,out.subspan(i,n),d))return false;}
      else if(!Read(std::uint64_t(address)+i,verified,d)||!std::equal(verified.begin(),verified.end(),out.begin()+i))
        return FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});
      i+=n;}
    return true;}
  return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
}
EntitySource::EntitySource(MemoryReader m,std::thread::id t,EntityServices s,EntitySourceCapacity c):impl_(std::make_unique<Impl>(m,t,s,c)) {}
EntitySource::~EntitySource()=default;
// 每根同步执行独立暂存；资源只追加，失败根不会把未齐备的记录交后台。
bool EntitySource::Capture(const CaptureWitness &w,EntityFacts f,BodyStream stream,CaptureValueWriter &writer,CaptureDiagnostic &d,bool reliable,
    std::uint64_t candidate_topology,std::uint16_t *resolved_layout) noexcept {
  return CaptureRootValues(w,f,stream,writer,d,reliable,candidate_topology,false,resolved_layout);
}
bool EntitySource::CaptureOrdinary(const CaptureWitness &w,EntityFacts f,CaptureValueWriter &writer,CaptureDiagnostic &d,std::uint16_t &resolved_layout,std::uint64_t reusable_base) noexcept {
  if(w.phase!=CapturePhase::Ordinary){FailCapture(d,CaptureFailure::InvalidWindow,{f.subject,f.address});return writer.Fail(d);}
  return CaptureRootValues(w,f,BodyStream::World,writer,d,true,0,true,&resolved_layout,reusable_base);
}
// 一个同步调用持有结构 arena 和证言表。两套选块的执行状态各自重置，原生来源保留至联合复核完成。
bool EntitySource::CaptureOrdinaryFlat(const CaptureWitness &w,EntityFacts f,CaptureValueWriter &writer,
    CaptureDiagnostic &d,std::uint16_t &resolved_layout) noexcept {
  if(w.phase!=CapturePhase::Ordinary){
    FailCapture(d,CaptureFailure::InvalidWindow,{f.subject,f.address});
    return writer.Fail(d);
  }
  return CaptureRootValues(w,f,BodyStream::World,writer,d,true,0,false,&resolved_layout,0,true);
}

bool EntitySource::CaptureRootValues(const CaptureWitness &w,EntityFacts f,BodyStream stream,CaptureValueWriter &writer,
    CaptureDiagnostic &d,bool reliable,std::uint64_t candidate_topology,bool pair_player,std::uint16_t *resolved_layout,
    std::uint64_t reusable_base,bool flat) noexcept {
  auto &s=*impl_;
  if(std::this_thread::get_id()!=s.thread){FailCapture(d,CaptureFailure::WrongThread,{f.subject,f.address});return writer.Fail(d);}
  if(s.active){s.services.reads->Revoke();FailCapture(d,CaptureFailure::Reentrant,{f.subject,f.address});return writer.Fail(d);}
  const bool defer_resource=w.phase==CapturePhase::Ordinary||w.phase==CapturePhase::PlayerPost;
  CaptureReadSession::RootRead root_read(*s.services.reads,defer_resource);
  s.facts=f;s.scratch_used=s.witness_used=0;s.topology_revision=0;s.map_ready=false;s.active=true;
  // 根尾资格通过前，新资源只属于当前根。回卷不触及任何已封存的目录前缀。
  struct End {
    EntitySource &owner;
    Impl &source;
    std::uint32_t resources,bytes,layers,config_owners;
    bool deferred,accepted=false;
    ~End(){
      if(deferred&&!accepted){
        source.config_owner_used=config_owners;
        if(resources!=source.resource_used){
          const auto first=source.resource_order.begin();
          std::remove_if(first,first+source.resource_used,[this](auto i){return i>=resources;});
          source.resource_used=resources;
          source.resource_bytes_used=bytes;
          source.layer_used=layers;
          source.memo_used=0;
          owner.InvalidateReuse();
        }
      }
      source.player.ready=false;source.player.shared_streams=false;source.player.costumes={};source.player.read_serial=0;
      source.active=false;source.root_qualified=false;source.root_input=nullptr;
      source.evidence_layout=0;source.evidence_field=kNullIndex;
    }
  } end{*this,s,s.resource_used,s.resource_bytes_used,s.layer_used,s.config_owner_used,defer_resource};
  s.window={s.thread,w,&s,
    [](void *p,const CaptureWitness &v,const CaptureRoot &r) noexcept {return static_cast<Impl *>(p)->Current(v,r);},
    // CaptureWindow::Copy 核当前根与范围，页权限和 SEH 由读段保护。
    [](void *p,std::uint32_t address,std::span<std::byte> out) noexcept {
      return static_cast<Impl *>(p)->CopyInput(address,out);},true};
  CaptureRoot root{f.subject,f.address,UINT32_MAX-f.address,f.layout};
  if(!f.ready){FailCapture(d,CaptureFailure::MissingQualification,root);return writer.Fail(d);}
  if(!s.window.Check(root,d))return writer.Fail(d);
  s.root_qualified=true;
  const auto *declared=FindLayout(f.layout);
  std::uint32_t vtable=0;
  if((!declared||declared->rtti_rva)&&!s.plan->Identify(s.window,root,d,&vtable))return writer.Fail(d);
  const auto *layout=FindLayout(root.layout);
  if(!layout){FailCapture(d,CaptureFailure::UnknownLayout,root);return writer.Fail(d);}
  if(resolved_layout)*resolved_layout=root.layout;
  root.extent=layout->native_size;
  if(!root.address || root.extent>(std::uint64_t{1}<<32)-root.address){FailCapture(d,CaptureFailure::AddressRange,root);return writer.Fail(d);}
  s.root_layout=root.layout;
  if(flat){
    capture::Cost execution(capture::CostPart::FieldExecution);
    end.accepted=s.ExecuteFlatTear(root,writer,d);
    return end.accepted;
  }
  const bool shared_streams=pair_player&&root.layout==197;
  if(root.layout==197){
    if(!f.player_ready){FailCapture(d,CaptureFailure::MissingQualification,root);return writer.Fail(d);}
    s.player.shared_streams=shared_streams;s.evidence_layout=root.layout;
    // 识别时读取的 vptr 直接登记为证言，最终复核只读一次，不再走 RTTI 分派。
    if(!vtable||!s.Remember(root.address,std::as_bytes(std::span(&vtable,1)),d,{root.address,NativeEvidenceUse::PlayerVtable})||
       !s.PrepareTopology(d)){
      if(d.reason==CaptureFailure::None)FailCapture(d,CaptureFailure::MissingQualification,root);
      if(!d.layout)d.layout=root.layout;
      return writer.Fail(d);
    }
    if(shared_streams)++s.stats.shared_player_prepared;
    s.facts.subject.topology=s.topology_revision;root.subject=s.facts.subject;
    const bool matching_candidate=w.phase==CapturePhase::PlayerPost&&w.required_revision&&candidate_topology==s.topology_revision;
    if(stream==BodyStream::Body&&!reliable&&!matching_candidate){
      const auto index=s.FindTopology(f.subject);
      if(index==UINT32_MAX||s.topologies[index].ordinary!=s.topology_revision){
        FailCapture(d,CaptureFailure::SourceChanged,root);return writer.Fail(d);
      }
    }
  }
  // 非附属 Effect 拒绝实际未知对象。空别名可保留控制块，但没有可读 payload。
  auto *base=FindLayout(root.layout);while(base && base->id!=101)base=FindLayout(base->parent);
  if(base && root.layout!=104 && root.layout!=105 && root.layout!=106){std::array<std::uint32_t,2> holder{};
    if(!s.Value(std::uint64_t(f.address)+0x4b4,holder,d))return writer.Fail(d);
    if(holder[0]){
      // 失败记录只保留变体和共享引用的存在状态，供实机定位。
      std::array<std::uint32_t,2> variant{};
      if(!s.Value(std::uint64_t(f.address)+0x2c,variant,d))return writer.Fail(d);
      std::snprintf(d.detail.data(),d.detail.size(),"effect variant=%u subtype=%u object=%u owner=%u",
        variant[0],variant[1],unsigned(holder[0]!=0),unsigned(holder[1]!=0));
      FailCapture(d,CaptureFailure::UnknownLayout,root);return writer.Fail(d);}}
  NativeRootReuse *reuse=nullptr;
  if(pair_player&&root.layout==host_field::EntityTear::kLayout&&!s.root_reuse.empty()){
    auto &slot=s.root_reuse[root.subject.local%s.root_reuse.size()];
    // 同一房间的存活根保有其槽；溢出的根正常采集，避免彼此覆盖上一帧证据。
    if(!slot.revision||slot.epoch!=s.reuse_epoch||slot.scope!=w.scope||
       (slot.subject.local==root.subject.local&&slot.subject.space==root.subject.space))
      reuse=&slot;
    if(reuse&&reusable_base&&reuse->revision==reusable_base&&reuse->epoch==s.reuse_epoch&&reuse->scope==w.scope&&
       reuse->subject==root.subject&&reuse->address==root.address&&reuse->layout==root.layout&&s.MatchesRoot(*reuse,root)){
      if(!writer.ReuseRoot(root.subject,root.layout,reusable_base))return false;
      reuse->revision=w.revision;++s.stats.unchanged_roots;
      end.accepted=true;
      return true;
    }
    // 命中前使用上一份证明。新证明只在正式采集并通过根尾复核后才可借用。
    if(reuse){
      reuse->revision=0;reuse->reference_count=0;
      reuse->input.evidence={0,0,true};
      s.root_input=reuse;
    }
  }
  std::optional<NativeInputRecorder> record;
  if(reuse)record.emplace(s.input_recorder,reuse->input);
  capture::Cost execution(capture::CostPart::FieldExecution);
  const bool complete=s.executor->ExecuteBound(s.window,root,stream,writer,d,CaptureExecutionLimits{reliable?32u<<20:65536u,262144,32,reliable},
    [](void *context,const CaptureRoot &r,CaptureDiagnostic &out) noexcept {return static_cast<Impl *>(context)->Verify(r,out);},shared_streams);
  if(reuse&&complete){
    if(reuse->input.evidence.valid&&reuse->input.evidence.count){
      reuse->subject=root.subject;reuse->scope=w.scope;reuse->epoch=s.reuse_epoch;
      reuse->revision=w.revision;reuse->address=root.address;reuse->layout=root.layout;
    }else ++s.stats.unchanged_unavailable;
  }
  if(shared_streams&&complete){s.stats.shared_player_consumed+=2;++s.stats.topology_hits;}
  end.accepted=complete;
  return complete;
}
std::uint32_t EntitySource::Impl::FindTopology(CaptureSubject subject) const noexcept {
  for(std::uint32_t i=0;i<topology_used;++i)if(topologies[i].subject.space==subject.space && topologies[i].subject.local==subject.local &&
      topologies[i].subject.use==subject.use)return i;return UINT32_MAX;
}
void EntitySource::OrdinaryCommitted(CaptureSubject subject) noexcept {auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return;
  const auto i=s.FindTopology(subject);if(i!=UINT32_MAX && s.topologies[i].subject==subject)s.topologies[i].ordinary=s.topologies[i].revision;}
void EntitySource::InvalidateReuse() noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return;
  if(++s.reuse_epoch==0){
    for(auto &entry:s.root_reuse)entry.epoch=0;
    for(auto &entry:s.resource_reuse)entry.epoch=0;
    s.reuse_epoch=1;
    s.memo_used=0;s.memo_epoch=0;
  }
}
void EntitySource::Invalidate() noexcept {auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return;
  InvalidateReuse();
  if(s.active)s.services.reads->Revoke();
  for(std::uint32_t i=0;i<s.topology_used;++i)s.topologies[i].ordinary=0;}
// 物理退休才回收拓扑槽，普通绑定撤销仍保留该身体的结构版本。
void EntitySource::Retire(CaptureSubject subject) noexcept {auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return;
  if(s.active&&s.facts.subject.space==subject.space&&s.facts.subject.local==subject.local&&s.facts.subject.use==subject.use)
    s.services.reads->Revoke();
  if(!s.root_reuse.empty()){
    auto &entry=s.root_reuse[subject.local%s.root_reuse.size()];
    if(entry.subject.space==subject.space&&entry.subject.local==subject.local&&entry.subject.use==subject.use)entry.revision=0;
  }
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
bool EntitySource::UsesSprite(std::uint32_t address,std::uint32_t &owner,CaptureSubject &subject) const noexcept {const auto &s=*impl_;
  const auto end=s.sprite_dependencies.begin()+s.sprite_dependency_used;
  const auto at=std::lower_bound(s.sprite_dependencies.begin(),end,address,[](const auto &entry,std::uint32_t p){return entry.address<p;});
  if(at==end||at->address!=address)return false;
  owner=at->owner;subject={at->space,at->local,at->use};return true;}
std::uint64_t EntitySource::RetainedBytes() const noexcept {const auto &s=*impl_;
  return s.memory_charge->Bytes()+s.plan->RetainedBytes()+s.executor->RetainedBytes()+(s.reuse_charge?s.reuse_charge->Bytes():0);}
NativeSourceStats EntitySource::Statistics() const noexcept {return impl_->stats;}
}
