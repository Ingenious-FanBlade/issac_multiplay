#include "state/capture_executor.h"
#include "core/capture_cost.h"
#include <algorithm>
#include <bit>
#include <cstring>

namespace cs::state {
namespace {
// 有界纯值小端读取；调用点已核恰好1..8字节，不将原生对象作未对齐强转。
std::uint64_t Integer(std::span<const std::byte> bytes) noexcept {
  // 标准宽度直接取得小端纯值；memcpy 保持未对齐访问合法，不改变 selector 的位表示。
  if constexpr(std::endian::native==std::endian::little){
    switch(bytes.size()){
      case 1:return std::to_integer<std::uint8_t>(bytes[0]);
      case 2:{std::uint16_t value;std::memcpy(&value,bytes.data(),2);return value;}
      case 4:{std::uint32_t value;std::memcpy(&value,bytes.data(),4);return value;}
      case 8:{std::uint64_t value;std::memcpy(&value,bytes.data(),8);return value;}
    }
  }
  std::uint64_t value=0;for(std::size_t i=0;i<bytes.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(bytes[i]))<<(8*i);
  return value;
}
bool IsInteger(ScalarKind kind) noexcept {return kind!=ScalarKind::None && kind!=ScalarKind::F32 && kind!=ScalarKind::F64;}
bool LessInteger(std::uint64_t a,std::uint64_t b,ScalarKind kind) noexcept {
  if(kind==ScalarKind::I8 || kind==ScalarKind::I16 || kind==ScalarKind::I32 || kind==ScalarKind::I64){
    const auto width=CaptureScalarWidth(kind);
    const auto sign=[width](std::uint64_t value){if(width<8 && (value&(std::uint64_t{1}<<(width*8-1))))value|=UINT64_MAX<<(width*8);
      return std::bit_cast<std::int64_t>(value);};
    return sign(a)<sign(b);
  }
  return a<b;
}
}

// 每次调用独占预分配状态；递归栈只保存小型游标，所有字段缓存按深度切片。
class CaptureExecution final {
  // 有序容器只借用子实例的首个标量键，返回前取纯值，避免复制来源游标。
  struct FieldKey {std::uint64_t bits=0;ScalarKind scalar=ScalarKind::None;bool integer=false;};
public:
  CaptureExecution(CaptureExecutor &executor,const CaptureWindow &window,CaptureValueWriter &writer,
      CaptureDiagnostic &diagnostic,CaptureExecutionLimits limits) noexcept
      :executor_(executor),window_(window),writer_(writer),out_(diagnostic),limits_(limits) {}
  bool Walk(const CaptureRoot &,const Layout &,std::uint32_t mask,BodyStream,FieldKey *key=nullptr) noexcept;
  std::uint32_t Bytes() const noexcept {return bytes_;}
  const capture::ExecutionCounts &Counts() const noexcept {return counts_;}
private:
  using State=CaptureExecutor::FieldState;
  // 每次 Walk 固定布局、实际选块和路径。字段状态切片、路径游标和复制段均不跨实例借用。
  struct Instance {
    const CaptureRoot &root;
    const Layout &layout;
    std::uint32_t mask;
    std::span<State> states;
    std::span<const Field> fields;
    std::span<const CapturePlan::Binding> values;
    std::span<const CaptureExecutor::Binding> operations;
    CaptureValueWriter::BoundPath path;
  };
  std::span<const CodecPathStep> Parents() const noexcept {return std::span(path_).first(depth_);}
  bool Reject(const CaptureRoot &root,CaptureFailure reason,std::uint16_t field=kNullIndex) noexcept {
    if(writer_.Diagnostic().reason!=CaptureFailure::None && out_.reason==CaptureFailure::None)out_=writer_.Diagnostic();
    FailCapture(out_,reason,root,field,Parents());
    if(out_.field==kNullIndex && field!=kNullIndex){out_.layout=root.layout;out_.field=field;out_.depth=depth_;
      std::copy_n(path_.begin(),depth_,out_.parents.begin());}
    writer_.Fail(out_);return false;
  }
  bool Visit(const CaptureRoot &root,std::uint16_t field=kNullIndex,std::uint32_t count=1) noexcept {
    if(count>limits_.field_visits-visits_)return Reject(root,CaptureFailure::TraversalBudget,field);
    visits_+=count;return true;
  }
  bool Add(const CaptureRoot &root,const Field &field,std::uint64_t count) noexcept {
    if(count>limits_.bytes-bytes_)return Reject(root,CaptureFailure::ValueBudget,field.index);
    bytes_+=std::uint32_t(count);return true;
  }
  CodecLocation Location(const Instance &instance,std::uint16_t i) const noexcept {
    return {Parents(),instance.layout.id,i,instance.values[i].format.canonical};
  }
  CaptureFieldAccess Access(const Instance &instance,std::uint16_t i,const CaptureTraversalSource *source=nullptr) const noexcept {
    return {window_,instance.root,instance.fields[i],instance.values[i].format,Location(instance,i),source};
  }
  bool Qualify(const Instance &,std::uint16_t) noexcept;
  bool Open(const Instance &,std::uint16_t) noexcept;
  bool Close(const Instance &,std::uint16_t) noexcept;
  bool CheckCounts(const Instance &,std::span<const CountConstraint>) noexcept;
  bool CheckShared(const Instance &,std::uint16_t) noexcept;
  bool Selected(const Instance &,std::uint16_t,bool &selected,bool &zero) noexcept;
  bool Direct(Instance &,std::uint16_t,CaptureWrittenValue &) noexcept;
  bool Leaf(Instance &,std::uint16_t,std::uint32_t count) noexcept;
  bool Child(Instance &,std::uint16_t,std::uint16_t element,FieldKey *key=nullptr) noexcept;
  bool FieldValue(Instance &,std::uint16_t) noexcept;
  bool Write(Instance &instance,std::uint16_t i,CaptureCellKind kind,std::uint32_t value,std::uint16_t aux=0) noexcept {
    ++counts_.bound_writes;
    return writer_.WriteBound(Location(instance,i),instance.path,kind,value,aux);
  }
  void ResetCopy() noexcept {copy_range_=nullptr;copied_={};}
  CaptureExecutor &executor_;
  const CaptureWindow &window_;
  CaptureValueWriter &writer_;
  CaptureDiagnostic &out_;
  CaptureExecutionLimits limits_;
  const CopyRange *copy_range_=nullptr;
  std::span<const std::byte> copied_;
  std::array<CodecPathStep,kCodecMaxDepth> path_{};
  capture::ExecutionCounts counts_;
  std::uint64_t qualified_=0;
  std::uint32_t bytes_=0,visits_=0;
  std::uint8_t depth_=0;
};

// 只在冷准备登记来源并记账，热路径不查字符串、不变更vector容量。
CaptureExecutor::CaptureExecutor(const CapturePlan &plan,std::span<const CaptureTraversalBinding> bindings,
    std::thread::id owner,CaptureExecutorCapacity capacity,CaptureExecutionContext context)
    :plan_(plan),owner_(owner),capacity_(capacity),context_(context) {
  if(owner==std::thread::id{} || !capacity.depth || capacity.depth>kCodecMaxDepth || !capacity.copy_bytes)
    throw std::invalid_argument("capture_executor_capacity_or_thread");
  for(std::size_t i=0;i<bindings.size();++i){const auto &binding=bindings[i];
    if(!binding.open || !binding.close || unsigned(binding.operation)>unsigned(Operation::Choice))
      throw std::invalid_argument("capture_traversal_binding");
    for(std::size_t j=0;j<i;++j)if(bindings[j].name==binding.name && bindings[j].operation==binding.operation)
      throw std::invalid_argument("duplicate_capture_traversal_binding");
  }
  field_stride_=BodySourceMaximumFields();
  const auto states=std::uint64_t(capacity.depth+1)*field_stride_;
  const auto fields=Fields();const auto layouts=Layouts();
  const auto shared_count=std::count_if(plan_.fields_.begin(),plan_.fields_.end(),[](const auto &v){return v.format.shared_owner;});
  memory_=memory::Reserve(memory::Pool::Workspace,sizeof(*this)+128+fields.size()*sizeof(Binding)+
    layouts.size()*sizeof(LayoutBinding)+fields.size()*sizeof(CopyRange)+states*sizeof(FieldState)+capacity.copy_bytes+shared_count*sizeof(std::uint16_t),"state_capture_executor");
  layouts_.resize(layouts.size());bindings_.resize(fields.size());states_.resize(std::size_t(states));
  copy_.resize(capacity.copy_bytes);shared_.reserve(shared_count);
  for(std::size_t i=0;i<layouts.size();++i){
    const auto &layout=layouts[i];auto &bound=layouts_[i];
    bound.valid=layout.field_count<=field_stride_&&(!layout.field_count||plan_.fields_[layout.first_field].valid);
    for(auto stream:{BodyStream::Value,BodyStream::World,BodyStream::Body})bound.masks[unsigned(stream)]=plan_.Mask(layout.id,stream);
  }
  for(const auto &budget:LayoutBudgets())if(const auto *layout=FindLayout(budget.layout))layouts_[layout-layouts.data()].budget=&budget;
  for(std::size_t i=0;i<fields.size();++i){const auto &field=fields[i];
    auto &compiled=bindings_[i];
    for(const auto &binding:bindings)if(binding.operation==field.operation&&binding.name==(field.handler?field.handler:"")){
      compiled.open=binding.open;compiled.element=binding.element;compiled.close=binding.close;
      compiled.in_read_segment=binding.in_read_segment;
    }
    const auto &value=plan_.fields_[i];if(!value.valid)continue;
    const auto &format=value.format;
    const auto *layout=FindLayout(field.layout);
    if((field.nested_layout||field.operation==Operation::Choice)&&format.canonical<layout->field_count){
      const auto slot=layout->first_field+format.canonical;
      if(plan_.fields_[slot].valid&&plan_.fields_[slot].format.canonical==format.canonical)compiled.parent=&fields[slot];
    }
    if((compiled.child=FindLayout(field.nested_layout))!=nullptr){
      auto mask=BoundLayout(*compiled.child).masks[unsigned(BodyStream::Value)];
      if(field.nested_block!=255)mask&=field.nested_block<compiled.child->block_count?std::uint32_t{1}<<field.nested_block:0;
      compiled.child_mask=mask;
      for(std::uint8_t block=0;block<compiled.child->block_count;++block)if(mask&(std::uint32_t{1}<<block))
        compiled.child_minimum+=Blocks()[compiled.child->first_block+block].min_size;
      compiled.inline_child=!layout->protocol&&!compiled.child->protocol&&field.source==FieldSource::Object&&
        (field.operation==Operation::Nested||field.operation==Operation::Copy)&&format.storage==BodyStorage::None&&
        (!field.handler||!field.handler[0]);
    }
    if(format.shared_owner&&format.canonical<layout->field_count){
      auto &group=bindings_[layout->first_field+format.canonical];
      if(!group.shared_count){
        group.shared_first=std::uint32_t(shared_.size());
        for(std::uint16_t f=0;f<layout->field_count;++f){const auto &other=plan_.fields_[layout->first_field+f].format;
          if(other.shared_owner&&other.canonical==format.canonical)shared_.push_back(f);}
        group.shared_count=std::uint32_t(shared_.size()-group.shared_first);
      }
    }
  }
  // 生成器保证不同计数组的字段互斥，组首字段的实例状态可用作唯一完成位。
  auto counts=CountConstraints();
  while(!counts.empty()){
    const auto &first=counts.front();
    const auto end=std::find_if(counts.begin(),counts.end(),[&](const auto &row){return row.layout!=first.layout||row.group!=first.group;});
    const auto group=std::span(counts.begin(),end);const auto *layout=FindLayout(first.layout);
    for(const auto &row:group){auto &bound=bindings_[layout->first_field+row.field];if(bound.counts.empty())bound.counts=group;}
    counts=counts.subspan(group.size());
  }
  auto choices=LayoutChoices();
  while(!choices.empty()){
    const auto &first=choices.front();
    const auto end=std::find_if(choices.begin(),choices.end(),[&](const auto &row){return row.layout!=first.layout||row.field!=first.field;});
    const auto group=std::span(choices.begin(),end);
    auto &bound=bindings_[FindLayout(first.layout)->first_field+first.field];if(bound.choices.empty())bound.choices=group;
    choices=choices.subspan(group.size());
  }
  // 只有定长无条件 Copy 叶才能直达复制；共享、计数组、条件和具名处理器仍走完整分派。
  for(std::size_t i=0;i<fields.size();++i){const auto &value=plan_.fields_[i];const auto &format=value.format;
    bindings_[i].scalar_leaf=value.direct&&!value.handler&&!format.conditional&&!format.canonical_zero&&
      !format.shared_owner&&bindings_[i].counts.empty();
  }
  // 网络变长偏移与原生复制资格无关。只合并执行顺序连续、原生字节严格相邻的无条件叶；
  // 每个成员保留自己的cell、selector和validator，不读未知间隙或跨handler／资格边界。
  copy_ranges_.reserve(fields.size());
  const auto eligible=[&](std::uint32_t slot){const auto &value=plan_.fields_[slot];
    return bindings_[slot].scalar_leaf&&(!value.required||value.qualify)&&
      (!value.qualify||value.qualification_slot<64);
  };
  for(const auto &layout:layouts)for(std::uint8_t block=0;block<layout.block_count;++block){
    const auto order=CaptureBlockFields(layout.first_block+block);
    for(std::size_t at=0;at<order.size();){
      const auto start=at;const auto slot=layout.first_field+order[at];
      if(!eligible(slot)){++at;continue;}
      const auto &first=fields[slot];const auto &qualification=plan_.fields_[slot];
      std::uint32_t size=first.native_size;bool dynamic=first.wire_offset==kDynamicOffset;
      for(++at;at<order.size();++at){
        const auto next=layout.first_field+order[at];const auto &field=fields[next];const auto &value=plan_.fields_[next];
        if(!eligible(next)||value.required!=qualification.required||value.qualify!=qualification.qualify||
            std::uint64_t(first.native_offset)+size!=field.native_offset)break;
        size+=field.native_size;dynamic|=field.wire_offset==kDynamicOffset;
      }
      if(at-start<2||!dynamic)continue;
      copy_ranges_.push_back({layout.id,block,first.native_offset,kDynamicOffset,size,first.read_requirement});
      // 上界预留保证指针稳定；后续只在冷准备填表，运行期不扩容。
      for(auto i=start;i<at;++i)bindings_[layout.first_field+order[i]].copy=&copy_ranges_.back();
    }
  }
  memory_->Resize(sizeof(*this)+128+layouts_.capacity()*sizeof(LayoutBinding)+bindings_.capacity()*sizeof(Binding)+
    copy_ranges_.capacity()*sizeof(CopyRange)+states_.capacity()*sizeof(FieldState)+copy_.capacity()+shared_.capacity()*sizeof(std::uint16_t));
}

// ready 类事实在本根首次使用时核一次；独立来源资格仍按当前字段检查。
bool CaptureExecution::Qualify(const Instance &instance,std::uint16_t i) noexcept {
  auto &state=instance.states[i];const auto &binding=instance.values[i];
  if(state.qualified)return true;
  if(binding.required&&!binding.qualify)return Reject(instance.root,CaptureFailure::MissingQualification,i);
  const auto bit=binding.qualification_slot<64?std::uint64_t{1}<<binding.qualification_slot:0;
  if(binding.qualify&&!(qualified_&bit)){
    if(!window_.Check(instance.root,out_))return Reject(instance.root,CaptureFailure::InvalidWindow,i);
    if(!bit)ResetCopy();
    if(!binding.qualify(Access(instance,i)))return Reject(instance.root,CaptureFailure::QualificationFailed,i);
    if(!bit&&!window_.Check(instance.root,out_))return Reject(instance.root,CaptureFailure::InvalidWindow,i);
    qualified_|=bit;
  }
  state.qualified=true;return true;
}
bool CaptureExecution::Open(const Instance &instance,std::uint16_t i) noexcept {
  auto &state=instance.states[i];const auto &root=instance.root;const auto &field=instance.fields[i];
  if(state.opened)return true;
  if(!Qualify(instance,i))return false;
  const auto &binding=instance.operations[i];
  if(binding.open){
    if(!binding.in_read_segment){
      ResetCopy();
      if(!window_.Check(root,out_))return Reject(root,CaptureFailure::InvalidWindow,i);
    }
    if(!binding.open(Access(instance,i),state.source,out_)||out_.reason!=CaptureFailure::None)
      return Reject(root,CaptureFailure::MissingValue,field.index);
    if(!binding.in_read_segment&&!window_.Check(root,out_))return Reject(root,CaptureFailure::InvalidWindow,i);
  }else if(!binding.inline_child||!root.address)return Reject(root,CaptureFailure::MissingHandler,field.index);
  if(field.operation==Operation::Container){
    const auto count=state.source.count;
    if(count>field.limit || count>UINT16_MAX || (field.count_bytes==1 && count>UINT8_MAX) ||
       (field.fixed_count && count!=field.fixed_count))return Reject(root,CaptureFailure::CountMismatch,field.index);
  }
  if((field.operation==Operation::Container || instance.values[i].format.shared_owner) && !state.source.identity)
    return Reject(root,CaptureFailure::MissingValue,field.index);
  if(field.operation==Operation::Choice && (field.limit>=32 || state.source.slot_mask&~((1u<<field.limit)-1u)))
    return Reject(root,CaptureFailure::InvalidValue,field.index);
  state.opened=true;return true;
}
bool CaptureExecution::Close(const Instance &instance,std::uint16_t i) noexcept {
  const auto &binding=instance.operations[i];
  if(!binding.close)return true;
  if(!binding.in_read_segment&&!window_.Check(instance.root,out_))return Reject(instance.root,CaptureFailure::InvalidWindow,i);
  if(!binding.close(Access(instance,i),instance.states[i].source,out_)||out_.reason!=CaptureFailure::None)
    return Reject(instance.root,CaptureFailure::SourceChanged,i);
  return binding.in_read_segment||window_.Check(instance.root,out_)||Reject(instance.root,CaptureFailure::InvalidWindow,i);
}

// 同组在第一项复制前预读全部选中投影数量，按slot去重求和；不读取任何元素补count。
bool CaptureExecution::CheckCounts(const Instance &instance,std::span<const CountConstraint> group) noexcept {
  const auto &root=instance.root;auto states=instance.states;
  auto &done=states[group.front().field].count_group_done;
  if(done){++counts_.group_reuses;return true;}
  ++counts_.group_checks;
  std::uint32_t sum=0,slot_value=0;std::uint16_t slot=kNullIndex;std::uint64_t identity=0;
  for(const auto &row:group){
    const auto &field=instance.fields[row.field];
    if(!(instance.mask&(std::uint32_t{1}<<field.block)))continue;
    bool selected=false,zero=false;
    if(!Selected(instance,row.field,selected,zero))return false;
    if(!selected || zero)return Reject(root,CaptureFailure::CountMismatch,field.index);
    auto &state=states[row.field];if(!Open(instance,row.field))return false;
    if(slot==row.slot){if(slot_value!=state.source.count || identity!=state.source.identity)return Reject(root,CaptureFailure::CountMismatch,field.index);}
    else{slot=row.slot;slot_value=state.source.count;identity=state.source.identity;
      if(sum>row.limit||slot_value>row.limit-sum)return Reject(root,CaptureFailure::CountMismatch,field.index);sum+=slot_value;}
  }
  done=true;return true;
}
// 共享 holder 先核触发投影，再复核本选块内其余投影的存在性与身份。
bool CaptureExecution::CheckShared(const Instance &instance,std::uint16_t trigger) noexcept {
  const auto &root=instance.root;auto states=instance.states;
  const auto canonical=instance.values[trigger].format.canonical;
  auto &done=states[canonical].shared_group_done;
  if(done){++counts_.group_reuses;return true;}
  ++counts_.group_checks;
  if(!Open(instance,trigger))return false;
  const auto &source=states[trigger].source;
  const auto &binding=instance.operations[canonical];
  for(auto i:std::span(executor_.shared_).subspan(binding.shared_first,binding.shared_count)){
    if(i==trigger||!(instance.mask&(std::uint32_t{1}<<instance.fields[i].block)))continue;
    bool selected=false,zero=false;if(!Selected(instance,i,selected,zero))return false;
    if(!selected || zero)return Reject(root,CaptureFailure::CountMismatch,i);
    if(!Open(instance,i))return false;
    if(source.identity!=states[i].source.identity || source.present!=states[i].source.present)
      return Reject(root,CaptureFailure::SourceChanged,i);
  }
  done=true;return true;
}

// 只消费当前实例已经封存的先行selector；条件为假及规范零均不访问旧负载。
bool CaptureExecution::Selected(const Instance &instance,std::uint16_t i,bool &selected,bool &zero) noexcept {
  const auto &root=instance.root;const auto &field=instance.fields[i];const auto states=instance.states;
  const auto &format=instance.values[i].format;selected=true;zero=false;
  if(format.conditional){
    if(format.selector>=states.size() || !states[format.selector].seen)return Reject(root,CaptureFailure::MissingValue,field.index);
    const auto &selector=states[format.selector];const auto value=selector.bits;
    if(format.predicate==PredicateKind::ItemConfig){
      ResetCopy();
      const auto &source=instance.values[format.selector].format;CodecItemConfig item;
      if(!selector.linked || selector.link.kind==CapturedReferenceKind::Null || !executor_.context_.item ||
         !window_.Check(root,out_)||!executor_.context_.item(executor_.context_.context,window_.witness,source.resource_table,selector.link,item) ||
         !window_.Check(root,out_))return Reject(root,CaptureFailure::ResourcePending,field.index);
      selected=item.id==format.operand && item.type<64 && (format.choices&(std::uint64_t{1}<<item.type));
      if(selected && !item.cooldown)return Reject(root,CaptureFailure::InvalidValue,field.index);
    }else{
      if(!selector.integer)return Reject(root,CaptureFailure::InvalidValue,field.index);
      switch(format.predicate){
        case PredicateKind::Equal:selected=value==format.operand;break;
        case PredicateKind::NotEqual:selected=value!=format.operand;break;
        case PredicateKind::AnyBits:selected=(value&format.operand)!=0;break;
        case PredicateKind::OneOf:selected=value<64 && (format.choices&(std::uint64_t{1}<<value));break;
        default:return Reject(root,CaptureFailure::InvalidValue,field.index);
      }
    }
  }
  if(selected && format.canonical_zero){
    if(format.zero_selector>=states.size() || !states[format.zero_selector].seen || !states[format.zero_selector].integer)
      return Reject(root,CaptureFailure::MissingValue,field.index);
    const auto value=states[format.zero_selector].bits;
    zero=format.zero_rule==BodyZeroRule::IfZero?value==0:format.zero_rule==BodyZeroRule::IfNullIndex && value==kNullIndex;
  }
  return true;
}

// 原生复制仍经 Copy 的窗口／地址与受保护读取；同实例的连续复制段共用已取得的 arena 字节。
bool CaptureExecution::Direct(Instance &instance,std::uint16_t i,CaptureWrittenValue &written) noexcept {
  const auto &field=instance.fields[i];const auto &bound=instance.values[i];const auto loc=Location(instance,i);
  const auto *range=instance.operations[i].copy?instance.operations[i].copy:bound.copy;
  const auto offset=range?range->native_offset:field.native_offset;
  if(!range||range!=copy_range_){
    const auto size=range?range->size:field.native_size;
    auto dest=writer_.ReserveScalars(size,loc);
    if(dest.size()!=size||!window_.Copy(instance.root,offset,dest,out_))return Reject(instance.root,CaptureFailure::ReadFailed,i);
    copied_=dest;copy_range_=range;
  }
  ++counts_.bound_writes;
  if(!writer_.WriteBoundScalar(loc,instance.path,bound.format.scalar,bound.width,bound.format.components,
      copied_.subspan(field.native_offset-offset,field.native_size),true,written))
    return Reject(instance.root,CaptureFailure::InvalidValue,i);
  return true;
}

// 具名叶仍经受检 writer，并核其确实只写当前路径的一条值。内部写入直接返回借用视图。
bool CaptureExecution::Leaf(Instance &instance,std::uint16_t i,std::uint32_t count) noexcept {
  const auto &root=instance.root;const auto &field=instance.fields[i];const auto &bound=instance.values[i];
  const auto &format=bound.format;auto &state=instance.states[i];
  if(!Visit(root,i,count)||!Qualify(instance,i))return false;
  const auto width=bound.width;
  const auto elements=std::uint64_t(count)*format.components;
  if(!width || elements>UINT32_MAX)return Reject(root,CaptureFailure::InvalidValue,field.index);
  if(!Add(root,field,elements*width))return false;
  CaptureWrittenValue value;
  const auto kind=format.linked?(format.resource_table==255?CaptureCellKind::References:CaptureCellKind::Resources):CaptureCellKind::Scalar;
  if(!count && field.operation==Operation::Container){
    if(format.linked){
      if(!Write(instance,i,kind,0,format.resource_table))return Reject(root,CaptureFailure::InvalidValue,i);
      value={ScalarKind::U16,0,format.resource_table,{}};
    }else{
      ++counts_.bound_writes;
      if(!writer_.WriteBoundScalar(Location(instance,i),instance.path,format.scalar,width,0,{},false,value))
        return Reject(root,CaptureFailure::InvalidValue,i);
    }
  }else if(bound.handler){
    ResetCopy();
    const auto access=Access(instance,i,field.operation==Operation::Container?&state.source:nullptr);
    const auto before=writer_.WrittenCells();
    if(!window_.Check(root,out_))return Reject(root,CaptureFailure::InvalidWindow,i);
    ++counts_.checked_writes;
    if(!bound.handler(access,writer_,out_))return Reject(root,CaptureFailure::MissingValue,i);
    if(!window_.Check(root,out_))return Reject(root,CaptureFailure::InvalidWindow,i);
    if(writer_.WrittenCells()!=before+1||!writer_.LastValue(access.location,kind,value))
      return Reject(root,CaptureFailure::MissingValue,i);
  }else if(!bound.direct)return Reject(root,CaptureFailure::MissingHandler,i);
  else if(!Direct(instance,i,value))return false;
  if(value.count!=elements)return Reject(root,CaptureFailure::MissingValue,i);
  if(format.linked){
    if(value.aux!=format.resource_table || value.bytes.size()!=elements*9)return Reject(root,CaptureFailure::InvalidValue,i);
    if(format.nonnull)for(std::size_t offset=0;offset<value.bytes.size();offset+=9)
      if(value.bytes[offset]==std::byte(CapturedReferenceKind::Null))return Reject(root,CaptureFailure::InvalidValue,i);
    if(field.operation!=Operation::Container && value.count==1){state.linked=true;
      state.link={CapturedReferenceKind(std::to_integer<unsigned>(value.bytes[0])),Integer(value.bytes.subspan(1,8))};}
  }else{
    if(value.scalar!=format.scalar || !ValidBoundBodyScalars(instance.layout.first_field+i,value.bytes))
      return Reject(root,CaptureFailure::InvalidValue,i);
    if(field.operation!=Operation::Container && value.count==1 && width<=8 && IsInteger(value.scalar)){
      state.integer=true;state.bits=Integer(value.bytes);state.scalar=value.scalar;}
  }
  return true;
}

// 静态父子关系取自绑定计划，只有实际数组位置、choice 布局和对象有效范围在这里决定。
bool CaptureExecution::Child(Instance &instance,std::uint16_t i,std::uint16_t element,FieldKey *key) noexcept {
  const auto &root=instance.root;const auto &field=instance.fields[i];const auto &state=instance.states[i];
  if(depth_>=limits_.depth)return Reject(root,CaptureFailure::TraversalBudget,i);
  const auto &binding=instance.operations[i];const auto *parent=binding.parent;
  if(!parent)return Reject(root,CaptureFailure::UnknownLayout,i);
  if(parent->operation==Operation::Container||parent->operation==Operation::Choice){
    if(element==kNullIndex||element>=parent->limit||(parent->fixed_count&&element>=parent->fixed_count))
      return Reject(root,CaptureFailure::InvalidValue,i);
  }else if(element!=kNullIndex)return Reject(root,CaptureFailure::InvalidValue,i);
  CaptureRoot child{root.subject,0,0,field.nested_layout};
  if(binding.element){
    if(!binding.in_read_segment&&!window_.Check(root,out_))return Reject(root,CaptureFailure::InvalidWindow,i);
    if(!binding.element(Access(instance,i),state.source,element,child,out_)||out_.reason!=CaptureFailure::None)
      return Reject(root,CaptureFailure::MissingValue,i);
  }else{
    const auto *layout=binding.child;
    if(binding.open || !layout || field.native_offset>root.extent || field.native_size>root.extent-field.native_offset ||
       layout->native_size>field.native_size || std::uint64_t(root.address)+field.native_offset>UINT32_MAX)
      return Reject(root,CaptureFailure::MissingHandler,i);
    child.address=root.address+field.native_offset;child.extent=field.native_size;
  }
  const auto *layout=field.operation==Operation::Choice?FindLayout(child.layout):binding.child;
  if(child.subject!=root.subject || !layout || (!layout->protocol && (!child.address || child.extent<layout->native_size)))
    return Reject(root,CaptureFailure::UnknownLayout,i);
  auto mask=binding.child_mask;
  if(field.operation==Operation::Choice){
    const bool allowed=std::any_of(binding.choices.begin(),binding.choices.end(),[&](const auto &choice){return choice.choice==child.layout;});
    if(!allowed||parent->operation!=Operation::Choice)return Reject(root,CaptureFailure::UnknownLayout,i);
    if(parent!=&field){
      const auto choices=instance.operations[parent->index].choices;
      if(std::none_of(choices.begin(),choices.end(),[&](const auto &choice){return choice.choice==child.layout;}))
        return Reject(root,CaptureFailure::UnknownLayout,i);
    }
    if(!Write(instance,i,CaptureCellKind::SlotLayout,child.layout,element)||!Add(root,field,2))
      return Reject(root,CaptureFailure::InvalidValue,i);
    mask=executor_.BoundLayout(*layout).masks[unsigned(BodyStream::Value)];
    if(field.nested_block!=255&&field.nested_block<layout->block_count)mask&=std::uint32_t{1}<<field.nested_block;
  }else if(child.layout!=field.nested_layout || parent->operation==Operation::Choice || parent->nested_layout!=child.layout)
    return Reject(root,CaptureFailure::UnknownLayout,i);
  if(field.nested_block!=255&&field.nested_block>=layout->block_count)return Reject(root,CaptureFailure::UnknownLayout,i);
  path_[depth_++]={root.layout,parent->index,element,field.operation==Operation::Choice?child.layout:std::uint16_t{0}};
  const bool success=Walk(child,*layout,mask,BodyStream::Value,key);
  --depth_;ResetCopy();return success;
}

// 结构由执行器写事实和递归，来源owner只解析当前元素；避免handler越权写整棵子树。
bool CaptureExecution::FieldValue(Instance &instance,std::uint16_t i) noexcept {
  const auto &root=instance.root;const auto &field=instance.fields[i];const auto &format=instance.values[i].format;
  auto &state=instance.states[i];
  // 每个递归实例的 canonical 字段保留唯一 count/presence，投影仍独立核来源与正文宽度。
  auto &canonical=instance.states[format.canonical];
  if(field.operation==Operation::Container){
    if(!Open(instance,i))return false;
    if(canonical.count_written){if(canonical.canonical_count!=state.source.count)return Reject(root,CaptureFailure::CountMismatch,field.index);}
    else if(!Write(instance,i,CaptureCellKind::Count,state.source.count))return Reject(root,CaptureFailure::InvalidValue,i);
    canonical.count_written=true;canonical.canonical_count=state.source.count;
    if(!field.fixed_count && !Add(root,field,field.count_bytes))return false;
    if(!field.nested_layout){if(!Leaf(instance,i,state.source.count))return false;}
    else{FieldKey previous;bool have_key=false;
      const auto &binding=instance.operations[i];
      if(!binding.child)return Reject(root,CaptureFailure::UnknownLayout,i);
      const auto minimum=binding.child_minimum*state.source.count;
      // 连最短合法元素都放不下时，在触碰首个元素前失败；实际变长长度仍逐项核算。
      if(minimum>limits_.bytes-bytes_ || minimum+(field.fixed_count?0:field.count_bytes)>field.max_size)
        return Reject(root,CaptureFailure::ValueBudget,field.index);
      if(state.source.count>limits_.field_visits-visits_)return Reject(root,CaptureFailure::TraversalBudget,field.index);
      const bool ordered=format.storage==BodyStorage::OrderedMap;
      for(std::uint32_t index=0;index<state.source.count;++index){FieldKey key;
        if(!Child(instance,i,std::uint16_t(index),ordered?&key:nullptr))return false;
        if(ordered){
          if(!key.integer || (have_key && (key.scalar!=previous.scalar || !LessInteger(previous.bits,key.bits,key.scalar))))
            return Reject(root,CaptureFailure::InvalidValue,field.index);
          previous=key;have_key=true;
        }
      }
    }
    return true;
  }
  if(field.operation==Operation::Choice){
    if(!format.slots||!Open(instance,i)||!Write(instance,i,CaptureCellKind::SlotMask,state.source.slot_mask)||!Add(root,field,1))
      return Reject(root,CaptureFailure::InvalidValue,field.index);
    for(std::uint8_t slot=0;slot<field.limit;++slot)if(state.source.slot_mask&(1u<<slot))
      if(!Child(instance,i,slot))return false;
    return true;
  }
  if(field.nested_layout){
    if(!Open(instance,i))return false;
    if(format.shared_owner){
      if(canonical.presence_written){if(canonical.canonical_presence!=state.source.present)return Reject(root,CaptureFailure::SourceChanged,field.index);}
      else if(!Write(instance,i,CaptureCellKind::Presence,state.source.present?1u:0u))return Reject(root,CaptureFailure::InvalidValue,i);
      canonical.presence_written=true;canonical.canonical_presence=state.source.present;
      if(!state.source.present)return Add(root,field,field.min_size);
      if((field.nested_block==0 || field.nested_block==255) && !Add(root,field,1))return false;
    }
    return Child(instance,i,kNullIndex);
  }
  return Leaf(instance,i,1);
}

// 按生成块序遍历完整选流并核实际长度；失败上交Execute统一回卷，不留下部分根。
bool CaptureExecution::Walk(const CaptureRoot &root,const Layout &layout,std::uint32_t mask,BodyStream stream,FieldKey *key) noexcept {
  const auto &bound=executor_.BoundLayout(layout);
  if(!bound.valid||!window_.Check(root,out_)||!Visit(root))return Reject(root,CaptureFailure::UnknownLayout);
  const auto first=layout.first_field,count=layout.field_count;
  auto states=std::span(executor_.states_).subspan(std::size_t(depth_)*executor_.field_stride_,count);
  Instance instance{root,layout,mask,states,Fields().subspan(first,count),
    std::span(executor_.plan_.fields_).subspan(first,count),std::span(executor_.bindings_).subspan(first,count),{}};
  std::fill(states.begin(),states.end(),State{});ResetCopy();const auto begin=bytes_;
  for(std::uint8_t block=0;block<layout.block_count;++block)if(mask&(std::uint32_t{1}<<block)){
    const auto &description=Blocks()[layout.first_block+block];const auto block_begin=bytes_;
    for(auto i:CaptureBlockFields(layout.first_block+block)){
      const auto &field=instance.fields[i];const auto &value=instance.values[i];const auto &format=value.format;
      if(!Visit(root,i))return false;
      if(!value.valid||format.canonical>=count)return Reject(root,CaptureFailure::InvalidValue,i);
      if(instance.operations[i].scalar_leaf){
        // 冷计划已证明 native_size == min_size == max_size == width*components。
        // 保持原来的双次访问计数、动态资格、窗口复制、arena边界与纯值validator，省去结构分派。
        if(!Visit(root,i)||!Qualify(instance,i)||!Add(root,field,field.native_size))return false;
        CaptureWrittenValue written;
        if(!Direct(instance,i,written))return false;
        if(!ValidBoundBodyScalars(first+i,written.bytes))return Reject(root,CaptureFailure::InvalidValue,i);
        auto &state=states[i];
        if(format.components==1&&IsInteger(format.scalar)){
          state.integer=true;state.bits=Integer(written.bytes);state.scalar=format.scalar;
        }
        state.seen=true;continue;
      }
      bool selected=false,zero=false;if(!Selected(instance,i,selected,zero))return false;
      if(!selected)continue;
      auto &state=states[i];const auto field_begin=bytes_;
      if(zero){
        const auto size=std::uint64_t(value.width)*format.components;
        if(field.min_size!=field.max_size || size!=field.max_size || size>executor_.copy_.size() || format.linked)
          return Reject(root,CaptureFailure::InvalidValue,i);
        auto zeros=std::span(executor_.copy_).first(std::size_t(size));std::fill(zeros.begin(),zeros.end(),std::byte{});
        CaptureWrittenValue written;
        ++counts_.bound_writes;
        if(!writer_.WriteBoundScalar(Location(instance,i),instance.path,format.scalar,value.width,format.components,zeros,false,written)||
           !ValidBoundBodyScalars(first+i,written.bytes)||!Add(root,field,size))return Reject(root,CaptureFailure::InvalidValue,i);
        if(format.components==1 && IsInteger(format.scalar)){state.integer=true;state.scalar=format.scalar;state.bits=0;}
      }else{
        if(format.shared_owner&&!CheckShared(instance,i))return false;
        const auto counts=instance.operations[i].counts;
        if(!counts.empty()&&!CheckCounts(instance,counts))return false;
        if(!FieldValue(instance,i))return false;
      }
      state.seen=true;
      if(bytes_-field_begin<field.min_size || bytes_-field_begin>field.max_size)return Reject(root,CaptureFailure::ValueBudget,i);
    }
    if(bytes_-block_begin<description.min_size || bytes_-block_begin>description.max_size)return Reject(root,CaptureFailure::ValueBudget);
  }
  if(const auto *budget=bound.budget){const auto size=bytes_-begin;
    if(size>budget->record_bytes)return Reject(root,CaptureFailure::ValueBudget);
    if(!limits_.reliable && stream==BodyStream::Body && size>budget->body_bytes)return Reject(root,CaptureFailure::NeedsOrdinaryJoint);
  }
  // 最后再核一次所有预读来源，防止后续子对象遍历使早期容器／holder证言失效。
  for(std::uint16_t i=0;i<count;++i)if(states[i].opened&&!Close(instance,i))return false;
  if(!window_.Check(root,out_))return Reject(root,CaptureFailure::InvalidWindow);
  if(key&&!states.empty()){const auto &first_state=states.front();*key={first_state.bits,first_state.scalar,first_state.integer};}
  return true;
}

bool CaptureExecutor::Execute(const CaptureWindow &window,CaptureRoot root,BodyStream stream,CaptureValueWriter &writer,
    CaptureDiagnostic &out,CaptureExecutionLimits limits) noexcept {
  return ExecuteBound(window,root,stream,writer,out,limits,nullptr);
}
bool CaptureExecutor::ExecuteBound(const CaptureWindow &window,CaptureRoot root,BodyStream stream,CaptureValueWriter &writer,
    CaptureDiagnostic &out,CaptureExecutionLimits limits,RootReview review,bool append_body) noexcept {
  const auto reject=[&](CaptureFailure reason){if(writer.Diagnostic().reason!=CaptureFailure::None && out.reason==CaptureFailure::None)out=writer.Diagnostic();
    FailCapture(out,reason,root);writer.Fail(out);return false;};
  if(std::this_thread::get_id()!=owner_ || window.thread!=owner_)return reject(CaptureFailure::WrongThread);
  if(active_)return reject(CaptureFailure::Reentrant);
  if(out.reason!=CaptureFailure::None || writer.Diagnostic().reason!=CaptureFailure::None)return reject(CaptureFailure::Incomplete);
  if(!limits.bytes || limits.bytes>(32u<<20) || (!limits.reliable && limits.bytes>65536) ||
     !limits.field_visits || !limits.depth || limits.depth>capacity_.depth)return reject(CaptureFailure::InvalidValue);
  if(unsigned(stream)>unsigned(BodyStream::Value))return reject(CaptureFailure::InvalidValue);
  if(window.witness!=writer.Witness())return reject(CaptureFailure::InvalidWindow);
  if((window.witness.phase==CapturePhase::PlayerPost && (stream!=BodyStream::Body || !window.witness.required_revision)) ||
     (window.witness.phase==CapturePhase::Production && stream!=BodyStream::Value) ||
     (limits.reliable && stream==BodyStream::Body && window.witness.phase!=CapturePhase::Ordinary))return reject(CaptureFailure::InvalidWindow);
  if(append_body&&(window.witness.phase!=CapturePhase::Ordinary||stream!=BodyStream::World||!limits.reliable||!review))
    return reject(CaptureFailure::InvalidWindow);
  if(append_body&&writer.open_)return reject(CaptureFailure::InvalidTransaction);
  active_=true;
  struct Reset {bool &active;~Reset(){active=false;}} reset{active_};
  const auto *layout=FindLayout(root.layout);
  if(!layout && root.layout)return reject(CaptureFailure::UnknownLayout);
  if(!review&&(!layout||layout->rtti_rva)){if(!plan_.Identify(window,root,out))return reject(CaptureFailure::UnknownLayout);layout=FindLayout(root.layout);}
  else if(!window.Check(root,out))return reject(CaptureFailure::InvalidWindow);
  if(!layout||!BoundLayout(*layout).valid)return reject(CaptureFailure::UnknownLayout);
  const auto &bound=BoundLayout(*layout);
  const auto write_stream=[&](BodyStream selected,RootReview finish,capture::ExecutionCounts &counts){
    const auto mask=bound.masks[unsigned(selected)];
    if(!writer.OpenRoot(root.subject,root.layout,selected,mask))return reject(CaptureFailure::InvalidTransaction);
    CaptureExecution execution(*this,window,writer,out,limits);
    const bool traversed=execution.Walk(root,*layout,mask,selected);
    counts=execution.Counts();
    if(!traversed||(finish&&(!finish(window.context,root,out)||!window.Check(root,out)))){
      reject(CaptureFailure::Incomplete);writer.AbortRoot();
      return false;
    }
    if(selected==BodyStream::Body&&execution.Bytes()<=65536)
      writer.MarkFastBody(!bound.budget||execution.Bytes()<=bound.budget->body_bytes,append_body);
    return writer.FinishRoot();
  };
  capture::ExecutionCounts primary;
  if(!append_body){
    const bool complete=write_stream(stream,review,primary);
    if(context_.observe)context_.observe(primary);
    return complete;
  }
  // World 的记录先留在同一事务内，Body 写完并统一复核以后才离开执行器。
  const auto records=writer.records_,cells=writer.cells_,paths=writer.paths_,bytes=writer.bytes_;
  const auto references=writer.reference_limit_;
  const auto previous_root=writer.root_;
  capture::ExecutionCounts body;
  const bool complete=write_stream(BodyStream::World,nullptr,primary)&&write_stream(BodyStream::Body,review,body);
  if(!complete){
    writer.records_=records;writer.cells_=cells;writer.paths_=paths;writer.bytes_=bytes;
    writer.reference_limit_=references;writer.root_=previous_root;
  }
  if(context_.observe){context_.observe(primary);context_.observe(body);}
  return complete;
}
}
