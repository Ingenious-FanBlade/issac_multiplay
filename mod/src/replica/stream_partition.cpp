#include "replica/stream_partition.h"
#include "replica/stream_model.h"
#include <algorithm>
#include <array>
#include <string_view>
#include <cstdio>

namespace cs::replica::stream {
namespace {
using Text=std::string_view;
// 分层枚举使用独立名称，避免与同命名空间的World(scope)函数冲突。
enum Layer : unsigned {Context,Structure,Resource,WorldLayer};
constexpr const char *slots[]={"context","structure","resource","world"};
const Value *Get(const Value &v,const char *key){if(!v.IsObject())return nullptr;const auto i=v.FindMember(key);return i==v.MemberEnd()?nullptr:&i->value;}
bool Is(const Value &v,const char *key,Text text){const auto *x=Get(v,key);return x&&x->IsString()&&Text(x->GetString(),x->GetStringLength())==text;}
Value CopyValue(const Value &v,Allocator &a){Value copy;copy.CopyFrom(v,a);return copy;}
// Lua未显式标记的空顺序表原合同编码为{}；读取时可以遍历零项，但输出不能偷偷改成[]。
const Value &Sequence(const Value &v){static const Value empty(rapidjson::kArrayType);if(v.IsArray())return v;if(v.IsObject()&&v.ObjectEmpty())return empty;throw ContractError("stream_source_sequence");}
bool Native(unsigned d){return d==0||d==1||d==2||d==6;}
bool Metadata(Text key){return key=="coverage"||key=="sampling"||key=="previous_scope"||key=="sample_revision"||key=="changes";}
// 与生成Lua目录使用相同scope列；相同成员重载按生成器的相同优先顺序处理。
Layer FieldLayer(Text id){
  // 目录生成时已按同一冲突规则排序；查询不分配，也不触发十MiB目录的惰性DOM初始化。
  struct Policy {Text name;Layer layer;};
  static constexpr Policy policy[]={
#include "stream_field_layers.inc"
  };
  const auto i=std::lower_bound(std::begin(policy),std::end(policy),id,[](const Policy &p,Text key){return p.name<key;});
  return i==std::end(policy)||i->name!=id?Context:i->layer;
}
Layer NamedLayer(Text key,Layer fallback){
  static const std::map<Text,Layer> names={
    {"application_links",Structure},{"application_player",Structure},{"application_player_role",WorldLayer},
    {"application_player_equipment",Structure},{"application_player_resources",Resource},{"application_resources",Resource},
    {"application_owned",WorldLayer},{"application_geometry",WorldLayer},{"application_runtime",WorldLayer},{"application_player_state",WorldLayer},
    {"application_player_special",WorldLayer},{"application_draw",WorldLayer},{"application_player_draw",WorldLayer},{"application_npc",WorldLayer},
    {"application_text_bytes",WorldLayer},{"application_player_equipment_dynamic",WorldLayer},{"pose_runtime",WorldLayer},
    {"resource",Resource},{"resource_generation",Resource},{"shaders",Resource},{"application_source",Context},{"native_source",Context},
    {"presentation_runtime",WorldLayer},{"map_application",WorldLayer},{"backdrop_sprites",WorldLayer},{"vein_sprites",WorldLayer},{"map_sprites",WorldLayer},
    {"rail_sprite",WorldLayer},{"backdrop_wall_trace",Structure},{"subject",Structure},{"membership",Structure},{"memberships",Structure},
    {"class",Structure},{"casts",Structure},{"location",Context},{"descriptor",Structure},{"seeds",Structure},{"config",Resource},
    {"override_config",Resource},{"stage_config",Resource},{"doors",WorldLayer},{"entities",Structure},{"grids",Structure},
    {"lifted_descriptors",Structure},{"control_groups",Structure},{"roster",Structure},{"backup_slots",Structure},
    {"relations",Structure},{"native_membership",Structure},{"canonical_state",Structure}};
  const auto i=names.find(key);return i==names.end()?fallback:i->second;
}
constexpr Text saveIdentity="immutable_collection_revision_and_ordinal;not_native_object_continuity";
Value Equipment(Value &,Allocator &);
// 采样字段的真实值仍属于当前World，不改变scope或把旧值重新盖时间戳。
bool AllSampling(Text key,const Value &v,const Value &parent,unsigned domain){
  return domain==5&&((key=="source_scope"&&Is(parent,"identity_basis",saveIdentity))||
    (key=="temporal_relation"&&Is(v,"kind","ObservationScopes")))||domain==6&&key=="continuity";
}
std::vector<Text> SamplingFields(Text key,const Value &v,unsigned d){
  if(key=="location"&&Get(v,"session")&&Get(v,"epoch"))return {"scope","entity_scope"};
  if(d==6&&key=="application_source"&&Is(v,"contract","player-source-v2"))return {"entity_scope"};
  if(d==6&&key=="canonical_state"&&Is(v,"schema","isaac.cs.entity"))return {"scope","source_scope"};
  if(d==9&&key=="canonical_player_state"&&Is(v,"schema","isaac.cs.entity"))return {"scope"};return {};
}
struct Partition final {
  Allocator &a;unsigned domain;std::uint64_t work=0,limit;using Pieces=std::map<Layer,Value>;
  void Check(unsigned depth){if(depth>64||++work>limit)throw ContractError("stream_partition_budget");}
  void Add(Pieces &out,Layer layer,const char *key,Value value,unsigned capacity){auto [i,added]=out.try_emplace(layer);if(added)i->second=SizedObject(capacity,a);Put(i->second,key,std::move(value),a);}
  Pieces Split(const Value &v,Layer fallback,unsigned depth=0){
    Check(depth);Pieces out;const auto leaf=[&]{out.emplace(fallback,CopyValue(v,a));};
    if(!v.IsObject()&&!v.IsArray()||v.IsObject()&&v.ObjectEmpty()||v.IsArray()&&v.Empty()){leaf();return out;}
    const bool retained=!Native(domain);
    if(Is(v,"type","Sprite")){const auto *pose=Get(v,"pose_runtime"),*loaded=pose?Get(*pose,"loaded"):nullptr;
      if(loaded&&loaded->IsBool()&&!loaded->GetBool()){out.emplace(retained?Context:Resource,CopyValue(v,a));return out;}}
    const bool saved=domain==5&&(Is(v,"identity_basis",saveIdentity)||Is(v,"lifetime","current_floor_and_construction_revision"));
    if(Is(v,"type","reference")||Get(v,"resolution")||Get(v,"status")&&!Is(v,"status","Value")&&!Get(v,"fields")&&!saved||Is(v,"type","graph_ref")){leaf();return out;}
    if(const auto *fields=Get(v,"fields");Get(v,"type")&&fields&&fields->IsObject()&&fields->ObjectEmpty()){
      bool empty=true;for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){Text k=i->name.GetString();if(k!="type"&&k!="version"&&k!="fields")empty=false;}if(empty){leaf();return out;}}
    if(Is(v,"type","table")&&Get(v,"entries")){
      const auto &entries=v["entries"];if(entries.IsObject()&&entries.ObjectEmpty()||entries.IsArray()&&entries.Empty()){leaf();return out;}
      if(!entries.IsArray())throw ContractError("stream_graph_entries");
      for(const auto &entry:entries.GetArray())for(auto &[layer,piece]:Split(entry["value"],fallback,depth+1)){
        auto [i,created]=out.try_emplace(layer);if(created){i->second=SizedObject(3,a);Put(i->second,"type","table",a);Put(i->second,"node",CopyValue(v["node"],a),a);Value values(rapidjson::kArrayType);values.Reserve(entries.Size(),a);Put(i->second,"entries",std::move(values),a);}
        auto record=SizedObject(2,a);Put(record,"key",CopyValue(entry["key"],a),a);Put(record,"value",std::move(piece),a);i->second["entries"].PushBack(record,a);
      }return out;
    }
    if(v.IsArray()){
      // 只为真正缺席的槽建立占位；已存在的null也是有效值，不能与缺席混同。
      std::map<Layer,std::vector<bool>> present;
      for(unsigned index=0;index<v.Size();++index)for(auto &[layer,piece]:Split(v[index],fallback,depth+1)){
        auto [i,created]=out.try_emplace(layer,rapidjson::kArrayType);if(created){i->second.Reserve(v.Size(),a);present[layer].resize(v.Size());for(unsigned n=0;n<v.Size();++n)i->second.PushBack(Value(),a);}i->second[index]=std::move(piece);present[layer][index]=true;
      }
      for(auto &[layer,piece]:out)for(unsigned n=0;n<v.Size();++n)if(!present[layer][n]){auto absent=SizedObject(1,a);Put(absent,"type","stream_absent_piece",a);piece[n]=std::move(absent);}
      return out;
    }
    for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){const auto *key=i->name.GetString();const Text name(key);const auto &child=i->value;if(Metadata(name))continue;
      if(name=="fields"){
        for(auto f=child.MemberBegin();f!=child.MemberEnd();++f){const auto *id=f->name.GetString();const Layer layer=retained?Context:FieldLayer(id);
          Pieces parts;if(!retained&&(Text(id)=="Sprite.GetCurrentAnimationData"||Text(id)=="Sprite.GetOverlayAnimationData"))parts.emplace(layer,CopyValue(f->value,a));else parts=Split(f->value,layer,depth+1);
          for(auto &[group,piece]:parts){auto [entry,added]=out.try_emplace(group);if(added)entry->second=SizedObject(v.MemberCount(),a);if(!entry->second.HasMember("fields"))Put(entry->second,"fields",SizedObject(child.MemberCount(),a),a);Put(entry->second["fields"],id,std::move(piece),a);}
        }
      }else if(name=="application_player_equipment"&&Is(child,"status","Value")){
        // 只复制需要拆分的装备结构，禁止为修改几个装备字段先复制完整Player查询大树。
        // 装备重排只在短命arena内进行，废弃的effects/weapon内容不会留在返回树里。
        auto fixed=Copy(child);auto runtime=Equipment(fixed,fixed.GetAllocator());
        for(auto &[layer,piece]:Split(fixed,retained?Context:Structure,depth+1))Add(out,layer,key,std::move(piece),v.MemberCount()+1);
        for(auto &[layer,piece]:Split(runtime,retained?Context:WorldLayer,depth+1))Add(out,layer,"application_player_equipment_dynamic",std::move(piece),v.MemberCount()+1);
      }else if(name=="type"&&child.IsString()||name=="version")continue;
      else if(AllSampling(name,child,v,domain))Add(out,WorldLayer,key,CopyValue(child,a),v.MemberCount());
      else {const auto selected=SamplingFields(name,child,domain);Json state(rapidjson::kObjectType);const Value *source=&child;
        if(!selected.empty()){auto &scratch=state.GetAllocator();auto sampling=SizedObject(unsigned(selected.size()),a);for(auto f=child.MemberBegin();f!=child.MemberEnd();++f){const Text id=f->name.GetString();
          if(std::find(selected.begin(),selected.end(),id)!=selected.end())Put(sampling,f->name.GetString(),CopyValue(f->value,a),a);
          else Put(state,f->name.GetString(),CopyValue(f->value,scratch),scratch);
        }if(!sampling.ObjectEmpty())Add(out,WorldLayer,key,std::move(sampling),v.MemberCount());source=&state;}
        for(auto &[layer,piece]:Split(*source,retained?Context:NamedLayer(name,fallback),depth+1))Add(out,layer,key,std::move(piece),v.MemberCount());
      }
    }
    for(auto &[layer,piece]:out){if(const auto *type=Get(v,"type"))Put(piece,"type",Is(v,"type","Sprite")?Value("stream_sprite_piece",a):CopyValue(*type,a),a);if(const auto *version=Get(v,"version"))Put(piece,"version",CopyValue(*version,a),a);}return out;
  }
};
// 图编号只在自有输出中规范化；跨分区的引用补入来源定义，循环保持graph_ref而非递归展开。
struct Graphs final {
  Allocator &a;std::map<std::uint64_t,const Value *> definitions;std::uint64_t next=0,work=0,limit;
  void Check(unsigned depth){if(depth>64||++work>limit)throw ContractError("stream_graph_budget");}
  void Scan(const Value &v,unsigned depth=0){Check(depth);if(Is(v,"type","table")&&Get(v,"node")){const auto id=Unsigned(v["node"]);auto [i,added]=definitions.emplace(id,&v);if(!added&&*i->second!=v)throw ContractError("stream_graph_conflict");}
    if(v.IsArray())for(const auto &c:v.GetArray())Scan(c,depth+1);if(v.IsObject())for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)Scan(i->value,depth+1);}
  void Close(Value &root){std::set<std::uint64_t> locals;std::map<std::uint64_t,std::uint64_t> ids;
    const auto scan=[&](const auto &self,const Value &v,unsigned depth)->void{Check(depth);if(Is(v,"type","table")&&Get(v,"node"))locals.insert(Unsigned(v["node"]));if(v.IsArray())for(const auto &c:v.GetArray())self(self,c,depth+1);if(v.IsObject())for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)self(self,i->value,depth+1);};scan(scan,root,0);
    // 分层树已经由本函数独占，闭图只补缺少的定义并原地改编号，不再克隆整棵树。
    // locals与ids始终保存原编号；先登记后递归，环仍以graph_ref终止。
    const auto close=[&](const auto &self,Value &v,unsigned depth)->void{Check(depth);
      if(Is(v,"type","graph_ref")){const auto id=Unsigned(v["node"]);if(!locals.contains(id)){const auto i=definitions.find(id);if(i==definitions.end())throw ContractError("stream_graph_missing");locals.insert(id);v=CopyValue(*i->second,a);self(self,v,depth+1);return;}}
      if(v.IsArray()){for(auto &c:v.GetArray())self(self,c,depth+1);return;}
      if(!v.IsObject())return;
      std::sort(v.MemberBegin(),v.MemberEnd(),[](const auto &x,const auto &y){return Text(x.name.GetString())<Text(y.name.GetString());});
      const bool graph=Is(v,"type","table")||Is(v,"type","graph_ref");
      for(auto member=v.MemberBegin();member!=v.MemberEnd();++member){if(Text(member->name.GetString())=="node"&&graph){auto [i,added]=ids.try_emplace(Unsigned(member->value),0);if(added)i->second=++next;member->value.SetUint64(i->second);}else self(self,member->value,depth+1);}
    };close(close,root,0);
  }
};
// 原有装备结构/运行字段拆分保持字段级合同；不缓存充能、效果计数或临时变化。
Value Equipment(Value &fixed,Allocator &a){
  Value dynamic(rapidjson::kObjectType);Put(dynamic,"version",Value(1),a);Put(dynamic,"status","Value",a);
  for(const char *key:{"head_animation_hex","history_mutation_disabled"})if(fixed.HasMember(key)){Put(dynamic,key,CopyValue(fixed[key],a),a);fixed.RemoveMember(key);}
  Value effects(rapidjson::kObjectType);if(fixed.HasMember("effects")){auto &old=fixed["effects"];for(const char *key:{"cache_flags","evaluate","disabled"})if(old.HasMember(key))Put(effects,key,CopyValue(old[key],a),a);
    Value members(rapidjson::kArrayType),counts(rapidjson::kArrayType);for(const auto &effect:Sequence(old["entries"]).GetArray()){Value member(rapidjson::kObjectType),count(rapidjson::kObjectType);for(const char *key:{"item_present","type","id"})if(effect.HasMember(key))Put(member,key,CopyValue(effect[key],a),a);for(const char *key:{"count","cooldown"})if(effect.HasMember(key))Put(count,key,CopyValue(effect[key],a),a);members.PushBack(member,a);counts.PushBack(count,a);}if(members.Empty()){members.SetObject();counts.SetObject();}old.SetObject();Put(old,"entries",std::move(members),a);Put(effects,"entries",std::move(counts),a);}
  Put(dynamic,"effects",std::move(effects),a);Value weapons(rapidjson::kArrayType);
  if(fixed["weapons"].IsArray())for(auto &weapon:fixed["weapons"].GetArray()){Value runtime(rapidjson::kObjectType);for(const char *key:{"present","charge_bar"})if(weapon.HasMember(key))Put(runtime,key,CopyValue(weapon[key],a),a);weapon.RemoveMember("charge_bar");weapons.PushBack(runtime,a);}
  else Sequence(fixed["weapons"]);if(weapons.Empty())weapons.SetObject();Put(dynamic,"weapons",std::move(weapons),a);return dynamic;
}
}
Json PartitionSource(const Value &payload,unsigned domain,std::uint32_t limit){
  if(domain>=11)throw ContractError("stream_source_domain_index");Json out(rapidjson::kObjectType);auto &a=out.GetAllocator();Put(out,"contract","stream-source-v1",a);
  // 返回arena直接承载分层和闭图结果；失败同时报告实际分配请求及已持有容量。
  const char *stage="split";std::uint64_t object_index=0;
  try {
  for(const char *slot:{"context","structure","resource","world","bodies","members"})Put(out,slot,Value(rapidjson::kArrayType),a);
  Partition partition{a,domain,0,limit};const auto add=[&](Partition::Pieces pieces,const Value *subject,bool player){for(auto &[layer,piece]:pieces){auto entry=SizedObject(subject?2:1,a);if(subject)Put(entry,"subject",CopyValue(*subject,a),a);Put(entry,"value",std::move(piece),a);out[player&&layer==WorldLayer?"bodies":slots[layer]].PushBack(entry,a);}};
  if(const auto *objects=Get(payload,"objects")){
    for(const auto &object:Sequence(*objects).GetArray()){const bool player=Is(object,"class","EntityPlayer");const auto &subject=object["subject"];auto member=SizedObject(4,a);Put(member,"subject",CopyValue(subject,a),a);Put(member,"category",player?Value("Player",a):Get(object,"class")?CopyValue(object["class"],a):Value("Unknown",a),a);Put(member,"use",CopyValue(subject["use"],a),a);Put(member,"shape",CopyValue(subject["revision"],a),a);out["members"].PushBack(member,a);
      add(partition.Split(object,WorldLayer),&subject,player);++object_index;}
    Json rest(rapidjson::kObjectType);auto &scratch=rest.GetAllocator();for(auto i=payload.MemberBegin();i!=payload.MemberEnd();++i)if(Text(i->name.GetString())!="objects")Put(rest,i->name.GetString(),CopyValue(i->value,scratch),scratch);add(partition.Split(rest,Context),nullptr,false);
  }else add(partition.Split(payload,domain==1?WorldLayer:Context),nullptr,false);
  stage="graph-scan";Graphs graph{a,{},0,0,limit};graph.Scan(payload);stage="context";graph.Close(out["context"]);
  for(const char *slot:{"structure","resource","world","bodies"}){
    stage=slot;
    for(auto &entry:out[slot].GetArray())graph.Close(entry["value"]);
  }
  return out;
  }catch(const std::bad_alloc &e){char reason[384];std::snprintf(reason,sizeof(reason),"stream_partition_allocation:%s:domain=%u:object=%llu:arena=%llu:%.128s",stage,domain,static_cast<unsigned long long>(object_index),static_cast<unsigned long long>(a.Capacity()),e.what());throw ContractError(reason);}
}
}
