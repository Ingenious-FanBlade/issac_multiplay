#pragma once
#include "replica/stream_model.h"
#ifdef ISAACCS_E2E
namespace cs::replica::stream {
// 编码失败时只取一次自有纯值，分成有界诊断页；不再访问游戏来源。
void E2EEncodingSource(const RangeData &,const char *reason) noexcept;
}
#endif
