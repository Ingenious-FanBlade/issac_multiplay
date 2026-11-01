#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/native_room_grid.h"
#include "game/presentation.h"
#include "game/client.h"
#include "runtime.h"
#include "game/native_entity_weapon.h"
#include <optional>
#include <GL/gl.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>

namespace cs::game {
namespace {
using J=rapidjson::Document;using V=rapidjson::Value;using A=J::AllocatorType;
[[noreturn]] void Fail(const char *s){throw std::runtime_error(s);}
const V &At(const V &v,const char *k){if(!v.IsObject()||!v.HasMember(k))Fail("native_missing_field");return v[k];}
std::string Str(const V &v){if(!v.IsString())Fail("native_string");return {v.GetString(),v.GetStringLength()};}
std::int64_t Int(const V &v){if(v.IsInt64())return v.GetInt64();if(v.IsUint64()&&v.GetUint64()<=INT64_MAX)return std::int64_t(v.GetUint64());
 if(v.IsString())return std::stoll(Str(v));if(v.IsObject()&&At(v,"type")=="int64")return std::stoll(Str(At(v,"value")));Fail("native_integer");}
float Num(const V &v){if(!v.IsNumber()||!std::isfinite(v.GetDouble())||std::abs(v.GetDouble())>3.402823466e38)Fail("native_number");return float(v.GetDouble());}
bool Flag(const V &v){if(!v.IsBool())Fail("native_boolean");return v.GetBool();}
V S(const std::string &s,A &a){return V(s.data(),unsigned(s.size()),a);}
void Put(V &v,const char *k,V value,A &a){v.AddMember(V(k,a),std::move(value),a);}
std::string Text(const V &v){rapidjson::StringBuffer b;rapidjson::Writer<rapidjson::StringBuffer>w(b);if(!v.Accept(w))Fail("native_json");return {b.GetString(),b.GetSize()};}
J Parse(const std::string &s){if(s.size()>32u<<20)Fail("native_byte_budget");J j;j.Parse<rapidjson::kParseIterativeFlag|rapidjson::kParseValidateEncodingFlag>(s.data(),s.size());if(j.HasParseError()||!j.IsObject())Fail("native_json");return j;}
const V &Resource(const V &v){return At(v,"type")=="resource"?At(v,"state"):v;}
const V &RoomValue(const J &j){return At(At(At(j,"components"),"room"),"state");}
const V &GridValues(const J &j){return At(At(At(At(j,"components"),"grid"),"state"),"objects");}
std::string Path(const V &v){auto p=Str(v);if(p.empty()||p.size()>512||p[0]=='/'||p.find(':')!=p.npos||p.find('\0')!=p.npos||p.find("..")!=p.npos||p.find('\\')!=p.npos)Fail("native_resource_path");return p;}
Vector Vec(const V &v){return Vector(Num(At(v,"x")),Num(At(v,"y")));}
ColorMod Color(const V &v){float cr=0,cg=0,cb=0,ca=0;
 if(v.HasMember("GetColorize")){for(const auto &e:At(v["GetColorize"],"entries").GetArray()){const auto key=Str(e["key"]);const auto n=Num(e["value"]);if(key=="R")cr=n;else if(key=="G")cg=n;else if(key=="B")cb=n;else if(key=="A")ca=n;}}
 return ColorMod(Num(v["R"]),Num(v["G"]),Num(v["B"]),Num(v["A"]),Num(v["RO"]),Num(v["GO"]),Num(v["BO"]),cr,cg,cb,ca);}
// 只把已经过纯值预算校验的目标压入Lua；不保留userdata或Lua借用到下一帧。
void Push(lua_State *L,const V &v,unsigned depth=0){if(depth>64)Fail("native_lua_depth");
 if(v.IsNull())lua_pushnil(L);else if(v.IsBool())lua_pushboolean(L,v.GetBool());else if(v.IsInt64())lua_pushinteger(L,lua_Integer(v.GetInt64()));
 else if(v.IsNumber())lua_pushnumber(L,v.GetDouble());else if(v.IsString())lua_pushlstring(L,v.GetString(),v.GetStringLength());
 else if(v.IsArray()){lua_createtable(L,int(v.Size()),0);unsigned n=0;for(const auto &e:v.GetArray()){Push(L,e,depth+1);lua_rawseti(L,-2,++n);}}
 else if(v.IsObject()){lua_createtable(L,0,int(v.MemberCount()));for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){Push(L,i->value,depth+1);lua_setfield(L,-2,i->name.GetString());}}
 else Fail("native_lua_value");}
int adapter_ref=LUA_NOREF;
void Adapter(lua_State *L,const char *method){
 if(adapter_ref==LUA_NOREF){const char *code=
#include "native_room_grid_lua.inc"
 ;if(luaL_loadbuffer(L,code,std::strlen(code),"IsaacCS.NativeRoomGrid")!=LUA_OK||lua_pcall(L,0,1,0)!=LUA_OK)Fail("native_adapter_load");adapter_ref=luaL_ref(L,LUA_REGISTRYINDEX);}
 lua_rawgeti(L,LUA_REGISTRYINDEX,adapter_ref);lua_getfield(L,-1,method);lua_remove(L,-2);if(!lua_isfunction(L,-1))Fail("native_adapter_function");
}
template<class T> std::string Lua(const char *method,T *object,lua::Metatables meta,const V *state=nullptr,bool result=false){
 auto *L=g_LuaEngine?g_LuaEngine->_state:nullptr;if(!L)Fail("native_lua_unavailable");const auto top=lua_gettop(L);
 try{Adapter(L,method);lua::luabridge::UserdataPtr::push(L,object,meta);if(state)Push(L,*state);
  if(lua_pcall(L,state?2:1,result?1:0,0)!=LUA_OK){const char *e=lua_tostring(L,-1);const auto message=e?std::string(e):"native_adapter_call";lua_settop(L,top);throw std::runtime_error(message);}
  std::string out;if(result){std::size_t n=0;const auto *p=lua_tolstring(L,-1,&n);if(!p||n>2u<<20)Fail("native_readback_bytes");out.assign(p,n);}lua_settop(L,top);return out;
 }catch(...){lua_settop(L,top);throw;}
}
// 两个callback registry入口均替换为nil返回函数；仅在本方原生窗口内生效，退出即恢复。
// 直接调用本方适配器不经gameplay dispatcher，必要原生初始化因此仍可运行。
int NoCallback(lua_State *){return 0;}
struct Callbacks final {
 lua_State *L=nullptr;std::map<int,int> saved;
 Callbacks(){L=g_LuaEngine?g_LuaEngine->_state:nullptr;if(!L)Fail("native_lua_unavailable");
  const int keys[]={g_LuaEngine->_runCallbackRegistryKey,g_LuaEngine->runCallbackRegistry?g_LuaEngine->runCallbackRegistry->key:LUA_NOREF};
  for(int key:keys){if(key<0||saved.count(key))continue;lua_rawgeti(L,LUA_REGISTRYINDEX,key);const auto ref=luaL_ref(L,LUA_REGISTRYINDEX);saved.emplace(key,ref);lua_pushcfunction(L,NoCallback);lua_rawseti(L,LUA_REGISTRYINDEX,key);}}
 ~Callbacks(){for(const auto &e:saved){lua_rawgeti(L,LUA_REGISTRYINDEX,e.second);lua_rawseti(L,LUA_REGISTRYINDEX,e.first);luaL_unref(L,LUA_REGISTRYINDEX,e.second);}}
};
// 配置由Authority完整公开值重建；Spawns只恢复已选配置，不调用PickEntry或生成房间。
RoomConfig_Room Config(const V &value){const auto &f=At(Resource(value),"fields");RoomConfig_Room c;
 c.StageId=int(Int(f["RoomConfigRoom.StageID"]));c.Mode=int(Int(f["RoomConfigRoom.Mode"]));c.Type=int(Int(f["RoomConfigRoom.Type"]));
 c.Variant=unsigned(Int(f["RoomConfigRoom.Variant"]));c.Subtype=int(Int(f["RoomConfigRoom.Subtype"]));c.Name=Str(f["RoomConfigRoom.Name"]);
 c.Shape=int(Int(f["RoomConfigRoom.Shape"]));c.Width=char(Int(f["RoomConfigRoom.Width"]));c.Height=char(Int(f["RoomConfigRoom.Height"]));
 c.Doors=unsigned(Int(f["RoomConfigRoom.Doors"]));c.originalVariant=unsigned(Int(f["RoomConfigRoom.OriginalVariant"]));
 c.Difficulty=int(Int(f["RoomConfigRoom.Difficulty"]));c.InitialWeight=Num(f["RoomConfigRoom.InitialWeight"]);c.Weight=Num(f["RoomConfigRoom.Weight"]);
 const auto &spawns=At(At(f,"RoomConfigRoom.Spawns"),"values");if(!spawns.IsArray()||spawns.Size()>65535||Int(f["RoomConfigRoom.SpawnCount"])!=spawns.Size())Fail("native_config_spawns");
 c.SpawnCount=static_cast<unsigned short>(spawns.Size());if(c.SpawnCount)c.Spawns=new RoomSpawn[c.SpawnCount];
 for(unsigned i=0;i<spawns.Size();++i){auto &s=c.Spawns[i];const auto &sf=At(spawns[i],"fields");s.X=static_cast<unsigned short>(Int(sf["RoomConfigSpawn.X"]));s.Y=static_cast<unsigned short>(Int(sf["RoomConfigSpawn.Y"]));
  s.SumWeights=Num(sf["RoomConfigSpawn.SumWeights"]);const auto &entries=At(sf["RoomConfigSpawn.Entries"],"values");if(entries.Size()>255||Int(sf["RoomConfigSpawn.EntryCount"])!=entries.Size())Fail("native_config_entries");
  s.CountEntries=static_cast<unsigned char>(entries.Size());if(s.CountEntries)s.Entries=new RoomEntry[s.CountEntries];
  for(unsigned e=0;e<entries.Size();++e){const auto &ef=At(entries[e],"fields");auto &dst=s.Entries[e];dst.type=unsigned(Int(ef["RoomConfigEntry.Type"]));dst.variant=static_cast<unsigned short>(Int(ef["RoomConfigEntry.Variant"]));dst.subtype=static_cast<unsigned short>(Int(ef["RoomConfigEntry.Subtype"]));dst.weight=Num(ef["RoomConfigEntry.Weight"]);}}
 return c;
}
struct Description final {
 // SDK结构内含STL成员。根用raw storage＋原生构造／析构，避免C++隐式析构再次释放原生容器。
 RoomDescriptor *root=nullptr;RoomDescriptor &value;RoomConfig_Room data,override_data;
 Description():root(static_cast<RoomDescriptor *>(::operator new(sizeof(RoomDescriptor)))),value(*root){
  std::memset(root,0,sizeof(RoomDescriptor));try{value.constructor();}catch(...){::operator delete(root);throw;}}
 ~Description(){value.Data=nullptr;value.OverrideData=nullptr;value.destructor();::operator delete(root);}
 void Set(const V &v){
  value.GridIndex=int(Int(v["grid_index"]));value.SafeGridIndex=int(Int(v["safe_grid_index"]));value.ListIndex=int(Int(v["list_index"]));value.Dimension=int(Int(v["dimension"]));
  value.AllowedDoors=int(Int(v["allowed_doors"]));value.DisplayFlags=int(Int(v["display_flags"]));value.VisitedCount=int(Int(v["visited_count"]));value.Flags=int(Int(v["flags"]));
  value.DecorationSeed=int(Int(v["decoration_seed"]));value.SpawnSeed=int(Int(v["spawn_seed"]));value.AwardSeed=int(Int(v["award_seed"]));value.BossDeathSeed=unsigned(Int(v["boss_death_seed"]));
  value.ClearCount=short(Int(v["clear_count"]));value.PitsCount=int(Int(v["pits_count"]));value.Group=int(Int(v["group"]));value.GreedSubType=int(Int(v["greed_subtype"]));
  value.DeliriumDistance=int(Int(v["delirium_distance"]));for(unsigned s=0;s<8;++s)value.Doors[s]=int(Int(v["doors"][s]));
  const auto &d=v["data"];data.StageId=int(Int(d["stage"]));data.Mode=int(Int(d["mode"]));data.Type=int(Int(d["type"]));data.Variant=unsigned(Int(d["variant"]));data.Subtype=int(Int(d["subtype"]));
  data.Shape=int(Int(d["shape"]));data.Width=char(Int(d["width"]));data.Height=char(Int(d["height"]));
  // SDK把这两个32位原生mask声明成bitset<8>，构造转换会抹掉virtual等高位，按具名mask保留全部位。
  const auto flags=std::uint32_t(Int(d["flags"])),quest=std::uint32_t(Int(d["quest_doors"]));static_assert(sizeof(data.Flags)==4&&sizeof(data.QuestDoors)==4,"J273 config mask");
  std::memcpy(&data.Flags,&flags,4);std::memcpy(&data.QuestDoors,&quest,4);value.Data=&data;
 }
};
struct Scene;
struct Environment;
std::unique_ptr<Environment> EnvironmentCreate();
void EnvironmentDetach(Environment &,Room *);
void EnvironmentApply(Environment &,Room *,const V &);
V EnvironmentCapture(Room *,A &);
thread_local Scene *bound=nullptr;
#include "native_entity_owner.inc"
void RetireNativeEntities(Scene &s);
// 仅显式E2E token可设置的一次性故障，覆盖原生变更已经发生后的停显／完整恢复。
std::string injected_failure;
void InjectFailure(const char *phase){if(injected_failure==phase){injected_failure.clear();Fail("native_injected_partial_failure");}}
struct Scene final {
 Room *room=nullptr;Game *game=nullptr;std::unique_ptr<Camera> camera;
 bool destroy_failed=false;
 std::map<std::pair<int,int>,std::unique_ptr<Description>> descriptions;
 Description *current=nullptr;
 std::map<std::uint64_t,GridEntity *> grids;
 // Entity的原生池、身份映射及必要Player只属于当前scene，后台没有这些地址。
 NativeEntityFactory entity_factory;
 std::map<std::uint64_t,Entity *> entities;
 // 只索引本方target文档；每次目标替换后重建，避免每个实体重新遍历整张纯值列表。
 std::map<std::uint64_t,const V *> entity_values;
 std::map<std::uint64_t,Entity_Player *> entity_players;
 std::map<std::uint64_t,std::uint64_t> entity_shapes;
 std::map<std::uint64_t,std::string> entity_categories;
 std::map<const void *,unsigned> entity_order_fragments;
 std::vector<Entity_Player *> player_roster;
 Entity_Player *player_esau[4]{};
 std::size_t entity_vector_bytes=0;
 std::map<void *,std::size_t> entity_vector_capacities;
 J target;NativeRoomGridTransaction receipt;
 std::mt19937 presentation_random{5489};
 std::uint32_t render_random=1;
 unsigned decor_cursor=0;
 KAGE_SmartPointer_ImageBase color_target;
 std::unique_ptr<Environment> environment;
 Scene(){target.SetObject();}
 void Destroy();
 ~Scene(){if(!destroy_failed)try{Destroy();}catch(...){destroy_failed=true;}}
};
// 保存／恢复具名Game头，不复制或回滚含指针的Game对象，不修改访客持久进度。
struct Binding final {
 Scene &s;Game *g;Room *old_room;EntityFactory *old_factory=nullptr;
 // 只交换已核定vector头和四个借用槽；访客manager的容器/缓冲从不被复制或释放。
 unsigned char old_roster[12]{};Entity_Player *old_esau[4]{};bool private_entities=false;
 unsigned stage,stage_type,curses,current,last,starting,dimension,last_dimension,enter,leave,difficulty,challenge,level_flags;
 int frame;float lightning=0,lighting[2]{};unsigned long long game_flags;std::uint32_t old_render_random=0;Callbacks callbacks;
 Binding(Scene &scene):s(scene),g(g_Game),old_room(g?g->_room:nullptr){
  if(!IsMain()||!ClientBusy()||!g||bound||s.game!=g)Fail("native_binding_owner");
  stage=g->_stage;stage_type=g->_stageType;curses=g->_curses;current=g->_currentRoomIdx;last=g->_lastRoomIdx;starting=g->_startingRoomIdx;dimension=g->_currentDimensionIdx;last_dimension=g->_lastRoomDimensionIdx;
  enter=g->_enterDoor;leave=g->_leaveDoor;difficulty=g->_difficulty;challenge=g->_challenge;frame=g->_frameCount;level_flags=g->_levelStateFlags;game_flags=g->_gameStateFlags;lightning=g->_lightningIntensity;
  const auto &c=At(RoomValue(s.target),"native_context");
  // 所有可抛异常的解码先完成；首个原生字段写入之后只执行不抛异常的标量覆盖。
  const unsigned ns=unsigned(Int(c["stage"])),nt=unsigned(Int(c["stage_type"])),nc=unsigned(Int(c["curses"])),
    nr=unsigned(Int(c["current_room"])),nl=unsigned(Int(c["previous_room"])),nd=unsigned(Int(c["dimension"])),
    ne=unsigned(Int(c["enter_door"])),nv=unsigned(Int(c["leave_door"])),nf=unsigned(Int(c["difficulty"])),nh=unsigned(Int(c["challenge"])),
    nflags=unsigned(Int(c["level_flags"])),nstarting=unsigned(Int(c["starting_room"]));const int nframe=int(Int(c["game_frame"]));const auto ngame=std::stoull(Str(c["game_flags"]));
  const auto nlightning=Num(RoomValue(s.target)["state"]["fields"]["Room.GetLightningIntensity"]);
  const float nlighting[]={Num(c["lighting_blend"]),Num(c["lighting_intensity"])};
  old_factory=g->_entityFactory;private_entities=s.entity_factory.root!=nullptr;
  if(private_entities){std::memcpy(old_roster,&g->_playerManager._playerList,12);std::memcpy(old_esau,g->_playerManager._esauJrState,sizeof(old_esau));
   g->_entityFactory=static_cast<EntityFactory *>(s.entity_factory.root);
   // std::vector的三指针头与J273 SDK一致；本方buffer只在主线程借用窗口内对原生Getter可见。
   auto **first=s.player_roster.empty()?nullptr:s.player_roster.data();auto **last=first?first+s.player_roster.size():nullptr;
   Entity_Player **header[]={first,last,last};
   std::memcpy(&g->_playerManager._playerList,header,sizeof(header));std::memcpy(g->_playerManager._esauJrState,s.player_esau,sizeof(old_esau));}
  std::memcpy(lighting,reinterpret_cast<char *>(g)+0x25a90,sizeof(lighting));
  g->_room=s.room;g->_stage=ns;g->_stageType=nt;g->_curses=nc;g->_currentRoomIdx=nr;g->_lastRoomIdx=nl;g->_startingRoomIdx=nstarting;
  g->_currentDimensionIdx=nd;g->_lastRoomDimensionIdx=nd;g->_enterDoor=ne;g->_leaveDoor=nv;g->_difficulty=nf;g->_challenge=nh;
  g->_frameCount=nframe;g->_levelStateFlags=nflags;g->_gameStateFlags=ngame;g->_lightningIntensity=nlightning;
  std::memcpy(reinterpret_cast<char *>(g)+0x25a90,nlighting,sizeof(nlighting));
  // 404F60专用渲染XorShift单独归属本场景，退出时恢复借用槽；不写玩法随机历史。
  std::memcpy(&old_render_random,reinterpret_cast<void *>(Base+0x7aa104),4);std::memcpy(reinterpret_cast<void *>(Base+0x7aa104),&s.render_random,4);bound=&s;
 }
 ~Binding(){if(bound!=&s)return;bound=nullptr;g->_room=old_room;g->_stage=stage;g->_stageType=stage_type;g->_curses=curses;g->_currentRoomIdx=current;g->_lastRoomIdx=last;g->_startingRoomIdx=starting;
  if(private_entities){g->_entityFactory=old_factory;std::memcpy(&g->_playerManager._playerList,old_roster,12);std::memcpy(g->_playerManager._esauJrState,old_esau,sizeof(old_esau));}
  g->_currentDimensionIdx=dimension;g->_lastRoomDimensionIdx=last_dimension;g->_enterDoor=enter;g->_leaveDoor=leave;g->_difficulty=difficulty;g->_challenge=challenge;
  g->_frameCount=frame;g->_levelStateFlags=level_flags;g->_gameStateFlags=game_flags;g->_lightningIntensity=lightning;
  std::memcpy(reinterpret_cast<char *>(g)+0x25a90,lighting,sizeof(lighting));
  std::memcpy(&s.render_random,reinterpret_cast<void *>(Base+0x7aa104),4);std::memcpy(reinterpret_cast<void *>(Base+0x7aa104),&old_render_random,4);}
};
void Scene::Destroy(){if(destroy_failed)Fail("native_retire_quarantined");if(!room)return;try{Binding binding(*this);
 RetireNativeEntities(*this);
 if(!Settings.world_e2e_token.empty())Trace("native_retire","phase=camera");
 // J273 SDK的Camera::destructor实为ret4的删除入口（7E03F0），还会用游戏CRT释放根。
 // 根归本DLL；六个16字节环形缓冲归游戏CRT，按同一原生free逐个释放后只在本DLL释放根。
 room->_Camera=nullptr;if(camera){
  const auto native_free=reinterpret_cast<void (__cdecl *)(void *)>(Base+0x686bc9);
  for(unsigned offset=0;offset<0x60;offset+=0x10){void *buffer=nullptr;std::memcpy(&buffer,reinterpret_cast<char *>(camera.get())+offset,sizeof(buffer));native_free(buffer);}
  camera.reset();
 }
 if(!Settings.world_e2e_token.empty())Trace("native_retire","phase=environment_detach");
 if(environment)EnvironmentDetach(*environment,room);
 if(!Settings.world_e2e_token.empty())Trace("native_retire","phase=room_destructor");
 room->destructor();
 if(!Settings.world_e2e_token.empty())Trace("native_retire","phase=room_root");
 ::operator delete(room);room=nullptr;grids.clear();
 entity_factory.Destroy();
 for(auto &entry:entity_players){entry.second->destructor();EntityFree(entry.second);}entity_players.clear();
 }catch(...){destroy_failed=true;throw;}
}
// Room/Grid/Entity共用同一份候选资源预算；不按域分别累加无限缓存。
struct SceneResourceBudget final {
 std::map<std::string,std::uint64_t> files,images;std::uint64_t file_bytes=0,image_bytes=0;unsigned nodes=0,sprites=0,layers=0;
};
thread_local SceneResourceBudget *resource_budget=nullptr;
struct ResourceWindow final {SceneResourceBudget budget;ResourceWindow(){if(resource_budget)Fail("native_resource_nested_window");resource_budget=&budget;}~ResourceWindow(){resource_budget=nullptr;}};
void ResourceImage(KAGE_Graphics_ImageBase *image){if(!image||!resource_budget)return;
 if(!image->_name)Fail("native_resource_image_name");const std::string name=image->_name;
 const auto width=image->GetPaddedWidth(),height=image->GetPaddedHeight();if(width<1||height<1||width>16384||height>16384)Fail("native_resource_image_size");
 const auto bytes=std::uint64_t(width)*std::uint64_t(height)*4;auto &b=*resource_budget;
 auto [it,inserted]=b.images.emplace(name,bytes);if(!inserted&&it->second!=bytes)Fail("native_resource_image_conflict");
 if(inserted){b.image_bytes+=bytes;if(b.images.size()>4096||b.image_bytes>256u<<20)Fail("native_resource_image_budget");}
}
// ANM2.Load即使文件读取失败也会置loaded=true；使用游戏文件管理器同时覆盖归档和挂载路径。
// 文件根由游戏创建并以游戏虚析构释放，不用DLL allocator删除；不读取或校验内容摘要。
void ResourceFile(const V &name){const auto path=Path(name);bool readable=false;
 for(const char *prefix:{"","resources/","resources-dlc1/","resources-dlc2/","resources-dlc3/"}){
  const auto file_path=std::string(prefix)+path;auto *file=g_FileManager.OpenRead(file_path.c_str());if(!file)continue;
  const auto size=file->GetSize();unsigned char first=0;readable=size>0&&size<=32*1024*1024&&file->Read(&first,1,1)==1;file->Free(true);
  if(readable&&resource_budget){auto &b=*resource_budget;auto [it,inserted]=b.files.emplace(path,size);if(inserted){b.file_bytes+=size;if(b.files.size()>4096||b.file_bytes>128u<<20)Fail("native_resource_file_budget");}}
  if(readable)break;
 }if(!readable)Fail("native_resource_unreadable");
}
// ANM2仍嵌在原生Grid／Backdrop中；从Authority姿态恢复，不创建第二个世界绘制器。
void Sprite(ANM2 &s,const V &pose){
 const auto &f=At(pose,"fields");const auto filename=Str(At(f,"Sprite.GetFilename"));
 if(!Flag(At(f,"Sprite.IsLoaded"))){s.Reset();return;}
 ResourceFile(f["Sprite.GetFilename"]);const auto path=Path(f["Sprite.GetFilename"]);if(s._filename!=path){std_string p=path;s.Load(p,true);}
 if(!s._loaded||s._layerCount>64)Fail("native_sprite_resource");
 const auto &layers=At(At(f,"Sprite.GetAllLayers"),"entries");
 if(!layers.IsArray()||layers.Size()!=s._layerCount)Fail("native_sprite_layer_membership");
 for(const auto &entry:layers.GetArray()){const auto &lf=At(At(entry,"value"),"fields");const auto id=int(Int(lf["LayerState.GetLayerID"]));
  unsigned ordinal=0;for(;ordinal<s._layerCount;++ordinal)if(s._layerState[ordinal]._layerData&&s._layerState[ordinal].GetLayerID()==id)break;
  if(ordinal==s._layerCount)Fail("native_sprite_layer");auto &l=s._layerState[ordinal];
  ResourceFile(lf["LayerState.GetSpritesheetPath"]);std_string sheet=Path(lf["LayerState.GetSpritesheetPath"]);if(l._spriteSheetPath!=sheet&&!s.ReplaceSpritesheet(int(ordinal),sheet))Fail("native_sprite_sheet");
 }
 s.LoadGraphics(true);
 for(unsigned i=0;i<s._layerCount;++i)ResourceImage(s._layerState[i]._spriteSheet.image);
 const auto animation=Str(f["Sprite.GetAnimation"]),overlay=Str(f["Sprite.GetOverlayAnimation"]);
 if(!animation.empty()){const auto a=s.GetAnimationByName(animation);if(!a)Fail("native_sprite_animation");std_string n=animation;s.SetFrame(&n,int(Int(f["Sprite.GetFrame"])));}
 if(!overlay.empty()){if(!s.GetAnimationByName(overlay))Fail("native_sprite_overlay");s.SetOverlayFrame(overlay.c_str(),int(Int(f["Sprite.GetOverlayFrame"])));}
 else{s._overlayAnimState._animData=nullptr;s._overlayAnimState._animFrame=-1;s._overlayAnimState._isPlaying=false;}
 s._offset=Vec(f["Sprite.Offset"]);s._scale=Vec(f["Sprite.Scale"]);s._rotation=Num(f["Sprite.Rotation"]);s._color=Color(f["Sprite.Color"]);
 s._flipX=Flag(f["Sprite.FlipX"]);s._flipY=Flag(f["Sprite.FlipY"]);s._playbackSpeed=Num(f["Sprite.PlaybackSpeed"]);s._bitflags=unsigned(Int(f["Sprite.GetRenderFlags"]));
 // 自主动画推进关闭，Authority下一目标负责事件与阶段；native Render的缓存维护仍保留。
 s._animState._isPlaying=s._overlayAnimState._isPlaying=false;s._animState._currentlyTriggeredEvents=s._animState._previouslyTriggeredEvents=0;
 s._overlayAnimState._currentlyTriggeredEvents=s._overlayAnimState._previouslyTriggeredEvents=0;
 if(pose.HasMember("native_pose")){
  const auto &p=pose["native_pose"];if(Int(At(p,"version"))!=1)Fail("native_sprite_pose_version");
  auto clock=[&](AnimationState &state,const V &v){
   state._animFrame=Num(At(v,"frame"));state._isPlaying=Flag(At(v,"playing"));
   // 丢弃SetFrame合成事件后恢复Authority当前事件后态；Client的AI/Update及gameplay dispatcher保持关闭。
   state._currentlyTriggeredEvents=int(Int(At(v,"events_current")));state._previouslyTriggeredEvents=int(Int(At(v,"events_previous")));
   const auto &layers=At(v,"layers"),&nulls=At(v,"nulls");
   if(!layers.IsArray()||!nulls.IsArray()||layers.Size()!=unsigned(state._animData?state._animData->_layerCount:0)||nulls.Size()!=unsigned(state._animData?state._animData->_nullLayerCount:0))Fail("native_sprite_pose_shape");
   if(layers.Size()&&!state._layerFrames)Fail("native_sprite_pose_storage");if(nulls.Size()&&!state._nullLayerFrames)Fail("native_sprite_pose_storage");
   std::set<int> seen;
   for(const auto &layer:layers.GetArray()){const int id=int(Int(At(layer,"id")));const auto n=Int(At(layer,"frame"));
    const auto order=state._animData->GetLayerOrder(id);auto *data=state._animData->GetLayerByOrder(order);
    if(!data||!seen.insert(id).second||n<-1||n>=data->_numFrames)Fail("native_sprite_pose_frame");state._layerFrames[order]=int(n);}
   const int *records=nullptr;if(state._animData)std::memcpy(&records,state._animData->_nullLayersPtr,sizeof(records));seen.clear();
   if(nulls.Size()&&!records)Fail("native_sprite_pose_null_records");
   for(const auto &layer:nulls.GetArray()){const int id=int(Int(At(layer,"id")));const auto n=Int(At(layer,"frame"));unsigned order=0;
    for(;order<nulls.Size();++order)if(records[order*4]==id)break;
    if(order==nulls.Size()||!seen.insert(id).second||n<-1||n>=records[order*4+3])Fail("native_sprite_pose_null_frame");state._nullLayerFrames[order]=int(n);}
  };clock(s._animState,At(p,"main"));clock(s._overlayAnimState,At(p,"overlay"));
 }
 for(const auto &entry:layers.GetArray()){const auto &lf=entry["value"]["fields"];const int id=int(Int(lf["LayerState.GetLayerID"]));LayerState *l=nullptr;
  for(unsigned i=0;i<s._layerCount;++i)if(s._layerState[i]._layerData&&s._layerState[i].GetLayerID()==id)l=&s._layerState[i];if(!l)Fail("native_sprite_layer");
  l->_size=Vec(lf["LayerState.GetSize"]);l->_pos=Vec(lf["LayerState.GetPos"]);l->_rotation=Num(lf["LayerState.GetRotation"]);l->_color=Color(lf["LayerState.GetColor"]);
  l->_cropOffset=Vec(lf["LayerState.GetCropOffset"]);l->_flipX=Flag(lf["LayerState.GetFlipX"]);l->_flipY=Flag(lf["LayerState.GetFlipY"]);l->_visible=Flag(lf["LayerState.IsVisible"]);
  l->_bitflags=unsigned(Int(lf["LayerState.GetRenderFlags"]));l->_wrapSMode=unsigned(Int(lf["LayerState.GetWrapSMode"]));l->_wrapTMode=unsigned(Int(lf["LayerState.GetWrapTMode"]));
  const auto &bf=At(lf["LayerState.GetBlendMode"],"fields");l->_blendMode._equation=unsigned(Int(bf["BlendMode.Equation"]));
  l->_blendMode._rgbSourceFactor=unsigned(Int(bf["BlendMode.RGBSourceFactor"]));l->_blendMode._rgbDestFactor=unsigned(Int(bf["BlendMode.RGBDestinationFactor"]));
  l->_blendMode._alphaSourceFactor=unsigned(Int(bf["BlendMode.AlphaSourceFactor"]));l->_blendMode._alphaDestFactor=unsigned(Int(bf["BlendMode.AlphaDestinationFactor"]));
 }
}
#include "native_room_environment.inc"
#include "native_room_motion.inc"
#include "native_room_color.inc"
// 在改动Room载体之前解析全部目标资源；失效资源不会使旧的完整世界停显或被部分覆盖。
void PreflightResources(const V &v){
 if(resource_budget&&++resource_budget->nodes>1048576)Fail("native_scene_node_budget");
 if(v.IsArray()){for(const auto &e:v.GetArray())PreflightResources(e);return;}if(!v.IsObject())return;
 // 正式DTO中的Sprite已经由15规范化为resource_pose，两种来源均需预检真实资源。
 if(v.HasMember("type")&&(v["type"]=="Sprite"||v["type"]=="resource_pose")&&v.HasMember("fields")){
  if(resource_budget){auto &b=*resource_budget;if(++b.sprites>4096)Fail("native_scene_sprite_budget");
   const auto &layers=At(At(At(v,"fields"),"Sprite.GetAllLayers"),"entries");if(layers.IsArray())b.layers+=layers.Size();if(b.layers>32768)Fail("native_scene_layer_budget");}
  auto *sprite=static_cast<ANM2 *>(::operator new(sizeof(ANM2)));try{sprite->constructor();}catch(...){::operator delete(sprite);throw;}
  try{Sprite(*sprite,v);}catch(...){sprite->destructor();::operator delete(sprite);throw;}sprite->destructor();::operator delete(sprite);return;}
 for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){if(i->name=="image"&&i->value.IsString()&&i->value.GetStringLength()){KAGE_SmartPointer_ImageBase image;FxImage(image,i->value);}else PreflightResources(i->value);}
}
GridEntity **Slots(Room *r){return reinterpret_cast<GridEntity **>(reinterpret_cast<unsigned char *>(r)+0x24);}
GridEntityDesc Descriptor(const V &object){const auto &f=At(At(At(object,"common"),"fields"),"GridEntity.GetSaveState")["fields"];GridEntityDesc d{};
 d._type=int(Int(f["GridEntityDesc.Type"]));d._variant=int(Int(f["GridEntityDesc.Variant"]));d._state=int(Int(f["GridEntityDesc.State"]));d._spawnCount=int(Int(f["GridEntityDesc.SpawnCount"]));
 d._spawnSeed=unsigned(Int(f["GridEntityDesc.SpawnSeed"]));d._varData=int(Int(f["GridEntityDesc.VarData"]));d._initialized=Flag(f["GridEntityDesc.Initialized"]);d._variableSeed=unsigned(Int(f["GridEntityDesc.VariableSeed"]));return d;
}
std::uint64_t ObjectId(const V &object){return std::stoull(Str(object["subject"]["target"]["id"]));}
const V &Object(const J &j,std::uint64_t id){for(const auto &o:GridValues(j).GetArray())if(ObjectId(o)==id)return o;Fail("native_subject_missing");}
void ApplyGrid(Scene &s,const V &o,bool create){const auto id=ObjectId(o);const auto index=unsigned(Int(o["index"]));auto desc=Descriptor(o);
 if(create){s.room->_gridPaths[index]=0;if(!s.room->SpawnGridEntityDesc(int(index),&desc))Fail("native_grid_create");auto *g=Slots(s.room)[index];if(!g||g->GetType()!=desc._type)Fail("native_grid_factory_type");s.grids.emplace(id,g);InjectFailure("create");}
 const auto it=s.grids.find(id);if(it==s.grids.end()||Slots(s.room)[index]!=it->second)Fail("native_grid_binding");auto *g=it->second;
 g->_desc=desc;g->_collisionClass=int(Int(o["common"]["fields"]["GridEntity.CollisionClass"]));
 g->_spawnFrame=unsigned(Int(RoomValue(s.target)["native_context"]["grid_birth"][index]));
 Lua("Grid",g,lua::Metatables::GRID_ENTITY,&o);g->_desc=desc;
 // SetLadder等公开恢复入口会维护碰撞；这些派生维护之后仍以Authority当前碰撞后态为准。
 g->_collisionClass=int(Int(o["common"]["fields"]["GridEntity.CollisionClass"]));InjectFailure("state");
}
void PoseGrid(Scene &s,const V &o){auto *g=s.grids.at(ObjectId(o));Sprite(g->_sprite,o["common"]["fields"]["GridEntity.GetSprite"]);
 const auto cls=Str(o["class"]);const auto &f=o["bodies"][cls.c_str()]["fields"];
 if(g->GetType()==16)Sprite(static_cast<GridEntity_Door *>(g)->_extraSprite,f["GridEntityDoor.ExtraSprite"]);
 if(g->GetType()==20){const auto &timer=f["GridEntityPressurePlate.TimerPlate"];if(timer.IsObject()&&timer.HasMember("fields"))Sprite(*reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(g)+0x154),timer);}
}
void Remove(Scene &s,std::uint64_t id){const auto it=s.grids.find(id);if(it==s.grids.end())return;auto *g=it->second;const unsigned index=g->_gridIndex;
 if(index>=448||Slots(s.room)[index]!=g)Fail("native_remove_binding");for(auto &door:s.room->_doors)if(door==g)door=nullptr;
 if(g->_desc._type==21)reinterpret_cast<EntityPtr *>(reinterpret_cast<char *>(g)+0x154)->SetReference(nullptr);
 s.room->RemoveGridEntityImmediate(int(index),0,false);if(Slots(s.room)[index])Fail("native_remove_occupied");s.grids.erase(it);
}
#include "native_entity_state.inc"
void Descriptions(Scene &s,const V &context){std::map<std::pair<int,int>,std::unique_ptr<Description>> descriptions;
 for(const auto &v:At(context,"descriptors").GetArray()){auto d=std::make_unique<Description>();d->Set(v);const auto key=std::make_pair(int(Int(v["lookup_index"])),int(Int(v["lookup_dimension"])));
  if(!descriptions.emplace(key,std::move(d)).second)Fail("native_descriptor_duplicate");}
 const auto key=std::make_pair(int(Int(context["current_room"])),int(Int(context["dimension"])));const auto it=descriptions.find(key);if(it==descriptions.end())Fail("native_current_descriptor");auto *current=it->second.get();
 const auto &r=RoomValue(s.target);const auto flags=current->data.Flags,quest=current->data.QuestDoors;
 current->data=Config(r["config"]);current->data.Flags=flags;current->data.QuestDoors=quest;current->value.Data=&current->data;
 const auto &o=r["override_config"];if(o.HasMember("type")&&o["type"]=="resource"){current->override_data=Config(o);current->value.OverrideData=&current->override_data;}
 // 先改成新owner再退休旧描述；任何失败均不会在Room内留下指向已析构描述的地址。
 s.current=current;if(s.room)s.room->_descriptor=&current->value;s.descriptions.swap(descriptions);
}
void Structure(Scene &s){const auto &r=RoomValue(s.target),&f=r["state"]["fields"];auto *room=s.room;
 room->_descriptor=&s.current->value;room->_roomType=unsigned(Int(f["Room.GetType"]));room->_gridWidth=unsigned(Int(f["Room.GetGridWidth"]));room->_gridHeight=unsigned(Int(f["Room.GetGridHeight"]));
 if(!room->_gridWidth||!room->_gridHeight||room->_gridWidth*room->_gridHeight>448)Fail("native_room_dimensions");
 // J273 Room边界+0x14..+0x20来自原生GetTopLeft/GetBottomRight所读的四个float。
 const Vector top=Vec(f["Room.GetTopLeftPos"]),bottom=Vec(f["Room.GetBottomRightPos"]);std::memcpy(reinterpret_cast<char *>(room)+0x14,&top,sizeof(top));std::memcpy(reinterpret_cast<char *>(room)+0x1c,&bottom,sizeof(bottom));
 const auto &positions=f["Room.GetDoorSlotPosition"]["values"];if(positions.Size()!=8)Fail("native_door_positions");
 for(unsigned i=0;i<8;++i){const auto p=Vec(positions[i]["value"]);const auto x=int(std::round((p.x-40)/40)),y=int(std::round((p.y-120)/40));room->_doorGridPositions[i]=unsigned(x+y*int(room->_gridWidth));}
 const auto frame=int(Int(r["native_context"]["game_frame"])),age=int(Int(f["Room.GetFrameCount"]));const auto born=frame-age;std::memcpy(reinterpret_cast<char *>(room)+0x11f0,&born,sizeof(born));
 room->_renderScrollOffset=Vec(f["Room.GetRenderScrollOffset"]);
}
// 捕获原生水色插值器的当前目标与时钟；正式来源和E2E读回都读各自实际Room。
V WaterLerpCapture(Room *room,A &a){V water(rapidjson::kObjectType);
 auto color=[&](const KColor &c){V v(rapidjson::kObjectType);Put(v,"R",V(c._red),a);Put(v,"G",V(c._green),a);Put(v,"B",V(c._blue),a);Put(v,"A",V(c._alpha),a);return v;};
 Put(water,"target_color",color(room->_waterLerpTargetColor),a);Put(water,"target_multiplier",color(room->_waterLerpTargetColorMult),a);
 Put(water,"duration",V(room->_waterLerpColorDuration),a);Put(water,"countdown",V(room->_waterLerpColorCountdown),a);return water;
}
void Layout(Scene &s){const auto &f=RoomValue(s.target)["state"]["fields"];
 const auto &paths=f["Room.GetGridPath"]["values"],&rails=f["Room.GetRail"]["values"];
 for(unsigned i=0;i<448;++i){s.room->_gridPaths[i]=int(Int(paths[i]["value"]));const auto &v=rails[i]["value"];reinterpret_cast<unsigned char *>(&s.room->_railManager)[4+i]=v.IsObject()&&v.HasMember("status")&&v["status"]=="Null"?255:static_cast<unsigned char>(Int(v));}
 for(auto &door:s.room->_doors)door=nullptr;
 for(const auto &o:GridValues(s.target).GetArray())for(const auto &m:o["memberships"].GetArray())if(Int(m["source"])==12)s.room->_doors[unsigned(Int(m["index"]))]=s.grids.at(ObjectId(o));
}
void Check(Scene &s){if(s.grids.size()!=GridValues(s.target).Size())Fail("native_actual_membership");std::set<GridEntity *> actual;
 // 受管成员必须来自本方Factory；最小Player不加入Room的玩法/绘制列表。
 CheckEntityLists(s,true);
 for(unsigned i=0;i<448;++i)if(auto *g=Slots(s.room)[i]){if(g->_gridIndex!=i||!actual.insert(g).second)Fail("native_actual_slot");}
 if(actual.size()!=s.grids.size())Fail("native_actual_membership");
 for(const auto &o:GridValues(s.target).GetArray()){auto *g=s.grids.at(ObjectId(o));const auto d=Descriptor(o);
  if(!actual.count(g)||g->_desc._type!=d._type||g->_desc._variant!=d._variant||g->_desc._state!=d._state||g->_desc._spawnCount!=d._spawnCount||
     g->_desc._spawnSeed!=d._spawnSeed||g->_desc._varData!=d._varData||g->_desc._variableSeed!=d._variableSeed||g->_desc._initialized!=d._initialized||
     g->_collisionClass!=Int(o["common"]["fields"]["GridEntity.CollisionClass"]))
   throw std::runtime_error("native_actual_grid_state:"+std::to_string(ObjectId(o))+":type="+std::to_string(g->_desc._type)+"/"+std::to_string(d._type)+":variant="+std::to_string(g->_desc._variant)+"/"+std::to_string(d._variant)+":state="+std::to_string(g->_desc._state)+"/"+std::to_string(d._state)+":collision="+std::to_string(g->_collisionClass)+"/"+std::to_string(Int(o["common"]["fields"]["GridEntity.CollisionClass"])));}
 const auto &r=RoomValue(s.target),&f=r["state"]["fields"];
 for(unsigned i=0;i<448;++i){const auto &rail=f["Room.GetRail"]["values"][i]["value"];const int expected=rail.IsObject()&&rail.HasMember("status")&&rail["status"]=="Null"?255:int(Int(rail));
  if(s.room->_gridPaths[i]!=Int(f["Room.GetGridPath"]["values"][i]["value"])||s.room->GetGridCollision(int(i))!=Int(f["Room.GetGridCollision"]["values"][i]["value"])||reinterpret_cast<unsigned char *>(&s.room->_railManager)[4+i]!=expected)Fail("native_actual_cell_state");
 }
 for(const auto &d:r["doors"].GetArray()){const auto slot=unsigned(Int(d["slot"]));const auto &ref=d["actual"];GridEntity *expected=nullptr;
  if(ref.HasMember("status")&&ref["status"]=="KnownCurrent")expected=s.grids.at(std::stoull(Str(ref["target"]["id"])));if(s.room->_doors[slot]!=expected)Fail("native_actual_door_alias");
 }
}
struct Service final {
 bool active=false,stopping=false,drawing=false,healthy=false,owns_draw=false,render_pending=false,applying=false;
 std::uint64_t connection=0,revision=0,published=0,rejected=0,draws=0,retired=0,duplicates=0,stale=0,creates=0,removes=0,changes=0,applies=0,resets=0,draw_apply=0,draw_reset=0;
 std::string pending,receipt,error,source_capture;std::unique_ptr<Scene> current;std::vector<std::unique_ptr<Scene>> retiring;
 // 一个完整候选最多等待120次绘制；显式依赖请求供传输owner取走，等待不撤销最后健康场景。
 std::string waiting,dependency_request;unsigned waiting_frames=0;
 unsigned source_capture_frames=0;
};
#include "native_room_decor.inc"
Service &State(){static auto *s=new Service;return *s;}
void Apply(const NativeRoomGridTransaction &tx){auto &service=State();
 if(tx.duplicate){++service.duplicates;return;}if(tx.stale){++service.stale;return;}
 if(tx.connection!=service.connection)Fail("native_connection_mismatch");
 // 原生Sprite／Grid会在原位换资源；只在上一帧原生队列排空且GPU完成后改动这些owner。
 if(!g_KAGE_Graphics_ImageManager._frameImages.empty()||!g_KAGE_Graphics_ImageManager._transparentBatches.empty())Fail("native_apply_inflight_graphics");glFinish();
 auto target=Parse(tx.model);const auto &room_value=RoomValue(target);At(room_value,"native_context");
 // 资源预检也属于Client准备窗口，不能通过Sprite加载回调进入玩法mod。
 Callbacks resource_callbacks;ResourceWindow resource_window;PreflightResources(target);
 // JSON文本的数值／转义规范化不代表原生写入；只用实际应用阶段标记判断是否可能部分修改。
 service.applying=true;
 if((!service.healthy||tx.reset)&&service.current){service.healthy=false;service.retiring.push_back(std::move(service.current));}
 if(!service.retiring.empty()){service.pending=tx.model;return;}
 if(!service.current){auto next=std::make_unique<Scene>();next->target=std::move(target);next->game=g_Game;if(!next->game)Fail("native_game_owner_unavailable");
  Descriptions(*next,room_value["native_context"]);next->render_random=unsigned(Int(room_value["native_context"]["render_random"]));
  next->room=static_cast<Room *>(::operator new(sizeof(Room)));std::memset(next->room,0,sizeof(Room));
  try{next->room->constructor();}catch(...){::operator delete(next->room);next->room=nullptr;throw;}service.current=std::move(next);
 }else{auto &s=*service.current;s.target=std::move(target);Descriptions(s,RoomValue(s.target)["native_context"]);}
 auto &s=*service.current;if(tx.has_entities)s.entity_factory.Allocate();Binding binding(s);Structure(s);
 const auto &r=RoomValue(s.target),&f=r["state"]["fields"];
 if(!Settings.world_e2e_token.empty())Trace("native_apply","phase=camera");
 if(!s.camera){s.camera.reset(new Camera(s.room));s.room->_Camera=s.camera.get();}
 s.room->_renderScrollOffset=Vec(f["Room.GetRenderScrollOffset"]);
 if(!Settings.world_e2e_token.empty())Trace("native_apply","phase=backdrop");
 if(s.room->_backdrop.backdropId!=unsigned(Int(f["Room.GetBackdropType"]))||service.receipt.empty()){
  // 先只设环境身份／边界。缓存生成要等完整Grid、门和房间状态成立后执行。
  s.room->_backdrop.Init(unsigned(Int(f["Room.GetBackdropType"])),false);
 }
 if(!Settings.world_e2e_token.empty())Trace("native_apply","phase=environment");
 if(!s.environment)s.environment=EnvironmentCreate();
 EnvironmentApply(*s.environment,s.room,r["native_context"]["environment"]);
 for(auto id:tx.remove){Remove(s,id);++service.removes;}for(auto id:tx.rebuild){Remove(s,id);}
 for(auto id:tx.create){ApplyGrid(s,Object(s.target,id),true);++service.creates;}
 for(auto id:tx.change){const bool rebuild=std::find(tx.rebuild.begin(),tx.rebuild.end(),id)!=tx.rebuild.end();ApplyGrid(s,Object(s.target,id),rebuild);++service.changes;}
 Layout(s);
 // PostInit在完整成员／门别名成立后执行；普通change不重跑Init或整Grid Update。
 // J273 Pit.PostInit的已初始化分支会检查／修改Player；新建载体仅走初始邻接维护分支。
 const auto initialized=*reinterpret_cast<unsigned char *>(s.room);*reinterpret_cast<unsigned char *>(s.room)=0;
 for(auto id:tx.create)Lua("PostInit",s.grids.at(id),lua::Metatables::GRID_ENTITY);
 for(auto id:tx.rebuild)Lua("PostInit",s.grids.at(id),lua::Metatables::GRID_ENTITY);
 *reinterpret_cast<unsigned char *>(s.room)=initialized;
 Lua("Room",s.room,lua::Metatables::ROOM,&r);
 // RGON水色Setter只写插值目标，Client不运行Room.Update，必须恢复Getter对应的当前值。
 auto kcolor=[](const V &v){return KColor(Num(v["R"]),Num(v["G"]),Num(v["B"]),Num(v["A"]));};
 s.room->_waterColor=kcolor(f["Room.GetWaterColor"]);s.room->_waterColorMultiplier=kcolor(f["Room.GetWaterColorMultiplier"]);
 // 816BB0在duration>=0时以目标／倒计时覆盖FX水色；Getter当前值不能代替这组插值后态。
 const auto &water=r["native_context"]["water_lerp"];
 s.room->_waterLerpTargetColor=kcolor(water["target_color"]);s.room->_waterLerpTargetColorMult=kcolor(water["target_multiplier"]);
 s.room->_waterLerpColorDuration=int(Int(water["duration"]));s.room->_waterLerpColorCountdown=int(Int(water["countdown"]));
 s.room->_redHeartDamage=Flag(f["Room.GetRedHeartDamage"]);
 // PostInit可能写邻接缓存和sprite；所有最终字段及姿态最后覆盖，以实际源状态为准。
 for(const auto &o:GridValues(s.target).GetArray()){ApplyGrid(s,o,false);PoseGrid(s,o);}Layout(s);
 // 原生墙／地板缓存依赖实际门、碰撞和最终Room状态，不能在空Grid载体上预生成。
 // J273原生Backdrop用[5,9,7]三个位移，对应公开SetSeed的shift index 35，不能把末位7当索引。
 s.room->_backdrop.seed=unsigned(Int(r["seeds"]["decoration"]));s.room->_backdrop.rng.SetSeed(s.room->_backdrop.seed,35);
 s.room->_backdrop.Init(unsigned(Int(f["Room.GetBackdropType"])),true);
 InjectFailure("refresh");
 if(const auto &b=f["Room.GetBackdrop"];b.IsObject()&&b.HasMember("fields")){
  const auto &bf=b["fields"];for(const auto &p:std::vector<std::pair<const char *,ANM2 *>>{{"Backdrop.GetFloorANM2",&s.room->_backdrop.floorANM2},{"Backdrop.GetDetailANM2",&s.room->_backdrop._wallDetailANM2},{"Backdrop.GetControlsANM2",&s.room->_backdrop._controlsANM2},{"Backdrop.GetControlsButtonsANM2",&s.room->_backdrop._controlsButtonsANM2}})
   if(bf.HasMember(p.first)&&bf[p.first].IsObject()&&bf[p.first].HasMember("fields"))Sprite(*p.second,bf[p.first]);
 }
 MotionApply(s.room,r["native_context"]["motion"]);
 ApplyNativeEntities(s,tx);
 *reinterpret_cast<unsigned char *>(s.room)=1;*reinterpret_cast<unsigned char *>(reinterpret_cast<char *>(s.room)+0x11f6)=1;
 Check(s);ColorTarget(s);s.receipt=tx;s.receipt.model.clear();service.receipt=tx.model;service.revision=tx.revision;service.healthy=true;service.error.clear();++service.published;
}
}
bool NativeRoomGridContextActive() noexcept{return IsMain()&&bound;}
bool NativeEntityFactoryAllowed(const void *factory,unsigned type,bool force) noexcept {
 return IsMain()&&ClientBusy()&&bound&&ConsumeFactoryTicket(factory,type,force);
}
bool NativeEntityRenderOrderFragment(const void *entity,unsigned *fragment) noexcept {
 if(!IsMain()||!ClientBusy()||!bound||!fragment)return false;const auto found=bound->entity_order_fragments.find(entity);
 if(found==bound->entity_order_fragments.end())return false;*fragment=found->second;return true;
}
void NativeBackdropWallsBegin(void *backdrop){DecorBegin(static_cast<Backdrop *>(backdrop));}
void NativeBackdropWallsEnd(void *backdrop,bool success){DecorEnd(static_cast<Backdrop *>(backdrop),success);}
void NativeBackdropEntityChoice(void *backdrop,int x,int y,unsigned layer,int kind,float px,float py,bool fx,bool fy,bool returned){DecorChoice(static_cast<Backdrop *>(backdrop),x,y,layer,kind,px,py,fx,fy,returned);}
bool NativeRoomGridReferenceOnly() noexcept{return IsMain()&&!ClientBusy()&&!Settings.world_e2e_token.empty()&&!State().source_capture.empty();}
int NativeRoomGridControlCharacter() noexcept{return IsMain()&&bound?RoomValue(bound->target)["native_context"]["controls_character"].GetInt():-1;}
// 原生表现初始化可使用本场景的私有随机流；不调用或恢复玩法全局RNG来掩盖推进。
std::uint32_t NativeRoomGridRandom() noexcept{return bound?bound->presentation_random():0;}
void NativeRoomGridSeed(std::uint32_t seed) noexcept{if(bound)bound->presentation_random.seed(seed);}
bool NativeRoomGridRandomObjectAllowed(const void *object) noexcept{
 if(!bound)return true;const auto address=reinterpret_cast<std::uintptr_t>(object),game=reinterpret_cast<std::uintptr_t>(bound->game);
 return address<game||address>=game+sizeof(Game);
}
void *NativeRoomGridCurrentDescriptor() noexcept{return bound&&bound->current?&bound->current->value:nullptr;}
void *NativeRoomGridDescriptor(int index,int dimension){if(!bound)return nullptr;if(dimension<0)dimension=int(bound->game->_currentDimensionIdx);
 const auto it=bound->descriptions.find({index,dimension});if(it==bound->descriptions.end())Fail("native_descriptor_dependency");return &it->second->value;
}
bool NativeRoomGridBegin(std::uint64_t connection) noexcept {auto &s=State();if(!IsMain()||s.active||s.current||!s.retiring.empty()||!connection)return false;
 s.active=true;s.stopping=s.healthy=s.owns_draw=false;s.connection=connection;s.revision=0;s.receipt.clear();s.pending.clear();s.waiting.clear();s.dependency_request.clear();s.waiting_frames=0;s.error.clear();return true;}
const char *NativeRoomGridSubmit(const std::string &text) noexcept {auto &s=State();if(!IsMain()||!ClientBusy()||!s.active||s.stopping)return "native_not_active";
 if(text.size()>32u<<20)return "native_byte_budget";if(!s.pending.empty())return s.pending==text?"":"native_candidate_busy";
 try{if(!s.waiting.empty()){if(s.waiting==text)return "";const auto before=Parse(s.waiting),next=Parse(text);
   const auto old_revision=std::stoull(Str(At(before,"revision"))),new_revision=std::stoull(Str(At(next,"revision")));
   if(At(before,"scope")!=At(next,"scope"))return "native_waiting_scope_conflict";
   if(new_revision==old_revision)return "native_entity_revision_conflict";if(new_revision<old_revision)return "native_waiting_stale";}
  s.pending=text;s.waiting.clear();s.waiting_frames=0;s.dependency_request.clear();s.owns_draw=true;return "";}catch(...){return "native_allocation";}}
void NativeRoomGridStop() noexcept {if(!IsMain())return;auto &s=State();s.stopping=true;s.pending.clear();s.waiting.clear();s.waiting_frames=0;s.dependency_request.clear();if(!s.current&&s.retiring.empty()){s.active=false;s.owns_draw=false;}}
bool NativeRoomGridDrained() noexcept {const auto &s=State();return !s.current&&s.retiring.empty()&&!s.drawing&&s.pending.empty()&&s.waiting.empty();}
bool NativeRoomGridOwnsDraw() noexcept {return State().active&&State().owns_draw;}
void NativeRoomGridDraw() noexcept {auto &s=State();if(!IsMain()||!s.active||s.drawing||!PresentationNativeGraphicsReady())return;
 s.drawing=true;s.render_pending=true;s.draw_apply=s.applies;s.draw_reset=s.resets;PresentationNativeDrawStarted();
 try{PresentationClearTarget();
  if(s.waiting_frames&&!--s.waiting_frames){s.waiting.clear();s.error="native_entity_dependency_timeout";}
  if(s.stopping&&s.current){s.retiring.push_back(std::move(s.current));s.healthy=false;}
  if(!s.pending.empty()&&!s.stopping){auto text=std::move(s.pending);s.pending.clear();
   s.applying=false;try{auto tx=PrepareNativeRoomGridTransaction(s.receipt,text,s.healthy);Apply(tx);}
   catch(const std::exception &e){++s.rejected;s.error=e.what();if(s.applying)s.healthy=false;
    else if(s.error.rfind("native_entity_waiting_",0)==0){s.waiting=std::move(text);s.waiting_frames=120;s.dependency_request=s.error;}}
   s.applying=false;}
  if(s.current&&s.healthy&&!s.stopping){Binding binding(*s.current);s.current->room->PreRender();RenderRoom(*s.current);Check(*s.current);++s.draws;}
  else if(g_Manager){const std::string message=s.stopping?"Retiring native room":s.error.empty()?"Waiting for native room":"Native room recovery: "+s.error;
   KColor color(1,1,1,1);FontSettings settings;g_Manager->_font8_LuaMiniOutlined.DrawString(message.c_str(),Vector(20,20),Vector(1,1),&color,&settings);}
  ClientUiHeartbeat();
 }catch(const std::exception &e){s.error=e.what();s.healthy=false;}catch(...){s.error="native_render_exception";s.healthy=false;}
 s.drawing=false;
}
void NativeRoomGridQueueReturned(bool reset) noexcept {if(!IsMain()||!ClientBusy())return;if(reset)++State().resets;else ++State().applies;}
void NativeRoomGridRenderReturned() noexcept {auto &s=State();if(!IsMain())return;
 if(!s.source_capture.empty()&&!ClientBusy()){
  // 双缓冲窗口Present后读取BACK可能仍是上一帧；保持域隔离至少两次完整render返回，
  // 避免把隔离前的Player灯光帧当作Room/Grid参考。
  if(++s.source_capture_frames<2)return;
  try{PresentationCaptureNativeReference(s.source_capture);s.source_capture.clear();}catch(const std::exception &e){s.error=e.what();}return;}
 if(!s.render_pending)return;s.render_pending=false;
 if(s.current&&s.current->environment&&!s.current->environment->retired.empty()&&g_KAGE_Graphics_ImageManager._frameImages.empty()&&g_KAGE_Graphics_ImageManager._transparentBatches.empty()){glFinish();s.current->environment->retired.clear();}
 if(s.retiring.empty())return;for(const auto &scene:s.retiring)if(scene->destroy_failed)return;
 if(s.applies<=s.draw_apply||s.resets<=s.draw_reset||!PresentationNativeGraphicsReady()||
   !g_KAGE_Graphics_ImageManager._frameImages.empty()||!g_KAGE_Graphics_ImageManager._transparentBatches.empty())return;
 try{glFinish();for(auto &scene:s.retiring)scene->Destroy();s.retired+=s.retiring.size();s.retiring.clear();if(s.stopping&&!s.current){s.active=false;s.owns_draw=false;s.receipt.clear();s.revision=0;}}
 catch(const std::exception &e){s.error=e.what();}
}
std::string NativeRoomGridFields(){const auto &s=State();std::ostringstream o;
 o<<"native.active="<<s.active<<"\nnative.connection="<<s.connection<<"\nnative.revision="<<s.revision<<"\nnative.healthy="<<s.healthy
  <<"\nnative.published="<<s.published<<"\nnative.rejected="<<s.rejected<<"\nnative.draws="<<s.draws<<"\nnative.retired="<<s.retired<<"\nnative.retiring="<<s.retiring.size()
  <<"\nnative.duplicates="<<s.duplicates<<"\nnative.stale="<<s.stale<<"\nnative.creates="<<s.creates<<"\nnative.removes="<<s.removes<<"\nnative.changes="<<s.changes
  <<"\nnative.grids="<<(s.current?s.current->grids.size():0)<<"\nnative.shape="<<(s.current?s.current->receipt.shape:0)<<"\nnative.external_dependencies="<<(s.current?s.current->receipt.external_dependencies.size():0)
  <<"\nnative.waiting="<<!s.waiting.empty()<<"\nnative.waiting_frames="<<s.waiting_frames<<"\nnative.dependency_request="<<s.dependency_request
  <<"\nnative.needs_recovery="<<(!s.healthy&&!s.receipt.empty())<<"\nnative.drained="<<NativeRoomGridDrained()<<"\nnative.error="<<s.error<<'\n';return o.str()+NativeRoomGridSourceFields();}
std::string NativeEntityReadback(){auto &service=State();if(!IsMain()||!ClientBusy()||!service.current||!service.healthy)Fail("native_entity_readback_context");
 auto &scene=*service.current;Binding binding(scene);EntityLuaValues values(scene);CheckEntityLists(scene,true);
 for(const auto &entry:scene.entities){const auto &object=EntityObject(scene,entry.first);values.Verify(scene,entry.second,object);CheckNativeText(entry.second,object);}
 std::ostringstream out;out<<"native.entities="<<scene.entities.size()<<"\nnative.player_carriers="<<scene.entity_players.size()<<"\nnative.entity_vector_bytes="<<scene.entity_vector_bytes<<"\nnative.entity_readback=equal\n";return out.str();
}
// 当前原生源上下文只序列化具名字段；不传对象内存、vptr、容器地址或图像像素。
std::string CaptureNativeRoomContext(){if(!IsMain()||!g_Game||!g_Game->_room||!g_Game->_room->_descriptor)Fail("native_source_context");J out(rapidjson::kObjectType);auto &a=out.GetAllocator();auto *g=g_Game;
 const std::pair<const char *,std::int64_t> header[]={{"stage",g->_stage},{"stage_type",g->_stageType},{"curses",g->_curses},{"current_room",int(g->_currentRoomIdx)},
  {"previous_room",int(g->_lastRoomIdx)},{"starting_room",int(g->_startingRoomIdx)},{"controls_character",*reinterpret_cast<int *>(reinterpret_cast<char *>(g->GetPlayer(0))+0x1368)},
  {"dimension",g->_currentDimensionIdx},{"enter_door",int(g->_enterDoor)},{"leave_door",int(g->_leaveDoor)},
  {"difficulty",g->_difficulty},{"challenge",g->_challenge},{"game_frame",g->_frameCount},{"level_flags",g->_levelStateFlags}};
 for(const auto &h:header)Put(out,h.first,V(h.second),a);Put(out,"game_flags",S(std::to_string(g->_gameStateFlags),a),a);
 // 原生光照插值由PreRender消费，颜色最终值由Game.Render消费；只复制纯标量，不运行Game.Update。
 float lighting[2]{};std::memcpy(lighting,reinterpret_cast<char *>(g)+0x25a90,sizeof(lighting));
 Put(out,"lighting_blend",V(lighting[0]),a);Put(out,"lighting_intensity",V(lighting[1]),a);
 V color(rapidjson::kArrayType);const auto &m=g->_currentColorModifier;
 for(float n:{m.r,m.g,m.b,m.a,m.brightness,m.contrast})color.PushBack(V(n),a);Put(out,"color_modifier",std::move(color),a);
 Put(out,"water_lerp",WaterLerpCapture(g->_room,a),a);
 Put(out,"wall_decor",DecorCapture(&g->_room->_backdrop,a),a);
 std::uint32_t render_random=0;std::memcpy(&render_random,reinterpret_cast<void *>(Base+0x7aa104),4);Put(out,"render_random",V(render_random),a);V descriptors(rapidjson::kArrayType);
 std::map<std::pair<int,int>,RoomDescriptor *> values;const int dim=int(g->_currentDimensionIdx),room=int(g->_currentRoomIdx);values[{room,dim}]=g->_room->_descriptor;
 for(auto *grid:g->_room->_doors)if(grid){const int index=static_cast<GridEntity_Door *>(grid)->_targetRoomIdx;if(index==-19||index<-101||index>168)continue;
  if(index<-20&&index!=-100&&index!=-101)continue;for(int d=0;d<3;++d){auto *desc=g->GetRoomByIdx(index,d);if(desc&&desc->Data)values[{index,d}]=desc;}}
 for(const auto &entry:values){const auto *d=entry.second;V v(rapidjson::kObjectType);
  const std::pair<const char *,std::int64_t> fields[]={{"lookup_index",entry.first.first},{"lookup_dimension",entry.first.second},{"grid_index",d->GridIndex},{"safe_grid_index",d->SafeGridIndex},
   {"list_index",d->ListIndex},{"dimension",d->Dimension},{"allowed_doors",d->AllowedDoors},{"display_flags",d->DisplayFlags},{"visited_count",d->VisitedCount},{"flags",d->Flags},
   {"decoration_seed",unsigned(d->DecorationSeed)},{"spawn_seed",unsigned(d->SpawnSeed)},{"award_seed",unsigned(d->AwardSeed)},{"boss_death_seed",d->BossDeathSeed},
   {"clear_count",d->ClearCount},{"pits_count",d->PitsCount},{"group",d->Group},{"greed_subtype",d->GreedSubType},{"delirium_distance",d->DeliriumDistance}};
  for(const auto &f:fields)Put(v,f.first,V(f.second),a);V doors(rapidjson::kArrayType);for(int n:d->Doors)doors.PushBack(V(n),a);Put(v,"doors",std::move(doors),a);V data(rapidjson::kObjectType);const auto *c=d->Data;
  const std::pair<const char *,std::int64_t> cf[]={{"stage",c->StageId},{"mode",c->Mode},{"type",c->Type},{"variant",c->Variant},{"subtype",c->Subtype},{"shape",c->Shape},{"width",c->Width},{"height",c->Height},{"flags",c->Flags.to_ulong()},{"quest_doors",c->QuestDoors.to_ulong()}};
  for(const auto &f:cf)Put(data,f.first,V(f.second),a);Put(v,"data",std::move(data),a);descriptors.PushBack(std::move(v),a);
 }Put(out,"descriptors",std::move(descriptors),a);Put(out,"environment",EnvironmentCapture(g->_room,a),a);Put(out,"motion",MotionCapture(g->_room,a),a);
 V inside(rapidjson::kArrayType),birth(rapidjson::kArrayType);for(unsigned i=0;i<448;++i){inside.PushBack(V(g->_room->IsValidGridIndex(int(i),true)),a);auto *grid=Slots(g->_room)[i];birth.PushBack(grid?V(grid->_spawnFrame):V(),a);}
 Put(out,"grid_inside",std::move(inside),a);Put(out,"grid_birth",std::move(birth),a);return Text(out);
}
// 原生读回可能包含完整FX树；大结果写本次E2E临时目录，不扩大通用控制口的64KiB固定响应。
std::string NativeTestObjectResult(const std::filesystem::path &dir,const std::string &name,const std::string &value){
 if(value.size()>32u<<20)Fail("native_readback_budget");if(value.size()<48000)return "ok=1\nnative.object="+value+"\n";
 const auto file="readback-"+name+".json";std::ofstream output(dir/file,std::ios::binary);output<<value;if(!output)Fail("native_readback_file");return "ok=1\nnative.object_file="+file+"\n";
}
std::string NativeRoomGridTest(const std::string &token,const std::string &action){
 if(!IsMain()||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token)return "ok=0\nerror=native_test_disabled\n";
 try{
  const auto dir=std::filesystem::u8path(Settings.world_output_dir)/"native";std::filesystem::create_directories(dir);
  auto name=[&](std::size_t offset){const auto n=action.substr(offset);if(n.empty()||n.size()>64||n.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")!=n.npos)Fail("native_test_name");return n;};
  if(action=="entity-readback")return "ok=1\n"+NativeEntityReadback();
  if(action.rfind("scene-source-",0)==0){const auto n=name(13);
   // 与18来源相同，等待请求后的完整普通窗口，不能用夹具操作前的缓存状态冒充当前来源。
   static std::map<std::string,int> after;const auto barrier=after.emplace(n,g_Game?g_Game->_frameCount:0).first->second;
   const auto text=CaptureNativeSceneSource();const auto model=Parse(text);if(Int(RoomValue(model)["native_context"]["game_frame"])<barrier)Fail("native_source_incomplete_window");
   std::ofstream f(dir/(n+".json"),std::ios::binary);f<<text;if(!f)Fail("native_source_write");return "ok=1\nnative.scene_source_saved=1\n";}
  if(action.rfind("source-",0)==0){const auto n=name(7);
   // 首次请求冻结最早采集帧，重试等待正式consumer产出新普通窗口，不能借用动作前的周期快照。
   static std::map<std::string,int> after;const auto barrier=after.emplace(n,g_Game?g_Game->_frameCount:0).first->second;
   const auto text=CaptureNativeRoomGridSource();const auto model=Parse(text);
   if(Int(RoomValue(model)["native_context"]["game_frame"])<barrier)Fail("native_source_incomplete_window");
   std::ofstream f(dir/(n+".json"),std::ios::binary);f<<text;if(!f)Fail("native_source_write");return "ok=1\nnative.source_saved=1\n";}
  if(action.rfind("reference-",0)==0){const auto n=name(10);if(ClientBusy()||!State().source_capture.empty())Fail("native_reference_busy");State().source_capture_frames=0;State().source_capture="source-"+n;return "ok=1\n";}
  if(action.rfind("action-",0)==0){const auto n=name(7);auto *L=g_LuaEngine->_state;const int top=lua_gettop(L);lua_getglobal(L,"IsaacCS_OriginalAction");lua_pushlstring(L,n.data(),n.size());
   if(!lua_isfunction(L,-2)||lua_pcall(L,1,1,0)!=LUA_OK){const auto *e=lua_tostring(L,-1);const auto error=e?std::string(e):"native_fixture";lua_settop(L,top);throw std::runtime_error(error);}std::size_t bytes=0;const auto *p=lua_tolstring(L,-1,&bytes);const std::string result=p?std::string(p,bytes):"null";lua_settop(L,top);return "ok=1\nnative.fixture="+result+"\n";}
  if(action.rfind("submit-",0)==0){const auto n=name(7);std::ifstream file(dir/(n+".json"),std::ios::binary);if(!file)Fail("native_target_file");std::string text(32u*1024u*1024u+1,'\0');file.read(text.data(),std::streamsize(text.size()));text.resize(std::size_t(file.gcount()));const auto *e=NativeRoomGridSubmit(text);return *e?"ok=0\nerror="+std::string(e)+"\n":"ok=1\n";}
  if(action.rfind("reconstruct-",0)==0){const auto n=name(12);std::ifstream file(dir/(n+".json"),std::ios::binary);if(!file)Fail("native_target_file");std::string text(64u*1024u*1024u+1,'\0');file.read(text.data(),std::streamsize(text.size()));text.resize(std::size_t(file.gcount()));if(text.size()>64u<<20)Fail("native_test_reconstruct_budget");J input;input.Parse(text.data(),text.size());if(input.HasParseError())Fail("native_test_reconstruct_json");
   const auto target=ReconstructNativeRoomGridTarget(Text(At(input,"baseline")),Text(At(input,"target")));const auto *e=NativeRoomGridSubmit(target);return *e?"ok=0\nerror="+std::string(e)+"\n":"ok=1\n";}
  if(action=="remove-room"){NativeRoomGridStop();return "ok=1\n";}
  if(action=="fail-apply"){State().healthy=false;return "ok=1\n";}
  if(action=="fail-next-create"||action=="fail-next-state"||action=="fail-next-refresh"){injected_failure=action.substr(10);return "ok=1\n";}
  if(action.rfind("object-",0)==0){const auto &s=State();if(!s.current||!s.healthy)return "ok=1\nnative.object=null\n";auto &scene=*s.current;Binding binding(scene);const auto idtext=action.substr(7);
   if(idtext=="environment"){J out;out.SetObject();auto value=EnvironmentCapture(scene.room,out.GetAllocator());return NativeTestObjectResult(dir,idtext,Text(value));}
   if(idtext=="water-lerp"){J out;out.SetObject();return NativeTestObjectResult(dir,idtext,Text(WaterLerpCapture(scene.room,out.GetAllocator())));}
   if(idtext=="motion"){J out;out.SetObject();auto value=MotionCapture(scene.room,out.GetAllocator());return NativeTestObjectResult(dir,idtext,Text(value));}
   if(idtext=="room-state")return NativeTestObjectResult(dir,idtext,Lua("ReadRoom",scene.room,lua::Metatables::ROOM,nullptr,true));
   if(idtext=="geometry")return NativeTestObjectResult(dir,idtext,Lua("ReadGeometry",scene.room,lua::Metatables::ROOM,nullptr,true));
   if(idtext=="clocks"){J out;out.SetArray();for(unsigned i=0;i<448;++i){auto *grid=Slots(scene.room)[i];out.PushBack(grid?V(grid->_spawnFrame):V(),out.GetAllocator());}return NativeTestObjectResult(dir,idtext,Text(out));}
   if(idtext=="room"){J result(rapidjson::kObjectType);auto &a=result.GetAllocator();Put(result,"shape",V(scene.room->_descriptor->Data->Shape),a);Put(result,"width",V(scene.room->_gridWidth),a);Put(result,"height",V(scene.room->_gridHeight),a);
    Put(result,"backdrop",V(scene.room->_backdrop.backdropId),a);Put(result,"water",V(scene.room->_waterAmount),a);V cells(rapidjson::kArrayType);for(unsigned i=0;i<448;++i){V c(rapidjson::kObjectType);Put(c,"index",V(i),a);Put(c,"collision",V(scene.room->GetGridCollision(int(i))),a);Put(c,"path",V(scene.room->_gridPaths[i]),a);Put(c,"rail",V(unsigned(reinterpret_cast<unsigned char *>(&scene.room->_railManager)[4+i])),a);Put(c,"occupied",V(Slots(scene.room)[i]!=nullptr),a);Put(c,"inside",V(scene.room->IsValidGridIndex(int(i),true)),a);cells.PushBack(std::move(c),a);}Put(result,"cells",std::move(cells),a);V doors(rapidjson::kArrayType);for(auto *d:scene.room->_doors){std::uint64_t id=0;for(const auto &pair:scene.grids)if(pair.second==d)id=pair.first;doors.PushBack(S(std::to_string(id),a),a);}Put(result,"doors",std::move(doors),a);return NativeTestObjectResult(dir,idtext,Text(result));}
   const auto id=std::stoull(idtext);const auto it=scene.grids.find(id);if(it==scene.grids.end())return "ok=1\nnative.object=null\n";
   const auto &object=Object(scene.target,id);J result=Parse(Lua("ReadGrid",it->second,lua::Metatables::GRID_ENTITY,&object["class"],true));return "ok=1\nnative.object="+Text(result)+"\n";
  }
  return "ok=0\nerror=native_test_action\n";
 }catch(const std::exception &e){return "ok=0\nerror="+std::string(e.what())+"\n";}
}
} // namespace cs::game
