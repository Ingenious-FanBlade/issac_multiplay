#include "IsaacRepentance.h"
#include "game/observation/binary_probability.h"
#include "game/observation/binary_display_inputs.h"
#include "game/common/context.h"
#include "game/common/bridge.h"
#include <cmath>

namespace cs::game {
// RE-7已核内部天使函数无参数、以XMM0返回；不能用普通float函数指针偷换为ST0 ABI。
bool QueryBinaryDisplayInputs(void *game,void *room,BinaryDisplayInputs &out) noexcept {
  if(!IsMain()||ClientBusy()||game!=g_Game||!room||!g_Manager||!g_Game||g_Game->GetCurrentRoom()!=room)return false;
  try {
    BinaryDisplayInputs value;value.devil_total=static_cast<Room *>(room)->GetDevilRoomChance();
    const auto query=Base+0x3ef590;float angel=0;
    __asm {
      call dword ptr [query]
      movss angel,xmm0
    }
    value.angel_share=angel;value.planetarium=g_Game->GetPlanetariumChance();
    value.progress_blocked=g_Manager->AchievementUnlocksDisallowed(false);
    if(!std::isfinite(value.devil_total)||!std::isfinite(value.angel_share)||value.angel_share<0||value.angel_share>1||!std::isfinite(value.planetarium))return false;
    if(game!=g_Game||g_Game->GetCurrentRoom()!=room)return false;out=value;return true;
  }catch(...){return false;}
}
// 用户明确授权：只在Host正常显示边界集中复用C++概率查询，不经Lua API，不运行HUD Update／Recompute。
// 最终保留官方REPENTOGON的GetPlanetariumChance资格修正；不复制其整套计算算法。
bool QueryBinaryHudProbability(void *game,void *room,std::uint8_t requested,const BinaryProbabilityInput &input,std::array<float,3> &out) noexcept {
  if(!IsMain()||ClientBusy()||!game||!room||!requested||(requested&~7u)||game!=g_Game)return false;
  auto *owner=static_cast<Game *>(game);if(owner->GetCurrentRoom()!=room)return false;
  try {
    if(requested&3){auto *players=owner->GetPlayerManager();if(!players||!std::isfinite(input.angel_modifier))return false;
      const auto has=[&](int id){return players->FirstCollectibleOwner(static_cast<CollectibleType>(id),nullptr,true)!=nullptr;};
      const auto raw=static_cast<Room *>(room)->GetDevilRoomChance();if(!std::isfinite(raw))return false;
      const float total=input.eligible?std::fmin(std::fmax(raw,0.0f),1.0f):0.0f;
      float angel=(input.game_flags&0x41)?0.5f:1.0f;
      for(const auto id:{238,239,584})if(has(id))angel=float(angel+float(float(1.0f-angel)*0.25f));
      if(players->AnyoneHasTrinket(static_cast<TrinketType>(7)))angel=float(angel+float(float(1.0f-angel)*0.5f));
      if(input.donation>9)angel=float(angel+float(float(1.0f-angel)*0.5f));
      if(input.level_flags&2)angel=float(angel+float(float(1.0f-angel)*0.25f));
      const bool beggar=(input.level_flags&8)!=0,devil_beggar=(input.level_flags&16)!=0;
      if(beggar!=devil_beggar){if(beggar)angel=float(angel+float(float(1.0f-angel)*0.1f));
        if(devil_beggar)angel=float(angel-float(float(1.0f-angel)*0.1f));}
      if(input.angel_modifier>0||(has(601)&&(input.game_flags&0x20))||has(584))angel=float(float(float(1.0f-angel)*input.angel_modifier)+angel);
      else if(!(input.game_flags&0x20)||input.deals)angel=0;
      if(has(499))angel=77;angel=std::fmin(std::fmax(angel,0.0f),1.0f);
      const bool combined=has(498);out[0]=combined?float(total*100.0f):float(float(float(1.0f-angel)*total)*100.0f);
      out[1]=combined?float(total*100.0f):float(float(angel*total)*100.0f);
    }
    if(requested&4){const auto chance=owner->GetPlanetariumChance();if(!std::isfinite(chance))return false;out[2]=float(chance*100.0f);}
    return true;
  }catch(...){return false;}
}
}
