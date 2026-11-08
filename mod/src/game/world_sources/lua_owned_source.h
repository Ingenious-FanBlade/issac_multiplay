#pragma once
#include "core/owned_source.h"
#include "core/capture_cost.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace cs::game {
inline constexpr const char *kOwnedBlockMetatable="IsaacCS.CaptureOwnedBlock.v1";
struct LuaOwnedBlock final {capture::Source source;};
// 只接受read.Value已复制的标量/表；raw接口不执行__index、__pairs或任何原生getter。
// 借用字符串在当前栈窗口复制，真实空数组由来源标记区分；环必须已经编码为具名graph_ref。
inline capture::Source CopyLuaSource(lua_State *L,int index,std::uint32_t node_limit,std::uint64_t byte_limit){
  capture::Cost cost(capture::CostPart::Copy);
  index=lua_absindex(L,index);
  return capture::OwnedSource::CreateGraph([&](replica::Json &out,std::uint64_t &bytes,std::uint64_t &nodes,auto &blocks){
    std::array<const void *,49> ancestors{};
    const auto add=[&](std::uint64_t n){if(n>byte_limit-bytes)throw std::runtime_error("lua_owned_byte_budget");bytes+=n;};
    const auto quoted=[&](const char *s,std::size_t size){std::uint64_t count=2;for(std::size_t i=0;i<size;++i){const auto c=static_cast<unsigned char>(s[i]);count+=c<32||c=='\\'||c=='"'?6:1;}add(count);};
    // 通过Document的SAX构建栈一次取得准确容器容量，避免每个小对象默认预留16个成员。
    // 字段、空数组、数值种类和块引用沿原顺序逐项复制，不改变采样或查询范围。
    const auto generate=[&](auto &handler){
    const auto copy=[&](const auto &self,int at,unsigned depth)->bool {
      if(depth>=ancestors.size()||++nodes>node_limit||!lua_checkstack(L,8))throw std::runtime_error("lua_owned_node_or_depth_budget");
      at=lua_absindex(L,at);
      switch(lua_type(L,at)){
      case LUA_TNIL:add(4);return handler.Null();
      case LUA_TBOOLEAN:add(lua_toboolean(L,at)?4:5);return handler.Bool(lua_toboolean(L,at)!=0);
      case LUA_TNUMBER:{char number[64];if(lua_isinteger(L,at)){const auto n=lua_tointeger(L,at);const auto size=std::snprintf(number,sizeof(number),"%lld",static_cast<long long>(n));if(size<=0)throw std::runtime_error("lua_owned_number");add(unsigned(size));return handler.Int64(n);}
        const auto n=lua_tonumber(L,at);if(!std::isfinite(n))throw std::runtime_error("lua_owned_nonfinite");add(32);return handler.Double(n);}
      case LUA_TSTRING:{std::size_t size=0;const auto *s=lua_tolstring(L,at,&size);quoted(s,size);return handler.String(s,rapidjson::SizeType(size),true);}
      case LUA_TUSERDATA:{
        const auto *block=static_cast<const LuaOwnedBlock *>(luaL_testudata(L,at,kOwnedBlockMetatable));
        if(!block||!block->source||blocks.size()>=4096||block->source->Nodes()>node_limit-nodes)throw std::runtime_error("lua_owned_block_budget_or_foreign_userdata");
        add(block->source->Bytes());nodes+=block->source->Nodes();blocks.push_back(block->source);
        return handler.StartObject()&&handler.Key("__capture_owned_block",21,true)&&handler.Uint(unsigned(blocks.size()-1))&&handler.EndObject(1);
      }
      case LUA_TTABLE:break;
      default:throw std::runtime_error("lua_owned_borrowed_value");
      }
      const auto *identity=lua_topointer(L,at);for(unsigned i=0;i<depth;++i)if(ancestors[i]==identity)throw std::runtime_error("lua_owned_unencoded_cycle");ancestors[depth]=identity;
      const auto length=lua_rawlen(L,at);bool array=length>0;
      if(!array&&lua_getmetatable(L,at)){lua_pushliteral(L,"__isaac_cs_array");lua_rawget(L,-2);array=lua_toboolean(L,-1)!=0;lua_pop(L,2);}
      std::size_t count=0;add(2);
      if(array){lua_pushnil(L);while(lua_next(L,at)){if(++count>node_limit)throw std::runtime_error("lua_owned_members_budget");
          if(!lua_isinteger(L,-2)||lua_tointeger(L,-2)<1||std::uint64_t(lua_tointeger(L,-2))>length)throw std::runtime_error("lua_owned_mixed_array");lua_pop(L,1);}
        if(count)add(count-1);if(count!=length)throw std::runtime_error("lua_owned_array_hole");if(!handler.StartArray())return false;
        for(std::size_t i=1;i<=count;++i){lua_rawgeti(L,at,lua_Integer(i));const bool ok=self(self,-1,depth+1);lua_pop(L,1);if(!ok)return false;}return handler.EndArray(rapidjson::SizeType(count));}
      // 对象的成员数可在SAX EndObject时提交，单次raw遍历同时校验/复制，不预扫一遍。
      if(!handler.StartObject())return false;lua_pushnil(L);while(lua_next(L,at)){
        if(++count>node_limit)throw std::runtime_error("lua_owned_members_budget");
        if(lua_type(L,-2)!=LUA_TSTRING)throw std::runtime_error("lua_owned_object_key");
        std::size_t size=0;const auto *key=lua_tolstring(L,-2,&size);quoted(key,size);add(count==1?1:2);
        const bool ok=handler.Key(key,rapidjson::SizeType(size),true)&&self(self,-1,depth+1);lua_pop(L,1);if(!ok)return false;}return handler.EndObject(rapidjson::SizeType(count));
    };
    return copy(copy,index,0);};out.Populate(generate);
  });
}
// 分块在同一个采样窗口立即封闭；Lua只保留自有值handle，游戏对象不会进入块或worker。
inline int OwnedBlockGc(lua_State *L){auto *block=static_cast<LuaOwnedBlock *>(luaL_checkudata(L,1,kOwnedBlockMetatable));block->~LuaOwnedBlock();return 0;}
// 先建立可被Lua回收的空handle，再构建C++所有权；Lua分配失败不能跨过持有Source的C++栈。
inline LuaOwnedBlock *NewOwnedBlock(lua_State *L){
  auto *block=static_cast<LuaOwnedBlock *>(lua_newuserdata(L,sizeof(LuaOwnedBlock)));new(block)LuaOwnedBlock{};luaL_setmetatable(L,kOwnedBlockMetatable);return block;
}
inline int SealOwnedBlock(lua_State *L){
  auto *block=NewOwnedBlock(L);
  try{block->source=CopyLuaSource(L,1,64u<<20,32u<<20);return 1;}
  catch(...){return luaL_error(L,"capture_owned_block_failed");}
}
inline int JoinOwnedBlocks(lua_State *L){
  luaL_checktype(L,1,LUA_TTABLE);auto *joined=NewOwnedBlock(L);
  try{
    const auto count=lua_rawlen(L,1);if(count>4096)throw std::runtime_error("capture_block_count");
    joined->source=capture::OwnedSource::CreateGraph([&](replica::Json &value,std::uint64_t &bytes,std::uint64_t &nodes,auto &blocks){
      value.SetArray();bytes=2;nodes=1;for(std::size_t i=1;i<=count;++i){lua_rawgeti(L,1,lua_Integer(i));auto *block=static_cast<LuaOwnedBlock *>(luaL_testudata(L,-1,kOwnedBlockMetatable));
        if(!block||!block->source||block->source->Concatenate()||!block->source->Read().IsArray())throw std::runtime_error("capture_array_chunk");
        if(block->source->Bytes()>(32u<<20)-bytes||block->source->Nodes()>(64u<<20)-nodes)throw std::runtime_error("capture_array_chunk_budget");
        blocks.push_back(block->source);bytes+=block->source->Bytes()-(i==1?2:1);nodes+=block->source->Nodes();lua_pop(L,1);}
    },true);return 1;
  }catch(...){return luaL_error(L,"capture_owned_array_failed");}
}
inline int OwnedBlockText(lua_State *L){
  const auto *block=static_cast<const LuaOwnedBlock *>(luaL_checkudata(L,1,kOwnedBlockMetatable));
  try{const auto text=capture::SourceText(block->source);lua_pushlstring(L,text.data(),text.size());return 1;}
  catch(...){return luaL_error(L,"capture_block_diagnostic_budget");}
}
inline int OwnedBlockEqual(lua_State *L){
  const auto *left=static_cast<const LuaOwnedBlock *>(luaL_testudata(L,1,kOwnedBlockMetatable));
  const auto *right=static_cast<const LuaOwnedBlock *>(luaL_testudata(L,2,kOwnedBlockMetatable));
  if(!left||!right){lua_pushboolean(L,false);return 1;}
  try{lua_pushboolean(L,capture::EqualSources(left->source,right->source));return 1;}catch(...){return luaL_error(L,"capture_block_compare_failed");}
}
}
