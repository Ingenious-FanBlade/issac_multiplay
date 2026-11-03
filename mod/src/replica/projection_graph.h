#pragma once
#include "replica/model.h"

namespace cs::replica {
// Lua图编号仅在同一component内有引用时有意义；不与稳定世界身份或resource_alias混用。
// 投影完成后检查引用闭合，再裁掉无引用table的node；不修改原始观测或展开图副本。
void PruneProjectionGraph(Value &);
// 模型接纳拒绝悬空、零编号和有歧义的图引用；独立组件在共用根投影中完成同样检查。
void CheckProjectionGraph(const Value &);
} // namespace cs::replica
