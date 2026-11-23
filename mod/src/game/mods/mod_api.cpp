#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "lualib.h"
#include "binding/offsets.h"
#include "game/mods/mod_api.h"
#include "game/mods/mod_native.h"
#include "game/common/bridge.h"
#include "game/client/display/client_display_bridge.h"
#include "game/client/session/client.h"
#include "game/client/isolation/client_isolation.h"
#include "mod_lua_sources.h"
#include "runtime.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <limits>
#include <set>
#include <stdexcept>
#include <vector>
#include <chrono>

namespace cs::game {
bool Armed() noexcept;
// 定义在 game/authority/tick.cpp；本单元按 C++17 编译，不能包含 tick.h，与 Armed 一样单独声明。
bool SoloBusy() noexcept;
namespace {
// 前序节点：子树结束位置帮助解释器跳过整个容器或不存在的结构字段。
using NodeKind=state::ModNodeKind;
using Node=state::ModNode;
// 失败序号的空值：MarkDirty 的序号从 0 递增，永远到不了这个值。
constexpr std::uint64_t kNoMark=(std::numeric_limits<std::uint64_t>::max)();
struct Declaration {
  std::string kind, name, scope, rate, text;
  bool private_value=false;
  unsigned get=0, set=0, reset=0, handler=0, filter=0, key=0;
  std::array<unsigned,3> shape{UINT32_MAX,UINT32_MAX,UINT32_MAX};
  std::map<state::ModEntityId,std::uint64_t> entity_marks,entity_failed;
  std::shared_ptr<state::ModLayout> layout=std::make_shared<state::ModLayout>();
  // 4 是 global 槽；失败序号只消费对应的 MarkDirty，不影响后续标记。kNoMark 表示该槽尚无失败。
  std::array<std::uint64_t,5> dirty{},committed_mark{};
  std::array<std::uint64_t,5> failed_mark{kNoMark,kNoMark,kNoMark,kNoMark,kNoMark};
  bool inspected=false;
  std::uint64_t drift=0,diagnostic_errors=0,default_errors=0;
  std::uint64_t read_count=0,read_us=0,read_failures=0,read_missing=0,install_us=0,install_count=0,install_failures=0,install_missing=0;
};
struct Registration {
  const void *table=nullptr;
  unsigned ordinal=0,table_ref=0;
  std::vector<Declaration> declarations;
  std::uint64_t callback_failures=0,reset_failures=0,blocked=0,removed_unplanned=0;
  std::uint64_t batches=0,batch_max_us=0,batch_avg_us=0,baseline_count=0,baseline_us=0;
  std::uint64_t decode_rejected=0;
};
struct LuaRegistration {
  lua_State *vm=nullptr;
  bool ready=false, loaded=false, reload_pending=false;
  std::string reload_directory;
  std::set<const void *> internal;
  std::map<std::string,Registration> registered;
  std::set<const void *> supported;
  std::array<unsigned,3> constructors{};
  std::vector<int> keys;
  state::ModSchemaRef schema;
  net::ModManifest enabled_manifest, compiled_manifest;
  bool compiled=false;
  std::uint64_t mark_sequence=0;
};
LuaRegistration registration;
std::uint64_t capture_skipped=0,install_vm_mismatches=0;
thread_local unsigned replay_audio=0;
thread_local bool menu_input=false;
char layout_references, sdk_entries, id_reader;
int SendIntentLua(lua_State *);

// 发布包和官方加载器免声明，但在客机仍要满足显示窗口与回调分类。
bool InternalMod(const std::string &dir) {
  return dir=="isaac-cs-ui" || dir=="isaac-cs-observation" || dir=="repentogon";
}

std::string ReadString(lua_State *L,int index) {
  std::size_t length=0;
  const auto *chars=lua_type(L,index)==LUA_TSTRING?lua_tolstring(L,index,&length):nullptr;
  return chars?std::string(chars,length):std::string{};
}

std::string TextField(lua_State *L,int table,const char *field) {
  lua_getfield(L,table,field);
  auto value=ReadString(L,-1);
  lua_pop(L,1);
  return value;
}

unsigned NumberField(lua_State *L,int table,const char *field) {
  lua_getfield(L,table,field);
  const auto value=static_cast<unsigned>(lua_tointeger(L,-1));
  lua_pop(L,1);
  return value;
}

// 官方 ModReference 持有游戏登记的 main.lua 路径，用其父目录关联原生清单。
std::string ModDirectory(lua_State *L,int index) {
  if (!g_LuaEngine || g_LuaEngine->_state!=L || !lua_istable(L,index)) return {};
  const auto *reference=g_LuaEngine->GetModRefByTable(index);
  return reference?std::filesystem::u8path(reference->_mainLuaPath).parent_path().filename().u8string():std::string{};
}

int Log(lua_State *L) {
  const auto *message=lua_tostring(L,1);
  Trace("mod_api", "%s", message?message:"unknown");
  return 0;
}

int Loading(lua_State *L) {
  bool open=false;
  try {
    const auto directory=ModDirectory(L,1);
    const bool in_session=ClientBusy() || HostBusy();
    if (registration.vm==L && !directory.empty() && (!in_session || registration.reload_pending)) {
      open=!registration.loaded;
      if (!open && directory==registration.reload_directory) {
        lua_Debug frame{};
        for (int level=1;lua_getstack(L,level,&frame);++level) {
          if (lua_getinfo(L,"S",&frame)&&std::strcmp(frame.what,"main")==0&&frame.source&&
              std::filesystem::u8path(frame.source).parent_path().filename().u8string()==registration.reload_directory) {
            open=true;
            break;
          }
        }
      }
    }
  } catch (...) {}
  lua_pushboolean(L,open);
  return 1;
}

int ModsLoaded(lua_State *) {
  registration.loaded=true;
  registration.compiled=false;
  registration.reload_directory.clear();
  return 0;
}

int ModUnloaded(lua_State *L) {
  const auto *identity=lua_topointer(L,1);
  registration.supported.erase(identity);
  registration.internal.erase(identity);
  registration.compiled=false;
  try {
    registration.reload_directory=ModDirectory(L,1);
    if (ClientBusy() || HostBusy()) registration.reload_pending=true;
  } catch (...) {}
  return 0;
}

int Side(lua_State *L) {
  lua_pushstring(L,ClientBusy()?"client":HostBusy()?"host":"offline");
  return 1;
}

bool ParticipantOrdinal(const Entity_Player *player,unsigned &ordinal) noexcept {
  return ClientBusy()?ClientBinaryPlayerOrdinal(player,ordinal):HostPlayerParticipant(player,ordinal);
}

int Participant(lua_State *L) {
  if (lua_isnoneornil(L,1)) return 0;
  const auto *player=lua::GetLuabridgeUserdata<Entity_Player *>(L,1,lua::Metatables::ENTITY_PLAYER,"EntityPlayer");
  unsigned ordinal=0;
  if (!player || !ParticipantOrdinal(player,ordinal)) return 0;
  lua_pushinteger(L,ordinal);
  return 1;
}

Entity_Player *FindPlayer(unsigned target) noexcept {
  if(!g_Game||target>=4)return nullptr;
  // 按原生名册顺序取首个有效身体；副身体仍可正查参与者。
  for (auto *player:g_Game->_playerManager._playerList) {
    unsigned ordinal=0;
    if(player&&ParticipantOrdinal(player,ordinal)&&ordinal==target)return player;
  }
  return nullptr;
}
void PushPlayer(lua_State *L,Entity_Player *player) {
  if(player)lua::luabridge::UserdataPtr::push(L,player,lua::GetMetatableKey(lua::Metatables::ENTITY_PLAYER));
  else lua_pushnil(L);
}
int Player(lua_State *L) {
  if(!lua_isinteger(L,1))return 0;
  const auto target=lua_tointeger(L,1);
  PushPlayer(L,target>=0&&target<4?FindPlayer(unsigned(target)):nullptr);
  return 1;
}

int MarkDirty(lua_State *L) {
  if(!IsMain()||!HostBusy()||registration.vm!=L||lua_type(L,2)!=LUA_TSTRING)return 0;
  const auto *owner=lua_topointer(L,1);
  for(auto &[dir,mod]:registration.registered)if(mod.table==owner){
    std::size_t length=0;const auto *key=lua_tolstring(L,2,&length);
    for(auto &item:mod.declarations)if(item.rate=="mark"&&item.name==std::string_view(key,length)){
      if(item.layout->entity){
        if(lua_isnoneornil(L,3))item.dirty[4]=++registration.mark_sequence;
        else if(const auto entity=lua::TestUserdata<Entity *>(L,3,lua::Metatables::ENTITY)){
          state::ModEntityId id;
          if(HostModIdentity(*entity,id))try {item.entity_marks.insert_or_assign(id,++registration.mark_sequence);}
          catch(...) {Trace("mod_state","phase=mark_storage");}
        }
        lua_pushboolean(L,1);return 1;
      }
      if(item.kind!="State")continue;
      if(item.scope=="global")++item.dirty[4];
      else if(lua_isnoneornil(L,3)){
        for(unsigned i=0;i<4;++i)++item.dirty[i];
      }else{
        const auto *player=lua::GetLuabridgeUserdata<Entity_Player *>(L,3,lua::Metatables::ENTITY_PLAYER,
          lua::Metatables::CONST_ENTITY_PLAYER,"EntityPlayer");
        unsigned ordinal=0;
        if(player&&ParticipantOrdinal(player,ordinal)&&ordinal<4)++item.dirty[ordinal];
      }
      lua_pushboolean(L,1);return 1;
    }
    break;
  }
  return 0;
}

int LocalOrdinal(lua_State *L) {
  unsigned ordinal=0;
  if (ClientBusy() && !ClientLocalOrdinal(ordinal)) return 0;
  lua_pushinteger(L,ordinal);
  return 1;
}

int CallbackAllowed(lua_State *L) {
  if (!ClientBusy()) {lua_pushboolean(L,1);return 1;}
  if (!IsMain() || registration.vm!=L) {lua_pushboolean(L,0);return 1;}
  const int callback=ModApiCallbackId(L);
  const auto *mod=lua_topointer(L,2);
  const bool internal=registration.internal.count(mod)!=0;
  // 本项目 UI 的主菜单绘制由外层分派执行一次。
  const bool direct_menu=internal && callback==kModMainMenuRender;
  const bool allow=ModApiCallbackAllowedInPhase(callback) &&
                   (internal ? !direct_menu && ClientNativeDisplayCallbacks() : registration.supported.count(mod)!=0);
  if (!allow && !direct_menu) ClientCallbackSkipped();
  lua_pushboolean(L,allow);
  return 1;
}

/*
扩展按键查询，供 lua/isaaccs_api.lua 的 IsKeyPressed / IsKeyTriggered 调用（第三个参数为 true 时查边沿）。
返回 nil 让它回落到本机键盘；联机会话里未知按键和身体保持中性（false）。
solo 单人会话也返回 nil：solo 下 side() 为 "offline"，IsaacCS 其余函数都按不联机处理；
而 SampleModKeys 只在 Host 和客机采样扩展键，solo 的权威输入里扩展键位图恒为 0，
若走 ReadModKey 会恒为 false。
*/
int KeyLua(lua_State *L) {
  if(!ClientBusy()&&(!Armed()||SoloBusy()))return 0;
  auto *player=lua::GetLuabridgeUserdata<Entity_Player *>(L,1,lua::Metatables::ENTITY_PLAYER,
    lua::Metatables::CONST_ENTITY_PLAYER,"EntityPlayer");
  const auto key=luaL_checkinteger(L,2);
  const auto found=std::lower_bound(registration.keys.begin(),registration.keys.end(),key);
  unsigned ordinal=0;
  const bool valid=!ClientBusy()&&found!=registration.keys.end()&&*found==key&&ParticipantOrdinal(player,ordinal);
  lua_pushboolean(L,valid&&ReadModKey(ordinal,1ull<<(found-registration.keys.begin()),lua_toboolean(L,3)!=0));
  return 1;
}
int ActionLua(lua_State *L) {
  if(!lua_isinteger(L,1)||!lua_isinteger(L,2))return 0;
  const auto controller=lua_tointeger(L,2),action=lua_tointeger(L,1);
  const auto kind=static_cast<unsigned>(lua_tointeger(L,3));
  float value=0;
  if(controller<0||controller>INT_MAX||action<0||action>=12||
     !ReadModAction(int(controller),int(action),kind,value))return 0;
  if(kind==3)lua_pushnumber(L,value);
  else lua_pushboolean(L,value!=0);
  return 1;
}
int Install(lua_State *L) {
  lua_newtable(L);
  const int native=lua_absindex(L,-1);
  lua_pushboolean(L,1);lua_setfield(L,native,"_isaaccs_native");
  const luaL_Reg methods[]={{"log",Log},{"loading",Loading},{"side",Side},{"participant",Participant},
    {"player",Player},{"localOrdinal",LocalOrdinal},{"allowed",CallbackAllowed},{"markDirty",MarkDirty},
    {"modsLoaded",ModsLoaded},{"unloaded",ModUnloaded},{"sendIntent",SendIntentLua},{"key",KeyLua},{"action",ActionLua},{nullptr,nullptr}};
  luaL_setfuncs(L,methods,0);
  InstallNativeModLua(L);
  /*
  先执行联机层的 IsaacCS 实现（lua/isaaccs_api.lua），它发布全局 IsaacCS 并返回该表；
  此时还没有任何 Mod 的 main.lua 执行过，Mod 随附的 SDK 薄壳引入时会直接返回这个实现。
  再执行回调分派脚本（lua/isaaccs_dispatch.lua），它返回内容 ID 表的读取函数。
  两段源都在构建时编进 DLL（mod_lua_sources.h），代码块名只用于报错信息里的文件名。
  */
  if (luaL_loadbuffer(L,reinterpret_cast<const char *>(kModApiSource),sizeof(kModApiSource),"@isaaccs_api.lua")!=LUA_OK)
    return lua_error(L);
  lua_pushvalue(L,native);
  lua_call(L,1,1);
  const int api=lua_absindex(L,-1);
  lua_getfield(L,api,"_isaaccs");
  lua_getfield(L,-1,"state");
  lua_getfield(L,-1,"entries");
  lua_rawsetp(L,LUA_REGISTRYINDEX,&sdk_entries);
  lua_pop(L,2);
  if (luaL_loadbuffer(L,reinterpret_cast<const char *>(kModDispatchSource),sizeof(kModDispatchSource),"@isaaccs_dispatch.lua")!=LUA_OK)
    return lua_error(L);
  lua_pushvalue(L,native);lua_pushvalue(L,api);
  // 正式 Mod 虚拟机没有全局 debug。仅把临时调试库表传给安装闭包，不登记到全局或 package.loaded。
  lua_pushcfunction(L,luaopen_debug);
  lua_call(L,0,1);
  lua_call(L,3,1);
  lua_rawsetp(L,LUA_REGISTRYINDEX,&id_reader);
  lua_getglobal(L,"REPENTOGON");
  if (lua_istable(L,-1)) registration.internal.insert(lua_topointer(L,-1));
  lua_pop(L,1);
  return 0;
}

unsigned Keep(lua_State *L,int refs) {
  if (lua_isnil(L,-1)) {lua_pop(L,1);return 0;}
  const auto slot=static_cast<unsigned>(lua_rawlen(L,refs)+1);
  lua_rawseti(L,refs,slot);
  return slot;
}

unsigned KeepField(lua_State *L,int table,int refs,const char *field) {
  lua_getfield(L,table,field);
  return Keep(L,refs);
}

void Compile(lua_State *L,int type,int refs,std::vector<Node> &layout) {
  if (!lua_istable(L,type) || !lua_checkstack(L,8)) throw std::runtime_error("mod_api_compile_type");
  static constexpr std::array<std::string_view,13> names={"bool","int","float","number","string",
    "vector","color","kcolor","entity","player","struct","list","map"};
  const auto kind=TextField(L,type,"kind");
  const auto found=std::find(names.begin(),names.end(),kind);
  if (found==names.end()) throw std::runtime_error("mod_api_compile_kind");
  const auto index=layout.size();
  Node node{};
  node.kind=static_cast<NodeKind>(found-names.begin());
  node.off=NumberField(L,type,"off");
  node.fixed=NumberField(L,type,"fixed");
  node.worst=NumberField(L,type,"worst");
  node.max=NumberField(L,type,"max");
  node.keymax=NumberField(L,type,"keymax");
  node.string_key=TextField(L,type,"mapKey")=="string";
  node.key=KeepField(L,type,refs,"key");
  if (!node.fixed || node.worst < node.fixed || node.worst > 65536)
    throw std::runtime_error("mod_api_compile_size");
  layout.push_back(node);
  if (node.kind==NodeKind::Struct) {
    lua_getfield(L,type,"fields");
    const int fields=lua_absindex(L,-1);
    node.children=static_cast<unsigned>(lua_rawlen(L,fields));
    for (unsigned i=1;i<=node.children;++i) {
      lua_rawgeti(L,fields,i);
      Compile(L,lua_absindex(L,-1),refs,layout);
      lua_pop(L,1);
    }
    lua_pop(L,1);
  } else if (node.kind==NodeKind::List || node.kind==NodeKind::Map) {
    lua_getfield(L,type,"element");
    node.children=1;
    Compile(L,lua_absindex(L,-1),refs,layout);
    lua_pop(L,1);
  }
  node.end=static_cast<unsigned>(layout.size());
  layout[index]=node;
}

Declaration CompileDeclaration(lua_State *L,int item,int refs) {
  Declaration result;
  result.kind=TextField(L,item,"kind");
  result.name=TextField(L,item,"name");
  result.key=KeepField(L,item,refs,"name");
  result.scope=TextField(L,item,"scope");
  result.rate=TextField(L,item,"rate");
  result.text=TextField(L,item,"text");
  lua_getfield(L,item,"private");
  result.private_value=lua_toboolean(L,-1)!=0;
  lua_pop(L,1);
  result.get=KeepField(L,item,refs,"get");
  result.set=KeepField(L,item,refs,"set");
  result.reset=KeepField(L,item,refs,"reset");
  result.handler=KeepField(L,item,refs,"handler");
  result.filter=KeepField(L,item,refs,"filter");
  result.layout->entity=result.scope=="entity";
  if(result.layout->entity){
    lua_getfield(L,item,"filter");
    const char *const names[]={"type","variant","subtype"};
    for(unsigned i=0;i<result.shape.size();++i){
      lua_getfield(L,-1,names[i]);if(lua_isinteger(L,-1))result.shape[i]=unsigned(lua_tointeger(L,-1));lua_pop(L,1);
    }
    lua_pop(L,1);
  }
  lua_getfield(L,item,"type");
  if (result.kind!="Keys") Compile(L,lua_absindex(L,-1),refs,result.layout->nodes);
  result.layout->player=result.scope=="player";
  result.layout->private_value=result.private_value;
  lua_pop(L,1);
  return result;
}

// 主线程同时核对引擎配置和实际 RegisterMod 身份，并把所有注册引用绑到当前 Lua VM。
void Capture(lua_State *L,net::ModManifest &manifest) {
  lua_newtable(L);
  const int refs=lua_absindex(L,-1);
  std::array<unsigned,3> constructors{};
  const char *const names[]={"Vector","Color","KColor"};
  for (unsigned i=0;i<constructors.size();++i) {
    lua_getglobal(L,names[i]);
    constructors[i]=Keep(L,refs);
    if (!constructors[i]) throw std::runtime_error("mod_api_constructor_unavailable");
  }

  std::map<std::string,std::pair<std::string,bool>> enabled;
  if (!g_Manager) throw std::runtime_error("mod_api_manager_unavailable");
  lua_newtable(L);
  const int sources=lua_absindex(L,-1);
  if (g_Manager->GetOptions()->_enableMods) {
    const auto root=std::filesystem::u8path(&g_ModsDirectory);
    for (const auto *mod:g_Manager->GetModManager()->_mods) {
      if (!mod) continue;
      const bool include=mod->_enabled && !InternalMod(mod->_directory);
      // REPENTOGON 的 sourceid 来自 Workshop ID，缺少时用目录。握手按目录关联。
      for (const auto *source:{&mod->_directory,&mod->_metadataWorkshopID}) {
        if (source->empty()) continue;
        if (include) lua_pushlstring(L,mod->_directory.data(),mod->_directory.size());
        else lua_pushboolean(L,0);
        lua_setfield(L,sources,source->c_str());
      }
      if (!include) continue;
      const auto file=root/std::filesystem::u8path(mod->_directory)/"main.lua";
      std::error_code ec;
      const auto status=std::filesystem::status(file,ec);
      if (ec && ec!=std::errc::no_such_file_or_directory)
        throw std::runtime_error("mod_manifest: 无法读取 " + net::ModManifestDetail(mod->_directory));
      const bool has_lua=std::filesystem::is_regular_file(status);
      auto dir=mod->_directory;
      if (!enabled.emplace(dir,std::make_pair(mod->_name,has_lua)).second)
        throw std::runtime_error("mod_manifest: 重复的 Mod 目录 " + net::ModManifestDetail(dir));
      manifest.emplace("1 Mod " + dir,mod->_name);
      manifest.emplace("2 Mod 版本 " + dir,mod->_metadataVersion);
      manifest.emplace("3 Mod main.lua " + dir,has_lua?"1":"0");
    }
  }

  if (registration.compiled && registration.enabled_manifest==manifest) {
    manifest=registration.compiled_manifest;
    lua_pop(L,2);
    return;
  }
  auto enabled_manifest=manifest;
  std::map<std::string,Registration> compiled;
  std::set<int> keys;
  std::set<const void *> supported;
  std::set<const void *> internal;
  lua_getglobal(L,"REPENTOGON");
  internal.insert(lua_topointer(L,-1));
  lua_pop(L,1);
  for (const auto &mod:g_Mods) {
    if (!mod._luaTableRef || mod._luaTableRef->_state!=L) continue;
    const auto dir=std::filesystem::u8path(mod._mainLuaPath).parent_path().filename().u8string();
    if (!InternalMod(dir)) continue;
    lua_rawgeti(L,LUA_REGISTRYINDEX,mod._luaTableRef->_ref);
    internal.insert(lua_topointer(L,-1));
    lua_pop(L,1);
  }
  lua_rawgetp(L,LUA_REGISTRYINDEX,&sdk_entries);
  const int entries=lua_absindex(L,-1);
  lua_pushnil(L);
  while (lua_next(L,entries)) {
    const auto dir=ModDirectory(L,-2);
    const auto enabled_mod=enabled.find(dir);
    const int entry=lua_absindex(L,-1);
    const auto release=TextField(L,entry,"version");
    if (enabled_mod!=enabled.end() && !release.empty()) {
      const auto api=NumberField(L,entry,"api");
      if (api>1) throw std::runtime_error("mod_manifest: " + net::ModManifestDetail(enabled_mod->second.first) + " 需要更新联机层");
      Registration mod;
      mod.table=lua_topointer(L,-2);
      lua_pushvalue(L,-2);mod.table_ref=Keep(L,refs);
      manifest.emplace("4 Mod 自报版本 " + dir,release);
      manifest.emplace("5 Mod SDK 版本 " + dir,std::to_string(api));
      lua_getfield(L,entry,"records");
      const int records=lua_absindex(L,-1);
      const auto count=lua_rawlen(L,records);
      for (std::size_t i=1;i<=count;++i) {
        lua_rawgeti(L,records,static_cast<lua_Integer>(i));
        const int index=lua_absindex(L,-1);
        auto item=CompileDeclaration(L,index,refs);
        if(item.kind=="Keys"){
          lua_getfield(L,index,"keys");
          for(std::size_t k=1;k<=lua_rawlen(L,-1);++k){
            lua_rawgeti(L,-1,k);
            keys.insert(static_cast<int>(lua_tointeger(L,-1)));
            lua_pop(L,1);
          }
          lua_pop(L,1);
          if(keys.size()>64)throw std::runtime_error("mod_manifest: 扩展按键合计最多 64 个");
        }
        if(item.rate=="world"&&std::any_of(item.layout->nodes.begin(),item.layout->nodes.end(),[](const Node &node){
            return (node.kind==NodeKind::List||node.kind==NodeKind::Map)&&node.max>256;
          })){
          if(Settings.diagnostics)Trace("mod_state","phase=large_world_container mod=%s key=%s",dir.c_str(),item.name.c_str());
        }
        manifest.emplace("6 Mod 声明 " + dir + " #" + std::to_string(i),item.text);
        mod.declarations.push_back(std::move(item));
        lua_pop(L,1);
      }
      lua_pop(L,1);
      supported.insert(mod.table);
      if (!compiled.emplace(dir,std::move(mod)).second)
        throw std::runtime_error("mod_manifest: 多次登记 " + net::ModManifestDetail(dir));
    }
    lua_pop(L,1);
  }
  lua_pop(L,1);
  unsigned ordinal=0;
  for (const auto &[dir,info]:enabled) {
    const auto mod=compiled.find(dir);
    if (info.second && mod==compiled.end())
      throw std::runtime_error("mod_manifest: 未声明的 Lua Mod " + net::ModManifestDetail(info.first.empty()?dir:info.first));
    if (mod!=compiled.end()) mod->second.ordinal=ordinal;
    ++ordinal;
  }

  if (!enabled.empty()) {
    lua_rawgetp(L,LUA_REGISTRYINDEX,&id_reader);
    lua_pushvalue(L,sources);
    if (lua_pcall(L,1,1,0)!=LUA_OK)
      throw std::runtime_error("mod_manifest: " + net::ModManifestDetail(ReadString(L,-1)));
    const int rows=lua_absindex(L,-1);
    std::map<std::string,std::vector<std::string>> ids;
    for (std::size_t i=1;i<=lua_rawlen(L,rows);++i) {
      lua_rawgeti(L,rows,static_cast<lua_Integer>(i));
      lua_rawgeti(L,-1,1);lua_rawgeti(L,-2,2);
      ids[ReadString(L,-2)].push_back(ReadString(L,-1));
      lua_pop(L,3);
    }
    for (auto &[name,values]:ids) {
      std::sort(values.begin(),values.end());
      std::string text;
      for (const auto &id:values) {text+=id;text+=';';}
      manifest.emplace("7 内容 ID " + name,std::move(text));
    }
    lua_pop(L,1);
  }
  lua_pop(L,1);

  std::size_t bytes=0;
  for (const auto &[key,value]:manifest) {
    bytes+=key.size()+value.size()+20;
    if (bytes>net::kModManifestBytes) throw std::runtime_error("mod_manifest_too_large");
  }
  auto compiled_manifest=manifest;
  auto schema=std::make_shared<state::ModSchema>();
  schema->declared=true;
  RegisterNativeModRows(*schema);
  std::size_t schema_bytes=sizeof(state::ModSchema);
  for(const auto &[dir,mod]:compiled)for(unsigned i=0;i<mod.declarations.size();++i){
    const auto &item=mod.declarations[i];
    if(item.kind!="State"&&!item.layout->entity)continue;
    schema->layouts.emplace(state::ModRecordKey{std::uint16_t(mod.ordinal),std::uint16_t(i),state::kModGlobal},item.layout);
    schema_bytes+=sizeof(state::ModLayout)+96+item.layout->nodes.capacity()*sizeof(Node);
  }
  schema->memory=memory::Reserve(memory::Pool::Values,schema_bytes,"mod_schema");
  lua_rawsetp(L,LUA_REGISTRYINDEX,&layout_references);
  registration.registered=std::move(compiled);
  registration.keys.assign(keys.begin(),keys.end());
  registration.supported=std::move(supported);
  registration.internal=std::move(internal);
  registration.constructors=constructors;
  registration.schema=std::move(schema);
  registration.enabled_manifest=std::move(enabled_manifest);
  registration.compiled_manifest=std::move(compiled_manifest);
  registration.compiled=true;
  registration.loaded=true;
  registration.reload_directory.clear();
}

int CaptureThunk(lua_State *L) {
  auto *manifest=static_cast<net::ModManifest *>(lua_touserdata(L,1));
  try {Capture(L,*manifest);return 0;}
  catch (const std::exception &error) {lua_pushstring(L,error.what());}
  catch (...) {lua_pushliteral(L,"mod_manifest_unavailable");}
  return lua_error(L);
}

void PushRef(lua_State *L,unsigned ref) {
  lua_rawgetp(L,LUA_REGISTRYINDEX,&layout_references);
  lua_rawgeti(L,-1,ref);lua_remove(L,-2);
}
int ModTraceback(lua_State *L) {
  luaL_traceback(L,L,lua_type(L,1)==LUA_TSTRING?lua_tostring(L,1):"Lua error",1);
  return 1;
}
bool ModCall(lua_State *L,lua_CFunction function,void *argument,const std::string &dir,
             const char *name,const char *phase,std::uint64_t &failures) noexcept {
  const int top=lua_gettop(L);
  if(!lua_checkstack(L,4)){++failures;return false;}
  lua_pushcfunction(L,ModTraceback);
  lua_pushcfunction(L,function);lua_pushlightuserdata(L,argument);
  const bool ok=lua_pcall(L,1,0,top+1)==LUA_OK;
  if(!ok&&++failures<=3){
    const auto *message=lua_tostring(L,-1);
    Trace("mod_state","phase=%s mod=%s key=%s error=%s",phase,dir.c_str(),name,message?message:"lua_error");
  }
  lua_settop(L,top);
  return ok;
}
bool CurrentModVm() noexcept {
  return IsMain()&&registration.vm&&registration.schema&&g_LuaEngine&&g_LuaEngine->_state==registration.vm&&
         registration.ready;
}

struct CaptureValue {
  const Declaration &declaration;
  const Registration &mod;
  state::ModRawValue &out;
  Entity_Player *target;
  void *context;
  ModCaptureEntity entity;
  Entity *entity_target=nullptr;
  unsigned missing=0;
  std::set<std::string> *ignored=nullptr;
};
// 通用实体参数使用官方回调的 Entity 基类注册；玩家作用域仍单独使用 EntityPlayer。
void PushModEntity(lua_State *L,Entity *entity) {
  if(entity)lua::luabridge::UserdataPtr::push(L,entity,lua::GetMetatableKey(lua::Metatables::ENTITY));
  else lua_pushnil(L);
}
void PushEntityData(lua_State *L,Entity *entity) {
  PushModEntity(L,entity);
  lua_getfield(L,-1,"GetData");lua_pushvalue(L,-2);lua_call(L,1,1);
  lua_remove(L,-2);
}
void Put(state::ModRawValue &out,std::size_t at,std::size_t size,std::uint64_t value) {
  state::ModWrite(out.bytes.data()+at,size,value);
}
void PutFloat(state::ModRawValue &out,std::size_t at,float value) {
  std::uint32_t bits;std::memcpy(&bits,&value,sizeof bits);Put(out,at,4,bits);
}
std::int32_t LuaI32(lua_State *L,int index) {
  int integer=0;const auto value=lua_tointegerx(L,index,&integer);
  if(lua_type(L,index)!=LUA_TNUMBER||!integer||value<INT32_MIN||value>INT32_MAX)
    luaL_error(L,"Mod 状态需要 i32");
  return static_cast<std::int32_t>(value);
}
void ReadStringSlot(lua_State *L,int value,state::ModRawValue &out,std::size_t at,unsigned max) {
  if(lua_type(L,value)!=LUA_TSTRING)luaL_error(L,"Mod 状态需要字符串");
  std::size_t length=0;
  const auto *bytes=reinterpret_cast<const std::byte *>(lua_tolstring(L,value,&length));
  if(length>max)luaL_error(L,"Mod 字符串超限");
  Put(out,at,2,length);
  out.bytes.insert(out.bytes.end(),bytes,bytes+length);
}
// 每个递归栈帧只持有平凡局部量，Lua 元方法抛错时由外层 pcall 回收暂存。
void CaptureNode(lua_State *L,CaptureValue &job,unsigned i,int value,std::size_t at) {
  if(!lua_checkstack(L,8))luaL_error(L,"Mod 状态 Lua 栈不足");
  const auto &nodes=job.out.layout->nodes;
  const auto &node=nodes[i];auto &out=job.out;
  switch(node.kind){
  case NodeKind::Bool:
    if(!lua_isboolean(L,value))luaL_error(L,"Mod 状态需要布尔值");
    Put(out,at,1,lua_toboolean(L,value));return;
  case NodeKind::Int: Put(out,at,4,std::uint32_t(LuaI32(L,value)));return;
  case NodeKind::Float:
  case NodeKind::Number:{
    if(lua_type(L,value)!=LUA_TNUMBER)luaL_error(L,"Mod 状态需要数值");
    const double number=lua_tonumber(L,value);
    if(node.kind==NodeKind::Float)PutFloat(out,at,static_cast<float>(number));
    else {std::uint64_t bits;std::memcpy(&bits,&number,sizeof bits);Put(out,at,8,bits);}
    return;
  }
  case NodeKind::String: ReadStringSlot(L,value,out,at,node.max);return;
  case NodeKind::Vector:{
    const auto *v=lua::GetLuabridgeUserdata<Vector *>(L,value,lua::Metatables::VECTOR,lua::Metatables::CONST_VECTOR,"Vector");
    PutFloat(out,at,v->x);PutFloat(out,at+4,v->y);return;
  }
  case NodeKind::Color:{
    const auto *v=lua::GetLuabridgeUserdata<ColorMod *>(L,value,lua::Metatables::COLOR,lua::Metatables::CONST_COLOR,"Color");
    for(unsigned n=0;n<4;++n)PutFloat(out,at+4*n,v->_tint[n]);
    for(unsigned n=0;n<3;++n)PutFloat(out,at+16+4*n,v->_offset[n]);
    return;
  }
  case NodeKind::KColor:{
    const auto *v=lua::GetLuabridgeUserdata<KColor *>(L,value,lua::Metatables::KCOLOR,lua::Metatables::CONST_KCOLOR,"KColor");
    PutFloat(out,at,v->_red);PutFloat(out,at+4,v->_green);PutFloat(out,at+8,v->_blue);PutFloat(out,at+12,v->_alpha);return;
  }
  case NodeKind::Entity:{
    Put(out,at,2,state::kModGlobal);
    if(lua_isnil(L,value))return;
    const auto *entity=lua::GetLuabridgeUserdata<Entity *>(L,value,lua::Metatables::ENTITY,lua::Metatables::CONST_ENTITY,"Entity");
    std::uint64_t id=0,use=0;
    if(entity&&job.entity&&job.entity(job.context,entity,id,use))out.references.push_back({at,id,use});
    else ++job.missing;
    return;
  }
  case NodeKind::Player:{
    unsigned ordinal=state::kModPublic;
    if(!lua_isnil(L,value)){
      const auto *player=lua::GetLuabridgeUserdata<Entity_Player *>(L,value,lua::Metatables::ENTITY_PLAYER,
        lua::Metatables::CONST_ENTITY_PLAYER,"EntityPlayer");
      if(!player||!ParticipantOrdinal(player,ordinal)){++job.missing;ordinal=state::kModPublic;}
    }
    Put(out,at,1,ordinal);return;
  }
  case NodeKind::Struct:{
    if(!lua_istable(L,value)&&!lua_isuserdata(L,value))luaL_error(L,"Mod 状态需要可索引对象");
    // 首次诊断只枚举原始 Lua 表，正常读取继续逐项走声明的索引。
    if(job.ignored&&lua_istable(L,value)){
      lua_pushnil(L);
      while(lua_next(L,value)){
        bool declared=false;
        for(auto field=i+1;field<node.end;field=nodes[field].end){
          PushRef(L,nodes[field].key);
          declared=lua_rawequal(L,-3,-1)!=0;
          lua_pop(L,1);
          if(declared)break;
        }
        if(!declared){
          if(lua_type(L,-2)==LUA_TSTRING){
            std::size_t size=0;const auto *key=lua_tolstring(L,-2,&size);
            job.ignored->emplace(key,size);
          }else if(lua_isinteger(L,-2))job.ignored->insert(std::to_string(lua_tointeger(L,-2)));
          else job.ignored->insert(lua_typename(L,lua_type(L,-2)));
        }
        lua_pop(L,1);
      }
    }
    auto child=i+1;
    for(unsigned n=0;n<node.children;++n){
      const auto &field=nodes[child];
      PushRef(L,field.key);lua_gettable(L,value);
      if(!lua_isnil(L,-1)){
        out.bytes[at+n/8]|=std::byte(1u<<(n%8));
        CaptureNode(L,job,child,lua_absindex(L,-1),at+field.off);
      }
      lua_pop(L,1);child=field.end;
    }
    return;
  }
  case NodeKind::List:{
    if(!lua_istable(L,value)&&!lua_isuserdata(L,value))luaL_error(L,"Mod 状态需要列表");
    unsigned count=0;
    while(lua_geti(L,value,count+1)!=LUA_TNIL){
      if(count==node.max)luaL_error(L,"Mod 列表超限");
      const auto offset=out.bytes.size();
      out.bytes.resize(offset+nodes[i+1].fixed);
      CaptureNode(L,job,i+1,lua_absindex(L,-1),offset);
      lua_pop(L,1);++count;
    }
    lua_pop(L,1);Put(out,at,2,count);return;
  }
  case NodeKind::Map:{
    if(!lua_istable(L,value))luaL_error(L,"Mod 状态需要 map 表");
    const auto map=out.maps.size();out.maps.push_back({i,{}});
    unsigned count=0;
    lua_pushnil(L);
    while(lua_next(L,value)){
      if(count==node.max)luaL_error(L,"Mod map 超限");
      const auto first=out.bytes.size();
      out.bytes.resize(first+(node.string_key?2:4));
      if(node.string_key)ReadStringSlot(L,-2,out,first,node.keymax);
      else Put(out,first,4,std::uint32_t(LuaI32(L,-2)));
      const auto offset=out.bytes.size();
      out.bytes.resize(offset+nodes[i+1].fixed);
      CaptureNode(L,job,i+1,lua_absindex(L,-1),offset);
      out.maps[map].items.push_back({first,out.bytes.size()});
      lua_pop(L,1);++count;
    }
    Put(out,at,2,count);return;
  }
  }
}
int CaptureValueThunk(lua_State *L) {
  auto &job=*static_cast<CaptureValue *>(lua_touserdata(L,1));
  try {
    if(job.declaration.get){
      PushRef(L,job.declaration.get);
      if(job.entity_target)PushModEntity(L,job.entity_target);
      else if(job.target)PushPlayer(L,job.target);
      lua_call(L,job.target||job.entity_target?1:0,1);
    }else{
      if(job.entity_target)PushEntityData(L,job.entity_target);
      else PushRef(L,job.mod.table_ref);
      PushRef(L,job.declaration.key);lua_gettable(L,-2);
    }
    if(!lua_isnil(L,-1)){
      job.out.bytes.resize(job.out.layout->nodes[0].fixed);
      CaptureNode(L,job,0,lua_absindex(L,-1),0);
    }
    return 0;
  }catch(...) {}
  return luaL_error(L,"Mod 状态暂存失败");
}

// 当前客机场景的已安装字节和失败见证。失败的同一值只在完整基线时重试。
struct InstalledModValue {
  state::ModValueRef value,failed;
  bool unresolved=false,failed_delete=false;
  state::ModEntityId entity;
  std::uint64_t instance=0,failed_instance=0;
};
std::map<state::ModRecordKey,InstalledModValue> installed;
memory::Lease installed_memory;
bool mod_scene_started=false;
Registration *mod_install_owner=nullptr;

struct InstallValue {
  const Declaration &declaration;
  const Registration &mod;
  const state::ModValue *value;
  const ModClientLookup &lookup;
  Entity_Player *player;
  Entity *entity=nullptr;
  bool unresolved=false;
};
template<class T,class B>T LuaBits(B bits) noexcept {
  static_assert(sizeof(T)==sizeof(B));T value;std::memcpy(&value,&bits,sizeof(value));return value;
}
// 用 Lua 栈保管遍历中的表，不在 protected call 内持有需要析构的 C++ 临时对象。
void ClearLuaTable(lua_State *L,int table) {
  lua_newtable(L);
  const auto keys=lua_absindex(L,-1);
  lua_Integer count=0;
  lua_pushnil(L);
  while(lua_next(L,table)){
    lua_pop(L,1);lua_pushvalue(L,-1);lua_rawseti(L,keys,++count);
  }
  for(lua_Integer i=1;i<=count;++i){
    lua_rawgeti(L,keys,i);lua_pushnil(L);lua_settable(L,table);
  }
  lua_pop(L,1);
}
bool TableNode(NodeKind kind) noexcept {
  return kind==NodeKind::Struct||kind==NodeKind::List||kind==NodeKind::Map;
}
void PushNode(lua_State *L,InstallValue &job,unsigned i,const std::byte *fixed,const std::byte *&tail,int existing) {
  if(!lua_checkstack(L,10))luaL_error(L,"Mod 写回 Lua 栈不足");
  const auto &nodes=job.declaration.layout->nodes;
  const auto &node=nodes[i];
  const auto bits=[&](unsigned size){return state::ModRead(fixed,size);};
  switch(node.kind){
  case NodeKind::Bool: lua_pushboolean(L,bits(1)!=0);return;
  case NodeKind::Int: lua_pushinteger(L,LuaBits<std::int32_t>(std::uint32_t(bits(4))));return;
  case NodeKind::Float: lua_pushnumber(L,LuaBits<float>(std::uint32_t(bits(4))));return;
  case NodeKind::Number: lua_pushnumber(L,LuaBits<double>(bits(8)));return;
  case NodeKind::String:{
    const auto length=std::size_t(bits(2));
    lua_pushlstring(L,reinterpret_cast<const char *>(tail),length);tail+=length;return;
  }
  case NodeKind::Vector:case NodeKind::Color:case NodeKind::KColor:{
    const auto constructor=node.kind==NodeKind::Vector?0u:node.kind==NodeKind::Color?1u:2u;
    PushRef(L,registration.constructors[constructor]);
    for(unsigned n=0;n<node.fixed/4;++n)
      lua_pushnumber(L,LuaBits<float>(std::uint32_t(state::ModRead(fixed+4*n,4))));
    lua_call(L,node.fixed/4,1);return;
  }
  case NodeKind::Entity:{
    const auto member=std::uint16_t(bits(2));bool known=false;std::uint64_t instance=0;
    auto *entity=member!=state::kModGlobal&&job.lookup.entity?
      static_cast<Entity *>(job.lookup.entity(job.lookup.context,member,known,instance)):nullptr;
    if(entity)PushModEntity(L,entity);
    else {lua_pushnil(L);job.unresolved|=known;}
    return;
  }
  case NodeKind::Player:{
    const auto ordinal=unsigned(bits(1));auto *player=FindPlayer(ordinal);
    if(!player&&ordinal<4&&(job.lookup.players&(1u<<ordinal)))job.unresolved=true;
    PushPlayer(L,player);return;
  }
  case NodeKind::Struct:{
    if(existing)lua_pushvalue(L,existing);
    else lua_createtable(L,0,node.children);
    const int table=lua_absindex(L,-1);
    auto child=i+1;
    for(unsigned n=0;n<node.children;++n){
      const auto &field=nodes[child];
      PushRef(L,field.key);
      if(state::ModRead(fixed+n/8,1)&(1u<<(n%8))){
        if(existing&&TableNode(field.kind)){
          lua_pushvalue(L,-1);lua_gettable(L,table);
          PushNode(L,job,child,fixed+field.off,tail,lua_istable(L,-1)?lua_absindex(L,-1):0);
          lua_remove(L,-2);
        }else PushNode(L,job,child,fixed+field.off,tail,0);
      }else lua_pushnil(L);
      if(existing)lua_settable(L,table);else lua_rawset(L,table);
      child=field.end;
    }
    return;
  }
  case NodeKind::List:case NodeKind::Map:{
    const auto count=unsigned(bits(2));const bool list=node.kind==NodeKind::List;
    if(existing){ClearLuaTable(L,existing);lua_pushvalue(L,existing);}
    else lua_createtable(L,list?count:0,list?0:count);
    const int table=lua_absindex(L,-1);
    for(unsigned n=0;n<count;++n){
      if(list)lua_pushinteger(L,n+1);
      else if(node.string_key){
        const auto length=std::size_t(state::ModRead(tail,2));tail+=2;
        lua_pushlstring(L,reinterpret_cast<const char *>(tail),length);tail+=length;
      }else{
        lua_pushinteger(L,LuaBits<std::int32_t>(std::uint32_t(state::ModRead(tail,4))));tail+=4;
      }
      const auto element=tail;tail+=nodes[i+1].fixed;
      PushNode(L,job,i+1,element,tail,0);
      if(existing)lua_settable(L,table);else lua_rawset(L,table);
    }
    return;
  }
  }
}
void PushValue(lua_State *L,InstallValue &job,int existing=0) {
  if(!job.value||job.value->bytes.empty()){lua_pushnil(L);return;}
  const auto *fixed=job.value->bytes.data();
  const auto *tail=fixed+job.declaration.layout->nodes.front().fixed;
  PushNode(L,job,0,fixed,tail,existing);
}
int InstallValueThunk(lua_State *L) {
  auto &job=*static_cast<InstallValue *>(lua_touserdata(L,1));
  const auto &item=job.declaration;
  if(item.set){
    PushRef(L,item.set);
    if(job.entity)PushModEntity(L,job.entity);
    else if(job.player)PushPlayer(L,job.player);
    PushValue(L,job);lua_call(L,job.player||job.entity?2:1,0);
  }else{
    if(job.entity)PushEntityData(L,job.entity);
    else PushRef(L,job.mod.table_ref);
    const auto mod=lua_absindex(L,-1);
    PushRef(L,item.key);
    if(job.value&&!job.value->bytes.empty()&&TableNode(item.layout->nodes.front().kind)){
      lua_pushvalue(L,-1);lua_gettable(L,mod);
      PushValue(L,job,lua_istable(L,-1)?lua_absindex(L,-1):0);lua_remove(L,-2);
    }else PushValue(L,job);
    lua_settable(L,mod);
  }
  return 0;
}
struct NotifyMod {
  Registration &mod;
  const char *event;
  const Declaration *item=nullptr;
  unsigned callback=0,count=0;
  Entity_Player *player=nullptr;
  Entity *entity=nullptr;
  const net::ModIntentResult *result=nullptr;
};
bool PushIntentResult(lua_State *,const std::byte *&,const std::byte *);
int NotifyModThunk(lua_State *L) {
  auto &job=*static_cast<NotifyMod *>(lua_touserdata(L,1));
  lua_rawgetp(L,LUA_REGISTRYINDEX,&sdk_entries);
  PushRef(L,job.mod.table_ref);lua_rawget(L,-2);
  if(!lua_istable(L,-1))return 0;
  lua_pushliteral(L,"callbacks");lua_rawget(L,-2);
  if(!lua_istable(L,-1))return 0;
  lua_pushstring(L,job.event);lua_rawget(L,-2);
  if(!lua_istable(L,-1))return 0;
  if(!job.callback){job.count=unsigned(lua_rawlen(L,-1));return 0;}
  lua_rawgeti(L,-1,job.callback);
  if(!lua_isfunction(L,-1))return 0;
  if(job.result){
    PushRef(L,job.item->key);lua_pushinteger(L,static_cast<lua_Integer>(job.result->id));
    lua_pushboolean(L,job.result->ok);
    const auto *cursor=job.result->bytes.data();
    const auto *end=cursor+job.result->size;
    if(cursor==end)lua_pushnil(L);
    else if(!PushIntentResult(L,cursor,end)||cursor!=end)return luaL_error(L,"Mod 意图结果格式无效");
    lua_call(L,4,0);return 0;
  }
  if(job.item){
    PushRef(L,job.item->key);
    if(job.entity)PushModEntity(L,job.entity);else PushPlayer(L,job.player);
  }else if(job.entity)PushModEntity(L,job.entity);
  lua_call(L,job.item?2:job.entity?1:0,0);return 0;
}
void Notify(const std::string &dir,NotifyMod &job) noexcept {
  if(!ModCall(registration.vm,NotifyModThunk,&job,dir,job.event,"notify",job.mod.callback_failures))return;
  for(job.callback=1;job.callback<=job.count;++job.callback)
    ModCall(registration.vm,NotifyModThunk,&job,dir,job.event,"notify",job.mod.callback_failures);
}
int ResetModThunk(lua_State *L) {
  PushRef(L,*static_cast<const unsigned *>(lua_touserdata(L,1)));
  lua_call(L,0,0);return 0;
}

// Lua 的玩法入口仍按客机隔离规则拒绝；分派作用域只授权本阶段的 protected call。
class ModInstallScope final {
public:
  ModInstallScope():dispatch_(ClientOperation::ModInstall,ClientEntry::ModStateInstall) {
    active_=dispatch_.Ready()&&ClientDispatchBefore(ClientEntry::ModStateInstall)==ClientEntryDecision::CallOriginal;
  }
  ~ModInstallScope(){
    mod_install_owner=nullptr;
    if(active_)ClientDispatchReturned(ClientEntry::ModStateInstall);
  }
  explicit operator bool()const noexcept {return active_;}
private:
  ClientDispatchScope dispatch_;
  bool active_=false;
};

// 结果是小型纯 Lua 值，用字节上限约束所有递归和容器项。表只允许标量键。
enum class ResultKind : unsigned {Nil,False,True,Integer,Number,String,Table};
bool PutResult(net::ModIntentResult &out,unsigned size,std::uint64_t bits) noexcept {
  if(size>out.bytes.size()-out.size)return false;
  state::ModWrite(out.bytes.data()+out.size,size,bits);out.size+=std::uint16_t(size);return true;
}
bool SaveIntentResult(lua_State *L,int index,net::ModIntentResult &out,bool key=false) {
  if(!lua_checkstack(L,5))return false;
  const auto tag=[&](ResultKind kind){return PutResult(out,1,unsigned(kind));};
  switch(lua_type(L,index)){
  case LUA_TNIL:return !key&&tag(ResultKind::Nil);
  case LUA_TBOOLEAN:return tag(lua_toboolean(L,index)?ResultKind::True:ResultKind::False);
  case LUA_TNUMBER:
    if(lua_isinteger(L,index))return tag(ResultKind::Integer)&&PutResult(out,8,std::uint64_t(lua_tointeger(L,index)));
    return tag(ResultKind::Number)&&PutResult(out,8,LuaBits<std::uint64_t>(double(lua_tonumber(L,index))));
  case LUA_TSTRING:{
    std::size_t size=0;const auto *chars=lua_tolstring(L,index,&size);
    if(!tag(ResultKind::String)||!PutResult(out,2,size)||size>out.bytes.size()-out.size)return false;
    std::memcpy(out.bytes.data()+out.size,chars,size);out.size+=std::uint16_t(size);return true;
  }
  case LUA_TTABLE:{
    if(key||!tag(ResultKind::Table))return false;
    const auto at=out.size;
    if(!PutResult(out,1,0))return false;
    unsigned count=0;
    lua_pushnil(L);
    while(lua_next(L,index)){
      if(!SaveIntentResult(L,lua_absindex(L,-2),out,true)||!SaveIntentResult(L,lua_absindex(L,-1),out))return false;
      ++count;lua_pop(L,1);
    }
    state::ModWrite(out.bytes.data()+at,1,count);return true;
  }
  default:return false;
  }
}
bool PushIntentResult(lua_State *L,const std::byte *&at,const std::byte *end) {
  if(at==end||!lua_checkstack(L,5))return false;
  switch(static_cast<ResultKind>(std::to_integer<unsigned>(*at++))){
  case ResultKind::Nil:lua_pushnil(L);return true;
  case ResultKind::False:lua_pushboolean(L,0);return true;
  case ResultKind::True:lua_pushboolean(L,1);return true;
  case ResultKind::Integer:case ResultKind::Number:{
    const bool integer=at[-1]==std::byte(unsigned(ResultKind::Integer));
    if(end-at<8)return false;
    const auto bits=state::ModRead(at,8);at+=8;
    if(integer)lua_pushinteger(L,LuaBits<std::int64_t>(bits));
    else lua_pushnumber(L,LuaBits<double>(bits));
    return true;
  }
  case ResultKind::String:{
    if(end-at<2)return false;
    const auto size=state::ModRead(at,2);at+=2;
    if(size>std::uint64_t(end-at))return false;
    lua_pushlstring(L,reinterpret_cast<const char *>(at),std::size_t(size));at+=std::size_t(size);return true;
  }
  case ResultKind::Table:{
    if(at==end)return false;
    const auto count=std::to_integer<unsigned>(*at++);
    lua_createtable(L,0,count);const auto table=lua_absindex(L,-1);
    for(unsigned i=0;i<count;++i){
      if(!PushIntentResult(L,at,end)||lua_istable(L,-1)||lua_isnil(L,-1)||!PushIntentResult(L,at,end))return false;
      lua_rawset(L,table);
    }
    return true;
  }
  }
  return false;
}
struct PendingModIntent {net::ModIntent intent;bool client;};
std::map<std::uint64_t,PendingModIntent> pending_intents;
std::uint64_t next_intent_id=0;
net::ModIntentRate local_intent_rate;

int ReadIntentArgs(lua_State *L) {
  auto &job=*static_cast<CaptureValue *>(lua_touserdata(L,1));
  try {
    if(!lua_isnoneornil(L,2)){
      job.out.bytes.resize(job.out.layout->nodes[0].fixed);
      CaptureNode(L,job,0,2,0);
    }
    return 0;
  }catch(...) {}
  return luaL_error(L,"Mod 意图参数暂存失败");
}
int SendIntentLua(lua_State *L) {
  if(!CurrentModVm()||registration.vm!=L||lua_type(L,2)!=LUA_TSTRING||
     pending_intents.size()>=net::kModIntentInFlight||next_intent_id>=std::uint64_t(INT64_MAX))return 0;
  const auto now=GetTickCount64();
  if(!local_intent_rate.Ready(now)||!lua_checkstack(L,5))return 0;
  const bool client=ClientBusy();
  net::ModIntent intent;
  if(!(client?ClientModIntentContext(intent):HostModIntentContext(intent)))return 0;
  const auto *table=lua_topointer(L,1);
  const auto top=lua_gettop(L);
  try {
    const auto name=ReadString(L,2);
    for(auto &[dir,mod]:registration.registered)if(mod.table==table){
      for(unsigned i=0;i<mod.declarations.size();++i){
        const auto &declaration=mod.declarations[i];
        if(declaration.kind!="Intent"||declaration.name!=name)continue;
        auto workspace=memory::Reserve(memory::Pool::Workspace,32ull*declaration.layout->nodes.front().worst,"mod_intent_args");
        state::ModRawValue raw;raw.layout=declaration.layout;
        auto number=client?ClientModIntentMember:HostModIntentMember;
        CaptureValue args{declaration,mod,raw,nullptr,&number,
          [](void *context,const void *entity,std::uint64_t &id,std::uint64_t &use) noexcept {
            const auto lookup=*static_cast<std::uint16_t (**)(const void *) noexcept>(context);
            const auto member=lookup(entity);
            id=std::uint64_t(member)+1;use=0;return member!=state::kModGlobal;
          }};
        lua_pushcfunction(L,ModTraceback);lua_pushcfunction(L,ReadIntentArgs);lua_pushlightuserdata(L,&args);
        if(top>=3)lua_pushvalue(L,3);else lua_pushnil(L);
        const bool ok=lua_pcall(L,2,0,top+1)==LUA_OK;
        lua_settop(L,top);
        if(!ok||args.missing||raw.bytes.size()>net::kModIntentBytes)return 0;
        std::vector<std::byte> bytes;std::uint64_t missing=0;
        state::CanonicalModValue(raw,bytes,nullptr,[](void *,state::ModEntityId id) noexcept {
          return std::uint16_t(id.id-1);
        },missing);
        intent.id=++next_intent_id;intent.mod=std::uint16_t(mod.ordinal);intent.declaration=std::uint16_t(i);
        intent.size=std::uint16_t(bytes.size());std::copy(bytes.begin(),bytes.end(),intent.bytes.begin());
        pending_intents.emplace(intent.id,PendingModIntent{intent,client});
        if(!(client?SubmitClientModIntent(intent):SubmitHostModIntent(intent))){
          pending_intents.erase(intent.id);return 0;
        }
        local_intent_rate.Accept(now);
        lua_pushinteger(L,static_cast<lua_Integer>(intent.id));return 1;
      }
      break;
    }
  }catch(...){lua_settop(L,top);}
  return 0;
}

struct IntentHandler {InstallValue &args;net::ModIntentResult &result;};
int IntentHandlerThunk(lua_State *L) {
  auto &job=*static_cast<IntentHandler *>(lua_touserdata(L,1));
  PushRef(L,job.args.declaration.handler);PushPlayer(L,job.args.player);PushValue(L,job.args);
  if(job.args.unresolved)return 0;
  lua_call(L,2,2);
  job.result.ok=lua_isboolean(L,-2)&&lua_toboolean(L,-2);
  if(!lua_isnil(L,-1)&&!SaveIntentResult(L,lua_absindex(L,-1),job.result)){
    job.result.ok=false;job.result.size=0;
  }
  return 0;
}
}

void SampleModKeys(std::uint64_t &held,std::uint64_t &edge) noexcept {
  held=edge=0;
  if(registration.keys.empty()||(!ClientBusy()&&!HostBusy()))return;
  // 设备枚举依赖 C++20 的参与者拓扑头文件，本单元按 SDK 要求以 C++17 编译，交给拓扑模块执行。
  ReadNativeKeyboardKeys(registration.keys.data(),registration.keys.size(),held,edge);
}
bool InstallModApi(lua_State *L) noexcept {
  if (!IsMain() || !L) return false;
  const auto top=lua_gettop(L);
  registration.vm=L;
  registration.schema.reset();
  registration.ready=false;
  registration.loaded=false;
  registration.compiled=false;
  try {
    lua_pushcfunction(L,Install);
    const bool ok=lua_pcall(L,0,0,0)==LUA_OK;
    registration.ready=ok;
    if (!ok) Trace("mod_api","install=%s",lua_type(L,-1)==LUA_TSTRING?lua_tostring(L,-1):"lua_error");
  } catch (...) {Trace("mod_api","install=exception");}
  lua_settop(L,top);
  return registration.ready;
}

void CloseModApi() noexcept {
  // 引用由 VM 自身释放，C++ 只撤销不会再访问的布局及身份。
  if (IsMain()) {
    const bool pending=registration.reload_pending ||
        (registration.ready && (HostBusy() || ClientBusy()));
    installed.clear();installed_memory.reset();pending_intents.clear();RetireNativeModState();
    registration=LuaRegistration{};
    registration.reload_pending=pending;
  }
}

const char *CaptureModManifest(net::ModManifestRef &out) noexcept {
  if (!IsMain() || !registration.ready || !g_LuaEngine || registration.vm!=g_LuaEngine->_state)
    return "mod_api_unavailable";
  static std::string failure;
  auto *L=registration.vm;
  const int top=lua_gettop(L);
  try {
    auto next=std::make_shared<net::ModManifest>();
    lua_pushcfunction(L,CaptureThunk);
    lua_pushlightuserdata(L,next.get());
    if (lua_pcall(L,1,0,0)==LUA_OK) {
      out=std::move(next);
      lua_settop(L,top);
      return "";
    }
    failure=ReadString(L,-1);
    lua_settop(L,top);
    Trace("mod_api","manifest=%s",failure.c_str());
    return failure.empty()?"mod_manifest_unavailable":failure.c_str();
  } catch (...) {
    lua_settop(L,top);
    return "mod_manifest_unavailable";
  }
}

state::ModSchemaRef FrozenModSchema() noexcept {return registration.schema;}

void RunModIntent(const net::ModIntent &intent,void *player,net::ModIntentResult &result) noexcept {
  if(!player||!CurrentModVm()||intent.size>intent.bytes.size())return;
  try {
    for(auto &[dir,mod]:registration.registered)if(mod.ordinal==intent.mod){
      if(intent.declaration>=mod.declarations.size())return;
      auto &item=mod.declarations[intent.declaration];
      if(item.kind!="Intent"||!item.handler||!state::ValidModBytes(*item.layout,intent.bytes.data(),intent.size))return;
      state::ModValue value;
      value.bytes.assign(intent.bytes.begin(),intent.bytes.begin()+intent.size);
      ModClientLookup lookup;
      lookup.context=const_cast<net::ModIntent *>(&intent);lookup.players=15;
      lookup.entity=[](void *context,std::uint16_t member,bool &known,std::uint64_t &) noexcept {
        known=true;return ResolveHostModIntentMember(*static_cast<const net::ModIntent *>(context),member);
      };
      InstallValue args{item,mod,&value,lookup,static_cast<Entity_Player *>(player)};
      IntentHandler job{args,result};
      if(!ModCall(registration.vm,IntentHandlerThunk,&job,dir,item.name.c_str(),"intent",mod.callback_failures)){
        result.ok=false;result.size=0;
      }
      return;
    }
  }catch(...){result.ok=false;result.size=0;}
}

void DispatchModIntentResult(const net::ModIntentResult &result,bool client) noexcept {
  const auto at=pending_intents.find(result.id);
  if(at==pending_intents.end()||at->second.client!=client)return;
  const auto &intent=at->second.intent;
  if(intent.mod!=result.mod||intent.declaration!=result.declaration||
     intent.connection!=result.connection||intent.run!=result.run||intent.floor!=result.floor)return;
  pending_intents.erase(at);
  if(!CurrentModVm())return;
  const auto deliver=[&]{
    for(auto &[dir,mod]:registration.registered)if(mod.ordinal==result.mod){
      if(result.declaration>=mod.declarations.size())return;
      const auto &item=mod.declarations[result.declaration];
      if(item.kind!="Intent")return;
      mod_install_owner=client?&mod:nullptr;
      NotifyMod job{mod,"CLIENT_INTENT_RESULT",&item};job.result=&result;
      Notify(dir,job);
      return;
    }
  };
  if(client||ClientBusy()){ModInstallScope scope;if(scope)deliver();}
  else deliver();
}

std::uint64_t CancelModIntents(bool client) noexcept {
  std::array<net::ModIntentResult,net::kModIntentInFlight> cancelled{};
  unsigned count=0;
  for(const auto &[id,pending]:pending_intents)if(pending.client==client){
    const auto &value=pending.intent;
    auto &result=cancelled[count++];result.id=id;result.connection=value.connection;result.run=value.run;result.floor=value.floor;
    result.mod=value.mod;result.declaration=value.declaration;
  }
  for(unsigned i=0;i<count;++i)DispatchModIntentResult(cancelled[i],client);
  return count?cancelled[count-1].id:0;
}

state::ModRawRef CaptureModWorld(const state::ModRawRef &before,std::uint64_t sample,unsigned frame,bool transition,
    void *context,ModCaptureEntity entity,unsigned object_count,ModCaptureAt object_at) noexcept {
  if(!HostBusy())return {};
  if(!CurrentModVm()){
    if(++capture_skipped<=3)Trace("mod_state","phase=mod_capture_vm_mismatch");
    return before;
  }
  if(registration.schema->layouts.empty())return {};
  try {
    using Clock=std::chrono::steady_clock;
    std::array<Entity_Player *,4> players{};
    for(unsigned i=0;i<players.size();++i)players[i]=FindPlayer(i);
    auto out=std::make_shared<state::ModRawSnapshot>();
    const auto capacity=registration.schema->layouts.size()*5;
    out->memory=memory::Reserve(memory::Pool::Values,sizeof(*out)+(std::max)(capacity,before?before->records.size():0)*96,
      "mod_capture_snapshot");
    std::map<state::ModEntityId,ModCaptureObject> objects;
    memory::Lease object_memory;
    if(std::any_of(registration.schema->layouts.begin(),registration.schema->layouts.end(),
        [](const auto &entry){return entry.second->entity;})){
      object_memory=memory::Reserve(memory::Pool::Workspace,96ull*object_count,"mod_capture_entities");
      for(unsigned i=0;i<object_count;++i){
        ModCaptureObject object;
        if(object_at(context,i,object))objects.emplace(object.id,object);
      }
    }
    if(before)out->records=before->records;
    // 原生扩展使用保留的声明号，和同一份 World 一起进入成员号编码与 ACK。
    const auto capture_native=[&](NativeModRow row,state::ModEntityId id,const auto &capture){
      try {
        auto bytes=capture();
        if(bytes.empty())return;
        if(bytes.size()>65534)throw std::length_error("native_mod_record_size");
        const state::ModRecordKey key{kNativeMod,std::uint16_t(row),state::kModGlobal};
        const state::ModRawKey record{key,id};
        const auto old=out->records.find(record);
        if(old!=out->records.end()&&old->second->bytes.size()==bytes.size()+2&&
           !std::memcmp(old->second->bytes.data()+2,bytes.data(),bytes.size()))return;
        auto raw=std::make_shared<state::ModRawValue>();
        raw->layout=registration.schema->layouts.at(key);raw->sample=sample;
        raw->memory=memory::Reserve(memory::Pool::Values,sizeof(*raw)+bytes.size()+2,"native_mod_row");
        raw->bytes.resize(bytes.size()+2);
        state::ModWrite(raw->bytes.data(),2,bytes.size());
        std::memcpy(raw->bytes.data()+2,bytes.data(),bytes.size());
        out->records.insert_or_assign(record,std::move(raw));
      }catch(...){if(Settings.diagnostics)Trace("mod_state","phase=native_capture_failed row=%u",unsigned(row));}
    };
    capture_native(NativeModRow::Display,{},CaptureNativeModDisplay);
    if(!before||frame%3==0){
      capture_native(NativeModRow::Progress,{},CaptureNativeModProgress);
      for(const auto row:{NativeModRow::Encounters,NativeModRow::Kills,NativeModRow::Deaths})
        capture_native(row,{},[&]{return CaptureNativeModBestiary(row);});
    }
    for(const auto &[id,object]:objects)
      if(object.shape[0]==1&&!object.retained)
        capture_native(NativeModRow::Player,id,[&]{return CaptureNativeModPlayer(object.address);});
    for(auto it=out->records.begin();it!=out->records.end();){
      if(it->first.entity.id&&!objects.count(it->first.entity))it=out->records.erase(it);
      else ++it;
    }
    // 会话激活或换局后，Host 已清空上一份采集（before 为空），旧的失败序号不再阻止补读。
    const bool fresh=!before;
    for(auto &[dir,mod]:registration.registered)for(unsigned i=0;i<mod.declarations.size();++i){
      auto &item=mod.declarations[i];
      if(item.kind!="State"&&!item.layout->entity)continue;
      const state::ModRecordKey base{std::uint16_t(mod.ordinal),std::uint16_t(i),state::kModGlobal};
      const auto capture=[&](state::ModRawKey key,Entity_Player *player,Entity *target,
                              std::uint64_t mark,std::uint64_t consumed,std::uint64_t failed){
        const auto old=out->records.find(key);
        const bool present=old!=out->records.end();
        // "mark"：记录已存在时只在新的 MarkDirty 后重读。记录缺失时（会话开始、换局、参与者身体或实体
        // 重新出现）补读一次初值，否则客机在下一次 MarkDirty 前一直拿不到值；同一标记下读失败则等下一次标记。
        if(transition||(item.rate=="low"&&present&&frame%3)||
           (item.rate=="mark"&&((!fresh&&failed==mark)||(present&&(!mark||consumed==mark)))))return true;
        auto raw=std::make_shared<state::ModRawValue>();raw->layout=item.layout;raw->mark=mark;raw->sample=sample;
        // 上限也覆盖最坏的 map 项和实体补丁索引，完成后改按实际保留容量记账。
        raw->memory=memory::Reserve(memory::Pool::Values,sizeof(*raw)+32ull*item.layout->nodes[0].worst,"mod_capture_value");
        CaptureValue args{item,mod,*raw,player,context,entity,target};
        const auto start=Clock::now();
        std::set<std::string> ignored;
        if(Settings.diagnostics&&!item.inspected)args.ignored=&ignored;
        const bool ok=ModCall(registration.vm,CaptureValueThunk,&args,dir,item.name.c_str(),"read",item.read_failures);
        if(ok&&args.ignored){
          item.inspected=true;
          for(const auto &key:ignored)Trace("mod_state","phase=ignored_field mod=%s key=%s field=%s",
            dir.c_str(),item.name.c_str(),key.c_str());
        }
        item.read_us+=std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-start).count();
        ++item.read_count;
        if(!ok)return false;
        std::size_t held=sizeof(*raw)+raw->bytes.capacity()+raw->references.capacity()*sizeof(state::ModRawValue::Reference)+
            raw->maps.capacity()*sizeof(state::ModRawValue::Map);
        for(const auto &map:raw->maps)held+=map.items.capacity()*sizeof(state::ModRawValue::Item);
        raw->memory->Resize(held);
        if(old==out->records.end())
          out->memory->Resize(sizeof(*out)+(out->records.size()+1)*96);
        out->records.insert_or_assign(key,std::move(raw));
        item.read_missing+=args.missing;
        if(args.missing && item.read_count<=3)
          Trace("mod_state","phase=read_reference mod=%s key=%s missing=%u",dir.c_str(),item.name.c_str(),args.missing);
        return true;
      };
      if(item.layout->entity){
        const auto prune=[&](auto &marks){for(auto it=marks.begin();it!=marks.end();){
          if(!objects.count(it->first))it=marks.erase(it);else ++it;
        }};
        prune(item.entity_marks);prune(item.entity_failed);
        for(const auto &[id,object]:objects){
          if(object.retained)continue;
          bool matches=true;
          for(unsigned n=0;n<item.shape.size();++n)
            matches&=item.shape[n]==UINT32_MAX||item.shape[n]==object.shape[n];
          if(!matches)continue;
          const state::ModRawKey key{base,id};
          const auto old=out->records.find(key);
          const auto mark_at=item.entity_marks.find(id),failed=item.entity_failed.find(id);
          const auto mark=(std::max)(item.dirty[4],mark_at==item.entity_marks.end()?0:mark_at->second);
          // 实体首次出现时也补读初值；失败按标记记录（含序号 0），避免失败的初读每帧重试。
          if(!capture(key,nullptr,static_cast<Entity *>(object.address),mark,
                      old==out->records.end()?0:old->second->mark,failed==item.entity_failed.end()?kNoMark:failed->second)&&
             item.rate=="mark")
            item.entity_failed.insert_or_assign(id,mark);
        }
      }else for(unsigned target=item.layout->player?0u:4u;target<(item.layout->player?4u:5u);++target){
        auto key=state::ModRawKey{base,{}};
        auto *player=target<4?players[target]:nullptr;
        if(target<4){
          key.record.target=std::uint16_t(target);
          if(!player){out->records.erase(key);continue;}
        }
        const auto mark=item.dirty[target];
        if(!capture(key,player,nullptr,mark,item.committed_mark[target],item.failed_mark[target]))
          item.failed_mark[target]=mark;
      }
    }
    out->memory->Resize(sizeof(*out)+out->records.size()*96);
    return out;
  }catch(...){
    if(++capture_skipped<=3)Trace("mod_state","phase=mod_capture_storage");
    return before;
  }
}

void CommitModWorld(const state::ModRawRef &values) noexcept {
  if(!values||!CurrentModVm())return;
  for(auto &[dir,mod]:registration.registered)for(unsigned i=0;i<mod.declarations.size();++i){
    auto &item=mod.declarations[i];
    if(item.kind!="State"||item.rate!="mark")continue;
    const auto consume=[&](std::uint16_t target,unsigned slot){
      const auto it=values->records.find({{std::uint16_t(mod.ordinal),std::uint16_t(i),target},{}});
      if(it!=values->records.end())item.committed_mark[slot]=it->second->mark;
    };
    if(item.layout->player)for(std::uint16_t n=0;n<4;++n)consume(n,n);
    else consume(state::kModGlobal,4);
  }
}

std::string ModStateFields() {
  std::string out="mod.capture.skipped="+std::to_string(capture_skipped)+"\n"+
    "mod.install.vm_mismatch="+std::to_string(install_vm_mismatches)+"\n";
  const auto add=[&](const std::string &prefix,const char *key,std::uint64_t value){
    out+=prefix+key+"="+std::to_string(value)+"\n";
  };
  for(const auto &[dir,mod]:registration.registered){
    const auto prefix="mod."+std::to_string(mod.ordinal)+".";
    add(prefix,"blocked",mod.blocked);add(prefix,"callback_failures",mod.callback_failures);add(prefix,"reset_failures",mod.reset_failures);
    add(prefix,"removed_unplanned",mod.removed_unplanned);
    add(prefix,"install_batches",mod.batches);add(prefix,"install_max_us",mod.batch_max_us);
    add(prefix,"install_avg_us",mod.batch_avg_us);add(prefix,"baseline_count",mod.baseline_count);
    add(prefix,"baseline_us",mod.baseline_us);
    add(prefix,"decode_rejected",mod.decode_rejected);
    std::uint64_t read_us=0,install_us=0;
    for(unsigned i=0;i<mod.declarations.size();++i){
      const auto &item=mod.declarations[i];
      if(item.kind!="State"&&!item.layout->entity)continue;
      const auto key=prefix+std::to_string(i)+".";
      add(key,"read_count",item.read_count);add(key,"read_us",item.read_us);add(key,"read_failures",item.read_failures);
      add(key,"read_missing",item.read_missing);add(key,"install_count",item.install_count);add(key,"install_us",item.install_us);
      add(key,"install_failures",item.install_failures);add(key,"install_missing",item.install_missing);
      add(key,"default_set_errors",item.default_errors);
      add(key,"custom_set_errors",item.install_failures-item.default_errors);add(key,"drift",item.drift);
      add(key,"diagnostic_errors",item.diagnostic_errors);
      read_us+=item.read_us;install_us+=item.install_us;
    }
    add(prefix,"read_us",read_us);add(prefix,"install_us",install_us);
  }
  return out;
}

void ModInstallBlocked() noexcept {
  if(mod_install_owner)++mod_install_owner->blocked;
}

bool ModWantsEntityLifecycle() noexcept {return CurrentModVm()&&!registration.registered.empty();}
void CountUnplannedModRemoval() noexcept {
  for(auto &[dir,mod]:registration.registered)++mod.removed_unplanned;
}
void NotifyModRemoved(void *address) noexcept {
  if(!address||!ModWantsEntityLifecycle())return;
  ModInstallScope scope;
  if(!scope)return;
  for(auto &[dir,mod]:registration.registered){
    mod_install_owner=&mod;NotifyMod job{mod,"CLIENT_ENTITY_REMOVED"};
    job.entity=static_cast<Entity *>(address);Notify(dir,job);
  }
}

bool InstallModWorld(const state::ModShadowRef &shadow,bool new_world,bool full,const ModClientLookup &lookup,
    void *const *joined,std::size_t joined_count) noexcept {
  if(!new_world&&!mod_scene_started)return true;
  if(!new_world&&!Settings.diagnostics&&std::none_of(installed.begin(),installed.end(),[](const auto &entry){return entry.second.unresolved;}))return true;
  if(!CurrentModVm()){
    if(++install_vm_mismatches<=3)Trace("mod_state","phase=mod_install_vm_mismatch");
    return false;
  }
  // 先装原生查询镜像，使本批的用户 set/通知读取同一份 Host 状态。
  if(new_world&&shadow){
    for(auto &[dir,mod]:registration.registered){
      const auto found=shadow->rejected_by_mod.find(std::uint16_t(mod.ordinal));
      mod.decode_rejected=found==shadow->rejected_by_mod.end()?0:found->second;
    }
    for(const auto &[key,record]:shadow->records){
      if(key.mod!=kNativeMod||!record.value||record.value->bytes.size()<2)continue;
      bool known=false;std::uint64_t instance=0;
      const auto row=static_cast<NativeModRow>(key.declaration);
      auto *object=row==NativeModRow::Player&&lookup.entity?lookup.entity(lookup.context,key.target,known,instance):nullptr;
      if(row==NativeModRow::Player&&!object)continue;
      try {InstallNativeModRow(row,object,record.value);}
      catch(...){if(Settings.diagnostics)Trace("mod_state","phase=native_install_failed row=%u",unsigned(row));}
    }
    EndNativeModPlayers();
  }
  if(registration.registered.empty())return true;
  ModInstallScope scope;
  if(!scope)return false;
  // 加入名单派发完成后置位；之后的 CHANGED、诊断等步骤失败不影响成员表推进，避免重复 INSTALLED。
  bool delivered=false;
  try {
    using Clock=std::chrono::steady_clock;
    struct Changed {const std::string *dir;Registration *mod;Declaration *item;unsigned target;std::uint64_t instance;};
    const auto count=installed.size()+(shadow?shadow->records.size():0);
    if(!installed_memory)installed_memory=memory::Reserve(memory::Pool::Values,0,"mod_installed");
    installed_memory->Resize(count*128);
    auto work=memory::Reserve(memory::Pool::Workspace,count*sizeof(Changed),"mod_install_changes");
    std::vector<Changed> changed;
    changed.reserve(count);
    std::map<Registration *,std::uint64_t> durations;
    const auto write=[&](const std::string &dir,Registration &mod,Declaration &item,
                         state::ModRecordKey key,const state::ModValueRef &value,Entity_Player *player,
                         const state::ModEntityId &identity={},Entity *entity=nullptr,std::uint64_t instance=0){
      auto found=installed.find(key);
      if(found==installed.end()&&!value)return;
      if(found==installed.end())found=installed.try_emplace(key).first;
      auto &old=found->second;
      if(!(old.entity==identity)){old={};old.entity=identity;}
      if(!new_world&&!old.unresolved)return;
      if(item.layout->entity&&!entity){old.unresolved=true;return;}
      const bool different=!state::ModSame(value,old.value);
      if(!full){
        if(value&&old.failed&&instance==old.failed_instance&&state::ModSame(value,old.failed)){
          old.unresolved=false;return;
        }
        if(!value&&old.failed_delete)return;
        if(!different&&!old.unresolved&&old.instance==instance){old.value=value;return;}
      }
      InstallValue job{item,mod,value.get(),lookup,player,entity};
      const auto start=Clock::now();
      mod_install_owner=&mod;
      const bool ok=ModCall(registration.vm,InstallValueThunk,&job,dir,item.name.c_str(),
                            item.set?"set":"default_set",item.install_failures);
      const auto elapsed=std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-start).count());
      item.install_us+=elapsed;durations[&mod]+=elapsed;
      if(full){++mod.baseline_count;mod.baseline_us+=elapsed;}
      if(!ok){if(!item.set)++item.default_errors;old.failed=value;old.failed_instance=instance;old.failed_delete=!value;old.unresolved=false;return;}
      ++item.install_count;
      item.install_missing+=job.unresolved;
      if(different)changed.push_back({&dir,&mod,&item,key.target,instance});
      if(value)old={value,{},job.unresolved,false,identity,instance};
      else installed.erase(found);
    };
    if(new_world)for(auto it=installed.begin();it!=installed.end();){
      const auto target=it->first.target;
      bool gone=false;
      if(it->second.entity.id){
        const auto found=shadow?shadow->records.find(it->first):std::map<state::ModRecordKey,state::ModRecord>::const_iterator{};
        gone=!shadow||found==shadow->records.end()||!(found->second.entity==it->second.entity);
      }else gone=target<4&&!(lookup.players&(1u<<target));
      if(gone)it=installed.erase(it);
      else ++it;
    }
    for(const bool player_scope:{false,true})for(auto &[dir,mod]:registration.registered)
      for(unsigned i=0;i<mod.declarations.size();++i){
        auto &item=mod.declarations[i];
        if(item.kind!="State"||item.layout->player!=player_scope)continue;
        for(unsigned target=player_scope?0u:4u;target<(player_scope?4u:5u);++target){
          auto *player=player_scope?FindPlayer(target):nullptr;
          if(player_scope&&!player)continue;
          const state::ModRecordKey key{std::uint16_t(mod.ordinal),std::uint16_t(i),player_scope?std::uint16_t(target):state::kModGlobal};
          state::ModValueRef value;
          if(shadow){
            const auto record=shadow->records.find(key);
            if(record!=shadow->records.end())value=record->second.value;
          }
          write(dir,mod,item,key,value,player);
        }
      }
    if(shadow&&lookup.entity)for(auto &[dir,mod]:registration.registered)
      for(unsigned i=0;i<mod.declarations.size();++i){
        auto &item=mod.declarations[i];
        if(!item.layout->entity)continue;
        const state::ModRecordKey first{std::uint16_t(mod.ordinal),std::uint16_t(i),0};
        for(auto at=shadow->records.lower_bound(first);at!=shadow->records.end()&&
            at->first.mod==first.mod&&at->first.declaration==first.declaration;++at){
          bool known=false;std::uint64_t instance=0;
          auto *entity=static_cast<Entity *>(lookup.entity(lookup.context,at->first.target,known,instance));
          if(known)write(dir,mod,item,at->first,at->second.value,nullptr,at->second.entity,entity,instance);
        }
      }
    for(std::size_t i=0;i<joined_count;++i)for(auto &[dir,mod]:registration.registered){
      mod_install_owner=&mod;NotifyMod job{mod,"CLIENT_ENTITY_INSTALLED"};
      job.entity=static_cast<Entity *>(joined[i]);Notify(dir,job);
    }
    delivered=true;
    for(const auto &entry:changed){
      mod_install_owner=entry.mod;
      NotifyMod job{*entry.mod,"CLIENT_STATE_CHANGED",entry.item,0,0,FindPlayer(entry.target)};
      if(entry.item->layout->entity){
        bool known=false;std::uint64_t instance=0;
        job.entity=static_cast<Entity *>(lookup.entity(lookup.context,std::uint16_t(entry.target),known,instance));
        if(!job.entity||instance!=entry.instance)continue;
      }
      Notify(*entry.dir,job);
    }
    if(new_world&&!mod_scene_started){
      mod_scene_started=true;
      for(auto &[dir,mod]:registration.registered){
        mod_install_owner=&mod;NotifyMod job{mod,"CLIENT_SCENE_STARTED"};Notify(dir,job);
      }
    }
    for(const auto &[mod,us]:durations){
      mod->batch_max_us=(std::max)(mod->batch_max_us,us);
      mod->batch_avg_us=mod->batches?(7*mod->batch_avg_us+us)/8:us;
      ++mod->batches;
    }
    // 只重写本机已经成功安装的值，不推进影子、版本或 CHANGED。
    if(Settings.diagnostics)for(auto &[dir,mod]:registration.registered){
      for(const auto &[key,old]:installed){
        if(key.mod!=mod.ordinal||key.declaration>=mod.declarations.size()||!old.value)continue;
        auto &item=mod.declarations[key.declaration];
        Entity_Player *player=item.layout->player?FindPlayer(key.target):nullptr;
        if(item.layout->player&&!player)continue;
        Entity *entity=nullptr;
        if(item.layout->entity){
          bool known=false;std::uint64_t instance=0;
          entity=lookup.entity?static_cast<Entity *>(lookup.entity(lookup.context,key.target,known,instance)):nullptr;
          if(!entity||instance!=old.instance)continue;
        }
        mod_install_owner=&mod;
        std::vector<std::byte> before,after;
        const auto inspect=[&](std::vector<std::byte> &bytes){
          state::ModRawValue raw;raw.layout=item.layout;
          CaptureValue capture{item,mod,raw,player,nullptr,
            [](void *,const void *entity,std::uint64_t &id,std::uint64_t &use) noexcept{
              const auto number=ClientModIntentMember(entity);
              if(number==state::kModGlobal)return false;
              id=std::uint64_t(number)+1;use=1;return true;
            },entity};
          if(!ModCall(registration.vm,CaptureValueThunk,&capture,dir,item.name.c_str(),"diagnostic_read",item.diagnostic_errors))return false;
          std::uint64_t missing=0;
          state::CanonicalModValue(raw,bytes,nullptr,[](void *,state::ModEntityId id) noexcept{
            return std::uint16_t(id.id-1);
          },missing);
          return true;
        };
        const bool had_value=inspect(before);
        InstallValue job{item,mod,old.value.get(),lookup,player,entity};
        if(ModCall(registration.vm,InstallValueThunk,&job,dir,item.name.c_str(),"diagnostic_write",item.diagnostic_errors)
            &&had_value&&inspect(after)&&before!=after){
          ++item.drift;
          if(item.drift<=3)Trace("mod_state","phase=client_key_changed mod=%s key=%s target=%u",
            dir.c_str(),item.name.c_str(),unsigned(key.target));
        }
      }
    }
    installed_memory->Resize(installed.size()*128);
  }catch(...){Trace("mod_state","phase=mod_install_storage");}
  return delivered;
}

void RetireModWorld() noexcept {
  if(!mod_scene_started&&installed.empty()){
    RetireNativeModState();
    return;
  }
  if(CurrentModVm()){
    ModInstallScope scope;
    if(scope){
      if(mod_scene_started)for(auto &[dir,mod]:registration.registered){
        mod_install_owner=&mod;NotifyMod job{mod,"CLIENT_SCENE_ENDED"};Notify(dir,job);
      }
      for(auto &[dir,mod]:registration.registered)for(auto &item:mod.declarations)
        if((item.kind=="State"||item.layout->entity)&&item.reset){
          mod_install_owner=&mod;
          ModCall(registration.vm,ResetModThunk,&item.reset,dir,item.name.c_str(),"reset",mod.reset_failures);
        }
    }
  }else {
    ++install_vm_mismatches;
    Trace("mod_state","phase=mod_retire_vm_mismatch");
  }
  installed.clear();installed_memory.reset();mod_scene_started=false;
  RetireNativeModState();
}

void ModApiBeforeManager() noexcept {
  if(!HostBusy())CancelModIntents(false);
  if (!registration.reload_pending) return;
  registration.reload_pending=false;
  if (ClientBusy()) ClientCallbackFault("mod_lua_reloaded");
  if (HostBusy()) RejectHostModReload();
}

void ModApiConsoleCommand(const char *text) noexcept {
  if (!IsMain() || !text || (!ClientBusy() && !HostBusy())) return;
  std::string_view command(text);
  const auto first=command.find_first_not_of(" \t");
  if (first==command.npos) return;
  command.remove_prefix(first);
  command=command.substr(0,command.find_first_of(" \t"));
  constexpr std::string_view reload="luamod";
  if (command.size()!=reload.size()) return;
  for (unsigned i=0;i<reload.size();++i)
    if (std::tolower(static_cast<unsigned char>(command[i]))!=reload[i]) return;
  registration.reload_pending=true;
}

int ModApiCallbackId(lua_State *L) noexcept {
  if (lua_type(L,1)!=LUA_TNUMBER) return -1;
  int integer=0;
  const auto id=lua_tointegerx(L,1,&integer);
  return integer && id>=0 && id<=(std::numeric_limits<int>::max)() ? static_cast<int>(id) : -1;
}

bool ModApiCallbackAllowedInPhase(int callback) noexcept {
  if (!IsMain()) return false;
  switch (callback) {
  // 原版和 REPENTOGON 1.1.3 已核对的原生显示回调，按固定 ID 放行。
  case 2: // MC_POST_RENDER
  case 21: // MC_GET_SHADER_PARAMS
  case 25: // MC_POST_FAMILIAR_RENDER
  case 28: // MC_POST_NPC_RENDER
  case 32: // MC_POST_PLAYER_RENDER
  case 36: // MC_POST_PICKUP_RENDER
  case 41: // MC_POST_TEAR_RENDER
  case 45: // MC_POST_PROJECTILE_RENDER
  case 49: // MC_POST_LASER_RENDER
  case 52: // MC_POST_KNIFE_RENDER
  case 56: // MC_POST_EFFECT_RENDER
  case 59: // MC_POST_BOMB_RENDER
  case 1022: // MC_HUD_RENDER
  case 1024: // MC_POST_HUD_RENDER
  case 1027: // MC_PRE_HISTORYHUD_RENDER
  case 1028: // MC_POST_HISTORYHUD_RENDER
  case 1038: // MC_PRE_RENDER_PLAYER_HEAD
  case 1039: // MC_PRE_RENDER_PLAYER_BODY
  case 1044: // MC_POST_ROOM_RENDER_ENTITIES
  case 1045: // MC_POST_RENDER_PLAYER_HEAD
  case 1046: // MC_POST_RENDER_PLAYER_BODY
  case 1079: // MC_POST_PLAYERHUD_RENDER_ACTIVE_ITEM
  case 1080: // MC_PRE_FAMILIAR_RENDER
  case 1081: // MC_PRE_NPC_RENDER
  case 1082: // MC_PRE_PLAYER_RENDER
  case 1083: // MC_PRE_PICKUP_RENDER
  case 1084: // MC_PRE_TEAR_RENDER
  case 1085: // MC_PRE_PROJECTILE_RENDER
  case 1086: // MC_PRE_KNIFE_RENDER
  case 1087: // MC_PRE_EFFECT_RENDER
  case 1088: // MC_PRE_BOMB_RENDER
  case 1089: // MC_PRE_SLOT_RENDER
  case 1090: // MC_POST_SLOT_RENDER
  case 1091: // MC_POST_PLAYERHUD_RENDER_HEARTS
  case 1102: // MC_POST_NIGHTMARE_SCENE_RENDER
  case 1106: // MC_PRE_BACKDROP_RENDER_WALLS
  case 1107: // MC_PRE_BACKDROP_RENDER_FLOOR
  case 1108: // MC_PRE_BACKDROP_RENDER_WATER
  case 1109: // MC_POST_BACKDROP_PRE_RENDER_WALLS
  case 1118: // MC_PRE_PLAYERHUD_RENDER_HEARTS
  case 1119: // MC_PRE_PLAYERHUD_RENDER_ACTIVE_ITEM
  case 1135: // MC_PRE_RENDER
  case 1139: // MC_POST_ITEM_OVERLAY_RENDER
  case 1150: // MC_PRE_GET_LIGHTING_ALPHA
  case 1151: // MC_PRE_RENDER_GRID_LIGHTING
  case 1152: // MC_PRE_RENDER_ENTITY_LIGHTING
  case 1264: // MC_PRE_PLAYERHUD_TRINKET_RENDER
  case 1272: // MC_POST_ROOM_TRANSITION_RENDER
  case 1293: // MC_PRE_PLAYERHUD_RENDER_INVENTORY
  case 1294: // MC_POST_PLAYERHUD_RENDER_INVENTORY
  case 1295: // MC_PRE_PLAYERHUD_RENDER_POOP_SPELL_QUEUE
  case 1296: // MC_POST_PLAYERHUD_RENDER_POOP_SPELL_QUEUE
  case 1297: // MC_PRE_PLAYERHUD_RENDER_CRAFTING_TABLE
  case 1298: // MC_POST_PLAYERHUD_RENDER_CRAFTING_TABLE
  case 1432: // MC_PRE_GRID_ENTITY_SPIKES_RENDER
  case 1433: // MC_POST_GRID_ENTITY_SPIKES_RENDER
  case 1434: // MC_PRE_GRID_ENTITY_WEB_RENDER
  case 1435: // MC_POST_GRID_ENTITY_WEB_RENDER
  case 1436: // MC_PRE_GRID_ENTITY_TNT_RENDER
  case 1437: // MC_POST_GRID_ENTITY_TNT_RENDER
  case 1438: // MC_PRE_GRID_ENTITY_TRAPDOOR_RENDER
  case 1439: // MC_POST_GRID_ENTITY_TRAPDOOR_RENDER
  case 1440: // MC_PRE_GRID_ENTITY_STAIRCASE_RENDER
  case 1441: // MC_POST_GRID_ENTITY_STAIRCASE_RENDER
  case 1444: // MC_PRE_GRID_ENTITY_DECORATION_RENDER
  case 1445: // MC_POST_GRID_ENTITY_DECORATION_RENDER
  case 1446: // MC_PRE_GRID_ENTITY_DOOR_RENDER
  case 1447: // MC_POST_GRID_ENTITY_DOOR_RENDER
  case 1448: // MC_PRE_GRID_ENTITY_FIRE_RENDER
  case 1449: // MC_POST_GRID_ENTITY_FIRE_RENDER
  case 1450: // MC_PRE_GRID_ENTITY_LOCK_RENDER
  case 1451: // MC_POST_GRID_ENTITY_LOCK_RENDER
  case 1452: // MC_PRE_GRID_ENTITY_TELEPORTER_RENDER
  case 1453: // MC_POST_GRID_ENTITY_TELEPORTER_RENDER
  case 1454: // MC_PRE_GRID_ENTITY_PIT_RENDER
  case 1455: // MC_POST_GRID_ENTITY_PIT_RENDER
  case 1456: // MC_PRE_GRID_ENTITY_POOP_RENDER
  case 1457: // MC_POST_GRID_ENTITY_POOP_RENDER
  case 1458: // MC_PRE_GRID_ENTITY_ROCK_RENDER
  case 1459: // MC_POST_GRID_ENTITY_ROCK_RENDER
  case 1460: // MC_PRE_GRID_ENTITY_PRESSUREPLATE_RENDER
  case 1461: // MC_POST_GRID_ENTITY_PRESSUREPLATE_RENDER
  case 1462: // MC_PRE_GRID_ENTITY_WALL_RENDER
  case 1463: // MC_POST_GRID_ENTITY_WALL_RENDER
  case 1479: // MC_PRE_MINIMAP_RENDER
  case 1480: // MC_POST_MINIMAP_RENDER
    return ClientNativeDisplayCallbacks();
  // 分别跟随本机 HUD 的原生维护入口，1021 同时覆盖旧别名 MC_HUD_POST_UPDATE。
  case 1020: // MC_HUD_UPDATE
    return ClientHudUpdateCallbacks();
  case 1021: // MC_POST_HUD_UPDATE
  case 1029: // MC_POST_HISTORYHUD_RECOMPUTE
    return ClientHudPostUpdateCallbacks();
  // 本地菜单输入只有原生输入生产点能授予资格。
  case 1464: // MC_MENU_INPUT_ACTION
    return menu_input && ClientCallbackInStableMenu();
  case kModMainMenuRender: // MC_MAIN_MENU_RENDER
  case 1329: // MC_PRE_RENDER_CHARACTER_SELECT_PAGE
  case 1330: // MC_POST_RENDER_CHARACTER_SELECT_PAGE
  case 1331: // MC_PRE_RENDER_CHARACTER_SELECT_PORTRAIT
  case 1332: // MC_POST_RENDER_CHARACTER_SELECT_PORTRAIT
  case 1333: // MC_PRE_RENDER_CUSTOM_CHARACTER_MENU
    return ClientCallbackMenuAllowed();
  case 1216: // MC_PRE_COMPLETION_MARKS_RENDER
  case 1217: // MC_POST_COMPLETION_MARKS_RENDER
    return ClientCallbackMenuAllowed() || ClientPauseRenderCallbacks();
  case 1218: // MC_PRE_PAUSE_SCREEN_RENDER
  case 1219: // MC_POST_PAUSE_SCREEN_RENDER
    return ClientPauseRenderCallbacks();
  case 1030: // MC_PRE_SFX_PLAY
  case 1031: // MC_POST_SFX_PLAY
  case 1034: // MC_PRE_MUSIC_PLAY
  case 1035: // MC_PRE_MUSIC_LAYER_TOGGLE
  case 1094: // MC_PRE_MUSIC_PLAY_JINGLE
    return !replay_audio;
  default:
    // 玩法、权威查询、已停用的 OpenGL 回调以及未知 ID 均不在客机执行。
    return false;
  }
}

ModApiMenuInputScope::ModApiMenuInputScope(bool main) noexcept : previous_(menu_input) {
  menu_input=main && ClientCallbackInStableMenu() && !Isaac::IsInGame();
}
ModApiMenuInputScope::~ModApiMenuInputScope() noexcept {menu_input=previous_;}

ModApiReplayAudio::ModApiReplayAudio(bool active) noexcept : active_(active) {
  if (active_) ++replay_audio;
}
ModApiReplayAudio::~ModApiReplayAudio() noexcept {
  if (active_) --replay_audio;
}
}
