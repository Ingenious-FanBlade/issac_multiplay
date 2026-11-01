#pragma once
#include "game/client_objects_native.h"
#include "replica/model.h"
#include <memory>
#include <string>
#include <vector>

namespace cs::game {
// 纯值A→T计划与原生身份表分别拥有寿命；执行失败后不再按历史A继续差分。
class ClientObjects final {
public:
  ClientObjects() noexcept;
  ~ClientObjects();
  bool PrepareTarget(const replica::Value &target,const replica::Value *applied,const char *&reason) noexcept;
  bool CancelPlanned() noexcept;
  ClientObjectStep Step(const ClientNativeRoots &,unsigned budget=16) noexcept;
  ClientObjectStep Retire(const ClientNativeRoots &) noexcept;
  bool Visit(void *context,ClientObjectVisitor visitor) const noexcept;
private:
  ClientNativeObjects *native_=nullptr;
  std::vector<ClientObjectCommand> commands_;
  std::unique_ptr<replica::Json> candidate_,committed_;
  std::string reason_;
  std::size_t cursor_=0;
  unsigned prefix_=0;
  bool failed_=false,written_=false,retired_=false;
};
} // namespace cs::game
