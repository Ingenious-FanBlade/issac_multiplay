#include "replica/model.h"
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
  if(depth>l.depth||++nodes>l.nodes) Fail("value_budget");
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
Value Authority(Allocator &a,const std::string &reason){
  Value out(rapidjson::kObjectType);Put(out,"status","AuthorityOnly",a);Put(out,"reason",reason,a);return out;
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
void CheckValue(const Value &v,Limits l){
  std::uint64_t nodes=0;Check(v,l,0,nodes);
  // 总字符串与节点开销也有上限，避免大量小字符串绕过单字符串限制。
  std::uint64_t bytes=24;auto count=[&](auto &&self,const Value &x)->void{
    bytes+=x.IsString()?std::uint64_t(x.GetStringLength())+5:9;if(bytes>l.bytes)Fail("value_byte_budget");
    if(x.IsArray())for(const auto &e:x.GetArray())self(self,e);
    if(x.IsObject())for(auto i=x.MemberBegin();i!=x.MemberEnd();++i){self(self,i->name);self(self,i->value);}
  };count(count,v);
}
void ValidateModel(const Value &v,Limits limits){
  CheckValue(v,limits);Model(v);const auto scope=ReadScope(v["scope"]);Unsigned(At(v,"event_watermark"));
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
    At(component,"state");
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
  if(session!=session_||!epoch||run.empty()||run=="0")Fail("invalid_source_generation");
  if(source_epoch_&&(source_epoch_!=epoch||source_run_!=run))Fail("source_run_or_epoch_changed");
  source_epoch_=epoch;source_run_=run;
}
Value Registry::Key(unsigned space,const std::string &local,const std::string &use,Allocator &a){
  if(!session_||!run_||space<1||space>14||local.empty()||local=="0") Fail("invalid_subject");
  const auto base=std::to_string(space)+":"+local;
  auto resolved=use;
  if(!use.empty()){
    // 先遇到无use关联，再遇到其首个主体时合并占位key；已知旧use后的复用仍分配新ID。
    if(!current_uses_.count(base)){if(auto placeholder=keys_.find(base+":");placeholder!=keys_.end()){const auto id=placeholder->second;keys_.erase(placeholder);keys_.emplace(base+":"+use,id);}}
    current_uses_[base]=use;
  }else if(auto it=current_uses_.find(base);it!=current_uses_.end())resolved=it->second;
  const auto name=base+":"+resolved;auto it=keys_.find(name);
  if(it==keys_.end()){
    if(keys_.size()>=262144||next_==UINT64_MAX) Fail("identity_exhausted");
    it=keys_.emplace(name,++next_).first;
  }
  Value out(rapidjson::kObjectType);Put(out,"run",Decimal(run_,a),a);Put(out,"space",Value(space),a);Put(out,"id",Decimal(it->second,a),a);return out;
}
Value Registry::Project(const Value &v,Allocator &a,std::uint64_t epoch,unsigned depth){
  if(depth>64) Fail("projection_depth");
  if(v.IsArray()){Value out(rapidjson::kArrayType);for(const auto &x:v.GetArray())out.PushBack(Project(x,a,epoch,depth+1),a);return out;}
  if(!v.IsObject()) {Value out;out.CopyFrom(v,a);return out;}
  if(v.HasMember("session")&&Unsigned(v["session"])!=session_) Fail("cross_session_subject");
  if(v.HasMember("epoch")&&v["epoch"].IsString()&&Unsigned(v["epoch"])!=epoch) Fail("cross_epoch_subject");
  // Entity／Grid与所有相关身份统一转换，绝不把本地Index／seed／地址当key。
  if(v.HasMember("space")&&v.HasMember("local_id")){
    Value out(rapidjson::kObjectType);const auto space=unsigned(Unsigned(v["space"]));
    const auto local=Str(v["local_id"]);std::string status=v.HasMember("resolution")?Str(v["resolution"]):v.HasMember("presence")&&Str(v["presence"])!="Known"?Str(v["presence"]):"KnownCurrent";
    Put(out,"type","reference",a);Put(out,"status",status,a);
    if(local!="0"&&!local.empty()&&(status=="KnownCurrent"||status=="KnownHistorical"||status=="Retired")){
      Put(out,"target",Key(space,std::to_string(epoch)+"/"+local,v.HasMember("use")?Str(v["use"]):"",a),a);
    }
    if(v.HasMember("revision"))Put(out,"shape_revision",Project(v["revision"],a,epoch,depth+1),a);
    for(const char *k:{"control_relation","control_group_membership"})if(v.HasMember(k))Put(out,k,Project(v[k],a,epoch,depth+1),a);
    return out;
  }
  Value out(rapidjson::kObjectType);
  for(auto it=v.MemberBegin();it!=v.MemberEnd();++it){
    const auto key=Str(it->name);
    if(IsInternal(key)) Fail("native_pointer_or_handle");
    if(key=="session"||key=="epoch") continue;
    Value x;
    if(const auto *p=Policy(key);p&&Str((*p)["ownership"])!="ReplicatedState"){
      x=Authority(a,Str((*p)["reason"]));Put(x,"status",Str((*p)["ownership"]),a);
    }else if(key=="fields"&&it->value.IsObject()){
      x.SetObject();
      for(auto f=it->value.MemberBegin();f!=it->value.MemberEnd();++f){
        const auto id=Str(f->name);const auto *rule=Policy(id);Value field;
        if(!rule){field.SetObject();Put(field,"status","Unsupported",a);Put(field,"reason","undeclared_member",a);}
        else if(Str((*rule)["ownership"])!="ReplicatedState") {field=Authority(a,Str((*rule)["reason"]));Put(field,"status",Str((*rule)["ownership"]),a);}
        else field=Project(f->value,a,epoch,depth+1);
        x.AddMember(String(id,a),field,a);
      }
    }else{
      unsigned space=0;
      if(key=="participant_local_id")space=9;else if(key=="control_group_local_id")space=10;
      else if(key=="floor")space=4;else if(key=="dimension")space=5;else if(key=="descriptor")space=6;else if(key=="room_instance")space=7;else if(key=="run")space=3;
      if(space&&it->value.IsString()&&Str(it->value)!="0")x=Key(space,std::to_string(epoch)+"/"+Str(it->value),"",a);
      else x=Project(it->value,a,epoch,depth+1);
    }
    out.AddMember(String(key,a),x,a);
  }
  if(v.HasMember("type")&&Str(v["type"])=="Sprite"){
    Put(out,"type","resource_pose",a);Value descriptor(rapidjson::kObjectType);
    Put(descriptor,"category","anm2",a);Put(descriptor,"content_version","REPENTOGON-1.1.3/J273;mod-version-required",a);Put(descriptor,"status","Unverified",a);
    if(v.HasMember("fields")&&v["fields"].HasMember("Sprite.GetFilename"))Put(descriptor,"logical_path",Project(v["fields"]["Sprite.GetFilename"],a,epoch,depth+1),a);
    else Put(descriptor,"status","Unavailable",a);
    Put(out,"resource",std::move(descriptor),a);
  }
  return out;
}

Json CaptureRows(const Scope &scope,std::uint64_t revision,const std::vector<Json> &rows,Registry &registry,Limits limits){
  Json out(rapidjson::kObjectType);auto &a=out.GetAllocator();ReadScope(ScopeValue(scope,a));if(!revision)Fail("zero_revision");
  Put(out,"format","isaac.cs.replica.model",a);Put(out,"scope",ScopeValue(scope,a),a);Put(out,"revision",Decimal(revision,a),a);
  Value components(rapidjson::kObjectType),missing(rapidjson::kArrayType);
  std::set<unsigned> domains;std::string window;bool complete=true;std::uint64_t watermark=0;std::uint64_t epoch=0;
  for(const auto &r:rows){
    if(Str(At(r,"format"))!="isaac.cs.observation"||Unsigned(At(r,"version"))!=2||Unsigned(At(r,"schema_version"))!=2)Fail("source_schema");
    const auto &c=At(r,"context");if(Unsigned(At(c,"session"))!=scope.session)Fail("cross_session");
    const auto e=Unsigned(At(c,"epoch"));if(!epoch)epoch=e;else if(epoch!=e)Fail("cross_epoch");
    const auto &run=At(c,"run");if(Str(At(run,"presence"))!="Known")Fail("source_run_unknown");registry.BindSource(scope.session,e,Str(At(run,"local_id")));
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
    Put(component,"capture",std::move(provenance),a);
    const auto coverage=Str(At(At(r,"coverage"),"status"));
    if(coverage!="Complete"&&coverage!="NotApplicable")complete=false;
    Put(component,"coverage",coverage,a);
    Value reasons;reasons.CopyFrom(At(r["coverage"],"reasons"),a);Put(component,"reasons",std::move(reasons),a);
    if(domain==9){Put(component,"ownership","AuthorityOnly",a);Put(component,"state",Authority(a,"all gameplay RNG stays at Authority;world results are replicated"),a);}
    else {Put(component,"ownership","ReplicatedState",a);auto state=registry.Project(At(r,"payload"),a,epoch);Dependencies(state,revision,a);Put(component,"state",std::move(state),a);}
    // 成员完整性由域来源明确声明；Partial不能被Delta当作全成员删除依据。
    Put(component,"membership_complete",Value(coverage=="Complete"),a);
    components.AddMember(String(DomainName(domain),a),component,a);
  }
  for(unsigned d=1;d<=11;++d)if(!domains.count(d)){missing.PushBack(String(DomainName(d),a),a);complete=false;}
  Put(out,"components",std::move(components),a);Put(out,"missing_domains",std::move(missing),a);
  Put(out,"publishable",Value(complete),a);Put(out,"event_watermark",Decimal(watermark,a),a);
  Put(out,"native_applied",Value(false),a);ValidateModel(out,limits);return out;
}
Json Capture(const Scope &scope,std::uint64_t revision,const std::vector<world::observation::ObservationEnvelope> &values,Registry &registry,Limits limits){
  std::vector<Json> rows;rows.reserve(values.size());for(const auto &v:values)rows.push_back(Parse(world::observation::EncodeEnvelope(v),limits));return CaptureRows(scope,revision,rows,registry,limits);
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
  Put(m.body,"value",domain==9?Authority(a,"independent RNG history remains AuthorityOnly"):registry.Project(At(r,"payload"),a,Unsigned(At(c,"epoch"))),a);
  return m;
}

namespace {
// 组件重建沿用15的替换／删除语义；调用者必须显式声明本次完整域，拒绝部分成员作为删除依据。
void CompleteDomains(const Value &model,const std::vector<std::string> &domains){
 if(domains.empty()){if(model["publishable"]!=true)Fail("incomplete_membership");return;}
 std::set<std::string> unique(domains.begin(),domains.end());if(unique.size()!=domains.size()||model["components"].MemberCount()!=domains.size())Fail("component_delta_domains");
 for(const auto &name:domains){const auto &component=At(model["components"],name.c_str());if(component["coverage"]!="Complete"||component["membership_complete"]!=true)Fail("incomplete_membership");}
}
}
Json Difference(const Value &baseline,const Value &target,Limits limits,const std::vector<std::string> &domains){
  ValidateModel(baseline,limits);ValidateModel(target,limits);if(ReadScope(baseline["scope"])!=ReadScope(target["scope"]))Fail("delta_scope");
  if(Unsigned(target["revision"])<=Unsigned(baseline["revision"]))Fail("delta_revision");
  CompleteDomains(target,domains);CompleteDomains(baseline,domains);
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
Json Reconstruct(const Value &baseline,const Value &delta,Limits limits,const std::vector<std::string> &domains){
  CheckValue(delta,limits);ValidateModel(baseline,limits);CompleteDomains(baseline,domains);
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
  Put(out,"components",std::move(components),a);ValidateModel(out,limits);CompleteDomains(out,domains);
  if(out["revision"]!=delta["target"]||ReadScope(out["scope"])!=ReadScope(delta["scope"]))Fail("delta_target");return out;
}
Message Applied(const Scope &s,std::uint64_t snapshot,std::uint64_t watermark,bool committed){
  if(!committed||!snapshot)Fail("native_commit_required");Message m;m.kind=Kind::AppliedAck;m.sequence=snapshot;m.body.SetObject();auto &a=m.body.GetAllocator();
  Put(m.body,"scope",ScopeValue(s,a),a);Put(m.body,"snapshot",Decimal(snapshot,a),a);Put(m.body,"event_watermark",Decimal(watermark,a),a);return m;
}
Message InputProgress(const Scope &s,std::uint64_t participant,std::uint64_t generation,std::uint64_t sequence,std::uint64_t tick){
  if(participant>3||!generation||!sequence||!tick)Fail("input_ack_range");Message m;m.kind=Kind::InputAck;m.sequence=sequence;m.body.SetObject();auto &a=m.body.GetAllocator();
  Put(m.body,"scope",ScopeValue(s,a),a);Put(m.body,"participant",Value(unsigned(participant)),a);Put(m.body,"generation",Decimal(generation,a),a);Put(m.body,"input_sequence",Decimal(sequence,a),a);Put(m.body,"processed_tick",Decimal(tick,a),a);return m;
}
// 网络身份到可信Participant的选择仍由16的主线程连接owner完成，不能由payload改投。
Message InputIntent(const Scope &s,unsigned participant,std::uint64_t generation,std::uint64_t sequence,std::uint64_t target,const core::InputStateDraftV2 &state){
  if(participant>3||!generation||!sequence||!target||!core::Valid(state))Fail("input_intent_invalid");
  Message m;m.kind=Kind::Input;m.sequence=sequence;m.body.SetObject();auto &a=m.body.GetAllocator();
  Put(m.body,"scope",ScopeValue(s,a),a);Put(m.body,"participant",Value(participant),a);Put(m.body,"generation",Decimal(generation,a),a);Put(m.body,"input_sequence",Decimal(sequence,a),a);Put(m.body,"target_tick",Decimal(target,a),a);
  Value movement(rapidjson::kArrayType),aim(rapidjson::kArrayType);movement.PushBack(Value(int(state.move_x)),a).PushBack(Value(int(state.move_y)),a);aim.PushBack(Value(int(state.aim_x)),a).PushBack(Value(int(state.aim_y)),a);
  Put(m.body,"movement",std::move(movement),a);Put(m.body,"aim",std::move(aim),a);Put(m.body,"held",Value(state.held_flags),a);Put(m.body,"edge",Value(state.edge_flags),a);return m;
}
Json Capabilities(){return Parse(R"({"protocol":1,"model":1,"game":"v1.9.7.12.J273","repentogon":"1.1.3","required":["typed-values-v1","stable-keys-v1","scope-components-v1","complete-delta-v1"],"supported":["typed-values-v1","stable-keys-v1","scope-components-v1","complete-delta-v1"],"domains":[1,2,3,4,5,6,7,8,9,10,11],"model_categories":{"entity":["Player","NPC","Delirium","Tear","Projectile","Bomb","Laser","Knife","Pickup","Familiar","Effect","Slot","NativeText","NativeDarkness"],"grid":"all target-version categories;specialized fields and explicit Unsupported","modes":["Normal","Hard","Greed","Greedier","Challenge","SeedEffects","DailyNonRanking"],"resource_pose":["ANM2","texture-replacements","main","overlay","layers","tracks","shader","camera","transition"],"unexposed":"Unavailable;never synthesized"},"native_appliers":[],"mods":[],"resources":{"base":"J273","status":"InstallationRequired"},"limits":{"bytes":33554432,"datagram":1200,"candidates":2,"fragments":32768,"timeout_ms":5000}})");}
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
  for(const auto &r:At(remote,"required").GetArray()){
    bool found=false;for(const auto &s:At(local,"supported").GetArray())if(r==s)found=true;if(!found)Fail("unsupported_capability");
  }
  for(const auto &r:At(local,"required").GetArray()){
    bool found=false;for(const auto &s:At(remote,"supported").GetArray())if(r==s)found=true;if(!found)Fail("unsupported_capability");
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
