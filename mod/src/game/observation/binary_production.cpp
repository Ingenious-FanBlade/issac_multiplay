#include "game/observation/binary_production.h"
#include "game/observation/binary_entities.h"
#include "game/observation/world_bridge.h"
#include "game/common/context.h"
#include "game/common/bridge.h"
#include "world/observe.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include <exception>

namespace cs::game {
namespace {thread_local BinaryProductionScope *production=nullptr;}
// 异常、错线程、Client或停用时不产生新结果；original的执行仍完全由原Hook链拥有。
BinaryProductionScope::BinaryProductionScope(BinaryProductionKind kind,void *object,void *manager) noexcept
    :kind_(kind),exceptions_(std::uncaught_exceptions()) {
  if(!IsMain() || ClientBusy())return;previous_=production;production=this;capture_=CurrentWorldRecording();
  if(capture_&&capture_->binary_entities)token_=capture_->binary_entities->ProductionBegin(kind,world::PackWord(object),world::PackWord(manager));
}
BinaryProductionScope::~BinaryProductionScope() noexcept {
  if(token_&&capture_&&capture_->binary_entities)capture_->binary_entities->ProductionEnd(token_,returned_&&exceptions_==std::uncaught_exceptions());
  if(production==this)production=previous_;
}
void BinaryProductionScope::Drop(void *player,bool held) noexcept {if(token_)capture_->binary_entities->ProductionDrop(token_,world::PackWord(player),held);}
void BinaryProductionScope::Mantle(void *effects) noexcept {if(token_)capture_->binary_entities->ProductionMantle(token_,world::PackWord(effects));}
void BinaryProductionScope::Progress(bool seed,bool blocked) noexcept {if(token_)capture_->binary_entities->ProductionProgress(token_,seed,blocked);}
void BinaryProductionScope::Audio(int sound,float volume,float pitch,float pan,bool loop,bool world) noexcept {if(token_)capture_->binary_entities->ProductionAudio(token_,sound,volume,pitch,pan,loop,world);}
void BinaryProductionScope::AudioPan(void *stream) noexcept {if(token_)capture_->binary_entities->ProductionAudioPan(token_,world::PackWord(stream));}
void BinaryProductionScope::RoomEffect(void *effects,bool enabled) noexcept {if(token_)capture_->binary_entities->ProductionRoomEffect(token_,world::PackWord(effects),enabled);}
void BinaryProductionScope::Stat(int index,std::uint32_t flags) noexcept {if(token_)capture_->binary_entities->ProductionStat(token_,index,flags);}
void BinaryProductionScope::Paused(void *game,bool paused) noexcept {if(token_)capture_->binary_entities->ProductionPaused(token_,world::PackWord(game),paused);}
void BinaryDrawClockObserved(void *game,bool paused) noexcept {
  if(!IsMain() || ClientBusy())return;
  for(auto *scope=production;scope;scope=scope->Previous())if(scope->Kind()==BinaryProductionKind::Screen){scope->Paused(game,paused);return;}
}
void BinaryHudDropObserved(int action,void *player,bool held) noexcept {
  if(action!=11 || !IsMain() || ClientBusy())return;for(auto *scope=production;scope;scope=scope->Previous())if(scope->Kind()==BinaryProductionKind::PlayerHud){scope->Drop(player,held);return;}
}
void BinaryHudMantleObserved(void *effects,int id) noexcept {
  if(id!=313 || !IsMain() || ClientBusy())return;for(auto *scope=production;scope;scope=scope->Previous())if(scope->Kind()==BinaryProductionKind::PlayerHud){scope->Mantle(effects);return;}
}
void BinaryRoomEffectObserved(void *effects,int id,bool enabled) noexcept {
  if(id!=481 || !IsMain() || ClientBusy())return;
  for(auto *scope=production;scope;scope=scope->Previous())if(scope->Kind()==BinaryProductionKind::RoomRender){scope->RoomEffect(effects,enabled);return;}
}
void BinaryHudProgressObserved(bool seed,bool blocked) noexcept {
  if(!IsMain() || ClientBusy())return;
  for(auto *scope=production;scope;scope=scope->Previous())if(scope->Kind()==BinaryProductionKind::HudRender){scope->Progress(seed,blocked);return;}
}
void BinaryAudioPanObserved(void *stream) noexcept {
  if(!IsMain() || ClientBusy())return;
  for(auto *scope=production;scope;scope=scope->Previous())if(scope->Kind()==BinaryProductionKind::SfxPlay){scope->AudioPan(stream);return;}
}
// 此窄来源只接真实Authority模拟窗口；菜单／未分类转场不能凭“存在Game”混入world。
bool BinaryAudioWorldContext() noexcept {
  if(!IsMain() || ClientBusy())return false;auto *runtime=Runtime();
  return runtime && !runtime->finished && !runtime->retiring && !runtime->failure && !runtime->blocks && runtime->pipeline.SimulationTick();
}
}
