#pragma once
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include "core/replication_memory.h"

namespace cs::memory {
// 每个可延后释放的owner在创建时占固定名额；最后引用只入侵入式链表，绝不在游戏线程析构大树。
// 队列与存活owner共用上限，释放路径无需分配，也没有“队列满后同步释放”的慢路径。
class ValueReclaimer final {
  struct Item {Item *next=nullptr;virtual ~Item()=default;};
  template<class T> struct Box final : Item {
    template<class... A> explicit Box(A&&... a):value(std::forward<A>(a)...){ }
    T value;
  };
public:
  static ValueReclaimer &Get(){static auto *owner=new ValueReclaimer;return *owner;}
  template<class T,class... A> std::shared_ptr<T> Make(A&&... a){
    auto used=owners_.load(std::memory_order_relaxed);
    do {if(used>=16384)throw std::runtime_error("capture_retirement_capacity");}
    while(!owners_.compare_exchange_weak(used,used+1,std::memory_order_acq_rel));
    Box<T> *box=nullptr;
    try {box=new Box<T>(std::forward<A>(a)...);}catch(...){--owners_;throw;}
    // shared_ptr控制块分配失败同样调用该deleter；名额仅由回收线程退还。
    return {&box->value,[this,box](T *) noexcept {auto *head=pending_.load(std::memory_order_relaxed);
      do {box->next=head;}while(!pending_.compare_exchange_weak(head,box,std::memory_order_release,std::memory_order_relaxed));wake_.notify_one();}};
  }
private:
  ValueReclaimer(){std::thread([this]{Run();}).detach();}
  void Run() noexcept {
    for(;;){auto *items=pending_.exchange(nullptr,std::memory_order_acquire);
      if(!items){std::unique_lock<std::mutex> lock(wait_);wake_.wait_for(lock,std::chrono::milliseconds(20));continue;}
      while(items){auto *next=items->next;delete items;--owners_;items=next;}
    }
  }
  std::atomic<Item *> pending_{nullptr};
  std::atomic<unsigned> owners_{0};
  std::mutex wait_;std::condition_variable wake_;
};
template<class T,class... A> std::shared_ptr<T> DeferredValue(A&&... a){return ValueReclaimer::Get().Make<T>(std::forward<A>(a)...);}
} // namespace cs::memory
