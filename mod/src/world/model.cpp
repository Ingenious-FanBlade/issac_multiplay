// tx-04-08 纯值模型：名称表、profile 声明、预算账本与记录清理。
// 本单元不读取游戏内存、不打开文件、不创建线程。

#include "world/model.h"

#include <algorithm>
#include <cstring>

namespace cs::world {
namespace {

// 把 0 起始的连续 ID 展开成定长数组，供类别允许集合拼接使用。
template <std::size_t N>
constexpr std::array<std::uint16_t, N> IdRange(std::uint16_t first) noexcept {
  std::array<std::uint16_t, N> out{};
  for (std::size_t i = 0; i < N; ++i)
    out[i] = static_cast<std::uint16_t>(first + i);
  return out;
}
template <std::size_t A, std::size_t B>
constexpr std::array<std::uint16_t, A + B>
JoinIds(const std::array<std::uint16_t, A> &left,
        const std::array<std::uint16_t, B> &right) noexcept {
  std::array<std::uint16_t, A + B> out{};
  for (std::size_t i = 0; i < A; ++i)
    out[i] = left[i];
  for (std::size_t i = 0; i < B; ++i)
    out[A + i] = right[i];
  return out;
}

constexpr auto kCommonIds = IdRange<30>(0x0001);
constexpr auto kPlayerIds = JoinIds(kCommonIds, IdRange<16>(0x0101));
constexpr auto kNpcIds = JoinIds(kCommonIds, IdRange<4>(0x0201));
constexpr auto kTearIds = JoinIds(kCommonIds, IdRange<1>(0x0601));
constexpr auto kProjectileIds = JoinIds(kCommonIds, IdRange<1>(0x0701));
constexpr auto kPickupIds = JoinIds(kCommonIds, IdRange<3>(0x0501));
constexpr auto kFamiliarIds = JoinIds(kCommonIds, IdRange<3>(0x0301));
constexpr auto kEffectIds = JoinIds(kCommonIds, IdRange<3>(0x0401));
constexpr auto kGridIds = IdRange<17>(0x0801);
// UnknownEntity 只允许 Entity 头部 0001–0004；其余字段不得伪装成受支持类别。
constexpr auto kUnknownIds = IdRange<4>(0x0001);

// 78 项字段声明，按 ID 升序。宽度与编码来自 min-read-v1 文件模型。
constexpr FieldDeclaration F(std::uint16_t id, const char *name,
                            std::uint8_t width, Encoding encoding) noexcept {
  return FieldDeclaration{id, name, width, encoding};
}
constexpr std::array<FieldDeclaration, 78> kFields{{
    // Entity 公共 0001–001E
    F(0x0001, "index", 4, Encoding::Raw),
    F(0x0002, "type", 4, Encoding::Raw),
    F(0x0003, "variant", 4, Encoding::Raw),
    F(0x0004, "subtype", 4, Encoding::Raw),
    F(0x0005, "flags", 8, Encoding::Raw),
    F(0x0006, "valid", 1, Encoding::Raw),
    F(0x0007, "visible", 1, Encoding::Raw),
    F(0x0008, "exists", 1, Encoding::Raw),
    F(0x0009, "dead", 1, Encoding::Raw),
    F(0x000A, "removed_by_factory", 1, Encoding::Raw),
    F(0x000B, "position.x", 4, Encoding::F32),
    F(0x000C, "position.y", 4, Encoding::F32),
    F(0x000D, "velocity.x", 4, Encoding::F32),
    F(0x000E, "velocity.y", 4, Encoding::F32),
    F(0x000F, "hp", 4, Encoding::F32),
    F(0x0010, "max_hp", 4, Encoding::F32),
    F(0x0011, "init_seed", 4, Encoding::Raw),
    F(0x0012, "drop_rng.word0", 4, Encoding::Raw),
    F(0x0013, "drop_rng.word1", 4, Encoding::Raw),
    F(0x0014, "drop_rng.word2", 4, Encoding::Raw),
    F(0x0015, "drop_rng.word3", 4, Encoding::Raw),
    F(0x0016, "parent", 4, Encoding::Reference),
    F(0x0017, "child", 4, Encoding::Reference),
    F(0x0018, "target", 4, Encoding::Reference),
    F(0x0019, "spawner", 4, Encoding::Reference),
    F(0x001A, "spawner_type", 4, Encoding::Raw),
    F(0x001B, "spawner_variant", 4, Encoding::Raw),
    F(0x001C, "birth_counter", 4, Encoding::Raw),
    F(0x001D, "game_frame_counter", 4, Encoding::Raw),
    F(0x001E, "frame_count", 4, Encoding::Derived),
    // Player 0101–0110
    F(0x0101, "player_type", 4, Encoding::Raw),
    F(0x0102, "player_index", 4, Encoding::Raw),
    F(0x0103, "damage_cooldown", 4, Encoding::Raw),
    F(0x0104, "max_hearts", 4, Encoding::Raw),
    F(0x0105, "red_hearts", 4, Encoding::Raw),
    F(0x0106, "eternal_hearts", 4, Encoding::Raw),
    F(0x0107, "soul_hearts", 4, Encoding::Raw),
    F(0x0108, "black_hearts", 4, Encoding::Raw),
    F(0x0109, "bone_hearts", 4, Encoding::Raw),
    F(0x010A, "golden_hearts", 4, Encoding::Raw),
    F(0x010B, "broken_hearts", 4, Encoding::Raw),
    F(0x010C, "rotten_hearts", 4, Encoding::Raw),
    F(0x010D, "sub_player", 4, Encoding::Reference),
    F(0x010E, "twin", 4, Encoding::Reference),
    F(0x010F, "backup", 4, Encoding::Reference),
    F(0x0110, "replaced", 4, Encoding::Reference),
    // NPC 0201–0204
    F(0x0201, "state_frame", 4, Encoding::Raw),
    F(0x0202, "state", 4, Encoding::Raw),
    F(0x0203, "projectile_cooldown", 4, Encoding::Raw),
    F(0x0204, "projectile_delay", 4, Encoding::Raw),
    // Familiar 0301–0303
    F(0x0301, "player", 4, Encoding::Reference),
    F(0x0302, "fire_cooldown", 4, Encoding::Raw),
    F(0x0303, "state", 4, Encoding::Raw),
    // Effect 0401–0403
    F(0x0401, "state", 4, Encoding::Raw),
    F(0x0402, "timeout", 4, Encoding::Raw),
    F(0x0403, "lifespan", 4, Encoding::Raw),
    // Pickup 0501–0503
    F(0x0501, "timeout", 4, Encoding::Raw),
    F(0x0502, "wait", 4, Encoding::Raw),
    F(0x0503, "state", 4, Encoding::Raw),
    // Tear 0601：完整 128 位
    F(0x0601, "tear_flags", 16, Encoding::Raw),
    // Projectile 0701：完整 64 位
    F(0x0701, "projectile_flags", 8, Encoding::Raw),
    // Grid 0801–0811；0810/0811 为已确认公式的派生格点坐标
    F(0x0801, "type", 4, Encoding::Raw),
    F(0x0802, "variant", 4, Encoding::Raw),
    F(0x0803, "state", 4, Encoding::Raw),
    F(0x0804, "spawn_count", 4, Encoding::Raw),
    F(0x0805, "spawn_seed", 4, Encoding::Raw),
    F(0x0806, "var_data", 4, Encoding::Raw),
    F(0x0807, "initialized", 1, Encoding::Raw),
    F(0x0808, "variable_seed", 4, Encoding::Raw),
    F(0x0809, "grid_index", 4, Encoding::Raw),
    F(0x080A, "spawn_frame", 4, Encoding::Raw),
    F(0x080B, "rng.word0", 4, Encoding::Raw),
    F(0x080C, "rng.word1", 4, Encoding::Raw),
    F(0x080D, "rng.word2", 4, Encoding::Raw),
    F(0x080E, "rng.word3", 4, Encoding::Raw),
    F(0x080F, "collision_class", 4, Encoding::Raw),
    F(0x0810, "position.x", 4, Encoding::F32),
    F(0x0811, "position.y", 4, Encoding::F32),
}};

constexpr const char *kClassNames[kClassCount] = {
    "Player", "NPC",   "Tear",   "Projectile", "Pickup",
    "Familiar", "Effect", "Grid", "UnknownEntity"};
constexpr const char *kReasonNames[kReasonCount] = {
    "NONE",
    "PHASE_INCOMPLETE",
    "ODD_PLAYER_ONLY",
    "LIFECYCLE",
    "READ_FAILED",
    "BUDGET_EXCEEDED",
    "RELATION_BUDGET_EXCEEDED",
    "BUFFER_BACKPRESSURE",
    "UNKNOWN_CONTINUITY",
    "WRONG_THREAD",
    "NONFINITE",
    "SEQUENCE_EXHAUSTED",
    "ROOT_CHANGED",
    "EVENT_OVERFLOW",
    "STOPPED",
    "UNKNOWN_CLASS",
    "NOT_READY",
    "SOURCE_CHANGED",
    "UNPAIRED_SCOPE",
    "FILE_ERROR",
    "NOTIFICATION_FAILED",
    "PREPARE_FAILED",
    "THREAD_FAILED",
    "INVALID_RECORD"};
constexpr const char *kFieldStatusNames[5] = {"VALUE", "UNCONFIRMED",
                                             "NOT_APPLICABLE", "NOT_READY",
                                             "READ_FAILED"};
constexpr const char *kEncodingNames[4] = {"RAW_LE", "F32_BITS", "REFERENCE",
                                          "DERIVED_RAW32"};
constexpr const char *kRelationNames[5] = {"NULL", "KNOWN_CURRENT", "UNRESOLVED",
                                           "RETIRED", "AMBIGUOUS_REUSE"};
constexpr const char *kStageNames[4] = {"OUTER", "OWNER", "ROOM", "TAIL"};
constexpr const char *kObservationNames[kObservationCount] = {
    "Factory",   "Init",       "Publish",     "Remove",  "ClearReferences",
    "Destroy",   "PlayerCtor", "PlayerCreate", "ManagerRestore", "Replace",
    "PlayerRestore", "NpcMorph", "PickupMorph", "GridFactory", "GridInit",
    "GridSpawn", "Door",       "GridFree",    "Spawn"};
constexpr const char *kEventKindNames[8] = {
    "NATIVE_HOOK", "BIRTH_CONFIRMED", "LIFECYCLE_BOUNDARY", "GAP_OR_UNKNOWN",
    "GRID_PHYSICAL_RETIRED", "UNUSED_5", "CURRENT_PLAYER_READ_WITNESS",
    "SOURCE_MEMBERSHIP_SNAPSHOT"};
constexpr const char *kOriginNames[3] = {"UNSET", "FIRST_OBSERVED",
                                         "OBSERVED_CREATION"};
constexpr const char *kLifecycleNames[7] = {
    "UNKNOWN", "ACQUIRED", "PUBLISHED", "CONSTRUCTED", "REMOVE_REQUESTED",
    "RETIRING", "DESTROYED"};
constexpr const char *kReadinessNames[4] = {
    "NO_OBSERVED_INIT", "BASE_INIT_RETURNED", "KNOWN_CREATION_OWNER_COMPLETED",
    "USER_ACCEPTED_CURRENT_PLAYER_UPDATE_WITNESS"};
constexpr const char *kAuthorityKindNames[3] = {"NONE", "STANDARD_ROOM",
                                                "ODD_PLAYER"};
constexpr const char *kSourceNames[kSourceCount] = {
    "main",         "persistent",     "update",        "render",
    "effect",       "temp",           "bufferTemplate", "wisp",
    "roster",       "esau",           "player_relations", "grid",
    "door",         "reserved_13",    "reserved_14"};

// 名称表必须与枚举上限一一对应：数组按上限声明时，漏登的项是 nullptr，
// 编码时会输出空指针，因此在编译期逐项检查。
template <std::size_t N>
constexpr bool AllNamed(const char *const (&values)[N]) noexcept {
  for (std::size_t i = 0; i < N; ++i)
    if (!values[i])
      return false;
  return true;
}
static_assert(std::size(kClassNames) == kClassCount && AllNamed(kClassNames),
              "类别名称表长度或登记项不符");
static_assert(std::size(kReasonNames) == kReasonCount && AllNamed(kReasonNames),
              "原因名称表长度或登记项不符");
static_assert(AllNamed(kFieldStatusNames) && AllNamed(kEncodingNames) &&
                  AllNamed(kRelationNames) && AllNamed(kStageNames) &&
                  AllNamed(kObservationNames) && AllNamed(kEventKindNames) &&
                  AllNamed(kOriginNames) && AllNamed(kLifecycleNames) &&
                  AllNamed(kReadinessNames) && AllNamed(kAuthorityKindNames) &&
                  AllNamed(kSourceNames),
              "名称表缺少登记项");
static_assert(std::size(kFieldStatusNames) == 5 &&
                  std::size(kEncodingNames) == 4 &&
                  std::size(kRelationNames) == 5 &&
                  std::size(kStageNames) == 4 &&
                  std::size(kObservationNames) == kObservationCount &&
                  std::size(kEventKindNames) == 8 &&
                  std::size(kOriginNames) == 3 &&
                  std::size(kLifecycleNames) == 7 &&
                  std::size(kReadinessNames) == 4 &&
                  std::size(kAuthorityKindNames) == 3 &&
                  std::size(kSourceNames) == kSourceCount,
              "名称表长度与枚举上限不符");

static_assert(kFields.size() == 78, "min-read-v1 必须登记 78 个字段");
// 表必须严格升序且无重复，离线严格模型按此顺序校验。
constexpr bool FieldsSorted() noexcept {
  for (std::size_t i = 1; i < kFields.size(); ++i)
    if (kFields[i - 1].id >= kFields[i].id)
      return false;
  return true;
}
static_assert(FieldsSorted(), "字段表必须按 ID 严格升序");
static_assert(kFields.front().id == 0x0001 && kFields.back().id == 0x0811,
              "字段表必须覆盖 0001–0811 的完整 profile，不能少登记首尾项");

} // namespace

const char *ClassName(Class value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < kClassCount ? kClassNames[index] : "UNKNOWN_CLASS";
}
const char *ReasonName(Reason value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < kReasonCount ? kReasonNames[index] : "UNKNOWN_ERROR";
}
const char *FieldStatusName(FieldStatus value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kFieldStatusNames) ? kFieldStatusNames[index]
                                              : "UNKNOWN_STATUS";
}
const char *EncodingName(Encoding value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kEncodingNames) ? kEncodingNames[index]
                                           : "UNKNOWN_ENCODING";
}
const char *RelationName(Relation value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kRelationNames) ? kRelationNames[index]
                                           : "UNKNOWN_RELATION";
}
const char *StageName(Stage value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kStageNames) ? kStageNames[index] : "UNKNOWN_STAGE";
}
const char *ObservationName(Observation value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < kObservationCount ? kObservationNames[index] : "UNKNOWN_HOOK";
}
const char *EventKindName(EventKind value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kEventKindNames) ? kEventKindNames[index]
                                            : "UNKNOWN_EVENT_KIND";
}
const char *OriginName(Origin value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kOriginNames) ? kOriginNames[index]
                                         : "UNKNOWN_ORIGIN";
}
const char *LifecycleName(Lifecycle value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kLifecycleNames) ? kLifecycleNames[index]
                                            : "UNKNOWN_LIFECYCLE";
}
const char *ReadinessName(Readiness value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kReadinessNames) ? kReadinessNames[index]
                                            : "UNKNOWN_READINESS";
}
const char *AuthorityKindName(AuthorityKind value) noexcept {
  const auto index = static_cast<std::size_t>(value);
  return index < std::size(kAuthorityKindNames) ? kAuthorityKindNames[index]
                                                : "UNKNOWN_AUTHORITY";
}
const char *SourceName(std::uint32_t source) noexcept {
  return source < kSourceCount ? kSourceNames[source] : "unknown_source";
}
bool ParseReason(std::uint32_t value, Reason &out) noexcept {
  if (value >= kReasonCount)
    return false;
  out = static_cast<Reason>(value);
  return true;
}

std::span<const FieldDeclaration> ProfileFields() noexcept {
  return std::span<const FieldDeclaration>(kFields.data(), kFields.size());
}
const FieldDeclaration *ProfileField(std::uint16_t id) noexcept {
  for (const auto &field : kFields)
    if (field.id == id)
      return &field;
  return nullptr;
}
std::span<const std::uint16_t> ProfileFieldsFor(Class kind) noexcept {
  switch (kind) {
  case Class::Player:
    return kPlayerIds;
  case Class::NPC:
    return kNpcIds;
  case Class::Tear:
    return kTearIds;
  case Class::Projectile:
    return kProjectileIds;
  case Class::Pickup:
    return kPickupIds;
  case Class::Familiar:
    return kFamiliarIds;
  case Class::Effect:
    return kEffectIds;
  case Class::Grid:
    return kGridIds;
  case Class::Unknown:
    return kUnknownIds;
  }
  return {};
}
bool ProfileAllows(Class kind, std::uint16_t id) noexcept {
  const auto allowed = ProfileFieldsFor(kind);
  return std::find(allowed.begin(), allowed.end(), id) != allowed.end();
}

bool BudgetLedger::Charge(std::uint64_t bytes) noexcept {
  if (!bytes)
    return true;
  auto used = used_.load(std::memory_order_relaxed);
  for (;;) {
    // 先在上限内做减法比较，避免 used + bytes 溢出绕过预算。
    if (used > limit_ || bytes > limit_ - used)
      return false;
    if (used_.compare_exchange_weak(used, used + bytes,
                                    std::memory_order_acq_rel,
                                    std::memory_order_relaxed))
      break;
  }
  const auto total = used + bytes;
  auto peak = peak_.load(std::memory_order_relaxed);
  while (peak < total &&
         !peak_.compare_exchange_weak(peak, total, std::memory_order_relaxed,
                                      std::memory_order_relaxed)) {
  }
  return true;
}
void BudgetLedger::Release(std::uint64_t bytes) noexcept {
  if (bytes)
    used_.fetch_sub(bytes, std::memory_order_relaxed);
}

void RawField::Reset() noexcept {
  id = 0;
  status = FieldStatus::NotReady;
  encoding = Encoding::Raw;
  width = 0;
  relation = Relation::Unresolved;
  reserved = 0;
  target_id = 0;
  target_use = 0;
  std::memset(raw.data(), 0, raw.size());
}
void RawObject::Reset() noexcept {
  local_id = 0;
  use = 0;
  birth = 0;
  revision = 0;
  domain = 0;
  kind = 0;
  address = 0;
  vptr = 0;
  origin = 0;
  lifecycle = 0;
  readiness = 0;
  birth_kind = 0;
  birth_type = 0;
  birth_variant = 0;
  birth_subtype = 0;
  birth_seed = 0;
  birth_known = 0;
  ordinal = kNoOrdinal;
  field_count = 0;
  grid_index = kNoGridIndex;
  // 字段数组只清到有效范围之外不必要；field_count=0 后旧字节不被读取。
}
void RawBatch::Reset() noexcept {
  header = BatchHeader{};
}

} // namespace cs::world
