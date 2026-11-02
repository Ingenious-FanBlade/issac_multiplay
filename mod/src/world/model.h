#pragma once
// tx-04-08 world_record 迁移：纯值模型。
//
// 本文件只定义与游戏、SDK、线程、文件系统无关的值类型、枚举、预算常量和
// min-read-v1 profile 声明。字段偏移、RTTI 描述符、初始化证据属于
// binding/world_layout，由 manifest 输入提供，不在此固化。
//
// 数值与文本约定（与原型文件格式及离线严格模型一致）：
//   - u64 在 JSONL 中写成规范十进制字符串；
//   - raw32/raw64 写成固定宽小写十六进制；
//   - 其他宽度写成 bytes_le；
//   - 地址、vptr、线程、容器描述符只进入 diagnostics，不参与比较 key；
//   - 不计算任何摘要或哈希，身份与比较都使用显式值。

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

// 20 项观察语义单独放在 C++17 兼容的头里：生成的 SDK-facing wrapper 也要用。
#include "world/observation.h"

namespace cs::world {

// ===========================================================================
// 1. 预算与容量（迁移设计第 7.1 节）
// ===========================================================================

// 单批对象、成员关系、单对象字段上限。超出即该批不完整，不得丢弃已有证据。
inline constexpr std::uint32_t kObjectBudget = 16384;
inline constexpr std::uint32_t kMembershipBudget = 65536;
inline constexpr std::uint32_t kFieldBudget = 52;
// 本地身份表与 Player 保留关系工作表上限。
inline constexpr std::uint32_t kIdentityBudget = 32768;
inline constexpr std::uint32_t kRelationNodes = 64;
// 原始批次槽与生命周期事件环上限。
inline constexpr std::uint32_t kBatchSlots = 3;
inline constexpr std::uint32_t kEventCapacity = 65536;
// 没有数据槽时的 gap 控制记录独立固定容量；再溢出即停用本次录制。
inline constexpr std::uint32_t kGapCapacity = 4096;
// 来源数（八 EL、roster、Esau、Player 保留关系、Grid、door、两个 reserved）。
inline constexpr std::uint32_t kSourceCount = 15;
// 类别数：七类 Entity、Grid、UnknownEntity。
inline constexpr std::uint32_t kClassCount = 9;
// 原始复制后的紧凑值队列：字节与项数两个条件同时成立。
inline constexpr std::uint64_t kQueueBytes = 64ull << 20;
inline constexpr std::uint32_t kQueueItems = 64;
// 可选Recorder容纳切房事件突发；仍共享64MiB字节预算，附加节点归管理预算。
inline constexpr std::uint32_t kRecorderQueueItems = 256;
// 每个 JSONL 文件 1 MiB 缓冲。
inline constexpr std::size_t kIoBufferBytes = 1u << 20;
// world 自有动态存储总预算，含批次、队列、身份/radix、控制记录、I/O 与管理。
inline constexpr std::uint64_t kWorldBudgetBytes = 512ull << 20;
// tx-04-10 扩展域和可选录制共同计账，不把每个模块都当成独享旧预算。
inline constexpr std::uint64_t kObservationReserveBytes=160ull<<20, kPublicationBytes=32ull<<20, kRecorderQueueBytes=kQueueBytes;
// 未逐项记账的管理/编码余量，明确计入总预算而不是当作免费。
inline constexpr std::uint64_t kManagementBytes = 8ull << 20;
// 生命周期事件环使用 sequence 关联，事件与批次各自保留 watermark。
// 控制组身体锚点容量不等于参与者容量；每个身体仍沿用同一 Entity identity。
inline constexpr std::uint32_t kControlBodiesPerParticipant = 64, kControlAnchorCapacity = 4 * kControlBodiesPerParticipant;
inline constexpr std::uint32_t kNoOrdinal = 0xFFFFFFFFu;
inline constexpr std::uint32_t kNoGridIndex = 0xFFFFFFFFu;
// 单个原生函数的观察调用编号从 1 开始，0 表示“无”。
inline constexpr std::uint32_t kNoCall = 0u;

// ===========================================================================
// 2. 枚举
// ===========================================================================

// 对象分类。数值与原型文件模型一致（CLASSES 顺序）。
enum class Class : std::uint32_t {
  Player = 0,
  NPC = 1,
  Tear = 2,
  Projectile = 3,
  Pickup = 4,
  Familiar = 5,
  Effect = 6,
  Grid = 7,
  Unknown = 8
};
// 完整样本缺失或拒绝的具体原因。0–18 沿用原型数值；19 起为本轮追加，
// 只追加不改写，旧记录可继续按原值解释。
enum class Reason : std::uint32_t {
  None = 0,
  PhaseIncomplete = 1,
  OddOnly = 2,
  Lifecycle = 3,
  ReadFailed = 4,
  Budget = 5,
  RelationBudget = 6,
  Backpressure = 7,
  Continuity = 8,
  WrongThread = 9,
  Nonfinite = 10,
  SequenceExhausted = 11,
  RootChanged = 12,
  EventOverflow = 13,
  Stopped = 14,
  UnknownClass = 15,
  NotReady = 16,
  SourceChanged = 17,
  UnpairedScope = 18,
  // 追加：世界自有故障。文件/通知/准备/线程失败各自独立，不冒称游戏故障。
  FileError = 19,
  NotificationFailed = 20,
  PrepareFailed = 21,
  ThreadFailed = 22,
  // 追加：编码时发现与 profile/批次结构不符的记录。属于采集器错误，
  // 不能把非法记录写进新 domain，也不能静默丢弃。
  InvalidRecord = 23
};
inline constexpr std::uint32_t kReasonCount = 24;
// 字段读取状态。UNCONFIRMED/NOT_READY/READ_FAILED 都不携带值。
enum class FieldStatus : std::uint8_t {
  Value = 0,
  Unconfirmed = 1,
  NotApplicable = 2,
  NotReady = 3,
  ReadFailed = 4
};
// 字段编码：Raw 为原始字节，F32 为浮点位，Reference 为同域地址引用，
// Derived 为由已确认值推导（例如 frame_count、Grid 格点坐标）。
enum class Encoding : std::uint8_t {
  Raw = 0,
  F32 = 1,
  Reference = 2,
  Derived = 3
};
// 引用解析状态。未知地址不得填成 NULL，也不得因“同批出现”消除复用风险。
enum class Relation : std::uint32_t {
  Null = 0,
  KnownCurrent = 1,
  Unresolved = 2,
  Retired = 3,
  AmbiguousReuse = 4
};
// 同一观察窗口内的阶段。Outer 是 Manager 最外层，其余是普通 owner/Room/尾部。
enum class Stage : std::uint32_t { Outer = 0, Owner = 1, Room = 2, Tail = 3 };
// 生命周期事件类别。5 在原型中未使用，保留空号以免旧记录换义。
enum class EventKind : std::uint32_t {
  NativeHook = 0,
  BirthConfirmed = 1,
  LifecycleBoundary = 2,
  GapOrUnknown = 3,
  GridPhysicalRetired = 4,
  CurrentPlayerReadWitness = 6,
  SourceMembershipSnapshot = 7,
  NativeBody = 8 // 本轮追加：内层原生到达/正常返回，沿用同 call/parent。
};
// 对象所在域：Entity 与 Grid 的地址空间不合并。
enum class ObjectDomain : std::uint32_t { Entity = 0, Grid = 1 };
// 本地身份来源：启用前已存在（FIRST_OBSERVED）与观察到创建。
enum class Origin : std::uint32_t { Unset = 0, FirstObserved = 1, ObservedCreation = 2 };
// 本地生命周期阶段。>=Retiring 视为已退休；退出成员关系不等于物理销毁。
enum class Lifecycle : std::uint32_t {
  Unknown = 0,
  Acquired = 1,
  Published = 2,
  Constructed = 3,
  RemoveRequested = 4,
  Retiring = 5,
  Destroyed = 6
};
// 派生字段的读取依据。3 是一次有效 Authority 窗口内原生 PlayerUpdate
// 正常返回的用户采纳依据，不改写 FIRST_OBSERVED，也不补造出生历史。
enum class Readiness : std::uint32_t {
  NoObservedInit = 0,
  BaseInitReturned = 1,
  CreationOwnerCompleted = 2,
  CurrentPlayerUpdateWitness = 3
};
// 本轮 Authority tick 的类型；0 表示本轮不是有效 Authority 样本。
enum class AuthorityKind : std::uint32_t { None = 0, StandardRoom = 1, OddPlayer = 2 };

const char *ClassName(Class value) noexcept;
const char *ReasonName(Reason value) noexcept;
const char *FieldStatusName(FieldStatus value) noexcept;
const char *EncodingName(Encoding value) noexcept;
const char *RelationName(Relation value) noexcept;
const char *StageName(Stage value) noexcept;
const char *EventKindName(EventKind value) noexcept;
const char *OriginName(Origin value) noexcept;
const char *LifecycleName(Lifecycle value) noexcept;
const char *ReadinessName(Readiness value) noexcept;
const char *AuthorityKindName(AuthorityKind value) noexcept;
// 来源 ID 的固定名称（15 项）；越界返回 "unknown_source"。
const char *SourceName(std::uint32_t source) noexcept;
// 由外部数值恢复枚举；越界返回 false，不猜测最近值。
bool ParseReason(std::uint32_t value, Reason &out) noexcept;

// ===========================================================================
// 3. 原始记录（只含值，按实际纯值布局登记上界）
// ===========================================================================

// 单字段原始值。target_id/target_use 只在 Relation::KnownCurrent 时有效；
// raw 保留原始字节，位图比较在离线工具中完成。
//
// 本类型不写默认成员初始化器：RawBatch 的 16384×52 字段数组在准备阶段一次性
// 分配，若存在非平凡默认构造会在 32 位进程里触碰上百 MiB 页。写入前必须显式
// 调用 Reset()，这是采集器的前置条件而不是可选清理。
struct RawField final {
  std::uint16_t id;
  FieldStatus status;
  Encoding encoding;
  std::uint32_t width;      // 字节宽度：1/4/8/16
  Relation relation;
  std::uint32_t reserved;   // 保留，必须为 0
  std::uint64_t target_id;
  std::uint64_t target_use;
  std::array<std::uint8_t, 16> raw;

  // 恢复为空字段；不清 identity 关联，只用于新对象写入前。
  void Reset() noexcept;
};
static_assert(sizeof(RawField) == 48,
              "RawField 布局必须是 48 字节，预算表按此计算");

// 单个对象的一次只读复制。address/vptr 只用于本地诊断，不参与比较 key。
// 与 RawField 相同，不写默认成员初始化器，写入前必须调用 Reset()。
// RNG 见证：0=未读/集合变化，1=成功，2=受保护读取失败。原始四字不作摘要。
struct RawRng final {
  std::array<std::uint32_t, 4> before, after;
  std::uint32_t before_status, after_status;
};
static_assert(sizeof(RawRng) == 40);
struct RawObject final {
  std::uint64_t local_id, use, birth, revision;
  std::uint32_t domain;            // ObjectDomain
  std::uint32_t kind;              // Class
  std::uint32_t address;           // 本地临时关联，仅诊断
  std::uint32_t vptr;              // 分类所用的原始 vptr，仅诊断
  std::uint32_t origin;            // Origin
  std::uint32_t lifecycle;         // Lifecycle
  std::uint32_t readiness;         // Readiness
  std::uint32_t birth_kind;        // 确认出生的 Observation 值
  std::uint32_t birth_type, birth_variant, birth_subtype, birth_seed;
  std::uint32_t birth_known;       // 1 = 出生描述已冻结
  std::uint32_t ordinal;           // 0=A、1=B，kNoOrdinal 为未绑定
  std::uint32_t field_count;
  std::uint32_t grid_index;        // Grid/Door 已确认的槽关联
  std::array<RawField, kFieldBudget> fields;
  RawRng rng;

  void Reset() noexcept;
};
static_assert(sizeof(RawObject) == 2632,
              "RawObject 布局必须是 2632 字节，预算表按此计算");

// 一批内的成员关系：同一对象可同时来自多个来源，不重复计出生。
struct RawMembership final {
  std::uint32_t object = 0;
  std::uint32_t source = 0;
  std::uint32_t index = 0;
  std::uint32_t reserved = 0;
};
static_assert(sizeof(RawMembership) == 16, "RawMembership 布局必须是 16 字节");

// 来源状态：status 为 0 表示已完整枚举，0xFFFFFFFF 表示 NOT_APPLICABLE，
// 其余数值是 Reason，表示该来源本次不可用。
struct RawSource final {
  std::uint32_t status = 0;
  std::uint32_t count = 0;
  std::array<std::uint32_t, 4> descriptor{}; // 容器描述符，仅诊断
};
inline constexpr std::uint32_t kSourceNotApplicable = 0xFFFFFFFFu;

// 批次头。event_watermark 是该批复制开始时已发布的生命周期事件序号，
// 使 captures 与 lifecycle-events 两份文件可按序号关联，不依赖写入交错。
// thread 只进入 diagnostics，不参与比较。sources 随头一起复制到 owned batch，
// reader 归还槽后后台仍能完整编码来源状态。
struct BatchHeader final {
  std::uint64_t run = 0, room = 0, mutation = 0, attempt = 0, capture = 0,
                outer = 0, tick = 0, scope = 0, event_watermark = 0;
  std::uint32_t authority = 0;          // 1 = 本轮有 Authority 提交
  std::uint32_t commit_kind = 0;        // AuthorityKind
  std::uint32_t thread = 0;             // GetCurrentThreadId，仅诊断
  std::uint32_t complete = 0;           // 1 = 完整普通样本
  std::uint32_t object_count = 0;
  std::uint32_t membership_count = 0;
  std::uint32_t required_objects = 0;
  std::uint32_t required_memberships = 0;
  std::uint32_t recording_generation = 0;
  std::uint32_t reason = 0;             // Reason
  // 实际阶段值随批次交付，供统一 E2E 关联 native pre/commit；不额外调用游戏查询。
  std::uint64_t loop = 0;
  std::array<std::uint64_t, 3> phase_scopes{};
  std::array<std::uint32_t, 3> phase_entered{}, phase_returned{};
  std::array<RawSource, kSourceCount> sources{};
  // 前置清单仅定义观察集合；见证窗口包住随后完整的来源枚举、字段、引用和尾部复核。
  std::uint32_t rng_inventory = 0, rng_matched = 0, rng_set_stable = 0;
  std::uint32_t rng_thread_before = 0, rng_thread_after = 0;
  std::uint64_t copy_started_qpc = 0, copy_finished_qpc = 0, qpc_frequency = 0;
};
static_assert(sizeof(BatchHeader) == 576, "BatchHeader 布局必须是 576 字节");

// 原始批次：槽内一次性写满的有效前缀，不把固定容量写成有效数据。
// objects/memberships 故意不带初始化器，分配时不触碰这两块大数组；
// 有效范围由 header.object_count/membership_count 决定。
struct RawBatch final {
  BatchHeader header{};
  std::array<RawObject, kObjectBudget> objects;
  std::array<RawMembership, kMembershipBudget> memberships;

  // 清头、来源与计数；对象/成员数组保留旧字节但不再被读取。
  void Reset() noexcept;
};
static_assert(sizeof(RawBatch) == 44171840,
              "RawBatch 布局必须是 44171840 字节，预算表按此计算");

// 生命周期事件。sequence 由 storage 分配；call/parent 是观察调用编号，
// 由主线程桥分配，0 表示没有（例如成员关系快照）。
// observation 故意用数值保存 kNoObservation 哨兵：生命周期/成员关系边界
// 事件不属于任何一个原生观察入口。
struct LifecycleEvent final {
  std::uint64_t sequence = 0;
  std::uint64_t run = 0, room = 0, outer = 0;
  std::uint64_t local_id = 0, use = 0, birth = 0, revision = 0;
  std::uint32_t call = kNoCall, parent = kNoCall;
  std::uint32_t observation = kNoObservation; // Observation 或 kNoObservation
  std::uint32_t entering = 0;
  // 1 = 原生异常展开：只撤销本次观察资格，不伪造正常返回或返回值。
  std::uint32_t exceptional = 0;
  EventKind kind = EventKind::NativeHook;
  Reason reason = Reason::None;
  std::uint32_t address = 0;   // 仅诊断
  std::uint32_t result = 0;    // 原生返回值/成员位集，仅诊断
  std::uint32_t memberships = 0;
  // episode 原始上下文：全部是主线程已观察值，后台不再次读取游戏。
  std::uint64_t mutation = 0, tick = 0, session = 0, participant_generation = 0;
  std::uint32_t domain = 0, lifecycle = 0, readiness = 0, ordinal = kNoOrdinal;
  std::int32_t assignment_key = -1;
  std::uint32_t device = 0;
  std::array<std::uint32_t, 8> args{};
  std::uint32_t arg_count = 0, slot_before = 0, slot_after = 0, thread = 0;
};
static_assert(sizeof(LifecycleEvent) == 208,
              "LifecycleEvent 布局必须是 208 字节，预算通过 sizeof 自动计入");

// 没有数据槽时仍要交付的控制记录：保留 attempt、原因与范围。
// 它不携带对象/字段，因此不能改写成“完整样本”。
struct GapRecord final {
  std::uint64_t attempt = 0, run = 0, room = 0, mutation = 0, outer = 0, tick = 0,
                scope = 0, event_watermark = 0;
  Reason reason = Reason::None;
  std::uint32_t recording_generation = 0;
  std::uint32_t authority_kind = 0; // AuthorityKind
  std::uint32_t thread = 0; // 实际生产线程，仅用于 odd-only 连续段证据，不参与比较 key。
};
static_assert(sizeof(GapRecord) == 80, "GapRecord 布局必须是 80 字节");

// 场景标记只携带主线程已有序号，不触发采样，也不改变身份或连续计数。
inline constexpr std::uint32_t kCheckpointCapacity = 128;
// 同一目录/场景标签规则供命令 admission 与 writer 使用，避免 Win32 别名越界。
bool ValidWorldLabel(std::string_view label) noexcept;
struct CheckpointRecord final {
  std::uint64_t request = 0, sequence = 0;
  std::uint64_t run = 0, room = 0, outer = 0, mutation = 0;
  std::uint64_t attempt = 0, capture = 0, event_watermark = 0;
  std::uint32_t recording_generation = 0;
  char label[65]{};
};

// ===========================================================================
// 4. 预算登记
// ===========================================================================

// 身份表与 radix 的计划单价。identity.h 用 static_assert 保证实际类型不超过
// 这里的登记值，因此 model 可以先给出完整计划而不依赖 identity.h。
inline constexpr std::uint64_t kIdentityRecordBytes = 136;
inline constexpr std::uint64_t kRadixNodeBytes = 12;
inline constexpr std::uint64_t kRadixNodes = std::uint64_t(kIdentityBudget) * 34;

// 默认上限下的计划占用：三个批次槽、值队列、事件环、gap 环、身份/radix、
// 两个 I/O 缓冲和明确的管理余量。
constexpr std::uint64_t PlannedWorldBytes() noexcept {
  return std::uint64_t(kBatchSlots) * sizeof(RawBatch) + kQueueBytes +
         std::uint64_t(kEventCapacity) * sizeof(LifecycleEvent) +
         std::uint64_t(kGapCapacity) * sizeof(GapRecord) +
         std::uint64_t(kCheckpointCapacity) * sizeof(CheckpointRecord) +
         kIdentityRecordBytes * kIdentityBudget +
         kRadixNodeBytes * kRadixNodes +
         2 * std::uint64_t(kIoBufferBytes) + kManagementBytes +
         kObservationReserveBytes + kPublicationBytes + kRecorderQueueBytes;
}
static_assert(PlannedWorldBytes() <= kWorldBudgetBytes,
              "默认 world 计划超过配置总预算，必须先缩减上限");
static_assert(kBatchSlots == 3 && kObjectBudget == 16384 &&
                  kMembershipBudget == 65536,
              "槽/对象/成员上限是文件模型与验收计数的共同前提");

// world 自有动态存储账本。Charge 原子预占，超限回滚并返回 false：
// 超预算使准备失败，而不是让游戏线程继续分配后才发现。
class BudgetLedger final {
public:
  explicit BudgetLedger(std::uint64_t limit = kWorldBudgetBytes) noexcept
      : limit_(limit) {}
  bool Charge(std::uint64_t bytes) noexcept;
  void Release(std::uint64_t bytes) noexcept;
  std::uint64_t Used() const noexcept {
    return used_.load(std::memory_order_relaxed);
  }
  std::uint64_t Peak() const noexcept {
    return peak_.load(std::memory_order_relaxed);
  }
  std::uint64_t Limit() const noexcept { return limit_; }

private:
  std::uint64_t limit_;
  std::atomic<std::uint64_t> used_{0}, peak_{0};
};

// ===========================================================================
// 5. min-read-v1 profile 声明
// ===========================================================================

// 字段声明：ID、语义名称、宽度与编码。目标版本偏移与初始化证据不属于这里。
struct FieldDeclaration final {
  std::uint16_t id = 0;
  const char *name = "";
  std::uint8_t width = 0;
  Encoding encoding = Encoding::Raw;
};

constexpr FieldDeclaration FieldDecl(std::uint16_t id, const char *name,
                                     std::uint8_t width,
                                     Encoding encoding) noexcept {
  return FieldDeclaration{id, name, width, encoding};
}

// min-read-v1 的 78 项字段声明（按 ID 升序）。
//
// 放在头里保持 constexpr 可比较：binding/world_layout 生成的偏移表必须与
// 这里的 ID/宽度/编码逐项一致，否则编译期 static_assert 直接失败，避免
// 文件模型与目标版本布局清单各自漂移。
#include "world/compatibility/min_read_v1_fields.inc"

// 全部 78 个字段，按 ID 升序。
std::span<const FieldDeclaration> ProfileFields() noexcept;
// 按 ID 查找；未登记返回 nullptr。
const FieldDeclaration *ProfileField(std::uint16_t id) noexcept;
// 该类别允许出现的字段集合（升序）；UnknownEntity 只允许 0001–0004 头部。
std::span<const std::uint16_t> ProfileFieldsFor(Class kind) noexcept;
// 该类别是否声明了该字段（Unknown 的 0001–0004 视作允许）。
bool ProfileAllows(Class kind, std::uint16_t id) noexcept;

// ===========================================================================
// 6. manifest 输入（由 binding/world_layout 提供，writer 只序列化）
// ===========================================================================

// 目标版本布局：字段偏移与静态证据出处。
struct LayoutField final {
  std::uint16_t id = 0;
  std::uint32_t offset = 0;
  const char *evidence = "";
};
// 目标版本来源：原生位置、固定容量和证据出处。
// capacity 只表示固定来源的容量；0 表示由运行期 descriptor（EL/roster）决定，
// 具体形状见 binding/world_layout.h 的 SourceSpec。
struct LayoutSource final {
  std::uint32_t source = 0;
  const char *location = "";
  std::uint32_t capacity = 0;
  const char *evidence = "";
};
// 目标版本分类：描述符 RVA 与继承/证据说明。
struct LayoutClass final {
  Class kind = Class::Player;
  const char *descriptor_rva = "";
  const char *evidence = "";
};

// writer 需要的 manifest 输入。表和字符串不复制，owner 必须持有到 writer 真正结束；
// 未提供布局时 manifest 明确记录布局未声明，而不是伪造偏移。
struct ManifestInput final {
  const char *session = "";
  const char *game_version = "";
  const char *repentogon_version = "";
  const char *build = "";
  const char *hook_manifest = "";
  const char *phase = "POST_STANDARD_UPDATE_PRE_TOP_EXIT";
  const LayoutField *layout_fields = nullptr;
  std::size_t layout_field_count = 0;
  const LayoutSource *layout_sources = nullptr;
  std::size_t layout_source_count = 0;
  const LayoutClass *layout_classes = nullptr;
  std::size_t layout_class_count = 0;
  const char *const *assumptions = nullptr;
  std::size_t assumption_count = 0;
  const char *const *unknowns = nullptr;
  std::size_t unknown_count = 0;
  const bool *observations = nullptr; // kObservationCount 项，true = 本次登记启用
  std::uint32_t recording_generation = 0;
  // 实际上限；E2E 可以显式降低，必须在 manifest 中登记而不是隐藏。
  std::uint32_t slot_count = kBatchSlots;
  std::uint32_t event_capacity = kEventCapacity;
  std::uint32_t gap_capacity = kGapCapacity;
  std::uint64_t queue_bytes = kQueueBytes;
  std::uint32_t queue_items = kQueueItems;
};

// 文件域与版本：与旧 isaac.cs.t0.world-record 明确区分。
inline constexpr const char *kDomain = "isaac.cs.world-record";
inline constexpr const char *kLegacyDomain = "isaac.cs.t0.world-record";
inline constexpr std::uint32_t kSchemaVersion = 1;
inline constexpr const char *kProfile = "min-read-v1";

} // namespace cs::world
