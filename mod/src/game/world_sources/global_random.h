#pragma once
// 原版全局Random的纯值边界；与公开RNG类的seed/shift模型分别命名。
#include <array>
#include <cstdint>
#include <string>
namespace cs::game {
struct GlobalRandomState final {
  std::array<std::uint32_t,624> words{};
  std::uint32_t index=625;
  std::uint8_t crt=0;
  bool readable=false;
};
// 每个观测epoch拥有独立主体和状态块版本；后台仅接收序列化副本。
struct GlobalRandomMirror final {
  GlobalRandomState state{};
  std::uint64_t subject=0,block=0,calls=0,seeds=0,failures=0;
};
void ArmGlobalRandom(bool armed) noexcept;
void GlobalRandomCheckpoint() noexcept;
std::string ObservationGlobalRandomState(bool test_disabled=false) noexcept;
// wrapper捕获原生返回值；观察器绝不额外调用Random，异常时保持原生展开。
class GlobalRandomCall final {
public:
  explicit GlobalRandomCall(bool seed=false,std::uint32_t seed_value=0,void *return_slot=nullptr) noexcept;
  ~GlobalRandomCall();
  void Returned(std::uint32_t result) noexcept;
private:
  GlobalRandomState before_{};
  std::string crt_before_;
  std::uint64_t block_=0,sequence_=0,parent_=0,epoch_=0,session_=0,fault_=0;
  std::array<std::uint32_t,24> callers_{};
  unsigned caller_count_=0;
  std::uint32_t seed_value_=0;
  bool active_=false,seed_=false,returned_=false;
};
}
