#pragma once
#include <cstdint>
#include <memory>

namespace cs::game {
struct WorldCapture;
// 每项对应已核正常生产owner；常驻Hook只在正式Host采集启用时观察。
enum class BinaryProductionKind : std::uint8_t { PlayerHud,Score,HudRender,StatRecompute,StatTick,Message,Minimap,SfxPlay,SfxVolume,SfxPitch,SfxStop,SfxStopLoops,SfxRemix,SfxAmbient,Screen,RoomRender };
class BinaryProductionScope final {
public:
  BinaryProductionScope(BinaryProductionKind,void *object,void *manager=nullptr) noexcept;
  ~BinaryProductionScope() noexcept;
  void Returned() noexcept {returned_=true;}
  void Drop(void *player,bool held) noexcept;
  void Mantle(void *effects) noexcept;
  void Progress(bool seed,bool blocked) noexcept;
  void Audio(int sound,float volume,float pitch,float pan,bool loop,bool world) noexcept;
  void AudioPan(void *stream) noexcept;
  void RoomEffect(void *effects,bool enabled) noexcept;
  void Stat(int index,std::uint32_t flags) noexcept;
  void Paused(void *game,bool paused) noexcept;
  BinaryProductionKind Kind() const noexcept {return kind_;}
  BinaryProductionScope *Previous() const noexcept {return previous_;}
private:
  std::shared_ptr<WorldCapture> capture_;
  BinaryProductionScope *previous_=nullptr;
  BinaryProductionKind kind_;
  std::uint32_t token_=0;
  int exceptions_=0;
  bool returned_=false;
};
void BinaryHudDropObserved(int action,void *player,bool held) noexcept;
void BinaryHudMantleObserved(void *effects,int id) noexcept;
void BinaryRoomEffectObserved(void *effects,int id,bool enabled) noexcept;
void BinaryHudProgressObserved(bool seed,bool blocked) noexcept;
void BinaryAudioPanObserved(void *stream) noexcept;
void BinaryDrawClockObserved(void *game,bool paused) noexcept;
bool BinaryAudioWorldContext() noexcept;
}
