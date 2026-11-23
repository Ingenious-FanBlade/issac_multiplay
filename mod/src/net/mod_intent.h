#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace cs::net {
inline constexpr unsigned kModIntentBytes=512,kModResultBytes=256,kModIntentInFlight=8,kModIntentsPerSecond=20;
// 跨线程意图只持固定宽度身份和声明编码。ordinal 来自已经绑定的 Peer。
struct ModIntent {
  std::uint64_t id=0,connection=0,run=0,floor=0,room=0;
  std::uint32_t member_epoch=0;
  std::uint16_t mod=0,declaration=0,size=0;
  unsigned ordinal=0;
  std::array<std::byte,kModIntentBytes> bytes{};
};
struct ModIntentResult {
  std::uint64_t id=0,connection=0,run=0,floor=0;
  std::uint16_t mod=0,declaration=0,size=0;
  bool ok=false;
  std::array<std::byte,kModResultBytes> bytes{};
};
struct ModIntentResults {
  std::array<ModIntentResult,kModIntentInFlight> values{};
  unsigned count=0;
};
// 房主本机与三个网络端在同一锁下入队，Manager 一次取走入口时已经到达的前缀。
class ModIntentInbox final {
public:
  bool Push(const ModIntent &value) noexcept {
    std::lock_guard lock(mutex_);
    if(value.ordinal>=4||per_peer_[value.ordinal]>=kModIntentInFlight)return false;
    values_[count_++]=value;++per_peer_[value.ordinal];
    return true;
  }
  using Batch=std::array<ModIntent,4*kModIntentInFlight>;
  unsigned Take(Batch &out) noexcept {
    std::lock_guard lock(mutex_);
    for(unsigned i=0;i<count_;++i)out[i]=values_[i];
    const auto count=count_;count_=0;per_peer_={};return count;
  }
private:
  std::mutex mutex_;
  Batch values_{};
  std::array<unsigned,4> per_peer_{};
  unsigned count_=0;
};
// 滑动一秒内只计算已接纳的新意图，重传沿用原序号。
struct ModIntentRate {
  std::array<std::uint64_t,kModIntentsPerSecond> accepted{};
  unsigned next=0;
  bool Ready(std::uint64_t now)const noexcept{return !accepted[next]||now-accepted[next]>=1000;}
  void Accept(std::uint64_t now)noexcept{accepted[next]=now;next=(next+1)%accepted.size();}
};
}
