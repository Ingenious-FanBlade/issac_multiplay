#pragma once
#include "state/mod_state.h"
#include <string>

struct lua_State;
namespace cs::game {
// 保留段与公开 Mod 声明共用 World 版本和实体成员身份。
inline constexpr std::uint16_t kNativeMod=0xfffe;
enum class NativeModRow : std::uint16_t { Player, Display, Progress, Encounters, Kills, Deaths };
void RegisterNativeModRows(state::ModSchema &);
std::string CaptureNativeModPlayer(void *);
std::string CaptureNativeModDisplay();
std::string CaptureNativeModProgress();
std::string CaptureNativeModBestiary(NativeModRow);
void InstallNativeModRow(NativeModRow,void *,const state::ModValueRef &);
void EndNativeModPlayers() noexcept;
void RetireNativeModState() noexcept;
void InstallNativeModLua(lua_State *);
bool ClientModPlayerValue(void *,unsigned kind,int first,int second,int &value) noexcept;
void HostModPlayerValue(void *,unsigned kind,int first,int second,int value) noexcept;
bool ClientModUnlocked(int id,bool &value) noexcept;
bool ClientModFloorName(unsigned stage,unsigned type,std::string &value);
// 同一序号在 World 事件和提前退休的可靠控制里共用。
struct ModCutscene {std::uint64_t serial=0;unsigned id=0;bool clear=false;std::uint64_t run=0;};
void ObserveModCutscene(unsigned id,bool clear) noexcept;
ModCutscene HostModCutscene() noexcept;
void QueueClientModCutscene(ModCutscene) noexcept;
bool AdvanceClientModCutscene(bool retired) noexcept;
bool ClientModCutsceneActive() noexcept;
void ResetClientModCutscene() noexcept;
}
