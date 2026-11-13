#include "state/body_codec.h"
#include "state/entity_counters.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <tuple>

namespace cs::state {
namespace {
enum class ValueRule : std::uint8_t { None, NonNull, NullableWater, EntityCounter, Countdown, DisplayUtf8 };
enum class LayoutRuleKind : std::uint8_t { EmptyActiveZero, NullCostumeLayerZero };
// 字段表和下面的数字计划按同一次生成的全局索引一一对应，不在热路径解析字符串。
struct BodyFieldPlan {
  ScalarKind scalar;
  std::uint16_t components;
  BodyStorage storage;
  std::uint8_t table;
  std::uint16_t canonical, selector;
  PredicateKind predicate;
  std::uint64_t operand, choices;
  ValueRule rule;
  std::uint32_t validator;
  std::uint16_t group, slot, count_limit, copies, slots;
};
struct BodyValidator {
  Validation kind;
  ScalarKind scalar;
  std::uint64_t minimum, maximum;
  std::uint32_t first;
  std::uint16_t count;
};
struct BodyLayoutRule { std::uint16_t layout; LayoutRuleKind kind; std::uint16_t selector, other; };
#include "state_body_codec.inc"

[[noreturn]] void Fail(const char *reason) { throw Gap("body_codec", reason); }
[[noreturn]] void Fail(const Field &field, const char *reason) { throw Gap(field.name, reason); }
// 显示串只含Unicode文字；C0/C1及NUL不作为内嵌命令透传。
// 样式是独立有限编号，不调用原生首码解析器；其他域的原始字节串不经此规则。
bool DisplayCodepoint(std::uint32_t value) noexcept {
  return value>=0x20 && value<=0x10ffff && !(value>=0x7f && value<=0x9f) &&
         !(value>=0xd800 && value<=0xdfff);
}
const char *DisplayUtf8Error(std::span<const std::byte> bytes) noexcept {
  if (bytes.size()>kBodyDisplayTextBytes) return "display_text_byte_budget";
  for (std::size_t i=0;i<bytes.size();) {
    const auto lead=std::to_integer<unsigned>(bytes[i++]);
    std::uint32_t value=lead,minimum=0;
    unsigned remaining=0;
    if (lead>=0xc2 && lead<=0xdf) {value=lead&0x1f;minimum=0x80;remaining=1;}
    else if (lead>=0xe0 && lead<=0xef) {value=lead&0xf;minimum=0x800;remaining=2;}
    else if (lead>=0xf0 && lead<=0xf4) {value=lead&7;minimum=0x10000;remaining=3;}
    else if (lead>0x7f) return "invalid_display_utf8";
    if (remaining>bytes.size()-i) return "truncated_display_utf8";
    while (remaining) {
      --remaining;
      const auto next=std::to_integer<unsigned>(bytes[i++]);
      if ((next&0xc0)!=0x80) return "invalid_display_utf8_continuation";
      value=(value<<6)|(next&0x3f);
    }
    if (value<minimum || !DisplayCodepoint(value)) return "display_codepoint_or_control";
  }
  return nullptr;
}
void CheckDisplayUtf8(const Field &field,std::span<const std::byte> bytes) {
  if(const auto *error=DisplayUtf8Error(bytes))Fail(field,error);
}
// 原生读取资格和终止符扫描由第10步owner负责；这里只走有界自有UTF-16片段。
std::uint32_t NextDisplayUtf16(std::span<const char16_t> text,std::size_t &position) {
  std::uint32_t value=text[position++];
  if (value>=0xd800 && value<=0xdbff) {
    if (position>=text.size()) Fail("display_utf16_missing_low_surrogate");
    const auto low=std::uint32_t(text[position++]);
    if (low<0xdc00 || low>0xdfff) Fail("display_utf16_invalid_low_surrogate");
    value=0x10000+((value-0xd800)<<10)+(low-0xdc00);
  }
  if (!DisplayCodepoint(value)) Fail("display_utf16_codepoint_or_control");
  return value;
}
unsigned Width(ScalarKind scalar) {
  switch (scalar) {
    case ScalarKind::U8: case ScalarKind::I8: return 1;
    case ScalarKind::U16: case ScalarKind::I16: return 2;
    case ScalarKind::U32: case ScalarKind::I32: case ScalarKind::F32: return 4;
    case ScalarKind::U64: case ScalarKind::I64: case ScalarKind::F64: return 8;
    default: Fail("non_scalar_plan");
  }
}
bool Signed(ScalarKind scalar) noexcept {
  return scalar==ScalarKind::I8 || scalar==ScalarKind::I16 || scalar==ScalarKind::I32 || scalar==ScalarKind::I64;
}
bool Floating(ScalarKind scalar) noexcept { return scalar==ScalarKind::F32 || scalar==ScalarKind::F64; }
// 有界小端读取不做未对齐强转，也不把u64先转double。
std::uint64_t Bits(std::span<const std::byte> bytes) {
  if (bytes.empty() || bytes.size()>8) Fail("scalar_width");
  std::uint64_t value=0;
  for (std::size_t i=0;i<bytes.size();++i) value|=std::uint64_t(std::to_integer<unsigned>(bytes[i]))<<(8*i);
  return value;
}
std::int64_t Sign(std::uint64_t value,unsigned width) noexcept {
  if (width<8 && (value&(std::uint64_t{1}<<(8*width-1)))) value|=(UINT64_MAX<<(8*width));
  return std::bit_cast<std::int64_t>(value);
}
double Real(std::uint64_t value,ScalarKind scalar) noexcept {
  return scalar==ScalarKind::F32 ? double(std::bit_cast<float>(std::uint32_t(value))) : std::bit_cast<double>(value);
}
void Put(std::span<std::byte> out,std::uint64_t value) {
  for (std::size_t i=0;i<out.size();++i) out[i]=std::byte(value>>(8*i));
}
bool Zero(std::span<const std::byte> bytes) {
  return std::all_of(bytes.begin(),bytes.end(),[](std::byte b){return b==std::byte{};});
}
// 可信数字计划上的纯值校验不依赖异常；采集热路径与正文共用完全相同的边界。
const char *ScalarError(const Field &field,const BodyFieldPlan &plan,std::span<const std::byte> encoded) noexcept {
  if(plan.scalar==ScalarKind::None)return "scalar_width";
  const auto width=Width(plan.scalar);
  if(encoded.size()%width)return "scalar_array_width";
  const auto &validator=kBodyValidators[plan.validator];
  for(std::size_t offset=0;offset<encoded.size();offset+=width){
    const auto value=Bits(encoded.subspan(offset,width));
    if(Floating(plan.scalar) && !std::isfinite(Real(value,plan.scalar)))return "nonfinite_scalar";
    if(validator.kind==Validation::Boolean && value>1)return "boolean_value";
    if(validator.kind==Validation::Range){
      const bool valid=Floating(plan.scalar)?Real(value,plan.scalar)>=Real(validator.minimum,plan.scalar) && Real(value,plan.scalar)<=Real(validator.maximum,plan.scalar):
        Signed(plan.scalar)?Sign(value,width)>=Sign(validator.minimum,width) && Sign(value,width)<=Sign(validator.maximum,width):
        value>=validator.minimum && value<=validator.maximum;
      if(!valid)return "scalar_range";
    }
    if(validator.kind==Validation::Enum &&
       std::find(kBodyEnumValues.begin()+validator.first,kBodyEnumValues.begin()+validator.first+validator.count,value)==
       kBodyEnumValues.begin()+validator.first+validator.count)return "scalar_enum";
    if(field.value_mask && (value&~std::uint64_t(field.value_mask)))return "undefined_value_mask_bits";
    if(plan.rule==ValueRule::NonNull && value==kNullIndex)return "required_nonnull_reference";
    if(plan.rule==ValueRule::NullableWater && value && (value>>32)!=1)return "nullable_water_encoding";
    if(plan.rule==ValueRule::EntityCounter && !ValidEntityCounter(value))return "entity_counter_mode";
    if(plan.rule==ValueRule::Countdown && !ValidEntityCountdown(value))return "countdown_mode";
  }
  return nullptr;
}
bool SameParents(const ProjectionFact &a,const ProjectionFact &b) {
  return a.depth==b.depth && std::equal(a.parents.begin(),a.parents.begin()+a.depth,b.parents.begin());
}
bool SameGroup(const ProjectionFact &a,const ProjectionFact &b) {
  return SameParents(a,b) && a.layout==b.layout && a.kind==b.kind && a.group==b.group;
}
bool FactLess(const ProjectionFact &a,const ProjectionFact &b) {
  const auto left=std::span(a.parents).first(a.depth),right=std::span(b.parents).first(b.depth);
  if (!std::equal(left.begin(),left.end(),right.begin(),right.end()))
    return std::lexicographical_compare(left.begin(),left.end(),right.begin(),right.end());
  return std::tie(a.layout,a.kind,a.group,a.slot,a.field)<std::tie(b.layout,b.kind,b.group,b.slot,b.field);
}
const Layout &LayoutFor(std::uint16_t id) {
  const auto *layout=FindLayout(id);
  if (!layout || layout->field_count>kBodyMaxFields) Fail("unknown_layout");
  if (std::find(kBodyDisabledLayouts.begin(),kBodyDisabledLayouts.end(),id)!=kBodyDisabledLayouts.end())
    Fail("disabled_media_body_layout");
  return *layout;
}
const Block &BlockFor(const Layout &layout,std::uint8_t id) {
  if (id>=layout.block_count) Fail("unknown_block");
  return Blocks()[layout.first_block+id];
}
std::uint32_t ValueMask(const Layout &layout) {
  std::uint32_t mask=0;
  for (std::uint8_t i=0;i<layout.block_count;++i) if (BlockFor(layout,i).max_size) mask|=std::uint32_t{1}<<i;
  return mask;
}

struct FieldSlice { std::span<const std::byte> bytes; bool seen=false; };
// 每次调用独立运行；源、context、scratch和字节区域仅借用到当前调用结束。
// 递归只使用生成大小的栈缓存；scratch用尽、深度／访问预算用尽时明确失败。
class BodyMachine {
public:
  BodyMachine(const BodyValueSource *source,std::span<std::byte> output,std::span<const std::byte> input,
              BodyCodecScratch &scratch,const BodyCodecContext *context,BodyCodecLimits limits)
      :source_(source),output_(output),input_(input),scratch_(scratch),context_(context),limits_(limits) {
    if (!limits.bytes || limits.bytes>(32u<<20) || !limits.field_visits || !limits.depth || limits.depth>kCodecMaxDepth)
      Fail("invalid_codec_limits");
    if (!limits.reliable && limits.bytes>65536) Fail("ordinary_message_protection");
    if (!source_ && input.size()>limits.bytes) Fail("body_byte_budget");
    scratch_.used=0;
    scratch_.root_field_count=0;
  }
  BodyView Run(std::uint16_t id,std::uint32_t mask,BodyStream stream) {
    const auto &layout=LayoutFor(id);
    if (stream!=BodyStream::World && stream!=BodyStream::Body && stream!=BodyStream::Value) Fail("invalid_body_stream");
    const auto allowed=ValueMask(layout);
    if ((mask&~allowed) || (stream!=BodyStream::Value && !mask)) Fail("unknown_reserved_or_empty_mask");
    BodyView result;
    result.layout=id;result.block_mask=mask;result.stream=stream;
    Walk(layout,mask,stream,&result);
    if (!source_ && position_!=input_.size()) Fail("trailing_body_bytes");
    result.bytes=Bytes().first(position_);
    result.facts=scratch_.facts.first(scratch_.used);
    result.fields=scratch_.root_fields.first(scratch_.root_field_count);
    result.needs_base_checks=ValidateProjectionFacts(scratch_.facts.first(scratch_.used),false);
    result.links_required=links_;
    result.field_visits=visits_;
    return result;
  }
private:
  std::span<const std::byte> Bytes() const { return source_ ? std::span<const std::byte>(output_) : input_; }
  void Visit() { if (visits_>=limits_.field_visits) throw BodyVisitError(std::uint64_t(visits_)+1,limits_.field_visits);++visits_; }
  void Capacity(std::uint64_t count) const {
    const auto size=source_?output_.size():input_.size();
    if (count>limits_.bytes-position_ || count>size-position_) Fail("truncated_or_body_byte_budget");
  }
  std::span<const std::byte> Raw(std::size_t count,std::span<const std::byte> source={}) {
    Capacity(count);
    const auto begin=position_;
    if (source_) {
      if (source.size()!=count) Fail("source_scalar_size");
      if (count) std::memmove(output_.data()+position_,source.data(),count);
    }
    position_+=count;
    return Bytes().subspan(begin,count);
  }
  std::uint64_t Number(unsigned width,std::uint64_t value=0) {
    if (source_) {
      std::array<std::byte,8> bytes{};
      Put(std::span(bytes).first(width),value);
      return Bits(Raw(width,std::span(bytes).first(width)));
    }
    return Bits(Raw(width));
  }
  void Zeros(std::size_t count) {
    Capacity(count);
    if (source_) std::fill_n(output_.data()+position_,count,std::byte{});
    else if (!Zero(input_.subspan(position_,count))) Fail("absent_shared_owner_not_zero");
    position_+=count;
  }
  CodecLocation Location(const Field &field,const BodyFieldPlan &plan) const {
    return {std::span(path_).first(depth_),field.layout,field.index,plan.canonical};
  }
  void Push(const Field &field,const BodyFieldPlan &plan,std::uint16_t element=kNullIndex,std::uint16_t choice=0) {
    if (depth_>=limits_.depth || depth_>=path_.size()) Fail("nested_depth_budget");
    node_path_[depth_]={field.layout,field.index,element,choice};
    path_[depth_++]={field.layout,plan.canonical,element,choice};
  }
  void Fact(const Field &field,const BodyFieldPlan &plan,ProjectionFactKind kind,std::uint16_t value) {
    if (kind==ProjectionFactKind::Count && plan.group==kNullIndex) return;
    if (scratch_.used>=scratch_.facts.size()) throw BodyScratchError(BodyScratchRegion::Facts,scratch_.used+1,std::uint32_t(scratch_.facts.size()));
    auto &fact=scratch_.facts[scratch_.used++];
    fact={};fact.depth=std::uint8_t(depth_);std::copy_n(path_.begin(),depth_,fact.parents.begin());
    fact.layout=field.layout;fact.field=field.index;fact.kind=kind;fact.value=value;
    if (kind==ProjectionFactKind::Count) {
      fact.group=plan.group;fact.slot=plan.slot;fact.limit=plan.count_limit;fact.copies=plan.copies;fact.slots=plan.slots;
    } else {
      fact.group=plan.canonical;fact.slots=1;fact.limit=1;
      const auto &layout=LayoutFor(field.layout);
      for (std::uint16_t i=0;i<layout.field_count;++i) {
        const auto &other=kBodyFieldPlans[layout.first_field+i];
        if (other.storage==BodyStorage::SharedOwner && other.canonical==plan.canonical && Fields()[layout.first_field+i].max_size)
          ++fact.copies;
      }
    }
  }
  std::optional<bool> PriorPresence(const Field &field,const BodyFieldPlan &plan) const {
    for (std::uint32_t i=scratch_.used;i>0;--i) {
      const auto &fact=scratch_.facts[i-1];
      if (fact.kind==ProjectionFactKind::SharedPresence && fact.layout==field.layout &&
          fact.group==plan.canonical && fact.depth==depth_ &&
          std::equal(path_.begin(),path_.begin()+depth_,fact.parents.begin())) return fact.value!=0;
    }
    return context_?context_->SharedPresence(Location(field,plan)):std::nullopt;
  }
  // 条件只消费当前实例先行整数；704分支必须借用已核配置身份，依赖缺失不能当false。
  bool Condition(const Field &field,const BodyFieldPlan &plan,const std::array<FieldSlice,kBodyMaxFields> &values) {
    if (plan.predicate==PredicateKind::None) return true;
    if (plan.selector>=values.size() || !values[plan.selector].seen) Fail(field,"missing_condition_selector");
    const auto value=Bits(values[plan.selector].bytes);
    switch (plan.predicate) {
      case PredicateKind::Equal: return value==plan.operand;
      case PredicateKind::NotEqual: return value!=plan.operand;
      case PredicateKind::AnyBits: return (value&plan.operand)!=0;
      case PredicateKind::OneOf: return value<64 && (plan.choices&(std::uint64_t{1}<<value));
      case PredicateKind::ItemConfig: {
        const auto &layout=LayoutFor(field.layout);
        const auto table=kBodyFieldPlans[layout.first_field+plan.selector].table;
        if (!context_ || value>=kNullIndex) throw BodyDependencyError(BodyDependency::ItemConfig,field.layout,field.index);
        const auto config=context_->Item(table,std::uint16_t(value));
        if (!config) throw BodyDependencyError(BodyDependency::ItemConfig,field.layout,field.index);
        const bool selected=config->id==plan.operand && config->type<64 && (plan.choices&(std::uint64_t{1}<<config->type));
        if (selected && !config->cooldown) Fail(field,"zero_collectible_704_divisor");
        return selected;
      }
      default: Fail(field,"unregistered_condition");
    }
  }
  // 保留原位模式检查；整数按真实符号／宽度比较，避免u64经浮点转换丢失边界。
  void CheckScalar(const Field &field,const BodyFieldPlan &plan,std::span<const std::byte> encoded) {
    if(const auto *error=ScalarError(field,plan,encoded))Fail(field,error);
    const auto width=Width(plan.scalar);
    for (std::size_t offset=0;offset<encoded.size();offset+=width) {
      const auto value=Bits(encoded.subspan(offset,width));
      if ((field.operation==Operation::Reference || field.operation==Operation::Resource ||
           (field.operation==Operation::Container && field.target && field.target[0])) && value!=kNullIndex) links_=true;
    }
  }
  // 观察器只借用已验证片段；真实路径保留字段／元素身份，不能泄漏canonical投影别名。
  void ObserveScalars(const Field &field,const BodyFieldPlan &plan,std::span<const std::byte> value,std::uint32_t count) {
    if (!scratch_.observer) return;
    if (depth_>=node_path_.size()) Fail(field,"node_depth_budget");
    const bool container=field.operation==Operation::Container;
    const auto operation=container && field.target && field.target[0] ?
      (plan.table==255?Operation::Reference:Operation::Resource):field.operation;
    const auto stride=std::size_t(plan.components)*Width(plan.scalar);
    for (std::uint32_t i=0;i<count;++i) {
      node_path_[depth_]={field.layout,field.index,container?std::uint16_t(i):kNullIndex,0};
      if(operation==Operation::Reference || operation==Operation::Resource){
        // 固定关系数组逐引用观察闭包，保留字段路径；每个节点仍恰好一个u16。
        for(std::uint16_t component=0;component<plan.components;++component)
          scratch_.observer->Node({std::span(node_path_).first(depth_+1),0,operation,plan.table,
            value.subspan(std::size_t(i)*stride+std::size_t(component)*2,2),component});
        continue;
      }
      scratch_.observer->Node({std::span(node_path_).first(depth_+1),0,operation,plan.table,
                              value.subspan(std::size_t(i)*stride,stride)});
    }
  }
  void Scalars(const Field &field,const BodyFieldPlan &plan,std::uint32_t count) {
    if (count>limits_.field_visits-visits_) throw BodyVisitError(std::uint64_t(visits_)+count,limits_.field_visits);
    visits_+=count;
    const auto elements=std::uint64_t(count)*plan.components;
    const auto size=elements*Width(plan.scalar);
    Capacity(size);
    if (elements>UINT32_MAX) Fail(field,"scalar_element_overflow");
    const auto value=source_?Raw(std::size_t(size),source_->Scalar(Location(field,plan),plan.scalar,std::uint32_t(elements))):Raw(std::size_t(size));
    CheckScalar(field,plan,value);
    if (plan.rule==ValueRule::DisplayUtf8) CheckDisplayUtf8(field,value);
    if (plan.storage==BodyStorage::CString && !ZeroFree(value)) Fail(field,"cstring_embedded_nul");
    if (plan.storage==BodyStorage::Set) {
      const auto width=Width(plan.scalar);
      for (std::uint32_t i=1;i<count;++i)
        if (!Less(Bits(value.subspan(std::size_t(i-1)*width,width)),Bits(value.subspan(std::size_t(i)*width,width)),plan.scalar))
          Fail(field,"set_not_strictly_ordered");
    }
    ObserveScalars(field,plan,value,count);
  }
  static bool ZeroFree(std::span<const std::byte> bytes) {
    return std::none_of(bytes.begin(),bytes.end(),[](std::byte b){return b==std::byte{};});
  }
  static bool Less(std::uint64_t a,std::uint64_t b,ScalarKind scalar) {
    return Signed(scalar)?Sign(a,Width(scalar))<Sign(b,Width(scalar)):a<b;
  }
  void Child(const Field &field,const BodyFieldPlan &plan,std::uint16_t layout_id,
             std::uint16_t element=kNullIndex,std::uint16_t choice=0) {
    const auto &layout=LayoutFor(layout_id);
    const auto mask=field.nested_block==255?ValueMask(layout):(BlockFor(layout,field.nested_block).max_size ? std::uint32_t{1}<<field.nested_block : 0u);
    Push(field,plan,element,choice);
    Walk(layout,mask,BodyStream::Value,nullptr);
    --depth_;
  }
  void Nested(const Field &field,const BodyFieldPlan &plan) {
    if (plan.storage==BodyStorage::SharedOwner) {
      const bool header=field.nested_block==0 || field.nested_block==255;
      const auto begin=position_;
      bool present=false;
      if (source_) {
        present=source_->SharedPresent(Location(field,plan));
        if (header) Number(1,present?1:0);
      } else if (header) {
        const auto marker=Number(1);
        if (marker>1) Fail(field,"shared_presence_boolean");
        present=marker!=0;
      } else {
        const auto known=PriorPresence(field,plan);
        if (!known) throw BodyDependencyError(BodyDependency::SharedPresence,field.layout,field.index);
        present=*known;
      }
      Fact(field,plan,ProjectionFactKind::SharedPresence,present?1:0);
      if (!present) {
        const auto consumed=position_-begin;
        if (field.min_size<consumed) Fail(field,"shared_zero_plan");
        Zeros(field.min_size-consumed);
        return;
      }
    }
    Child(field,plan,field.nested_layout);
  }
  // 先按宽计数检查再窄化；固定槽不询问动态count、不添加长度头，也不擅自排序有序结果。
  void Container(const Field &field,const BodyFieldPlan &plan) {
    const auto count=field.fixed_count ? field.fixed_count :
      (source_?source_->Count(Location(field,plan)):std::uint32_t(Number(field.count_bytes)));
    if (count>field.limit || count>UINT16_MAX || (field.count_bytes==1 && count>UINT8_MAX)) Fail(field,"container_count");
    if (source_ && !field.fixed_count) Number(field.count_bytes,count);
    Fact(field,plan,ProjectionFactKind::Count,std::uint16_t(count));
    if (!field.nested_layout) { Scalars(field,plan,count);return; }
    bool previous=false;
    std::uint64_t last=0;
    for (std::uint32_t i=0;i<count;++i) {
      const auto begin=position_;
      Child(field,plan,field.nested_layout,std::uint16_t(i));
      if (plan.storage==BodyStorage::OrderedMap) {
        const auto &layout=LayoutFor(field.nested_layout);
        const auto &key=kBodyFieldPlans[layout.first_field];
        if (key.scalar==ScalarKind::None || key.components!=1 || Floating(key.scalar)) Fail(field,"map_key_plan");
        const auto value=Bits(Bytes().subspan(begin,Width(key.scalar)));
        if (previous && !Less(last,value,key.scalar)) Fail(field,"map_not_strictly_ordered");
        previous=true;last=value;
      }
    }
  }
  // 仅允许生成的Weapon视图集合；存在位、槽序、布局号及实际type分别校验。
  void Choice(const Field &field,const BodyFieldPlan &plan) {
    const auto selected=source_?source_->SlotMask(Location(field,plan)):std::uint32_t(Number(1));
    if (selected&~((1u<<field.limit)-1u)) Fail(field,"slot_mask_bits");
    if (source_) Number(1,selected);
    for (std::uint8_t slot=0;slot<field.limit;++slot) if (selected&(1u<<slot)) {
      const auto id=source_?source_->SlotLayout(Location(field,plan),slot):std::uint16_t(Number(2));
      bool allowed=false;
      for (const auto &choice:LayoutChoices()) if (choice.layout==field.layout && choice.field==field.index && choice.choice==id) {allowed=true;break;}
      if (!allowed) Fail(field,"unregistered_slot_layout");
      if (source_) Number(2,id);
      Child(field,plan,id,slot,id);
    }
  }
  void LocalRules(const Layout &layout,const std::array<FieldSlice,kBodyMaxFields> &values) {
    for (const auto &rule:kBodyLayoutRules) if (rule.layout==layout.id && values[rule.selector].seen) {
      const auto value=Bits(values[rule.selector].bytes);
      if (rule.kind==LayoutRuleKind::EmptyActiveZero && !value) {
        for (std::uint16_t i=0;i<layout.field_count;++i) if (i!=rule.selector && values[i].seen && !Zero(values[i].bytes)) Fail("empty_active_noncanonical");
      }
      if (rule.kind==LayoutRuleKind::NullCostumeLayerZero && value==kNullIndex && values[rule.other].seen && !Zero(values[rule.other].bytes))
        Fail("null_costume_layer_nonzero");
    }
  }
  // 空主动槽与Null costume映射由已核规范直接写零，不向源询问旧负载。
  bool CanonicalZero(const Layout &layout,std::uint16_t field,const std::array<FieldSlice,kBodyMaxFields> &values) {
    if (!source_) return false;
    for (const auto &rule:kBodyLayoutRules) if (rule.layout==layout.id && values[rule.selector].seen) {
      const auto selector=Bits(values[rule.selector].bytes);
      if (rule.kind==LayoutRuleKind::EmptyActiveZero && !selector && field!=rule.selector) return true;
      if (rule.kind==LayoutRuleKind::NullCostumeLayerZero && selector==kNullIndex && field==rule.other) return true;
    }
    return false;
  }
  void RootField(const Field &field,bool present,std::span<const std::byte> bytes) {
    if (scratch_.root_fields.empty()) return;
    if (scratch_.root_field_count>=scratch_.root_fields.size()) throw BodyScratchError(BodyScratchRegion::RootFields,scratch_.root_field_count+1,std::uint32_t(scratch_.root_fields.size()));
    scratch_.root_fields[scratch_.root_field_count++]={field.index,field.block,present,bytes};
  }
  // 观察器只看已校验的本地边界；字段尾回调仍不授权任何原生副作用。
  void ObserveField(const Field &field,const BodyFieldPlan &plan,bool present,std::span<const std::byte> bytes) {
    if(!scratch_.observer)return;
    if(depth_>=node_path_.size())Fail(field,"field_node_depth_budget");
    node_path_[depth_]={field.layout,field.index,kNullIndex,0};
    BodyFieldNodeView value{std::span(node_path_).first(depth_+1),bytes,present?1u:0u,0,present,present};
    if(present&&field.operation==Operation::Container)
      value.elements=field.fixed_count?field.fixed_count:std::uint32_t(Bits(bytes.first(field.count_bytes)));
    if(present&&field.operation==Operation::Choice){value.slot_mask=std::uint32_t(Bits(bytes.first(1)));value.elements=std::popcount(value.slot_mask);}
    if(present&&plan.storage==BodyStorage::SharedOwner){
      const auto known=PriorPresence(field,plan);if(!known)throw BodyDependencyError(BodyDependency::SharedPresence,field.layout,field.index);
      value.payload_present=*known;value.elements=*known?1u:0u;
    }
    scratch_.observer->Field(value);
  }
  // 同一递归实例独占selector缓存；块号、条件和长度均由可信计划决定，失败不交付视图。
  void Walk(const Layout &layout,std::uint32_t mask,BodyStream stream,BodyView *root) {
    Visit();
    std::array<FieldSlice,kBodyMaxFields> values{};
    const auto begin=position_;
    for (std::uint8_t block=0;block<layout.block_count;++block) if (mask&(std::uint32_t{1}<<block)) {
      const auto &b=BlockFor(layout,block);
      if (stream!=BodyStream::Value && b.body!=(stream==BodyStream::Body)) Fail("block_stream_mismatch");
      const auto block_begin=position_;
      for (std::uint16_t i=0;i<layout.field_count;++i) {
        const auto &field=Fields()[layout.first_field+i];
        if (field.block!=block) continue;
        Visit();
        const auto &plan=kBodyFieldPlans[layout.first_field+i];
        if (!Condition(field,plan,values)) {
          if (root) RootField(field,false,{});
          ObserveField(field,plan,false,{});
          continue;
        }
        const auto field_begin=position_;
        if (CanonicalZero(layout,i,values)) {
          if (field.min_size!=field.max_size || plan.scalar==ScalarKind::None) Fail(field,"canonical_zero_plan");
          Zeros(field.max_size);
          CheckScalar(field,plan,Bytes().subspan(field_begin,field.max_size));
          ObserveScalars(field,plan,Bytes().subspan(field_begin,field.max_size),1);
        } else switch (field.operation) {
          case Operation::Container: Container(field,plan);break;
          case Operation::Choice: Choice(field,plan);break;
          case Operation::Nested: Nested(field,plan);break;
          default:
            if (field.nested_layout) Child(field,plan,field.nested_layout);
            else Scalars(field,plan,1);
            break;
        }
        const auto size=position_-field_begin;
        if (size<field.min_size || size>field.max_size) Fail(field,"field_byte_budget");
        values[i]={Bytes().subspan(field_begin,size),true};
        if (root) RootField(field,true,values[i].bytes);
        ObserveField(field,plan,true,values[i].bytes);
      }
      const auto size=position_-block_begin;
      if (size<b.min_size || size>b.max_size) Fail("block_byte_budget");
      if (root) root->blocks[root->block_count++]={block,Bytes().subspan(block_begin,size)};
    }
    LocalRules(layout,values);
    for (const auto &budget:LayoutBudgets()) if (budget.layout==layout.id) {
      const auto bytes=position_-begin;
      if (bytes>budget.record_bytes || (!limits_.reliable && stream==BodyStream::Body && bytes>budget.body_bytes)) Fail("layout_record_budget");
      break;
    }
    if (scratch_.observer)
      scratch_.observer->Node({std::span(node_path_).first(depth_),layout.id,Operation::Nested,255,
                              Bytes().subspan(begin,position_-begin)});
  }
  const BodyValueSource *source_;
  std::span<std::byte> output_;
  std::span<const std::byte> input_;
  BodyCodecScratch &scratch_;
  const BodyCodecContext *context_;
  BodyCodecLimits limits_;
  std::array<CodecPathStep,kCodecMaxDepth> path_{};
  std::array<CodecPathStep,kCodecMaxDepth> node_path_{};
  std::size_t position_=0,depth_=0;
  std::uint32_t visits_=0;
  bool links_=false;
};
} // namespace

std::span<const std::byte> TranscodeDisplayTextUtf16Into(
    std::span<const char16_t> text,std::span<std::byte> output) {
  if (text.size()>kBodyDisplayTextBytes) Fail("display_utf16_unit_budget");
  std::size_t required=0;
  // 先检查全部文字和容量；失败不返回被截断或替换的半个文字结果。
  for (std::size_t position=0;position<text.size();) {
    const auto value=NextDisplayUtf16(text,position);
    required+=value<0x80?1:value<0x800?2:value<0x10000?3:4;
    if (required>kBodyDisplayTextBytes || required>output.size()) Fail("display_utf8_output_budget");
  }
  if (!text.empty() && required) {
    const auto source_address=reinterpret_cast<std::uintptr_t>(text.data());
    const auto output_address=reinterpret_cast<std::uintptr_t>(output.data());
    if (source_address<=output_address ? output_address-source_address<text.size_bytes() :
        source_address-output_address<required) Fail("display_transcode_overlapping_buffers");
  }
  std::size_t written=0;
  for (std::size_t position=0;position<text.size();) {
    const auto value=NextDisplayUtf16(text,position);
    const auto emit=[&](std::uint32_t byte) {output[written++]=std::byte(byte);};
    if (value<0x80) emit(value);
    else if (value<0x800) {emit(0xc0|(value>>6));emit(0x80|(value&0x3f));}
    else if (value<0x10000) {emit(0xe0|(value>>12));emit(0x80|((value>>6)&0x3f));emit(0x80|(value&0x3f));}
    else {emit(0xf0|(value>>18));emit(0x80|((value>>12)&0x3f));emit(0x80|((value>>6)&0x3f));emit(0x80|(value&0x3f));}
  }
  return std::span<const std::byte>(output).first(written);
}

bool ValidateProjectionFacts(std::span<ProjectionFact> facts,bool require_complete) {
  for (const auto &fact:facts) if (fact.depth>kCodecMaxDepth || !fact.copies || !fact.slots || fact.slot>=fact.slots ||
      (fact.kind!=ProjectionFactKind::Count && fact.kind!=ProjectionFactKind::SharedPresence)) Fail("invalid_projection_fact");
  std::sort(facts.begin(),facts.end(),FactLess);
  bool missing=false;
  for (std::size_t group_begin=0;group_begin<facts.size();) {
    const auto &first=facts[group_begin];
    auto group_end=group_begin+1;
    while (group_end<facts.size() && SameGroup(first,facts[group_end])) ++group_end;
    std::uint32_t sum=0,slots=0;
    for (auto slot_begin=group_begin;slot_begin<group_end;) {
      const auto &value=facts[slot_begin];
      auto slot_end=slot_begin+1;
      while (slot_end<group_end && facts[slot_end].slot==value.slot) ++slot_end;
      for (auto i=slot_begin+1;i<slot_end;++i) {
        const auto &other=facts[i];
        if (other.value!=value.value || other.limit!=value.limit || other.copies!=value.copies || other.slots!=value.slots)
          Fail("projection_value_conflict");
        if (other.field==facts[i-1].field) Fail("duplicate_projection_fact");
      }
      if (slot_end-slot_begin>value.copies || value.value>value.limit) Fail("projection_count_bounds");
      missing|=slot_end-slot_begin<value.copies;
      sum+=value.value;++slots;
      slot_begin=slot_end;
    }
    if (slots>first.slots || (first.kind==ProjectionFactKind::Count && sum>first.limit)) Fail("container_count_group_sum");
    missing|=slots<first.slots;
    group_begin=group_end;
  }
  if (require_complete && missing) Fail("missing_base_projection");
  return missing;
}

bool DescribeBodySourceField(std::uint16_t id,std::uint16_t index,BodySourceFieldInfo &out) noexcept {
  out={};const auto *layout=FindLayout(id);
  if(!layout || index>=layout->field_count ||
     std::find(kBodyDisabledLayouts.begin(),kBodyDisabledLayouts.end(),id)!=kBodyDisabledLayouts.end())return false;
  const auto &plan=kBodyFieldPlans[layout->first_field+index];
  const auto &field=Fields()[layout->first_field+index];
  out={plan.scalar,plan.components,plan.canonical,plan.table,plan.predicate!=PredicateKind::None,false,
       plan.storage==BodyStorage::SharedOwner,plan.storage==BodyStorage::SlotsMask,
       field.operation==Operation::Reference || field.operation==Operation::Resource ||
       (field.operation==Operation::Container && field.target && field.target[0])};
  out.storage=plan.storage;out.predicate=plan.predicate;out.selector=plan.selector;
  out.operand=plan.operand;out.choices=plan.choices;out.nonnull=plan.rule==ValueRule::NonNull;
  for(const auto &rule:kBodyLayoutRules)if(rule.layout==id &&
      ((rule.kind==LayoutRuleKind::EmptyActiveZero && index!=rule.selector) ||
       (rule.kind==LayoutRuleKind::NullCostumeLayerZero && index==rule.other))){
    out.canonical_zero=true;out.zero_selector=rule.selector;
    out.zero_rule=rule.kind==LayoutRuleKind::EmptyActiveZero?BodyZeroRule::IfZero:BodyZeroRule::IfNullIndex;
  }
  return true;
}
std::uint32_t BodySourceMaximumFields() noexcept {return std::uint32_t(kBodyMaxFields);}
bool ValidBodySourceScalars(std::uint16_t id,std::uint16_t index,std::span<const std::byte> bytes) noexcept {
  const auto *layout=FindLayout(id);BodySourceFieldInfo info;
  if(!layout || !DescribeBodySourceField(id,index,info) || info.linked)return false;
  const auto &plan=kBodyFieldPlans[layout->first_field+index];const auto &field=Fields()[layout->first_field+index];
  if(ScalarError(field,plan,bytes))return false;
  if(plan.rule==ValueRule::DisplayUtf8 && DisplayUtf8Error(bytes))return false;
  if(plan.storage==BodyStorage::CString &&
     std::any_of(bytes.begin(),bytes.end(),[](std::byte b){return b==std::byte{};}))return false;
  if(plan.storage==BodyStorage::Set){
    const auto width=Width(plan.scalar);
    for(std::size_t i=width;i<bytes.size();i+=width){const auto a=Bits(bytes.subspan(i-width,width)),b=Bits(bytes.subspan(i,width));
      if(Signed(plan.scalar)?Sign(a,width)>=Sign(b,width):a>=b)return false;
    }
  }
  return true;
}
BodyView EncodeBodyInto(std::uint16_t layout,std::uint32_t mask,BodyStream stream,const BodyValueSource &source,
    std::span<std::byte> out,BodyCodecScratch &scratch,const BodyCodecContext *context,BodyCodecLimits limits) {
  return BodyMachine(&source,out,{},scratch,context,limits).Run(layout,mask,stream);
}
BodyView DecodeBody(std::uint16_t layout,std::uint32_t mask,BodyStream stream,std::span<const std::byte> bytes,
    BodyCodecScratch &scratch,const BodyCodecContext *context,BodyCodecLimits limits) {
  return BodyMachine(nullptr,{},bytes,scratch,context,limits).Run(layout,mask,stream);
}
BodyView EncodeValueInto(std::uint16_t id,const BodyValueSource &source,std::span<std::byte> out,
    BodyCodecScratch &scratch,const BodyCodecContext *context,BodyCodecLimits limits) {
  return EncodeBodyInto(id,ValueMask(LayoutFor(id)),BodyStream::Value,source,out,scratch,context,limits);
}
BodyView DecodeValue(std::uint16_t id,std::span<const std::byte> bytes,BodyCodecScratch &scratch,
    const BodyCodecContext *context,BodyCodecLimits limits) {
  return DecodeBody(id,ValueMask(LayoutFor(id)),BodyStream::Value,bytes,scratch,context,limits);
}
BodyRecordView EncodeRecordInto(std::uint16_t member,std::uint16_t layout,std::uint32_t mask,BodyStream stream,
    const BodyValueSource &source,std::span<std::byte> out,BodyCodecScratch &scratch,
    const BodyCodecContext *context,BodyCodecLimits limits) {
  if (member==kNullIndex || stream==BodyStream::Value || out.size()<12 || limits.bytes<=12 || limits.bytes>(32u<<20) ||
      (!limits.reliable && limits.bytes>65536)) Fail("record_header");
  limits.bytes-=12;
  auto body=EncodeBodyInto(layout,mask,stream,source,out.subspan(12),scratch,context,limits);
  Put(out.first(2),member);Put(out.subspan(2,2),layout);Put(out.subspan(4,4),mask);Put(out.subspan(8,4),body.bytes.size());
  return {member,body,std::uint32_t(12+body.bytes.size())};
}
BodyRecordView DecodeRecordPrefix(std::uint16_t expected_member,std::uint16_t expected_layout,BodyStream stream,
    std::span<const std::byte> bytes,BodyCodecScratch &scratch,const BodyCodecContext *context,BodyCodecLimits limits) {
  if (expected_member==kNullIndex || stream==BodyStream::Value || bytes.size()<12 || limits.bytes<=12 || limits.bytes>(32u<<20) ||
      (!limits.reliable && limits.bytes>65536)) Fail("record_header");
  const auto member=std::uint16_t(Bits(bytes.first(2))),layout=std::uint16_t(Bits(bytes.subspan(2,2)));
  const auto mask=std::uint32_t(Bits(bytes.subspan(4,4))),size=std::uint32_t(Bits(bytes.subspan(8,4)));
  if (member!=expected_member || layout!=expected_layout) Fail("record_member_layout_mismatch");
  if (size>bytes.size()-12 || size>limits.bytes-12) Fail("record_length");
  limits.bytes-=12;
  auto body=DecodeBody(layout,mask,stream,bytes.subspan(12,size),scratch,context,limits);
  return {member,body,12+size};
}
} // namespace cs::state
