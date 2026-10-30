#include "world/common/publication.h"
namespace cs::world::observation {
PublicationQueue::~PublicationQueue() { const auto bytes=bytes_.load(); if(bytes) ledger_.Release(bytes); }
bool PublicationQueue::Push(std::string value) noexcept {
  const auto head=head_.load(std::memory_order_relaxed);
  const auto bytes=std::uint64_t(value.capacity())+sizeof(std::string)+64;
  if(std::uint32_t(head-tail_.load(std::memory_order_acquire))>=queue_.size()||bytes>kPublicationBytes-bytes_.load(std::memory_order_acquire)||!ledger_.Charge(bytes)) { ++loss_; return false; }
  queue_[head%queue_.size()]=std::move(value); sizes_[head%queue_.size()]=bytes;
  bytes_.fetch_add(bytes,std::memory_order_relaxed);
  head_.store(head+1,std::memory_order_release); return true;
}
bool PublicationQueue::Pop(std::string &value) noexcept {
  const auto tail=tail_.load(std::memory_order_relaxed);
  if(tail==head_.load(std::memory_order_acquire)) return false;
  const auto bytes=sizes_[tail%queue_.size()];
  value=std::move(queue_[tail%queue_.size()]);
  bytes_.fetch_sub(bytes,std::memory_order_release); ledger_.Release(bytes);
  tail_.store(tail+1,std::memory_order_release); return true;
}
bool PublicationQueue::Empty() noexcept { return tail_.load(std::memory_order_acquire)==head_.load(std::memory_order_acquire); }
}
