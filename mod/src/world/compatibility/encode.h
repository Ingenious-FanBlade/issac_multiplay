#pragma once
#include "world/model.h"
#include <string>
namespace cs::world::compatibility {
// v2 的 native 来源投影与 v1 共用同一次 RawBatch，引用输出规范 ID 而非原生地址。
std::string EncodeDomain(const BatchHeader &,const RawObject *,const RawMembership *,ObjectDomain,bool &truncated);
}
