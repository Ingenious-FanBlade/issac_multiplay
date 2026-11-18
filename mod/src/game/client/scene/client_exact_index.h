#pragma once
#include <cstdint>
#include <memory_resource>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace cs::game {
// 描述期只插入／等值查询的完整数字键索引，不依赖排序或节点地址。
// 预留容量最多半满；碰撞逐项比较完整键，内存由调用者的计费arena拥有。
template<class Key> class ClientExactIndex final {
  struct Slot {Key first{};std::uint32_t second=0;bool occupied=false;};
  std::pmr::vector<Slot> slots_;
  std::size_t count_=0;
  static std::uint32_t Hash(const Key &key) noexcept {
    std::uint32_t h=2166136261u;
    std::apply([&](auto... value){((h=(h^std::uint32_t(value))*16777619u),...);},key);
    h^=h>>16;h*=0x85ebca6bu;h^=h>>13;h*=0xc2b2ae35u;return h^(h>>16);
  }
public:
  explicit ClientExactIndex(std::pmr::memory_resource *memory):slots_(memory){}
  void reserve(std::size_t count){
    if(count_||count>SIZE_MAX/4)throw std::length_error("binary_exact_index_capacity");
    std::size_t capacity=2;while(capacity<count*2)capacity*=2;
    slots_.resize(capacity);
  }
  std::size_t size()const noexcept{return count_;}
  void clear() noexcept {slots_.clear();count_=0;}
  const Slot *end()const noexcept{return nullptr;}
  const Slot *find(const Key &key)const noexcept {
    if(slots_.empty())return nullptr;
    auto at=Hash(key)&(slots_.size()-1);
    for(std::size_t n=0;n<slots_.size();++n,at=(at+1)&(slots_.size()-1)){
      const auto &slot=slots_[at];if(!slot.occupied)return nullptr;if(slot.first==key)return &slot;
    }
    return nullptr;
  }
  std::pair<const Slot *,bool> emplace(const Key &key,std::uint32_t value){
    if(slots_.empty())throw std::length_error("binary_exact_index_unreserved");
    auto at=Hash(key)&(slots_.size()-1);
    for(std::size_t n=0;n<slots_.size();++n,at=(at+1)&(slots_.size()-1)){
      auto &slot=slots_[at];if(slot.occupied){if(slot.first==key)return {&slot,false};continue;}
      if(count_>=slots_.size()/2)throw std::length_error("binary_exact_index_full");
      slot={key,value,true};++count_;return {&slot,true};
    }
    throw std::length_error("binary_exact_index_full");
  }
};
} // namespace cs::game
