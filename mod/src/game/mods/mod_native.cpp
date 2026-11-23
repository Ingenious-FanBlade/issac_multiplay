#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/mods/mod_native.h"
#include "game/mods/mod_api.h"
#include "game/common/bridge.h"
#include "game/client/display/client_display_bridge.h"
#include "game/client/player/client_player_spoof_layout.h"
#include "runtime.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <stdexcept>
#include <string_view>

namespace cs::game {
std::array<std::uint64_t,4> CurrentLiveSource() noexcept;
namespace {
constexpr unsigned kRows=6;
using QueryKey=std::tuple<unsigned,int,int>;
using Queries=std::map<QueryKey,int>;
// 查询结果按当前 EntityPlayerPlus 实例隔离，避免重用地址时继承前一个身体。
struct PlayerQueries {
  const void *owner=nullptr;
  Queries values;
};
std::map<const void *,PlayerQueries> host_queries,client_queries,next_queries;
using IntMap=std::map<int,int>;
struct Progress {
  std::array<unsigned char,641> achievements{};
  std::array<int,523> counters{};
  std::array<unsigned char,733> collection{};
  std::array<unsigned char,104> bosses{};
  std::array<unsigned char,46> challenges{};
  std::set<int> extra_achievements,extra_challenges;
  std::array<IntMap,3> bestiary;
};
Progress progress;
bool has_progress=false,has_display=false;
unsigned floor_stage=0,floor_type=0;
std::string floor_name;
std::vector<int> achievement_ids,challenge_ids;
std::array<state::ModValueRef,kRows> installed_rows;
ModCutscene host_cutscene;
std::array<ModCutscene,512> client_cutscenes;
unsigned cutscene_head=0,cutscene_count=0;
std::uint64_t cutscene_serial=0,client_cutscene_seen=0;
bool client_cutscene_active=false;

// 使用官方布局核对过的伴随结构。STL 对象留在进程内，只发送元素。
player_sdk::PlusCache *Cache(Entity_Player *player) noexcept {
  if(!player)return nullptr;
  const auto holder=(std::uintptr_t(player->_entityExtraDataPtrUpperBytes)<<16)|player->_entityExtraDataPtrLowerBytes;
  if(!holder)return nullptr;
  auto *plus=*reinterpret_cast<player_sdk::PlusHeader **>(holder);
  if(!plus||std::strcmp(typeid(*plus).raw_name(),".?AVEntityPlayerPlus@@"))return nullptr;
  return reinterpret_cast<player_sdk::PlusCache *>(reinterpret_cast<unsigned char *>(plus)+
    sizeof(player_sdk::PlusHeader)+sizeof(player_sdk::PlusItems));
}
template<class T> void Put(std::string &bytes,const T &value) {
  bytes.append(reinterpret_cast<const char *>(&value),sizeof(value));
}
void PutText(std::string &bytes,std::string_view value) {
  if(value.size()>UINT16_MAX)throw std::length_error("native_mod_string");
  Put(bytes,std::uint16_t(value.size()));
  bytes.append(value);
}
class Reader {
public:
  explicit Reader(std::string_view bytes):bytes_(bytes){}
  template<class T> T Read() {
    if(bytes_.size()<sizeof(T))throw std::runtime_error("native_mod_truncated");
    T value;std::memcpy(&value,bytes_.data(),sizeof(value));bytes_.remove_prefix(sizeof(value));return value;
  }
  std::string Text() {
    const auto size=Read<std::uint16_t>();
    if(bytes_.size()<size)throw std::runtime_error("native_mod_text");
    std::string text(bytes_.substr(0,size));bytes_.remove_prefix(size);return text;
  }
  bool Empty()const noexcept{return bytes_.empty();}
private:
  std::string_view bytes_;
};
template<class T> void Count(std::string &bytes,const T &container) {
  if(container.size()>UINT16_MAX)throw std::length_error("native_mod_count");
  Put(bytes,std::uint16_t(container.size()));
}
std::set<int> ReadIds(Reader &input) {
  std::set<int> ids;
  for(unsigned n=input.Read<std::uint16_t>();n;--n)
    if(!ids.insert(input.Read<int>()).second)throw std::runtime_error("native_mod_duplicate_id");
  return ids;
}
IntMap ReadIntMap(Reader &input) {
  IntMap values;
  for(unsigned n=input.Read<std::uint16_t>();n;--n){
    const int index=input.Read<int>(),value=input.Read<int>();
    if(!values.emplace(index,value).second)throw std::runtime_error("native_mod_duplicate_entry");
  }
  return values;
}
template<class A> int At(const A &values,int index) noexcept {
  return index>=0&&unsigned(index)<values.size()?values[unsigned(index)]:0;
}
int Bestiary(unsigned kind,int type,int variant) {
  if(!g_Manager)return 0;
  // 690320 会按 bestiaryParentType/Variant 归并，单查普通 EntityConfig 会漏掉父条目。
  using Lookup=EntityConfig_Entity *(__thiscall *)(void *,int,int);
  const auto *entry=reinterpret_cast<Lookup>(Base+0x290320)(&g_Manager->_entityConfig,type,variant);
  if(!entry||!entry->bestiary)return 0;
  const int key=int((std::uint32_t(entry->variant)|(std::uint32_t(entry->id)<<12))<<8);
  const auto &values=progress.bestiary[kind];
  const auto found=values.find(key);
  return found==values.end()?0:found->second;
}
enum class ProgressQuery { Unlocked, Counter, Collection, Challenge, Boss, Encounter, Kill, Death };
int ProgressLua(lua_State *L) {
  if(!ClientBusy()){
    const int count=lua_gettop(L);
    lua_pushvalue(L,lua_upvalueindex(1));lua_insert(L,1);lua_call(L,count,LUA_MULTRET);
    return lua_gettop(L);
  }
  luaL_checkudata(L,1,lua::metatables::PersistentGameDataMT);
  const auto index=luaL_checkinteger(L,2);
  const auto kind=static_cast<ProgressQuery>(lua_tointeger(L,lua_upvalueindex(2)));
  int value=0;
  if(index>=INT_MIN&&index<=INT_MAX){
    const int id=static_cast<int>(index);
    switch(kind){
    case ProgressQuery::Unlocked:{bool result=false;ClientModUnlocked(id,result);value=result;break;}
    case ProgressQuery::Counter:value=At(progress.counters,id);break;
    case ProgressQuery::Collection:value=At(progress.collection,id);break;
    case ProgressQuery::Challenge:value=At(progress.challenges,id)||progress.extra_challenges.count(id);break;
    case ProgressQuery::Boss:value=At(progress.bosses,id);break;
    case ProgressQuery::Encounter:case ProgressQuery::Kill:case ProgressQuery::Death:{
      const auto variant=luaL_checkinteger(L,3);
      if(variant>=INT_MIN&&variant<=INT_MAX)value=Bestiary(unsigned(kind)-unsigned(ProgressQuery::Encounter),id,int(variant));
      break;
    }
    }
  }
  if(kind==ProgressQuery::Counter||kind>=ProgressQuery::Encounter)lua_pushinteger(L,value);
  else lua_pushboolean(L,value);
  return 1;
}
int ReadProgressIds(lua_State *L) {
  // 官方 XML 表给出本机已分配的自定义 ID，而不是猜测连续编号。
  for(const auto category:{26,9}){
    auto &ids=category==26?achievement_ids:challenge_ids;
    ids.clear();
    lua_getglobal(L,"XMLData");
    const int xml=lua_absindex(L,-1);
    lua_getfield(L,xml,"GetNumEntries");lua_pushinteger(L,category);lua_call(L,1,1);
    const auto count=lua_tointeger(L,-1);lua_pop(L,1);
    for(lua_Integer i=1;i<=count;++i){
      lua_getfield(L,xml,"GetEntryByOrder");lua_pushinteger(L,category);lua_pushinteger(L,i);lua_call(L,2,1);
      if(lua_istable(L,-1)){
        lua_getfield(L,-1,"id");
        int numeric=0;const auto id=lua_tointegerx(L,-1,&numeric);
        if(numeric&&id>=(category==26?641:46)&&id<=INT_MAX)ids.push_back(int(id));
        lua_pop(L,1);
      }
      lua_pop(L,1);
    }
    lua_pop(L,1);
    std::sort(ids.begin(),ids.end());ids.erase(std::unique(ids.begin(),ids.end()),ids.end());
  }
  return 0;
}
int ReadCustomChallenges(lua_State *L) {
  auto &values=*static_cast<std::vector<int> *>(lua_touserdata(L,1));
  lua_getglobal(L,"Isaac");lua_getfield(L,-1,"GetPersistentGameData");lua_call(L,0,1);
  const int pgd=lua_absindex(L,-1);
  for(const int id:challenge_ids){
    lua_getfield(L,pgd,"IsChallengeCompleted");lua_pushvalue(L,pgd);lua_pushinteger(L,id);lua_call(L,2,1);
    if(lua_toboolean(L,-1))values.push_back(id);
    lua_pop(L,1);
  }
  return 0;
}
}

void RegisterNativeModRows(state::ModSchema &schema) {
  for(unsigned id=0;id<kRows;++id){
    auto layout=std::make_shared<state::ModLayout>();
    layout->entity=id==unsigned(NativeModRow::Player);
    state::ModNode field{};
    field.kind=state::ModNodeKind::String;field.fixed=2;field.worst=65536;field.max=65534;field.end=1;
    layout->nodes.push_back(field);
    schema.layouts.emplace(state::ModRecordKey{kNativeMod,std::uint16_t(id),state::kModGlobal},std::move(layout));
  }
  if(g_LuaEngine&&g_LuaEngine->_state){
    auto *L=g_LuaEngine->_state;
    lua_pushcfunction(L,ReadProgressIds);
    if(lua_pcall(L,0,0,0)!=LUA_OK){lua_pop(L,1);achievement_ids.clear();challenge_ids.clear();}
  }
  host_queries.clear();host_cutscene={};
}

bool ClientModPlayerValue(void *player,unsigned kind,int first,int second,int &value) noexcept {
  if(!IsMain()||!ClientBusy())return false;
  const auto owner=client_queries.find(player);
  if(owner==client_queries.end()||Cache(static_cast<Entity_Player *>(player))!=owner->second.owner)return false;
  const auto found=owner->second.values.find({kind,first,second});
  if(found==owner->second.values.end())return false;
  value=found->second;return true;
}
void HostModPlayerValue(void *player,unsigned kind,int first,int second,int value) noexcept {
  if(!IsMain()||!HostBusy()||ClientBusy())return;
  auto *cache=Cache(static_cast<Entity_Player *>(player));
  if(!cache)return;
  try {
    auto &entry=host_queries[player];
    if(entry.owner!=cache)entry={cache,{}};
    // 保存该身体真实执行过的查询，限制历史参数缓存，优先保留当前槽。
    if(entry.values.size()>=256&&entry.values.find({kind,first,second})==entry.values.end()){
      const auto old=entry.values.lower_bound({2,INT_MIN,INT_MIN});
      if(old!=entry.values.end())entry.values.erase(old);
    }
    entry.values.insert_or_assign({kind,first,second},value);
  }catch(...){}
}
std::string CaptureNativeModPlayer(void *object) {
  auto *player=static_cast<Entity_Player *>(object);
  const auto *cache=Cache(player);
  if(!cache)return {};
  std::string bytes;
  Put(bytes,std::uint8_t(cache->camo));Put(bytes,cache->previous_health);
  Count(bytes,cache->tags);
  for(const auto &tag:cache->tags)PutText(bytes,tag);
  const std::map<std::string,double> sorted(cache->results.begin(),cache->results.end());
  Count(bytes,sorted);
  for(const auto &[key,value]:sorted){PutText(bytes,key);Put(bytes,value);}
  Queries values;
  const auto observed=host_queries.find(player);
  if(observed!=host_queries.end()&&observed->second.owner==cache)values=observed->second.values;
  if(cache->previous_health>=0)values.try_emplace(QueryKey{0,0,0},cache->previous_health);
  Count(bytes,values);
  for(const auto &[key,value]:values){
    Put(bytes,std::uint8_t(std::get<0>(key)));Put(bytes,std::get<1>(key));Put(bytes,std::get<2>(key));Put(bytes,value);
  }
  return bytes;
}
std::string CaptureNativeModDisplay() {
  if(!g_Game)return {};
  std::string bytes;
  Put(bytes,unsigned(g_Game->_stage));Put(bytes,unsigned(g_Game->_stageType));
  Put(bytes,g_Game->GetHUD()->_bossHPBarFill);
  // 原生 GetName 经 REPENTOGON GetLevelName，保留实际优先级和生效的最终文字。
  PutText(bytes,reinterpret_cast<Level *>(g_Game)->GetName());
  return bytes;
}
std::string CaptureNativeModProgress() {
  if(!g_Manager)return {};
  auto *pgd=g_Manager->GetPersistentGameData();
  std::string bytes;
  std::array<unsigned char,641> unlocked{};
  for(unsigned id=0;id<unlocked.size();++id)unlocked[id]=pgd->Unlocked(int(id));
  Put(bytes,unlocked);
  bytes.append(reinterpret_cast<const char *>(pgd->eventCounter),sizeof(pgd->eventCounter));
  bytes.append(reinterpret_cast<const char *>(pgd->itemsCollection),sizeof(pgd->itemsCollection));
  bytes.append(reinterpret_cast<const char *>(pgd->bosses),sizeof(pgd->bosses));
  // 当前 REPENTOGON 的 Lua API 包含挑战 45。底层声明的 C 数组只到 44。
  bytes.append(reinterpret_cast<const char *>(pgd->challenges),sizeof(pgd->challenges));
  Put(bytes,*(reinterpret_cast<const unsigned char *>(pgd)+0xe6f+45));
  std::vector<int> extra;
  for(const int id:achievement_ids)if(pgd->Unlocked(id))extra.push_back(id);
  Count(bytes,extra);for(const int id:extra)Put(bytes,id);
  extra.clear();extra.reserve(challenge_ids.size());
  if(g_LuaEngine&&g_LuaEngine->_state&&!challenge_ids.empty()){
    auto *L=g_LuaEngine->_state;
    lua_pushcfunction(L,ReadCustomChallenges);lua_pushlightuserdata(L,&extra);
    if(lua_pcall(L,1,0,0)!=LUA_OK){lua_pop(L,1);throw std::runtime_error("native_mod_challenge_query");}
  }
  Count(bytes,extra);for(const int id:extra)Put(bytes,id);
  return bytes;
}
std::string CaptureNativeModBestiary(NativeModRow row) {
  if(!g_Manager)return {};
  const auto &book=g_Manager->GetPersistentGameData()->bestiary;
  const auto &values=row==NativeModRow::Encounters?book.encounters:row==NativeModRow::Kills?book.kills:book.deaths;
  std::string bytes;
  Count(bytes,values);
  for(const auto &[key,value]:values){Put(bytes,key);Put(bytes,value);}
  return bytes;
}

void InstallNativeModRow(NativeModRow row,void *object,const state::ModValueRef &data) {
  const auto index=unsigned(row);
  if(index>=installed_rows.size()||!data||data->bytes.size()<2)return;
  // 进度是只读镜像，影子字节共享时无需再次拷贝图鉴容器。
  const bool memoize=row!=NativeModRow::Player&&row!=NativeModRow::Display;
  if(memoize&&installed_rows[index]==data)return;
  Reader input({reinterpret_cast<const char *>(data->bytes.data()+2),data->bytes.size()-2});
  switch(row){
  case NativeModRow::Player:{
    auto *cache=Cache(static_cast<Entity_Player *>(object));
    if(!cache)return;
    const auto camo=input.Read<unsigned char>();
    const auto health=input.Read<int>();
    std::set<std::string> tags;
    for(unsigned n=input.Read<std::uint16_t>();n;--n)tags.insert(input.Text());
    std::unordered_map<std::string,double> results;
    for(unsigned n=input.Read<std::uint16_t>();n;--n){auto key=input.Text();results.emplace(std::move(key),input.Read<double>());}
    Queries queries;
    for(unsigned n=input.Read<std::uint16_t>();n;--n){
      const auto kind=input.Read<unsigned char>();
      const auto first=input.Read<int>(),second=input.Read<int>(),value=input.Read<int>();
      if(kind>4||!queries.emplace(QueryKey{kind,first,second},value).second)throw std::runtime_error("native_mod_player_query");
    }
    if(!input.Empty()||camo>1)throw std::runtime_error("native_mod_player_tail");
    next_queries.insert_or_assign(object,PlayerQueries{cache,std::move(queries)});
    if(cache->tags!=tags)cache->tags.swap(tags);
    if(cache->results!=results)cache->results.swap(results);
    cache->previous_health=health;cache->camo=camo!=0;
    break;
  }
  case NativeModRow::Display:{
    const auto stage=input.Read<unsigned>(),type=input.Read<unsigned>();
    const auto boss=input.Read<float>();
    auto name=input.Text();
    if(!input.Empty())throw std::runtime_error("native_mod_display_tail");
    floor_stage=stage;floor_type=type;floor_name=std::move(name);has_display=true;
    if(g_Game)g_Game->GetHUD()->_bossHPBarFill=boss;
    break;
  }
  case NativeModRow::Progress:{
    auto achievements=input.Read<decltype(progress.achievements)>();
    auto counters=input.Read<decltype(progress.counters)>();
    auto collection=input.Read<decltype(progress.collection)>();
    auto bosses=input.Read<decltype(progress.bosses)>();
    auto challenges=input.Read<decltype(progress.challenges)>();
    auto extra_achievements=ReadIds(input),extra_challenges=ReadIds(input);
    if(!input.Empty())throw std::runtime_error("native_mod_progress_tail");
    progress.achievements=achievements;progress.counters=counters;
    progress.collection=collection;progress.bosses=bosses;progress.challenges=challenges;
    progress.extra_achievements.swap(extra_achievements);progress.extra_challenges.swap(extra_challenges);
    has_progress=true;break;
  }
  case NativeModRow::Encounters:case NativeModRow::Kills:case NativeModRow::Deaths:{
    auto values=ReadIntMap(input);
    if(!input.Empty())throw std::runtime_error("native_mod_bestiary_tail");
    progress.bestiary[unsigned(row)-unsigned(NativeModRow::Encounters)].swap(values);break;
  }
  }
  if(memoize)installed_rows[index]=data;
}
void EndNativeModPlayers() noexcept {
  client_queries.swap(next_queries);next_queries.clear();
}
void RetireNativeModState() noexcept {
  next_queries.clear();client_queries.clear();installed_rows={};progress={};has_progress=false;has_display=false;floor_name.clear();
}
bool ClientModUnlocked(int id,bool &value) noexcept {
  if(!IsMain()||!ClientBusy()||(!ClientNativeDisplayActive()&&!has_progress))return false;
  value=id!=-2&&(id<=0||(id<641?At(progress.achievements,id)!=0:progress.extra_achievements.count(id)!=0));
  return true;
}
bool ClientModFloorName(unsigned stage,unsigned type,std::string &value) {
  if(!IsMain()||!ClientNativeDisplayActive()||!has_display||stage!=floor_stage||type!=floor_type)return false;
  value=floor_name;return true;
}
int PlayCutsceneLua(lua_State *L) {
  const auto id=luaL_checkinteger(L,1);
  const bool clear=lua_isboolean(L,2)&&lua_toboolean(L,2)&&id<=26;
  if(ClientBusy())return 0;
  const int count=lua_gettop(L);
  lua_pushvalue(L,lua_upvalueindex(1));lua_insert(L,1);lua_call(L,count,0);
  if(id>=0&&id<=INT_MAX)ObserveModCutscene(unsigned(id),clear);
  return 0;
}
void ObserveModCutscene(unsigned id,bool clear) noexcept {
  if(IsMain()&&HostBusy()&&!ClientBusy()&&cutscene_serial!=UINT64_MAX)
    if(const auto source=CurrentLiveSource();source[2])
      host_cutscene={++cutscene_serial,id,clear,source[2]};
}
ModCutscene HostModCutscene() noexcept {return host_cutscene;}
void QueueClientModCutscene(ModCutscene next) noexcept {
  if(!IsMain()||next.serial<=client_cutscene_seen||next.id>INT_MAX)return;
  if(cutscene_count==client_cutscenes.size()){
    Trace("mod_state","phase=cutscene_queue_full serial=%llu",next.serial);
    return;
  }
  client_cutscenes[(cutscene_head+cutscene_count++)%client_cutscenes.size()]=next;
  client_cutscene_seen=next.serial;
}
bool ClientModCutsceneActive() noexcept {return IsMain()&&client_cutscene_active;}
void ResetClientModCutscene() noexcept {
  cutscene_head=cutscene_count=0;client_cutscene_seen=0;
  if(client_cutscene_active&&g_Manager)g_Manager->GetCutscene()->Show(0);
  client_cutscene_active=false;
}
bool AdvanceClientModCutscene(bool retired) noexcept {
  if(!IsMain()||!g_Manager)return false;
  auto *cutscene=g_Manager->GetCutscene();
  const auto &request=client_cutscenes[cutscene_head];
  if(!client_cutscene_active&&cutscene_count&&(!request.clear||retired)){
    const auto next=request;
    cutscene_head=unsigned((cutscene_head+1)%client_cutscenes.size());--cutscene_count;
    // Show 的公开官方 Hook 解析自定义资源。绕过 Manager::ShowCutscene 的 PGD 计数和清档命令。
    ModApiReplayAudio audio;
    cutscene->Show(int(next.id));
    client_cutscene_active=cutscene->_state!=0;
  }
  if(!client_cutscene_active)return false;
  // J273 92EABD：Manager 内的过场专用推进／绘制入口。没有 Game Update 或存档清理。
  ModApiReplayAudio audio;
  reinterpret_cast<int (__thiscall *)(Cutscene *)>(Base+0x5373d0)(cutscene);
  client_cutscene_active=cutscene->_state!=0;
  return true;
}
void InstallNativeModLua(lua_State *L) {
  lua_getglobal(L,"Isaac");
  if(lua_istable(L,-1)){
    lua_getfield(L,-1,"PlayCutscene");
    if(lua_isfunction(L,-1)){lua_pushcclosure(L,PlayCutsceneLua,1);lua_setfield(L,-2,"PlayCutscene");}
    else lua_pop(L,1);
  }
  lua_pop(L,1);
  luaL_getmetatable(L,lua::metatables::PersistentGameDataMT);
  if(lua_istable(L,-1)){
    constexpr const char *names[]={"Unlocked","GetEventCounter","IsItemInCollection",
      "IsChallengeCompleted","IsBossKilled","GetBestiaryEncounterCount","GetBestiaryKillCount","GetBestiaryDeathCount"};
    for(unsigned i=0;i<std::size(names);++i){
      lua_getfield(L,-1,names[i]);
      if(!lua_isfunction(L,-1)){lua_pop(L,1);continue;}
      lua_pushinteger(L,i);lua_pushcclosure(L,ProgressLua,2);
      lua_setfield(L,-2,names[i]);
    }
  }
  lua_pop(L,1);
}
}
