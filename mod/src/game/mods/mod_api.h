#pragma once
#include "net/mod_manifest.h"
#include "net/mod_intent.h"
#include "state/mod_state.h"

struct lua_State;
namespace cs::game {
// Lua 装载、关闭及握手快照均由游戏主线程调用。
bool InstallModApi(lua_State *) noexcept;
// 主线程按冻结清单采样原始键，位序在两端一致。
void SampleModKeys(std::uint64_t &held,std::uint64_t &edge) noexcept;
// 读取 ControllerIndex 0 原生键盘上的一组按键：第 i 个键按住置 held 第 i 位，本帧触发置 edge 第 i 位
// （count≤64）。InputManager 的按键查询会重新获取非递归后端锁，这里只在持有输入租约时直接调用
// 键盘 vtable 的两个只读入口；设备不唯一、vtable 或入口不符时保持全零。实现在 participants/topology.cpp。
void ReadNativeKeyboardKeys(const int *keys,std::size_t count,std::uint64_t &held,std::uint64_t &edge) noexcept;
bool ReadModAction(int controller,int action,unsigned kind,float &value) noexcept;
bool ReadModKey(unsigned ordinal,std::uint64_t key,bool edge) noexcept;
void CloseModApi() noexcept;
const char *CaptureModManifest(net::ModManifestRef &) noexcept;
state::ModSchemaRef FrozenModSchema() noexcept;

// 只在当前原生 World 窗口解析地址，交给后台的值只含稳定身份。
using ModCaptureEntity=bool (*)(void *,const void *,std::uint64_t &,std::uint64_t &) noexcept;
struct ModCaptureObject {
  void *address=nullptr;
  state::ModEntityId id;
  std::array<std::uint32_t,3> shape{};
  bool retained=false;
};
using ModCaptureAt=bool (*)(void *,unsigned,ModCaptureObject &) noexcept;
bool HostModIdentity(const void *,state::ModEntityId &) noexcept;
state::ModRawRef CaptureModWorld(const state::ModRawRef &,std::uint64_t sample,unsigned frame,bool transition,
    void *,ModCaptureEntity,unsigned objects,ModCaptureAt) noexcept;
void CommitModWorld(const state::ModRawRef &) noexcept;
std::string ModStateFields();

// 客机读取已发布成员和当前参与者，Lua 调用期间不持有后台对象地址。
struct ModClientLookup {
  void *context=nullptr;
  void *(*entity)(void *,std::uint16_t,bool &known,std::uint64_t &instance) noexcept=nullptr;
  unsigned players=0;
};
// 返回 false 表示本批没有走到 CLIENT_ENTITY_INSTALLED 派发（VM 不一致、进不了安装作用域或存储失败）。
// 调用方据此保留旧的已通知成员表，下一份 World 重新列出这些新成员，INSTALLED 与 REMOVED 保持成对。
bool InstallModWorld(const state::ModShadowRef &,bool new_world,bool full,const ModClientLookup &,
    void *const *joined=nullptr,std::size_t joined_count=0) noexcept;
void NotifyModRemoved(void *) noexcept;
void CountUnplannedModRemoval() noexcept;
bool ModWantsEntityLifecycle() noexcept;
// Lua 接口和队列之间只交换固定大小的意图，原生身体只在 Manager 入口借用。
bool HostModIntentContext(net::ModIntent &) noexcept;
bool ClientModIntentContext(net::ModIntent &) noexcept;
bool SubmitHostModIntent(const net::ModIntent &) noexcept;
bool SubmitClientModIntent(const net::ModIntent &) noexcept;
std::uint16_t HostModIntentMember(const void *) noexcept;
std::uint16_t ClientModIntentMember(const void *) noexcept;
void *ResolveHostModIntentMember(const net::ModIntent &,std::uint16_t) noexcept;
void RunModIntent(const net::ModIntent &,void *player,net::ModIntentResult &) noexcept;
void DispatchModIntentResult(const net::ModIntentResult &,bool client) noexcept;
std::uint64_t CancelModIntents(bool client) noexcept;
void RetireModWorld() noexcept;
void ModInstallBlocked() noexcept;
void ModApiBeforeManager() noexcept;
void RejectHostModReload() noexcept;
void ModApiConsoleCommand(const char *) noexcept;
inline constexpr int kModMainMenuRender=1023;
// 原生分派和逐条执行共用同一套回调资格；无法识别的 ID 返回 -1。
int ModApiCallbackId(lua_State *) noexcept;
bool ModApiCallbackAllowedInPhase(int) noexcept;

// 仅由原生 LuaCallbackCaller::CallInputAction 建立客机主菜单输入作用域。
class ModApiMenuInputScope final {
public:
  explicit ModApiMenuInputScope(bool main) noexcept;
  ~ModApiMenuInputScope() noexcept;
  ModApiMenuInputScope(const ModApiMenuInputScope &) = delete;
  ModApiMenuInputScope &operator=(const ModApiMenuInputScope &) = delete;
private:
  bool previous_;
};

// 标记客机回放的原生音频调用，嵌套播放继承标记。
class ModApiReplayAudio final {
public:
  explicit ModApiReplayAudio(bool active=true) noexcept;
  ~ModApiReplayAudio() noexcept;
  ModApiReplayAudio(const ModApiReplayAudio &) = delete;
  ModApiReplayAudio &operator=(const ModApiReplayAudio &) = delete;
private:
  bool active_;
};
}
