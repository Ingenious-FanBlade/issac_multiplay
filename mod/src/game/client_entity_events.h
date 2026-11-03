#pragma once
#include "game/client_objects_native.h"
#include "replica/model.h"
#include <deque>
#include <cstddef>

namespace cs::game {
// Entity独立记录拥有自己的scope/序号；不改普通T、成员集合或网络事件水位。
class ClientEntityEvents final {
public:
  struct Receipt final {replica::Kind kind=replica::Kind::Component;std::uint64_t sequence=0,source_scope=0,required_state=0;std::string phase;};
  using Visitor=bool (*)(void *,const replica::Value &,const Receipt &) noexcept;
  using Resolve=bool (*)(void *,const ClientObjectKey &,std::uint64_t) noexcept;
  struct Statistics final {unsigned pending=0,presented=0,expired=0,superseded=0;std::size_t bytes=0;};
  void Reset(const replica::Scope &) noexcept;
  bool Admit(const replica::Message &,const char *&) noexcept;
  bool Visit(std::uint64_t applied,std::uint64_t tick,void *resolver,Resolve,void *,Visitor) noexcept;
  bool Complete(replica::Kind,std::uint64_t sequence,std::uint64_t source,const char *phase,std::uint64_t applied) noexcept;
  unsigned Pending() const noexcept;
  Statistics Status() const noexcept;
private:
  struct Dependency final {ClientObjectKey key{};std::uint64_t shape=0;};
  enum class State {Pending,Presented,Expired,Superseded};
  struct Entry final {
    Receipt receipt;replica::Json body;std::vector<Dependency> dependencies;
    std::uint64_t starts=0,expires=0,offered_at=0;std::size_t bytes=0;
    State state=State::Pending;bool unresolved=false;
  };
  replica::Scope scope_{};std::deque<Entry> entries_;
  std::size_t bytes_=0;std::uint64_t retired_component_=0,retired_event_=0;
};
} // namespace cs::game
