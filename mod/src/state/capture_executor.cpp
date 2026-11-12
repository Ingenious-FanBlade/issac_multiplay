#include "state/capture_executor.h"
#include <algorithm>
#include <bit>

namespace cs::state {
namespace {
// 有界纯值小端读取；调用点已核恰好1..8字节，不将原生对象作未对齐强转。
std::uint64_t Integer(std::span<const std::byte> bytes) noexcept {
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
public:
  CaptureExecution(CaptureExecutor &executor,const CaptureWindow &window,CaptureValueWriter &writer,
      CaptureDiagnostic &diagnostic,CaptureExecutionLimits limits) noexcept
      :executor_(executor),window_(window),writer_(writer),out_(diagnostic),limits_(limits),scratch_{executor.copy_} {}
  bool Walk(const CaptureRoot &root,std::uint32_t mask,BodyStream stream) noexcept;
private:
  using State=CaptureExecutor::FieldState;
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
  const CaptureExecutor::Binding &Binding(const Field &field) const noexcept {
    return executor_.bindings_[FindLayout(field.layout)->first_field+field.index];
  }
  CaptureFieldAccess Access(const CaptureRoot &root,const Field &field) const noexcept {
    const auto &format=*executor_.plan_.Format(root.layout,field.index);
    return {window_,root,field,format,{Parents(),root.layout,field.index,format.canonical}};
  }
  bool Qualify(const CaptureRoot &root,const Field &field) noexcept {
    return executor_.plan_.Qualify(window_,root,field.index,Parents(),writer_,out_);
  }
  bool Open(const CaptureRoot &,const Field &,State &) noexcept;
  bool Close(const CaptureRoot &,const Field &,const State &) noexcept;
  bool Counts(const CaptureRoot &,const Layout &,std::uint32_t mask,std::span<State>,std::uint16_t group) noexcept;
  bool Shared(const CaptureRoot &,const Layout &,std::uint32_t mask,std::span<State>,const Field &) noexcept;
  bool Selected(const CaptureRoot &,const Field &,std::span<State>,bool &selected,bool &zero) noexcept;
  bool Leaf(const CaptureRoot &,const Field &,State &,std::uint32_t count) noexcept;
  bool Child(const CaptureRoot &,const Field &,const State &,std::uint16_t element,State &key) noexcept;
  bool FieldValue(const CaptureRoot &,const Field &,State &) noexcept;
  CaptureExecutor &executor_;
  const CaptureWindow &window_;
  CaptureValueWriter &writer_;
  CaptureDiagnostic &out_;
  CaptureExecutionLimits limits_;
  CaptureCopyScratch scratch_;
  std::array<CodecPathStep,kCodecMaxDepth> path_{};
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
  const auto states=std::uint64_t(capacity.depth+1)*BodySourceMaximumFields();
  memory_=memory::Reserve(memory::Pool::Workspace,sizeof(*this)+128+Fields().size()*sizeof(Binding)+states*sizeof(FieldState)+capacity.copy_bytes,
    "state_capture_executor");
  bindings_.resize(Fields().size());states_.resize(std::size_t(states));copy_.resize(capacity.copy_bytes);
  for(std::size_t i=0;i<Fields().size();++i){const auto &field=Fields()[i];
    for(const auto &binding:bindings)if(binding.operation==field.operation && binding.name==(field.handler?field.handler:""))
      bindings_[i]={binding.open,binding.element,binding.close};
  }
  memory_->Resize(sizeof(*this)+128+bindings_.capacity()*sizeof(Binding)+states_.capacity()*sizeof(FieldState)+copy_.capacity());
}

bool CaptureExecution::Open(const CaptureRoot &root,const Field &field,State &state) noexcept {
  if(state.opened)return Qualify(root,field);
  if(!Qualify(root,field))return false;
  const auto access=Access(root,field);const auto &binding=Binding(field);
  if(binding.open){
    if(!binding.open(access,state.source,out_) || out_.reason!=CaptureFailure::None)
      return Reject(root,CaptureFailure::MissingValue,field.index);
  }else{
    const auto *layout=FindLayout(root.layout),*child=FindLayout(field.nested_layout);
    // 仅真正内嵌、无特殊owner的对象视图可自动加偏移；其余来源必须具名解析。
    if(!child || !layout || !root.address || layout->protocol || child->protocol || field.source!=FieldSource::Object ||
       (field.operation!=Operation::Nested && field.operation!=Operation::Copy) ||
       access.format.storage!=BodyStorage::None || (field.handler && field.handler[0]))
      return Reject(root,CaptureFailure::MissingHandler,field.index);
  }
  if(field.operation==Operation::Container){
    const auto count=state.source.count;
    if(count>field.limit || count>UINT16_MAX || (field.count_bytes==1 && count>UINT8_MAX) ||
       (field.fixed_count && count!=field.fixed_count))return Reject(root,CaptureFailure::CountMismatch,field.index);
  }
  if((field.operation==Operation::Container || access.format.shared_owner) && !state.source.identity)
    return Reject(root,CaptureFailure::MissingValue,field.index);
  if(field.operation==Operation::Choice && (field.limit>=32 || state.source.slot_mask&~((1u<<field.limit)-1u)))
    return Reject(root,CaptureFailure::InvalidValue,field.index);
  if(!Qualify(root,field))return false;
  state.opened=true;return true;
}
bool CaptureExecution::Close(const CaptureRoot &root,const Field &field,const State &state) noexcept {
  const auto &binding=Binding(field);
  if(binding.close && (!binding.close(Access(root,field),state.source,out_) || out_.reason!=CaptureFailure::None))
    return Reject(root,CaptureFailure::SourceChanged,field.index);
  return Qualify(root,field);
}

// 同组在第一项复制前预读全部选中投影数量，按slot去重求和；不读取任何元素补count。
bool CaptureExecution::Counts(const CaptureRoot &root,const Layout &layout,std::uint32_t mask,std::span<State> states,std::uint16_t group) noexcept {
  std::uint32_t sum=0,slot_value=0;std::uint16_t slot=kNullIndex;std::uint64_t identity=0;
  for(const auto &row:CountConstraints())if(row.layout==layout.id && row.group==group){
    const auto &field=Fields()[layout.first_field+row.field];
    if(!(mask&(std::uint32_t{1}<<field.block)))continue;
    bool selected=false,zero=false;
    if(!Selected(root,field,states,selected,zero))return false;
    if(!selected || zero)return Reject(root,CaptureFailure::CountMismatch,field.index);
    auto &state=states[row.field];if(!Open(root,field,state))return false;
    if(slot==row.slot){if(slot_value!=state.source.count || identity!=state.source.identity)return Reject(root,CaptureFailure::CountMismatch,field.index);}
    else{slot=row.slot;slot_value=state.source.count;identity=state.source.identity;
      if(slot_value>row.limit-sum)return Reject(root,CaptureFailure::CountMismatch,field.index);sum+=slot_value;}
  }
  return true;
}
bool CaptureExecution::Shared(const CaptureRoot &root,const Layout &layout,std::uint32_t mask,std::span<State> states,const Field &field) noexcept {
  const auto &format=*executor_.plan_.Format(root.layout,field.index);
  if(!Open(root,field,states[field.index]))return false;
  const auto &source=states[field.index].source;
  for(std::uint16_t i=0;i<layout.field_count;++i){const auto &other=Fields()[layout.first_field+i];
    const auto &info=*executor_.plan_.Format(root.layout,i);
    if(i==field.index || !(mask&(std::uint32_t{1}<<other.block)) || !info.shared_owner || info.canonical!=format.canonical)continue;
    bool selected=false,zero=false;if(!Selected(root,other,states,selected,zero))return false;
    if(!selected || zero)return Reject(root,CaptureFailure::CountMismatch,i);
    if(!Open(root,other,states[i]))return false;
    if(source.identity!=states[i].source.identity || source.present!=states[i].source.present)
      return Reject(root,CaptureFailure::SourceChanged,i);
  }
  return true;
}

// 只消费当前实例已经封存的先行selector；条件为假及规范零均不访问旧负载。
bool CaptureExecution::Selected(const CaptureRoot &root,const Field &field,std::span<State> states,bool &selected,bool &zero) noexcept {
  if(!window_.Check(root,out_))return Reject(root,CaptureFailure::InvalidWindow,field.index);
  const auto &format=*executor_.plan_.Format(root.layout,field.index);selected=true;zero=false;
  if(format.conditional){
    if(format.selector>=states.size() || !states[format.selector].seen)return Reject(root,CaptureFailure::MissingValue,field.index);
    const auto &selector=states[format.selector];const auto value=selector.bits;
    if(format.predicate==PredicateKind::ItemConfig){
      const auto *source=executor_.plan_.Format(root.layout,format.selector);CodecItemConfig item;
      if(!selector.linked || selector.link.kind==CapturedReferenceKind::Null || !executor_.context_.item ||
         !executor_.context_.item(executor_.context_.context,window_.witness,source->resource_table,selector.link,item) ||
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

// 叶handler只能写当前路径的一条值；执行器逐项核验实际数量、空间、宽度和数字validator。
bool CaptureExecution::Leaf(const CaptureRoot &root,const Field &field,State &state,std::uint32_t count) noexcept {
  const auto access=Access(root,field);const auto before=writer_.WrittenCells();
  if(!Visit(root,field.index,count))return false;
  const auto width=CaptureScalarWidth(access.format.scalar);
  const auto elements=std::uint64_t(count)*access.format.components;
  if(!width || elements>UINT32_MAX)return Reject(root,CaptureFailure::InvalidValue,field.index);
  if(!Add(root,field,elements*width))return false;
  if(!count && field.operation==Operation::Container){
    if(access.format.linked){if(!writer_.PutLinks(access.location,access.format.resource_table!=255,access.format.resource_table,{}))return Reject(root,CaptureFailure::InvalidValue,field.index);}
    else if(!writer_.PutScalar(access.location,access.format.scalar,0,{}))return Reject(root,CaptureFailure::InvalidValue,field.index);
  }else if(!executor_.plan_.DispatchSelected(window_,root,field.index,Parents(),writer_,scratch_,out_,true,
      field.operation==Operation::Container?&state.source:nullptr))return false;
  CaptureWrittenValue value;
  const auto kind=access.format.linked?(access.format.resource_table==255?CaptureCellKind::References:CaptureCellKind::Resources):CaptureCellKind::Scalar;
  if(writer_.WrittenCells()!=before+1 || !writer_.LastValue(access.location,kind,value) || value.count!=elements)
    return Reject(root,CaptureFailure::MissingValue,field.index);
  if(access.format.linked){
    if(value.aux!=access.format.resource_table || value.bytes.size()!=elements*9)return Reject(root,CaptureFailure::InvalidValue,field.index);
    if(access.format.nonnull)for(std::size_t i=0;i<value.bytes.size();i+=9)if(value.bytes[i]==std::byte(CapturedReferenceKind::Null))
      return Reject(root,CaptureFailure::InvalidValue,field.index);
    if(field.operation!=Operation::Container && value.count==1){state.linked=true;
      state.link={CapturedReferenceKind(std::to_integer<unsigned>(value.bytes[0])),Integer(value.bytes.subspan(1,8))};}
  }else{
    if(value.scalar!=access.format.scalar || !ValidBodySourceScalars(root.layout,field.index,value.bytes))return Reject(root,CaptureFailure::InvalidValue,field.index);
    if(field.operation!=Operation::Container && value.count==1 && width<=8 && IsInteger(value.scalar)){
      state.integer=true;state.bits=Integer(value.bytes);state.scalar=value.scalar;}
  }
  return true;
}

bool CaptureExecution::Child(const CaptureRoot &root,const Field &field,const State &state,std::uint16_t element,State &key) noexcept {
  if(depth_>=limits_.depth)return Reject(root,CaptureFailure::TraversalBudget,field.index);
  if(!Qualify(root,field))return false;
  const auto access=Access(root,field);const auto &binding=Binding(field);
  CaptureRoot child{root.subject,0,0,field.nested_layout};
  if(binding.element){
    if(!binding.element(access,state.source,element,child,out_) || out_.reason!=CaptureFailure::None)return Reject(root,CaptureFailure::MissingValue,field.index);
  }else{
    const auto *layout=FindLayout(field.nested_layout);
    if(binding.open || !layout || field.native_offset>root.extent || field.native_size>root.extent-field.native_offset ||
       layout->native_size>field.native_size || std::uint64_t(root.address)+field.native_offset>UINT32_MAX)
      return Reject(root,CaptureFailure::MissingHandler,field.index);
    child.address=root.address+field.native_offset;child.extent=field.native_size;
  }
  const auto *layout=FindLayout(child.layout);
  if(child.subject!=root.subject || !layout || (!layout->protocol && (!child.address || child.extent<layout->native_size)))
    return Reject(root,CaptureFailure::UnknownLayout,field.index);
  if(field.operation==Operation::Choice){
    bool allowed=false;for(const auto &choice:LayoutChoices())if(choice.layout==root.layout && choice.field==field.index && choice.choice==child.layout){allowed=true;break;}
    if(!allowed)return Reject(root,CaptureFailure::UnknownLayout,field.index);
    if(!writer_.PutSlotLayout(access.location,std::uint8_t(element),child.layout) || !Add(root,field,2))return Reject(root,CaptureFailure::InvalidValue,field.index);
  }else if(child.layout!=field.nested_layout)return Reject(root,CaptureFailure::UnknownLayout,field.index);
  if(!Qualify(root,field) || !window_.Check(child,out_))return Reject(root,CaptureFailure::InvalidWindow,field.index);
  auto mask=executor_.plan_.Mask(child.layout,BodyStream::Value);
  if(field.nested_block!=255){if(field.nested_block>=layout->block_count)return Reject(root,CaptureFailure::UnknownLayout,field.index);
    mask&=std::uint32_t{1}<<field.nested_block;}
  path_[depth_++]={root.layout,access.format.canonical,element,field.operation==Operation::Choice?child.layout:std::uint16_t{0}};
  const bool success=Walk(child,mask,BodyStream::Value);
  if(success)key=executor_.states_[std::size_t(depth_)*BodySourceMaximumFields()];
  --depth_;return success && Qualify(root,field);
}

// 结构由执行器写事实和递归，来源owner只解析当前元素；避免handler越权写整棵子树。
bool CaptureExecution::FieldValue(const CaptureRoot &root,const Field &field,State &state) noexcept {
  const auto access=Access(root,field);
  if(field.operation==Operation::Container){
    if(!Open(root,field,state) || !writer_.PutCount(access.location,state.source.count))return Reject(root,CaptureFailure::InvalidValue,field.index);
    if(!field.fixed_count && !Add(root,field,field.count_bytes))return false;
    if(!field.nested_layout){if(!Leaf(root,field,state,state.source.count))return false;}
    else{State previous;bool have_key=false;
      const auto *child=FindLayout(field.nested_layout);std::uint64_t minimum=0;
      if(!child)return Reject(root,CaptureFailure::UnknownLayout,field.index);
      for(std::uint8_t block=0;block<child->block_count;++block)if(field.nested_block==255 || field.nested_block==block)
        minimum+=Blocks()[child->first_block+block].min_size;
      minimum*=state.source.count;
      // 连最短合法元素都放不下时，在触碰首个元素前失败；实际变长长度仍逐项核算。
      if(minimum>limits_.bytes-bytes_ || minimum+(field.fixed_count?0:field.count_bytes)>field.max_size)
        return Reject(root,CaptureFailure::ValueBudget,field.index);
      if(state.source.count>limits_.field_visits-visits_)return Reject(root,CaptureFailure::TraversalBudget,field.index);
      for(std::uint32_t i=0;i<state.source.count;++i){State key;
        if(!Child(root,field,state,std::uint16_t(i),key))return false;
        if(access.format.storage==BodyStorage::OrderedMap){
          if(!key.integer || (have_key && (key.scalar!=previous.scalar || !LessInteger(previous.bits,key.bits,key.scalar))))
            return Reject(root,CaptureFailure::InvalidValue,field.index);
          previous=key;have_key=true;
        }
      }
    }
    return Close(root,field,state);
  }
  if(field.operation==Operation::Choice){
    if(!Open(root,field,state) || !writer_.PutSlotMask(access.location,state.source.slot_mask) || !Add(root,field,1))
      return Reject(root,CaptureFailure::InvalidValue,field.index);
    for(std::uint8_t slot=0;slot<field.limit;++slot)if(state.source.slot_mask&(1u<<slot)){State ignored;
      if(!Child(root,field,state,slot,ignored))return false;}
    return Close(root,field,state);
  }
  if(field.nested_layout){
    if(!Open(root,field,state))return false;
    if(access.format.shared_owner){
      if(!writer_.PutPresence(access.location,state.source.present))return Reject(root,CaptureFailure::InvalidValue,field.index);
      if(!state.source.present){if(!Add(root,field,field.min_size))return false;return Close(root,field,state);}
      if((field.nested_block==0 || field.nested_block==255) && !Add(root,field,1))return false;
    }
    State ignored;return Child(root,field,state,kNullIndex,ignored) && Close(root,field,state);
  }
  return Leaf(root,field,state,1);
}

// 按生成块序遍历完整选流并核实际长度；失败上交Execute统一回卷，不留下部分根。
bool CaptureExecution::Walk(const CaptureRoot &root,std::uint32_t mask,BodyStream stream) noexcept {
  const auto *layout=FindLayout(root.layout);BodySourceFieldInfo info;
  if(!layout || layout->field_count>BodySourceMaximumFields() ||
     (layout->field_count && !DescribeBodySourceField(layout->id,0,info)) ||
     !window_.Check(root,out_) || !Visit(root))return Reject(root,CaptureFailure::UnknownLayout);
  auto states=std::span(executor_.states_).subspan(std::size_t(depth_)*BodySourceMaximumFields(),layout->field_count);
  std::fill(states.begin(),states.end(),State{});scratch_.Reset();const auto begin=bytes_;
  for(std::uint8_t block=0;block<layout->block_count;++block)if(mask&(std::uint32_t{1}<<block)){
    const auto &description=Blocks()[layout->first_block+block];const auto block_begin=bytes_;
    if(stream!=BodyStream::Value && description.body!=(stream==BodyStream::Body))return Reject(root,CaptureFailure::InvalidValue);
    for(std::uint16_t i=0;i<layout->field_count;++i){const auto &field=Fields()[layout->first_field+i];if(field.block!=block)continue;
      if(!Visit(root,i))return false;
      bool selected=false,zero=false;if(!Selected(root,field,states,selected,zero))return false;
      if(!selected)continue;
      auto &state=states[i];const auto field_begin=bytes_;
      if(zero){
        const auto access=Access(root,field);const auto size=std::uint64_t(CaptureScalarWidth(access.format.scalar))*access.format.components;
        if(field.min_size!=field.max_size || size!=field.max_size || size>executor_.copy_.size() || access.format.linked)
          return Reject(root,CaptureFailure::InvalidValue,i);
        scratch_.Reset();auto value=std::span(executor_.copy_).first(std::size_t(size));std::fill(value.begin(),value.end(),std::byte{});
        if(!ValidBodySourceScalars(root.layout,i,value) || !writer_.PutScalar(access.location,access.format.scalar,access.format.components,value) ||
           !Add(root,field,size))return Reject(root,CaptureFailure::InvalidValue,i);
        if(access.format.components==1 && IsInteger(access.format.scalar)){state.integer=true;state.scalar=access.format.scalar;state.bits=0;}
      }else{
        if(executor_.plan_.Format(root.layout,i)->shared_owner && !Shared(root,*layout,mask,states,field))return false;
        for(const auto &row:CountConstraints())if(row.layout==root.layout && row.field==i)
          if(!Counts(root,*layout,mask,states,row.group))return false;
        if(!FieldValue(root,field,state))return false;
      }
      state.seen=true;
      if(bytes_-field_begin<field.min_size || bytes_-field_begin>field.max_size)return Reject(root,CaptureFailure::ValueBudget,i);
    }
    if(bytes_-block_begin<description.min_size || bytes_-block_begin>description.max_size)return Reject(root,CaptureFailure::ValueBudget);
  }
  for(const auto &budget:LayoutBudgets())if(budget.layout==root.layout){const auto size=bytes_-begin;
    if(size>budget.record_bytes)return Reject(root,CaptureFailure::ValueBudget);
    if(!limits_.reliable && stream==BodyStream::Body && size>budget.body_bytes)return Reject(root,CaptureFailure::NeedsOrdinaryJoint);
    break;}
  // 最后再核一次所有预读来源，防止后续子对象遍历使早期容器／holder证言失效。
  for(std::uint16_t i=0;i<layout->field_count;++i)if(states[i].opened &&
      !Close(root,Fields()[layout->first_field+i],states[i]))return false;
  return window_.Check(root,out_) || Reject(root,CaptureFailure::InvalidWindow);
}

bool CaptureExecutor::Execute(const CaptureWindow &window,CaptureRoot root,BodyStream stream,CaptureValueWriter &writer,
    CaptureDiagnostic &out,CaptureExecutionLimits limits) noexcept {
  const auto reject=[&](CaptureFailure reason){if(writer.Diagnostic().reason!=CaptureFailure::None && out.reason==CaptureFailure::None)out=writer.Diagnostic();
    FailCapture(out,reason,root);writer.Fail(out);return false;};
  if(std::this_thread::get_id()!=owner_ || window.thread!=owner_)return reject(CaptureFailure::WrongThread);
  if(active_)return reject(CaptureFailure::Reentrant);
  if(out.reason!=CaptureFailure::None || writer.Diagnostic().reason!=CaptureFailure::None)return reject(CaptureFailure::Incomplete);
  if(!limits.bytes || limits.bytes>(32u<<20) || (!limits.reliable && limits.bytes>65536) ||
     !limits.field_visits || !limits.depth || limits.depth>capacity_.depth)return reject(CaptureFailure::InvalidValue);
  if(window.witness!=writer.Witness())return reject(CaptureFailure::InvalidWindow);
  if((window.witness.phase==CapturePhase::PlayerPost && (stream!=BodyStream::Body || !window.witness.required_revision)) ||
     (window.witness.phase==CapturePhase::Production && stream!=BodyStream::Value) ||
     (limits.reliable && stream==BodyStream::Body && window.witness.phase!=CapturePhase::Ordinary))return reject(CaptureFailure::InvalidWindow);
  active_=true;
  struct Reset {bool &active;~Reset(){active=false;}} reset{active_};
  const auto *layout=FindLayout(root.layout);
  if(!layout && root.layout)return reject(CaptureFailure::UnknownLayout);
  if(!layout || layout->rtti_rva){if(!plan_.Identify(window,root,out))return reject(CaptureFailure::UnknownLayout);layout=FindLayout(root.layout);}
  else if(!window.Check(root,out))return reject(CaptureFailure::InvalidWindow);
  BodySourceFieldInfo info;
  if(!layout || (layout->field_count && !DescribeBodySourceField(layout->id,0,info)))return reject(CaptureFailure::UnknownLayout);
  const auto mask=plan_.Mask(root.layout,stream);
  if(!writer.BeginRoot(root.subject,root.layout,stream,mask))return reject(CaptureFailure::InvalidTransaction);
  CaptureExecution execution(*this,window,writer,out,limits);
  if(!execution.Walk(root,mask,stream) || !window.Check(root,out) || !writer.FinishRoot()){
    reject(CaptureFailure::Incomplete);writer.AbortRoot();return false;}
  return true;
}
}
