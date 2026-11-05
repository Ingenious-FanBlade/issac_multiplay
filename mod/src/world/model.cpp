// tx-04-08 纯值模型：名称表、profile 声明、预算账本与记录清理。
// 本单元不读取游戏内存、不打开文件、不创建线程。

#include "world/model.h"

#include <algorithm>
#include <cstring>

namespace cs::world {
bool ValidWorldLabel(std::string_view label) noexcept {
  if (label.empty() || label.size() > 64 || label.back() == '.' ||
      label.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") != std::string_view::npos)
    return false;
  const auto stem = label.substr(0, label.find('.'));
  char upper[5]{};
  if (stem.size() <= 4) {
    for (size_t i = 0; i < stem.size(); ++i)
      upper[i] = stem[i] >= 'a' && stem[i] <= 'z' ? char(stem[i] - 'a' + 'A') : stem[i];
    const std::string_view name(upper, stem.size());
    if (name == "CON" || name == "PRN" || name == "AUX" || name == "NUL" ||
        (name.size() == 4 && (name.substr(0, 3) == "COM" || name.substr(0, 3) == "LPT") &&
         name[3] >= '1' && name[3] <= '9')) return false;
  }
  return label != "." && label != "..";
}
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

// 78 项字段声明表在 model.h 的 kProfileFieldTable，保持 constexpr 可比较，
// 使 binding/world_layout 生成的偏移表能与文件模型在编译期互相校验。

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
    "GridSpawn", "Door",       "GridFree",    "Spawn", "GridCtor"};
constexpr const char *kEventKindNames[9] = {
    "NATIVE_HOOK", "BIRTH_CONFIRMED", "LIFECYCLE_BOUNDARY", "GAP_OR_UNKNOWN",
    "GRID_PHYSICAL_RETIRED", "UNUSED_5", "CURRENT_PLAYER_READ_WITNESS",
    "SOURCE_MEMBERSHIP_SNAPSHOT", "NATIVE_BODY"};
constexpr const char *kOriginNames[3] = {"UNSET", "FIRST_OBSERVED",
                                         "OBSERVED_CREATION"};
constexpr const char *kLifecycleNames[7] = {
    "UNKNOWN", "ACQUIRED", "PUBLISHED", "CONSTRUCTED", "REMOVE_REQUESTED",
    "RETIRING", "DESTROYED"};
constexpr const char *kReadinessNames[6] = {
    "NO_OBSERVED_INIT", "BASE_INIT_RETURNED", "KNOWN_CREATION_OWNER_COMPLETED",
    "USER_ACCEPTED_CURRENT_PLAYER_UPDATE_WITNESS", "NAMED_PLAYER_CHILD_INIT_COMPLETED", "NAMED_PLAYER_STATE_OWNER_COMPLETED"};
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
                  std::size(kEventKindNames) == 9 &&
                  std::size(kOriginNames) == 3 &&
                  std::size(kLifecycleNames) == 7 &&
                  std::size(kReadinessNames) == 6 &&
                  std::size(kAuthorityKindNames) == 3 &&
                  std::size(kSourceNames) == kSourceCount,
              "名称表长度与枚举上限不符");

static_assert(kProfileFieldTable.size() == 78,
              "min-read-v1 必须登记 78 个字段");
// 表必须严格升序且无重复，离线严格模型按此顺序校验。
constexpr bool FieldsSorted() noexcept {
  for (std::size_t i = 1; i < kProfileFieldTable.size(); ++i)
    if (kProfileFieldTable[i - 1].id >= kProfileFieldTable[i].id)
      return false;
  return true;
}
static_assert(FieldsSorted(), "字段表必须按 ID 严格升序");
static_assert(kProfileFieldTable.front().id == 0x0001 &&
                  kProfileFieldTable.back().id == 0x0811,
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
  return std::span<const FieldDeclaration>(kProfileFieldTable.data(),
                                           kProfileFieldTable.size());
}
const FieldDeclaration *ProfileField(std::uint16_t id) noexcept {
  for (const auto &field : kProfileFieldTable)
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
  rng = {};
  // 字段数组只清到有效范围之外不必要；field_count=0 后旧字节不被读取。
}
void RawBatch::Reset() noexcept {
  header = BatchHeader{};
}

} // namespace cs::world
