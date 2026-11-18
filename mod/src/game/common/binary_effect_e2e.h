#pragma once

#ifdef ISAACCS_E2E
#include "runtime.h"
#include <algorithm>
#include <array>
#include <cstdint>

// 原生显示桥仍以C++17编译，只声明开关，不引入C++20状态协议类型。
namespace cs::replica::stream { bool E2ETraceEnabled() noexcept; }

namespace cs::game {
// 只观察真实普通快照与已提交客机世界中的击杀血效，不驱动实体或动画。
struct BinaryBloodEffects {
  static constexpr std::array<unsigned,7> variants{2,3,5,7,42,70,77};
  std::array<unsigned,variants.size()> counts{};

  void Observe(unsigned type,unsigned variant) noexcept {
    if(type!=1000)return;
    const auto match=std::find(variants.begin(),variants.end(),variant);
    if(match!=variants.end())++counts[static_cast<std::size_t>(match-variants.begin())];
  }

  void TraceChange(const char *role,std::uint32_t frame,const BinaryBloodEffects &prior) const noexcept {
    if(!replica::stream::E2ETraceEnabled()||counts==prior.counts)return;
    // 只在数量变化时记录，保留 Host 帧以便与 Client 已安装状态对齐。
    Trace("binary_blood_effect_e2e_fault_probe","side=%s frame=%u variants_2_3_5_7_42_70_77=%u/%u/%u/%u/%u/%u/%u",
      role,frame,counts[0],counts[1],counts[2],counts[3],counts[4],counts[5],counts[6]);
  }
};

void TraceBinaryBloodRender(void *game,const char *role) noexcept;
}
#endif
