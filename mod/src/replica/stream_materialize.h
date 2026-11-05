#pragma once
#include "replica/stream_model.h"

namespace cs::replica::stream {
// 只按显式不可变版本闭包组合；缺依赖返回等待，不继承任何可变旧目标。
std::vector<Range> Closure(const Store &, const RangeData &);
bool DependenciesReady(const Store &, const RangeData &) noexcept;
// 合并局部来源图时重新编号，typed table按键合并，普通数组按固定槽合并。
void MergePiece(Value &, const Value &, Allocator &);
void FinishPieces(Value &, Allocator &, std::uint64_t revision);
Json SubjectValue(const Store &, const RangeData &, SubjectKey,
                  const char *domain);
std::map<SubjectKey, Json> SubjectValues(const Store &, const RangeData &,
                                         const char *domain);
Json DomainValue(const Store &, const RangeData &, const char *domain);
// 只用于结构/资源变化；冷模型元数据来自真实网络Baseline，不读取文件或本地世界。
Json StructuralModel(const Store &, const RangeData &, const Value &cold,
                     std::uint64_t connection);
Json DomainModel(const Store &, const RangeData &, const Value &skeleton,
                 std::initializer_list<const char *> domains);
Value WorldValue(WorldScope, Allocator &);
Value VersionValues(const std::vector<Version> &, Allocator &);
} // namespace cs::replica::stream
