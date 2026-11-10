#pragma once
#include "game/client/grid/client_grid_description.h"
#include "game/client/grid/client_grid_legacy.h"

namespace cs::game {
// 每个门槽分别保存候选、允许位、实际主体及路由；禁止把候选自动生成成门。
struct ClientGridDoorBinding final {
  ClientObjectKey actual{},descriptor{};
  int slot=0,candidate=0,target=0,route=0;
  bool is_null=true,shape_allowed=false,enabled=false,descriptor_null=true;
};
// 前缀在写前增加；异常后不重复调用已进入的构造/析构，交回场景安全退休。
struct ClientGridStep final {
  bool done=false,written=false,failed=false;
  unsigned prefix=0,owners=0;
  const char *reason="grid_waiting";
  ClientObjectKey key{};
};
struct ClientNativeGrids;
ClientNativeGrids *ClientNativeGridsCreate() noexcept;
bool ClientNativeGridsBeginAttempt(ClientNativeGrids *) noexcept;
void ClientNativeGridsDestroy(ClientNativeGrids *) noexcept;
std::uint64_t ClientNativeGridsValueBytes(const ClientNativeGrids *);
ClientGridStep ClientNativeGridsExecute(ClientNativeGrids *,const ClientNativeRoots &,const LegacyClientGridCommand &) noexcept;
ClientGridStep ClientNativeGridsBindDoors(ClientNativeGrids *,const ClientNativeRoots &,const std::array<ClientGridDoorBinding,8> &) noexcept;
ClientGridStep ClientNativeGridsRetire(ClientNativeGrids *,const ClientNativeRoots &) noexcept;
using ClientGridVisitor=bool (*)(void *,void *,const ClientObjectKey &,std::uint64_t,unsigned) noexcept;
bool ClientNativeGridsVisit(const ClientNativeGrids *,void *,ClientGridVisitor) noexcept;
// 整个draw栈保护含water内部调用；暂借深拷贝索引，避免SetAnimation释放原数组。
class ClientGridDrawScope final {
public:
  ClientGridDrawScope(const ClientNativeRoots &,const std::array<int,5> &) noexcept;
  ~ClientGridDrawScope();
  bool Ready() const noexcept {return ready_;}
  ClientGridDrawScope(const ClientGridDrawScope &)=delete;
  ClientGridDrawScope &operator=(const ClientGridDrawScope &)=delete;
private:void *state_=nullptr;bool ready_=false;
};
enum class ClientGridPass : unsigned { Light, Decoration, Door, Under, Main, Dungeon, PillarRow, PillarEnd, TexturedPitPre, TexturedPit };
bool ClientGridDraw(const ClientNativeRoots &,ClientGridPass,const ClientPoseVector &,unsigned row=0) noexcept;
bool ClientGridMembersMatch(void *room) noexcept;
// 统一Hook清单中五个只读查询仅在Grid灯光栈内返回正式来源，其它调用原样透传。
bool ClientGridLightQuery(unsigned query,const void *object,unsigned argument,std::int64_t &result) noexcept;
// 六个原生 WaterClipInfo wrapper 原链各一次后调用；按实例解释有效值，兼容同 index 门/装饰。
void ClientGridWaterClip(void *grid,void *out) noexcept;
// 只读正式来源：复制原生构造后、公开 API 未暴露但本地承载需要的具名状态。
int ClientGridCaptureLua(void *);
int ClientGridSpritesLua(void *);
} // namespace cs::game
