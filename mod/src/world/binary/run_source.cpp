#include "world/binary/entity_source_internal.h"
#include <cmath>

namespace cs::world::binary {
#include "state_host_fields.inc"
EntityRead RunRecipe(std::string_view name) noexcept {
  return name=="run_seed_display_flags_read" || name=="run_daily_display_read" || name=="run_ascent_display_read" ||
    name=="run_time_counter_anchor_read" || name=="run_hud_progress_result_observed" ||
    name=="run_seed_effect_ids_read" || name=="run_host_paused_read" || name=="run_display_branch_read" ||
    name=="sfx_volume_modifier_read"?EntityRead::Run:EntityRead::None;
}
// D5只读Seeds+54的既有树，沿用有界中序与前后证言；节点地址只存当前主线程arena。
bool EntitySource::Impl::RunOpen(const CaptureFieldAccess &a,CaptureTraversalSource &out,CaptureDiagnostic &d) noexcept {
  if(a.field.layout!=230 || a.field.index!=host_field::RunSeeds::effect_tree_head || a.root.address>UINT32_MAX-0x54)
    return Reject(a,d,CaptureFailure::MissingHandler);
  if(!facts.scene||std::uint64_t(facts.scene->game)+0x1bb84!=a.root.address)return Reject(a,d,CaptureFailure::MissingQualification);
  const auto count=facts.scene->seed_effect_count;
  auto bytes=Allocate(std::uint64_t(count)*4,4,d);if(d.reason!=CaptureFailure::None)return false;
  if(count)std::memcpy(bytes.data(),facts.scene->seed_effect_ids.data(),bytes.size());
  out={};out.identity=std::uint64_t(a.root.address)+0x54;out.count=count;
  out.opaque={NativeAddress(bytes),4u|(std::uint64_t(NativeSequence::Owned)<<32),0,0};return true;
}
// Q12在Host原生Update完成后的ordinary窗口读取原始谓词，不执行Console／Overlay／菜单Update。
// 原版Online目录只按原生谓词只读，绝不将C/S参与者写入该目录。
bool EntitySource::ReadDisplayContext(const CaptureWitness &w,EntityFacts &f,CaptureDiagnostic &d) noexcept {
  auto &s=*impl_;if(s.active || w.phase!=CapturePhase::Ordinary || std::this_thread::get_id()!=s.thread || !f.manager)
    return FailCapture(d,CaptureFailure::InvalidWindow,{f.subject,f.address,0,f.layout});
  f.display_ready=false;s.facts=f;s.active=true;s.witness_used=0;
  struct End{bool &active;~End(){active=false;}} end{s.active};
  s.window={s.thread,w,&s,[](void *p,const CaptureWitness &v,const CaptureRoot &r) noexcept {return static_cast<Impl *>(p)->Current(v,r);},nullptr};
  const auto game=std::uint64_t(f.address),manager=std::uint64_t(f.manager);
  std::uint32_t console=0,over=0,leader=0,stage=0,transition=0,overlay=0,pause=0,extra=0,manager_state=0,prompt_sprite=0;
  std::int32_t countdown=0;std::uint8_t prompt_playing=0,unknown=0;float secondary=0;
  std::array<std::uint32_t,2> online{};
  if(!s.Tracked(game+0x67fd4,console,d) || !s.Tracked(game+0x25a6c,countdown,d) ||
     !s.Tracked(game+0x1d4c8,over,d) || !s.Tracked(game+0x1d5fc,leader,d) || !s.Tracked(game+0x1ba78,stage,d) ||
     !s.Tracked(game+0x1b83c,transition,d) || !s.Tracked(game+0x1c034,overlay,d) || !s.Tracked(game+0x2363c,pause,d) ||
     !s.Tracked(game+0x24444,extra,d) || !s.Tracked(game+0x25d24,prompt_sprite,d) || !s.Tracked(game+0x25d34,prompt_playing,d) ||
     !s.Tracked(game+0x24ecc,unknown,d) || !s.Tracked(game+0x25b10,secondary,d) ||
     !s.Tracked(manager+8,manager_state,d) || !s.Tracked(manager+0x4afb8,online,d))return false;
  if(online[0]>online[1] || (online[1]-online[0])%4 || !std::isfinite(secondary) || prompt_playing>1)
    return FailCapture(d,CaptureFailure::InvalidValue,{f.subject,f.address,0,f.layout});
  const bool native_peers=online[0]!=online[1],small_native=(online[1]-online[0])<=4;
  const bool console_hold=console&&small_native,prompt=prompt_sprite&&prompt_playing,pause_hold=pause&&small_native;
  f.host_paused=console_hold||countdown>0||over||leader||stage||transition||overlay==2||pause_hold||manager_state!=2||prompt||unknown;
  using B=DisplayBranch;auto branch=B::Normal;
  if(console_hold)branch=B::ConsoleHold;else if(countdown>0)branch=B::PauseCountdown;else if(over)branch=B::GameOver;
  else if(leader)branch=B::Leaderboard;else if(stage)branch=B::StageTransition;else if(transition)branch=B::RoomTransition;
  else if(overlay==2&&(!pause||native_peers))branch=B::ActiveOverlay;
  else if(!overlay&&prompt)branch=B::GenericPrompt;else if(!overlay&&unknown)branch=B::UnknownUiByte;
  else if(secondary>0)branch=B::SecondaryFade;else if(pause_hold)branch=B::PauseScreen;
  else if(!native_peers&&extra)branch=B::UnknownUiWord;
  if(!s.Verify({f.subject,f.address,0,f.layout},d))return false;
  f.display_branch=branch;f.display_ready=true;return true;
}
// 只读当前已声明Run窄投影；分数与最终进度资格必须有正常生产证言，不调用其生产器。
bool EntitySource::Impl::RunLeaf(const CaptureFieldAccess &a,CaptureValueWriter &writer,CaptureDiagnostic &d) noexcept {
  std::uint64_t result=0;const auto address=std::uint64_t(a.root.address)+a.field.native_offset;
  if(a.field.layout==host_field::SFXVolumeState::kLayout){float value=0;
    if(!Value(address,value,d) || !std::isfinite(value))return Reject(a,d,CaptureFailure::InvalidValue);
    return writer.PutScalar(a.location,ScalarKind::F32,1,std::as_bytes(std::span(&value,1)));}
  if(a.field.layout==230){
    if(!facts.scene)return Reject(a,d,CaptureFailure::MissingQualification);
    constexpr unsigned effects[]={8,9,10,29,30,78,79};for(unsigned i=0;i<std::size(effects);++i)if(facts.scene->seed_effects[effects[i]])result|=std::uint64_t{1}<<i;
  }else if(a.field.layout==233){std::uint32_t id=0;if(!Value(address,id,d))return false;result=id!=0;}
  else if(a.field.layout==236){std::uint32_t upper=0;if(!Value(address,upper,d))return false;result=(upper>>16)&1u;}
  else if(a.field.layout==237 && a.field.index==host_field::GameRun::time_counter_base){
    std::uint32_t counter=0;if(!Value(address,counter,d))return false;result=std::uint32_t(a.window.witness.game_frame-counter);
  }else if(a.field.layout==237 && (a.field.index==host_field::GameRun::host_paused || a.field.index==host_field::GameRun::display_branch)){
    if(!facts.display_ready)return Reject(a,d,CaptureFailure::MissingQualification);
    result=a.field.index==host_field::GameRun::host_paused?std::uint8_t(facts.host_paused):std::uint8_t(facts.display_branch);
  }
  else return Reject(a,d,CaptureFailure::MissingHandler);
  std::array<std::byte,8> bytes{};const auto width=CaptureScalarWidth(a.format.scalar);
  if(!width || width>8 || (width<8 && result>>(width*8)))return Reject(a,d,CaptureFailure::InvalidValue);
  for(unsigned i=0;i<width;++i)bytes[i]=std::byte(result>>(8*i));return writer.PutScalar(a.location,a.format.scalar,1,std::span(bytes).first(width));
}
}
