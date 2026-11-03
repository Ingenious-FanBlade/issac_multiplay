#pragma once
#include "game/client_objects_native.h"
namespace cs::replica { struct EntityObjectTarget; }
namespace cs::game {
// 类别目标只含自有值；raw存储偏移来自共同声明，不接受网络提供的偏移/函数地址。
struct ClientEntityClassState final {
  std::vector<ClientEntityField> fields;
  struct Resource final {std::string member;unsigned index=0;bool is_null=false;ClientResourcePose pose;};
  std::vector<Resource> resources;
  bool delirium=false;
  // NPC复合绘制的两份原生deque用纯值重建；关系槽按稳定Key两阶段绑定。
  unsigned npc_composite_count=0;
  std::vector<ClientPoseVector> npc_path,npc_secondary_path;
  ClientPoseVector player_minecart_offset{};
  std::uint32_t player_minecart_sort_bits=0; // Y偏移还被原生按i32位模式排序，保留负零等位模式。
  bool player_minecart_draw=false;
  // 官方EntityPlus的optional<bool>：-1=Null，0=false，1=true，不能把Null压成false。
  int flying_override=-1;
  // 激光四份容器各自拥有存储；Bezier为三对已核浮点值，不含原生地址或容器头。
  struct LaserGeometry final {
    ClientPoseVector origin{},end{};
    float parameter=0,spacing=0;
    std::uint32_t sample_count=0;
    std::int32_t cursor=0;
    std::vector<ClientPoseVector> optimized,unoptimized;
    std::vector<std::int32_t> indices;
    std::vector<std::array<float,6>> bezier;
  };
  std::shared_ptr<const LaserGeometry> laser;
  // 循环道具是有界完整后态，空集合会清除旧内容；Flip玩法保存状态不在客户端重演。
  std::vector<std::uint32_t> pickup_cycle;
  bool effect_tear_flags=false;
  // 举起描述属于Effect136内嵌值；不拥有活动Grid，不按VarData/flags别名覆盖。
  struct LiftedDescriptor final {
    std::int32_t type=0,variant=0,state=0,spawn_count=0,var_data=0;
    std::uint32_t spawn_seed=0,variable_seed=0;
    bool initialized=false;
  } lifted;
  bool has_lifted=false;
  // Text保留原始字节及完整KColor20；不通过Update生成字符串或倒计时。
  std::string text;
  std::array<float,4> text_color{};
  std::int32_t text_color_extra=0;
  // 普通采集窗口的表现输入，与独立Render事件各自保持scope。
  struct DrawInputs final {
    std::uint32_t manager_frame=0,game_frame=0;
    unsigned subframe=0;
    std::array<std::uint32_t,4> pickup_random{},familiar_random{};
    bool present=false,paused=false;
  } draw;
  // 私有表现payload按真实动态类型建模；源地址/vptr/引用计数和容器头均不进入协议。
  struct OwnedPresentation final {
    enum Kind : unsigned { None, MrMe, SpriteTrail, PickupGhost, IsaacsBody, AbyssLocust };
    Kind kind=None;
    bool authority_only=false;
    unsigned count=0;
    std::vector<std::array<std::uint32_t,16>> trail;
    std::array<std::int32_t,4> body_indices{};
    unsigned sprites_a=0,sprites_b=0;
  } owned;
};
// 模型阶段一次解释完整派生payload，原生步骤仅消费此不可变结果。
void ClientDescribeEntityClass(const replica::EntityObjectTarget &,ClientObjectDescription &);
void ClientDescribeEntityPlayerDraw(const replica::EntityObjectTarget &,ClientObjectDescription &);
std::vector<ClientEntityClassAdapter> ClientEntityBuiltinAdapters();
const char *ClientEntityClassDisposition(const std::string &) noexcept;
// 复杂容器沿同一类别步骤写入/回读；不调用CalculateSamples、Update或玩法setter。
bool ClientEntityCollections(void *,const ClientObjectDescription &,bool,const char *&);
bool ClientEntityOwnedPresentation(void *,const ClientObjectDescription &,bool,const char *&);
void *ClientEntityOwnedSprite(void *,unsigned,unsigned);
bool ClientEntityRetireOwnedPresentation(void *,unsigned,const char *&);
void *ClientEntityAuxiliarySprite(void *,const ClientEntityClassState::Resource &);
bool ClientEntityNpcCollections(void *,const ClientObjectDescription &,bool,const char *&);
bool ClientEntityNpcRelation(void *,const ClientEntityRelation &,void *,bool,bool,const char *&);
bool ClientEntityNpcRetire(void *,const char *&);
} // namespace cs::game
