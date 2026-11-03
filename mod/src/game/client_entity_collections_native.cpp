#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_entity_adapters.h"
#include "game/bridge.h"
#include "game/world_sources/lua_entity.h"
#include "runtime.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace cs::game {
namespace {
// 单实体几何有界；不改变64M节点或组件字节预算，更不将节点上限当作实体池容量。
constexpr std::size_t GeometryLimit=16384;
void Number(lua_State *L,const char *key,float n){if(!std::isfinite(n))luaL_error(L,"entity_geometry_nonfinite");lua_pushnumber(L,n);lua_setfield(L,-2,key);}
void Point(lua_State *L,const Vector &v){lua_newtable(L);Number(L,"x",v.x);Number(L,"y",v.y);}
template<class T> bool Bounded(const T &v){return v.size()<=GeometryLimit;}
template<class T> void Points(lua_State *L,const char *name,const T &v){
  if(!Bounded(v))luaL_error(L,"entity_geometry_budget");lua_newtable(L);unsigned i=0;
  for(const auto &p:v){Point(L,p);lua_rawseti(L,-2,++i);}lua_setfield(L,-2,name);
}
template<class T> bool MatchPoints(const T &v,const std::vector<ClientPoseVector> &points){
  if(v.size()!=points.size())return false;for(std::size_t i=0;i<points.size();++i)if(v[i].x!=points[i].x||v[i].y!=points[i].y)return false;return true;
}
// 先构造本方候选再交换，分配失败不破坏原容器；异常由公共执行器记录真实写入前缀。
template<class T> void ReplacePoints(T &v,const std::vector<ClientPoseVector> &points){
  T candidate;candidate.reserve(points.size());for(const auto &p:points)candidate.emplace_back(p.x,p.y);v.swap(candidate);
}
}
// 只在正式同use创建见证下复制当前四容器和标量，不读取上一池use，不计算新采样。
int ClientEntityGeometryLua(lua_State *L){
  auto *e=lua::GetLuabridgeUserdata<Entity_Laser *>(L,1,lua::Metatables::ENTITY_LASER,"EntityLaser");
  const auto subject=ObservationSubject(e,false);lua_newtable(L);
  if(!IsMain()||!e||subject.status!=1||subject.readiness<2){lua_pushliteral(L,"NotCaptured");lua_setfield(L,-2,"status");return 1;}
  auto &h=e->_homingLaser;
  if(!Bounded(h._samples)||!Bounded(h._nonOptimizedSamples)||!Bounded(h._unkVec)||!Bounded(h._bezier))return luaL_error(L,"entity_geometry_budget");
  lua_pushliteral(L,"Value");lua_setfield(L,-2,"status");lua_pushinteger(L,1);lua_setfield(L,-2,"version");
  Point(L,h._origin);lua_setfield(L,-2,"origin");Point(L,h._endPoint);lua_setfield(L,-2,"end");
  Number(L,"parameter",h._unkFloat1);Number(L,"spacing",h._unkFloat2);
  lua_pushinteger(L,h._sampleNum);lua_setfield(L,-2,"sample_count");lua_pushinteger(L,h._unkInt);lua_setfield(L,-2,"cursor");
  Points(L,"optimized",h._samples);Points(L,"unoptimized",h._nonOptimizedSamples);
  lua_newtable(L);unsigned index=0;for(int v:h._unkVec){lua_pushinteger(L,v);lua_rawseti(L,-2,++index);}lua_setfield(L,-2,"indices");
  lua_newtable(L);index=0;
  for(const auto &b:h._bezier){std::array<float,6> values{};static_assert(sizeof(b)==sizeof(values),"J273 Bezier storage");std::memcpy(values.data(),&b,sizeof(b));
    lua_newtable(L);for(unsigned i=0;i<values.size();++i){if(!std::isfinite(values[i]))return luaL_error(L,"entity_bezier_nonfinite");lua_pushnumber(L,values[i]);lua_rawseti(L,-2,i+1);}lua_rawseti(L,-2,++index);}
  lua_setfield(L,-2,"bezier");return 1;
}
bool ClientEntityCollections(void *pointer,const ClientObjectDescription &d,bool check,const char *&reason){
  if(!pointer||!d.derived||!IsMain()||!ClientBusy()){reason="entity_collection_context";return false;}
  if(d.category==5){
    if(!d.derived->laser){reason="entity_laser_geometry_missing";return false;}auto &h=static_cast<Entity_Laser *>(pointer)->_homingLaser;const auto &g=*d.derived->laser;
    if(check){
      if(h._origin.x!=g.origin.x||h._origin.y!=g.origin.y||h._endPoint.x!=g.end.x||h._endPoint.y!=g.end.y||h._unkFloat1!=g.parameter||h._unkFloat2!=g.spacing||h._sampleNum!=g.sample_count||h._unkInt!=g.cursor||
         !MatchPoints(h._samples,g.optimized)||!MatchPoints(h._nonOptimizedSamples,g.unoptimized)||h._unkVec.size()!=g.indices.size()||h._bezier.size()!=g.bezier.size())return false;
      for(std::size_t i=0;i<g.indices.size();++i)if(h._unkVec[i]!=g.indices[i])return false;
      for(std::size_t i=0;i<g.bezier.size();++i)if(std::memcmp(&h._bezier[i],g.bezier[i].data(),sizeof(h._bezier[i])))return false;
    }else {
      // 每个真实vector保持自己的分配owner，绝不安装Authority的begin/end/capacity。
      ReplacePoints(h._samples,g.optimized);ReplacePoints(h._nonOptimizedSamples,g.unoptimized);
      decltype(h._unkVec) indices(g.indices.begin(),g.indices.end());h._unkVec.swap(indices);
      decltype(h._bezier) bezier;bezier.resize(g.bezier.size());for(std::size_t i=0;i<g.bezier.size();++i)std::memcpy(&bezier[i],g.bezier[i].data(),sizeof(bezier[i]));h._bezier.swap(bezier);
      h._origin=Vector(g.origin.x,g.origin.y);h._endPoint=Vector(g.end.x,g.end.y);h._unkFloat1=g.parameter;h._unkFloat2=g.spacing;h._sampleNum=g.sample_count;h._unkInt=g.cursor;
    }
  }
  if(d.category==7){auto &p=*static_cast<Entity_Pickup *>(pointer);const auto &cycle=d.derived->pickup_cycle;
    if(cycle.size()>8){reason="entity_pickup_cycle_capacity";return false;}
    if(check){if(p._cycleCollectibleCount!=cycle.size())return false;for(std::size_t i=0;i<cycle.size();++i)if(p._cycleCollectibleList[i]!=cycle[i])return false;}
    else {std::fill(std::begin(p._cycleCollectibleList),std::end(p._cycleCollectibleList),0u);std::copy(cycle.begin(),cycle.end(),p._cycleCollectibleList);p._cycleCollectibleCount=unsigned(cycle.size());}
  }
  reason=check?"entity_collections_checked":"entity_collections_installed";return true;
}
} // namespace cs::game
