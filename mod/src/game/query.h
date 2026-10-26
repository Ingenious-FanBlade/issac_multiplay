#pragma once
#include "game/bridge.h"
#include <array>
#include <cstdint>

namespace cs::game {
// 固定容量的重复读取诊断。bucket 只保存值，不改变 query 的返回路径。
inline constexpr unsigned kQueryActionCount = 12;
inline constexpr unsigned kQueryKindCount = 3;
inline constexpr unsigned kQueryConsumerCount = 6;
inline constexpr unsigned kQueryOrdinalCount = 2;
inline constexpr unsigned kQueryRepeatCapacity = 192;
// 完整查询键：只有 tick、participant 代次、资格段、consumer、查询类型与动作
// 全部相同，才允许比较两次原生读取。失效前后绝不混为“同一快照不一致”。
struct QueryKey {
  std::uint64_t tick = 0, generation = 0, epoch = 0;
  unsigned ordinal = 0, consumer = 0, query = 0, action = 0;
  bool operator==(const QueryKey &) const = default;
};
// 一个键的首次有限结果与读取/重复/不一致计数；只作诊断，不 Submit/Resolve。
struct QueryRepeatBucket {
  QueryKey key{};
  float first = 0;
  std::uint64_t reads = 0, repeats = 0, mismatches = 0;
  bool used = false;
};
// 累计键保留 ordinal/consumer/查询类型/action，tick/代次/资格段仍由比较桶隔离。
// 所有可能的键均有固定槽位；零读取与零重复如实表示 NOT_OBSERVED。
struct QueryCoverage {
  std::uint64_t reads = 0, repeats = 0, mismatches = 0, nonfinite = 0;
};
using QueryCoverageTable =
    std::array<std::array<std::array<std::array<QueryCoverage, kQueryKindCount>,
                                    kQueryActionCount>,
                         kQueryConsumerCount>, kQueryOrdinalCount>;
// 场景累计保留完整分类，避免 A 的重复读取冒充 B、或不同 consumer 相互覆盖。
struct QueryRepeatStats {
  std::uint64_t reads = 0, repeats = 0, mismatches = 0, overflow = 0,
                nonfinite = 0;
  QueryCoverageTable coverage{};
  // 首个不一致的准确键与值，避免在 trace 中逐查询输出。
  bool mismatch_known = false;
  QueryKey mismatch_key{};
  float mismatch_first = 0, mismatch_last = 0;
};
// 无 tick（加载窗口）查询的独立汇总：只统计中性与上下文，不写快照。
struct LoadingQueryStats {
  std::uint64_t queries = 0, nonzero = 0, unknown_context = 0;
};
struct SessionRuntime;
// checkpoint 非空时输出带场景标签的只读累计快照，可前后作差；不会清零比较桶。
// 默认会话结束输出只执行一次。每行固定 36 个 action/query 键，控制日志负荷。
void PublishQueryCoverage(SessionRuntime &runtime,
                          const char *checkpoint = nullptr) noexcept;
} // namespace cs::game
