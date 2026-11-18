#include "game/observation/binary_production.h"
#include "game/observation/binary_entities.h"
#include "game/observation/binary_intentions.h"
#include "game/observation/world_bridge.h"
#include "game/common/context.h"
#include "game/common/bridge.h"
#include "game/common/display_paths.h"
#include "world/observe.h"
#include "core/capture_cost.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include <exception>

namespace cs::game {
namespace {thread_local BinaryProductionScope *production=nullptr;thread_local unsigned display_path=0,music_tick_depth=0;}
BinaryMusicTickScope::BinaryMusicTickScope() noexcept:active_(IsMain()&&!ClientBusy()) {if(active_)++music_tick_depth;}
BinaryMusicTickScope::~BinaryMusicTickScope() noexcept {if(active_)--music_tick_depth;}
bool BinaryMusicQueueAdvance() noexcept {return IsMain()&&music_tick_depth!=0;}
BinaryDisplayPathScope::BinaryDisplayPathScope(unsigned entry) noexcept:previous_(display_path) {
  if(IsMain())display_path=NativeDisplayPath(static_cast<ClientEntry>(entry))?display_path+1:0;
}
BinaryDisplayPathScope::~BinaryDisplayPathScope() noexcept {if(IsMain())display_path=previous_;}
bool BinaryInDisplayPath() noexcept{return IsMain()&&display_path!=0;}
bool BinaryIntentActive() noexcept {
  if(!IsMain()||ClientBusy())return false;auto capture=CurrentWorldRecording();auto *runtime=Runtime();
  return capture&&capture->binary_entities&&capture->binary_entities->Active()&&runtime&&!runtime->finished&&!runtime->retiring&&!runtime->failure;
}
void BinaryMusicIntentObserved(const BinaryMusicIntent &intent) noexcept {
  if(!BinaryIntentActive())return;auto capture=CurrentWorldRecording();capture->binary_entities->Intention(intent);
}
void BinarySfxIntentObserved(const BinarySfxIntent &intent) noexcept {
  if(!BinaryIntentActive())return;auto capture=CurrentWorldRecording();capture->binary_entities->Intention(intent);
}
void BinaryMapFocusObserved(std::uint32_t owner,std::uint32_t descriptor,std::uint32_t frame,std::uint32_t selected) noexcept {
  if(!BinaryIntentActive())return;auto capture=CurrentWorldRecording();capture->binary_entities->MapFocus(owner,descriptor,frame,selected);
}
void BinaryOverlayObserved(std::uint32_t owner,std::int32_t controller) noexcept {
  if(!BinaryIntentActive())return;auto capture=CurrentWorldRecording();capture->binary_entities->OverlayShown(owner,controller);
}
void BinaryBossObserved(std::uint32_t owner,std::uint32_t first,std::uint32_t second,std::int32_t room,std::int32_t dimension) noexcept {
  if(!BinaryIntentActive())return;auto capture=CurrentWorldRecording();capture->binary_entities->BossShown(owner,first,second,room,dimension);
}
// 异常、错线程、Client或停用时不产生新结果；original的执行仍完全由原Hook链拥有。
BinaryProductionScope::BinaryProductionScope(BinaryProductionKind kind,void *object,void *manager) noexcept
    :kind_(kind),exceptions_(std::uncaught_exceptions()),object_(object) {
  capture::Cost cost(capture::CostPart::Native);
  if(!IsMain() || ClientBusy())return;previous_=production;production=this;capture_=CurrentWorldRecording();
  if(kind>=BinaryProductionKind::MusicPlay&&kind<=BinaryProductionKind::MusicLayerOff)music_before_=BinaryNativeMusicCurrent(object);
  if(capture_&&capture_->binary_entities)token_=capture_->binary_entities->ProductionBegin(kind,world::PackWord(object),world::PackWord(manager));
}
BinaryProductionScope::~BinaryProductionScope() noexcept {
  capture::Cost cost(capture::CostPart::Native);
  if(token_&&capture_&&capture_->binary_entities)capture_->binary_entities->ProductionEnd(token_,returned_&&exceptions_==std::uncaught_exceptions());
  if(!token_&&returned_&&exceptions_==std::uncaught_exceptions()&&IsMain()&&!ClientBusy()){
    if(kind_>=BinaryProductionKind::MusicPlay&&kind_<=BinaryProductionKind::MusicLayerOff){bool nested=false;
      for(auto *p=previous_;p;p=p->previous_)nested|=p->kind_>=BinaryProductionKind::MusicPlay&&p->kind_<=BinaryProductionKind::MusicLayerOff;
      if(!nested)BinaryNativeMusicFallback(kind_,object_,request_,music_before_);
    }else if(kind_==BinaryProductionKind::MapCache&&random_seen_)BinaryNativeMapFocusFallback(object_,random_value_);
    else if(kind_==BinaryProductionKind::OverlayShow)BinaryNativeOverlayFallback(object_,request_);
    else if(kind_==BinaryProductionKind::BossIntro)BinaryNativeBossFallback(object_,request_);
    else if(kind_>=BinaryProductionKind::SfxPlay&&kind_<=BinaryProductionKind::SfxAmbient&&audio_world_&&
        (kind_!=BinaryProductionKind::SfxPlay||audio_selections_==1))
      BinaryNativeSfxFallback(kind_,object_,audio_id_,audio_volume_,int(request_[0]),audio_loop_,audio_pitch_,audio_pan_,audio_stream_,audio_ambient_);
  }
  if(production==this)production=previous_;
}
void BinaryProductionScope::Audio(int sound,float volume,float pitch,float pan,bool loop,bool world) noexcept {
  audio_id_=sound;audio_volume_=volume;audio_pitch_=pitch;audio_pan_=pan;audio_loop_=loop;audio_world_=world;
  if(kind_==BinaryProductionKind::SfxPlay)for(auto *p=previous_;p;p=p->previous_)if(p->kind_==BinaryProductionKind::SfxAmbient){audio_ambient_=p->audio_id_;audio_world_=p->audio_world_;break;}
  if(token_)capture_->binary_entities->ProductionAudio(token_,sound,volume,pitch,pan,loop,world);
}
void BinaryProductionScope::AudioPan(void *stream) noexcept {audio_stream_=stream;++audio_selections_;if(token_)capture_->binary_entities->ProductionAudioPan(token_,world::PackWord(stream));}
void BinaryProductionScope::Request(std::uint32_t a0,std::uint32_t a1,std::uint32_t a2,std::uint32_t a3,std::uint32_t a4) noexcept {
  request_={a0,a1,a2,a3,a4};
  if(token_)capture_->binary_entities->ProductionRequest(token_,{a0,a1,a2,a3,a4});
}
void BinaryProductionScope::Random(std::uint32_t value) noexcept {
  random_value_=value;random_seen_=true;if(token_)capture_->binary_entities->ProductionRandom(token_,value);
}
void BinaryProductionScope::BannerText(const void *text,bool subtitle) noexcept {if(token_)capture_->binary_entities->ProductionBannerText(token_,world::PackWord(text),subtitle);}
void BinaryDisplayRandomObserved(unsigned value,const void *return_address) noexcept {
  if(!IsMain()||ClientBusy())return;
  const auto site=reinterpret_cast<std::uintptr_t>(return_address);
  for(auto *scope=production;scope;scope=scope->Previous())if((scope->Kind()==BinaryProductionKind::FortuneSource&&site==Base+0x558cbb)||
      (scope->Kind()==BinaryProductionKind::MapCache&&site==Base+0x5636cc)){scope->Random(value);return;}
}
void BinaryBannerTextObserved(const void *text,bool subtitle) noexcept {
  if(!IsMain()||ClientBusy())return;
  for(auto *scope=production;scope;scope=scope->Previous())if(scope->Kind()==BinaryProductionKind::ItemBanner){scope->BannerText(text,subtitle);return;}
}
void BinaryAudioPanObserved(void *stream) noexcept {
  if(!IsMain() || ClientBusy())return;
  for(auto *scope=production;scope;scope=scope->Previous())if(scope->Kind()==BinaryProductionKind::SfxPlay){scope->AudioPan(stream);return;}
}
// 此窄来源只接真实Authority模拟窗口；菜单／未分类转场不能凭“存在Game”混入world。
bool BinaryAudioWorldContext() noexcept {
  if(!IsMain() || ClientBusy() || BinaryInDisplayPath())return false;auto *runtime=Runtime();
  // 转场和ItemOverlay会在无Authority tick时发玩法声音；以正式run和正常生产窗口为来源。
  return runtime && !runtime->finished && !runtime->retiring && !runtime->failure;
}
}
