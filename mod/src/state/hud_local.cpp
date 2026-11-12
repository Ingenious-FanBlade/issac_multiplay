#include "state/hud_local.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <utility>

namespace cs::state {
namespace {
#include "state_hud_local.inc"
[[noreturn]] void Fail(const char *reason) {throw Gap("hud_local",reason);}
bool IdentityValid(const HudLocalIdentity &v) noexcept {
  return v.scope.session&&v.scope.run&&v.scope.room&&v.scope.scene&&v.body.id&&v.body.use&&v.body.shape&&
      v.body.layout==197&&v.group.layout==196&&v.group.id&&v.group.use&&v.group.shape&&
      v.binding&&v.instance&&v.generation&&v.topology;
}
// Host采集键的原生shape修订可以从0开始；转为Client成员shape由正式表owner负责。
// 这里仍精确比较完整键，不把0补成1或借用另一个身体的身份。
bool CaptureIdentityValid(const HudLocalIdentity &v) noexcept {
  return v.scope.session&&v.scope.run&&v.scope.room&&v.scope.scene&&v.body.space==1&&v.body.id&&v.body.use&&
      v.body.layout==197&&v.group.space==10&&v.group.layout==196&&v.group.id&&v.group.use&&
      v.binding&&v.instance&&v.generation&&v.topology;
}
void ClockValid(const HudLocalClock &c) {
  if(!c.binding||!c.update||!c.mantle_revision||!c.drop_anchor||c.drop_anchor>c.update||c.drop_phase>4)
    Fail("clock_input_missing_or_invalid");
}
// 九项顺序只由生成表指定；本地存储为小端纯值，不把C++结构填充放到网络上。
class ClockSource final : public BodyValueSource {
public:
  explicit ClockSource(const HudClockWireValue &v) {
    const std::array<std::uint64_t,9> values={v.player,v.clock.binding,v.clock.update,v.clock.game_frame,
        std::uint64_t(v.clock.critical_health),v.clock.mantle_revision,v.clock.drop_anchor,
        std::uint64_t(v.clock.drop_held),v.clock.drop_phase};
    for(unsigned i=0;i<values.size();++i)for(unsigned b=0;b<storage_[i].size();++b)storage_[i][b]=std::byte(values[i]>>(8*b));
  }
  std::span<const std::byte> Scalar(const CodecLocation &location,ScalarKind,std::uint32_t count) const override {
    if(location.layout!=kHudClockLayout||!location.parents.empty()||count!=1)Fail("clock_scalar_path");
    for(unsigned i=0;i<kHudClockFields.size();++i)if(location.field==kHudClockFields[i])return std::span(storage_[i]).first(kHudClockWidths[i]);
    Fail("clock_scalar_field");
  }
  std::uint32_t Count(const CodecLocation &) const override {Fail("clock_not_container");}
  bool SharedPresent(const CodecLocation &) const override {Fail("clock_not_shared");}
  std::uint32_t SlotMask(const CodecLocation &) const override {Fail("clock_not_slots");}
  std::uint16_t SlotLayout(const CodecLocation &,std::uint8_t) const override {Fail("clock_not_slots");}
private:
  std::array<std::array<std::byte,8>,9> storage_{};
};
bool ImageValid(const HudPreparedImage &v) noexcept {
  return v.key.epoch&&v.key.index!=kNullIndex&&v.key.configuration&&v.key.content&&
      std::isfinite(v.center_x)&&std::isfinite(v.center_y);
}

std::span<const std::byte> EncodeClockValue(const HudClockWireValue &v,std::span<std::byte> output,BodyCodecScratch &scratch) {
  ClockValid(v.clock);if(v.player==kNullIndex)Fail("clock_body_reference_missing");
  return EncodeValueInto(kHudClockLayout,ClockSource(v),output,scratch,nullptr,{kHudClockBytes,32,2,true}).bytes;
}
HudClockWireValue DecodeClockValue(std::span<const std::byte> input,BodyCodecScratch &scratch) {
  std::array<BodyFieldView,9> fields{};auto local=scratch;local.root_fields=fields;
  const auto view=DecodeValue(kHudClockLayout,input,local,nullptr,{kHudClockBytes,32,2,true});
  std::array<std::uint64_t,9> values{};
  for(unsigned i=0;i<kHudClockFields.size();++i){
    const auto found=std::find_if(view.fields.begin(),view.fields.end(),[&](const BodyFieldView &f){return f.field==kHudClockFields[i];});
    if(found==view.fields.end()||!found->present||found->bytes.size()!=kHudClockWidths[i])Fail("clock_field_missing");
    for(unsigned b=0;b<found->bytes.size();++b)values[i]|=std::uint64_t(std::to_integer<unsigned>(found->bytes[b]))<<(8*b);
  }
  HudClockWireValue result;result.player=std::uint16_t(values[0]);auto &c=result.clock;
  c.binding=values[1];c.update=values[2];c.game_frame=std::uint32_t(values[3]);c.critical_health=values[4]!=0;
  c.mantle_revision=values[5];c.drop_anchor=values[6];c.drop_held=values[7]!=0;c.drop_phase=std::uint8_t(values[8]);
  ClockValid(c);if(result.player==kNullIndex)Fail("clock_body_reference_missing");return result;
}
HudPreparedImage RequireImage(std::optional<HudPreparedImage> value,const HudLocalResources &r) {
  if(!value||!ImageValid(*value)||!r.Ready(value->key))Fail("exact_local_image_not_ready");
  return *value;
}
HudQueueKey RequireQueue(std::optional<HudQueueKey> value,const HudLocalResources &r) {
  if(!value||!value->epoch||value->file==kNullIndex||value->animation==kNullIndex||
      !value->configuration||!value->content||!r.QueueReady(*value))Fail("exact_queue_resource_not_ready");
  return *value;
}
std::int32_t TrinketId(std::uint32_t raw) {
  if(raw&~(kHudGoldenMask|(kHudGoldenMask-1u)))Fail("unknown_trinket_identity_bits");
  return std::int32_t(raw&~kHudGoldenMask);
}
// 普通分支不解释保留结果的旧槽字节；特殊分支绝不回退到配置普通图。
HudLocalIcon Active(const HudActiveInput &v,unsigned slot,const HudLocalResources &r) {
  if(!std::isfinite(v.partial_charge))Fail("active_charge_nonfinite");
  HudLocalIcon out;
  switch(v.source){
    case HudIconSource::Empty:if(v.item)Fail("active_empty_state_conflict");break;
    case HudIconSource::Local:
      if(!HudOrdinaryIcon(HudIconKind::Active,v.item,slot))Fail("active_mapping_not_declared");
      out.visual=v.item;out.image=RequireImage(r.Ordinary(HudIconKind::Active,v.item,slot),r);out.has_image=true;break;
    case HudIconSource::Host:
      out.visual=v.special_visual;
      if(v.special_image!=kNullIndex){out.image=RequireImage(r.Special(v.special_image),r);out.has_image=true;}
      break;
    default:Fail("active_icon_source");
  }
  return out;
}
HudLocalIcon Trinket(const HudTrinketInput &v,unsigned slot,const HudLocalResources &r) {
  const auto id=TrinketId(v.item);HudLocalIcon out;out.golden=(v.item&kHudGoldenMask)!=0;out.tint_override=v.tint_override;
  switch(v.source){
    case HudIconSource::Empty:if(v.item)Fail("trinket_empty_state_conflict");break;
    case HudIconSource::Local:
      if(!HudOrdinaryIcon(HudIconKind::Trinket,id,slot))Fail("trinket_mapping_not_declared");
      out.visual=id;out.image=RequireImage(r.Ordinary(HudIconKind::Trinket,id,slot),r);out.has_image=true;break;
    case HudIconSource::Host:
      out.visual=v.special_visual;
      if(!std::isfinite(v.special_center[0])||!std::isfinite(v.special_center[1]))Fail("special_center_nonfinite");
      if(v.special_image!=kNullIndex){out.image=RequireImage(r.Special(v.special_image),r);out.has_image=true;
        out.image.center_x=v.special_center[0];out.image.center_y=v.special_center[1];}
      break;
    default:Fail("trinket_icon_source");
  }
  return out;
}
}

std::span<const std::byte> EncodeHudLocalClockInto(const HudClockWireValue &v,std::span<std::byte> output,BodyCodecScratch &scratch) {
  return EncodeClockValue(v,output,scratch);
}
HudClockWireValue DecodeHudLocalClock(std::span<const std::byte> input,BodyCodecScratch &scratch) {
  return DecodeClockValue(input,scratch);
}
bool HudCriticalHealth(std::int32_t red,std::int32_t eternal,std::int32_t soul,std::int32_t bone) noexcept {
  // x86原生加法按32位回绕，避免C++有符号溢出，同时不遗漏永恒心/骨心。
  return std::uint32_t(red)+std::uint32_t(eternal)+std::uint32_t(soul)+std::uint32_t(bone)==1u;
}
bool HudHolyMantle(const HudEffectsInput &v) {
  if(!v.complete||!v.revision||v.entries.size()>16384)Fail("complete_effect_revision_missing");
  if(v.disabled)return false;
  for(const auto &entry:v.entries)
    if((entry.category==1||entry.category==3||entry.category==4)&&entry.id==313)return true;
  return false;
}
std::int16_t HudLowHealthFlash(bool critical,bool mantle,std::uint32_t frame) noexcept {
  const auto phase=frame%45u;
  return critical&&!mantle&&phase<10?std::int16_t(255u*(9u-phase)/9u):0;
}
float HudDropScale(const HudLocalClock &c) {
  ClockValid(c);
  // 中途错过多个报文也从最新完整锚点恢复，最多四次显示更新即饱和，无逐帧循环。
  const auto steps=std::uint8_t(std::min<std::uint64_t>(c.update-c.drop_anchor,4));
  const auto phase=c.drop_held?std::min<unsigned>(unsigned(c.drop_phase)+steps,4):
      (c.drop_phase>steps?unsigned(c.drop_phase-steps):0u);
  return float(phase)*0.25f;
}
HudClockCapture::HudClockCapture(std::thread::id owner):charge_(memory::Pool::Values,sizeof(*this),"hud-clock-capture"),thread_(owner) {}
HudLocalClock HudClockCapture::Observe(const HudNormalUpdate &v) {
  if(thread_!=std::this_thread::get_id())Fail("capture_wrong_thread");
  if(!CaptureIdentityValid(v.identity)||!v.source_revision||!v.mantle_revision||v.completed_drop_phase>4)
    Fail("normal_update_witness_missing");
  const bool same=present_&&identity_==v.identity;
  if(same&&v.source_revision<=source_revision_)Fail("duplicate_or_reordered_normal_update");
  if(same&&clock_.update==UINT64_MAX)Fail("hud_update_counter_exhausted");
  HudLocalClock next=same?clock_:HudLocalClock{};
  next.binding=v.identity.binding;next.update=same?clock_.update+1:1;next.game_frame=v.game_frame;
  next.critical_health=v.critical_health;next.mantle_revision=v.mantle_revision;
  if(!same||v.drop_held!=clock_.drop_held){
    next.drop_anchor=next.update;next.drop_phase=v.completed_drop_phase;next.drop_held=v.drop_held;
  }else if(HudDropScale(next)!=float(v.completed_drop_phase)*0.25f){
    // 可识别的正常恢复/纠正重新锚定，绝不通过到包次数推测漏掉了几次按放。
    next.drop_anchor=next.update;next.drop_phase=v.completed_drop_phase;
  }
  ClockValid(next);identity_=v.identity;source_revision_=v.source_revision;clock_=next;present_=true;return next;
}
void HudClockCapture::Retire() noexcept {if(thread_!=std::this_thread::get_id())return;present_=false;identity_={};clock_={};source_revision_=0;}
bool HudOrdinaryIcon(HudIconKind kind,std::int32_t id,unsigned slot) noexcept {
  if(id<=0)return false;
  if(kind==HudIconKind::Active)return slot<4&&std::uint32_t(id)<=kHudActiveLast&&
      std::find(kHudActiveSpecial.begin(),kHudActiveSpecial.end(),std::uint32_t(id))==kHudActiveSpecial.end();
  if(kind==HudIconKind::Trinket)return slot<2&&std::uint32_t(id)<=kHudTrinketLast&&
      std::find(kHudTrinketSpecial.begin(),kHudTrinketSpecial.end(),std::uint32_t(id))==kHudTrinketSpecial.end();
  return false;
}
HudIconSource SelectHudIconSource(HudIconKind kind,unsigned slot,std::int32_t state_id,
    std::int32_t visual_id,bool empty,std::uint32_t category,bool exact_image) noexcept {
  if(empty&&state_id==0&&visual_id==0)return HudIconSource::Empty;
  const bool category_matches=kind==HudIconKind::Active?category==3:kind==HudIconKind::Trinket&&category==2;
  return category_matches&&exact_image&&state_id==visual_id&&HudOrdinaryIcon(kind,state_id,slot)?HudIconSource::Local:HudIconSource::Host;
}
std::array<float,2> HudOpaqueCenter(std::span<const std::uint8_t> alpha,unsigned w,unsigned h,unsigned stride) {
  if(!w||!h||w>32||h>32||stride<w||std::uint64_t(h-1)*stride+w>alpha.size())Fail("image_alpha_extent");
  unsigned left=w,right=0,top=h,bottom=0;bool found=false;
  for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)if(alpha[std::size_t(y)*stride+x]){
    found=true;left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);}
  if(!found)Fail("image_alpha_empty");
  return {float(left+(right-left)/2),float(top+(bottom-top)/2)};
}
HudLocalFrame PrepareHudLocal(const HudLocalInputs &v,const HudLocalResources &r) {
  if(!v.complete||!IdentityValid(v.identity)||!v.target||v.clock.binding!=v.identity.binding)
    Fail("complete_same_body_target_missing");
  ClockValid(v.clock);
  if(v.effects.body!=v.identity.body||v.effects.revision!=v.clock.mantle_revision)
    Fail("exact_mantle_effect_revision_missing");
  HudLocalFrame out;out.identity=v.identity;out.target=v.target;out.clock=v.clock;out.mantle=HudHolyMantle(v.effects);
  out.low_health_flash=HudLowHealthFlash(v.clock.critical_health,out.mantle,v.clock.game_frame);out.drop_scale=HudDropScale(v.clock);
  for(unsigned i=0;i<v.active.size();++i){out.active[i]=Active(v.active[i],i,r);out.charge_flash[i]=v.active[i].charge_flash;
    if(v.active[i].book_image!=kNullIndex)out.books[i]=RequireImage(r.Special(v.active[i].book_image),r);}
  for(unsigned i=0;i<v.trinkets.size();++i)out.trinkets[i]=Trinket(v.trinkets[i],i,r);
  // 原槽序和空值直接交显示消费者；不按counts重排，不计算合成输出或抽取队列。
  out.materials=v.materials;out.spells=v.spells;
  // 839A80/83A840：材料Idle原帧；法术首槽Idle，其余IdleSmall，原消费顺序5→0。
  for(unsigned i=0;i<v.materials.size();++i){
    out.material_draw[i]={v.materials[i],std::uint8_t(i),std::uint8_t(i),false,float(12*(i&3))-64.0f,float(12*(i>>2))-6.0f,1};
    out.material_draw[i].resource=RequireQueue(r.Queue(HudQueueKind::Material,v.materials[i],false),r);
  }
  for(unsigned i=0;i<v.spells.size();++i){
    out.spell_draw[i]={v.spells[i],std::uint8_t(i),std::uint8_t(5-i),i!=0,float(12*i)+8,8,int(i)>=v.poop_mana?0.1f:1.0f};
    out.spell_draw[i].resource=RequireQueue(r.Queue(HudQueueKind::Spell,v.spells[i],i!=0),r);
  }
  if(v.stat_precision>6)Fail("stat_precision_budget");
  for(unsigned i=0;i<v.stat_values.size();++i){
    if(!std::isfinite(v.stat_values[i]))Fail("stat_value_nonfinite");
    auto &text=out.stat_text[i];const auto formatted=std::to_chars(text.data(),text.data()+text.size()-1,
        v.stat_values[i],std::chars_format::fixed,int(v.stat_precision));
    if(formatted.ec!=std::errc{})Fail("stat_text_budget");*formatted.ptr='\0';
  }
  return out;
}
bool HudLocalResourcesReady(const HudLocalFrame &v,const HudLocalResources &r) noexcept {
  try{
    for(const auto &icon:v.active)if(icon.has_image&&!r.Ready(icon.image.key))return false;
    for(const auto &icon:v.trinkets)if(icon.has_image&&!r.Ready(icon.image.key))return false;
    for(const auto &book:v.books)if(book&&!r.Ready(book->key))return false;
    for(const auto &glyph:v.material_draw)if(!r.QueueReady(glyph.resource))return false;
    for(const auto &glyph:v.spell_draw)if(!r.QueueReady(glyph.resource))return false;
    return true;
  }catch(...){return false;}
}
HudLocalOwner::HudLocalOwner():charge_(memory::Pool::Values,sizeof(*this),"hud-local-owner") {}
std::uint64_t HudLocalOwner::Prepare(const HudLocalInputs &v,const HudLocalResources &r) {
  if(thread_!=std::this_thread::get_id())Fail("owner_wrong_thread");
  if(retiring_||pending_)Fail("owner_retiring_or_candidate_busy");
  if(ticket_==UINT64_MAX)Fail("local_ticket_exhausted");
  if(present_&&committed_.identity==v.identity&&v.target<=committed_.target)Fail("target_not_newer");
  if(present_&&committed_.identity==v.identity&&
      (v.clock.update<committed_.clock.update||(v.clock.update==committed_.clock.update&&v.clock!=committed_.clock)))
    Fail("clock_reordered_or_same_update_conflict");
  auto value=PrepareHudLocal(v,r);
  if(present_&&committed_.identity==v.identity&&v.clock.update==committed_.clock.update&&value.mantle!=committed_.mantle)
    Fail("immutable_effect_revision_conflict");
  prepared_=std::move(value);pending_=true;return ++ticket_;
}
bool HudLocalOwner::Commit(std::uint64_t ticket,const HudLocalIdentity &identity,std::uint32_t target,const HudLocalResources &r) {
  if(thread_!=std::this_thread::get_id()||retiring_||!pending_||borrows_||ticket!=ticket_||identity!=prepared_.identity||target!=prepared_.target||
      !HudLocalResourcesReady(prepared_,r))return false;
  committed_=std::move(prepared_);prepared_={};pending_=false;present_=true;return true;
}
void HudLocalOwner::Cancel() noexcept {if(thread_!=std::this_thread::get_id())return;pending_=false;prepared_={};}
const HudLocalFrame *HudLocalOwner::Borrow() noexcept {
  if(thread_!=std::this_thread::get_id()||retiring_||!present_||borrows_==UINT32_MAX)return nullptr;++borrows_;return &committed_;
}
void HudLocalOwner::Release() noexcept {
  if(thread_!=std::this_thread::get_id())return;if(borrows_)--borrows_;if(retiring_&&!borrows_)committed_={};
}
void HudLocalOwner::Retire() noexcept {if(thread_!=std::this_thread::get_id())return;retiring_=true;present_=false;Cancel();if(!borrows_)committed_={};}

std::span<const HudLocalPlacement> PlanHudLocalLayout(std::span<const HudLayoutBody> bodies,
    const PresentationMember &local,const HudViewport &v,std::span<HudLocalPlacement> output) {
  if(bodies.size()>128||output.size()<bodies.size()||!std::isfinite(v.width)||!std::isfinite(v.height)||
      !std::isfinite(v.inset)||!std::isfinite(v.preferred_scale)||v.inset<0||v.preferred_scale<=0||
      v.width<=v.inset*2||v.height<=v.inset*2)Fail("layout_viewport_or_capacity");
  std::array<unsigned,4> count{};std::array<PresentationMember,4> groups{};
  std::array<std::uint32_t,4> order_mask{};
  for(std::size_t i=0;i<bodies.size();++i){const auto &b=bodies[i];
    if(!b.body.id||!b.body.use||!b.body.shape||b.body.layout!=197||!b.group.id||!b.group.use||
        !b.group.shape||b.group.layout!=196||b.ordinal>=4||!b.roles||(b.roles&~31u)||b.display_order>=32)
      Fail("layout_body_selection_invalid");
    if(count[b.ordinal]&&groups[b.ordinal]!=b.group)Fail("layout_group_ordinal_conflict");
    for(std::size_t j=0;j<i;++j)if(bodies[j].body==b.body||
        (bodies[j].group==b.group&&bodies[j].ordinal!=b.ordinal))Fail("layout_duplicate_body_or_group");
    if(order_mask[b.ordinal]&(1u<<b.display_order))Fail("layout_duplicate_display_order");
    order_mask[b.ordinal]|=1u<<b.display_order;groups[b.ordinal]=b.group;++count[b.ordinal];
  }
  for(unsigned g=0;g<4;++g)if(count[g]&&order_mask[g]!=(count[g]==32?UINT32_MAX:(1u<<count[g])-1u))
    Fail("layout_display_order_not_dense");
  if(v.no_hud||!v.world_visible)return {};
  const float cell_width=(v.width-2*v.inset)/2,cell_height=(v.height-2*v.inset)/2;
  // 为每组所有必要公共HUD留位置，再按本地视口等比压缩；从不改PlayerIndex或roster。
  for(std::size_t i=0;i<bodies.size();++i){const auto &b=bodies[i];HudLocalPlacement p;p.body=b.body;
    p.roles=b.group==local?b.roles:std::uint8_t(b.roles&17u);
    p.scale=std::min({v.preferred_scale,cell_width/160.0f,cell_height/(float(count[b.ordinal])*48.0f)});
    if(!std::isfinite(p.scale)||p.scale<=0)Fail("layout_scale_unrepresentable");
    p.clip_x=v.inset+float(b.ordinal%2)*cell_width;p.clip_y=v.inset+float(b.ordinal/2)*cell_height;
    p.clip_width=cell_width;p.clip_height=cell_height;p.x=p.clip_x;p.y=p.clip_y+float(b.display_order)*48.0f*p.scale;
    if(p.roles&4u)p.history_prefix=std::min(b.history_count,std::uint32_t(std::min(cell_width/(16*p.scale),65535.0f)));
    output[i]=p;
  }
  return output.first(bodies.size());
}
} // namespace cs::state
