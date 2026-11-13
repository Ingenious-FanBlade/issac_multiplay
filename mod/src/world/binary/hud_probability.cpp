#include "world/binary/entity_source_internal.h"
#include <bit>

namespace cs::world::binary {
// Wiki的总出现率不包含可生成楼层判断；只读743800的资格，不重复实现概率或道具查询。
bool EntitySource::HudDealEligibility(const CaptureWitness &w,EntityFacts f,bool &eligible,CaptureDiagnostic &d) noexcept {
  auto &s=*impl_;eligible=false;
  if(s.active||std::this_thread::get_id()!=s.thread||!f.ready||!s.services.current(s.services.owner,w,f))return FailCapture(d,CaptureFailure::InvalidWindow,{f.subject});
  NativeScene scene;scene.game=f.address;
  if(!s.memory.copy||!s.memory.copy(scene.game+0x18300,&scene.room,4)||!scene.room)return FailCapture(d,CaptureFailure::MissingQualification,{f.subject});
  if(!ReadSceneContext(w,f,scene,d))return false;s.facts=f;s.active=true;s.scratch_used=s.witness_used=0;
  struct End{bool &active;~End(){active=false;}}end{s.active};
  s.window={s.thread,w,&s,[](void *p,const CaptureWitness &v,const CaptureRoot &r) noexcept{return static_cast<Impl *>(p)->Current(v,r);},nullptr};
  const auto game=std::uint64_t(scene.game);std::int32_t stage=0,stage_type=0,difficulty=0,challenge=0,special=0,last_boss=0;
  std::uint32_t upper=0;std::uint8_t initialized=0,level_initializing=0;
  if(!s.Tracked(scene.room,initialized,d)||initialized!=1||!s.Tracked(game+0x183a0,level_initializing,d)||level_initializing||
     !s.Tracked(game,stage,d)||!s.Tracked(game+4,stage_type,d)||!s.Tracked(game+0x25f40,difficulty,d)||
     !s.Tracked(game+0x25afc,challenge,d)||!s.Tracked(game+0x25bb4,special,d)||!s.Tracked(game+0x25ac4,upper,d))return FailCapture(d,CaptureFailure::MissingQualification,{f.subject});
  const bool greed=difficulty==2||difficulty==3,alt=stage_type==4||stage_type==5;
  const auto xl_stage=std::int64_t(stage)+((scene.effective_curses&2)?1:0),deal_stage=xl_stage+(alt?1:0);
  eligible=challenge!=44;
  if(!greed&&alt&&xl_stage==8)eligible=false;
  if((stage>=1&&stage<=6&&(upper&0x10000))||(!greed&&alt&&xl_stage==6&&(upper&0x8000)))eligible=false;
  if(!greed){if(deal_stage==1){if(special!=17)eligible=false;}
    else if(deal_stage>8)eligible=false;
    else if(deal_stage==6){std::uint8_t unlocked=0,rerun=0;std::uint32_t mode=0,daily=0;
      if(!s.Tracked(std::uint64_t(f.manager)+80,unlocked,d)||!s.Tracked(std::uint64_t(f.manager)+8,mode,d)||
         !s.Tracked(game+0x25ba8,daily,d)||!s.Tracked(game+0x25b01,rerun,d))return false;
      if(!unlocked&&(mode!=2||(!daily&&!rerun)))eligible=false;}}
  if(eligible){std::uint32_t descriptors=0,config=0,doors=0;
    if(!s.Tracked(game+0x18314,last_boss,d)||!s.Tracked(game+0x182cc,descriptors,d)||descriptors>527||last_boss<0||std::uint32_t(last_boss)>=descriptors||
       !s.Tracked(game+36+std::uint32_t(last_boss)*184u,config,d)||!config||!s.Tracked(std::uint64_t(config)+56,doors,d))return FailCapture(d,CaptureFailure::MissingQualification,{f.subject});
    eligible=std::popcount(doors&255u)>=2;}
  std::array<std::byte,32> after{};for(unsigned i=0;i<s.witness_used;++i){const auto &mark=s.witnesses[i];
    if(!s.Read(mark.address,std::span(after).first(mark.size),d)||!std::equal(after.begin(),after.begin()+mark.size,mark.bytes.begin()))return FailCapture(d,CaptureFailure::SourceChanged,{f.subject});}
  return true;
}
}
