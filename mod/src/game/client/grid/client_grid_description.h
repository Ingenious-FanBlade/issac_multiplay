#pragma once
#include "game/client/common/client_object_key.h"
#include "game/client/resources/client_resources.h"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
namespace cs::state {class ClientStateMemory;class ClientShadowState;}

namespace cs::game {
// 语义角色在生成时解析为真实布局字段索引；运行访问不比较字符串或猜原生偏移。
enum class ClientGridFieldRole : std::uint8_t {
  Descriptor, GridIndex, WaterClipFlags,
  MainAppearance, MainAnimation, MainLayers,
  FireAppearance, FireAnimation, FireLayers,
  EmberAppearance, EmberAnimation, EmberLayers,
  TimerAppearance, TimerAnimation, TimerLayers,
  ExtraAppearance, ExtraAnimation, ExtraLayers,
  Embers, RubbleAnimation, RubbleOffsets,
  TargetRoomType, ExtraVisible, TargetRoomIndex, DoorLightBit, Count
};
enum class ClientGridSpriteRole : std::uint8_t { Main, Fire, Ember, TimerPlate, DoorExtra };
// 输入是第12步已递归校验并解析到同一成员形态的完整影子记录，不是网络增量包。
struct ClientGridShadowIdentity final {
  ClientObjectKey key{}, room{};
  std::uint64_t use=0, shape=0;
  std::uint32_t member_epoch=0, resource_epoch=0, target_version=0, game_frame=0;
  std::uint16_t member=UINT16_MAX, layout=0, layout_version=0;
};
// 块版本是最后变化版本，可以早于当前目标版本；输入字节只借用到生成函数返回。
struct ClientGridShadowBlockView final {
  ClientObjectKey key{};
  std::uint64_t use=0, shape=0;
  std::uint32_t version=0, size=0;
  std::uint16_t layout=0;
  const std::uint8_t *data=nullptr;
  bool present=false;
};
struct ClientGridShadowRecordView final {
  ClientGridShadowIdentity identity;
  std::array<ClientGridShadowBlockView,32> blocks{};
  std::shared_ptr<const state::ClientShadowState> backing;
};
// 字节借用的寿命不超过持有它的描述／投影；不含原生对象、资源或容器地址。
struct ClientGridBytes final { const std::uint8_t *data=nullptr; std::uint32_t size=0; };
struct ClientGridShadowStorage;
class ClientGridDescription;

// 投影共享自有不可变字节。nested_layout／nested_block来自可信生成表，供第13步递归消费。
class ClientGridProjection final {
public:
  ClientGridProjection() noexcept=default;
  ClientGridBytes Bytes() const noexcept;
  std::uint16_t RootLayout() const noexcept;
  std::uint16_t FieldIndex() const noexcept;
  std::uint16_t NestedLayout() const noexcept;
  std::uint8_t NestedBlock() const noexcept;
  std::uint32_t Version() const noexcept;
private:
  friend class ClientGridDescription;
  ClientGridProjection(std::shared_ptr<const ClientGridShadowStorage>, ClientGridFieldRole) noexcept;
  std::shared_ptr<const ClientGridShadowStorage> storage_;
  ClientGridFieldRole role_=ClientGridFieldRole::Count;
};
struct ClientGridSpriteDescription final {
  std::uint16_t layout=0;
  std::array<ClientGridProjection,3> projections;
  // Statue216合法为零负载；这不是loaded/Null判断，也不创造本地资源就绪资格。
  bool HasPayload() const noexcept;
};
struct ClientGridDescriptor final {
  std::int32_t type=0, variant=0, state=0;
  // PressurePlate／Door的224描述没有此字段；缺席保持缺席，不用0填充。
  std::optional<std::uint32_t> spawn_seed;
};
struct ClientGridDoorDescription final {
  std::int32_t target_room_type=0, target_room_index=0;
  bool extra_visible=false, light_bit=false;
};
// Fire只保留16B线上元素；速度属于Host，不与旧24B原生元素混用。
struct ClientGridEmberDescription final { float x=0,y=0; std::int32_t age=0,max_life=0; };

// 新正式描述只从影子状态生成；复制描述共享不可变负载，不恢复旧字符串fields字典。
class ClientGridDescription final {
public:
  const ClientGridShadowIdentity &Identity() const noexcept;
  ClientGridDescriptor Descriptor() const;
  std::uint32_t Index() const;
  std::uint32_t WaterClipFlags() const;
  std::optional<ClientGridProjection> Field(ClientGridFieldRole) const noexcept;
  std::optional<ClientGridSpriteDescription> Sprite(ClientGridSpriteRole) const noexcept;
  std::optional<ClientGridDoorDescription> Door() const;
  std::uint16_t EmberCount() const;
  ClientGridEmberDescription EmberAt(std::uint16_t) const;
  std::uint16_t RubbleCount() const;
  ClientPoseVector RubbleAt(std::uint16_t) const;
  // 返回同一负载owner及其计费量；汇总计量用owner去重，复制描述不重复收费。
  const void *StorageOwner() const noexcept;
  std::uint64_t StorageBytes() const noexcept;
  long StorageReferences()const noexcept;
private:
  friend ClientGridDescription ClientDescribeGrid(const ClientGridShadowRecordView &, std::uint32_t,std::shared_ptr<state::ClientStateMemory>);
  explicit ClientGridDescription(std::shared_ptr<const ClientGridShadowStorage>) noexcept;
  std::shared_ptr<const ClientGridShadowStorage> storage_;
};
// 根记录结构检查与纯值描述生成；不替代第12步ANM2／资源递归校验，不写世界、不回ACK。
// 上限只能收紧32MiB world消息预算；所有自有负载同时计入现有Values池。
ClientGridDescription ClientDescribeGrid(const ClientGridShadowRecordView &, std::uint32_t byte_limit=32u<<20,std::shared_ptr<state::ClientStateMemory> memory={});
} // namespace cs::game
