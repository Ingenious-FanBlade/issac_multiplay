#include "replica/stream_model.h"
#include "core/deferred_values.h"
#include "replica/codec.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <bit>
#include <cstring>

namespace cs::replica::stream {
namespace {
[[noreturn]] void Fail(const char *s){throw ContractError(s);}
bool Key(const SubjectKey &k){return k.run&&k.id&&k.space>=1&&k.space<=14;}
bool ScopeValid(const WorldScope &s){return s.session&&s.run&&s.floor&&s.dimension&&s.room&&s.scene;}
bool Id(const RangeId &id){return unsigned(id.kind)>=1&&unsigned(id.kind)<=6&&!id.name.empty()&&id.name.size()<=128&&(id.kind==RangeKind::PlayerFast?Key(id.body):id.body==SubjectKey{});}
bool Versions(const std::vector<Version> &vs){std::set<RangeId> ids;for(const auto &v:vs)if(!Id(v.id)||!v.revision||!ids.insert(v.id).second)return false;return true;}
bool Has(const std::vector<std::uint32_t> &v,std::uint32_t id){return std::find(v.begin(),v.end(),id)!=v.end();}
// 与纯值codec判等语义一致：整数种类/双精度bits及数组顺序保留，对象键顺序无关。
// 常见同序成员线性比较；不为每次ordinary比较重新排序、分配和编码两份完整字节树。
bool ValueEqual(const Value &a,const Value &b){
  if(a.GetType()!=b.GetType())return false;
  if(a.IsNull())return true;
  if(a.IsBool())return a.GetBool()==b.GetBool();
  if(a.IsNumber()){
    if(a.IsUint64()!=b.IsUint64()||a.IsInt64()!=b.IsInt64()||a.IsDouble()!=b.IsDouble())return false;
    if(a.IsUint64())return a.GetUint64()==b.GetUint64();
    if(a.IsInt64())return a.GetInt64()==b.GetInt64();
    return std::bit_cast<std::uint64_t>(a.GetDouble())==std::bit_cast<std::uint64_t>(b.GetDouble());
  }
  if(a.IsString())return a.GetStringLength()==b.GetStringLength()&&!std::memcmp(a.GetString(),b.GetString(),a.GetStringLength());
  if(a.IsArray()){if(a.Size()!=b.Size())return false;for(rapidjson::SizeType i=0;i<a.Size();++i)if(!ValueEqual(a[i],b[i]))return false;return true;}
  if(a.MemberCount()!=b.MemberCount())return false;
  bool same_order=true;auto right=b.MemberBegin();
  for(auto left=a.MemberBegin();left!=a.MemberEnd();++left,++right)if(!ValueEqual(left->name,right->name)){same_order=false;break;}
  right=b.MemberBegin();
  for(auto left=a.MemberBegin();left!=a.MemberEnd();++left){
    if(!same_order)right=b.FindMember(left->name);
    if(right==b.MemberEnd()||!ValueEqual(left->value,right->value))return false;
    if(same_order)++right;
  }
  return true;
}
bool AtomEqual(const Atom &a,const Atom &b){
  if(a.index()!=b.index())return false;
  if(const auto *v=std::get_if<std::shared_ptr<const Json>>(&a)){const auto &w=std::get<std::shared_ptr<const Json>>(b);return *v&&w?ValueEqual(**v,*w):!*v&&!w;}
  if(const auto *v=std::get_if<double>(&a))return std::bit_cast<std::uint64_t>(*v)==std::bit_cast<std::uint64_t>(std::get<double>(b));
  return a==b;
}
Decision D(Admission a,const char *r){return {a,r};}
Decision Membership(const RangeData &r,const Store &store){
  if(r.id.kind!=RangeKind::WorldDynamic&&r.id.kind!=RangeKind::PlayerFast)return D(Admission::Complete,"stream_membership_ready");
  std::map<SubjectKey,Member> expected;
  for(const auto &v:r.dependencies)if(v.id.kind==RangeKind::Structure){const auto structure=store.Find(v);if(!structure)return D(Admission::WaitingDependencies,"stream_structure_missing");for(const auto &m:structure->members)if(r.id.kind==RangeKind::PlayerFast?m.key==r.id.body:m.category!="Player")if(auto [it,inserted]=expected.emplace(m.key,m);!inserted&&it->second!=m)return D(Admission::Invalid,"stream_structure_member_conflict");}
  if(expected.size()!=r.members.size())return D(Admission::IncompleteMembership,"stream_structure_membership_mismatch");
  for(const auto &m:r.members){const auto i=expected.find(m.key);if(i==expected.end()||i->second!=m)return D(Admission::Invalid,"stream_structure_use_or_shape_mismatch");}
  return D(Admission::Complete,"stream_membership_ready");
}
std::uint64_t ArenaBytes(const Json &v){
  return OwnedJsonBytes(v);
}
void Pure(const Value &v){
  if(v.IsArray())for(const auto &x:v.GetArray())Pure(x);
  if(v.IsObject())for(auto i=v.MemberBegin();i!=v.MemberEnd();++i){const std::string key(i->name.GetString(),i->name.GetStringLength());if(key=="pointer"||key=="ptr"||key=="vptr"||key=="address"||key=="native_address"||key=="device_pointer"||key=="handle"||key=="assignment_key")Fail("stream_native_pointer");Pure(i->value);}
}
}
const Json &Catalog(){static const Json catalog=Parse(
#include "stream_catalog.inc"
);return catalog;}
bool Field::operator==(const Field &b) const {return id==b.id&&status==b.status&&AtomEqual(value,b.value);}
WorldScope World(const Scope &s) noexcept {return {s.session,s.run,s.floor,s.dimension,s.room,s.scene};}
Scope Connected(const WorldScope &s,std::uint64_t c){if(!ScopeValid(s)||!c)Fail("stream_scope");return {s.session,c,s.run,s.floor,s.dimension,s.room,s.scene};}
std::uint64_t Footprint(const RangeData &d){
  std::uint64_t bytes=sizeof(d)+d.id.name.capacity()+d.members.capacity()*sizeof(Member)+d.records.capacity()*sizeof(Record);
  const auto add=[&](std::uint64_t n){if(n>UINT64_MAX-bytes)Fail("stream_size_overflow");bytes+=n;};
  for(const auto &v:d.dependencies)add(sizeof(v)+v.id.name.capacity());
  for(const auto &v:d.required)add(sizeof(v)+v.id.name.capacity());
  for(const auto &m:d.members)add(m.category.capacity());
  for(const auto &r:d.records){add(r.member.category.capacity()+r.layout.capacity()+r.fields.capacity()*sizeof(Field));for(const auto &f:r.fields){
    if(const auto *s=std::get_if<std::string>(&f.value))add(s->capacity());
    if(const auto *v=std::get_if<std::shared_ptr<const Json>>(&f.value);v&&*v)add(ArenaBytes(**v));}}
  if(d.components)add(ArenaBytes(*d.components));return bytes;
}
Decision Validate(const RangeData &d,const Layouts &layouts,std::uint64_t limit) noexcept {
  try {
    if(!ScopeValid(d.world)||!Id(d.id)||!d.revision||d.revision==UINT64_MAX||!d.sample.epoch||!d.sample.sequence||d.sample.sequence==UINT64_MAX||!d.sample.tick||!d.sample.outer||!d.sample.owner||unsigned(d.sample.kind)>1)return D(Admission::Invalid,"stream_identity_or_sample");
    if(!d.complete)return D(Admission::Invalid,"stream_partial_range");
    if(!Versions(d.dependencies)||!Versions(d.required)||d.dependencies.size()>64||d.required.size()>64)return D(Admission::Invalid,"stream_dependencies");
    for(const auto &v:d.dependencies)if(v.id==d.id)return D(Admission::Invalid,"stream_self_dependency");
    for(const auto &v:d.required)if(v.id==d.id||v.id.kind==RangeKind::Manifest)return D(Admission::Invalid,"stream_manifest_dependency");
    if(d.id.kind!=RangeKind::Manifest&&(!d.required.empty()||d.native_domains||d.retained_domains||d.event_watermark))return D(Admission::Invalid,"stream_manifest_only_field");
    if(d.id.kind==RangeKind::Manifest&&(d.required.empty()||!d.records.empty()||!d.membership_complete||(d.native_domains&d.retained_domains)||((d.native_domains|d.retained_domains)&~2047ull)))return D(Admission::Invalid,"stream_manifest");
    if(d.id.kind!=RangeKind::PlayerFast&&d.sample.kind!=CaptureKind::Ordinary)return D(Admission::Invalid,"stream_ordinary_required");
    if(d.id.kind==RangeKind::PlayerFast){
      if(!d.sample.last_native_update_tick||d.sample.last_native_update_tick!=d.sample.tick||!d.sample.binding_revision||!d.sample.required_world_sequence||d.members.size()!=1||d.members[0].key!=d.id.body||d.members[0].category!="Player"||d.id.body.space!=1)return D(Admission::Invalid,"stream_body_witness");
    } else if(d.sample.last_native_update_tick||d.sample.binding_revision||d.sample.required_world_sequence)return D(Admission::Invalid,"stream_body_only_field");
    if((d.id.kind==RangeKind::Structure||d.id.kind==RangeKind::WorldDynamic||d.id.kind==RangeKind::PlayerFast)&&!d.membership_complete)return D(Admission::IncompleteMembership,"stream_membership_partial");
    if(d.members.size()>16384||d.records.size()>16384)return D(Admission::OverBudget,"stream_member_budget");
    std::map<SubjectKey,Member> members;
    for(const auto &m:d.members){if(!Key(m.key)||m.key.run!=d.world.run||!m.use||m.category.empty()||!members.emplace(m.key,m).second)return D(Admission::Invalid,"stream_member_identity");if(d.id.kind==RangeKind::WorldDynamic&&m.category=="Player")return D(Admission::Invalid,"stream_player_field_owner");}
    if(d.id.kind==RangeKind::PlayerFast||d.id.kind==RangeKind::WorldDynamic){bool structure=false;for(const auto &v:d.dependencies)if(v.id.kind==RangeKind::Structure)structure=true;if(!structure)return D(Admission::Invalid,"stream_structure_required");}
    std::set<SubjectKey> recorded;
    for(const auto &r:d.records){
      const auto member=members.find(r.member.key);if(member==members.end()||member->second!=r.member||!recorded.insert(r.member.key).second)return D(Admission::Invalid,"stream_record_membership");
      const auto layout=layouts.find(r.layout);if(r.layout.empty()||r.layout.size()>128||layout==layouts.end()||layout->second.kind!=d.id.kind)return D(Admission::Invalid,"stream_layout");
      const auto &l=layout->second;if(l.types.size()!=l.fields.size())return D(Admission::Invalid,"stream_layout_types");std::set<std::uint32_t> fields;
      for(const auto &f:r.fields){
        if(!f.id||!Has(l.fields,f.id)||!fields.insert(f.id).second)return D(Admission::Invalid,"stream_field_identity");
        if(f.status==FieldStatus::Value){const auto type=l.types.find(f.id);if(type==l.types.end()||type->second!=f.value.index()||type->second<1||type->second>6)return D(Admission::Invalid,"stream_field_type");if(const auto *n=std::get_if<double>(&f.value);n&&!std::isfinite(*n))return D(Admission::Invalid,"stream_nonfinite");if(const auto *v=std::get_if<std::shared_ptr<const Json>>(&f.value)){if(!*v)return D(Admission::MissingField,"stream_value_missing");CheckValue(**v);Pure(**v);}}
        else {if(!std::holds_alternative<std::monostate>(f.value))return D(Admission::Invalid,"stream_status_payload");if(!(f.status==FieldStatus::Empty&&Has(l.empty,f.id))&&!(f.status==FieldStatus::Null&&Has(l.nullable,f.id))&&!(f.status==FieldStatus::NotApplicable&&Has(l.not_applicable,f.id)))return D(Admission::MissingField,"stream_required_field_unavailable");}
      }
      if(fields.size()!=l.fields.size())return D(Admission::MissingField,"stream_required_field_missing");
    }
    if((d.id.kind==RangeKind::Structure||d.id.kind==RangeKind::WorldDynamic||d.id.kind==RangeKind::PlayerFast)&&recorded.size()!=members.size())return D(Admission::MissingField,"stream_required_record_missing");
    if(d.components){if(d.id.kind!=RangeKind::Context||!d.components->IsObject())return D(Admission::Invalid,"stream_cold_components");CheckValue(*d.components);Pure(*d.components);}
    if(Footprint(d)>limit)return D(Admission::OverBudget,"stream_value_budget");
    return D(Admission::Complete,"stream_complete");
  }catch(...){return D(Admission::OverBudget,"stream_validation_budget");}
}
Range Freeze(RangeData d,const Layouts &l,std::uint64_t limit){
  const auto before=Validate(d,l,limit);if(!before)Fail(before.reason);
  // 分配前为独立副本申请额度；输入别名的租约不能替新内容付费。
  std::uint64_t bound=Footprint(d)+128;
  const auto account=[&](const Json &v){bound-=OwnedJsonBytes(v);bound+=OwnedValueUpperBound(v);};
  for(const auto &r:d.records)for(const auto &f:r.fields)if(const auto *v=std::get_if<std::shared_ptr<const Json>>(&f.value);v&&*v)account(**v);
  if(d.components)account(*d.components);
  // 输入树由调用者租约持有；此处只预付将独立拥有的新树及容器，不重复计入旧副本。
  auto lease=memory::Reserve(memory::Pool::Values,bound,"range_freeze");
  // const shared_ptr本身不能阻止外部别名写树；边界总是深复制一次。
  for(auto &r:d.records)for(auto &f:r.fields)if(auto *v=std::get_if<std::shared_ptr<const Json>>(&f.value);v&&*v)*v=std::make_shared<const Json>(Copy(**v));
  if(d.components)d.components=std::make_shared<const Json>(Copy(*d.components));
  const auto valid=Validate(d,l,limit);if(!valid)Fail(valid.reason);const auto actual=Footprint(d)+128;if(actual>bound)Fail("stream_freeze_footprint");lease->Resize(actual);d.memory=std::move(lease);return memory::DeferredValue<RangeData>(std::move(d));
}
Range FreezeCaptured(RangeData d,const Layouts &l,CaptureOwnership,const memory::Lease &prepaid,std::uint64_t limit){
  // Capture的Tree只接局部唯一DOM或深复制const来源，不对外暴露可变别名。
  // 即使具备移交资格仍执行完整图/身份/字段/依赖校验；use_count不是封闭性的证明。
  const auto valid=Validate(d,l,limit);if(!valid)Fail(valid.reason);
  const auto bytes=Footprint(d)+128;d.memory=memory::Reserve(memory::Pool::Values,0,"range_capture_transfer");
  if(!prepaid||bytes>prepaid->Bytes())Fail("stream_capture_transfer_budget");
  d.memory->TransferFrom(*prepaid,bytes);
  return memory::DeferredValue<RangeData>(std::move(d));
}
bool Equal(const RangeData &a,const RangeData &b){
  const auto &x=a.sample;const auto &y=b.sample;
  return a.world==b.world&&a.id==b.id&&a.revision==b.revision&&a.complete==b.complete&&a.membership_complete==b.membership_complete&&a.dependencies==b.dependencies&&a.members==b.members&&a.records==b.records&&a.required==b.required&&a.native_domains==b.native_domains&&a.retained_domains==b.retained_domains&&a.event_watermark==b.event_watermark&&
    x.epoch==y.epoch&&x.sequence==y.sequence&&x.tick==y.tick&&x.outer==y.outer&&x.owner==y.owner&&x.mutation==y.mutation&&x.loss==y.loss&&x.last_native_update_tick==y.last_native_update_tick&&x.required_world_sequence==y.required_world_sequence&&x.binding_revision==y.binding_revision&&x.kind==y.kind&&
    (a.components&&b.components?ValueEqual(*a.components,*b.components):!a.components&&!b.components);
}
Range Store::Find(const Version &v) const noexcept {const auto i=values_.find(v);return i==values_.end()?Range{}:i->second;}
Decision Store::Admit(Range r,const Layouts &l) noexcept {
  if(!r)return D(Admission::Invalid,"stream_null_range");
  auto valid=Validate(*r,l,budget_);if(!valid)return valid;if(r->world!=world_)return D(Admission::Invalid,"stream_wrong_world");
  try {
    const Version v{r->id,r->revision};if(const auto old=Find(v))return Equal(*old,*r)?D(Admission::Duplicate,"stream_duplicate"):D(Admission::Conflict,"stream_version_conflict");
    std::size_t history=0;std::uint64_t bytes=Footprint(*r);for(const auto &[k,value]:values_){bytes+=Footprint(*value);if(k.id==r->id){++history;if(k.revision>r->revision||value->sample.sequence>=r->sample.sequence||value->sample.tick>r->sample.tick)return D(Admission::Stale,"stream_stale");if(value->sample.epoch!=r->sample.epoch)return D(Admission::Invalid,"stream_source_epoch_changed");}}
    if(bytes>budget_||history>=128||values_.size()>=8192)return D(Admission::OverBudget,"stream_store_budget");
    bool ready=true;for(const auto &d:r->dependencies)if(!Find(d))ready=false;for(const auto &d:r->required)if(!Find(d))ready=false;
    if(ready){const auto members=Membership(*r,*this);if(!members)return members;}
    values_.emplace(v,std::move(r));return ready?D(Admission::Complete,"stream_complete"):D(Admission::WaitingDependencies,"stream_waiting_dependencies");
  }catch(...){return D(Admission::OverBudget,"stream_store_allocation");}
}
std::vector<Range> Store::Compose(const RangeData &m) const {
  if(m.world!=world_||m.id.kind!=RangeKind::Manifest||!m.complete)Fail("stream_manifest");
  std::vector<Range> out;std::set<Version> visited,active;
  const auto add=[&](const auto &self,const Version &v)->void{if(active.contains(v))Fail("stream_dependency_cycle");if(visited.contains(v))return;if(active.size()>=64)Fail("stream_dependency_depth");active.insert(v);const auto r=Find(v);if(!r)Fail("stream_missing_dependency");for(const auto &d:r->dependencies)self(self,d);active.erase(v);visited.insert(v);out.push_back(r);};
  for(const auto &v:m.required)add(add,v);return out;
}
Decision Store::Commit(const Version &v) noexcept {
  try {
    const auto r=Find(v);if(!r)return D(Admission::Invalid,"stream_unknown_commit");
    for(const auto &d:r->dependencies){const auto a=applied_.find(d.id);const bool later_world=r->id.kind==RangeKind::PlayerFast&&d.id.kind==RangeKind::WorldDynamic&&a!=applied_.end()&&a->second->revision>=d.revision;if(a==applied_.end()||(!later_world&&a->second->revision!=d.revision))return D(Admission::WaitingDependencies,"stream_commit_dependency");}
    for(const auto &d:r->required){const auto a=applied_.find(d.id);if(a==applied_.end()||a->second->revision!=d.revision)return D(Admission::WaitingDependencies,"stream_manifest_not_applied");}
    const auto members=Membership(*r,*this);if(!members)return members;
    if(const auto a=applied_.find(v.id);a!=applied_.end()&&a->second->revision>=v.revision)return D(a->second->revision==v.revision?Admission::Duplicate:Admission::Stale,"stream_commit_old");
    if(r->id.kind==RangeKind::PlayerFast){bool world=false;for(const auto &[id,a]:applied_)if(id.kind==RangeKind::WorldDynamic&&a->sample.sequence>=r->sample.required_world_sequence)world=true;if(!world)return D(Admission::WaitingDependencies,"stream_body_world_dependency");}
    // 普通世界范围不包含Player公共动态字段；较新身体视图保留独立sample。
    applied_.insert_or_assign(v.id,r);
    if(r->id.kind==RangeKind::Structure)for(auto i=applied_.begin();i!=applied_.end();) {
      bool obsolete=false;if(i->first.kind==RangeKind::PlayerFast)for(const auto &d:i->second->dependencies)if(d.id==r->id&&d.revision!=r->revision)obsolete=true;
      if(obsolete)i=applied_.erase(i);else ++i;
    }
    return D(Admission::Complete,"stream_committed");
  }catch(...){return D(Admission::OverBudget,"stream_commit_allocation");}
}
void Store::Reset(WorldScope s) noexcept {applied_.clear();values_.clear();world_=s;}
void Store::ReleaseCold() noexcept {for(auto it=applied_.begin();it!=applied_.end();)if(it->first.kind==RangeKind::Manifest||it->second->components)it=applied_.erase(it);else ++it;for(auto it=values_.begin();it!=values_.end();)if(it->first.id.kind==RangeKind::Manifest||it->second->components)it=values_.erase(it);else ++it;}
void Store::RevokeBodies() noexcept {for(auto it=applied_.begin();it!=applied_.end();)if(it->first.kind==RangeKind::PlayerFast)it=applied_.erase(it);else ++it;}
void Store::Collect(const std::vector<Version> &roots){
  std::set<Version> keep;
  const auto visit=[&](const auto &self,const Version &v)->void{
    if(!keep.insert(v).second)return;
    const auto i=values_.find(v);if(i==values_.end())return;
    for(const auto &d:i->second->dependencies)self(self,d);
    for(const auto &d:i->second->required)self(self,d);
  };
  for(const auto &v:roots)visit(visit,v);
  for(const auto &[id,r]:applied_)visit(visit,Version{r->id,r->revision});
  for(const auto &[v,r]:values_)if(r.use_count()>1)visit(visit,v);
  for(auto i=values_.begin();i!=values_.end();)if(!keep.contains(i->first))i=values_.erase(i);else ++i;
}
std::uint64_t ColdModelUpperBound(const Store &store,const RangeData &m){
  std::uint64_t bytes=65536;
  const auto add=[&](std::uint64_t n){if(n>UINT64_MAX-bytes)Fail("value_memory_overflow");bytes+=n;};
  // 深复制前计入每个组件及版本重写的新字符串；移除旧成员不会归还arena空间。
  for(const auto &r:store.Compose(m))if(r->components){
    // 多份值合入同一arena会改变chunk边界；冷模型仍给合并与版本重写留保守废尾额度。
    add(2*OwnedValueUpperBound(*r->components)+65536);
    const auto visit=[&](const auto &self,const Value &v)->void{
      if(v.IsArray())for(const auto &x:v.GetArray())self(self,x);
      if(v.IsObject()){if(v.HasMember("required_revision"))add(128);for(auto i=v.MemberBegin();i!=v.MemberEnd();++i)self(self,i->value);}
    };visit(visit,*r->components);
  }
  return bytes;
}
Json ColdModel(const Store &store,const RangeData &m,std::uint64_t connection){
  Json out;out.SetObject();auto &a=out.GetAllocator();Value components(rapidjson::kObjectType);bool found=false;
  for(const auto &r:store.Compose(m))if(r->components){for(auto i=r->components->MemberBegin();i!=r->components->MemberEnd();++i){
    if(components.HasMember(i->name))Fail("stream_component_conflict");Value key,value;key.CopyFrom(i->name,a);value.CopyFrom(i->value,a);
    // 旧18—20入口要求十一域同普通窗口；不把混合来源改tick伪装成原子目标。
    if(!value.IsObject()||!value.HasMember("capture")||!value["capture"].IsObject())Fail("stream_cold_capture");const auto &capture=value["capture"];
    for(const auto *field:{"authority_tick","outer","owner","mutation_before","mutation_after"})if(!capture.HasMember(field))Fail("stream_cold_capture");
    if(Unsigned(capture["authority_tick"])!=m.sample.tick||Unsigned(capture["outer"])!=m.sample.outer||Unsigned(capture["owner"])!=m.sample.owner||Unsigned(capture["mutation_before"])!=m.sample.mutation||Unsigned(capture["mutation_after"])!=m.sample.mutation)Fail("stream_cold_mixed_window");
    Put(value,"version",Decimal(m.revision,a),a);
    // required_revision是合成模型版本，源capture/source版本保留原值。
    const auto rebase=[&](const auto &self,Value &v)->void{if(v.IsArray())for(auto &x:v.GetArray())self(self,x);if(v.IsObject()){if(v.HasMember("required_revision"))Put(v,"required_revision",Decimal(m.revision,a),a);for(auto j=v.MemberBegin();j!=v.MemberEnd();++j)self(self,j->value);}};
    rebase(rebase,value);components.AddMember(key,value,a);
  }found=true;}
  if(!found)Fail("stream_missing_cold_components");
  Put(out,"format",std::string("isaac.cs.replica.model"),a);Put(out,"scope",ScopeValue(Connected(m.world,connection),a),a);Put(out,"revision",Decimal(m.revision,a),a);Put(out,"components",std::move(components),a);Put(out,"missing_domains",Value(rapidjson::kArrayType),a);Put(out,"publishable",Value(true),a);Put(out,"event_watermark",Decimal(m.event_watermark,a),a);Put(out,"native_applied",Value(false),a);ValidateModel(out);return out;
}
} // namespace cs::replica::stream
