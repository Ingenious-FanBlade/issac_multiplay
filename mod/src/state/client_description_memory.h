#pragma once
#include "state/client_memory.h"
#include <memory>

namespace cs::state {
// 既有描述独立调用沿用其Values租约；二进制批次传入Peer账户，按实际分配计费。
// 两条分配路径互斥，不同时保留全局估算租约与Peer分配器造成重复收费。
template<class T>class ClientDescriptionAllocator {
public:
  using value_type=T;
  using propagate_on_container_move_assignment=std::true_type;
  using propagate_on_container_swap=std::true_type;
  explicit ClientDescriptionAllocator(std::shared_ptr<ClientStateMemory> memory={}):memory_(std::move(memory)) {}
  template<class U>ClientDescriptionAllocator(const ClientDescriptionAllocator<U> &v):memory_(v.Memory()) {}
  T *allocate(std::size_t count){return memory_?ClientStateAllocator<T>(memory_).allocate(count):std::allocator<T>{}.allocate(count);}
  void deallocate(T *p,std::size_t count)noexcept {if(memory_)ClientStateAllocator<T>(memory_).deallocate(p,count);else std::allocator<T>{}.deallocate(p,count);}
  const std::shared_ptr<ClientStateMemory> &Memory()const noexcept{return memory_;}
  template<class U>bool operator==(const ClientDescriptionAllocator<U> &v)const noexcept{return memory_==v.Memory();}
private:std::shared_ptr<ClientStateMemory> memory_;
};
} // namespace cs::state
