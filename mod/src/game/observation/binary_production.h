#pragma once
#include <cstdint>
#include <memory>
#include <cstring>
#include <array>

namespace cs::game {
inline std::uint32_t BinaryFloatWord(float value) noexcept {std::uint32_t bits=0;std::memcpy(&bits,&value,4);return bits;}
struct WorldCapture;
// 仅反映采集端自有事件副本；transferred是worker接管水位，不是远端播放／AppliedAck。
struct BinaryEventCaptureStats {
  std::uint64_t watermark=0,transferred=0,dropped=0;
  std::uint32_t pending=0,bytes=0,intentions=0;
  bool incomplete=false;
};
// 每项对应已核正常生产owner；常驻Hook只在正式Host采集启用时观察。
enum class BinaryProductionKind : std::uint8_t { Score,SfxPlay,SfxVolume,SfxPitch,SfxStop,SfxStopLoops,SfxRemix,SfxAmbient,
  MusicPlay,MusicCrossfade,MusicQueue,MusicFadein,MusicFadeout,MusicPause,MusicResume,MusicJingle,MusicLayerOn,MusicLayerOff,
  ItemBanner,CustomBanner,Fortune,OverlayShow,BossIntro,TransitionStart,TransitionFinish,FortuneSource,MapCache };
class BinaryProductionScope final {
public:
  BinaryProductionScope(BinaryProductionKind,void *object,void *manager=nullptr) noexcept;
  ~BinaryProductionScope() noexcept;
  void Returned() noexcept {returned_=true;}
  void Audio(int sound,float volume,float pitch,float pan,bool loop,bool world) noexcept;
  void AudioPan(void *stream) noexcept;
  void Request(std::uint32_t a0=0,std::uint32_t a1=0,std::uint32_t a2=0,std::uint32_t a3=0,std::uint32_t a4=0) noexcept;
  void Random(std::uint32_t value) noexcept;
  void BannerText(const void *text,bool subtitle) noexcept;
  bool Capturing() const noexcept {return token_!=0;}
  BinaryProductionKind Kind() const noexcept {return kind_;}
  BinaryProductionScope *Previous() const noexcept {return previous_;}
private:
  std::shared_ptr<WorldCapture> capture_;
  BinaryProductionScope *previous_=nullptr;
  BinaryProductionKind kind_;
  std::uint32_t token_=0;
  int exceptions_=0;
  bool returned_=false;
  void *object_=nullptr,*audio_stream_=nullptr;
  std::array<std::uint32_t,5> request_{};
  std::uint32_t music_before_=UINT32_MAX,audio_selections_=0;
  std::uint32_t random_value_=0;bool random_seen_=false;
  int audio_id_=0,audio_ambient_=-1;
  float audio_volume_=0,audio_pitch_=1,audio_pan_=0;
  bool audio_loop_=false,audio_world_=false;
};
void BinaryAudioPanObserved(void *stream) noexcept;
bool BinaryAudioWorldContext() noexcept;
// Host 原生暂停页（Game+2363C 状态非0：主菜单／选项／确认）打开期间的一次性 Play 是本机菜单音效
// （打开17、关闭18、选项调节等），不进入 world 事件；循环 Play 与 Stop／StopLoops 等控制不受影响。
bool BinaryHostPauseMenuOpen() noexcept;
// SoundEffects::Play 的 world 资格：普通来源判定之外，排除 Host 暂停页内的一次性菜单音效。
bool BinaryAudioPlayWorldContext(bool loop) noexcept;
void BinaryDisplayRandomObserved(unsigned value,const void *return_address) noexcept;
void BinaryBannerTextObserved(const void *text,bool subtitle) noexcept;
// Q7实际显示白名单由Client与Host共用；混合子根暂挂显示身份，退出自动恢复。
class BinaryDisplayPathScope final {
public:
  explicit BinaryDisplayPathScope(unsigned entry) noexcept;
  ~BinaryDisplayPathScope() noexcept;
private:
  unsigned previous_=0;
};
bool BinaryInDisplayPath() noexcept;
// 正常音乐尾部的唯一Play调用是Queue消费；scope只观察，不额外执行tick。
class BinaryMusicTickScope final {
public:
  BinaryMusicTickScope() noexcept;
  ~BinaryMusicTickScope() noexcept;
private:
  bool active_=false;
};
bool BinaryMusicQueueAdvance() noexcept;
}
