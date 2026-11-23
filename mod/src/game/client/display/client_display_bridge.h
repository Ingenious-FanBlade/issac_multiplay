#pragma once
#include "replica/nightmare.h"
#include <cstdint>
namespace cs::game {
// 仅在 Host 的 Manager 原生过场绘制栈内观察图层，原函数照常执行。
class NightmareCaptureScope final {
public:
  explicit NightmareCaptureScope(void *scene) noexcept;
  ~NightmareCaptureScope() noexcept;
  void Returned() noexcept {returned_=true;}
private:
  bool active_=false,returned_=false;
};
void CaptureNightmareLayer(void *layer,int frame,void *sprite,const void *position,
                           const void *top_left,const void *bottom_right) noexcept;
replica::NightmareFrameRef HostNightmareFrame() noexcept;
bool HostNightmareActive() noexcept;
std::uint64_t HostNightmareSession() noexcept;
bool RenderNightmareFrame(const replica::NightmareFrame &) noexcept;
void ReleaseNightmareSprites() noexcept;
// Hook生成单元维持C++17；桥只传用途状态，不暴露span、模型或原生所有权。
class ClientMapMirrorScope final {
public:
  explicit ClientMapMirrorScope(void *configuration) noexcept;
  ~ClientMapMirrorScope() noexcept;
  bool Ready() const noexcept {return ready_;}
private:void *state_=nullptr;bool ready_=false;
};
class ClientGameRenderScope final {
public:
  explicit ClientGameRenderScope(void *game) noexcept;
  ~ClientGameRenderScope() noexcept;
  bool CallOriginal() const noexcept {return call_;}
private:bool active_=false,call_=false;int exceptions_=0;
};
// Projectile::Render 暂改 PositionOffset，离开正常调用时按原位恢复。
class ClientProjectileRenderScope final {
public:
  explicit ClientProjectileRenderScope(void *projectile) noexcept;
  ~ClientProjectileRenderScope() noexcept;
  bool Ready() const noexcept {return ready_;}
  void Returned() noexcept {returned_=true;}
private:
  void *projectile_=nullptr,*room_=nullptr;
  unsigned vptr_=0,index_=0,birth_=0,seed_=0,bits_=0;
  bool ready_=true,returned_=false;
};
bool ClientNativeDisplayActive() noexcept;
bool ClientNativeDisplayPaused(bool &) noexcept;
// 已安装 World 的 Host 终结分支（state::DisplayBranch 数值）；未安装／失效时返回 false。
bool ClientNativeDisplayBranch(unsigned &) noexcept;
// 客机本地暂停菜单的原生调用作用域内为真：菜单音效可在本机播放。
bool ClientPauseMenuSoundAllowed() noexcept;
// 客机获准执行的原生显示 Lua 回调内，可以发出本机单次音效。
bool ClientDisplayLuaSoundAllowed() noexcept;
bool ClientNativeDisplayCallbacks() noexcept;
bool ClientHudUpdateCallbacks() noexcept;
bool ClientHudPostUpdateCallbacks() noexcept;
bool ClientPauseRenderCallbacks() noexcept;
void ClientDisplayMusicQueued(const void *owner,int id) noexcept;
void ClientBinaryRenderOverlays(const void *global) noexcept;
void ClientDisplaySfxRefreshed(void *owner) noexcept;
bool ClientDisplayMapRandom(const void *site,unsigned maximum,unsigned &result) noexcept;
bool ClientDisplayFxRandom(const void *site,unsigned &result) noexcept;
bool ClientDisplayBossAudioSuppressed(const void *site) noexcept;
bool ClientDisplayDealChance(const void *room,float &) noexcept;
bool ClientDisplayAngelShare(float &) noexcept;
bool ClientDisplayDealQualified(const void *game,bool &) noexcept;
bool ClientDisplayProgressBlocked(const void *site,bool &) noexcept;
} // namespace cs::game
