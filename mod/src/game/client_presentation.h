#pragma once
#include "game/client_presentation_native.h"
#include "replica/model.h"
#include <string>

namespace cs::game {
// 目标解析、原生维护与像素提交分开；仅完整原生维护结束后允许 Scene 发布。
class ClientPresentation final {
public:
  ClientPresentation() noexcept;
  ~ClientPresentation();
  bool PrepareTarget(const replica::Value &,const char *&reason,const ClientRoomState &) noexcept;
  ClientPresentationStep Step(const ClientNativeRoots &) noexcept;
  ClientPresentationStep Render(const ClientNativeRoots &) noexcept;
  ClientPresentationStep Retire(const ClientNativeRoots &) noexcept;
private:
  ClientNativePresentation *native_=nullptr;
  ClientPresentationDescription target_{};
  std::string reason_;
  bool planned_=false,ready_=false;
};
} // namespace cs::game
