#include "replica/projection_graph.h"
#include <map>
#include <set>
#include <string_view>

namespace cs::replica {
namespace {
bool Type(const Value &v,const char *name) {
  return v.IsObject()&&v.HasMember("type")&&v["type"].IsString()&&
      std::string_view(v["type"].GetString(),v["type"].GetStringLength())==name;
}
// 一次投影/一个组件一张局部图；只保留定义计数，不持有源DOM或Lua指针。
struct Graph final {
  std::map<std::uint64_t,unsigned> definitions;
  std::set<std::uint64_t> references;
  std::uint64_t visited=0;
};
std::uint64_t Node(const Value &v) {
  if(!v.HasMember("node"))throw ContractError("projection_graph_target_missing");
  const auto id=Unsigned(v["node"]);if(!id)throw ContractError("projection_graph_zero_node");return id;
}
void Scan(const Value &v,Graph &g,unsigned depth=0) {
  if(depth>Limits{}.depth||++g.visited>Limits{}.nodes)throw ContractError("projection_graph_budget");
  if(Type(v,"graph_ref"))g.references.insert(Node(v));
  else if(Type(v,"table")&&v.HasMember("node"))++g.definitions[Node(v)];
  if(v.IsArray())for(const auto &x:v.GetArray())Scan(x,g,depth+1);
  if(v.IsObject())for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)Scan(i->value,g,depth+1);
}
Graph Read(const Value &v) {
  Graph g;Scan(v,g);
  // 不跨组件借用同编号定义；省略字段造成的悬空引用必须显式拒绝，不能补零或删引用。
  for(const auto id:g.references){const auto it=g.definitions.find(id);
    if(it==g.definitions.end())throw ContractError("projection_graph_dangling_reference");
    if(it->second!=1)throw ContractError("projection_graph_ambiguous_reference");
  }
  return g;
}
}
void CheckProjectionGraph(const Value &v) {Read(v);}
void PruneProjectionGraph(Value &v) {
  const auto graph=Read(v);
  const auto prune=[&](auto &&self,Value &x)->void {
    if(Type(x,"table")&&x.HasMember("node")&&!graph.references.count(Node(x)))x.RemoveMember("node");
    if(x.IsArray())for(auto &e:x.GetArray())self(self,e);
    if(x.IsObject())for(auto i=x.MemberBegin();i!=x.MemberEnd();++i)self(self,i->value);
  };prune(prune,v);
}
} // namespace cs::replica
