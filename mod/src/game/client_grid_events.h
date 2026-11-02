#pragma once
#include "replica/model.h"
#include <deque>

namespace cs::game {
// 独立 Lifecycle/Render 只交接表现资料，不修改完整状态、成员或 Applied ACK 水位。
// 27 的消费者在实际呈现后显式 Complete；入队成功不表示效果已经完成。
class ClientGridEvents final {
public:
  void Reset(const replica::Scope &) noexcept;
  bool Admit(const replica::Message &,const char *&reason) noexcept;
  using Visitor=bool (*)(void *,const replica::Value &,std::uint64_t) noexcept;
  bool Visit(std::uint64_t applied,void *,Visitor) const noexcept;
  bool Complete(std::uint64_t sequence,std::uint64_t source_scope,const char *phase) noexcept;
  unsigned Pending() const noexcept;
private:
  struct Entry final {
    std::uint64_t sequence=0,source_scope=0,required=0;
    std::string phase;
    replica::Json value;
    bool complete=false;
    mutable bool offered=false;
    std::size_t bytes=0;
  };
  replica::Scope scope_{};
  std::deque<Entry> entries_;
  std::size_t bytes_=0;
  std::uint64_t retired_sequence_=0;
};
} // namespace cs::game
