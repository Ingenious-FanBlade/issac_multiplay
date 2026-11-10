#pragma once
#include "game/client/entity/client_objects_native.h"
#include <cstdio>
namespace cs::game {
// 主线程更新的有界纯值摘要；原生owner销毁后reason仍有效，状态发布只复制这些值。
struct ClientPlayerDiagnostics final {
  ClientObjectKey body{},dependency{};
  std::uint64_t instance=0,shape=0;
  unsigned prefix=0,body_role=0;
  int player_type=-1;
  std::array<char,32> phase{};
  std::array<char,256> field{};
  std::array<char,192> reason{};
  // 外部字段路径不能伪造状态响应的新行；固定数组仍显式终止，不分配内存。
  void Normalize() noexcept {
    const auto clean=[](auto &value){value.back()=0;for(auto &c:value){if(!c)break;if(static_cast<unsigned char>(c)<32||c==127)c='?';}};
    clean(phase);clean(field);clean(reason);
  }
  void Record(const ClientObjectStep &step,const char *stage) noexcept {
    body=step.key;dependency=step.dependency;instance=step.instance;shape=step.shape_revision;
    prefix=step.object_prefix;body_role=step.body_role;player_type=step.player_type;field=step.field;
    std::snprintf(phase.data(),phase.size(),"%s",step.phase&&step.phase[0]?step.phase:stage);
    std::snprintf(reason.data(),reason.size(),"%s",step.reason?step.reason:"unknown");
    Normalize();
  }
};
}
