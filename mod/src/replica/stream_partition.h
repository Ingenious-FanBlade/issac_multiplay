#pragma once
#include "replica/model.h"
namespace cs::replica::stream {
// 只处理已封闭纯值；保持原stream-source-v1分层/图语义，不延迟调用任何来源getter。
Json PartitionSource(const Value &,unsigned domain,std::uint32_t node_limit);
}
