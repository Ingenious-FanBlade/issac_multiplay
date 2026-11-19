#pragma once
namespace cs::game {
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
bool ClientNativeDisplayCallbacks() noexcept;
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
