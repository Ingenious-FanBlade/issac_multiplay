#pragma once
#include "world/common/frame_read_regions.h"
#include <cstddef>
#include <cstdint>
#include <span>

namespace cs::world::binary {
// 主线程捕获段只持有当段序号和失效状态。共享页表不授予对象资格。
class CaptureReadSession final {
public:
  struct Counters : NativeReadCounts {
    std::uint64_t copies=0,bytes=0,string_blocks=0,failures=0;
    std::uint64_t root_resource_checks=0,root_resource_revocations=0;
  };
  bool Begin(bool (*current)(void *) noexcept=nullptr,void *owner=nullptr) noexcept;
  void End() noexcept {active_=false;}
  void Revoke() noexcept;
  // 外部资源水位只撤销数据证言，页权限由共享表独立处理。
  void RevokeForResourceChange() noexcept {failed_=revoked_=true;}
  // 入口核定读段后借用其序号，外部写水位移到根尾；独立读段仍逐次检查。
  class RootRead final {
  public:
    RootRead(CaptureReadSession &session,bool defer) noexcept:session_(session) {session_.deferred_serial_=defer?session_.serial_:0;}
    ~RootRead() {session_.deferred_serial_=0;}
    RootRead(const RootRead &)=delete;
    RootRead &operator=(const RootRead &)=delete;
  private:
    CaptureReadSession &session_;
  };
  bool ReviewRoot() noexcept;
  bool ContinueAfterObjectFailure() noexcept;
  bool Good() const noexcept { return active_&&!failed_&&(deferred_serial_||!current_||current_(owner_)); }
  std::uint64_t Serial() const noexcept { return Good()?serial_:0; }
  // 优化探测失败交回正常读取，不能因此把本根标成采集失败。
  bool Probe(std::uint64_t address,std::span<std::byte> output) noexcept;
  bool Copy(std::uint64_t address,void *output,std::size_t size) noexcept;
  // owner 提供可读 extent 时按区域和 64B 上限切块；未知 extent 的调用方传 1。
  bool StringBlock(std::uint64_t address,std::span<std::byte> output,std::size_t &size) noexcept;
  const Counters &Statistics() const noexcept { return counters_; }
private:
  bool Reject() noexcept {failed_=true;++counters_.failures;return false;}
  std::uint64_t serial_=0,deferred_serial_=0;
  bool ExternalCurrent() noexcept;
  bool active_=false,failed_=false,revoked_=false;
  bool (*current_)(void *) noexcept=nullptr;
  void *owner_=nullptr;
  Counters counters_;
};
}
