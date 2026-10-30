#pragma once
// 旧 profile 保持原 ID、位型、资格及失败语义；域通过此适配输出旧格式，不再反读游戏。
#include "world/common/native_source.h"
namespace cs::world::compatibility {
void CopyNativeFields(const NativeSource &,RawObject &,const Identity &,bool ready=true) noexcept;
}
