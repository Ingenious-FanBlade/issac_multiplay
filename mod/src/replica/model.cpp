#include "replica/model.h"
#include "core/owned_source.h"
#ifdef ISAACCS_E2E
#include "replica/stream_source_value.h"
#include "replica/projection_graph.h"
#include "replica/room_grid_target.h"
#include "replica/entity_target.h"
#include "replica/player_target.h"
#endif
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <set>

namespace cs::replica {
namespace {
[[noreturn]] void Fail(const char *s){throw ContractError(s);}
std::string Str(const Value &v){return v.IsString()?std::string(v.GetString(),v.GetStringLength()):std::string{};}
const Value &At(const Value &v,const char *k){if(!v.IsObject()||!v.HasMember(k)) Fail("missing_field");return v[k];}
Value String(const std::string &s,Allocator &a){return Value(s.data(),rapidjson::SizeType(s.size()),a);}
void Check(const Value &v,const Limits &l,unsigned depth,std::uint64_t &nodes){
  if(depth>l.depth)Fail("value_depth_budget");
  if(++nodes>l.nodes)Fail("value_node_budget");
  if(v.IsString()&&v.GetStringLength()>l.string_bytes) Fail("string_budget");
  if(v.IsDouble()&&!std::isfinite(v.GetDouble())) Fail("nonfinite");
  if(v.IsArray()) for(const auto &x:v.GetArray()) Check(x,l,depth+1,nodes);
  if(v.IsObject()){
    std::set<std::string> seen;
    for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){
      if(!seen.insert(Str(i->name)).second) Fail("duplicate_field");
      Check(i->name,l,depth+1,nodes);Check(i->value,l,depth+1,nodes);
    }
  }
}
void Dependencies(Value &v,std::uint64_t revision,Allocator &a){
  if(v.IsArray())for(auto &x:v.GetArray())Dependencies(x,revision,a);
  if(!v.IsObject())return;
  for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)Dependencies(i->value,revision,a);
  if(v.HasMember("schema")&&v["schema"].IsString()){
    const auto schema=Str(v["schema"]);if(schema.rfind("isaac.cs.",0)==0){
      Put(v,"component",schema.substr(9),a);Put(v,"required_revision",Decimal(revision,a),a);
    }
  }
}
bool IsInternal(const std::string &key){
  return key=="pointer"||key=="ptr"||key=="vptr"||key=="address"||key=="native_address"||key=="device_pointer"||key=="handle"||key=="assignment_key";
}
const char *DomainName(unsigned d){
  static const char *names[]={"invalid","entity","grid","room","level","exploration","room_save","player","run","random","presentation","mod"};
  return d>0&&d<12?names[d]:"invalid";
}
// 归属和未调度占位仅传状态；解释位于版本化目录，读取失败的诊断仍按原值保留。
Value Status(Allocator &a,const std::string &status){
  auto out=SizedObject(1,a);Put(out,"status",status,a);return out;
}
bool Same(const Value &a,const Value &b){return a==b;}
void Model(const Value &v){
  if(Str(At(v,"format"))!="isaac.cs.replica.model"||!At(v,"revision").IsString()||!Unsigned(v["revision"])) Fail("model_schema");
  ReadScope(At(v,"scope"));
  if(!At(v,"components").IsObject()||v["components"].MemberCount()>11||!At(v,"publishable").IsBool()||!At(v,"missing_domains").IsArray()) Fail("component_budget");
}
} // namespace

Json Parse(const std::string &s,Limits l){
  if(s.size()>l.bytes) Fail("message_budget");
  // SAX预检先限制嵌套和节点，再构造DOM，避免恶意深度在分配／递归阶段绕过预算。
  struct Guard : rapidjson::BaseReaderHandler<rapidjson::UTF8<>,Guard> {
    Limits l;std::uint64_t nodes=0;unsigned depth=0;bool Count(){return ++nodes<=l.nodes;}
    explicit Guard(Limits x):l(x){}
    bool Null(){return Count();}bool Bool(bool){return Count();}bool Int(int){return Count();}bool Uint(unsigned){return Count();}
    bool Int64(std::int64_t){return Count();}bool Uint64(std::uint64_t){return Count();}bool Double(double d){return std::isfinite(d)&&Count();}
    bool String(const char*,rapidjson::SizeType n,bool){return n<=l.string_bytes&&Count();}
    bool Key(const char*,rapidjson::SizeType n,bool){return n<=l.string_bytes&&Count();}
    bool StartObject(){return ++depth<=l.depth&&Count();}bool EndObject(rapidjson::SizeType){--depth;return true;}
    bool StartArray(){return ++depth<=l.depth&&Count();}bool EndArray(rapidjson::SizeType){--depth;return true;}
  } guard(l);
  rapidjson::Reader reader;rapidjson::StringStream stream(s.c_str());
  if(!reader.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseFullPrecisionFlag>(stream,guard)) Fail("invalid_json_or_budget");
  Json d;d.Parse<rapidjson::kParseValidateEncodingFlag|rapidjson::kParseFullPrecisionFlag>(s.data(),s.size());
  if(d.HasParseError()) Fail("invalid_json");CheckValue(d,l);return d;
}
std::string Text(const Value &v){rapidjson::StringBuffer b;rapidjson::Writer<rapidjson::StringBuffer> w(b);if(!v.Accept(w)) Fail("invalid_value");return {b.GetString(),b.GetSize()};}
Json Copy(const Value &v){Json d;d.CopyFrom(v,d.GetAllocator());return d;}
std::uint64_t OwnedValueUpperBound(const Value &v){
  CheckValue(v);
  // CopyFrom与本计量器接受完全相同的SAX顺序：字符串先分配，容器在End时一次精确分配。
  // 按当前SDK的8字节对齐、自有chunk和UTF8内联字符串布局模拟，不创建另一份DOM。
  struct CopyArena final : rapidjson::BaseReaderHandler<rapidjson::UTF8<>,CopyArena> {
    std::uint64_t capacity=0,remaining=0;
    void Allocate(std::uint64_t n){
      if(!n)return;if(n>UINT64_MAX-7)Fail("value_memory_overflow");n=(n+7)&~7ull;
      if(n>remaining){const auto chunk=(std::max)(kJsonArenaChunk,n);if(chunk>UINT64_MAX-capacity)Fail("value_memory_overflow");capacity+=chunk;remaining=chunk;}
      remaining-=n;
    }
    bool String(const char *,rapidjson::SizeType n,bool copy){if(copy&&n>sizeof(Value)-3)Allocate(std::uint64_t(n)+1);return true;}
    bool Key(const char *s,rapidjson::SizeType n,bool copy){return String(s,n,copy);}
    bool EndArray(rapidjson::SizeType n){Allocate(std::uint64_t(n)*sizeof(Value));return true;}
    bool EndObject(rapidjson::SizeType n){Allocate(std::uint64_t(n)*sizeof(Value::Member));return true;}
  } arena;
  static_assert(sizeof(Value)==16||sizeof(Value)==24);
  if(!v.Accept(arena))Fail("value_memory_measure");
  const auto headers=(arena.capacity/kJsonArenaChunk+1)*64;
  if(arena.capacity>UINT64_MAX-sizeof(Json)||headers>UINT64_MAX-arena.capacity-sizeof(Json))Fail("value_memory_overflow");
  return arena.capacity+headers+sizeof(Json);
}
std::uint64_t OwnedJsonBytes(const Json &v){
  // RapidJSON缺少const GetAllocator；这里只读取Capacity，不修改值树或分配器。
  const auto capacity=const_cast<Json &>(v).GetAllocator().Capacity();
  return capacity+(capacity/kJsonArenaChunk+1)*64+sizeof(Json);
}
void CheckValue(const Value &v,Limits l){
  std::uint64_t nodes=0;Check(v,l,0,nodes);
  // 总字符串与节点开销也有上限，避免大量小字符串绕过单字符串限制。
  std::uint64_t bytes=24;auto count=[&](auto &&self,const Value &x)->void{
    bytes+=x.IsString()?std::uint64_t(x.GetStringLength())+5:9;if(bytes>l.value_bytes)Fail("value_byte_budget");
    if(x.IsArray())for(const auto &e:x.GetArray())self(self,e);
    if(x.IsObject())for(auto i=x.MemberBegin();i!=x.MemberEnd();++i){self(self,i->name);self(self,i->value);}
  };count(count,v);
}
#ifdef ISAACCS_E2E
void ValidateModel(const Value &v){
  CheckValue(v);Model(v);const auto scope=ReadScope(v["scope"]);Unsigned(At(v,"event_watermark"));
  if(At(v,"native_applied")!=false)Fail("wire_cannot_claim_native_apply");
  std::set<unsigned> domains;std::set<std::uint64_t> captures;std::string tick;
  for(auto i=v["components"].MemberBegin();i!=v["components"].MemberEnd();++i){
    const auto &component=i->value;const auto d=unsigned(Unsigned(At(component,"domain")));
    if(d<1||d>11||Str(i->name)!=DomainName(d)||!domains.insert(d).second||Unsigned(At(component,"version"))!=Unsigned(v["revision"]))Fail("component_identity_version");
    const auto coverage=Str(At(component,"coverage"));
    if(coverage!="Complete"&&coverage!="Partial"&&coverage!="Failed"&&coverage!="NotApplicable"&&coverage!="NotScheduled"&&coverage!="Disabled"&&coverage!="Unsupported")Fail("unknown_coverage");
    if(!At(component,"reasons").IsArray()||!At(component,"membership_complete").IsBool())Fail("component_coverage");
    if(v["publishable"]==true&&coverage!="Complete"&&coverage!="NotApplicable")Fail("incomplete_model");
    if(component["membership_complete"]!=(coverage=="Complete"))Fail("membership_claim");
    if(Str(At(component,"ownership"))!=(d==9?"AuthorityOnly":"ReplicatedState"))Fail("ownership_claim");
    const auto &c=At(component,"capture");if(Str(At(c,"phase"))!="OrdinaryPost"||!Unsigned(At(c,"authority_tick"))||!captures.insert(Unsigned(At(c,"scope"))).second)Fail("component_capture");
    const auto t=Text(c["authority_tick"]);if(tick.empty())tick=t;else if(tick!=t)Fail("mixed_component_tick");
    CheckProjectionGraph(At(component,"state"));
  }
  if(v["publishable"]==true&&(domains.size()!=11||!v["missing_domains"].Empty()))Fail("missing_required_domain");
  auto references=[&](auto &&self,const Value &x)->void{
    if(x.IsArray())for(const auto &e:x.GetArray())self(self,e);
    if(!x.IsObject())return;
    if(x.HasMember("local_id")&&x.HasMember("space"))Fail("observation_key_on_wire");
    if(x.HasMember("type")&&Str(x["type"])=="reference"){
      const auto status=Str(At(x,"status"));
      if(status!="Null"&&status!="KnownCurrent"&&status!="KnownHistorical"&&status!="Unresolved"&&status!="NotCaptured"&&status!="UnsupportedTarget"&&status!="Retired"&&status!="AmbiguousReuse"&&status!="CrossEpoch"&&status!="NotApplicable"&&status!="Unknown")Fail("reference_status");
      if(x.HasMember("target")){const auto &key=x["target"];if(!key.IsObject()||key.MemberCount()!=3||Unsigned(At(key,"run"))!=scope.run||Unsigned(At(key,"space"))<1||Unsigned(key["space"])>14||!Unsigned(At(key,"id")))Fail("reference_key");}
      else if(status=="KnownCurrent"||status=="KnownHistorical"||status=="Retired")Fail("reference_target_missing");
    }
    if(x.HasMember("required_revision")&&Unsigned(x["required_revision"])!=Unsigned(v["revision"]))Fail("component_dependency");
    for(auto i=x.MemberBegin();i!=x.MemberEnd();++i){if(IsInternal(Str(i->name)))Fail("native_pointer_or_handle");self(self,i->value);}
  };references(references,v["components"]);
}
#endif
Value Decimal(std::uint64_t n,Allocator &a){return String(std::to_string(n),a);}
std::uint64_t Unsigned(const Value &v){
  if(v.IsUint64()) return v.GetUint64();
  if(!v.IsString()||!v.GetStringLength()) Fail("invalid_u64");
  std::uint64_t n=0;const auto *b=v.GetString(),*e=b+v.GetStringLength();
  const auto r=std::from_chars(b,e,n);if(r.ec!=std::errc{}||r.ptr!=e||(e-b>1&&*b=='0')) Fail("invalid_u64");return n;
}
void Put(Value &v,const char *k,Value x,Allocator &a){if(v.HasMember(k))v.RemoveMember(k);v.AddMember(String(k,a),x,a);}
void Put(Value &v,const char *k,const std::string &s,Allocator &a){Put(v,k,String(s,a),a);}
Value ScopeValue(const Scope &s,Allocator &a){
  Value v(rapidjson::kObjectType);const char *names[]={"session","connection","run","floor","dimension","room","scene"};
  const std::uint64_t values[]={s.session,s.connection,s.run,s.floor,s.dimension,s.room,s.scene};
  for(unsigned i=0;i<7;++i)Put(v,names[i],Decimal(values[i],a),a);return v;
}
Scope ReadScope(const Value &v){
  if(!v.IsObject()||v.MemberCount()!=7) Fail("scope_schema");
  Scope s{Unsigned(At(v,"session")),Unsigned(At(v,"connection")),Unsigned(At(v,"run")),Unsigned(At(v,"floor")),Unsigned(At(v,"dimension")),Unsigned(At(v,"room")),Unsigned(At(v,"scene"))};
  if(!s.session||!s.connection||!s.scene) Fail("scope_generation");return s;
}
#ifdef ISAACCS_E2E
const Json &Catalog(){static const auto d=Parse(
#include "replication_catalog.inc"
  ,Limits{32u<<20,1048576,64,2u<<20});return d;}
const Value *Policy(const std::string &id){
  // 相同语义在多个目录中出现时优先规范Entity项；目录原行全部保留供审阅。
  static const auto index=[](){std::map<std::string,const Value *> m;for(const auto &r:Catalog()["members"].GetArray()){
    const auto id=Str(r["id"]);auto it=m.find(id);if(it==m.end()||Str(r["source_catalog"])=="entity-catalog.json") m[id]=&r;
  }return m;}();
  const auto i=index.find(id);return i==index.end()?nullptr:i->second;
}

void Registry::BindSource(std::uint64_t session,std::uint64_t epoch,const std::string &run){
  if(session!=source_session_||!epoch||run.empty()||run=="0")Fail("invalid_source_generation");
  if(source_epoch_&&(source_epoch_!=epoch||source_run_!=run))Fail("source_run_or_epoch_changed");
  source_epoch_=epoch;source_run_=run;
}
void Registry::DeclareCurrent(unsigned space,const std::string &local,const std::string &use){
  const auto name=std::to_string(space)+":"+local+":"+use;
  if((space!=1&&space!=2)||local.empty()||use.empty()||use=="0"||retired_sources_.contains(name))Fail("stream_retired_or_invalid_source");
  const auto base=std::to_string(space)+":"+local;
  if(const auto old=current_uses_.find(base);old!=current_uses_.end()&&old->second!=use&&keys_.contains(name))Fail("stream_old_use_reappeared");
  current_uses_[base]=use;
}
void Registry::RetireSource(unsigned space,const std::string &local,const std::string &use){
  const auto base=std::to_string(space)+":"+local;
  retired_sources_.insert(base+":"+use);
  if(auto i=current_uses_.find(base);i!=current_uses_.end()&&i->second==use)current_uses_.erase(i);
}
Value Registry::Key(unsigned space,const std::string &local,const std::string &use,Allocator &a){
  if(!session_||!run_||space<1||space>14||local.empty()||local=="0") Fail("invalid_subject");
  const auto base=std::to_string(space)+":"+local;
  auto resolved=use;
  if(strict_sources_&&(space==1||space==2)){
    const auto current=current_uses_.find(base);
    if(use.empty()){if(current==current_uses_.end())Fail("stream_reference_use_unknown");resolved=current->second;}
    else if(!keys_.contains(base+":"+use)&&(current==current_uses_.end()||current->second!=use))Fail("stream_reference_not_declared");
  }else if(!use.empty()){
    // 先遇到无use关联，再遇到其首个主体时合并占位key；已知旧use后的复用仍分配新ID。
    if(!current_uses_.count(base)){if(auto placeholder=keys_.find(base+":");placeholder!=keys_.end()){const auto id=placeholder->second;keys_.erase(placeholder);keys_.emplace(base+":"+use,id);}}
    current_uses_[base]=use;
  }else if(auto it=current_uses_.find(base);it!=current_uses_.end())resolved=it->second;
  const auto name=base+":"+resolved;auto it=keys_.find(name);
  if(it==keys_.end()){
    if(keys_.size()>=262144||next_==UINT64_MAX) Fail("identity_exhausted");
    it=keys_.emplace(name,++next_).first;
  }
  auto out=SizedObject(3,a);Put(out,"run",Decimal(run_,a),a);Put(out,"space",Value(space),a);Put(out,"id",Decimal(it->second,a),a);return out;
}
Value Registry::Project(const Value &v,Allocator &a,std::uint64_t epoch){
  auto out=ProjectValue(v,a,epoch,0);PruneProjectionGraph(out);return out;
}
// 递归阶段只处理显式成员归属，绝不按结果值决定省略；图闭合检查在整组件完成后执行。
Value Registry::ProjectValue(const Value &v,Allocator &a,std::uint64_t epoch,unsigned depth){
  if(depth>64) Fail("projection_depth");
  if(v.IsArray()){Value out(rapidjson::kArrayType);out.Reserve(v.Size(),a);for(const auto &x:v.GetArray())out.PushBack(ProjectValue(x,a,epoch,depth+1),a);return out;}
  if(!v.IsObject()) {Value out;out.CopyFrom(v,a);return out;}
  if(v.HasMember("session")&&Unsigned(v["session"])!=source_session_) Fail("cross_session_subject");
  if(v.HasMember("epoch")&&v["epoch"].IsString()&&Unsigned(v["epoch"])!=epoch) Fail("cross_epoch_subject");
  if(v.HasMember("status")&&Str(v["status"])=="NotScheduled")return Status(a,"NotScheduled");
  // Entity／Grid与所有相关身份统一转换，绝不把本地Index／seed／地址当key。
  if(v.HasMember("space")&&v.HasMember("local_id")){
    auto out=SizedObject(3+unsigned(v.HasMember("revision"))+unsigned(v.HasMember("construction_revision"))+unsigned(v.HasMember("control_relation"))+unsigned(v.HasMember("control_group_membership")),a);const auto space=unsigned(Unsigned(v["space"]));
    const auto local=Str(v["local_id"]);std::string status=v.HasMember("resolution")?Str(v["resolution"]):v.HasMember("presence")&&Str(v["presence"])!="Known"?Str(v["presence"]):"KnownCurrent";
    if(strict_sources_&&(space==1||space==2)&&status=="KnownCurrent"&&local!="0"&&!local.empty()){
      const auto base=std::to_string(space)+":"+std::to_string(epoch)+"/"+local;
      const auto current=current_uses_.find(base);
      const auto use=v.HasMember("use")?Str(v["use"]):current==current_uses_.end()?std::string{}:current->second;
      if(retired_sources_.contains(base+":"+use))Fail("stream_reference_retired");
      if(current==current_uses_.end()||current->second!=use)Fail("stream_reference_use_conflict");
    }
    Put(out,"type","reference",a);Put(out,"status",status,a);
    if(local!="0"&&!local.empty()&&(status=="KnownCurrent"||status=="KnownHistorical"||status=="Retired")){
      Put(out,"target",Key(space,std::to_string(epoch)+"/"+local,v.HasMember("use")?Str(v["use"]):"",a),a);
    }
    if(v.HasMember("revision"))Put(out,"shape_revision",ProjectValue(v["revision"],a,epoch,depth+1),a);
    // descriptor构建修订是当前配置见证，不是出生；稳定Key投影不能吞掉该变更依据。
    if(v.HasMember("construction_revision"))Put(out,"construction_revision",ProjectValue(v["construction_revision"],a,epoch,depth+1),a);
    for(const char *k:{"control_relation","control_group_membership"})if(v.HasMember(k))Put(out,k,ProjectValue(v[k],a,epoch,depth+1),a);
    return out;
  }
  // 显式只读Query把成员名放在值中；它也遵守同一目录，不能借result包装绕过发送选择。
  const auto *query=v.HasMember("member")&&v["member"].IsString()&&v.HasMember("parameters")&&v.HasMember("result")?Policy(Str(v["member"])):nullptr;
  const bool omitted_query=query&&Str((*query)["ownership"])=="NotReplicated";
  // 来源的小Query/值包装按已知成员数分配，避免身份投影重新放大整个域。
  auto out=SizedObject(v.MemberCount()+unsigned(omitted_query&&!v.HasMember("status"))+
    unsigned(v.HasMember("type")&&v["type"]=="Sprite"&&!v.HasMember("resource")),a);
  for(auto it=v.MemberBegin();it!=v.MemberEnd();++it){
    const auto key=Str(it->name);
    if(IsInternal(key)) Fail("native_pointer_or_handle");
    if(key=="session"||key=="epoch") continue;
    if(omitted_query&&(key=="parameters"||key=="result"))continue;
    Value x;
    if(const auto *p=Policy(key);p&&Str((*p)["ownership"])!="ReplicatedState"){
      const auto ownership=Str((*p)["ownership"]);if(ownership=="NotReplicated")continue;
      x=Status(a,ownership);
    }else if(key=="fields"&&it->value.IsObject()){
      x=SizedObject(it->value.MemberCount(),a);
      for(auto f=it->value.MemberBegin();f!=it->value.MemberEnd();++f){
        const auto id=Str(f->name);const auto *rule=Policy(id);Value field;
        if(!rule){field=SizedObject(2,a);Put(field,"status","Unsupported",a);Put(field,"reason","undeclared_member",a);}
        else if(Str((*rule)["ownership"])!="ReplicatedState") {const auto ownership=Str((*rule)["ownership"]);if(ownership=="NotReplicated")continue;field=Status(a,ownership);}
        else field=ProjectValue(f->value,a,epoch,depth+1);
        x.AddMember(String(id,a),field,a);
      }
    }else{
      unsigned space=0;
      if(key=="participant_local_id")space=9;else if(key=="control_group_local_id")space=10;
      else if(key=="floor")space=4;else if(key=="dimension")space=5;else if(key=="descriptor")space=6;else if(key=="room_instance")space=7;else if(key=="run")space=3;
      if(space&&it->value.IsString()&&Str(it->value)!="0")x=Key(space,std::to_string(epoch)+"/"+Str(it->value),"",a);
      else x=ProjectValue(it->value,a,epoch,depth+1);
    }
    out.AddMember(String(key,a),x,a);
  }
  if(omitted_query)Put(out,"status","NotReplicated",a);
  if(v.HasMember("type")&&Str(v["type"])=="Sprite"){
    Put(out,"type","resource_pose",a);Value descriptor(rapidjson::kObjectType);
    Put(descriptor,"category","anm2",a);Put(descriptor,"content_version","REPENTOGON-1.1.3/J273;mod-version-required",a);Put(descriptor,"status","Unverified",a);
    if(v.HasMember("fields")&&v["fields"].HasMember("Sprite.GetFilename"))Put(descriptor,"logical_path",ProjectValue(v["fields"]["Sprite.GetFilename"],a,epoch,depth+1),a);
    else Put(descriptor,"status","Unavailable",a);
    Put(out,"resource",std::move(descriptor),a);
  }
  return out;
}

// 共用完整窗口校验，但每次只取得一域封套；冷构建不并存十一棵展开来源树。
template<class RowAt>
Json CaptureFromRows(const Scope &scope,std::uint64_t revision,std::size_t count,RowAt row_at,Registry &registry){
  Json out(rapidjson::kObjectType);auto &a=out.GetAllocator();ReadScope(ScopeValue(scope,a));if(!revision)Fail("zero_revision");
  Put(out,"format","isaac.cs.replica.model",a);Put(out,"scope",ScopeValue(scope,a),a);Put(out,"revision",Decimal(revision,a),a);
  Value components(rapidjson::kObjectType),missing(rapidjson::kArrayType);
  std::set<unsigned> domains;std::string window;bool complete=true;std::uint64_t watermark=0;std::uint64_t epoch=0;
  for(std::size_t index=0;index<count;++index){
    const auto &r=row_at(index);
    if(Str(At(r,"format"))!="isaac.cs.observation"||Unsigned(At(r,"version"))!=2||Unsigned(At(r,"schema_version"))!=2)Fail("source_schema");
    const auto &c=At(r,"context");if(Unsigned(At(c,"session"))!=registry.SourceSession())Fail("cross_session");
    const auto e=Unsigned(At(c,"epoch"));if(!epoch)epoch=e;else if(epoch!=e)Fail("cross_epoch");
    const auto &run=At(c,"run");if(Str(At(run,"presence"))!="Known")Fail("source_run_unknown");registry.BindSource(registry.SourceSession(),e,Str(At(run,"local_id")));
    const auto domain=unsigned(Unsigned(At(r,"domain")));if(domain<1||domain>11)Fail("unsupported_domain");
    const auto phase=Str(At(c,"phase"));const auto schema=Str(At(r,"schema"));
    if(schema!="isaac.cs."+std::string(DomainName(domain)))Fail("not_ordinary_domain_state");
    if(phase!="OrdinaryPost"||Str(At(r,"kind"))!="State")Fail("independent_scope_requires_event_or_component");
    if(!domains.insert(domain).second)Fail("duplicate_component");
    const auto w=Text(At(c,"authority_tick"))+":"+Text(At(c,"outer"))+":"+Text(At(c,"owner"));
    if(!Unsigned(c["authority_tick"])||!Unsigned(c["outer"])||!Unsigned(c["owner"]))Fail("invalid_source_window");
    if(window.empty())window=w;else if(window!=w)Fail("mixed_source_window");
    if(At(c,"mutation_before")!=At(c,"mutation_after"))Fail("source_mutation");
    watermark=(std::max)(watermark,Unsigned(At(c,"event_watermark")));
    Value component(rapidjson::kObjectType);
    Put(component,"domain",Value(domain),a);Put(component,"version",Decimal(revision,a),a);
    Value provenance(rapidjson::kObjectType);
    for(const char *k:{"scope","sequence","authority_tick","phase","source","source_version","event_watermark"}){Value value;value.CopyFrom(At(c,k),a);Put(provenance,k,std::move(value),a);}
    // 20保留真实采集owner/token，Player解析不只凭相同tick拼接两个来源。
    for(const char *k:{"outer","owner","mutation_before","mutation_after"}){Value value;value.CopyFrom(At(c,k),a);Put(provenance,k,std::move(value),a);}
    Put(component,"capture",std::move(provenance),a);
    const auto coverage=Str(At(At(r,"coverage"),"status"));
    if(coverage!="Complete"&&coverage!="NotApplicable")complete=false;
    Put(component,"coverage",coverage,a);
    Value reasons;reasons.CopyFrom(At(r["coverage"],"reasons"),a);Put(component,"reasons",std::move(reasons),a);
    if(domain==9){Put(component,"ownership","AuthorityOnly",a);Put(component,"state",Status(a,"AuthorityOnly"),a);}
    else {Put(component,"ownership","ReplicatedState",a);auto state=registry.Project(At(r,"payload"),a,epoch);Dependencies(state,revision,a);Put(component,"state",std::move(state),a);}
    // 成员完整性由域来源明确声明；Partial不能被Delta当作全成员删除依据。
    Put(component,"membership_complete",Value(coverage=="Complete"),a);
    components.AddMember(String(DomainName(domain),a),component,a);
  }
  for(unsigned d=1;d<=11;++d)if(!domains.count(d)){missing.PushBack(String(DomainName(d),a),a);complete=false;}
  Put(out,"components",std::move(components),a);Put(out,"missing_domains",std::move(missing),a);
  Put(out,"publishable",Value(complete),a);Put(out,"event_watermark",Decimal(watermark,a),a);
  Put(out,"native_applied",Value(false),a);ValidateModel(out);return out;
}
Json CaptureRows(const Scope &scope,std::uint64_t revision,const std::vector<Json> &rows,Registry &registry){
  return CaptureFromRows(scope,revision,rows.size(),[&](std::size_t index)->const Json &{return rows[index];},registry);
}
Json Capture(const Scope &scope,std::uint64_t revision,const std::vector<world::observation::ObservationEnvelope> &values,Registry &registry){
  return CaptureFromRows(scope,revision,values.size(),[&](std::size_t index){
    const auto &v=values[index];
    if(!v.owned_source)return Parse(world::observation::EncodeEnvelope(v));
    // 只编码小封套元数据；来源正文直接从同窗纯值投影，冷构建不恢复大JSON往返。
    auto metadata=v;metadata.payload.clear();metadata.stream_payload.clear();metadata.owned_source.reset();
    auto envelope=Parse(world::observation::EncodeEnvelope(metadata));auto row=stream::ExpandSource(v.owned_source);Value payload=std::move(static_cast<Value &>(row));row.SetObject();
    for(auto i=envelope.MemberBegin();i!=envelope.MemberEnd();++i)if(std::strcmp(i->name.GetString(),"payload")){Value value;value.CopyFrom(i->value,row.GetAllocator());Put(row,i->name.GetString(),std::move(value),row.GetAllocator());}
    Put(row,"payload",std::move(payload),row.GetAllocator());
    if(unsigned(v.domain)>=3&&row["payload"].IsObject())Put(row["payload"],"sample_revision",Decimal(v.context.scope,row.GetAllocator()),row.GetAllocator());
    return row;
  },registry);
}
// 独立回调／Render／查询是自己的component，不获得普通完整世界采样资格。
Message Independent(const Scope &scope,const Value &r,Registry &registry){
  const auto &c=At(r,"context");if(Unsigned(At(c,"session"))!=scope.session)Fail("cross_session");
  if(Str(At(c,"phase"))=="OrdinaryPost"||!At(c,"authority_tick").IsNull())Fail("independent_borrowed_tick");
  const auto domain=unsigned(Unsigned(At(r,"domain")));if(domain<1||domain>11)Fail("unsupported_domain");
  Message m;m.kind=Kind::Component;m.sequence=Unsigned(At(c,"sequence"));m.body.SetObject();auto &a=m.body.GetAllocator();
  Put(m.body,"scope",ScopeValue(scope,a),a);Put(m.body,"domain",Value(domain),a);Put(m.body,"source_schema",Str(At(r,"schema")),a);
  Put(m.body,"capture_kind",Str(At(c,"phase")),a);Put(m.body,"source_scope",Decimal(Unsigned(At(c,"scope")),a),a);
  Put(m.body,"required_state",Decimal(0,a),a);Put(m.body,"coverage",Str(At(r["coverage"],"status")),a);
  Put(m.body,"value",domain==9?Status(a,"AuthorityOnly"):registry.Project(At(r,"payload"),a,Unsigned(At(c,"epoch"))),a);
  return m;
}

Json Difference(const Value &baseline,const Value &target){
  ValidateModel(baseline);ValidateModel(target);if(ReadScope(baseline["scope"])!=ReadScope(target["scope"]))Fail("delta_scope");
  if(Unsigned(target["revision"])<=Unsigned(baseline["revision"]))Fail("delta_revision");
  if(target["publishable"]!=true||baseline["publishable"]!=true)Fail("incomplete_membership");
  Json out(rapidjson::kObjectType);auto &a=out.GetAllocator();Put(out,"scope",ScopeValue(ReadScope(target["scope"]),a),a);
  Put(out,"base",Decimal(Unsigned(baseline["revision"]),a),a);Put(out,"target",Decimal(Unsigned(target["revision"]),a),a);
  Value updates(rapidjson::kObjectType),removed(rapidjson::kArrayType),header(rapidjson::kObjectType);
  for(auto i=target.MemberBegin();i!=target.MemberEnd();++i)if(Str(i->name)!="components"){Value v;v.CopyFrom(i->value,a);header.AddMember(String(Str(i->name),a),v,a);}
  for(auto i=target["components"].MemberBegin();i!=target["components"].MemberEnd();++i){
    const auto k=Str(i->name);if(!baseline["components"].HasMember(k.c_str())||!Same(baseline["components"][k.c_str()],i->value)){Value v;v.CopyFrom(i->value,a);updates.AddMember(String(k,a),v,a);}
  }
  for(auto i=baseline["components"].MemberBegin();i!=baseline["components"].MemberEnd();++i)if(!target["components"].HasMember(i->name)){
    Value deletion(rapidjson::kObjectType);Put(deletion,"status","Deleted",a);Put(deletion,"component",Str(i->name),a);removed.PushBack(deletion,a);
  }
  Put(out,"header",std::move(header),a);Put(out,"replace",std::move(updates),a);Put(out,"remove",std::move(removed),a);return out;
}
Json Reconstruct(const Value &baseline,const Value &delta){
  Model(baseline);if(Unsigned(At(delta,"base"))!=Unsigned(baseline["revision"]))Fail("missing_baseline");
  if(ReadScope(At(delta,"scope"))!=ReadScope(baseline["scope"]))Fail("delta_scope");
  if(Unsigned(At(delta,"target"))<=Unsigned(delta["base"]))Fail("delta_revision");
  if(!At(delta,"remove").IsArray()||!At(delta,"replace").IsObject()||!At(delta,"header").IsObject())Fail("delta_schema");
  Json out=Copy(At(delta,"header"));auto &a=out.GetAllocator();Value components;components.CopyFrom(baseline["components"],a);
  std::set<std::string> removed;
  for(const auto &v:At(delta,"remove").GetArray()){
    const auto k=Str(At(v,"component"));if(Str(At(v,"status"))!="Deleted"||!removed.insert(k).second||!components.HasMember(k.c_str()))Fail("invalid_delete");components.RemoveMember(k.c_str());
  }
  for(auto i=At(delta,"replace").MemberBegin();i!=delta["replace"].MemberEnd();++i){
    const auto k=Str(i->name);if(removed.count(k))Fail("delete_update_conflict");Value v;v.CopyFrom(i->value,a);Put(components,k.c_str(),std::move(v),a);
  }
  Put(out,"components",std::move(components),a);ValidateModel(out);
  if(out["revision"]!=delta["target"]||ReadScope(out["scope"])!=ReadScope(delta["scope"])||out["publishable"]!=true)Fail("delta_target");return out;
}
Message Applied(const Scope &s,std::uint64_t snapshot,std::uint64_t watermark,bool committed){
  if(!committed||!snapshot)Fail("native_commit_required");Message m;m.kind=Kind::AppliedAck;m.sequence=snapshot;m.body.SetObject();auto &a=m.body.GetAllocator();
  Put(m.body,"scope",ScopeValue(s,a),a);Put(m.body,"snapshot",Decimal(snapshot,a),a);Put(m.body,"event_watermark",Decimal(watermark,a),a);return m;
}
Message InputProgress(const Scope &s,std::uint64_t participant,std::uint64_t generation,std::uint64_t sequence,std::uint64_t tick){
  if(participant>3||!generation||!sequence||!tick)Fail("input_ack_range");Message m;m.kind=Kind::InputAck;m.sequence=sequence;m.body.SetObject();auto &a=m.body.GetAllocator();
  Put(m.body,"scope",ScopeValue(s,a),a);Put(m.body,"participant",Value(unsigned(participant)),a);Put(m.body,"generation",Decimal(generation,a),a);Put(m.body,"input_sequence",Decimal(sequence,a),a);Put(m.body,"processed_tick",Decimal(tick,a),a);return m;
}
#endif
// 网络身份到可信Participant的选择仍由16的主线程连接owner完成，不能由payload改投。
Message InputIntent(const Scope &s,unsigned participant,std::uint64_t generation,std::uint64_t sequence,std::uint64_t target,const core::InputStateDraftV2 &state){
  if(participant>3||!generation||!sequence||!target||!core::Valid(state))Fail("input_intent_invalid");
  Message m;m.kind=Kind::Input;m.sequence=sequence;m.body.SetObject();auto &a=m.body.GetAllocator();
  Put(m.body,"scope",ScopeValue(s,a),a);Put(m.body,"participant",Value(participant),a);Put(m.body,"generation",Decimal(generation,a),a);Put(m.body,"input_sequence",Decimal(sequence,a),a);Put(m.body,"target_tick",Decimal(target,a),a);
  Value movement(rapidjson::kArrayType),aim(rapidjson::kArrayType);movement.PushBack(Value(int(state.move_x)),a).PushBack(Value(int(state.move_y)),a);aim.PushBack(Value(int(state.aim_x)),a).PushBack(Value(int(state.aim_y)),a);
  Put(m.body,"movement",std::move(movement),a);Put(m.body,"aim",std::move(aim),a);Put(m.body,"held",Value(state.held_flags),a);Put(m.body,"edge",Value(state.edge_flags),a);return m;
}
#ifdef ISAACCS_E2E
Json Capabilities(){auto out=Parse(R"({"protocol":1,"model":2,"game":"v1.9.7.12.J273","repentogon":"1.1.3","required":["typed-values-v1","stable-keys-v1","scope-components-v1","complete-delta-v1"],"supported":["typed-values-v1","stable-keys-v1","scope-components-v1","complete-delta-v1"],"domains":[1,2,3,4,5,6,7,8,9,10,11],"model_categories":{"entity":["Player","NPC","Delirium","Tear","Projectile","Bomb","Laser","Knife","Pickup","Familiar","Effect","Slot","NativeText","NativeDummy"],"grid":"all target-version categories;specialized fields and explicit Unsupported","modes":["Normal","Hard","Greed","Greedier","Challenge","SeedEffects","DailyNonRanking"],"resource_pose":["ANM2","texture-replacements","main","overlay","layers","tracks","shader","camera","transition"],"unexposed":"Unavailable;never synthesized"},"native_appliers":[],"mods":[],"resources":{"base":"J273","status":"InstallationRequired"},"limits":{"bytes":33554432,"datagram":1200,"candidates":2,"fragments":32768,"timeout_ms":5000}})");
  // 只提升模型语义版本；ICSR v1类型标签和网络lane保持原格式，旧模型在协商时明确拒绝。
  out["required"].PushBack(Value("replica-projection-v2",out.GetAllocator()),out.GetAllocator());
  out["supported"].PushBack(Value("replica-projection-v2",out.GetAllocator()),out.GetAllocator());
  auto &allocator=out.GetAllocator();auto contract=RoomGridCapabilities();
  Value target;target.CopyFrom(contract,allocator);Put(out["model_categories"],"room_grid_target",std::move(target),allocator);
  out["supported"].PushBack(Value("room-grid-target-v10",allocator),allocator);
  auto entity=EntityCapabilities();Value entity_contract;entity_contract.CopyFrom(entity,allocator);
  Put(out["model_categories"],"entity_target",std::move(entity_contract),allocator);
  out["required"].PushBack(Value("entity-target-v1",allocator),allocator);
  out["supported"].PushBack(Value("entity-target-v1",allocator),allocator);
  auto player=PlayerCapabilities();Value player_contract;player_contract.CopyFrom(player,allocator);
  Put(out["model_categories"],"player_target",std::move(player_contract),allocator);
  out["required"].PushBack(Value("player-target-v2",allocator),allocator);
  out["supported"].PushBack(Value("player-target-v2",allocator),allocator);return out;
}
#endif
Json Negotiate(const Value &local,const Value &remote){
  for(const auto *v:{&local,&remote}){
    if(!v->IsObject()||v->MemberCount()!=12)Fail("capability_schema");
    for(const char *key:{"required","supported","domains","native_appliers","mods"})if(!At(*v,key).IsArray()||(*v)[key].Size()>256)Fail("capability_budget");
    for(const char *key:{"required","supported"}){std::set<std::string> seen;for(const auto &item:(*v)[key].GetArray())if(!item.IsString()||!seen.insert(Str(item)).second)Fail("capability_schema");}
    if(!At(*v,"resources").IsObject()||!At(*v,"limits").IsObject()||(*v)["limits"].MemberCount()!=5)Fail("capability_schema");
    std::set<std::uint64_t> domains;for(const auto &domain:(*v)["domains"].GetArray()){const auto d=Unsigned(domain);if(d<1||d>11||!domains.insert(d).second)Fail("domain_capability");}if(domains.size()!=11)Fail("missing_domain_capability");
    const char *keys[]={"bytes","datagram","candidates","fragments","timeout_ms"};const std::uint64_t maxima[]={32ull<<20,65507,2,32768,60000};
    for(unsigned i=0;i<5;++i){const auto n=Unsigned(At((*v)["limits"],keys[i]));if(!n||n>maxima[i]||(i==1&&n<=32))Fail("capability_budget");}
  }
  for(const char *k:{"protocol","model","game","repentogon"})if(At(local,k)!=At(remote,k))Fail("incompatible_version");
  // 二进制布局也使用必需能力匹配，单独报告版本协商失败，方便区分其他缺失能力。
  for(const auto &r:At(remote,"required").GetArray()){
    bool found=false;for(const auto &s:At(local,"supported").GetArray())if(r==s)found=true;
    if(!found)Fail(Str(r).starts_with("binary-state-v1-layout-")?"binary_layout_version_mismatch":"unsupported_capability");
  }
  for(const auto &r:At(local,"required").GetArray()){
    bool found=false;for(const auto &s:At(remote,"supported").GetArray())if(r==s)found=true;
    if(!found)Fail(Str(r).starts_with("binary-state-v1-layout-")?"binary_layout_version_mismatch":"unsupported_capability");
  }
  if(At(local,"mods")!=At(remote,"mods"))Fail("mod_version_mismatch");
  if(At(local,"model_categories")!=At(remote,"model_categories"))Fail("category_version_mismatch");
  if(At(local,"resources")!=At(remote,"resources"))Fail("resource_version_mismatch");
  Json result=Copy(local);auto &a=result.GetAllocator();
  for(auto i=result["limits"].MemberBegin();i!=result["limits"].MemberEnd();++i){const auto key=Str(i->name);const auto n=(std::min)(Unsigned(i->value),Unsigned(At(At(remote,"limits"),key.c_str())));if(!n)Fail("invalid_budget");i->value.SetUint64(n);}
  // 模型能力与原生应用能力分开；没有共同applier不谎称游戏Client已经能显示世界。
  Value apply(rapidjson::kArrayType);for(const auto &v:local["native_appliers"].GetArray())for(const auto &r:remote["native_appliers"].GetArray())if(v==r){Value x;x.CopyFrom(v,a);apply.PushBack(x,a);break;}
  Put(result,"native_appliers",std::move(apply),a);return result;
}
} // namespace cs::replica
