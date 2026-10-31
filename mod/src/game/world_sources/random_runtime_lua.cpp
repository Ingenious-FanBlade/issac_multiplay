#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/world_sources/random_sources.h"
#include "hooks/random_runtime.h"
#include "runtime.h"
#include "hde.h"
#include <set>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <cmath>
namespace cs::game {
namespace {
struct RuntimeState final {void *state=nullptr;std::uint32_t words=0;};
std::array<RuntimeState,5> states{};
struct LuaRoot final {void *value;const void *registry;int reference;};
std::vector<LuaRoot> lua_roots;
void PinLuaRandomState(lua_State *L,int index){
  if(lua_type(L,index)!=LUA_TUSERDATA||lua_rawlen(L,index)!=32)return;
  auto *p=lua_touserdata(L,index);const auto registry=lua_topointer(L,LUA_REGISTRYINDEX);
  for(const auto &root:lua_roots)if(root.value==p&&root.registry==registry)return;
  if(lua_roots.size()>=128)return;
  lua_pushvalue(L,index);const auto ref=luaL_ref(L,LUA_REGISTRYINDEX);lua_roots.push_back({p,registry,ref});
}
void *(__cdecl *get_ptd)()=nullptr;
std::uint32_t rand_offset=0;
bool rand_layout=false;
std::uint64_t lua_sequence=0;
struct LuaTicket final {bool active=false;std::uint64_t id=0;RandomWords before;std::string arguments,stream;};
std::array<LuaTicket,5> pending{};
bool Copy(const void *p,void *out,std::size_t bytes) noexcept {
  SIZE_T copied=0;return p&&ReadProcessMemory(GetCurrentProcess(),p,out,bytes,&copied)&&copied==bytes;
}
void *Follow(void *pointer) noexcept {
  auto p=reinterpret_cast<std::uintptr_t>(pointer);
  for(unsigned i=0;i<4;++i){unsigned char b[6]{};if(!Copy(reinterpret_cast<void *>(p),b,6))return nullptr;
    if(b[0]==0xe9){std::int32_t delta=0;std::memcpy(&delta,b+1,4);p=p+5+delta;}
    else if(b[0]==0xff&&b[1]==0x25){std::uint32_t slot=0;std::memcpy(&slot,b+2,4);if(!Copy(reinterpret_cast<void *>(slot),&p,4))return nullptr;}
    else break;
  }return reinterpret_cast<void *>(p);
}
RandomAlgorithm Kind(unsigned source){return source==2?RandomAlgorithm::MsvcMT19937:RandomAlgorithm::Xoshiro256;}
const char *Member(unsigned source){return source==2?"C::RandomFloat":source==3?"C::math.random":"C::math.randomseed";}
void BindRuntime(unsigned source,bool observed_return=false){
  if(source==0)BindRandomSource({nullptr,RandomAlgorithm::CrtRand,"{\"kind\":\"CRT-main-thread\",\"source\":\"ucrtbase.rand\"}","CRT.rand/srand","process/main-thread"},observed_return);
  else if(states[source].state)BindRandomSource({states[source].state,Kind(source),
    source==2?"{\"kind\":\"REPENTOGON-global\",\"member\":\"RandomFloat\"}":"{\"kind\":\"Lua-math-random-upvalue\"}",source==2?"RandomFloat":"math.random/randomseed","Lua-state/process"},observed_return);
}
// 用解码后的实际静态地址引用定位MSVC引擎；要求索引与尾部掩码符合已知布局，歧义明确失败。
void FindMsvcEngine(void *fn){
  std::set<std::uintptr_t> candidates,visited;std::vector<std::pair<std::uintptr_t,unsigned>> work{{reinterpret_cast<std::uintptr_t>(fn),0}};
  HMODULE module=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(fn),&module);
  for(std::size_t wi=0;wi<work.size()&&wi<24;++wi){
    const auto start=work[wi].first;const auto depth=work[wi].second;if(!visited.insert(start).second)continue;
    for(unsigned offset=0;offset<384;){
      unsigned char bytes[16]{};if(!Copy(reinterpret_cast<void *>(start+offset),bytes,16))break;hde32s h{};const auto n=hde32_disasm(bytes,&h);if(!n||h.flags&F_ERROR)break;
      const auto add=[&](std::uintptr_t p){
        for(unsigned shift=0;shift<=4;shift+=4){
          if(p<shift+4)continue;const auto base=p-shift;MEMORY_BASIC_INFORMATION memory{};
          if(!VirtualQuery(reinterpret_cast<void *>(base),&memory,sizeof(memory))||memory.AllocationBase!=module||!(memory.Protect&(PAGE_READWRITE|PAGE_WRITECOPY)))continue;
          std::uint32_t idx=0,mask=0;if(Copy(reinterpret_cast<void *>(base),&idx,4)&&Copy(reinterpret_cast<void *>(base+4996),&mask,4)&&idx<=1248&&mask==0xffffffff)candidates.insert(base);
        }
      };
      if(h.flags&F_IMM32&&!(h.flags&F_RELATIVE))add(h.imm.imm32);
      if(h.flags&F_DISP32&&h.modrm_mod==0&&h.modrm_rm==5)add(h.disp.disp32);
      if(h.opcode==0xe8&&depth<3){
        const auto target=start+offset+n+std::int32_t(h.imm.imm32);MEMORY_BASIC_INFORMATION m{};
        if(VirtualQuery(reinterpret_cast<void *>(target),&m,sizeof(m))&&m.AllocationBase==module)work.push_back({target,depth+1});
      }
      offset+=n;if(h.opcode==0xc3||h.opcode==0xc2)break;
    }
  }
  if(candidates.size()==1)states[2]={reinterpret_cast<void *>(*candidates.begin()),1250};
  Trace("random_runtime_layout","source=2 candidates=%u ready=%u",unsigned(candidates.size()),unsigned(states[2].state!=nullptr));
}
}
void SetRuntimeRandomSource(unsigned source,void *state,std::uint32_t words) noexcept {if(source<states.size())states[source]={state,words};}
void PrepareRuntimeRandomHook(unsigned source,void *target) noexcept {
  if(!IsMain())return;
  try{
    if(source==2){FindMsvcEngine(target);return;}
    if(source!=0)return;
    const auto start=reinterpret_cast<std::uintptr_t>(Follow(target));if(!start)return;
    std::uintptr_t getter=0;std::uint32_t offset=0;bool found=false,add=false;
    for(unsigned i=0;i<128;){
      unsigned char bytes[16]{};if(!Copy(reinterpret_cast<void *>(start+i),bytes,16))break;hde32s h{};const auto n=hde32_disasm(bytes,&h);if(!n||h.flags&F_ERROR)break;
      if(h.opcode==0xe8&&!getter)getter=start+i+n+std::int32_t(h.imm.imm32);
      if(h.opcode==0x69&&h.imm.imm32==214013&&h.modrm_mod!=3){
        offset=h.modrm_mod==1?h.disp.disp8:h.disp.disp32;found=offset<8192;
      }
      if((h.flags&F_IMM32)&&h.imm.imm32==2531011)add=true;
      i+=n;if(h.opcode==0xc3||h.opcode==0xc2)break;
    }
    if(getter&&found&&add){get_ptd=reinterpret_cast<void *(__cdecl *)()>(getter);rand_offset=offset;rand_layout=true;}
    Trace("random_runtime_layout","source=0 ready=%u state_offset=%u",unsigned(rand_layout),rand_offset);
  }catch(...){Trace("random_runtime_layout","source=%u reason=exception",source);}
}
RandomWords CaptureRuntimeRandomWords(unsigned source) noexcept {
  RandomWords s;if(!IsMain())return s;
  try{
    if(source<=1){
      if(!rand_layout||!get_ptd)return s;
      // 游戏主线程已进入CRT；这里只取得既有PTD，不调用rand/srand或补种。
      auto *ptd=static_cast<unsigned char *>(get_ptd());if(!ptd)return s;
      s.words.resize(1);s.readable=Copy(ptd+rand_offset,s.words.data(),4);
    }else if(source<states.size()&&states[source].state){
      s.words.resize(states[source].words);s.readable=Copy(states[source].state,s.words.data(),s.words.size()*4);
    }
    if(!s.readable)s.words.clear();
  }catch(...){s.readable=false;s.words.clear();}return s;
}
void RegisterRuntimeRandomFunctions() noexcept {
  if(!IsMain()||!g_LuaEngine||!g_LuaEngine->_state)return;
  InitializeRuntimeRandomHooks();
  auto *L=g_LuaEngine->_state;const auto top=lua_gettop(L);
  try{
    for(unsigned source=2;source<=4;++source){
      if(source==2)lua_getglobal(L,"RandomFloat");else{lua_getglobal(L,"math");lua_getfield(L,-1,source==3?"random":"randomseed");}
      if(lua_iscfunction(L,-1)){
        const auto fn=lua_tocfunction(L,-1);
        if(source>=3){
          if(lua_getupvalue(L,-1,1)){if(lua_type(L,-1)==LUA_TUSERDATA&&lua_rawlen(L,-1)==32){states[source]={lua_touserdata(L,-1),8};PinLuaRandomState(L,-1);}lua_pop(L,1);}
        }
        InstallRuntimeRandomHook(source,reinterpret_cast<void *>(fn));
      }lua_settop(L,top);
    }
  }catch(...){Trace("random_runtime_register","reason=exception");}
  lua_settop(L,top);
}
void CaptureRuntimeRandomOwners() noexcept {
  if(!RandomSourcesNeeded())return;
  for(unsigned s:{0u,2u})BindRuntime(s);
  if(!g_LuaEngine||!g_LuaEngine->_state)return;
  const auto registry=lua_topointer(g_LuaEngine->_state,LUA_REGISTRYINDEX);
  for(const auto &root:lua_roots)if(root.registry==registry)BindRandomSource({root.value,RandomAlgorithm::Xoshiro256,
    "{\"kind\":\"Lua-math-random-upvalue;registry-pinned\"}","math.random/randomseed","Lua-state"});
}
bool RuntimeRandomSourcesComplete() noexcept{
  if(!IsMain())return false;
  for(unsigned s=0;s<5;++s)if(!RuntimeRandomHookInstalled(s))return false;
  for(unsigned s:{0u,2u,3u,4u})if(!CaptureRuntimeRandomWords(s).readable)return false;
  return true;
}
std::string RuntimeRandomDiagnostics() noexcept{
  try{std::ostringstream o;o<<"{\"sources\":[";for(unsigned i=0;i<5;++i){if(i)o<<',';const auto s=CaptureRuntimeRandomWords(i);o<<"{\"source\":"<<i<<",\"installed\":"<<(RuntimeRandomHookInstalled(i)?"true":"false")<<",\"readable\":"<<(s.readable?"true":"false")<<",\"words\":"<<s.words.size()<<",\"import_module_mask\":"<<RuntimeRandomImportMask(i)<<'}';}o<<"]}";return o.str();}catch(...){return "{}";}
}
std::string RuntimeRandomArguments(void *lua) noexcept {
  try{
    auto *L=static_cast<lua_State *>(lua);std::ostringstream o;o<<std::setprecision(17)<<'[';
    for(int i=1;i<=lua_gettop(L);++i){if(i>1)o<<',';if(lua_isinteger(L,i))o<<"{\"integer\":\""<<lua_tointeger(L,i)<<"\"}";else if(lua_type(L,i)==LUA_TNUMBER){const auto n=lua_tonumber(L,i);if(std::isfinite(n))o<<n;else o<<"{\"status\":\"Nonfinite\"}";}else if(lua_isnil(L,i))o<<"null";else o<<"{\"type\":"<<lua_type(L,i)<<'}';}o<<']';return o.str();
  }catch(...){return "[]";}
}
std::string RuntimeRandomResults(void *lua,int count) noexcept{
  try{
    auto *L=static_cast<lua_State *>(lua);std::ostringstream o;o<<std::setprecision(17)<<'[';const int top=lua_gettop(L);
    for(int i=top-count+1;i<=top;++i){if(i>top-count+1)o<<',';if(lua_isinteger(L,i))o<<"{\"integer\":\""<<lua_tointeger(L,i)<<"\"}";else if(lua_type(L,i)==LUA_TNUMBER){const auto n=lua_tonumber(L,i);if(std::isfinite(n))o<<n;else o<<"{\"status\":\"Nonfinite\"}";}else if(lua_isnil(L,i))o<<"null";else o<<"{\"type\":"<<lua_type(L,i)<<'}';}o<<']';return o.str();
  }catch(...){return "[]";}
}
std::uint64_t NativeRandomLuaEnter(unsigned source,void *lua) noexcept{
  if(source<2||source>4||!RandomSourcesNeeded())return 0;
  if(pending[source].active)NativeRandomLuaAbortPending();
  try{
    if(source>=3){
      auto *L=static_cast<lua_State *>(lua);const auto index=lua_upvalueindex(1);
      if(lua_type(L,index)==LUA_TUSERDATA&&lua_rawlen(L,index)==32){states[source]={lua_touserdata(L,index),8};PinLuaRandomState(L,index);}
    }
    BindRuntime(source);auto &t=pending[source];t.before=CaptureRuntimeRandomWords(source);t.arguments=RuntimeRandomArguments(lua);
    t.stream=RandomSourceReference(states[source].state);t.id=++lua_sequence;t.active=true;return t.id;
  }catch(...){return 0;}
}
void NativeRandomLuaReturned(unsigned source,void *lua,int count) noexcept{
  if(source>=pending.size()||!pending[source].active)return;
  auto &t=pending[source];t.active=false;
  try{
    const auto after=CaptureRuntimeRandomWords(source);
    std::ostringstream o;o<<"{\"member\":\""<<Member(source)<<"\",\"source\":"<<source<<",\"call\":\""<<t.id<<"\",\"stream_reference\":"<<t.stream
      <<",\"arguments\":"<<t.arguments<<",\"returns\":"<<RuntimeRandomResults(lua,count)<<",\"before\":"<<EncodeRandomWords(t.before,Kind(source))
      <<",\"after\":"<<EncodeRandomWords(after,Kind(source))<<",\"completion\":\"Returned\"}";
    PublishRandomObservation("isaac.cs.random.runtime-call",o.str(),t.before.readable&&after.readable,unsigned(t.before.words.size()+after.words.size()));BindRuntime(source,true);
  }catch(...){}
}
void NativeRandomLuaAbortPending() noexcept{
  if(!IsMain())return;
  for(unsigned source=2;source<=4;++source)if(pending[source].active){
    auto &t=pending[source];t.active=false;
    try{
      const auto after=CaptureRuntimeRandomWords(source);
      PublishRandomObservation("isaac.cs.random.runtime-call","{\"member\":\""+std::string(Member(source))+"\",\"call\":\""+std::to_string(t.id)+
        "\",\"completion\":\"NonLocalLuaExit;return-unavailable\",\"before\":"+EncodeRandomWords(t.before,Kind(source))+",\"after\":"+EncodeRandomWords(after,Kind(source))+"}",false,unsigned(after.words.size()));
      BindRuntime(source,true);
    }catch(...){}
  }
}
}
