#pragma once
#include "game/client/presentation/client_presentation_native.h"
#include "replica/model.h"
#include <string>

namespace cs::game {
// 目标解析、原生维护与像素提交分开；仅完整原生维护结束后允许 Scene 发布。
class ClientPresentation final {
public:
  ClientPresentation() noexcept;
  ~ClientPresentation();
  bool PrepareTarget(const replica::Value &,const char *&reason,const ClientRoomState &,const ClientMapTarget &) noexcept;
  ClientPresentationStep Step(const ClientNativeRoots &) noexcept;
  ClientPresentationStep Render(const ClientNativeRoots &) noexcept;
  ClientPresentationStep Retire(const ClientNativeRoots &) noexcept;
  // 热普通范围沿同一个native owner提交；完整Player已存在，不由局部Room DTO猜profile。
  void SetStreamPlayers() noexcept {target_.full_players=true;target_.stream_values=true;}
  bool StreamCompatible() const noexcept {return ClientPresentationStreamCompatible(native_,target_);}
  std::uint64_t StreamValueBytes() const;
  void StreamMapFacts(std::vector<std::uint64_t> facts) {facts.push_back(target_.effective_curses);target_.stream_map_facts=std::move(facts);}
  ClientPresentationStep FastSort(const ClientNativeRoots &roots) noexcept {return ClientPresentationFastSort(native_,roots);}
private:
  ClientNativePresentation *native_=nullptr;
  ClientPresentationDescription target_{};
  std::string reason_;
  bool planned_=false,ready_=false;
};
} // namespace cs::game
