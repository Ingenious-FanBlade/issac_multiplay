#include "state/presentation_envelope.h"
#include <algorithm>
#include <cstring>
#include <numeric>
#include <tuple>

namespace cs::state {
namespace {
struct ResultPolicy {
  std::uint16_t layout, subject_layout, subject_field;
  bool body, event, cacheable, clock, topology;
  std::uint8_t paired_streams;
  std::uint32_t passes;
};
struct ResultRequirement { std::uint16_t layout, dependency_layout; };
#include "state_presentation_envelope.inc"
[[noreturn]] void Fail(const char *reason) { throw Gap("presentation_envelope",reason); }
std::uint64_t Bits(std::span<const std::byte> bytes) {
  if (bytes.empty() || bytes.size()>8) Fail("integer_width");
  std::uint64_t value=0;
  for (std::size_t i=0;i<bytes.size();++i) value|=std::uint64_t(std::to_integer<unsigned>(bytes[i]))<<(i*8);
  return value;
}
const ResultPolicy *FindPolicy(std::uint16_t layout) {
  const auto it=std::lower_bound(kResultPolicies.begin(),kResultPolicies.end(),layout,
      [](const ResultPolicy &p,std::uint16_t id){return p.layout<id;});
  return it==kResultPolicies.end() || it->layout!=layout ? nullptr : &*it;
}
const ResultPolicy &Policy(std::uint16_t layout) {
  const auto *policy=FindPolicy(layout);
  if (!policy) Fail("unregistered_result_purpose");
  return *policy;
}
void CheckBatch(const PresentationBatchContext &batch,PresentationEnvelopeLimits limits) {
  if (batch.layout_version!=LayoutVersion()) Fail("layout_version_mismatch");
  if (!batch.scope.session || !batch.scope.run || !batch.scope.scene) Fail("missing_scope_identity");
  if (batch.stream!=BodyStream::World && batch.stream!=BodyStream::Body) Fail("invalid_stream");
  if (!batch.member_epoch || !batch.resource_epoch || !batch.target_version ||
      batch.target_version<=batch.base_version || !batch.source_revision || !batch.presentation_batch)
    Fail("invalid_batch_versions_or_source");
  if (!limits.body.bytes || limits.body.bytes>(32u<<20) || (!limits.body.reliable && limits.body.bytes>65536) ||
      !limits.body.field_visits || !limits.body.depth || limits.body.depth>kCodecMaxDepth) Fail("invalid_limits");
  if (batch.stream==BodyStream::Body && (batch.event_watermark || batch.installed_event_watermark)) Fail("body_event_watermark");
  const auto stream=std::size_t(batch.stream);
  if (batch.paired_targets[stream] && batch.paired_targets[stream]!=batch.target_version) Fail("paired_self_target_mismatch");
  if (batch.paired_targets[1-stream] && (!limits.body.reliable || batch.paired_targets[stream]!=batch.target_version))
    Fail("paired_batch_requires_reliable_joint_target");
}
// 固定头也经过现有值codec；字段顺序、宽度和校验来自同次生成，手写代码不计算偏移。
template<class E> using FixedValues=std::array<std::uint64_t,std::size_t(E::count)>;
template<class E> std::uint64_t &At(FixedValues<E> &values,E field) { return values[std::size_t(field)]; }
class FixedSource final : public BodyValueSource {
public:
  explicit FixedSource(std::span<const std::uint64_t> values):values_(values) {}
  std::span<const std::byte> Scalar(const CodecLocation &loc,ScalarKind kind,std::uint32_t count) const override {
    const auto width=kind==ScalarKind::U8?1u:kind==ScalarKind::U16?2u:kind==ScalarKind::U32?4u:kind==ScalarKind::U64?8u:0u;
    if (!loc.parents.empty() || !width || count!=1 || loc.field>=values_.size()) Fail("fixed_header_plan");
    const auto value=values_[loc.field];
    if (width<8 && (value>>(width*8))) Fail("fixed_header_narrowing");
    for (unsigned i=0;i<width;++i) bytes_[i]=std::byte(value>>(i*8));
    return std::span(bytes_).first(width);
  }
  std::uint32_t Count(const CodecLocation &) const override { Fail("fixed_container"); }
  bool SharedPresent(const CodecLocation &) const override { Fail("fixed_shared"); }
  std::uint32_t SlotMask(const CodecLocation &) const override { Fail("fixed_choice"); }
  std::uint16_t SlotLayout(const CodecLocation &,std::uint8_t) const override { Fail("fixed_choice"); }
private:
  std::span<const std::uint64_t> values_;
  mutable std::array<std::byte,8> bytes_{};
};
template<class E> FixedValues<E> ReadFixed(std::uint16_t layout,std::span<const std::byte> bytes) {
  std::array<BodyFieldView,std::size_t(E::count)> fields{};
  BodyCodecScratch scratch;scratch.root_fields=fields;
  const auto body=DecodeValue(layout,bytes,scratch);
  if (body.fields.size()!=fields.size()) Fail("fixed_header_fields");
  FixedValues<E> values{};
  for (const auto &field:body.fields) {
    if (!field.present || field.field>=values.size()) Fail("fixed_header_field");
    values[field.field]=Bits(field.bytes);
  }
  return values;
}
template<class E> void WriteFixed(std::uint16_t layout,const FixedValues<E> &values,std::span<std::byte> out) {
  FixedSource source(values);BodyCodecScratch scratch;
  const auto body=EncodeValueInto(layout,source,out,scratch);
  if (body.bytes.size()!=out.size()) Fail("fixed_header_size");
}
class Cursor {
public:
  explicit Cursor(std::span<const std::byte> bytes):bytes_(bytes) {}
  std::span<const std::byte> Take(std::size_t count) {
    if (count>bytes_.size()-position_) Fail("truncated_envelope");
    const auto result=bytes_.subspan(position_,count);position_+=count;return result;
  }
  std::size_t Position() const { return position_; }
private:
  std::span<const std::byte> bytes_;
  std::size_t position_=0;
};
// 编码输入不能与输出重叠；否则先写固定头会污染尚未复制的typed值或元数据。
bool Overlaps(std::span<const std::byte> a,std::span<const std::byte> b) {
  if (a.empty() || b.empty()) return false;
  const auto left=reinterpret_cast<std::uintptr_t>(a.data()),right=reinterpret_cast<std::uintptr_t>(b.data());
  return left<=right ? right-left<a.size() : left-right<b.size();
}
auto DependencyKey(const ResultDependency &d) {
  return std::tuple(d.kind,d.table,d.member,d.purpose,d.index,d.instance,d.generation);
}
void CheckDependency(const ResultDependency &d,const ResultPolicy &policy,const PresentationBatchContext &batch) {
  ResultDependency canonical;canonical.kind=d.kind;
  switch (d.kind) {
    case ResultDependencyKind::Member:
      if (d.member==kNullIndex) Fail("null_member_dependency");
      canonical.member=d.member;break;
    case ResultDependencyKind::Resource:
      if (d.table>7 || d.index==kNullIndex) Fail("invalid_resource_dependency");
      canonical.table=d.table;canonical.index=d.index;break;
    case ResultDependencyKind::PlayerTopology:
      if (!policy.topology || !d.revision || d.revision>UINT32_MAX)
        Fail("invalid_topology_dependency");
      if (d.member==kNullIndex) Fail("null_topology_member");
      canonical.member=d.member;canonical.revision=d.revision;break;
    case ResultDependencyKind::ResultGroup:
      if (Policy(d.purpose).event || !d.revision || !d.instance || !d.generation) Fail("invalid_group_dependency");
      if (d.revision>batch.source_revision || std::none_of(kResultRequirements.begin(),kResultRequirements.end(),[&](const auto &allowed){
        return allowed.layout==policy.layout && allowed.dependency_layout==d.purpose;
      })) Fail("unregistered_result_dependency");
      canonical.member=d.member;canonical.purpose=d.purpose;canonical.revision=d.revision;
      canonical.instance=d.instance;canonical.generation=d.generation;break;
    case ResultDependencyKind::PairedTarget:
      if (d.index>1 || d.index==std::uint16_t(batch.stream) || !(policy.paired_streams&(1u<<d.index)) ||
          !d.revision || d.revision>UINT32_MAX || batch.paired_targets[d.index]!=d.revision)
        Fail("unregistered_or_missing_paired_target");
      canonical.index=d.index;canonical.revision=d.revision;break;
    default: Fail("unknown_dependency_kind");
  }
  if (canonical!=d) Fail("noncanonical_dependency_fields");
}
bool SamePath(std::span<const CodecPathStep> a,std::span<const CodecPathStep> b) {
  return std::equal(a.begin(),a.end(),b.begin(),b.end());
}
int ComparePath(std::span<const CodecPathStep> a,std::span<const CodecPathStep> b) {
  const auto order=std::lexicographical_compare_three_way(a.begin(),a.end(),b.begin(),b.end());
  return order<0?-1:order>0?1:0;
}
std::span<const CodecPathStep> Path(const ContributionTarget &target) { return std::span(target.path).first(target.path_size); }

// 构建实际节点路径，再由正文的真实条件／容器遍历确认。索引存在于布局但当前分支
// 不存在时不会收到节点，最终拒绝；不扫描原生对象，也不按字符串访问typed值。
void PrepareContributions(const ResultGroupView &group,PresentationEnvelopeScratch &scratch,const ResultPolicy &policy) {
  const auto count=group.contributions.size();
  if (count>kEnvelope_max_result_contributions || count>scratch.targets.size() || count>scratch.contribution_order.size())
    Fail("contribution_scratch_or_count_budget");
  for (std::size_t i=0;i<count;++i) {
    const auto &entry=group.contributions[i];auto &target=scratch.targets[i];target={};target.parent_depth=1;
    if (entry.id!=i || entry.flags || entry.pass>=32 || !(policy.passes&(1u<<entry.pass))) Fail("invalid_contribution_identity_or_pass");
    target.layout=group.header.layout;
    if (entry.parent!=UINT32_MAX) {
      if (entry.parent>=i) Fail("contribution_parent_not_prior");
      target=scratch.targets[entry.parent];target.matched=false;
      if (++target.parent_depth>16) Fail("contribution_parent_depth");
    }
    if (entry.field==kNullIndex) {
      if (entry.element!=kNullIndex || entry.parent!=UINT32_MAX) Fail("noncanonical_root_contribution");
    } else {
      if (!target.layout) Fail("scalar_contribution_has_child");
      const auto *field=FindField(target.layout,entry.field);
      if (!field || !field->max_size || field->operation==Operation::Choice) Fail("invalid_contribution_field");
      const bool container=field->operation==Operation::Container;
      if (container ? (entry.element==kNullIndex || entry.element>=field->limit) : entry.element!=kNullIndex)
        Fail("invalid_contribution_element");
      if (target.path_size>=target.path.size()) Fail("contribution_path_depth");
      target.path[target.path_size++]={target.layout,entry.field,entry.element,0};
      target.layout=field->nested_layout;
    }
    scratch.contribution_order[i]=std::uint16_t(i);
  }
  auto order=scratch.contribution_order.first(count);
  std::sort(order.begin(),order.end(),[&](auto a,auto b){
    const auto compare=ComparePath(Path(scratch.targets[a]),Path(scratch.targets[b]));
    return compare?compare<0:group.contributions[a].pass<group.contributions[b].pass;
  });
  for (std::size_t i=1;i<count;++i) if (SamePath(Path(scratch.targets[order[i-1]]),Path(scratch.targets[order[i]])) &&
      group.contributions[order[i-1]].pass==group.contributions[order[i]].pass) Fail("duplicate_contribution_location");
}
// 单次遍历同时核贡献位置和真实引用。排序的数字scratch只用于查找，线上顺序不变。
class ResultObserver final : public BodyNodeObserver {
public:
  ResultObserver(const ResultGroupView &group,PresentationEnvelopeScratch &scratch,const ResultPolicy &policy,
                 const PresentationEnvelopeContext *context=nullptr,const PresentationBatchContext *batch=nullptr)
      :group_(group),scratch_(scratch),policy_(policy),context_(context),batch_(batch) {}
  bool Has(const ResultDependency &key) const {
    const auto order=scratch_.dependency_order.first(group_.dependencies.size());
    const auto it=std::lower_bound(order.begin(),order.end(),key,[&](auto index,const ResultDependency &value){
      return DependencyKey(group_.dependencies[index])<DependencyKey(value);
    });
    return it!=order.end() && DependencyKey(group_.dependencies[*it])==DependencyKey(key);
  }
  void Node(const BodyNodeView &node) override {
    if (!node.layout && node.path.size()==1 && node.path[0].layout==policy_.layout &&
        node.path[0].field==policy_.subject_field && Bits(node.bytes)!=group_.header.subject)
      Fail("typed_subject_does_not_match_group");
    if (!node.layout && (node.operation==Operation::Resource || node.operation==Operation::Reference)) {
      const auto index=Bits(node.bytes);
      if (index!=kNullIndex) {
        if (index>UINT16_MAX) Fail("link_width");
        ResultDependency key;
        if (node.operation==Operation::Resource) {
          key.kind=ResultDependencyKind::Resource;key.table=node.resource_table;key.index=std::uint16_t(index);
          if (context_ && !context_->ResourceReady(batch_->resource_epoch,key.table,key.index)) Fail("payload_resource_not_ready");
        } else {
          key.member=std::uint16_t(index);
          if (context_ && !context_->Member(batch_->member_epoch,key.member)) Fail("payload_member_not_ready");
        }
        if (!Has(key)) Fail("missing_payload_link_dependency");
      }
    }
    const auto order=scratch_.contribution_order.first(group_.contributions.size());
    auto it=std::lower_bound(order.begin(),order.end(),node.path,[&](auto index,auto path){
      return ComparePath(Path(scratch_.targets[index]),path)<0;
    });
    bool drawing=false;
    for (;it!=order.end() && SamePath(Path(scratch_.targets[*it]),node.path);++it) {
      auto &target=scratch_.targets[*it];const auto pass=group_.contributions[*it].pass;
      if (target.layout!=node.layout || target.matched) Fail("ambiguous_contribution_node");
      const bool valid=pass==kEnvelope_Context ? node.layout!=0 :
        pass==kEnvelope_Value ? node.path.empty() && node.layout==group_.header.layout :
        pass==kEnvelope_Sprite ? node.layout==365 : pass==kEnvelope_Image ? node.layout==366 :
        pass==kEnvelope_LocalHistory ? node.layout==424 : pass==kEnvelope_Fade ? node.layout==372 :
        (pass==kEnvelope_RandomPrimary || pass==kEnvelope_RandomSecondary || pass==kEnvelope_RandomRecursive) &&
          (policy_.layout==357 || policy_.layout==358) && node.path.empty() && node.layout==policy_.layout;
      if (!valid) Fail("pass_does_not_match_typed_node");
      target.matched=true;drawing|=pass!=kEnvelope_Context;
    }
    if ((node.layout==365 || node.layout==366 || node.layout==424 || node.layout==372 ||
         (node.path.empty() && (policy_.layout==357 || policy_.layout==358))) && !drawing)
      Fail("missing_ordered_contribution");
  }
private:
  const ResultGroupView &group_;
  PresentationEnvelopeScratch &scratch_;
  const ResultPolicy &policy_;
  const PresentationEnvelopeContext *context_;
  const PresentationBatchContext *batch_;
};
// observer和root_fields只在此调用中借用；失败也恢复，防止scratch留下悬空回调。
class ScratchLease {
public:
  explicit ScratchLease(BodyCodecScratch &scratch,BodyNodeObserver *observer):scratch_(scratch),saved_(scratch) {
    scratch_.observer=observer;scratch_.root_fields={};
  }
  ~ScratchLease() { scratch_=saved_; }
private:
  BodyCodecScratch &scratch_;
  BodyCodecScratch saved_;
};
std::uint32_t CheckGroup(const PresentationBatchContext &batch,const ResultGroupView &group,
    PresentationEnvelopeScratch &scratch,const BodyCodecContext *context,PresentationEnvelopeLimits limits,
    const PresentationEnvelopeContext *ready=nullptr) {
  const auto &h=group.header;const auto &policy=Policy(h.purpose);
  if (policy.event || h.layout!=policy.layout || policy.body!=(batch.stream==BodyStream::Body) || h.flags ||
      !h.instance || !h.generation || !h.produced_revision || h.produced_revision>batch.source_revision)
    Fail("invalid_group_header_or_sequence");
  if (policy.subject_layout ? h.subject==kNullIndex : h.subject!=kNullIndex) Fail("invalid_subject_scope");
  if (h.operation==ResultOperation::Retire) {
    if (!batch.base_version || !group.dependencies.empty() || !group.contributions.empty() || !group.payload.empty()) Fail("invalid_retire_payload");
    return 0;
  }
  if (h.operation!=ResultOperation::Replace && h.operation!=ResultOperation::Reference) Fail("unknown_group_operation");
  if (!policy.cacheable && (h.operation==ResultOperation::Reference || h.produced_revision!=batch.source_revision)) Fail("transient_result_reused");
  if (group.dependencies.size()>kEnvelope_max_result_dependencies || group.dependencies.size()>scratch.dependency_order.size())
    Fail("dependency_scratch_or_count_budget");
  auto order=scratch.dependency_order.first(group.dependencies.size());
  std::iota(order.begin(),order.end(),std::uint16_t{});
  for (const auto &dependency:group.dependencies) {
    CheckDependency(dependency,policy,batch);
    if (dependency.kind==ResultDependencyKind::PlayerTopology && dependency.member!=h.subject) Fail("unrelated_topology_dependency");
  }
  std::sort(order.begin(),order.end(),[&](auto a,auto b){return DependencyKey(group.dependencies[a])<DependencyKey(group.dependencies[b]);});
  for (std::size_t i=1;i<order.size();++i) if (DependencyKey(group.dependencies[order[i-1]])==DependencyKey(group.dependencies[order[i]]))
    Fail("duplicate_or_conflicting_dependency");
  ResultObserver observer(group,scratch,policy,ready,&batch);
  for (const auto member:{h.owner,h.subject}) if (member!=kNullIndex) {
    ResultDependency key;key.member=member;
    if (!observer.Has(key)) Fail("missing_owner_or_subject_dependency");
  }
  if (policy.topology) {
    ResultDependency key;key.kind=ResultDependencyKind::PlayerTopology;key.member=h.subject;
    if (!observer.Has(key)) Fail("missing_player_topology_dependency");
  }
  for (const auto &required:kResultRequirements) if (required.layout==h.layout) {
    if (std::none_of(group.dependencies.begin(),group.dependencies.end(),[&](const auto &d){
      return d.kind==ResultDependencyKind::ResultGroup && d.purpose==required.dependency_layout &&
        (required.dependency_layout!=356 || d.member==kNullIndex);
    })) Fail("missing_required_result_dependency");
  }
  if (h.operation==ResultOperation::Reference) {
    if (!group.payload.empty() || !group.contributions.empty()) Fail("reference_has_typed_payload");
    return 0;
  }
  PrepareContributions(group,scratch,policy);
  ScratchLease lease(scratch.body,&observer);
  auto body=DecodeValue(h.layout,group.payload,scratch.body,context,limits.body);
  if (body.needs_base_checks) Fail("incomplete_result_projection");
  for (std::size_t i=0;i<group.contributions.size();++i) if (!scratch.targets[i].matched) Fail("contribution_node_not_present");
  return body.field_visits;
}
std::uint64_t GroupBytes(const ResultGroupView &group) {
  return kGroupBytes+std::uint64_t(group.dependencies.size())*kDependencyBytes+
      std::uint64_t(group.contributions.size())*kContributionBytes+group.payload.size();
}
} // namespace

ResultGroupView DecodeResultGroupPrefix(const PresentationBatchContext &batch,std::span<const std::byte> encoded,
    PresentationEnvelopeScratch &scratch,const BodyCodecContext *context,PresentationEnvelopeLimits limits) {
  CheckBatch(batch,limits);Cursor input(encoded);
  const auto h=ReadFixed<GroupField>(kGroupLayout,input.Take(kGroupBytes));
  const auto value=[&](GroupField f){return h[std::size_t(f)];};
  const auto dependencies=std::size_t(value(GroupField::dependency_count));
  const auto contributions=std::size_t(value(GroupField::contribution_count));
  const auto bytes=std::size_t(value(GroupField::payload_bytes));
  if (bytes>limits.body.bytes || kGroupBytes>limits.body.bytes-bytes) Fail("group_byte_budget");
  if (scratch.dependencies_used>scratch.dependencies.size() || dependencies>scratch.dependencies.size()-scratch.dependencies_used ||
      scratch.contributions_used>scratch.contributions.size() || contributions>scratch.contributions.size()-scratch.contributions_used ||
      std::uint64_t(scratch.dependencies_used)+dependencies>kEnvelope_max_result_dependencies ||
      std::uint64_t(scratch.contributions_used)+contributions>kEnvelope_max_result_contributions) Fail("batch_scratch_or_count_budget");
  auto deps=scratch.dependencies.subspan(scratch.dependencies_used,dependencies);
  auto entries=scratch.contributions.subspan(scratch.contributions_used,contributions);
  Cursor payload(input.Take(bytes));
  for (auto &d:deps) {
    const auto values=ReadFixed<DependencyField>(kDependencyLayout,payload.Take(kDependencyBytes));
    const auto v=[&](DependencyField f){return values[std::size_t(f)];};
    d={ResultDependencyKind(v(DependencyField::kind)),std::uint8_t(v(DependencyField::table)),
       std::uint16_t(v(DependencyField::member)),std::uint16_t(v(DependencyField::purpose)),std::uint16_t(v(DependencyField::index)),
       v(DependencyField::revision),v(DependencyField::instance),v(DependencyField::generation)};
  }
  for (auto &c:entries) {
    const auto values=ReadFixed<ContributionField>(kContributionLayout,payload.Take(kContributionBytes));
    const auto v=[&](ContributionField f){return values[std::size_t(f)];};
    c={std::uint32_t(v(ContributionField::contribution_id)),std::uint32_t(v(ContributionField::parent_id)),
       std::uint16_t(v(ContributionField::pass)),std::uint16_t(v(ContributionField::field)),
       std::uint16_t(v(ContributionField::element)),std::uint16_t(v(ContributionField::flags))};
  }
  ResultGroupView group;
  group.header={ResultOperation(value(GroupField::op)),std::uint8_t(value(GroupField::flags)),
      std::uint16_t(value(GroupField::owner_member)),std::uint16_t(value(GroupField::subject_member)),
      std::uint16_t(value(GroupField::purpose)),std::uint16_t(value(GroupField::layout)),value(GroupField::owner_instance),
      value(GroupField::owner_generation),value(GroupField::produced_revision)};
  group.dependencies=deps;group.contributions=entries;
  group.payload=payload.Take(bytes-payload.Position());group.consumed=std::uint32_t(input.Position());
  group.field_visits=CheckGroup(batch,group,scratch,context,limits);
  scratch.dependencies_used+=std::uint32_t(dependencies);scratch.contributions_used+=std::uint32_t(contributions);
  return group;
}

namespace {
// 整批编码已统一检查typed值时复用纯封装阶段，避免逐组重复遍历同一payload。
std::span<const std::byte> EncodeGroup(const PresentationBatchContext &batch,const ResultGroupView &group,
    std::span<std::byte> output,PresentationEnvelopeScratch &scratch,const BodyCodecContext *context,
    PresentationEnvelopeLimits limits,bool checked) {
  CheckBatch(batch,limits);
  const auto total=GroupBytes(group);
  if (total>limits.body.bytes || total>output.size()) Fail("group_output_budget");
  const auto destination=std::span<const std::byte>(output.first(std::size_t(total)));
  if (Overlaps(group.payload,destination) || Overlaps(std::as_bytes(group.dependencies),destination) ||
      Overlaps(std::as_bytes(group.contributions),destination)) Fail("overlapping_group_encode_buffers");
  if (!checked) CheckGroup(batch,group,scratch,context,limits);
  FixedValues<GroupField> h{};const auto &g=group.header;
  At(h,GroupField::op)=std::uint8_t(g.operation);At(h,GroupField::flags)=g.flags;
  At(h,GroupField::owner_member)=g.owner;At(h,GroupField::subject_member)=g.subject;
  At(h,GroupField::purpose)=g.purpose;At(h,GroupField::layout)=g.layout;
  At(h,GroupField::owner_instance)=g.instance;At(h,GroupField::owner_generation)=g.generation;
  At(h,GroupField::produced_revision)=g.produced_revision;At(h,GroupField::dependency_count)=group.dependencies.size();
  At(h,GroupField::contribution_count)=group.contributions.size();At(h,GroupField::payload_bytes)=total-kGroupBytes;
  WriteFixed<GroupField>(kGroupLayout,h,output.first(kGroupBytes));std::size_t position=kGroupBytes;
  for (const auto &d:group.dependencies) {
    FixedValues<DependencyField> v{};
    At(v,DependencyField::kind)=std::uint8_t(d.kind);At(v,DependencyField::table)=d.table;
    At(v,DependencyField::member)=d.member;At(v,DependencyField::purpose)=d.purpose;At(v,DependencyField::index)=d.index;
    At(v,DependencyField::revision)=d.revision;At(v,DependencyField::instance)=d.instance;At(v,DependencyField::generation)=d.generation;
    WriteFixed<DependencyField>(kDependencyLayout,v,output.subspan(position,kDependencyBytes));position+=kDependencyBytes;
  }
  for (const auto &c:group.contributions) {
    FixedValues<ContributionField> v{};
    At(v,ContributionField::contribution_id)=c.id;At(v,ContributionField::parent_id)=c.parent;
    At(v,ContributionField::pass)=c.pass;At(v,ContributionField::field)=c.field;
    At(v,ContributionField::element)=c.element;At(v,ContributionField::flags)=c.flags;
    WriteFixed<ContributionField>(kContributionLayout,v,output.subspan(position,kContributionBytes));position+=kContributionBytes;
  }
  if (!group.payload.empty()) std::memmove(output.data()+position,group.payload.data(),group.payload.size());
  return output.first(std::size_t(total));
}
} // namespace
std::span<const std::byte> EncodeResultGroupInto(const PresentationBatchContext &batch,const ResultGroupView &group,
    std::span<std::byte> output,PresentationEnvelopeScratch &scratch,const BodyCodecContext *context,PresentationEnvelopeLimits limits) {
  return EncodeGroup(batch,group,output,scratch,context,limits,false);
}
namespace {
class AudioObserver final : public BodyNodeObserver {
public:
  AudioObserver(const PresentationBatchContext &batch,const PresentationEnvelopeContext *context):batch_(batch),context_(context) {}
  void Node(const BodyNodeView &node) override {
    if (!node.layout && node.path.size()==1 && node.path[0].layout==383 && node.path[0].field==kAudioKindField)
      kind=std::uint8_t(Bits(node.bytes));
    if (context_ && !node.layout && node.operation==Operation::Resource) {
      const auto index=Bits(node.bytes);
      if (index!=kNullIndex && !context_->ResourceReady(batch_.resource_epoch,node.resource_table,std::uint16_t(index)))
        Fail("audio_resource_not_ready");
    }
  }
  std::uint8_t kind=255;
private:
  const PresentationBatchContext &batch_;
  const PresentationEnvelopeContext *context_;
};
std::uint32_t CheckEvent(const PresentationBatchContext &batch,const PresentationAudioEvent &event,
    BodyCodecScratch &scratch,PresentationEnvelopeLimits limits,const PresentationEnvelopeContext *context=nullptr) {
  if (batch.stream!=BodyStream::World || !event.sequence || event.flags || event.layout!=383 || event.purpose!=382 ||
      event.clear_scope>3 || event.payload.size()+kEventBytes>limits.body.bytes) Fail("invalid_audio_event_header");
  AudioObserver observer(batch,context);ScratchLease lease(scratch,&observer);
  const auto body=DecodeValue(event.layout,event.payload,scratch,nullptr,limits.body);
  if (observer.kind>4) Fail("missing_audio_event_kind");
  if (observer.kind!=3 || !event.clear_scope) {
    if (event.clear_scope || !event.voice || !event.generation) Fail("invalid_exact_voice_event");
  } else {
    if (event.voice || event.generation) Fail("ranged_clear_has_voice");
    if (event.clear_scope==1 ? event.owner==kNullIndex : event.owner!=kNullIndex) Fail("invalid_clear_owner_scope");
  }
  if (context && event.owner!=kNullIndex && !context->Member(batch.member_epoch,event.owner)) Fail("audio_owner_not_ready");
  if (context) if (const auto known=context->KnownEvent(batch.scope,event.sequence)) {
    if (std::tie(known->sequence,known->logical_frame,known->owner,known->purpose,known->voice,known->generation,
                 known->clear_scope,known->flags,known->layout)!=
        std::tie(event.sequence,event.logical_frame,event.owner,event.purpose,event.voice,event.generation,
                 event.clear_scope,event.flags,event.layout) ||
        !std::equal(known->payload.begin(),known->payload.end(),event.payload.begin(),event.payload.end()))
      Fail("conflicting_audio_event_retransmission");
  }
  return body.field_visits;
}
PresentationMember ResolveMember(const PresentationBatchContext &batch,std::uint16_t index,const PresentationEnvelopeContext &context) {
  if (index==kNullIndex) return {};
  const auto member=context.Member(batch.member_epoch,index);
  if (!member || !member->id || !member->use || !member->shape || !FindLayout(member->layout)) Fail("member_identity_not_ready");
  return *member;
}
ResultGroupKey ResolveKey(const PresentationBatchContext &batch,const ResultGroupHeader &h,const PresentationEnvelopeContext &context) {
  ResultGroupKey key{batch.scope,ResolveMember(batch,h.owner,context),ResolveMember(batch,h.subject,context),h.purpose,h.instance,h.generation};
  const auto &policy=Policy(h.purpose);
  if (key.subject.layout!=policy.subject_layout) Fail("subject_layout_mismatch");
  return key;
}
bool SameInstance(const ResultGroupKey &a,const ResultGroupKey &b) {
  return a.scope==b.scope && a.owner==b.owner && a.purpose==b.purpose && a.instance==b.instance && a.generation==b.generation;
}
void CheckCommitted(const CommittedResult &cached,const ResultGroupKey &key,std::uint64_t revision,
    const PresentationBatchContext &batch) {
  if (cached.key!=key || cached.layout!=key.purpose || cached.produced_revision!=revision) Fail("cached_result_identity_conflict");
  if (cached.member_epoch!=batch.member_epoch || cached.resource_epoch!=batch.resource_epoch) Fail("cached_result_requires_epoch_remap");
}
// 显示区域预算包含所有外层头、依赖和贡献；事件水位只属于world，baseline不重播历史。
std::uint64_t CheckCollections(const PresentationBatchContext &batch,std::span<const ResultGroupView> groups,
    std::span<const PresentationAudioEvent> events,PresentationEnvelopeLimits limits) {
  if (groups.size()>kEnvelope_max_result_groups || events.size()>kEnvelope_max_audio_events) Fail("batch_count_budget");
  if (batch.stream==BodyStream::Body && !events.empty()) Fail("body_audio_events");
  std::uint64_t bytes=0,dependencies=0,contributions=0;
  for (std::size_t i=0;i<groups.size();++i) {
    const auto &g=groups[i];bytes+=GroupBytes(g);dependencies+=g.dependencies.size();contributions+=g.contributions.size();
    for (std::size_t j=0;j<i;++j) {
      const auto &a=g.header,&b=groups[j].header;
      if (std::tie(a.owner,a.purpose,a.instance,a.generation)==std::tie(b.owner,b.purpose,b.instance,b.generation))
        Fail("duplicate_wire_group_instance");
    }
  }
  for (const auto &event:events) bytes+=std::uint64_t(event.payload.size())+kEventBytes;
  if (bytes>limits.body.bytes || dependencies>kEnvelope_max_result_dependencies || contributions>kEnvelope_max_result_contributions)
    Fail("whole_batch_budget");
  if (!batch.base_version) {
    if (!events.empty() || (batch.stream==BodyStream::World && batch.event_watermark<batch.installed_event_watermark))
      Fail("baseline_replays_or_rewinds_events");
  } else if (batch.stream==BodyStream::World) {
    auto watermark=batch.installed_event_watermark;
    for (const auto &event:events) {
      if (watermark==UINT64_MAX || event.sequence!=watermark+1) Fail("audio_event_sequence_gap_or_duplicate");
      watermark=event.sequence;
    }
    if (watermark!=batch.event_watermark) Fail("audio_event_watermark_gap");
  }
  return bytes;
}
} // namespace

PresentationAudioEvent DecodePresentationAudioEventPrefix(const PresentationBatchContext &batch,
    std::span<const std::byte> encoded,BodyCodecScratch &scratch,PresentationEnvelopeLimits limits) {
  CheckBatch(batch,limits);Cursor input(encoded);
  const auto values=ReadFixed<EventField>(kEventLayout,input.Take(kEventBytes));
  const auto v=[&](EventField f){return values[std::size_t(f)];};
  const auto bytes=std::size_t(v(EventField::payload_bytes));
  if (bytes>limits.body.bytes || kEventBytes>limits.body.bytes-bytes) Fail("event_byte_budget");
  PresentationAudioEvent event;
  event.sequence=v(EventField::event_sequence);event.logical_frame=std::uint32_t(v(EventField::logical_frame));
  event.owner=std::uint16_t(v(EventField::owner_member));event.purpose=std::uint16_t(v(EventField::purpose));
  event.voice=v(EventField::voice_id);event.generation=v(EventField::voice_generation);
  event.clear_scope=std::uint8_t(v(EventField::clear_scope));event.flags=std::uint8_t(v(EventField::flags));
  event.layout=std::uint16_t(v(EventField::layout));event.payload=input.Take(bytes);event.consumed=std::uint32_t(input.Position());
  event.field_visits=CheckEvent(batch,event,scratch,limits);return event;
}
namespace {
std::span<const std::byte> EncodeEvent(const PresentationBatchContext &batch,
    const PresentationAudioEvent &event,std::span<std::byte> output,BodyCodecScratch &scratch,
    PresentationEnvelopeLimits limits,bool checked) {
  CheckBatch(batch,limits);
  const auto total=std::uint64_t(event.payload.size())+kEventBytes;
  if (total>output.size() || total>limits.body.bytes) Fail("event_output_budget");
  if (Overlaps(event.payload,std::span<const std::byte>(output.first(std::size_t(total))))) Fail("overlapping_event_encode_buffers");
  if (!checked) CheckEvent(batch,event,scratch,limits);
  FixedValues<EventField> v{};
  At(v,EventField::event_sequence)=event.sequence;At(v,EventField::logical_frame)=event.logical_frame;
  At(v,EventField::owner_member)=event.owner;At(v,EventField::purpose)=event.purpose;
  At(v,EventField::voice_id)=event.voice;At(v,EventField::voice_generation)=event.generation;
  At(v,EventField::clear_scope)=event.clear_scope;At(v,EventField::flags)=event.flags;
  At(v,EventField::layout)=event.layout;At(v,EventField::payload_bytes)=event.payload.size();
  WriteFixed<EventField>(kEventLayout,v,output.first(kEventBytes));
  if (!event.payload.empty()) std::memmove(output.data()+kEventBytes,event.payload.data(),event.payload.size());
  return output.first(std::size_t(total));
}
} // namespace
std::span<const std::byte> EncodePresentationAudioEventInto(const PresentationBatchContext &batch,
    const PresentationAudioEvent &event,std::span<std::byte> output,BodyCodecScratch &scratch,PresentationEnvelopeLimits limits) {
  return EncodeEvent(batch,event,output,scratch,limits,false);
}

PresentationBatchView DecodePresentationBatch(const PresentationBatchContext &batch,std::uint32_t group_count,
    std::uint32_t event_count,std::span<const std::byte> encoded,std::span<ResultGroupView> groups,
    std::span<PresentationAudioEvent> events,PresentationEnvelopeScratch &scratch,
    const BodyCodecContext *context,PresentationEnvelopeLimits limits) {
  CheckBatch(batch,limits);
  if (group_count>kEnvelope_max_result_groups || event_count>kEnvelope_max_audio_events ||
      group_count>groups.size() || event_count>events.size() || encoded.size()>limits.body.bytes ||
      std::uint64_t(group_count)*kGroupBytes+std::uint64_t(event_count)*kEventBytes>encoded.size()) Fail("batch_header_or_byte_budget");
  scratch.dependencies_used=0;scratch.contributions_used=0;
  auto remaining=limits;std::size_t position=0;
  for (std::uint32_t i=0;i<group_count;++i) {
    remaining.body.bytes=limits.body.bytes-std::uint32_t(position);
    groups[i]=DecodeResultGroupPrefix(batch,encoded.subspan(position),scratch,context,remaining);
    position+=groups[i].consumed;remaining.body.field_visits-=groups[i].field_visits;
  }
  for (std::uint32_t i=0;i<event_count;++i) {
    remaining.body.bytes=limits.body.bytes-std::uint32_t(position);
    events[i]=DecodePresentationAudioEventPrefix(batch,encoded.subspan(position),scratch.body,remaining);
    position+=events[i].consumed;remaining.body.field_visits-=events[i].field_visits;
  }
  if (position!=encoded.size()) Fail("trailing_batch_bytes");
  CheckCollections(batch,groups.first(group_count),events.first(event_count),limits);
  return {groups.first(group_count),events.first(event_count),std::uint32_t(position)};
}
std::span<const std::byte> EncodePresentationBatchInto(const PresentationBatchContext &batch,
    std::span<const ResultGroupView> groups,std::span<const PresentationAudioEvent> events,std::span<std::byte> output,
    PresentationEnvelopeScratch &scratch,const BodyCodecContext *context,PresentationEnvelopeLimits limits,std::uint32_t *field_visits) {
  CheckBatch(batch,limits);const auto total=CheckCollections(batch,groups,events,limits);
  if (total>output.size()) Fail("batch_output_budget");
  const auto destination=std::span<const std::byte>(output.first(std::size_t(total)));
  if (Overlaps(std::as_bytes(groups),destination) || Overlaps(std::as_bytes(events),destination)) Fail("overlapping_batch_encode_metadata");
  auto remaining=limits;
  // 先检查整批输入不被先行写入污染，并累计实际遍历成本；随后只做固定封装。
  for (const auto &g:groups) {
    if (Overlaps(g.payload,destination) || Overlaps(std::as_bytes(g.dependencies),destination) ||
        Overlaps(std::as_bytes(g.contributions),destination)) Fail("overlapping_batch_encode_input");
    remaining.body.field_visits-=CheckGroup(batch,g,scratch,context,remaining);
  }
  for (const auto &event:events) {
    if (Overlaps(event.payload,destination)) Fail("overlapping_batch_encode_input");
    remaining.body.field_visits-=CheckEvent(batch,event,scratch.body,remaining);
  }
  const auto visits=limits.body.field_visits-remaining.body.field_visits;
  remaining=limits;std::size_t position=0;
  for (const auto &g:groups) {
    remaining.body.bytes=limits.body.bytes-std::uint32_t(position);
    const auto bytes=EncodeGroup(batch,g,output.subspan(position),scratch,context,remaining,true);position+=bytes.size();
  }
  for (const auto &event:events) {
    remaining.body.bytes=limits.body.bytes-std::uint32_t(position);
    const auto bytes=EncodeEvent(batch,event,output.subspan(position),scratch.body,remaining,true);position+=bytes.size();
  }
  if (field_visits) *field_visits=visits;
  return output.first(position);
}

void ValidatePresentationBatch(const PresentationBatchContext &batch,std::span<const ResultGroupView> groups,
    std::span<const PresentationAudioEvent> events,const PresentationEnvelopeContext &context,
    std::span<ResultGroupKey> keys,std::span<std::uint8_t> state,PresentationEnvelopeScratch &scratch,
    PresentationEnvelopeLimits limits) {
  CheckBatch(batch,limits);
  if (groups.size()>kEnvelope_max_result_groups || events.size()>kEnvelope_max_audio_events ||
      keys.size()<groups.size() || state.size()<groups.size()) Fail("batch_count_or_scratch_budget");
  if (batch.stream==BodyStream::Body && !events.empty()) Fail("body_audio_events");
  CheckCollections(batch,groups,events,limits);
  auto remaining=limits;
  const auto consume=[&](std::uint32_t visits){
    if (visits>remaining.body.field_visits) Fail("whole_batch_visit_budget");
    remaining.body.field_visits-=visits;
  };
  for (std::size_t i=0;i<groups.size();++i) {
    const auto &group=groups[i];
    // 即使只有Reference／Retire也校验基础头；资格、解析与typed检查全部成功前不发布键。
    consume(CheckGroup(batch,group,scratch,&context,remaining,&context));
    keys[i]=ResolveKey(batch,group.header,context);state[i]=0;
    for (std::size_t j=0;j<i;++j) if (SameInstance(keys[i],keys[j])) Fail("duplicate_or_rebound_group_instance");
    if (!context.SourceQualified(keys[i],group.header.produced_revision,batch,group.header.operation)) Fail("result_source_not_qualified");
    if (group.header.operation==ResultOperation::Reference) {
      const auto cached=context.Cached(keys[i],group.header.produced_revision);
      if (!cached) Fail("referenced_committed_cache_missing");
      CheckCommitted(*cached,keys[i],group.header.produced_revision,batch);
      auto value=group;value.header.operation=ResultOperation::Replace;
      value.payload=cached->payload;value.contributions=cached->contributions;
      // 当前显式依赖必须覆盖缓存typed值中的实际链接，不能只相信旧strong或旧依赖。
      consume(CheckGroup(batch,value,scratch,&context,remaining,&context));
    }
    for (const auto &d:group.dependencies) {
      switch (d.kind) {
        case ResultDependencyKind::Member: (void)ResolveMember(batch,d.member,context);break;
        case ResultDependencyKind::Resource:
          if (!context.ResourceReady(batch.resource_epoch,d.table,d.index)) Fail("declared_resource_not_ready");
          break;
        case ResultDependencyKind::PlayerTopology: {
          const auto topology=context.PlayerTopology(batch.member_epoch,d.member);
          if (!topology || *topology!=d.revision) Fail("player_p0_p6_topology_not_ready");
          break;
        }
        default: break;
      }
    }
  }
  // 同批前向依赖可以成立；仅引用完整同身份目标或已提交缓存。三色DFS拒绝环，
  // 深度限制只保护依赖解析栈，不截断数据，也不把失败组变成Retire或空值。
  const auto resolve=[&](auto &self,std::size_t index,unsigned depth)->void {
    if (depth>32) Fail("result_dependency_depth_budget");
    if (state[index]==2) return;
    if (state[index]==1) Fail("result_dependency_cycle");
    state[index]=1;
    for (const auto &d:groups[index].dependencies) if (d.kind==ResultDependencyKind::ResultGroup) {
      const auto owner=ResolveMember(batch,d.member,context);
      bool found=false;
      for (std::size_t j=0;j<groups.size();++j) if (keys[j].owner==owner && keys[j].purpose==d.purpose &&
          keys[j].instance==d.instance && keys[j].generation==d.generation) {
        if (groups[j].header.operation==ResultOperation::Retire || groups[j].header.produced_revision!=d.revision)
          Fail("result_dependency_shadowed_or_retired");
        self(self,j,depth+1);found=true;break;
      }
      if (!found) {
        const auto cached=context.Result(batch.scope,owner,d.purpose,d.instance,d.generation,d.revision);
        if (!cached || cached->key.scope!=batch.scope || cached->key.owner!=owner || cached->key.purpose!=d.purpose ||
            cached->key.instance!=d.instance || cached->key.generation!=d.generation || cached->layout!=d.purpose ||
            cached->produced_revision!=d.revision || cached->member_epoch!=batch.member_epoch || cached->resource_epoch!=batch.resource_epoch ||
            !context.SourceQualified(cached->key,d.revision,batch,ResultOperation::Reference))
          Fail("result_dependency_cache_not_ready");
        if (!Policy(d.purpose).cacheable && d.revision!=batch.source_revision) Fail("transient_dependency_reused");
      }
    }
    state[index]=2;
  };
  for (std::size_t i=0;i<groups.size();++i) resolve(resolve,i,0);
  for (const auto &event:events) consume(CheckEvent(batch,event,scratch.body,remaining,&context));
  // 不更新context／缓存／事件水位。第12步完整影子和第13步原生共同提交后才可ACK。
}
} // namespace cs::state
