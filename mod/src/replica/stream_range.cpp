#include "replica/stream_codec.h"
#include "replica/stream_capture.h"
#include <set>

namespace cs::replica::stream {
namespace {
[[noreturn]] void Fail(const char *s){throw ContractError(s);}
const Value &At(const Value &v,const char *k){if(!v.IsObject()||!v.HasMember(k))Fail("stream_range_missing_field");return v[k];}
std::uint64_t N(const Value &v,const char *k){return Unsigned(At(v,k));}
std::string S(const Value &v){if(!v.IsString()||v.GetStringLength()>128)Fail("stream_range_string");return {v.GetString(),v.GetStringLength()};}
void Members(const Value &v,std::initializer_list<const char *> names){if(!v.IsObject()||v.MemberCount()!=names.size())Fail("stream_range_schema");for(auto k:names)At(v,k);}
void Number(Value &v,const char *k,std::uint64_t n,Allocator &a){Put(v,k,Decimal(n,a),a);}
Value KeyValue(SubjectKey k,Allocator &a){if(k==SubjectKey{})return Value();Value v(rapidjson::kObjectType);Number(v,"run",k.run,a);Number(v,"id",k.id,a);Number(v,"space",k.space,a);return v;}
SubjectKey Key(const Value &v,bool nullable=false){if(nullable&&v.IsNull())return {};Members(v,{"run","id","space"});if(!N(v,"run")||!N(v,"id")||N(v,"space")<1||N(v,"space")>14)Fail("stream_range_key");return {N(v,"run"),N(v,"id"),unsigned(N(v,"space"))};}
Value VersionValues(const std::vector<Version> &versions,Allocator &a){Value vs(rapidjson::kArrayType);for(const auto &x:versions){Value v(rapidjson::kObjectType);Number(v,"kind",unsigned(x.id.kind),a);Put(v,"name",x.id.name,a);Put(v,"body",KeyValue(x.id.body,a),a);Number(v,"revision",x.revision,a);vs.PushBack(v,a);}return vs;}
std::vector<Version> Versions(const Value &v){if(!v.IsArray()||v.Size()>64)Fail("stream_range_versions");std::vector<Version> out;for(const auto &x:v.GetArray()){Members(x,{"kind","name","body","revision"});if(N(x,"kind")<1||N(x,"kind")>6)Fail("stream_range_kind");out.push_back({{RangeKind(N(x,"kind")),S(x["name"]),Key(x["body"],true)},N(x,"revision")});}return out;}
Value MemberValue(const Member &m,Allocator &a){Value v(rapidjson::kObjectType);Put(v,"key",KeyValue(m.key,a),a);Number(v,"use",m.use,a);Number(v,"shape",m.shape,a);Put(v,"category",m.category,a);return v;}
Member ReadMember(const Value &v){Members(v,{"key","use","shape","category"});return {Key(v["key"]),N(v,"use"),N(v,"shape"),S(v["category"])};}
RangeData ParseData(const Message &m){
  if(m.kind!=Kind::Range)Fail("stream_not_range");const auto &b=m.body;const auto &p=At(b,"payload"),&w=At(b,"world");
  Members(p,{"body","sample","membership_complete","members","records","required","native_domains","retained_domains","event_watermark","components"});
  RangeData d;d.world={N(w,"session"),N(w,"run"),N(w,"floor"),N(w,"dimension"),N(w,"room"),N(w,"scene")};d.id={RangeKind(N(b,"range_kind")),S(At(b,"name")),Key(p["body"],true)};d.revision=N(b,"revision");
  if(At(b,"complete")!=true||!p["membership_complete"].IsBool())Fail("stream_range_completeness");d.complete=true;d.membership_complete=p["membership_complete"].GetBool();d.dependencies=Versions(At(b,"dependencies"));d.required=Versions(p["required"]);d.native_domains=N(p,"native_domains");d.retained_domains=N(p,"retained_domains");d.event_watermark=N(p,"event_watermark");
  const auto &s=p["sample"];Members(s,{"epoch","sequence","tick","outer","owner","mutation","loss","last_native_update_tick","required_world_sequence","binding_revision","kind"});
  if(N(s,"kind")>1)Fail("stream_capture_kind");
  d.sample={N(s,"epoch"),N(s,"sequence"),N(s,"tick"),N(s,"outer"),N(s,"owner"),N(s,"mutation"),N(s,"loss"),N(s,"last_native_update_tick"),N(s,"required_world_sequence"),N(s,"binding_revision"),CaptureKind(N(s,"kind"))};
  if(!p["members"].IsArray()||p["members"].Size()>16384||!p["records"].IsArray()||p["records"].Size()>16384)Fail("stream_range_member_budget");
  for(const auto &v:p["members"].GetArray())d.members.push_back(ReadMember(v));
  for(const auto &v:p["records"].GetArray()){
    Members(v,{"member","layout","fields"});Record record;record.member=ReadMember(v["member"]);record.layout=S(v["layout"]);if(!v["fields"].IsArray()||v["fields"].Size()>32768)Fail("stream_range_field_budget");
    for(const auto &x:v["fields"].GetArray()){
      Members(x,{"id","status","tag","value"});if(!N(x,"id")||N(x,"id")>UINT32_MAX||N(x,"status")>unsigned(FieldStatus::Truncated)||N(x,"tag")>7)Fail("stream_range_field_type");
      Field f;f.id=unsigned(N(x,"id"));f.status=FieldStatus(N(x,"status"));const auto &value=x["value"];
      switch(N(x,"tag")){
        case 0:if(!value.IsNull())Fail("stream_atom_null");break;
        case 1:if(!value.IsBool())Fail("stream_atom_bool");f.value=value.GetBool();break;
        case 2:if(!value.IsUint64())Fail("stream_atom_u64");f.value=value.GetUint64();break;
        case 3:if(!value.IsInt64())Fail("stream_atom_i64");f.value=value.GetInt64();break;
        case 4:if(!value.IsNumber())Fail("stream_atom_float");f.value=value.GetDouble();break;
        case 5:if(!value.IsString())Fail("stream_atom_string");f.value=std::string(value.GetString(),value.GetStringLength());break;
        case 6:if(!value.IsObject()&&!value.IsArray())Fail("stream_atom_tree");f.value=std::make_shared<const Json>(Copy(value));break;
        default:Fail("stream_atom_tag");
      }record.fields.push_back(std::move(f));
    }d.records.push_back(std::move(record));
  }
  if(!p["components"].IsNull())d.components=std::make_shared<const Json>(Copy(p["components"]));return d;
}
}
Message RangeMessage(const RangeData &d,std::uint64_t connection,std::uint64_t route){
  Message m;m.kind=Kind::Range;m.sequence=d.sample.sequence;m.connection=connection;m.route=route;m.body.SetObject();auto &a=m.body.GetAllocator();auto &b=m.body;
  Value w(rapidjson::kObjectType);Number(w,"session",d.world.session,a);Number(w,"run",d.world.run,a);Number(w,"floor",d.world.floor,a);Number(w,"dimension",d.world.dimension,a);Number(w,"room",d.world.room,a);Number(w,"scene",d.world.scene,a);Put(b,"world",std::move(w),a);Number(b,"range_kind",unsigned(d.id.kind),a);Put(b,"name",d.id.name,a);Number(b,"revision",d.revision,a);Number(b,"schema",kSchema,a);Put(b,"dependencies",VersionValues(d.dependencies,a),a);Put(b,"complete",Value(d.complete),a);
  Value p(rapidjson::kObjectType),s(rapidjson::kObjectType);Put(p,"body",KeyValue(d.id.body,a),a);const auto &sample=d.sample;
  Number(s,"epoch",sample.epoch,a);Number(s,"sequence",sample.sequence,a);Number(s,"tick",sample.tick,a);Number(s,"outer",sample.outer,a);Number(s,"owner",sample.owner,a);Number(s,"mutation",sample.mutation,a);Number(s,"loss",sample.loss,a);Number(s,"last_native_update_tick",sample.last_native_update_tick,a);Number(s,"required_world_sequence",sample.required_world_sequence,a);Number(s,"binding_revision",sample.binding_revision,a);Number(s,"kind",unsigned(sample.kind),a);Put(p,"sample",std::move(s),a);Put(p,"membership_complete",Value(d.membership_complete),a);
  Value members(rapidjson::kArrayType),records(rapidjson::kArrayType);for(const auto &member:d.members)members.PushBack(MemberValue(member,a),a);
  for(const auto &record:d.records){Value r(rapidjson::kObjectType),fields(rapidjson::kArrayType);Put(r,"member",MemberValue(record.member,a),a);Put(r,"layout",record.layout,a);for(const auto &field:record.fields){Value f(rapidjson::kObjectType),v;Number(f,"id",field.id,a);Number(f,"status",unsigned(field.status),a);Number(f,"tag",field.value.index(),a);
    switch(field.value.index()){case 0:break;case 1:v.SetBool(std::get<bool>(field.value));break;case 2:v.SetUint64(std::get<std::uint64_t>(field.value));break;case 3:v.SetInt64(std::get<std::int64_t>(field.value));break;case 4:v.SetDouble(std::get<double>(field.value));break;case 5:{const auto &text=std::get<std::string>(field.value);v.SetString(text.data(),rapidjson::SizeType(text.size()),a);break;}case 6:{const auto &tree=std::get<std::shared_ptr<const Json>>(field.value);if(!tree)Fail("stream_null_tree");v.CopyFrom(*tree,a);break;}default:Fail("stream_atom_tag");}Put(f,"value",std::move(v),a);fields.PushBack(f,a);}Put(r,"fields",std::move(fields),a);records.PushBack(r,a);}
  Put(p,"members",std::move(members),a);Put(p,"records",std::move(records),a);Put(p,"required",VersionValues(d.required,a),a);Number(p,"native_domains",d.native_domains,a);Number(p,"retained_domains",d.retained_domains,a);Number(p,"event_watermark",d.event_watermark,a);Value components;if(d.components)components.CopyFrom(*d.components,a);Put(p,"components",std::move(components),a);Put(b,"payload",std::move(p),a);ValidateMessage(m);return m;
}
RangeData RangeDataFromMessage(const Message &m){return m.range?*m.range:ParseData(m);}
void ValidateRangePayload(const Message &m){
  std::optional<RangeData> parsed;const auto &d=m.range?*m.range:parsed.emplace(ParseData(m));if(m.sequence!=d.sample.sequence)Fail("stream_range_sequence");
  // 报文不能用自己携带的字段反推“完整布局”；使用本地声明的同一来源/应用schema。
  const auto decision=Validate(d,CaptureLayouts());if(!decision)Fail(decision.reason);
}
Range ReadRange(const Message &m,const Layouts &l,std::uint64_t budget){ValidateMessage(m);if(m.range){const auto decision=Validate(*m.range,l,budget);if(!decision)Fail(decision.reason);return m.range;}return Freeze(ParseData(m),l,budget);}
} // namespace cs::replica::stream
