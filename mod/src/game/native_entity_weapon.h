#pragma once
#include "IsaacRepentance.h"
#include "runtime.h"
#include <array>
#include <stdexcept>

namespace cs::game {
// J273全部Weapon的ClearReferences槽及其受管EntityPtr字段。网络只传固定名称的关系序列，不能指定写偏移。
struct NativeWeaponLayout final {const char *name="";std::array<unsigned,7> offsets{};unsigned count=0;};
inline NativeWeaponLayout EntityWeaponLayout(const Weapon *weapon){
 if(!weapon)throw std::runtime_error("native_weapon_missing");
 const auto clear=(*reinterpret_cast<const std::uintptr_t *const *>(weapon))[5]-cs::Base;
 switch(clear){
 case 0x5c5050:return {"base",{8},1};
 case 0x5d7e10:return {"single",{8,0x44},2};
 case 0x5d4240:return {"brimstone",{8,0x48},2};
 case 0x5d6a00:return {"knife",{8,0x44,0x48,0x4c,0x50},5};
 case 0x5d0990:return {"bone",{8,0x44,0x48,0x4c,0x50,0x54,0x58},7};
 case 0x5e0d50:return {"urn",{8,0x4c},2};
 default:throw std::runtime_error("native_weapon_reference_layout");
 }
}
}
