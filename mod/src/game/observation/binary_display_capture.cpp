#include "game/observation/binary_display_capture.h"
#include "game/mods/mod_native.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#ifdef ISAACCS_E2E
#include "replica/stream_e2e.h"
#include "runtime.h"
#endif

namespace cs::game {
namespace {
using namespace state;
using namespace world::binary;
#include "state_host_fields.inc"
namespace F=host_field;
template<class T> auto Bytes(const T &v){return std::as_bytes(std::span(&v,1));}
}
// arena只存自有字节和数字索引；reset不释放内存，节点在同步执行器返回后即可复用。
struct BinaryDisplayCapture::Impl {
  std::thread::id thread;
  memory::Lease memory;
  std::vector<DisplayValueNode> nodes;
  std::vector<DisplayFieldValue> fields;
  std::vector<std::byte> bytes;
  std::vector<CaptureLink> links;
  std::uint32_t node_count=0,field_count=0,byte_count=0,link_count=0;
  BinaryDisplayRead read;
  CaptureWitness witness;
  EntityFacts facts;
  CaptureDiagnostic *diagnostic=nullptr;
  struct MusicState {
    std::uint64_t revision=1,generation=0,jingle_generation=0;
    std::int32_t id=0,queued=0,jingle=0;float parameter=0,fade=0;
    std::uint8_t mode=0,layers=0;bool paused=false,known=false,jingle_active=false;
  } music;
  struct Loop {CaptureLink asset;std::uint64_t instance=0,generation=0;std::uint32_t stream=0,frame=0;int id=0,ambient=-1;
    float volume=0,pitch=1,pan=0;bool active=false;};
  std::array<Loop,512> loops{};std::uint64_t loop_revision=1,next_loop=0;
  struct Overlay {std::uint32_t address=0;std::uint64_t generation=0;std::int32_t controller=-1;};
  std::array<Overlay,65> overlays{};std::uint64_t next_overlay=0;
  // Start之后不再保存Player地址；完成关联可跨房，身份只能取本次接受时的稳定值。
  struct Transition {std::array<std::uint32_t,5> args{};CaptureReferenceEntry player;std::uint64_t sequence=0;bool active=false;};
  Transition transition;std::uint64_t next_transition=0;
#ifdef ISAACCS_E2E
  bool transition_observed=false;
#endif
  // 演出身份跨房和事件退休保留，普通world每次从真实RoomTransition采当前帧。
  struct Boss {std::uint32_t owner=0,first=0,second=0;std::int32_t room=0,dimension=0;std::uint64_t generation=0;};
  Boss boss;std::uint64_t next_boss=0;
  struct Map {std::uint32_t owner=0,descriptor=0,frame=0,focus=0;bool known=false;};
  std::array<Map,2> maps{};
  ProductionCache events;
  CaptureBuffer event_scratch;
  struct Event {CaptureSubject subject;std::uint16_t layout=0;std::uint64_t sequence=0;std::uint32_t frame=0,bytes=0;bool cosmetic=false;StateScope scope;};
  std::array<Event,512> pending{};unsigned event_count=0;std::uint32_t event_bytes=0;
  std::uint64_t next_event=0,transferred_event=0,dropped=0;bool gap=false;
  CaptureDiagnostic gap_failure;
  std::array<BinarySfxIntent,64> intentions{};unsigned intention_count=0;
  std::uint32_t event_frame=0;
  explicit Impl(std::thread::id owner):thread(owner),events(owner,2u<<20,512),event_scratch({1,1024,2048,160u<<10,16392}) {
    constexpr unsigned node_cap=4800,field_cap=32768,byte_cap=768u<<10,link_cap=8192;
    memory=memory::Reserve(memory::Pool::Values,sizeof(*this)+std::uint64_t(node_cap)*sizeof(DisplayValueNode)+
      std::uint64_t(field_cap)*sizeof(DisplayFieldValue)+byte_cap+std::uint64_t(link_cap)*sizeof(CaptureLink),"host_display_capture");
    nodes.resize(node_cap);fields.resize(field_cap);bytes.resize(byte_cap);links.resize(link_cap);
  }
  bool Fail(CaptureFailure reason,std::uint16_t layout=237,std::source_location origin=std::source_location::current()) noexcept {
    return FailCapture(*diagnostic,reason,{facts.subject,facts.address,0,layout},kNullIndex,{},origin);
  }
  bool GapFailure() noexcept {
    if(gap_failure.reason!=CaptureFailure::None){*diagnostic=gap_failure;return false;}
    return Fail(CaptureFailure::Incomplete);
  }
  void MarkGap(CaptureFailure reason,std::uint16_t layout,
               std::source_location origin=std::source_location::current()) noexcept {
    gap=true;
    if(gap_failure.reason==CaptureFailure::None)FailCapture(gap_failure,reason,{{},0,0,layout},kNullIndex,{},origin);
  }
  bool Read(std::uint64_t address,std::span<std::byte> out) noexcept {
    return (read.read&&address&&address<=UINT32_MAX&&out.size()<=(std::uint64_t{1}<<32)-address&&
      read.read(read.context,address,out))||Fail(CaptureFailure::ReadFailed);
  }
  template<class T> bool Value(std::uint64_t p,T &v) noexcept{return Read(p,std::as_writable_bytes(std::span(&v,1)));}
  void Begin(const CaptureWitness &w,EntityFacts f,BinaryDisplayRead r,CaptureDiagnostic &d) noexcept {
    witness=w;facts=f;read=r;diagnostic=&d;event_frame=w.game_frame;node_count=field_count=byte_count=link_count=0;
  }
  std::uint32_t Node(std::uint16_t layout,unsigned count,std::uint32_t native=0) noexcept {
    if(node_count==nodes.size()||count>fields.size()-field_count){Fail(CaptureFailure::ValueBudget,layout);return UINT32_MAX;}
    const auto index=node_count++;auto values=std::span(fields).subspan(field_count,count);field_count+=count;
    std::fill(values.begin(),values.end(),DisplayFieldValue{});nodes[index]={layout,native,values};return index;
  }
  DisplayFieldValue *Field(unsigned node,std::uint16_t field) noexcept {
    if(node>=node_count)return nullptr;auto &n=nodes[node];const auto first=std::size_t(n.fields.data()-fields.data());
    for(std::size_t i=0;i<n.fields.size();++i)if(fields[first+i].scalar!=ScalarKind::None||!fields[first+i].links.empty()||fields[first+i].first_child){
      if(fields[first+i].field==field){Fail(CaptureFailure::DuplicateValue,n.layout);return nullptr;}}
    for(std::size_t i=0;i<n.fields.size();++i){auto &v=fields[first+i];if(v.scalar==ScalarKind::None&&v.links.empty()&&!v.first_child){v.field=field;return &v;}}
    Fail(CaptureFailure::ValueBudget,n.layout);return nullptr;
  }
  bool Scalar(unsigned node,std::uint16_t field,ScalarKind kind,std::span<const std::byte> value,std::uint32_t count=1) noexcept {
    auto *v=Field(node,field);if(!v||value.size()>bytes.size()-byte_count)return Fail(CaptureFailure::ValueBudget);
    auto out=std::span(bytes).subspan(byte_count,value.size());std::copy(value.begin(),value.end(),out.begin());byte_count+=std::uint32_t(value.size());
    v->scalar=kind;v->count=count;v->bytes=out;return true;
  }
  template<class T> bool Scalar(unsigned node,std::uint16_t field,ScalarKind kind,const T &value) noexcept {return Scalar(node,field,kind,Bytes(value));}
  bool Link(unsigned node,std::uint16_t field,CaptureLink value) noexcept {
    auto *v=Field(node,field);if(!v||link_count==links.size()||value.kind==CapturedReferenceKind::Invalid)return Fail(CaptureFailure::ReferencePending);
    links[link_count]=value;v->links=std::span(links).subspan(link_count++,1);return true;
  }
  bool Children(unsigned node,std::uint16_t field,unsigned first,unsigned count) noexcept {
    auto *v=Field(node,field);if(!v)return false;v->first_child=first;v->count=count;return true;
  }
  // 有界读取J273 string／C串，复制的只有有效UTF8字节；头与内容前后变化均失败。
  bool Text(unsigned node,std::uint16_t field,std::uint64_t address,bool cstring=false,unsigned limit=4096,bool path=false) noexcept {
    std::array<std::byte,4096> text{};std::uint32_t count=0;
    if(!address)return Scalar(node,field,ScalarKind::U8,std::span<const std::byte>{},0);
    if(cstring){for(;count<limit;++count){if(!Read(address+count,std::span(text).subspan(count,1)))return false;if(text[count]==std::byte{})break;}
      if(count==limit)return Fail(CaptureFailure::ValueBudget);}
    else {std::array<std::uint32_t,2> header{},after{};if(!Value(address+16,header)||header[0]>limit||header[0]>header[1]||
        (header[1]<16&&header[1]!=15))return Fail(CaptureFailure::InvalidValue);count=header[0];
      std::uint32_t pointer=std::uint32_t(address);if(header[1]>=16&&(!Value(address,pointer)||!pointer))return false;
      if(count&&!Read(pointer,std::span(text).first(count)))return false;
      if(!Value(address+16,after)||after!=header)return Fail(CaptureFailure::SourceChanged);}
    if(path)for(auto &ch:std::span(text).first(count))if(ch==std::byte{'\\'})ch=std::byte{'/'};
    return Scalar(node,field,ScalarKind::U8,std::span<const std::byte>(text).first(count),count);
  }
  bool Bump(std::uint64_t &value) noexcept {if(value==UINT64_MAX)return Fail(CaptureFailure::SequenceExhausted);++value;return true;}
  bool MusicSnapshot(std::uint32_t node,std::uint32_t manager) noexcept;
  bool InjectionSnapshot(std::uint32_t,const BinaryDisplayInputs &,EntitySource &) noexcept;
  bool LoopSnapshot(std::uint32_t) noexcept;
  bool OverlaySnapshot(std::uint32_t) noexcept;
  bool ProceduralSnapshot(std::uint32_t,EntitySource &) noexcept;
  bool BossSnapshot(std::uint32_t) noexcept;
  std::uint64_t cutscene_serial=0;
  bool EventStore(std::uint16_t,bool,EntitySource &,std::span<const CaptureReferenceEntry>) noexcept;
  bool SfxIntent(const BinarySfxIntent &,CaptureLink,EntitySource &,std::span<const CaptureReferenceEntry>) noexcept;
  void Drop(unsigned,bool discarded=true) noexcept;
};
// 仅完整当前集合进入world；后台／设备状态不作为逻辑Stop来源。
bool BinaryDisplayCapture::Impl::LoopSnapshot(std::uint32_t node) noexcept {
  const auto first=node_count;unsigned count=0;
  for(const auto &loop:loops)if(loop.active){const auto n=Node(F::LoopSoundState::kLayout,8);if(n==UINT32_MAX)return false;++count;
    if(!Scalar(n,F::LoopSoundState::instance,ScalarKind::U64,loop.instance)||!Scalar(n,F::LoopSoundState::generation,ScalarKind::U64,loop.generation)||
       !Scalar(n,F::LoopSoundState::id,ScalarKind::I32,loop.id)||!Scalar(n,F::LoopSoundState::volume,ScalarKind::F32,loop.volume)||
       !Scalar(n,F::LoopSoundState::pitch,ScalarKind::F32,loop.pitch)||!Scalar(n,F::LoopSoundState::pan,ScalarKind::F32,loop.pan)||
       !Scalar(n,F::LoopSoundState::start_frame,ScalarKind::U32,loop.frame)||!Link(n,F::LoopSoundState::asset,loop.asset))return false;}
  return Scalar(node,F::LoopSoundSet::revision,ScalarKind::U64,loop_revision)&&Children(node,F::LoopSoundSet::voices,first,count);
}
bool BinaryDisplayCapture::Impl::MusicSnapshot(std::uint32_t node,std::uint32_t manager) noexcept {
  const auto owner=std::uint64_t(manager)+0x29df8;std::int32_t current=0,queued=0;float pitch=0,target=0;std::uint8_t enabled=0;
  if(!Value(owner+0x30c,current)||!Value(owner+0x310,queued)||!Value(owner+0x314,pitch)||!Value(owner+0x318,target)||!Value(owner+0x328,enabled)||
     !std::isfinite(pitch)||!std::isfinite(target)||enabled>1)return Fail(CaptureFailure::InvalidValue,439);
  if(!music.known){if(current)return Fail(CaptureFailure::MissingQualification,439);music.known=true;}
  std::uint32_t selected=0,layers=0;std::uint8_t jingle_active=0;
  if(!Value(owner,selected)||selected>1||!Value(owner+std::uint64_t(selected)*388+0x170,layers)||!Value(owner+0x354,jingle_active)||jingle_active>1)
    return Fail(CaptureFailure::InvalidValue,439);
  const auto mask=std::uint8_t(layers&63);if(mask!=music.layers||bool(jingle_active)!=music.jingle_active){music.layers=mask;music.jingle_active=jingle_active!=0;if(!Bump(music.revision))return false;}
  // queued的正常消费可能发生在音乐tick内部；Play观察保留代次，后态负责明确清队列。
  if(current!=music.id)return Fail(CaptureFailure::SourceChanged,439);
  if(queued!=music.queued){music.queued=queued;if(!Bump(music.revision))return false;}
  const auto u8=[&](std::uint16_t f,unsigned v){const auto x=std::uint8_t(v);return Scalar(node,f,ScalarKind::U8,x);};
  return Scalar(node,F::MusicRequestState::revision,ScalarKind::U64,music.revision)&&Scalar(node,F::MusicRequestState::generation,ScalarKind::U64,music.generation)&&
    u8(F::MusicRequestState::mode,music.mode)&&Scalar(node,F::MusicRequestState::id,ScalarKind::I32,music.id)&&
    Scalar(node,F::MusicRequestState::parameter,ScalarKind::F32,music.parameter)&&Scalar(node,F::MusicRequestState::fade_rate,ScalarKind::F32,music.fade)&&
    Scalar(node,F::MusicRequestState::queued,ScalarKind::I32,music.queued)&&Scalar(node,F::MusicRequestState::pitch,ScalarKind::F32,pitch)&&
    Scalar(node,F::MusicRequestState::target_pitch,ScalarKind::F32,target)&&u8(F::MusicRequestState::paused,music.paused)&&
    u8(F::MusicRequestState::enabled,enabled)&&u8(F::MusicRequestState::layers,music.layers)&&Scalar(node,F::MusicRequestState::jingle_id,ScalarKind::I32,music.jingle)&&
    Scalar(node,F::MusicRequestState::jingle_generation,ScalarKind::U64,music.jingle_generation)&&u8(F::MusicRequestState::jingle_active,music.jingle_active);
}
bool BinaryDisplayCapture::Impl::InjectionSnapshot(std::uint32_t node,const BinaryDisplayInputs &input,EntitySource &source) noexcept {
  std::array<std::int32_t,15> pills{};std::array<std::uint8_t,15> identified{};std::array<float,2> current{};
  const auto game=std::uint64_t(facts.address);bool eligible=false;
  if(!source.HudDealEligibility(witness,facts,eligible,*diagnostic)||!Value(game+0x1aee4,pills)||!Value(game+0x1af20,identified)||
     !Value(std::uint64_t(facts.scene->room)+0x7460,current))return false;
  if(std::any_of(identified.begin(),identified.end(),[](auto v){return v>1;})||!std::isfinite(current[0])||!std::isfinite(current[1]))return Fail(CaptureFailure::InvalidValue,438);
  std::array<std::int32_t,2> focus{-1,-1};std::uint32_t challenge=0;if(!Value(game+0x25afc,challenge))return false;
  if(challenge==32){for(const auto &map:maps)if(map.known&&map.descriptor==facts.scene->current_descriptor){
      if(map.owner==game+0x2545c)focus[0]=std::int32_t(map.focus);if(map.owner==game+0x255bc)focus[1]=std::int32_t(map.focus);}
    if(focus[0]<0||focus[1]<0)return Fail(CaptureFailure::MissingQualification,438);}
  // 从同一正常采集窗口读构建输入。Poop只取有效相位与颜色，粒子和贴图由客机生成。
  const auto room=std::uint64_t(facts.scene->room);
  std::int32_t wall_style=0,entry=0,start=0,door=-1,previous=0,return_room=0,poop=0;
  std::uint32_t phase_flags=0,stage=0,stage_type=0;
  std::uint8_t fx_loaded=0;
  std::array<float,2> return_position{};
  std::array<float,11> poop_color{};
  if(!Value(room+0x7068,wall_style)||!Value(game,stage)||!Value(game+4,stage_type)||
     !Value(game+0x25ac4,phase_flags)||!Value(game+0x182d0,start)||!Value(game+0x18304,entry)||
     !Value(game+0x18308,previous)||!Value(game+0x1831c,door)||!Value(game+0x18328,return_room)||
     !Value(room+0x1620,fx_loaded))return false;
  const auto tutorial=std::uint8_t(stage==1&&stage_type!=4&&stage_type!=5&&entry==((phase_flags&0x10000u)?84:start));
  const auto graphics_flags=std::uint8_t((phase_flags>>14)&3u);
  const auto return_focus=std::uint8_t(entry>=0&&entry==return_room);
  if(return_focus&&!Value(game+0x18320,return_position))return false;
  if(fx_loaded){
    if(!Value(room+0x164c,poop))return false;
    if(poop>0&&!Value(room+0x169c,poop_color))return false;
    if(poop<0)poop=0;
  }
  const auto blocked=std::uint8_t(input.progress_blocked),qualified=std::uint8_t(eligible);
  return Scalar(node,F::DisplayInjectionState::devil_total,ScalarKind::F32,input.devil_total)&&
    Scalar(node,F::DisplayInjectionState::angel_share,ScalarKind::F32,input.angel_share)&&Scalar(node,F::DisplayInjectionState::deal_qualified,ScalarKind::U8,qualified)&&
    Scalar(node,F::DisplayInjectionState::planetarium,ScalarKind::F32,input.planetarium)&&Scalar(node,F::DisplayInjectionState::progress_blocked,ScalarKind::U8,blocked)&&
    Scalar(node,F::DisplayInjectionState::pill_effects,ScalarKind::I32,Bytes(pills),15)&&Scalar(node,F::DisplayInjectionState::pill_identified,ScalarKind::U8,Bytes(identified),15)&&
    Scalar(node,F::DisplayInjectionState::water_current,ScalarKind::F32,Bytes(current),2)&&Scalar(node,F::DisplayInjectionState::map_focus,ScalarKind::I32,focus[0])&&
    Scalar(node,F::DisplayInjectionState::map_focus_large,ScalarKind::I32,focus[1])&&
    Scalar(node,F::DisplayInjectionState::backdrop_wall_style,ScalarKind::I32,wall_style)&&
    Scalar(node,F::DisplayInjectionState::backdrop_tutorial,ScalarKind::U8,tutorial)&&
    Scalar(node,F::DisplayInjectionState::room_graphics_flags,ScalarKind::U8,graphics_flags)&&
    Scalar(node,F::DisplayInjectionState::camera_entry_door,ScalarKind::I32,door)&&
    Scalar(node,F::DisplayInjectionState::camera_previous_entry,ScalarKind::I32,previous)&&
    Scalar(node,F::DisplayInjectionState::camera_return_focus,ScalarKind::U8,return_focus)&&
    Scalar(node,F::DisplayInjectionState::camera_return_position,ScalarKind::F32,Bytes(return_position),2)&&
    Scalar(node,F::DisplayInjectionState::poop_fx_countdown,ScalarKind::I32,poop)&&
    Scalar(node,F::DisplayInjectionState::poop_fx_color,ScalarKind::F32,Bytes(poop_color),11);
}
bool BinaryDisplayCapture::Impl::OverlaySnapshot(std::uint32_t node) noexcept {
  const auto global=facts.address+0x1c034;bool found=false;for(const auto &owner:overlays)found|=owner.address==global;
  if(!found){auto free=std::find_if(overlays.begin(),overlays.end(),[](auto &v){return !v.address;});if(free==overlays.end())return Fail(CaptureFailure::ValueBudget,442);free->address=global;}
  const auto first=node_count;unsigned count=0;
  for(auto &owner:overlays)if(owner.address){std::int32_t state=0;std::uint32_t player=0;
    if(!Value(owner.address,state)||state<0||state>2||!Value(std::uint64_t(owner.address)+0x1148,player))return Fail(CaptureFailure::InvalidValue,442);
    if(owner.controller>=0&&(!state||!player)){owner={};continue;}
    if(state&&!owner.generation)return Fail(CaptureFailure::MissingQualification,442);
    const auto n=Node(F::OverlayDisplayState::kLayout,3,owner.address);if(n==UINT32_MAX)return false;++count;
    const auto ref=player?read.reference(read.context,player):CaptureLink{CapturedReferenceKind::Null,0};
    if(!Scalar(n,F::OverlayDisplayState::generation,ScalarKind::U64,owner.generation)||!Scalar(n,F::OverlayDisplayState::controller,ScalarKind::I32,owner.controller)||
       !Link(n,F::OverlayDisplayState::player,ref))return false;}
  return Children(node,F::OverlayDisplaySet::owners,first,count);
}
// TMTRAINER只读现有程序配置与键／图形选择；不读取effects，不生成或复制图集指针。
bool BinaryDisplayCapture::Impl::ProceduralSnapshot(std::uint32_t node,EntitySource &source) noexcept {
  std::array<std::uint32_t,3> header{},after{};const auto at=std::uint64_t(facts.address)+0x669bc;
  if(!Value(at,header)||header[0]>header[1]||header[1]>header[2]||(header[1]-header[0])%4)return Fail(CaptureFailure::InvalidValue,445);
  const auto count=(header[1]-header[0])/4,first=node_count;if(count>1024||count>nodes.size()-node_count)return Fail(CaptureFailure::ValueBudget,445);
  // 先占满连续子节点，再填字符串；container元素号永远按~id保持完整顺序。
  for(unsigned i=0;i<count;++i)if(Node(F::ProceduralDisplayDefinition::kLayout,5)==UINT32_MAX)return false;
  for(unsigned i=0;i<count;++i){std::uint32_t item=0,config=0;std::int32_t id=0;CaptureLink resource;
    if(!Value(std::uint64_t(header[0])+i*4,item)||!item||!Value(item,config)||!config||!Value(std::uint64_t(item)+4,id)||id!=~std::int32_t(i)||
       !source.DisplayItemResource(witness,facts,config,resource,*diagnostic))return Fail(CaptureFailure::ResourcePending,444);
    const auto n=first+i;if(!Scalar(n,F::ProceduralDisplayDefinition::id,ScalarKind::I32,id)||!Link(n,F::ProceduralDisplayDefinition::configuration,resource)||
       !Text(n,F::ProceduralDisplayDefinition::name,std::uint64_t(config)+8)||!Text(n,F::ProceduralDisplayDefinition::description,std::uint64_t(config)+0x20)||
       !Text(n,F::ProceduralDisplayDefinition::gfx,std::uint64_t(config)+0x38,false,4096,true))return false;
  }
  if(!Value(at,after)||after!=header)return Fail(CaptureFailure::SourceChanged,445);
  using namespace F::ProceduralDisplaySet;
  const auto atlas_link=[&](unsigned offset,unsigned field){
    CaptureLink resource{CapturedReferenceKind::Null,0};
    if(count){
      std::array<std::uint32_t,2> strong{};
      if(!Value(at+offset,strong)||!strong[0]||!strong[1]||
         !source.DisplayProceduralAtlas(witness,facts,strong[0],resource,*diagnostic))return false;
    }
    return Link(node,field,resource);
  };
  return Children(node,definitions,first,count)&&atlas_link(0xc,atlas)&&atlas_link(0x14,small_atlas);
}
BinaryDisplayCapture::BinaryDisplayCapture(std::thread::id owner):impl_(std::make_unique<Impl>(owner)){}
BinaryDisplayCapture::~BinaryDisplayCapture()=default;
// 正常原生调用返回后才接收请求；无效id、节流失败与缺资源不伪造成一次播放。
bool BinaryDisplayCapture::Observe(BinaryProductionKind kind,std::uint32_t object,std::span<const std::uint32_t> args,const CaptureWitness &w,
    EntityFacts facts,EntitySource &source,BinaryDisplayRead read,std::span<const CaptureReferenceEntry> references,CaptureDiagnostic &d,
    const CaptureReferenceEntry *event_player) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread||args.size()<8)return FailCapture(d,CaptureFailure::InvalidWindow,{facts.subject});
  s.Begin(w,facts,read,d);using K=BinaryProductionKind;
  const auto signed_arg=[&](unsigned n){return std::bit_cast<std::int32_t>(args[n]);};
  const auto float_arg=[&](unsigned n){return std::bit_cast<float>(args[n]);};
  if(kind>=K::MusicPlay&&kind<=K::MusicLayerOff){
    if(object!=std::uint64_t(facts.manager)+0x29df8)return s.Fail(CaptureFailure::SourceChanged,439);
    std::int32_t current=0,queued=0;if(!s.Value(std::uint64_t(object)+0x30c,current)||!s.Value(std::uint64_t(object)+0x310,queued))return false;
    const bool select=kind==K::MusicPlay||kind==K::MusicCrossfade||kind==K::MusicFadein||kind==K::MusicJingle;
    if(select||kind==K::MusicQueue){std::array<std::uint32_t,3> entries{};std::uint32_t configured=0;
      if(!s.Value(std::uint64_t(object)+0x31c,entries)||entries[0]>entries[1]||entries[1]>entries[2]||(entries[1]-entries[0])%96)return s.Fail(CaptureFailure::InvalidValue,439);
      if(signed_arg(0)<0||args[0]>=(entries[1]-entries[0])/96)return true;
      if(select&&kind!=K::MusicCrossfade){if(!s.Value(std::uint64_t(entries[0])+args[0]*96u+68,configured))return false;if(!configured)return true;}}
    if(select&&kind!=K::MusicJingle&&current!=signed_arg(0))return true;
    if(kind==K::MusicJingle&&(args[5]==52||args[5]==118))return true;
    if(kind==K::MusicQueue&&queued!=signed_arg(0))return true;
    // 普通生产窗与生命周期后绑定共用同一状态变换，避免Fadein／Queue语义分叉。
    BinaryMusicIntent intent;intent.kind=kind;std::copy_n(args.begin(),5,intent.args.begin());
    intent.previous=signed_arg(5);intent.current=current;intent.queued=queued;BootstrapMusic(intent);
    if(s.gap)return s.Fail(CaptureFailure::InvalidValue,439);return true;
  }
  if(kind==K::OverlayShow){if(args[0]>46)return true;std::int32_t id=0,state=0;
    if(!s.Value(object,state)||!s.Value(std::uint64_t(object)+4,id))return false;if(id!=signed_arg(0)||state<1||state>2)return true;
    std::int32_t controller=-1;
    if(object!=facts.address+0x1c034){
      if(!args[2])return s.Fail(CaptureFailure::MissingQualification,442);
      if(!s.Value(std::uint64_t(args[2])+0x160c,controller))return false;
      if(controller<0||controller>=64)return s.Fail(CaptureFailure::InvalidValue,442);
    }
    OverlayShown(object,controller);
    return !s.gap||s.Fail(CaptureFailure::ValueBudget,442);
  }
  if(kind==K::TransitionStart){std::int32_t mode=0,index=0,animation=0,dimension=0;
    // J273 Start：换层活跃且animation!=15时没有接受请求。
    if(args[5]&&signed_arg(2)!=15)return true;
    if(!s.Value(object,mode)||!s.Value(std::uint64_t(object)+0x14,index)||!s.Value(std::uint64_t(object)+0x1c,animation)||!s.Value(std::uint64_t(object)+0x18,dimension))return false;
    if(!mode||index!=signed_arg(0)||animation!=signed_arg(2))return true;
    const auto serial=std::uint64_t(args[6])|(std::uint64_t(args[7])<<32);
    if(!serial||serial>s.next_transition)return s.Fail(CaptureFailure::MissingQualification,453);
    CaptureReferenceEntry player;
    // Start请求的Player参数只标识事件归属；生命周期可撤销字段读取资格，
    // 不因此丢弃已在入口取得、返回时复核的存活身份。此处不读Player字段。
    if(args[3]){
      if(!event_player||event_player->space!=1||!event_player->local||!event_player->use)
        return s.Fail(CaptureFailure::ReferencePending,453);
      player=*event_player;
    }
    std::copy_n(args.begin(),5,s.transition.args.begin());
    s.transition.args[3]=0;
    // 原生 Start 已把负维度解析为当前维度；开始和完成事件使用同一已接受值。
    s.transition.args[4]=static_cast<std::uint32_t>(dimension);
    s.transition.player=player;s.transition.sequence=serial;s.transition.active=true;
    // 接受事件仅关联当前旧房；实际目标由ChangeRoom正常完成后另发同序号关联。
  }else if(kind==K::TransitionFinish){if(!s.transition.active)return true;}
  std::uint16_t layout=0;bool cosmetic=true;
  if(kind==K::ItemBanner||kind==K::CustomBanner){layout=449;const auto root=s.Node(layout,7);if(root==UINT32_MAX)return false;
    CaptureLink config{CapturedReferenceKind::Null,0};if(kind==K::ItemBanner&&!source.DisplayItemResource(w,facts,args[1],config,d))return false;
    // 拾取期间角色状态可能变更；横幅只登记入口保存、正常返回时复核的同一玩家身份。
    CaptureLink player{CapturedReferenceKind::Null,0};
    if(args[0]){
      if(!event_player||event_player->space!=1||!read.stable_reference)return s.Fail(CaptureFailure::ReferencePending,layout);
      player=read.stable_reference(read.context,*event_player);
      if(player.kind==CapturedReferenceKind::Invalid)return s.Fail(CaptureFailure::ReferencePending,layout);
    }
    if(!s.Link(root,F::ItemBannerEvent::player,player)||
       !s.Link(root,F::ItemBannerEvent::configuration,config))return false;
    const auto style=std::uint8_t(kind==K::ItemBanner?0:1);
    if(!s.Scalar(root,F::ItemBannerEvent::style,ScalarKind::U8,style))return false;
    if(kind==K::ItemBanner){if(!s.Text(root,F::ItemBannerEvent::title_key,std::uint64_t(args[1])+8)||!s.Text(root,F::ItemBannerEvent::subtitle_key,std::uint64_t(args[1])+0x20))return false;
      // 最终UTF16消费点的UTF8副本来自正常LoadText，涵盖Manuel／seed文字变换及多人mini text。
      if(!args[6])return s.Fail(CaptureFailure::MissingQualification,449);
      if(!s.Text(root,F::ItemBannerEvent::title,args[6],true)||!s.Text(root,F::ItemBannerEvent::subtitle,args[7],true))return false;
    }else {if(!s.Text(root,F::ItemBannerEvent::title_key,0,true)||!s.Text(root,F::ItemBannerEvent::subtitle_key,0,true)||
       !s.Text(root,F::ItemBannerEvent::title,args[1],true)||!s.Text(root,F::ItemBannerEvent::subtitle,args[2],true))return false;}
  }else if(kind==K::Fortune){layout=451;const auto root=s.Node(layout,3);if(root==UINT32_MAX)return false;
    if(!s.Text(root,F::FortuneEvent::category,args[1],true,128)||!s.Scalar(root,F::FortuneEvent::index,ScalarKind::I32,signed_arg(2)))return false;
    std::array<std::uint32_t,2> header{},sentinel{};if(!s.Value(args[0],header)||!header[0]||header[1]>32||!s.Value(header[0],sentinel))return s.Fail(CaptureFailure::InvalidValue,451);
    const auto first=s.node_count;for(unsigned i=0;i<header[1];++i)if(s.Node(450,1)==UINT32_MAX)return false;
    auto node=sentinel[0],previous=header[0];
    for(unsigned i=0;i<header[1];++i){std::array<std::uint32_t,2> next{};if(!node||node==header[0]||!s.Value(node,next)||next[1]!=previous||
        !s.Text(first+i,F::FortuneLine::text,std::uint64_t(node)+8))return s.Fail(CaptureFailure::SourceChanged,451);previous=node;node=next[0];}
    std::array<std::uint32_t,2> after{};if(node!=header[0]||previous!=sentinel[1]||!s.Value(args[0],after)||after!=header)return s.Fail(CaptureFailure::SourceChanged,451);
    if(!s.Children(root,F::FortuneEvent::lines,first,header[1]))return false;
  }else if(kind==K::BossIntro){layout=452;cosmetic=false;const auto root=s.Node(layout,5);if(root==UINT32_MAX)return false;
    std::int32_t index=0,dimension=0;if(!s.Value(std::uint64_t(object)+0x14,index)||!s.Value(std::uint64_t(object)+0x18,dimension))return false;
    BossShown(object,args[0],args[1],index,dimension);
    if(!s.Scalar(root,F::BossIntroEvent::boss_id1,ScalarKind::U32,args[0])||!s.Scalar(root,F::BossIntroEvent::boss_id2,ScalarKind::U32,args[1])||
       !s.Scalar(root,F::BossIntroEvent::room_index,ScalarKind::I32,index)||!s.Scalar(root,F::BossIntroEvent::dimension,ScalarKind::I32,dimension)||
       !s.Scalar(root,F::BossIntroEvent::transition,ScalarKind::U64,args[6]||args[7]?(std::uint64_t(args[6])|(std::uint64_t(args[7])<<32)):s.transition.sequence))return false;
  }else if(kind==K::TransitionStart||kind==K::TransitionFinish){layout=453;cosmetic=false;const auto root=s.Node(layout,10);if(root==UINT32_MAX)return false;
    const auto &request=s.transition.args;std::uint32_t descriptor=0;std::int32_t final_index=0,dimension=0;
    if(!s.Value(std::uint64_t(facts.address)+0x18300,descriptor)||!descriptor||!s.Value(std::uint64_t(descriptor)+4,descriptor)||!descriptor||
       !s.Value(descriptor,final_index)||!s.Value(std::uint64_t(descriptor)+12,dimension))return false;
    const auto done=std::uint8_t(kind==K::TransitionFinish);const auto room=done?w.scope.room:std::uint64_t{};
    if(!s.Scalar(root,F::RoomTransitionEvent::transition,ScalarKind::U64,s.transition.sequence)||
       !s.Scalar(root,F::RoomTransitionEvent::requested_index,ScalarKind::I32,request[0])||!s.Scalar(root,F::RoomTransitionEvent::direction,ScalarKind::I32,request[1])||
       !s.Scalar(root,F::RoomTransitionEvent::animation,ScalarKind::I32,request[2])||!s.Link(root,F::RoomTransitionEvent::player,s.transition.player.local?
         (read.stable_reference?read.stable_reference(read.context,s.transition.player):CaptureLink{CapturedReferenceKind::Invalid,0}):CaptureLink{CapturedReferenceKind::Null,0})||
       !s.Scalar(root,F::RoomTransitionEvent::dimension,ScalarKind::I32,request[4])||!s.Scalar(root,F::RoomTransitionEvent::final_room,ScalarKind::U64,room)||
       !s.Scalar(root,F::RoomTransitionEvent::final_index,ScalarKind::I32,final_index)||!s.Scalar(root,F::RoomTransitionEvent::final_dimension,ScalarKind::I32,dimension)||
       !s.Scalar(root,F::RoomTransitionEvent::completed,ScalarKind::U8,done))return false;
  }else return true;
  if(!s.EventStore(layout,cosmetic,source,references))return false;
#ifdef ISAACCS_E2E
  if(layout==453&&replica::stream::E2ETraceEnabled()){
    const auto &request=s.transition.args;
    Trace("e39-route","phase=%s sequence=%llu requested=%d direction=%d animation=%d dimension=%d scope_room=%llu frame=%u",
      kind==K::TransitionFinish?"finish":"start",s.transition.sequence,int(request[0]),int(request[1]),int(request[2]),
      int(request[4]),w.scope.room,w.game_frame);
  }
#endif
  if(kind==K::TransitionFinish)s.transition.active=false;
  return true;
}
// SFX在已通过原生节流、已选sample并完成pan赋值的共同尾叶之后封存，设备为空仍有效。
bool BinaryDisplayCapture::Sfx(const CaptureWitness &w,EntityFacts facts,EntitySource &source,BinaryDisplayRead read,
    int id,float volume,int delay,bool loop,float pitch,float pan,std::uint32_t stream,int ambient,
    std::span<const CaptureReferenceEntry> references,CaptureDiagnostic &d) noexcept {
  auto &s=*impl_;s.Begin(w,facts,read,d);if(std::this_thread::get_id()!=s.thread||stream<24||id<0||!std::isfinite(volume)||!std::isfinite(pitch)||!std::isfinite(pan))return s.Fail(CaptureFailure::InvalidValue,448);
  CaptureLink asset;if(!source.AudioResource(w,facts,stream-24,asset,d))return false;
  BinarySfxIntent intent;intent.id=id;intent.volume=volume;intent.delay=delay;intent.loop=loop;intent.pitch=pitch;intent.pan=pan;intent.stream=stream;intent.ambient=ambient;intent.frame=w.game_frame;
  return s.SfxIntent(intent,asset,source,references);
}
bool BinaryDisplayCapture::Impl::SfxIntent(const BinarySfxIntent &intent,CaptureLink asset,EntitySource &source,std::span<const CaptureReferenceEntry> references) noexcept {
  auto &s=*this;const auto &w=witness;const auto id=intent.id,delay=intent.delay,ambient=intent.ambient;const auto stream=intent.stream;
  const auto volume=intent.volume,pitch=intent.pitch,pan=intent.pan;const bool loop=intent.loop;s.event_frame=intent.frame;
  // 同一原生sample的重播退休旧loop；不同sample即便同id仍是独立逻辑实例。
  auto old=std::find_if(s.loops.begin(),s.loops.end(),[&](auto &v){return v.active&&v.stream==stream;});
  if(loop){auto at=old;
    if(ambient>=0){auto same=std::find_if(s.loops.begin(),s.loops.end(),[&](auto &v){return v.active&&v.ambient==ambient;});if(same!=s.loops.end())at=same;}
    if(at==s.loops.end())at=std::find_if(s.loops.begin(),s.loops.end(),[](auto &v){return !v.active;});
    if(at==s.loops.end())return s.Fail(CaptureFailure::ValueBudget,441);
    const bool retry=ambient>=0&&at->active&&at->ambient==ambient&&at->id==id&&at->asset.kind==asset.kind&&at->asset.handle==asset.handle;
    if(!retry&&!s.Bump(s.next_loop))return false;
    if(old!=s.loops.end()&&old!=at)old->active=false;
    const auto instance=retry?at->instance:s.next_loop;const auto frame=retry?at->frame:intent.frame;
    *at={asset,instance,w.generation,stream,frame,id,ambient,volume,pitch,pan,true};return s.Bump(s.loop_revision);
  }
  if(old!=s.loops.end()){old->active=false;if(!s.Bump(s.loop_revision))return false;}
  const auto root=s.Node(448,6);if(root==UINT32_MAX||!s.Scalar(root,F::SfxPlayEvent::id,ScalarKind::I32,id)||
     !s.Scalar(root,F::SfxPlayEvent::volume,ScalarKind::F32,volume)||!s.Scalar(root,F::SfxPlayEvent::frame_delay,ScalarKind::I32,delay)||
     !s.Scalar(root,F::SfxPlayEvent::pitch,ScalarKind::F32,pitch)||!s.Scalar(root,F::SfxPlayEvent::pan,ScalarKind::F32,pan)||!s.Link(root,F::SfxPlayEvent::asset,asset))return false;
  return s.EventStore(448,true,source,references);
}
bool BinaryDisplayCapture::Impl::BossSnapshot(std::uint32_t node) noexcept {
  const auto owner=facts.address+0x1b83c;
  std::int32_t mode=0;std::uint32_t marker=0;
  if(!Value(owner,mode)||!Value(std::uint64_t(owner)+0x238,marker))return false;
  const bool active=mode==2&&marker;
  const Boss empty{};
  const auto &value=active?boss:empty;
  if(active&&(boss.owner!=owner||boss.first!=marker||!boss.generation))return Fail(CaptureFailure::MissingQualification,454);
  using namespace F::BossDisplayState;
  if(!Scalar(node,generation,ScalarKind::U64,value.generation)||!Scalar(node,boss_id1,ScalarKind::U32,value.first)||
     !Scalar(node,boss_id2,ScalarKind::U32,value.second)||!Scalar(node,room_index,ScalarKind::I32,value.room)||
     !Scalar(node,dimension,ScalarKind::I32,value.dimension))return false;
  const auto first=node_count;unsigned count=0;
  const auto pose=[&](int key,std::uint64_t sprite){
    float frame=0;std::uint8_t playing=0;
    if(!Value(sprite+0x40,frame)||!Value(sprite+0x44,playing)||!std::isfinite(frame)||playing>1)return false;
    const auto n=Node(F::BossDisplayPose::kLayout,3);++count;
    return n!=UINT32_MAX&&Scalar(n,F::BossDisplayPose::key,ScalarKind::I32,key)&&
      Scalar(n,F::BossDisplayPose::frame,ScalarKind::F32,frame)&&Scalar(n,F::BossDisplayPose::playing,ScalarKind::U8,playing);
  };
  if(active){
    if(!pose(-1,std::uint64_t(owner)+0x38))return Fail(CaptureFailure::InvalidValue,455);
    // J273 map头为head／size，节点+0x10为key，+0x14为原生ANM2。
    std::array<std::uint32_t,2> map{},after{};const auto at=std::uint64_t(owner)+0x224;
    std::uint32_t head=0;if(!Value(at,map)||!map[0]||map[1]>32||!Value(std::uint64_t(map[0])+4,head))return Fail(CaptureFailure::InvalidValue,455);
    const auto walk=[&](auto &&self,std::uint32_t n,unsigned depth)->bool {
      if(n==map[0])return true;
      if(!n||depth>=32||count>map[1])return false;
      std::array<std::uint32_t,3> links{};std::int32_t key=0;
      return Value(n,links)&&self(self,links[0],depth+1)&&count<=map[1]&&
        Value(std::uint64_t(n)+0x10,key)&&key>=0&&pose(key,std::uint64_t(n)+0x14)&&self(self,links[2],depth+1);
    };
    if(!walk(walk,head,0)||count!=map[1]+1||!Value(at,after)||after!=map)return Fail(CaptureFailure::SourceChanged,455);
  }
  return Children(node,poses,first,count);
}
bool BinaryDisplayCapture::Prepare(const CaptureWitness &w,EntityFacts &facts,EntitySource &source,BinaryDisplayRead read,const BinaryDisplayInputs &inputs,CaptureDiagnostic &d) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread||w.phase!=CapturePhase::Ordinary||!facts.scene)return FailCapture(d,CaptureFailure::InvalidWindow,{facts.subject});
  s.Begin(w,facts,read,d);if(s.gap)return s.GapFailure();
  // 生命周期内仅暂存接受意图；此处在真实ordinary窗口登记资源并绑定新world，不读旧原生指针。
  while(s.intention_count){const auto &intent=s.intentions[0];s.Begin(w,facts,read,d);
    CaptureLink asset;const auto path=std::as_bytes(std::span(intent.path)).first(intent.path_size);
    if(!source.DisplayAudioPathResource(w,facts,path,asset,d)||!s.SfxIntent(intent,asset,source,{}))return false;
    std::move(s.intentions.begin()+1,s.intentions.begin()+s.intention_count,s.intentions.begin());--s.intention_count;
  }
  s.Begin(w,facts,read,d);
#ifdef ISAACCS_E2E
  // 在 ordinary 窗口读取原生阶段，诊断读取失败不影响正式采集。
  if(replica::stream::E2ETraceEnabled()&&read.read){
    std::array<std::int32_t,8> value{};
    if(read.read(read.context,std::uint64_t(facts.address)+0x1b83c,std::as_writable_bytes(std::span(value)))&&
       (value[0]||s.transition_observed)){
      std::int32_t boss=0;std::uint8_t hud=0;
      read.read(read.context,std::uint64_t(facts.address)+0x1ba74,std::as_writable_bytes(std::span(&boss,1)));
      read.read(read.context,std::uint64_t(facts.address)+0x22e90,std::as_writable_bytes(std::span(&hud,1)));
      Trace("e39-host","revision=%llu frame=%u scope_room=%llu mode=%d counter=%d duration=%d progress=%08x direction=%d target=%d dimension=%d animation=%d boss=%d hud=%u",
        w.revision,w.game_frame,w.scope.room,value[0],value[1],value[2],unsigned(value[3]),value[4],value[5],value[6],value[7],boss,unsigned(hud));
      s.transition_observed=value[0]!=0;
    }
  }
#endif
  // 先绑定更早接受的音频意图，再给不调用ChangeRoom的animation 11／16等发送最终关联。
  if(s.transition.active){std::int32_t mode=0;if(!s.Value(std::uint64_t(facts.address)+0x1b83c,mode))return false;
    if(!mode){std::array<std::uint32_t,8> args{};
      if(!Observe(BinaryProductionKind::TransitionFinish,facts.address+0x1b83c,args,w,facts,source,read,{},d))return false;
      s.Begin(w,facts,read,d);}}
  const auto cutscene=HostModCutscene();
  if(cutscene.run==w.scope.run&&cutscene.serial>s.cutscene_serial){
    const auto event=s.Node(F::ModCutsceneEvent::kLayout,3);
    if(event==UINT32_MAX||!s.Scalar(event,F::ModCutsceneEvent::serial,ScalarKind::U64,cutscene.serial)||
       !s.Scalar(event,F::ModCutsceneEvent::id,ScalarKind::U32,cutscene.id)||
       !s.Scalar(event,F::ModCutsceneEvent::clear,ScalarKind::U8,std::uint8_t(cutscene.clear))||
       !s.EventStore(F::ModCutsceneEvent::kLayout,false,source,{}))return false;
    s.cutscene_serial=cutscene.serial;
    s.Begin(w,facts,read,d);
  }
  const auto root=s.Node(237,6),injection=s.Node(438,19),music=s.Node(439,15),loops=s.Node(441,2),overlays=s.Node(443,1),procedural=s.Node(445,3),boss=s.Node(454,6);
  if(boss==UINT32_MAX||!s.Children(root,F::GameRun::display_injections,injection,1)||!s.Children(root,F::GameRun::music_request,music,1)||
     !s.Children(root,F::GameRun::loop_sounds,loops,1)||!s.Children(root,F::GameRun::item_overlays,overlays,1)||!s.Children(root,F::GameRun::procedural_display,procedural,1)||
     !s.Children(root,F::GameRun::boss_display,boss,1)||!s.InjectionSnapshot(injection,inputs,source)||!s.MusicSnapshot(music,facts.manager)||
     !s.LoopSnapshot(loops)||!s.OverlaySnapshot(overlays)||!s.ProceduralSnapshot(procedural,source)||!s.BossSnapshot(boss))return false;
  facts.display_values=std::span<const DisplayValueNode>(s.nodes).first(s.node_count);return true;
}
void BinaryDisplayCapture::Impl::Drop(unsigned index,bool discarded) noexcept {
  const auto event=pending[index];events.RetireInstance(event.subject,event.layout,event.sequence);event_bytes-=event.bytes;
  std::move(pending.begin()+index+1,pending.begin()+event_count,pending.begin()+index);--event_count;if(discarded)++dropped;
}
// 可靠事件与latest world分开保存；容量按条数和实际自有字节同时限制，先退最旧纯外观。
bool BinaryDisplayCapture::Impl::EventStore(std::uint16_t layout,bool cosmetic,EntitySource &source,std::span<const CaptureReferenceEntry> references) noexcept {
  facts.layout=layout;facts.display_values=std::span<const DisplayValueNode>(nodes).first(node_count);
  CaptureValueWriter writer(event_scratch,witness);
  if(!source.Capture(witness,facts,BodyStream::Value,writer,*diagnostic))return false;
  if(next_event>=UINT64_MAX-1){gap=true;return Fail(CaptureFailure::SequenceExhausted,layout);}
  if(!Bump(next_event))return false;
  CaptureRecord origin;origin.production=witness;if(witness.phase==CapturePhase::Ordinary)origin.production.phase=CapturePhase::IntentionBinding;
  origin.owner=facts.subject;origin.instance=next_event;origin.generation=witness.generation;origin.event_game_frame=event_frame;
  origin.event_sequence=next_event;origin.purpose=layout;origin.pass=cosmetic?1:0;
  const auto resources=source.Resources();CapturePacket packet;
  if(read.references)references=read.references(read.context);
  if(!writer.Production(origin)||!writer.Catalog({0,std::uint32_t(resources.entries.size()),std::uint32_t(resources.bytes.size()),std::uint32_t(resources.layers.size())},references)||
     !writer.Seal(1,packet))return false;
  // 缓存按1KiB页拥有记录、路径、字节及紧缩引用表；不能只按正文估算256KiB限额。
  // 当前事件每种至多一个Entity引用；按引用cell数量计上界也覆盖以后出现的重复引用。
  const auto align=[](std::uint64_t n){return (n+7)&~std::uint64_t{7};};
  std::uint64_t references_used=0;
  for(const auto &cell:packet.cells)if(cell.kind==CaptureCellKind::References)references_used+=cell.count;
  const auto storage=align(packet.records.size_bytes())+align(packet.cells.size_bytes())+align(packet.paths.size_bytes())+
    align(packet.bytes.size())+std::min<std::uint64_t>(references_used,packet.references.size())*sizeof(CaptureReferenceEntry);
  const auto pages=(storage+1023)&~std::uint64_t{1023};
  if(pages>256u<<10){if(cosmetic){++dropped;return true;}gap=true;return Fail(CaptureFailure::ValueBudget,layout);}
  const auto size=std::uint32_t(pages);
  while(event_count==pending.size()||size>(256u<<10)-std::min(event_bytes,256u<<10)){
    unsigned oldest=event_count;for(unsigned i=0;i<event_count;++i)if(pending[i].cosmetic){oldest=i;break;}
    if(oldest==event_count){if(cosmetic){++dropped;return true;}gap=true;return Fail(CaptureFailure::ValueBudget,layout);}Drop(oldest);}
  if(!events.Store(packet,*diagnostic)){gap=true;return false;}
  pending[event_count++]={facts.subject,layout,next_event,event_frame,size,cosmetic,witness.scope};event_bytes+=size;return true;
}
bool BinaryDisplayCapture::Events(const StateScope &scope,CaptureValueWriter &writer,std::uint32_t &count,CaptureDiagnostic &d,CaptureAppendContext context) noexcept {
  auto &s=*impl_;s.diagnostic=&d;if(s.gap)return s.GapFailure();
  // 跨房纯外观不重放，当前房事件仍等Peer确认；非外观关联在最终房间完成后生成。
  for(unsigned i=0;i<s.event_count;){const auto &e=s.pending[i];const auto age=std::uint32_t(writer.Witness().game_frame-e.frame);
    if(e.cosmetic&&(e.scope!=scope||(age<0x80000000u&&age>15u))){s.Drop(i);continue;}++i;}
  return s.events.AppendEvents(scope,writer,count,d,context)&&writer.EventRange(s.transferred_event+1,s.next_event);
}
void BinaryDisplayCapture::MapFocus(std::uint32_t owner,std::uint32_t descriptor,std::uint32_t frame,std::uint32_t selected) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread||selected>=507)return;
  auto at=std::find_if(s.maps.begin(),s.maps.end(),[&](auto &v){return v.owner==owner;});if(at==s.maps.end())at=std::find_if(s.maps.begin(),s.maps.end(),[](auto &v){return !v.known;});
  if(at!=s.maps.end())*at={owner,descriptor,frame,std::uint32_t(std::clamp(std::int32_t(selected),0,169)),true};else s.gap=true;
}
void BinaryDisplayCapture::Reset(bool new_run) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return;s.events.Clear();s.pending={};s.event_count=s.event_bytes=0;s.transferred_event=s.next_event;s.gap=false;s.gap_failure={};s.maps={};
  if(new_run){s.music={};s.loops={};s.overlays={};s.transition={};s.boss={};s.loop_revision=1;s.intention_count=0;}
}
void BinaryDisplayCapture::Fault(const CaptureDiagnostic &diagnostic) noexcept {
  auto &s=*impl_;
  if(s.gap_failure.reason==CaptureFailure::None&&diagnostic.reason!=CaptureFailure::Incomplete)s.gap_failure=diagnostic;
  s.gap=true;
}
CaptureDiagnostic BinaryDisplayCapture::FirstFailure() const noexcept {
  const auto &s=*impl_;
  return std::this_thread::get_id()==s.thread?s.gap_failure:CaptureDiagnostic{};
}
std::uint64_t BinaryDisplayCapture::ReserveTransition() noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return 0;
  if(s.next_transition==UINT64_MAX){s.gap=true;return 0;}return ++s.next_transition;
}
void BinaryDisplayCapture::OverlayShown(std::uint32_t address,std::int32_t controller) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread||!address)return;
  if(controller< -1||controller>=64){s.gap=true;return;}
  auto at=std::find_if(s.overlays.begin(),s.overlays.end(),[&](auto &v){return v.address&&(v.address==address||v.controller==controller);});
  if(at==s.overlays.end())at=std::find_if(s.overlays.begin(),s.overlays.end(),[](auto &v){return !v.address;});
  if(at==s.overlays.end()||s.next_overlay==UINT64_MAX){s.gap=true;return;}
  for(auto &owner:s.overlays)if(&owner!=&*at&&owner.address&&(owner.controller==controller||owner.address==address))owner={};
  *at={address,++s.next_overlay,controller};
}
void BinaryDisplayCapture::BossShown(std::uint32_t owner,std::uint32_t first,std::uint32_t second,std::int32_t room,std::int32_t dimension) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread||!owner||!first)return;
  if(s.next_boss==UINT64_MAX){s.gap=true;return;}
  s.boss={owner,first,second,room,dimension,++s.next_boss};
}
// 只有实际正常API已接受的纯值会到达这里；生命周期中的缓存不具备world发布资格。
void BinaryDisplayCapture::BootstrapMusic(const BinaryMusicIntent &intent) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return;auto &music=s.music;using K=BinaryProductionKind;
  const auto bump=[&](std::uint64_t &v){if(v==UINT64_MAX){s.gap=true;return false;}++v;return true;};
  const auto kind=intent.kind;const auto &a=intent.args;
  if(kind==K::MusicPause)music.paused=true;else if(kind==K::MusicResume)music.paused=false;
  else if(kind==K::MusicLayerOn||kind==K::MusicLayerOff){if(a[0]>=6)return;if(kind==K::MusicLayerOn)music.layers|=std::uint8_t(1u<<a[0]);else music.layers&=std::uint8_t(~(1u<<a[0]));}
  else if(kind==K::MusicJingle){music.jingle=int(a[0]);music.jingle_active=true;if(!bump(music.jingle_generation))return;}
  else if(kind==K::MusicQueue)music.queued=intent.queued;
  else {const bool same=kind==K::MusicCrossfade&&intent.previous==int(a[0]);
    const bool continuation=kind==K::MusicPlay&&a[4]!=0;
    // 7D8303仅是Queue消费中的零起始gain；真实目标由同一tick按本机Options生成，速率0.08。
    // 各原生请求的参数位：Play(id,volume)、Crossfade(id,faderate)、Fadein(id,volume,faderate)、Fadeout(faderate)。
    // Crossfade 只有速率没有音量：速率必须进 fade（Client 按 Crossfade(id,fade) 调用）；
    // 旧实现把它放进 parameter、fade 留 0，Client 以速率0交叉淡化，标题曲永不淡出、关卡曲永不淡入。
    // Play／Fadein 的音量参数是 Host 本机音乐选项（原版 17 处 Play 调用中 12 处直传 Manager+0x2A190），仅作记录；
    // Client 不使用它，一律按自己的音量设定播放（见 client_display_native.cpp LocalMusicVolume）。
    const float parameter=continuation||kind==K::MusicCrossfade?1.0f:kind==K::MusicFadeout?0:std::bit_cast<float>(a[1]),
      fade=continuation?0.08f:kind==K::MusicCrossfade?std::bit_cast<float>(a[1]):kind==K::MusicFadein?std::bit_cast<float>(a[2]):
        kind==K::MusicFadeout?std::bit_cast<float>(a[0]):0;
    if(!std::isfinite(parameter)||!std::isfinite(fade)){s.gap=true;return;}
    if(!same){using M=MusicRequestMode;music.mode=std::uint8_t(continuation?M::QueuedContinuation:kind==K::MusicPlay?M::Play:kind==K::MusicCrossfade?M::Crossfade:kind==K::MusicFadein?M::Fadein:M::Fadeout);music.id=intent.current;music.parameter=parameter;music.fade=fade;
      if(kind!=K::MusicFadeout&&!bump(music.generation))return;music.known=true;}
    music.queued=intent.queued;music.paused=false;}
  bump(music.revision);
}
void BinaryDisplayCapture::BootstrapSfx(const BinarySfxIntent &intent) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return;
  using K=BinaryProductionKind;
  if(intent.kind!=K::SfxPlay){
    // 转场内的停止／混音调用作用于循环声音的当前状态。折入已登记或待登记的循环声，
    // 让逐声道停止不占用下一帧才能绑定到房间的播放队列，保留一次性声音的顺序。
    const auto control=[&](auto &loop){
      if(intent.kind==K::SfxStopLoops||
         (intent.kind==K::SfxStop&&loop.id==intent.id)||
         (intent.kind==K::SfxAmbient&&(loop.ambient==intent.id||loop.id==intent.id)&&intent.volume<=0.009999999776482582f))return true;
      if(loop.id==intent.id&&intent.kind==K::SfxVolume)loop.volume=intent.volume;
      if(loop.id==intent.id&&intent.kind==K::SfxPitch)loop.pitch=intent.pitch;
      return false;
    };
    bool changed=false;
    for(auto &loop:s.loops)if(loop.active){
      const auto volume=loop.volume,pitch=loop.pitch;
      if(control(loop))loop.active=false;
      changed|=!loop.active||volume!=loop.volume||pitch!=loop.pitch;
    }
    if(changed){
      if(s.loop_revision==UINT64_MAX)s.MarkGap(CaptureFailure::SequenceExhausted,F::LoopSoundState::kLayout);
      else ++s.loop_revision;
    }
    unsigned kept=0;
    for(unsigned i=0;i<s.intention_count;++i){
      auto &queued=s.intentions[i];
      if(queued.loop&&control(queued))continue;
      if(kept!=i)s.intentions[kept]=std::move(queued);
      ++kept;
    }
    s.intention_count=kept;
    return;
  }
  if(s.intention_count==s.intentions.size()){unsigned oldest=s.intention_count;
    for(unsigned i=0;i<s.intention_count;++i)if(!s.intentions[i].loop){oldest=i;break;}
    if(oldest==s.intention_count){if(!intent.loop){++s.dropped;return;}
      s.MarkGap(CaptureFailure::ValueBudget,F::LoopSoundState::kLayout);return;}
    std::move(s.intentions.begin()+oldest+1,s.intentions.end(),s.intentions.begin()+oldest);--s.intention_count;++s.dropped;}
  s.intentions[s.intention_count++]=intent;
}
std::uint64_t BinaryDisplayCapture::RetainedBytes() const noexcept {const auto &s=*impl_;return s.memory->Bytes()+s.events.RetainedBytes()+s.event_scratch.RetainedBytes();}
std::uint64_t BinaryDisplayCapture::EventWatermark() const noexcept{return impl_->next_event;}
void BinaryDisplayCapture::EventsTransferred(std::uint64_t watermark) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread||watermark<=s.transferred_event)return;
  if(watermark>s.next_event){s.MarkGap(CaptureFailure::InvalidValue,0);return;}
  for(unsigned i=0;i<s.event_count;)if(s.pending[i].sequence<=watermark)s.Drop(i,false);else ++i;
  s.transferred_event=watermark;
}
BinaryEventCaptureStats BinaryDisplayCapture::Statistics() const noexcept {
  const auto &s=*impl_;if(std::this_thread::get_id()!=s.thread)return {};
  return {s.next_event,s.transferred_event,s.dropped,s.event_count,s.event_bytes,s.intention_count,s.gap};
}
}
