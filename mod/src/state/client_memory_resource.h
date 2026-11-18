#pragma once
#include "state/client_memory.h"
#include <memory_resource>

namespace cs::state {
// 批次索引的arena上游：大块只记账一次，实际保留字节仍受同一Peer／进程池上限约束。
// arena必须先析构，随后才释放本适配器持有的账本；不跨线程共享可变arena。
class ClientStateMemoryResource final:public std::pmr::memory_resource {
public:
  ClientStateMemoryResource(std::shared_ptr<ClientStateMemory> memory,memory::Pool pool)
    :memory_(std::move(memory)),pool_(pool) {}
private:
  void *do_allocate(std::size_t bytes,std::size_t alignment) override {
    return memory_->Allocate(pool_,bytes,alignment);
  }
  void do_deallocate(void *value,std::size_t bytes,std::size_t alignment) override {
    memory_->Deallocate(pool_,value,bytes,alignment);
  }
  bool do_is_equal(const std::pmr::memory_resource &other)const noexcept override {return this==&other;}
  std::shared_ptr<ClientStateMemory> memory_;
  memory::Pool pool_;
};
}
