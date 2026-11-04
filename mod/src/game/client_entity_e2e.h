#pragma once
#include <string>
#include <cstdint>
#ifdef ISAACCS_E2E
#include <chrono>
#endif
namespace cs::game {
// 故障只在已解释的正常返回边界触发一次；产品编译消除全部注入分支。
#ifdef ISAACCS_E2E
bool ClientEntityTestFault(const char *point) noexcept;
bool ClientTestDummyAcquire(void *,unsigned,bool,void *&) noexcept;
std::string ClientEntityTestArm(const std::string &,const std::string &) noexcept;
std::string ClientEntityTestStatus(const std::string &) noexcept;
std::string ClientTestInspect(const std::string &) noexcept;
struct ClientNativeObjects;
std::string ClientTestInspectNative(const ClientNativeObjects *) noexcept;
void ClientEntityTestTime(const char *,std::uint64_t) noexcept;
// Lua来源用平凡起止时间，错误longjmp不跨越新增的C++析构职责。
inline std::uint64_t ClientEntityTestClock() noexcept {
  return std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}
// steady_clock只量具名代码包围窗口的经过时间，并非线程CPU；和harness含IPC/等待的墙钟另报。
struct ClientEntityTestMeasure final {
  const char *name;std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
  explicit ClientEntityTestMeasure(const char *value):name(value){}
  ~ClientEntityTestMeasure(){ClientEntityTestTime(name,std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count()));}
};
#else
inline bool ClientEntityTestFault(const char *) noexcept {return false;}
inline bool ClientTestDummyAcquire(void *,unsigned,bool,void *&) noexcept {return false;}
inline std::uint64_t ClientEntityTestClock() noexcept {return 0;}
inline void ClientEntityTestTime(const char *,std::uint64_t) noexcept {}
struct ClientEntityTestMeasure final {explicit ClientEntityTestMeasure(const char *){}};
#endif
}
