#pragma once
// 由统一 Hook 清单调用的 SDK-facing 原始结果复制，不额外执行掉落或随机查询。
#include "IsaacRepentance.h"
#include "game/observation/sources/value_source.h"
namespace cs::game {
void ObserveActualLoot(void *owner,const char *member,bool should_advance,const LootList &values) noexcept;
}
