#pragma once
#include "game/client_objects_native.h"

namespace cs::game {
// 只允许目录中具名公开成员；不是可以从网络调用任意 Lua 方法的 RPC。
enum class ClientGridValueKind : unsigned { Integer, Boolean, Text };
struct ClientGridField final {
  unsigned cast=0;
  std::string name,text;
  ClientGridValueKind kind=ClientGridValueKind::Integer;
  int integer=0;
  bool boolean=false;
  bool null_text=false;
};
// 资源仍由原生内嵌 ANM 拥有；TimerPlate 的 Null 与空资源分开。
struct ClientGridSprite final {
  unsigned role=0; // 0=主、1=Door extra、2=TimerPlate、3/4=Fire 专用。
  bool present=false;
  ClientResourcePose pose;
};
struct ClientGridEmber final {float x=0,y=0,vx=0,vy=0;int age=0,max_life=0;};
// 保存描述八项保持原值；本地构造完成与来源 Initialized 没有相互推导关系。
struct ClientGridDescription final {
  ClientObjectKey key{},room{};
  std::uint64_t revision=0;
  int index=0,type=0,variant=0,state=0,var_data=0,collision=0;
  int desc_state=0,desc_var_data=0,spawn_count=0;
  unsigned spawn_seed=0,variable_seed=0,water_clip=0,cast_mask=0;
  unsigned spawn_frame=0;
  bool source_initialized=false,grid_member=false;
  unsigned door_aliases=0;
  ClientPoseVector position{},render_position{};
  std::vector<ClientGridField> fields;
  std::vector<ClientGridSprite> sprites;
  int big_rock_frame=0;
  int fire_timer=0,fire_style=0;
  bool door_animation_mode=false;
  unsigned door_light_flags=0;
  std::vector<ClientGridEmber> embers;
  std::vector<ClientPoseVector> rubble;
};
enum class ClientGridOperation : unsigned { Remove, Create, Change, Replace, BindDoors, Check };
struct ClientGridCommand final {
  ClientGridOperation operation=ClientGridOperation::Check;
  ClientGridDescription value;
};
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
ClientGridStep ClientNativeGridsExecute(ClientNativeGrids *,const ClientNativeRoots &,const ClientGridCommand &) noexcept;
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
