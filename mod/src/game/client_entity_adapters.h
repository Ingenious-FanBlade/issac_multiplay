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
std::vector<ClientEntityClassAdapter> ClientEntityBuiltinAdapters();
const char *ClientEntityClassDisposition(const std::string &) noexcept;
// 复杂容器沿同一类别步骤写入/回读；不调用CalculateSamples、Update或玩法setter。
bool ClientEntityCollections(void *,const ClientObjectDescription &,bool,const char *&);
bool ClientEntityOwnedPresentation(void *,const ClientObjectDescription &,bool,const char *&);
void *ClientEntityOwnedSprite(void *,unsigned,unsigned);
bool ClientEntityRetireOwnedPresentation(void *,unsigned,const char *&);
} // namespace cs::game
