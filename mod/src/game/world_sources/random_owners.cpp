#include "IsaacRepentance.h"
#include "game/world_sources/lua_entity.h"
#include "game/world_sources/random_sources.h"
#include "runtime.h"
#include <sstream>
namespace cs::game {
namespace {
std::string Token(const std::string &context,const char *key) {
  const auto prefix="\""+std::string(key)+"\":\"";const auto begin=context.find(prefix);
  if(begin==std::string::npos)return "0";const auto start=begin+prefix.size();return context.substr(start,context.find('"',start)-start);
}
std::string Subject(const ObservationSubjectValue &s,unsigned space){
  return "{\"space\":"+std::to_string(space)+",\"session\":\""+std::to_string(s.session)+"\",\"epoch\":\""+std::to_string(s.epoch)+
    "\",\"local_id\":\""+std::to_string(s.local)+"\",\"use\":\""+std::to_string(s.use)+"\"}";
}
struct Sink final {void *owner;void (*visit)(void *,const RandomBinding &) noexcept;std::string run,room;std::uint32_t errors=0;};
void Emit(Sink &s,void *p,std::string owner,std::string purpose,std::string life,std::string identity) {
  s.visit(s.owner,{p,RandomAlgorithm::XorShift32,std::move(owner),std::move(purpose),std::move(life),std::move(identity)});
}
bool Copy(const void *p,void *out,std::size_t n) noexcept {SIZE_T read=0;return ReadProcessMemory(GetCurrentProcess(),p,out,n,&read)&&read==n;}
}
std::uint32_t EnumerateNativeRandomOwners(void *owner,void (*visit)(void *,const RandomBinding &) noexcept) noexcept {
  if(!IsMain()||!g_Game||!visit)return 1;
  try{
    const auto context=ObservationCaptureContext();Sink s{owner,visit,Token(context,"run"),Token(context,"room_instance")};
    if(s.run=="0")return 1;
    static bool first=true;const auto diagnostic=first&&Settings.diagnostics;first=false;
    if(diagnostic)Trace("random_owner_stage","stage=begin");
    const auto game=[&](void *p,const char *purpose,const std::string &name){
      Emit(s,p,"{\"kind\":\""+name+"\",\"run\":\""+s.run+"\"}",purpose,"run:"+s.run,name+":"+purpose+":"+s.run);
    };
    // J273根/字段分别经实际native函数核定；不从seed值推断owner或身份。
    game(&g_Game->_generationRNG,"Game.GenerationRNG","Game");
    game(&g_Game->_seedEffects._rng,"Seeds.InternalRNG","Seeds");
    game(&g_Game->_itemPool._itemPoolRNG,"ItemPool.GlobalRNG","ItemPool");
    game(&g_Game->_playerManager._rng,"PlayerManager.InternalRNG","PlayerManager");
    game(&g_Game->_ambush.rng,"Ambush.InternalRNG","Ambush");
    for(unsigned i=0;i<31;++i){
      game(&g_Game->_itemPool._pools[i]._rng1,("ItemPool["+std::to_string(i)+"].rng1").c_str(),"ItemPool");
      game(&g_Game->_itemPool._pools[i]._rng2,("ItemPool["+std::to_string(i)+"].rng2").c_str(),"ItemPool");
    }
    for(unsigned i=0;i<37;++i)game(&g_Game->_bossPool._pool[i]._rng,("BossPool["+std::to_string(i)+"].rng").c_str(),"BossPool");
    if(g_Game->_room)Emit(s,&g_Game->_room->_backdrop.rng,"{\"kind\":\"Backdrop\",\"room\":\""+s.room+"\"}","Backdrop.RNG","room:"+s.room,"Backdrop:"+s.run+":"+s.room);
    if(diagnostic)Trace("random_owner_stage","stage=game-complete");
    ObservationEntities(&s,[](void *owner,void *object,std::uint32_t)noexcept{
      auto &s=*static_cast<Sink *>(owner);auto *e=static_cast<Entity *>(object);
      try{
        const auto key=ObservationSubject(e,true);if(!key.local)return;
        const auto id=std::to_string(key.local)+":"+std::to_string(key.use);
        const auto subject=Subject(key,1);Emit(s,&e->_dropRNG,subject,"Entity.GetDropRNG",std::to_string(key.use),"Entity.Drop:"+id);
        if(e->_type!=1)return;
        // 四个getter在J273只访问已分配vector，不懒初始化；直接枚举当前成员，不调用getter或resize。
        const std::uint32_t offsets[]={0x1760,0x176c,0x1778,0x1784};
        const char *names[]={"Collectible","Trinket","Pill","Card"};
        for(unsigned kind=0;kind<4;++kind){
          std::uint32_t v[3]{};if(!Copy(static_cast<unsigned char *>(object)+offsets[kind],v,sizeof(v))){++s.errors;continue;}
          if(!v[0]&&!v[1]&&!v[2])continue;
          if(!v[0]||v[1]<v[0]||v[2]<v[1]||(v[1]-v[0])%16||(v[2]-v[0])%16||(v[2]-v[0])/16>8192){++s.errors;continue;}
          for(unsigned index=0;index<(v[1]-v[0])/16;++index)
            Emit(s,reinterpret_cast<void *>(v[0]+index*16),subject,std::string("EntityPlayer.")+names[kind]+"RNG["+std::to_string(index)+"]",
              std::to_string(key.use),"Player:"+id+":"+names[kind]+":"+std::to_string(index));
        }
      }catch(...){++s.errors;}
    });
    if(diagnostic)Trace("random_owner_stage","stage=entities-complete");
    ObservationGrids(&s,[](void *owner,void *object,std::uint32_t)noexcept{
      auto &s=*static_cast<Sink *>(owner);
      try{const auto key=ObservationSubject(object,true,true);if(key.local)Emit(s,&static_cast<GridEntity *>(object)->_rng,Subject(key,2),
        "GridEntity.GetRNG",std::to_string(key.use),"Grid:"+std::to_string(key.local)+":"+std::to_string(key.use));}catch(...){}
    });
    if(diagnostic)Trace("random_owner_stage","stage=grids-complete errors=%u",s.errors);
    return s.errors;
  }catch(...){return 1;}
}
}
