#include "IsaacRepentance.h"
#include "game/observation/binary_intentions.h"
#include "game/common/bridge.h"
#include "runtime.h"
#include "binding/offsets.h"
#include <cmath>
#include <cstring>

namespace cs::game {
namespace {
template<class T> T Read(const void *p,unsigned offset){T v;std::memcpy(&v,static_cast<const char *>(p)+offset,sizeof(v));return v;}
bool MusicOwner(void *p){return IsMain()&&!ClientBusy()&&BinaryIntentActive()&&g_Manager&&p==&g_Manager->_musicmanager;}
bool SfxOwner(void *p){return IsMain()&&!ClientBusy()&&BinaryIntentActive()&&g_Manager&&g_Game&&p==&g_Manager->_sfxManager;}
}
// 本层只观察原函数已正常完成的参数／原生选择，不创建临时world凭据或补跑生产器。
std::uint32_t BinaryNativeMusicCurrent(void *object) noexcept {return MusicOwner(object)?Read<std::uint32_t>(object,0x30c):UINT32_MAX;}
void BinaryNativeMusicFallback(BinaryProductionKind kind,void *object,const std::array<std::uint32_t,5> &args,std::uint32_t previous) noexcept {
  if(!MusicOwner(object)||previous==UINT32_MAX)return;
  using K=BinaryProductionKind;BinaryMusicIntent intent;intent.kind=kind;intent.args=args;intent.previous=int(previous);
  intent.current=Read<int>(object,0x30c);intent.queued=Read<int>(object,0x310);
  if(kind==K::MusicPlay||kind==K::MusicCrossfade||kind==K::MusicFadein||kind==K::MusicJingle||kind==K::MusicQueue){
    const auto first=Read<std::uint32_t>(object,0x31c),last=Read<std::uint32_t>(object,0x320);const auto id=int(args[0]);
    if(first>last||(last-first)%96||id<0||std::uint32_t(id)>=(last-first)/96)return;
    const auto *entry=reinterpret_cast<const void *>(first+std::uint32_t(id)*96);
    if(kind!=K::MusicQueue&&!(kind==K::MusicCrossfade&&previous==args[0])&&!Read<std::uint32_t>(entry,68))return;
    if(kind==K::MusicJingle){if(previous==52||previous==118)return;}
    else if(kind==K::MusicQueue){if(intent.queued!=id)return;}
    else if(intent.current!=id)return;
  }
  BinaryMusicIntentObserved(intent);
}
void BinaryNativeSfxFallback(BinaryProductionKind kind,void *object,int id,float volume,int delay,bool loop,float pitch,float pan,void *stream,int ambient) noexcept {
  if(!SfxOwner(object)||!std::isfinite(volume)||!std::isfinite(pitch)||!std::isfinite(pan))return;
  BinarySfxIntent intent;intent.kind=kind;intent.id=id;intent.volume=volume;intent.delay=delay;intent.loop=loop;intent.pitch=pitch;intent.pan=pan;intent.ambient=ambient;
  intent.frame=Read<std::uint32_t>(g_Game,0x25a70);
  if(kind==BinaryProductionKind::SfxPlay){if(!stream||id<0)return;
    const auto first=Read<std::uint32_t>(object,0),last=Read<std::uint32_t>(object,4);
    if(first>last||(last-first)%440||std::uint32_t(id)>=(last-first)/440)return;
    const auto effect=first+std::uint32_t(id)*440,selected=std::uint32_t(reinterpret_cast<std::uintptr_t>(stream));
    const auto samples=Read<std::uint32_t>(reinterpret_cast<void *>(effect),400);
    if(!samples||samples>7||selected<effect+32||(selected-effect-32)%56||(selected-effect-32)/56>=samples)return;
    const auto *sample=reinterpret_cast<const void *>(selected-24);const auto size=Read<std::uint32_t>(sample,16),capacity=Read<std::uint32_t>(sample,20);
    if(!size||size>=intent.path.size()||size>capacity||(capacity<16&&capacity!=15))return;
    const auto *text=capacity<16?static_cast<const char *>(sample):Read<const char *>(sample,0);if(!text)return;
    std::memcpy(intent.path.data(),text,size);intent.path[size]=0;intent.path_size=size;intent.stream=selected;
  }
  BinarySfxIntentObserved(intent);
}
// J273 静态：PauseScreen::Show 98ED80 先写 state=1 再 Play(17)；关闭 98EC00 在 state!=0 时 Play(18)，
// state 由后续 Update 在消失动画后清零；选项输入 989970 只在 state=2 运行。三者的一次性音效都在 state!=0 窗口内。
// 暂停页打开时 Game 处于暂停分支，不运行实体／房间玩法更新，因此此窗口内的一次性 Play 只可能来自本机菜单。
bool BinaryHostPauseMenuOpen() noexcept {
  if(!IsMain()||ClientBusy()||!g_Game)return false;
  return Read<std::int32_t>(g_Game,cs::offset::PauseScreen)!=0;
}
bool BinaryAudioPlayWorldContext(bool loop) noexcept {
  // Show 内部的 StopLoops 走独立控制入口，仍作为 world 停止发给 Client；这里只排除一次性菜单声。
  return BinaryAudioWorldContext()&&(loop||!BinaryHostPauseMenuOpen());
}
void BinaryNativeMapFocusFallback(void *object,std::uint32_t selected) noexcept {
  if(!IsMain()||ClientBusy()||!BinaryIntentActive()||!g_Game)return;
  const auto game=reinterpret_cast<std::uintptr_t>(g_Game),owner=reinterpret_cast<std::uintptr_t>(object);
  if(owner!=game+0x2545c&&owner!=game+0x255bc)return;
  const auto count=Read<std::uint32_t>(g_Game,0x182cc);if(selected>=count||count>527)return;
  const auto room=Read<std::uint32_t>(g_Game,0x18300);if(!room)return;
  const auto descriptor=Read<std::uint32_t>(reinterpret_cast<void *>(room),4);if(!descriptor)return;
  const auto focus=Read<std::uint32_t>(g_Game,36+selected*184+4);
  BinaryMapFocusObserved(std::uint32_t(owner),descriptor,Read<std::uint32_t>(g_Game,0x25a70),focus);
}
void BinaryNativeOverlayFallback(void *object,const std::array<std::uint32_t,5> &args) noexcept {
  if(!IsMain()||ClientBusy()||!BinaryIntentActive()||!g_Game||!object||args[0]>46)return;
  const auto state=Read<int>(object,0),id=Read<int>(object,4);
  if(state<1||state>2||id!=int(args[0]))return;
  const bool global=reinterpret_cast<std::uintptr_t>(object)==reinterpret_cast<std::uintptr_t>(g_Game)+0x1c034;
  if(!global&&!args[2])return;
  const auto controller=global?-1:Read<int>(reinterpret_cast<void *>(args[2]),0x160c);
  BinaryOverlayObserved(std::uint32_t(reinterpret_cast<std::uintptr_t>(object)),controller);
}
void BinaryNativeBossFallback(void *object,const std::array<std::uint32_t,5> &args) noexcept {
  if(!IsMain()||ClientBusy()||!g_Game||object!=&g_Game->_roomTransition||!args[0]||Read<unsigned>(object,0x238)!=args[0])return;
  BinaryBossObserved(std::uint32_t(reinterpret_cast<std::uintptr_t>(object)),args[0],args[1],Read<int>(object,0x14),Read<int>(object,0x18));
}
}
