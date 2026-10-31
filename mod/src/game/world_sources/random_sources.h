#pragma once
// 跨SDK边界只交付已复制随机状态；原生地址仅用于本进程主线程关联。
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace cs::game {
enum class RandomAlgorithm : unsigned { XorShift32=1, MTRNG, MsvcMT19937, Xoshiro256, CrtRand };
struct RandomBinding final {
  void *address=nullptr;
  RandomAlgorithm algorithm=RandomAlgorithm::XorShift32;
  std::string owner,purpose,lifetime;
  std::string identity;
};
struct RandomWords final {
  std::vector<std::uint32_t> words;
  bool readable=false;
};
// 注册表的实现留在C++20侧；SDK适配不携带WorldCapture、LuaState或C++对象到后台。
struct RandomRegistry;
std::shared_ptr<RandomRegistry> MakeRandomRegistry();
void ArmRandomSources(bool armed) noexcept;
bool RandomSourcesNeeded() noexcept;
void BindRandomSource(const RandomBinding &,bool observed_return=false) noexcept;
void RetireRandomSource(void *address,const char *reason) noexcept;
std::string CaptureRandomSources() noexcept;
// Lua只接收小型摘要；完整自有JSON由普通窗口的C++协调器直接合并，避免跨语言往返解析。
std::string RandomSourcesSummary() noexcept;
std::string RandomSourceReference(void *address) noexcept;
void RandomSourcesCheckpoint() noexcept;
std::uint32_t EnumerateNativeRandomOwners(void *owner,void (*visit)(void *,const RandomBinding &) noexcept) noexcept;
// SDK来源枚举只在已有普通采集窗口调用，不懒创建Player物品随机流。
void CaptureRuntimeRandomOwners() noexcept;
bool RuntimeRandomSourcesComplete() noexcept;
void RegisterRuntimeRandomFunctions() noexcept;
void NativeRandomLuaReturned(unsigned source,void *lua,int results) noexcept;
std::uint64_t NativeRandomLuaEnter(unsigned source,void *lua) noexcept;
void NativeRandomLuaAbortPending() noexcept;
// 有界纯值事件入口，kind不由Lua数据冒充；用于C++实现的Lua随机函数。
bool PublishRandomObservation(const char *schema,std::string payload,bool complete,std::uint32_t nodes) noexcept;
RandomWords ReadRandomWords(void *,RandomAlgorithm) noexcept;
std::string EncodeRandomWords(const RandomWords &,RandomAlgorithm);
// 原生wrapper正常返回/异常展开的观察，不更改原始函数及其调用次数。
class NativeRandomCall final {
public:
  NativeRandomCall(void *object,RandomAlgorithm algorithm,const char *member,
                   std::uint32_t argument=0,bool initialize=false,std::uint32_t argument2=0) noexcept;
  ~NativeRandomCall();
  void Returned(std::uint32_t value,bool floating=false) noexcept;
private:
  struct Ticket;
  std::unique_ptr<Ticket> ticket_;
};
// 中央Hook清单动态入口安装后提供纯值状态provider，不重复安装或会话卸载。
void SetRuntimeRandomSource(unsigned source,void *state,std::uint32_t words) noexcept;
void PrepareRuntimeRandomHook(unsigned source,void *target) noexcept;
RandomWords CaptureRuntimeRandomWords(unsigned source) noexcept;
std::string RuntimeRandomArguments(void *lua) noexcept;
std::string RuntimeRandomResults(void *lua,int count) noexcept;
std::string RuntimeRandomDiagnostics() noexcept;
std::string TestNativeRandomSources(const char *token,const char *mode,unsigned count,unsigned seed);
}
