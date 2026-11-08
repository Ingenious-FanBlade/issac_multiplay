#pragma once
#include "core/owned_source.h"
namespace cs::replica::stream {
// 展开同窗不可变块只发生在纯值worker；内部块索引不进入协议或规范身份。
Json ExpandSource(const capture::Source &);
}
