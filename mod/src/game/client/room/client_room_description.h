#pragma once
#include "game/client/common/client_object_key.h"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
namespace cs::state {class ClientStateMemory;class ClientShadowState;}

namespace cs::game {
// 角色只用于索引同源生成表；不把字段名、布局号或网络偏移当执行参数。
enum class ClientRoomFieldRole : std::uint8_t {
  DescriptorSlot, Dimension, RoomType, GridWidth, GridHeight, Bounds,
  FrameBaseline, GreedTimer, FloorColor, WallColor, WaterAmount, LavaIntensity, RenderScroll,
  DoorMembers, DoorPositions, Backdrop, FXLayers, Water, Lighting, Waves, ForcedFocus, Rails,
  HellBackdrop, WaveSpritePresent, WaveSprite,
  Stage, StageType, EffectiveCurses, CurrentEntry, CurrentDimension, GreedWave, CurrentIsLastBoss,
  Occupancy, CurrentVisual,
  GridIndex, SafeGridIndex, DescriptorDimension, Data, OverrideData, AllowedDoors, DoorTargets,
  DisplayFlags, Visited, DisplayBits, GreedTreasureIcon, SavedIcons,
  ConfigStageId, ConfigType, ConfigSubtype, ConfigWidth, ConfigHeight, ConfigShape,
  RawCurses, SpawnSeed, WaterStart, WaterMultiplierStart, WaterTarget, WaterMultiplierTarget,
  WaterLerp, ShockwaveA, ShockwaveB, Count
};
enum class ClientWorldDescriptionKind : std::uint8_t { Room, Level, Descriptor, Configuration };
// 第12步接受的同一完整world上下文；显示批次修订与GameFrame分别保留，不能互相替代。
struct ClientRoomLevelContext final {
  ClientObjectKey floor{},room{};
  std::uint64_t world_revision=0,presentation_revision=0;
  std::uint32_t member_epoch=0,resource_epoch=0,target_version=0,game_frame=0;
  std::uint16_t layout_version=0;
  bool operator==(const ClientRoomLevelContext &) const noexcept=default;
};
struct ClientWorldShadowIdentity final {
  ClientRoomLevelContext context;
  ClientObjectKey key{};
  std::uint64_t use=0,shape=0;
  std::uint16_t member=UINT16_MAX,layout=0;
};
// 各块版本是最后变化版本，允许早于target_version；输入字节只借用到生成函数返回。
struct ClientWorldShadowBlockView final {
  ClientObjectKey key{};
  std::uint64_t use=0,shape=0;
  std::uint32_t version=0,size=0;
  std::uint16_t layout=0;
  const std::uint8_t *data=nullptr;
  bool present=false;
};
struct ClientWorldShadowRecordView final {
  ClientWorldShadowIdentity identity;
  std::array<ClientWorldShadowBlockView,32> blocks{};
};
// source_slot是0..526的物理存储槽；ordinary_ordinal仅表达有序普通成员，不重建原ListIndex。
struct ClientRoomDescriptorShadowView final {
  std::uint16_t source_slot=UINT16_MAX,ordinary_ordinal=UINT16_MAX;
  ClientWorldShadowRecordView record;
};
// 只提供本次描述引用的已接受typed配置项；明确Null不用建项，Wait／Gap不能给空payload。
struct ClientRoomConfigShadowView final {
  std::uint32_t resource_epoch=0,size=0;
  std::uint16_t resource=UINT16_MAX,layout=0,layout_version=0;
  const std::uint8_t *data=nullptr;
};
// 已接受的当前Room Grid成员关系；这里只做结构引用绑定，不读取或构造原生Grid。
struct ClientRoomGridMember final {
  ClientObjectKey key{},room{};
  std::uint64_t use=0,shape=0;
  std::uint16_t member=UINT16_MAX,layout=0,index=UINT16_MAX;
};
struct ClientRoomLevelShadowView final {
  ClientWorldShadowRecordView room,level;
  std::span<const ClientRoomDescriptorShadowView> descriptors;
  std::span<const ClientRoomConfigShadowView> configurations;
  std::span<const ClientRoomGridMember> grids;
  // 正式流借精确影子块；独立输入继续由描述构造函数拷贝。
  std::shared_ptr<const state::ClientShadowState> backing;
};
// 不含Name／Variant／权重／Spawn／Merged、原生指针或可触发选房的参数。
struct ClientRoomConfiguration final {
  std::uint16_t resource=UINT16_MAX;
  std::int32_t stage_id=0,type=0,subtype=0;
  std::int8_t width=0,height=0;
  std::uint8_t shape=0;
};
struct ClientRoomValues final {
  std::int32_t descriptor_slot=0,dimension=0,greed_timer=0;
  std::uint32_t type=0,grid_width=0,grid_height=0,frame_baseline=0;
  std::array<float,4> bounds{};
  std::array<float,11> floor_color{},wall_color{};
  float water_amount=0,lava_intensity=0;
  std::array<float,2> render_scroll{};
  std::array<std::uint16_t,8> door_members{};
  std::array<std::int32_t,8> door_positions{};
};
struct ClientLevelValues final {
  std::uint32_t stage=0,stage_type=0,raw_curses=0;
  std::int32_t current_entry=0,greed_wave=0;
  std::uint8_t dimension=0;
  bool current_is_last_boss=false;
};
struct ClientRoomDescriptorValues final {
  std::int32_t grid=0,safe_grid=0;
  std::uint8_t dimension=0,allowed_doors=0,display_bits=0;
  std::uint32_t display_flags=0,spawn_seed=0;
    bool visited=false,greed_treasure_icon=false,spawn_seed_present=false;
  std::array<std::int32_t,8> doors{};
};
// 静态帧0由mask类别表示，动态帧保留于对应bit位置；不是原生保存槽列表。
struct ClientSavedMapIcons final {
  bool present=false;
  std::uint16_t mask=0;
  std::array<std::uint8_t,16> frames{};
};
struct ClientWorldBytes final {const std::uint8_t *data=nullptr;std::uint32_t size=0;};
struct ClientRoomLevelStorage;
class ClientRoomDescription;
class ClientLevelDescription;
class ClientRoomDescriptorDescription;
class ClientRoomLevelDescription;

// 显示／容器投影共享自有字节，不建字段树；第13步按可信数字布局递归消费。
class ClientRoomProjection final {
public:
  ClientRoomProjection() noexcept=default;
  ClientWorldBytes Bytes() const noexcept;
  std::uint16_t RootLayout() const noexcept;
  std::uint16_t FieldIndex() const noexcept;
  std::uint16_t NestedLayout() const noexcept;
  std::uint8_t NestedBlock() const noexcept;
  std::uint32_t Version() const noexcept;
private:
  friend class ClientRoomDescription;
  friend class ClientLevelDescription;
  friend class ClientRoomDescriptorDescription;
  ClientRoomProjection(std::shared_ptr<const ClientRoomLevelStorage>,ClientWorldDescriptionKind,
                       std::uint16_t,ClientRoomFieldRole) noexcept;
  std::shared_ptr<const ClientRoomLevelStorage> storage_;
  ClientWorldDescriptionKind kind_=ClientWorldDescriptionKind::Room;
  std::uint16_t record_=0;
  ClientRoomFieldRole role_=ClientRoomFieldRole::Count;
};
class ClientRoomDescriptorDescription final {
public:
  const ClientWorldShadowIdentity &Identity() const noexcept;
  std::uint16_t SourceSlot() const noexcept;
  std::optional<std::uint16_t> OrdinaryOrdinal() const noexcept;
  ClientRoomDescriptorValues Values() const;
  std::optional<ClientRoomConfiguration> Configuration(bool override_data=false) const;
  ClientSavedMapIcons SavedIcons() const;
  std::optional<ClientRoomProjection> Field(ClientRoomFieldRole) const noexcept;
private:
  friend class ClientLevelDescription;
  friend class ClientRoomDescription;
  friend class ClientRoomLevelDescription;
  friend ClientRoomLevelDescription ClientDescribeRoomLevel(const ClientRoomLevelShadowView &,std::uint32_t,std::shared_ptr<state::ClientStateMemory>);
  ClientRoomDescriptorDescription(std::shared_ptr<const ClientRoomLevelStorage>,std::uint16_t) noexcept;
  std::shared_ptr<const ClientRoomLevelStorage> storage_;
  std::uint16_t record_=0;
};
class ClientRoomDescription final {
public:
  const ClientWorldShadowIdentity &Identity() const noexcept;
  ClientRoomValues Values() const;
  ClientRoomDescriptorDescription Descriptor() const noexcept;
  std::optional<ClientRoomGridMember> Door(std::uint8_t slot) const;
  std::optional<ClientRoomProjection> Field(ClientRoomFieldRole) const noexcept;
private:
  friend class ClientRoomLevelDescription;
  explicit ClientRoomDescription(std::shared_ptr<const ClientRoomLevelStorage>) noexcept;
  std::shared_ptr<const ClientRoomLevelStorage> storage_;
};
class ClientLevelDescription final {
public:
  const ClientWorldShadowIdentity &Identity() const noexcept;
  ClientLevelValues Values() const;
  std::uint16_t OrdinaryCount() const noexcept;
  ClientRoomDescriptorDescription OrdinaryAt(std::uint16_t ordinal) const;
  std::optional<ClientRoomDescriptorDescription> DescriptorAt(std::uint8_t dimension,std::uint16_t grid) const;
  std::optional<ClientRoomDescriptorDescription> DescriptorBySourceSlot(std::uint16_t) const;
  std::optional<ClientRoomProjection> Field(ClientRoomFieldRole) const noexcept;
private:
  friend class ClientRoomLevelDescription;
  explicit ClientLevelDescription(std::shared_ptr<const ClientRoomLevelStorage>) noexcept;
  std::shared_ptr<const ClientRoomLevelStorage> storage_;
};
// 新正式描述与Legacy JSON DTO没有转换；复制它只共享同一Values池租约。
class ClientRoomLevelDescription final {
public:
  const ClientRoomLevelContext &Context() const noexcept;
  ClientRoomDescription Room() const noexcept;
  ClientLevelDescription Level() const noexcept;
  std::uint16_t DescriptorCount() const noexcept;
  ClientRoomDescriptorDescription DescriptorAt(std::uint16_t) const;
  const void *StorageOwner() const noexcept;
  std::uint64_t StorageBytes() const noexcept;
  long StorageReferences()const noexcept;
private:
  friend ClientRoomLevelDescription ClientDescribeRoomLevel(const ClientRoomLevelShadowView &,std::uint32_t,std::shared_ptr<state::ClientStateMemory>);
  explicit ClientRoomLevelDescription(std::shared_ptr<const ClientRoomLevelStorage>) noexcept;
  std::shared_ptr<const ClientRoomLevelStorage> storage_;
};
// 输入必须已完成第12步递归字段／资源校验；此处核根结构及同world关系，取得自有不可变副本。
// byte_limit只能收紧32MiB完整world负载上限；不执行原生写入、资源加载或AppliedAck。
ClientRoomLevelDescription ClientDescribeRoomLevel(const ClientRoomLevelShadowView &,std::uint32_t byte_limit=32u<<20,std::shared_ptr<state::ClientStateMemory> memory={});
} // namespace cs::game
