#include "game/client/scene/client_install_plan.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>

namespace cs::game {
namespace {
namespace S=state;
#include "state_host_fields.inc"
namespace F=host_field;
[[noreturn]] void Fail(S::StateReadiness r,const char *why){throw S::ClientShadowError(r,why);}
ClientObjectKey Key(std::uint64_t run,const S::PresentationMember &id){return {run,id.id,id.space};}
S::ClientShadowRecordView Record(const S::ClientStateInstallPlan &plan,std::uint32_t object) {
  if(object>=plan.Objects().size())Fail(S::StateReadiness::InvalidReference,"client_scene_plan_object");
  const auto r=plan.Effective(S::BodyStream::World)->Find(plan.Objects()[object].member.identity);
  if(!r)Fail(S::StateReadiness::SourcePending,"client_scene_plan_record");return *r;
}
// 原始块只借用到描述函数返回；返回描述拥有自己的字节和Peer分配器。
ClientWorldShadowRecordView WorldRecord(const S::ClientStateInstallPlan &plan,std::uint32_t object,const ClientRoomLevelContext &context) {
  const auto r=Record(plan,object);ClientWorldShadowRecordView value;const auto &id=r.Identity();
  value.identity={context,Key(context.floor.run,id),id.use,id.shape,r.Member(),id.layout};
  for(unsigned b=0;b<32;++b)if(const auto block=r.Block(std::uint8_t(b)))value.blocks[b]={value.identity.key,id.use,id.shape,block->version,
    std::uint32_t(block->bytes.size()),id.layout,reinterpret_cast<const std::uint8_t *>(block->bytes.data()),true};
  return value;
}
ClientGridShadowRecordView GridRecord(const S::ClientStateInstallPlan &plan,std::uint32_t object) {
  const auto r=Record(plan,object);const auto &id=r.Identity();const auto &c=plan.Effective(S::BodyStream::World)->Context();ClientGridShadowRecordView value;
  value.identity={Key(c.scope.run,id),{c.scope.run,c.scope.room,7},id.use,id.shape,c.member_epoch,c.resource_epoch,c.target_version,c.game_frame,r.Member(),id.layout,c.layout_version};
  for(unsigned b=0;b<32;++b)if(const auto block=r.Block(std::uint8_t(b)))value.blocks[b]={value.identity.key,id.use,id.shape,block->version,
    std::uint32_t(block->bytes.size()),id.layout,reinterpret_cast<const std::uint8_t *>(block->bytes.data()),true};
  value.backing=plan.Effective(S::BodyStream::World);return value;
}
// 已解码实例的字段按 record／父路径索引，保留文字及资源目录的原始寿命。
class DisplayFields final {
public:
  using Key=std::tuple<std::uint32_t,std::uint32_t,std::uint16_t,std::uint16_t>;
  using Row=std::pair<const Key,S::ClientInstallField>;
  explicit DisplayFields(const S::ClientStateInstallPlan &p):plan(p),
    fields(S::ClientStateAllocator<Row>(p.Memory(),memory::Pool::Workspace)){
    for(const auto i:p.PresentationFields()){
      const auto &f=p.Field(i);const auto &d=S::Fields()[f.definition];
      fields.emplace(Key{f.record,p.Paths()[f.path].parent,d.layout,d.index},f);
    }
  }
  template<class T> T Number(const S::ClientInstallValue &v,unsigned field)const {
    const auto b=Bytes(v,field);
    if(b.size()!=sizeof(T))Fail(S::StateReadiness::InvalidEncoding,"binary_display_number_width");
    T value;std::memcpy(&value,b.data(),sizeof(value));return value;
  }
  std::string_view Text(const S::ClientInstallValue &v,unsigned field)const {
    const auto b=Bytes(v,field);
    if(b.size()<2)Fail(S::StateReadiness::InvalidEncoding,"binary_display_text");
    std::uint16_t n;std::memcpy(&n,b.data(),2);
    if(b.size()!=std::size_t(n)+2)Fail(S::StateReadiness::InvalidEncoding,"binary_display_text_width");
    const std::string_view text(reinterpret_cast<const char *>(b.data()+2),n);
    if(text.find('\0')!=std::string_view::npos)Fail(S::StateReadiness::InvalidEncoding,"binary_display_embedded_nul");
    return text;
  }
  S::ResourceKey Resource(const S::ClientInstallValue &v,unsigned field)const {
    S::BodySourceFieldInfo info;
    if(!S::DescribeBodySourceField(v.layout,field,info))Fail(S::StateReadiness::InvalidReference,"binary_display_resource_field");
    return {info.resource_table,Number<std::uint16_t>(v,field)};
  }
  std::uint32_t Player(const S::ClientInstallValue &v,unsigned field)const {
    const auto index=Number<std::uint16_t>(v,field);
    if(index==S::kNullIndex)return S::kClientPlanNone;
    const auto *member=S::FindStateMember(plan.Records()[v.record].members->Directory(),index);
    if(member)for(std::uint32_t i=0;i<plan.Objects().size();++i)if(plan.Objects()[i].member.identity==member->identity)return i;
    Fail(S::StateReadiness::InvalidReference,"binary_display_player_binding");
  }
  std::string_view Sample(const S::ClientInstallValue &v,S::ResourceKey key)const {
    const auto *entry=S::FindResource(plan.Records()[v.record].resources->Directory(),key);
    if(!entry)Fail(S::StateReadiness::MissingClosure,"binary_display_sample");
    std::array<S::ProjectionFact,32> facts{};std::array<S::BodyFieldView,32> values{};
    S::ResourceCodecScratch scratch{{facts,0,values}};
    return S::DescribeResourceLocal(*entry,scratch).text[0];
  }
private:
  std::span<const std::byte> Bytes(const S::ClientInstallValue &v,unsigned field)const {
    const auto i=fields.find({v.record,v.path,v.layout,std::uint16_t(field)});
    if(i==fields.end()||!i->second.present)Fail(S::StateReadiness::InvalidEncoding,"binary_display_field_missing");
    return plan.Bytes(i->second.bytes);
  }
  const S::ClientStateInstallPlan &plan;
  std::map<Key,S::ClientInstallField,std::less<Key>,S::ClientStateAllocator<Row>> fields;
};
}
ClientSceneInstallPlan::ClientSceneInstallPlan(std::shared_ptr<const S::ClientStateInstallPlan> s)
  :state_(std::move(s)),grids_(S::ClientStateAllocator<ClientSceneGridDisplay>(state_->Memory())),
    sounds_(S::ClientStateAllocator<ClientDisplaySound>(state_->Memory())),
    sound_links_(S::ClientStateAllocator<ClientSceneDisplayLinks>(state_->Memory())),
    event_links_(S::ClientStateAllocator<ClientSceneDisplayLinks>(state_->Memory())),
    events_(S::ClientStateAllocator<ClientDisplayEvent>(state_->Memory())) {}
bool ClientSceneInstallPlan::Releasable()const noexcept {
  // batch.target.install与本描述各持一份数字计划；Room描述另由Display内部共享一次。
  if(state_.use_count()>2||display_.use_count()>1)return false;
  if(display_&&display_->StorageReferences()>1)return false;
  if(rooms_&&rooms_->StorageReferences()>(display_?2:1))return false;
  return true;
}
// World 派生描述只展开具名显示投影；格子和物理房间槽各自按项续做。
struct ClientSceneDescribeWork final:S::ClientStateNativePlanWork {
  const S::ClientStateInstallPlan &plan;
  std::shared_ptr<S::ClientStateMemory> account;
  std::shared_ptr<ClientSceneInstallPlan> out;
  S::ClientShadowStateRef world;
  std::shared_ptr<const ClientSceneInstallPlan> prior;
  S::ClientShadowStateRef prior_world;
  const S::PresentationBatchContext &c;
  ClientRoomLevelContext context;
  DisplayFields f;
  S::ClientStateVector<ClientRoomDescriptorShadowView> descriptors;
  S::ClientStateVector<ClientRoomConfigShadowView> configs;
  S::ClientStateVector<ClientRoomGridMember> grids;
  std::size_t value=0,grid=0,descriptor=0;
  ClientSceneDescribeWork(const S::ClientStatePreparedTarget &t,std::shared_ptr<const ClientSceneInstallPlan> previous):
    plan(*t.install),account(plan.Memory()),
    out(std::allocate_shared<ClientSceneInstallPlan>(S::ClientStateAllocator<ClientSceneInstallPlan>(account),t.install)),
    world(plan.Effective(S::BodyStream::World)),prior(std::move(previous)),
    prior_world(prior&&prior->State().Target().route==plan.Target().route?prior->State().Effective(S::BodyStream::World):nullptr),
    c(world->Context()),
    context{{c.scope.run,c.scope.floor,4},{c.scope.run,c.scope.room,7},c.source_revision,c.presentation_batch,c.member_epoch,c.resource_epoch,c.target_version,c.game_frame,c.layout_version},
    f(plan),descriptors(S::ClientStateAllocator<ClientRoomDescriptorShadowView>(account,memory::Pool::Workspace)),
    configs(S::ClientStateAllocator<ClientRoomConfigShadowView>(account,memory::Pool::Workspace)),
    grids(S::ClientStateAllocator<ClientRoomGridMember>(account,memory::Pool::Workspace)) {
    out->events_.resize(plan.Events().size());out->event_links_.resize(plan.Events().size());
    out->grids_.reserve(plan.Grids().size());grids.reserve(plan.Grids().size());descriptors.reserve(plan.Descriptors().size());
  }
  void Present(const S::ClientInstallValue &v){
    const auto &record=plan.Records()[v.record];
    if(v.layout==F::MusicRequestState::kLayout){
      using namespace F::MusicRequestState;
      out->music_={f.Number<std::uint64_t>(v,revision),f.Number<std::uint64_t>(v,generation),f.Number<std::uint64_t>(v,jingle_generation),
        f.Number<int>(v,id),f.Number<int>(v,queued),f.Number<int>(v,jingle_id),f.Number<float>(v,parameter),f.Number<float>(v,fade_rate),
        f.Number<float>(v,pitch),f.Number<float>(v,target_pitch),f.Number<std::uint8_t>(v,mode),f.Number<std::uint8_t>(v,layers),
        bool(f.Number<std::uint8_t>(v,paused)),bool(f.Number<std::uint8_t>(v,enabled)),bool(f.Number<std::uint8_t>(v,jingle_active))};
    }else if(v.layout==F::LoopSoundState::kLayout){
      using namespace F::LoopSoundState;
      const auto key=f.Resource(v,asset);
      out->sounds_.push_back({f.Number<std::uint64_t>(v,instance),f.Number<std::uint64_t>(v,generation),f.Number<int>(v,id),
        f.Number<float>(v,volume),f.Number<float>(v,pitch),f.Number<float>(v,pan),nullptr,f.Sample(v,key)});
      out->sound_links_.push_back({S::kClientPlanNone,key});
    }else if(v.layout==F::BossDisplayState::kLayout){
      using namespace F::BossDisplayState;
      auto &boss=out->boss_;
      boss.generation=f.Number<std::uint64_t>(v,generation);
      boss.ids[0]=f.Number<unsigned>(v,boss_id1);boss.ids[1]=f.Number<unsigned>(v,boss_id2);
      boss.index=f.Number<int>(v,room_index);boss.dimension=f.Number<int>(v,dimension);
    }else if(v.layout==F::BossDisplayPose::kLayout){
      using namespace F::BossDisplayPose;
      auto &boss=out->boss_;
      boss.poses.at(boss.count++)={f.Number<int>(v,key),f.Number<float>(v,frame),bool(f.Number<std::uint8_t>(v,playing))};
    }else if(v.layout&&record.event!=S::kClientPlanNone){
      auto &event=out->events_.at(record.event);auto &links=out->event_links_.at(record.event);
      event.kind=unsigned(plan.Events()[record.event].kind);
      switch(v.layout){
        case F::SfxPlayEvent::kLayout:{
          using namespace F::SfxPlayEvent;
          links.resource=f.Resource(v,asset);event.sound={0,0,f.Number<int>(v,id),f.Number<float>(v,volume),f.Number<float>(v,pitch),
            f.Number<float>(v,pan),nullptr,f.Sample(v,links.resource)};break;
        }
        case F::ItemBannerEvent::kLayout:
          links.player=f.Player(v,F::ItemBannerEvent::player);links.resource=f.Resource(v,F::ItemBannerEvent::configuration);
          event.title=f.Text(v,F::ItemBannerEvent::title);event.subtitle=f.Text(v,F::ItemBannerEvent::subtitle);break;
        case F::FortuneEvent::kLayout:
          event.category=f.Text(v,F::FortuneEvent::category);event.index=f.Number<int>(v,F::FortuneEvent::index);break;
        case F::FortuneLine::kLayout:{
          const auto i=plan.Paths()[v.path].step.element;
          event.lines.at(i)=f.Text(v,F::FortuneLine::text);event.line_count=(std::max)(event.line_count,unsigned(i+1));break;
        }
        case F::RoomTransitionEvent::kLayout:
          event.completed=f.Number<std::uint8_t>(v,F::RoomTransitionEvent::completed)!=0;
          event.final_room=f.Number<std::uint64_t>(v,F::RoomTransitionEvent::final_room);
          event.index=f.Number<int>(v,event.completed?F::RoomTransitionEvent::final_index:F::RoomTransitionEvent::requested_index);
          event.dimension=f.Number<int>(v,event.completed?F::RoomTransitionEvent::final_dimension:F::RoomTransitionEvent::dimension);
          event.animation=f.Number<int>(v,F::RoomTransitionEvent::animation);
          event.direction=f.Number<int>(v,F::RoomTransitionEvent::direction);break;
      }
    }
  }
  void AddGrid(const S::ClientInstallGrid &g){
    const auto r=Record(plan,g.object);const auto &id=r.Identity();
    grids.push_back({Key(c.scope.run,id),context.room,id.use,id.shape,r.Member(),id.layout,std::uint16_t(g.index)});
    if(r.Flat()){
      const auto clip=r.FlatValue("water_clip_flags");
      if(clip.size()!=4)Fail(S::StateReadiness::InvalidEncoding,"client_scene_plan_flat_grid_clip");
      out->grids_.push_back({id,g.index,S::FlatWord(clip)});
      return;
    }
    // 已验证的格子值只在同路由、同实例和全部实际块一致时沿用。
    if(prior_world)if(const auto before=prior_world->Find(id)){
      bool same=before->Mask()==r.Mask();
      for(unsigned b=0;b<32&&same;++b)if(r.Mask()&(std::uint32_t(1)<<b))same=r.SameBlock(*before,std::uint8_t(b));
      if(same){
        const auto values=prior->Grids();
        const auto old=std::lower_bound(values.begin(),values.end(),id,[](const auto &row,const auto &key){return row.member<key;});
        if(old!=values.end()&&old->member==id&&old->index==g.index){out->grids_.push_back(*old);return;}
      }
    }
    const auto description=ClientDescribeGrid(GridRecord(plan,g.object),32u<<20,account);
    out->grids_.push_back({id,g.index,description.WaterClipFlags()});
  }
  void AddDescriptor(const S::ClientInstallDescriptor &d){
    descriptors.push_back({d.source_slot,d.ordinary_ordinal,WorldRecord(plan,d.object,context)});
    const auto r=Record(plan,d.object);
    // 资源目标来自可信字段表，只收本目标实际引用的两份配置，不加载历史目录。
    for(const auto &field:r.Fields()) {
      const auto *definition=S::FindField(r.Identity().layout,field.field);S::BodySourceFieldInfo info;
      if(!field.present||!definition||definition->operation!=S::Operation::Resource||
          !S::DescribeBodySourceField(definition->layout,definition->index,info))continue;
      if(field.bytes.size()!=2)Fail(S::StateReadiness::InvalidEncoding,"client_scene_plan_config_index");
      std::uint16_t index;std::memcpy(&index,field.bytes.data(),2);if(index==S::kNullIndex)continue;
      if(std::any_of(configs.begin(),configs.end(),[&](const auto &v){return v.resource==index;}))continue;
      const auto *entry=S::FindResource(world->Resources()->Directory(),{info.resource_table,index});
      if(!entry)Fail(S::StateReadiness::MissingClosure,"client_scene_plan_config_missing");
      configs.push_back({c.resource_epoch,std::uint32_t(entry->payload.size()),index,263,c.layout_version,reinterpret_cast<const std::uint8_t *>(entry->payload.data())});
    }
  }
  bool Step(){
    if(value<plan.PresentationValues().size()){Present(plan.Value(plan.PresentationValues()[value++]));return false;}
    if(grid<plan.Grids().size()){AddGrid(plan.Grids()[grid++]);return false;}
    if(descriptor<plan.Descriptors().size()){AddDescriptor(plan.Descriptors()[descriptor++]);return false;}
    if(!out->music_.revision)Fail(S::StateReadiness::SourcePending,"binary_display_music_missing");
    const auto &w=plan.World();
    ClientRoomLevelShadowView source{WorldRecord(plan,w.room,context),WorldRecord(plan,w.level,context),descriptors,configs,grids,world};
    out->rooms_=ClientDescribeRoomLevel(source,32u<<20,account);
    const auto identity=[&](std::uint32_t i){return plan.Objects()[i].member.identity;};
    out->display_=ClientDescribeDisplayWorld(world,{identity(w.run),identity(w.level),identity(w.room),identity(w.sfx),identity(w.current_descriptor)},*out->rooms_,account);
    const auto meta=ClientDisplayMeta(out->display_);
    const auto &boss=out->boss_;
    if(bool(boss.generation)!=(meta.transition_mode==2&&meta.transition_boss!=0)||
       (boss.generation&&(boss.ids[0]!=unsigned(meta.transition_boss)||!boss.count||boss.poses[0].key!=-1))||
       (!boss.generation&&boss.count))Fail(S::StateReadiness::InvalidReference,"binary_boss_baseline_identity");
    for(unsigned n=1;n<boss.count;++n)if(boss.poses[n].key<=boss.poses[n-1].key)
      Fail(S::StateReadiness::InvalidReference,"binary_boss_baseline_pose_keys");
    return true;
  }
};
ClientSceneStatePreparation::ClientSceneStatePreparation(std::shared_ptr<const ClientSceneInstallReadiness> readiness):readiness_(std::move(readiness)) {}
bool ClientSceneStatePreparation::Available()const noexcept{return readiness_&&readiness_->Available();}
S::StateReadiness ClientSceneStatePreparation::Inspect(const S::ClientStatePreparedTarget &target,const S::ClientStateTables &tables)const {
  if(!Available())return S::StateReadiness::Lifecycle;
  const auto plan=std::dynamic_pointer_cast<const ClientSceneInstallPlan>(target.described);
  if(!plan||&plan->State()!=target.install.get())return S::StateReadiness::InvalidEncoding;
  return readiness_->Inspect(*plan,tables);
}
std::shared_ptr<const S::ClientStateNativePlan> ClientSceneStatePreparation::PrepareStep(const S::ClientStatePreparedTarget &t,
    std::shared_ptr<S::ClientStateNativePlanWork> &work,std::chrono::steady_clock::time_point stop)const {
  if(!t.install||!t.candidate||!t.install->Describes(t.candidate)||t.install->Target().route!=t.candidate->Target().route||
      t.install->Effective(S::BodyStream::World)!=t.effective[0]||t.install->Effective(S::BodyStream::Body)!=t.effective[1])
    Fail(S::StateReadiness::Lifecycle,"client_scene_plan_target_mismatch");
  const auto account=t.install->Memory();
  if(!t.candidate->State(S::BodyStream::World))
    return std::allocate_shared<ClientSceneInstallPlan>(S::ClientStateAllocator<ClientSceneInstallPlan>(account),t.install);
  S::ClientStateWorkspace workspace;
  try{
    if(!work)work=std::allocate_shared<ClientSceneDescribeWork>(S::ClientStateAllocator<ClientSceneDescribeWork>(account),t,last_world_.lock());
    auto &w=static_cast<ClientSceneDescribeWork &>(*work);
    do{if(w.Step()){auto result=w.out;last_world_=result;work.reset();return result;}}while(std::chrono::steady_clock::now()<stop);
    return {};
  }catch(const S::Gap &){work.reset();Fail(S::StateReadiness::InvalidReference,"client_scene_plan_domain_description");}
   catch(...){work.reset();throw;}
}
} // namespace cs::game
