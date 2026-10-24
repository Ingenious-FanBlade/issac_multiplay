#pragma once
namespace cs {
// tx-04-04 仅验证命令队列的占位状态，不含 tick、参与者或网络。
struct PlaceholderSession {
  bool hosting = false;
  bool Host() noexcept {
    if (hosting)
      return false;
    hosting = true;
    return true;
  }
  void Stop() noexcept {
    hosting = false;
  }
};
} // namespace cs
