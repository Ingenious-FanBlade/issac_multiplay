#include "state/client_flat.h"
#include "state/client_shadow.h"
#include "state/flat_encode.h"
#include "state/flat_simd.h"
#include "state/entity_counters.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <tuple>

namespace cs::state {
namespace {
constexpr std::size_t kFlatRecordLimit=2u<<20;
using ByteView=std::span<const std::byte>;
std::uint64_t Word(ByteView bytes) noexcept {
  std::uint64_t value=0;
  std::memcpy(&value,bytes.data(),bytes.size());
  return value;
}
bool Bit(std::span<const std::uint32_t> bits,unsigned n) noexcept {return (bits[n/32]&(1u<<(n%32)))!=0;}
void Set(std::span<std::uint32_t> bits,unsigned n) noexcept {bits[n/32]|=1u<<(n%32);}
bool Zero(ByteView bytes) noexcept {
  return std::all_of(bytes.begin(),bytes.end(),[](auto b){return b==std::byte{};});
}
template<class T> T Load(ByteView bytes) noexcept {T value;std::memcpy(&value,bytes.data(),sizeof(value));return value;}
double Real(std::uint64_t bits,ScalarKind scalar) noexcept {
  return scalar==ScalarKind::F32?double(std::bit_cast<float>(std::uint32_t(bits))):std::bit_cast<double>(bits);
}
std::int64_t Signed(std::uint64_t value,unsigned width) noexcept {
  const auto shift=64-width*8;
  return std::bit_cast<std::int64_t>(value<<shift)>>shift;
}

bool ValidScalar(const FlatField &field,ByteView value) noexcept {
  const auto atom=field.scalar;
  const auto width=CaptureScalarWidth(atom);
  if(!width||value.size()!=std::size_t(width)*field.components)return false;
  const bool floating=atom==ScalarKind::F32||atom==ScalarKind::F64;
  const bool signed_value=atom==ScalarKind::I8||atom==ScalarKind::I16||atom==ScalarKind::I32||atom==ScalarKind::I64;
  const auto &check=field.validation;
  for(std::size_t i=0;i<value.size();i+=width){
    const auto bits=Word(value.subspan(i,width));
    if(floating&&!std::isfinite(Real(bits,atom)))return false;
    if(check.kind==Validation::Boolean&&bits>1)return false;
    if(check.kind==Validation::Enum&&std::find(check.enum_values.begin(),check.enum_values.end(),bits)==check.enum_values.end())return false;
    if(check.kind==Validation::Range){
      if(floating){
        const auto real=Real(bits,atom);
        if(real<Real(check.minimum,atom)||real>Real(check.maximum,atom))return false;
      }else if(signed_value){
        if(Signed(bits,width)<Signed(check.minimum,width)||Signed(bits,width)>Signed(check.maximum,width))return false;
      }else if(bits<check.minimum||bits>check.maximum)return false;
    }
    if(field.value_rule==FlatValueRule::NullableWater&&bits&&(bits>>32)!=1)return false;
    if(field.value_rule==FlatValueRule::EntityCounter&&!ValidEntityCounter(bits))return false;
    if(field.value_rule==FlatValueRule::Countdown&&!ValidEntityCountdown(bits))return false;
  }
  return true;
}

// 线上有界游标。定长镜像和附加字节始终通过生成表解释。
class FlatReader {
public:
  FlatReader(const StateRecordPayload &record,BodyStream stream,const ClientMemberSnapshotRef &members,
             const ClientResourceSnapshotRef &resources,std::uint32_t &remaining)
    :record_(record),members_(members),resources_(resources),remaining_(remaining),stream_(unsigned(stream)) {}

  [[noreturn]] void Bad(const char *reason,std::uint16_t field=kNullIndex,std::uint16_t layout=0,
                        StateReadiness readiness=StateReadiness::InvalidEncoding) const {
    throw ClientShadowError(readiness,reason,record_.member,layout?layout:record_.layout,field);
  }
  ByteView Take(ByteView &from,std::size_t size,std::uint16_t field=kNullIndex) const {
    if(size>from.size())Bad("flat_record_truncated",field);
    const auto head=from.first(size);
    from=from.subspan(size);
    return head;
  }
  void Visit(std::size_t count) {
    if(count>remaining_)Bad("flat_visit_budget",kNullIndex,0,StateReadiness::Budget);
    remaining_-=std::uint32_t(count);
  }

  unsigned Leb(ByteView &wire) const {
    unsigned value=0;
    for(unsigned i=0;i<3;++i){
      const auto byte=std::to_integer<unsigned>(Take(wire,1)[0]);
      value|=(byte&127u)<<(7*i);
      if(!(byte&128u)){
        if((i&&!(byte&127u))||value>UINT16_MAX)Bad("flat_field_set_leb");
        return value;
      }
    }
    Bad("flat_field_set_leb");
  }

  void Select(const FlatLayout &layout,const StateSectionView &section,ByteView &wire,std::span<std::uint32_t> selected) const {
    const auto flow=unsigned(section.context.stream);
    if(record_.mask||record_.layout!=layout.id||flow>1||
       (!section.context.base_version&&record_.set_mode!=FlatSetMode::All))Bad("flat_record_mode_or_layout");
    const auto stream=layout.stream_fields[flow];
    const auto add=[&](unsigned field){
      if(field>=layout.fields.size()||!Bit(stream,field))Bad("flat_field_set_stream_or_range",std::uint16_t(field));
      Set(selected,field);
    };
    if(record_.set_mode==FlatSetMode::All){
      std::copy(stream.begin(),stream.end(),selected.begin());
    }else if(record_.set_mode==FlatSetMode::Deltas){
      const auto count=Leb(wire);
      if(!count||count>layout.fields.size())Bad("flat_field_set_count");
      unsigned next=0;
      for(unsigned i=0;i<count;++i){const auto field=next+Leb(wire);add(field);next=field+1;}
    }else if(record_.set_mode==FlatSetMode::Bitmap){
      const auto groups=(layout.fields.size()+15)/16;
      const auto top=Take(wire,(groups+7)/8);
      bool any=false;
      for(std::size_t i=0;i<top.size()*8;++i)if(std::to_integer<unsigned>(top[i/8])&(1u<<(i%8))){
        if(i>=groups)Bad("flat_field_set_group_range");
        unsigned bits=Load<std::uint16_t>(Take(wire,2));
        if(!bits)Bad("flat_field_set_empty_group");
        any=true;
        while(bits){add(unsigned(i*16+std::countr_zero(bits)));bits&=bits-1;}
      }
      if(!any)Bad("flat_field_set_empty");
    }else Bad("flat_record_set_mode");
  }

  // 先从线上或前代切出每个字段的整段，再一次分配完整目标。owner 子树没有独立长度字。
  void Advance(const FlatPatch &patch,ByteView slot,ByteView &cursor,unsigned depth) {
    if(depth>=kFlatMaxDepth)Bad("flat_tail_depth",patch.field);
    if(patch.header==FlatHeader::Weapon){
      std::array<ByteView,5> children{};
      const auto ids=Weapon(patch,slot,cursor,children);
      for(unsigned i=0;i<ids.size();++i)if(ids[i]){
        Visit(1);
        for(const auto &nested:ids[i]->patches)
          if(nested.kind==FlatPatchKind::Container||nested.kind==FlatPatchKind::Owner)
            Advance(nested,children[i].subspan(nested.mirror_offset,nested.size),cursor,depth+1);
      }
      return;
    }
    if(patch.header==FlatHeader::Owner){
      const auto tag=FlatWord(slot);
      if(!tag)return;
      if(tag!=(kFlatOwnerPresent|patch.child_layout))Bad("flat_owner_layout",patch.field);
      AdvanceChildren(patch.child_layout,1,cursor,depth);
      return;
    }
    const auto [count,size]=FlatContainer(slot,patch.header);
    switch(patch.header){
      case FlatHeader::String:
        if(FlatWord(slot,20)||!Zero(slot.subspan(count<=15?count:0,16-(count<=15?count:0))))
          Bad("flat_string_template",patch.field);
        break;
      case FlatHeader::Deque:
        if(FlatWord(slot)||FlatWord(slot,8)||FlatWord(slot,12))Bad("flat_deque_template",patch.field);
        break;
      case FlatHeader::Vector:case FlatHeader::Unordered:case FlatHeader::InlinePrefix:
        if(!Zero(slot.subspan(8)))Bad("flat_container_template",patch.field);
        break;
      case FlatHeader::Tree:case FlatHeader::LayerArray:case FlatHeader::Array:
      case FlatHeader::Aggregate:case FlatHeader::CString:break;
      default:Bad("flat_tail_header",patch.field);
    }
    if(count>patch.limit||(patch.max_bytes&&size>patch.max_bytes))Bad("flat_tail_limit",patch.field);
    auto child=Take(cursor,size,patch.field);
    if(patch.header==FlatHeader::CString&&std::find(child.begin(),child.end(),std::byte{})!=child.end())
      Bad("flat_cstring_embedded_nul",patch.field);
    if(patch.header==FlatHeader::Unordered){
      if(patch.stride!=4||std::uint64_t(count)*4!=size)Bad("flat_unordered_size",patch.field);
      for(unsigned i=1;i<count;++i)
        if(Load<std::int32_t>(child.subspan((i-1)*4,4))>=Load<std::int32_t>(child.subspan(i*4,4)))
          Bad("flat_unordered_key_order",patch.field);
    }
    if(patch.child_layout){
      AdvanceChildren(patch.child_layout,count,child,depth);
      if(!child.empty())Bad("flat_child_tail_length",patch.field);
    }else if(patch.header!=FlatHeader::String&&std::uint64_t(count)*patch.stride!=size)
      Bad("flat_tail_length",patch.field);
  }

  std::array<const FlatLayout *,5> Weapon(const FlatPatch &patch,ByteView slot,ByteView &tail,
                                         std::array<ByteView,5> &mains) const {
    if(slot.size()!=20)Bad("flat_weapon_width",patch.field);
    std::array<const FlatLayout *,5> layouts{};
    for(unsigned i=0;i<layouts.size();++i)if(const auto word=FlatWord(slot,i*4)){
      const auto id=std::uint16_t(word);
      const auto *child=FindFlatLayout(id);
      if(word!=(kFlatOwnerPresent|id)||!child||std::find(patch.choices.begin(),patch.choices.end(),id)==patch.choices.end())
        Bad("flat_weapon_layout",patch.field);
      layouts[i]=child;mains[i]=Take(tail,child->mirror_size,patch.field);
    }
    return layouts;
  }

  void AdvanceChildren(std::uint16_t id,std::uint32_t count,ByteView &cursor,unsigned depth) {
    const auto *layout=FindFlatLayout(id);
    if(!layout||std::uint64_t(count)*layout->mirror_size>cursor.size())Bad("flat_child_main_bounds");
    const auto mains=Take(cursor,std::size_t(count)*layout->mirror_size);
    Visit(count);
    for(std::uint32_t i=0;i<count;++i){
      const auto main=mains.subspan(std::size_t(i)*layout->mirror_size,layout->mirror_size);
      for(const auto &patch:layout->patches)if(patch.kind==FlatPatchKind::Container||patch.kind==FlatPatchKind::Owner)
        Advance(patch,main.subspan(patch.mirror_offset,patch.size),cursor,depth+1);
    }
  }

  void Validate(const FlatLayout &layout,ByteView main,ByteView &tail,ClientFlatImage &image,
                std::span<const std::uint32_t> selected={},std::span<const std::uint32_t> groups={},
                unsigned depth=0,std::uint16_t root_field=kNullIndex,bool check_scalars=true) {
    Visit(layout.fields.size());
    // 连续复制区里的原样叶没有补丁，条件叶则须在确认有效分支后单独校验。
    for(std::uint16_t i=0;check_scalars&&i<layout.fields.size();++i){
      const auto &field=layout.fields[i];
      if((!depth&&!FlatFieldInStream(layout,i,stream_))||(!selected.empty()&&!Bit(selected,i)))continue;
      const auto patch=std::lower_bound(layout.patches.begin(),layout.patches.end(),field.offset,
        [](const auto &p,auto offset){return p.mirror_offset<offset;});
      if(patch!=layout.patches.end()&&patch->field==i&&!patch->conditions.empty())continue;
      if((field.kind==FlatPatchKind::Copy||field.kind==FlatPatchKind::Convert)&&
         !ValidScalar(field,main.subspan(field.offset,field.size)))Bad("flat_scalar",i,layout.id);
    }
    if(check_scalars&&layout.id==205){
      const auto age=Load<std::int32_t>(main.subspan(layout.fields[2].offset,4));
      const auto life=Load<std::int32_t>(main.subspan(layout.fields[3].offset,4));
      if(age<0||age>=life||life>29)Bad("flat_ember_lifetime",2,layout.id);
    }
    for(const auto &g:layout.count_groups){
      if(!groups.empty()&&std::none_of(g.fields.begin(),g.fields.end(),[&](auto i){return Bit(groups,layout.fields[i].group);}))continue;
      std::uint64_t count=0;
      for(auto i:g.fields){
        const auto &f=layout.fields[i];
        const auto patch=std::lower_bound(layout.patches.begin(),layout.patches.end(),f.offset,
          [](const auto &p,auto offset){return p.mirror_offset<offset;});
        if(patch==layout.patches.end()||patch->field!=i||f.tail==kNullIndex)Bad("flat_count_group_layout",i,layout.id);
        const auto slot=main.subspan(f.offset,f.size);
        count+=FlatContainer(slot,patch->header).count;
      }
      if(count>g.limit)Bad("flat_count_group",g.fields.front(),layout.id);
    }
    for(const auto &z:layout.zero_constraints){
      if(!groups.empty()&&!Bit(groups,layout.fields[z.selector].group))continue;
      const auto &selector=layout.fields[z.selector];
      if(Word(main.subspan(selector.offset,selector.size))!=z.empty)continue;
      for(auto i:z.fields){
        const auto &f=layout.fields[i];
        if(!Zero(main.subspan(f.offset,f.size)))Bad("flat_zero_constraint",i,layout.id);
      }
    }

    const ResourceEntryView *file=nullptr;
    std::optional<ResourceLocalDescription> animation;
    bool have_sprite=false;
    for(const auto &patch:layout.patches){
      const auto slot=main.subspan(patch.mirror_offset,patch.size);
      if(patch.kind==FlatPatchKind::Zero){
        if(!Zero(slot))Bad("flat_padding",patch.field,layout.id);
        continue;
      }
      if(!depth&&!FlatFieldInStream(layout,patch.field,stream_)){
        if(!Zero(slot))Bad("flat_other_stream_slot",patch.field,layout.id);
        continue;
      }
      if(!Selected(layout,patch,main)){
        if(!Zero(slot))Bad("flat_inactive_field",patch.field,layout.id);
        image.inactive.push_back(Offset(image,slot));
        continue;
      }
      const auto &field=layout.fields[patch.field];
      if(check_scalars&&!patch.conditions.empty()&&(selected.empty()||Bit(selected,patch.field))&&
         (field.kind==FlatPatchKind::Copy||field.kind==FlatPatchKind::Convert)&&!ValidScalar(field,slot))
        Bad("flat_scalar",patch.field,layout.id);
      const auto owner=depth?root_field:patch.field;
      const bool affected=groups.empty()||Bit(groups,field.group);
      if(patch.kind==FlatPatchKind::Reference||patch.kind==FlatPatchKind::Resource){
        const auto *resource=Links(layout,patch,slot,image,owner);
        const std::string_view read=patch.host;
        if(read=="anm2_selected_file_to_resource"){file=resource;have_sprite=true;animation.reset();}
        if(read=="anm2_animation_to_index"){
          animation.reset();
          if(resource&&affected){
            ResourceCodecScratch scratch{};
            try{animation=DescribeResourceLocal(*resource,scratch);}catch(const Gap &){Bad("flat_animation_resource",patch.field,layout.id);}
            if(!file||animation->numbers[0]!=file->key.index)Bad("flat_animation_file",patch.field,layout.id);
          }
        }
      }else if(field.tail!=kNullIndex){
        auto segment=depth?tail:image.Tail(field.tail);
        const auto start=segment;
        if(patch.header==FlatHeader::LayerArray&&affected&&have_sprite&&!file&&FlatWord(slot,4))
          Bad("flat_unloaded_layers",patch.field,layout.id);
        if(patch.header==FlatHeader::Array&&affected&&
            (std::string_view(patch.host)=="read_layer_frame_selectors"||std::string_view(patch.host)=="read_null_layer_frame_selectors")){
          const unsigned group=std::string_view(patch.host)=="read_null_layer_frame_selectors"?1:0;
          const auto selectors=FlatWord(slot);
          if(selectors!=(animation?unsigned(animation->layer_count[group]):0u))Bad("flat_selector_count",patch.field,layout.id);
          if(selectors){
            if(std::uint64_t(selectors)*4>start.size())Bad("flat_selector_length",patch.field,layout.id);
            for(unsigned i=0;i<selectors;++i){
              const auto value=Load<std::int32_t>(start.subspan(i*4,4));
              const auto limit=animation->layers[group][i].frames;
              if(value< -1||(value>=0&&(limit?std::uint32_t(value)>=limit:value>0)))
                Bad("flat_selector_range",patch.field,layout.id);
            }
          }
        }
        ValidateTail(patch,slot,segment,image,owner,depth,check_scalars&&(selected.empty()||Bit(selected,patch.field)));
        const auto length=std::uint32_t(start.size()-segment.size());
        if(length)image.extras.push_back({Offset(image,slot),{Offset(image,start),length}});
        if(depth)tail=segment;
        else if(!segment.empty())Bad("flat_recursive_tail_length",patch.field,layout.id);
      }else if(patch.value_mask && affected && (Word(slot)&~std::uint64_t(patch.value_mask)))
        Bad("flat_mask_bits",patch.field,layout.id);
    }
  }

  static std::uint32_t Offset(const ClientFlatImage &image,ByteView bytes) noexcept {
    return std::uint32_t(bytes.data()-image.Bytes().data());
  }
  bool Selected(const FlatLayout &layout,const FlatPatch &patch,ByteView main) const {
    if(std::none_of(patch.conditions.begin(),patch.conditions.end(),[](const auto &c){return c.predicate==FlatPredicate::ItemConfig;}))
      return ClientFlatSelected(layout,patch,main);
    for(const auto &condition:patch.conditions){
      if(condition.predicate!=FlatPredicate::ItemConfig)Bad("flat_mixed_item_condition",patch.field,layout.id);
      const auto &field=layout.fields[condition.selector];
      const auto number=FlatWord(main.subspan(field.offset,field.size));
      if(number>=kNullIndex)Bad("flat_item_condition_index",patch.field,layout.id,StateReadiness::InvalidReference);
      const auto item=ResourceBodyContext(resources_->Directory()).Item(3,std::uint16_t(number));
      if(!item)Bad("flat_item_condition_config",patch.field,layout.id,StateReadiness::InvalidReference);
      if(item->id!=condition.operand||item->type>=64||!(condition.mask&(std::uint64_t{1}<<item->type)))return false;
      if(!item->cooldown)Bad("flat_item_condition_divisor",patch.field,layout.id);
    }
    return true;
  }

  const ResourceEntryView *Links(const FlatLayout &layout,const FlatPatch &patch,ByteView slot,
                                ClientFlatImage &image,std::uint16_t root_field) {
    const auto &field=layout.fields[patch.field];
    const auto words=std::size_t(field.components)*4;
    if(words>slot.size()||!Zero(slot.subspan(words)))Bad("flat_link_padding",patch.field,layout.id);
    const auto resource=patch.kind==FlatPatchKind::Resource;
    const ResourceEntryView *entry=nullptr;
    for(unsigned i=0;i<field.components;++i){
      const auto value=FlatWord(slot,i*4);
      if(value>kNullIndex||(value==kNullIndex&&field.value_rule==FlatValueRule::NonNull))
        Bad("flat_link_index",patch.field,layout.id);
      if(value==kNullIndex)continue;
      ClientFlatLink link{std::uint32_t(slot.data()+i*4-image.Bytes().data()),root_field,std::uint16_t(value),{},{},resource};
      if(resource){
        link.resource={patch.resource_table,link.index};
        entry=FindResource(resources_->Directory(),link.resource);
        const auto read=std::string_view(patch.host);
        if(!entry || (patch.resource_table==0&&entry->kind!=6) || (patch.resource_table==2&&entry->kind!=0) ||
           (patch.resource_table==3&&entry->kind!=7) ||
           (patch.resource_table==4&&entry->kind!=8) ||
           (read=="layer_spritesheet_path_to_resource"&&entry->kind!=kResourceImagePathKind) ||
           (read=="image_base_name_to_resource"&&entry->kind!=1&&entry->kind!=4))
          Bad("flat_resource_type",patch.field,layout.id,StateReadiness::InvalidReference);
      }else{
        const auto *member=FindStateMember(members_->Directory(),link.index);
        const std::string_view target=patch.target;
        const auto space=target=="control-group"?10u:target=="participant"?9u:target=="grid"?2u:1u;
        if(!member||member->identity.space!=space||(target=="player"&&member->identity.layout!=197))
          Bad("flat_reference_type",patch.field,layout.id,StateReadiness::InvalidReference);
        link.member=member->identity;
      }
      image.links.push_back(link);
    }
    return entry;
  }

  void ValidateTail(const FlatPatch &patch,ByteView slot,ByteView &tail,ClientFlatImage &image,
                    std::uint16_t root_field,unsigned depth,bool affected) {
    if(depth+1>=kFlatMaxDepth)Bad("flat_child_depth",patch.field);
    const auto owner=Offset(image,slot);
    if(patch.header==FlatHeader::Weapon){
      std::array<ByteView,5> mains{};
      const auto layouts=Weapon(patch,slot,tail,mains);
      for(std::uint16_t i=0;i<layouts.size();++i)if(const auto *child=layouts[i]){
        image.children.push_back({owner,Offset(image,mains[i]),child->id,i});
        Validate(*child,mains[i],tail,image,{},{},depth+1,root_field,affected);
      }
      return;
    }
    if(!patch.child_layout){Advance(patch,slot,tail,depth);return;}
    const auto *child=FindFlatLayout(patch.child_layout);
    if(!child||depth+1>=kFlatMaxDepth)Bad("flat_child_layout",patch.field);
    if(patch.header==FlatHeader::Owner&&!FlatWord(slot)&&std::string_view(patch.host).starts_with("read_existing_"))
      Bad("flat_required_plus",patch.field);
    const auto count=patch.header==FlatHeader::Owner?(FlatWord(slot)?1u:0u):FlatContainer(slot,patch.header).count;
    if(std::string_view(patch.host)=="read_umbilical_rope_nodes"&&!count)Bad("flat_rope_empty",patch.field);
    auto segment=patch.header==FlatHeader::Owner?tail:Take(tail,FlatContainer(slot,patch.header).bytes,patch.field);
    if(std::uint64_t(count)*child->mirror_size>segment.size())Bad("flat_child_main_bounds",patch.field);
    const auto mains=Take(segment,std::size_t(count)*child->mirror_size,patch.field);
    if(patch.header==FlatHeader::Tree||patch.header==FlatHeader::Aggregate){
      const auto key=child->fields.front().offset;
      for(unsigned i=1;i<count;++i)
        if(Load<std::int32_t>(mains.subspan((i-1)*child->mirror_size+key,4))>=
           Load<std::int32_t>(mains.subspan(i*child->mirror_size+key,4)))
          Bad("flat_associative_key_order",patch.field);
    }
    // 子记录随父字段整体携带。未更新的子记录只重新解析依赖，不重复标量校验。
    for(unsigned i=0;i<count;++i){
      const auto main=mains.subspan(std::size_t(i)*child->mirror_size,child->mirror_size);
      image.children.push_back({owner,Offset(image,main),child->id,std::uint16_t(i)});
      Validate(*child,main,segment,image,{},{},depth+1,root_field,affected);
    }
    if(patch.header==FlatHeader::Owner)tail=segment;
    else if(!segment.empty())Bad("flat_child_tail_length",patch.field);
  }

private:
  const StateRecordPayload &record_;
  const ClientMemberSnapshotRef &members_;
  const ClientResourceSnapshotRef &resources_;
  std::uint32_t &remaining_;
  unsigned stream_;
};
}

ClientFlatImage::ClientFlatImage(const std::shared_ptr<ClientStateMemory> &memory)
  :storage(ClientStateAllocator<Block>(memory)),tails(ClientStateAllocator<FlatTailSpan>(memory)),
   links(ClientStateAllocator<ClientFlatLink>(memory)),children(ClientStateAllocator<ClientFlatChild>(memory)),
   extras(ClientStateAllocator<ClientFlatExtra>(memory)),inactive(ClientStateAllocator<std::uint32_t>(memory)) {}
ByteView ClientFlatImage::Tail(std::uint16_t index) const noexcept {
  if(index>=tails.size())return {};
  const auto &range=tails[index];
  return Bytes().subspan(range.offset,range.size);
}
const ClientFlatLink *ClientFlatImage::Link(std::uint32_t offset) const noexcept {
  const auto at=std::lower_bound(links.begin(),links.end(),offset,[](const auto &l,auto value){return l.offset<value;});
  return at!=links.end()&&at->offset==offset?&*at:nullptr;
}
std::span<const std::byte> ClientFlatImage::Extra(std::uint32_t offset) const noexcept {
  const auto at=std::lower_bound(extras.begin(),extras.end(),offset,[](const auto &e,auto value){return e.owner<value;});
  return at!=extras.end()&&at->owner==offset?Bytes().subspan(at->bytes.offset,at->bytes.size):ByteView{};
}
std::span<const ClientFlatChild> ClientFlatImage::Children(std::uint32_t offset) const noexcept {
  const auto begin=std::lower_bound(children.begin(),children.end(),offset,[](const auto &e,auto value){return e.owner<value;});
  const auto end=std::upper_bound(begin,children.end(),offset,[](auto value,const auto &e){return value<e.owner;});
  return {begin,end};
}
bool ClientFlatImage::Active(std::uint32_t offset) const noexcept {
  return !std::binary_search(inactive.begin(),inactive.end(),offset);
}

bool ClientFlatSelected(const FlatLayout &layout,const FlatPatch &patch,std::span<const std::byte> main) noexcept {
  for(const auto &c:patch.conditions){
    const auto &f=layout.fields[c.selector];
    if(f.size>sizeof(std::uint64_t))return false;
    const auto value=Word(main.subspan(f.offset,f.size));
    switch(c.predicate){
      case FlatPredicate::Equal:if(value!=c.operand)return false;break;
      case FlatPredicate::NotEqual:if(value==c.operand)return false;break;
      case FlatPredicate::AnyBits:if(!(value&c.operand))return false;break;
      case FlatPredicate::OneOf:if(value>=64||!(c.mask&(std::uint64_t{1}<<value)))return false;break;
      default:return false;
    }
  }
  return true;
}

ClientFlatImageRef DecodeClientFlat(const FlatLayout &layout,const StateRecordPayload &record,const StateSectionView &section,
    const ClientFlatImageRef &base,const ClientMemberSnapshotRef &members,const ClientResourceSnapshotRef &resources,
    const std::shared_ptr<ClientStateMemory> &memory,std::uint32_t &remaining) {
  FlatReader read(record,section.context.stream,members,resources,remaining);
  if(!base&&record.set_mode!=FlatSetMode::All)read.Bad("flat_delta_without_image");
  auto wire=record.bytes;
  ClientStateVector<std::uint32_t> selected((layout.fields.size()+31)/32,0,ClientStateAllocator<std::uint32_t>(memory,memory::Pool::Workspace));
  ClientStateVector<std::uint32_t> groups((layout.write_groups.size()+31)/32,0,selected.get_allocator());
  ClientStateVector<std::byte> main(layout.mirror_size,std::byte{},ClientStateAllocator<std::byte>(memory,memory::Pool::Workspace));
  ClientStateVector<ByteView> spans(layout.tail_fields.size(),{},ClientStateAllocator<ByteView>(memory,memory::Pool::Workspace));
  if(base)std::copy_n(base->Bytes().begin(),main.size(),main.begin());
  read.Select(layout,section,wire,selected);
  for(std::uint16_t i=0;i<layout.fields.size();++i)if(Bit(selected,i)){
    const auto &field=layout.fields[i];
    const auto value=read.Take(wire,field.size,i);
    std::memcpy(main.data()+field.offset,value.data(),value.size());
    Set(groups,field.group);
  }
  std::size_t total=main.size();
  for(std::uint16_t i=0;i<layout.tail_fields.size();++i){
    const auto ordinal=layout.tail_fields[i];
    const auto &field=layout.fields[ordinal];
    if(Bit(selected,ordinal)){
      const auto patch=std::lower_bound(layout.patches.begin(),layout.patches.end(),field.offset,
        [](const auto &p,auto offset){return p.mirror_offset<offset;});
      if(patch==layout.patches.end()||patch->field!=ordinal)read.Bad("flat_tail_layout",ordinal);
      const auto before=wire;
      read.Advance(*patch,std::span(main).subspan(field.offset,field.size),wire,0);
      spans[i]=before.first(before.size()-wire.size());
    }else if(base)spans[i]=base->Tail(i);
    if(spans[i].size()>kFlatRecordLimit-total)read.Bad("flat_complete_image_budget",ordinal);
    total+=spans[i].size();
  }
  if(!wire.empty())read.Bad("flat_record_trailing_bytes");
  auto next=std::allocate_shared<ClientFlatImage>(ClientStateAllocator<ClientFlatImage>(memory),memory);
  next->size=std::uint32_t(total);
  next->storage.resize((total+31)/32);
  next->tails.resize(spans.size());
  const auto image=std::as_writable_bytes(std::span(next->storage));
  std::memcpy(image.data(),main.data(),main.size());
  auto position=std::uint32_t(main.size());
  for(std::uint16_t i=0;i<spans.size();++i){
    const auto segment=spans[i];
    if(segment.empty())continue;
    next->tails[i]={position,std::uint32_t(segment.size())};
    std::memcpy(image.data()+position,segment.data(),segment.size());
    position+=std::uint32_t(segment.size());
  }
  ByteView rest;
  read.Validate(layout,next->Bytes().first(layout.mirror_size),rest,*next,selected,groups);
  if(layout.id==125){
    const auto type=ClientFlatNamed(layout,*next,"type");
    if(type.size()!=4||FlatWord(type)!=2)read.Bad("flat_tear_factory_type");
  }
  std::sort(next->links.begin(),next->links.end(),[](const auto &a,const auto &b){return a.offset<b.offset;});
  std::sort(next->children.begin(),next->children.end(),[](const auto &a,const auto &b){
    return std::tie(a.owner,a.element)<std::tie(b.owner,b.element);
  });
  std::sort(next->extras.begin(),next->extras.end(),[](const auto &a,const auto &b){return a.owner<b.owner;});
  std::sort(next->inactive.begin(),next->inactive.end());
  if(base){
    for(const auto &link:base->links)if(!Bit(selected,link.field)){
      const auto &field=layout.fields[link.field];
      const auto offset=field.tail==kNullIndex?link.offset:
        next->tails[field.tail].offset+(link.offset-base->tails[field.tail].offset);
      const auto *inherited=next->Link(offset);
      auto expected=link;expected.offset=offset;
      if(!inherited||*inherited!=expected)read.Bad("flat_inherited_link_rebound",link.field,0,StateReadiness::InvalidReference);
    }
    if(base->links==next->links&&std::ranges::equal(base->Bytes(),next->Bytes()))return base;
  }
  return next;
}

bool ClientFlatLinksCurrent(const ClientFlatImage &image,const ClientMemberSnapshotRef &members,const ClientResourceSnapshotRef &resources) noexcept {
  for(const auto &link:image.links){
    if(link.is_resource){if(!FindResource(resources->Directory(),link.resource))return false;}
    else {
      const auto *entry=FindStateMember(members->Directory(),link.index);
      if(!entry||entry->identity!=link.member)return false;
    }
  }
  return true;
}

bool ClientFlatPlayerTopologyMatches(const ClientFlatImage &world,const ClientFlatImage &body) noexcept {
  const auto *layout=FindFlatLayout(197);
  if(!layout)return false;
  const auto w=ClientFlatNamed(*layout,world,"costume_topology_revision");
  const auto b=ClientFlatNamed(*layout,body,"render_topology_revision");
  const auto costumes=ClientFlatNamed(*layout,body,"_costumeSpriteDescs");
  if(w.size()!=4||b.size()!=4||!std::ranges::equal(w,b)||costumes.size()!=12)return false;
  const auto count=FlatWord(costumes);
  for(const auto &field:layout->fields){
    const std::string_view name=field.name;
    if(!name.starts_with("_playerCostumeMap[")||!name.ends_with("].index"))continue;
    const auto index=Load<std::uint16_t>(world.Bytes().subspan(field.offset,field.size));
    if(index!=kNullIndex&&index>=count)return false;
  }
  return true;
}

void ClientFlatWriteGroups(const FlatLayout &layout,const ClientFlatImage &image,const ClientFlatImage *before,
    ClientStateVector<std::uint32_t> &groups,const std::shared_ptr<ClientStateMemory> &memory) {
  groups.assign((layout.write_groups.size()+31)/32,0);
  if(!before||before==&image)return;
  const auto allocator=ClientStateAllocator<std::uint32_t>(memory,memory::Pool::Workspace);
  ClientStateVector<std::uint32_t> bytes(layout.mirror_size/32,0,allocator);
  ClientStateVector<std::uint32_t> fields((layout.fields.size()+31)/32,0,allocator);
  if(!MarkFlatMainChanges(layout,before->Bytes().first(layout.mirror_size),image.Bytes().first(layout.mirror_size),bytes,fields))
    throw ClientShadowError(StateReadiness::InvalidEncoding,"flat_write_padding");
  const auto &simd=FlatKernels();
  for(std::uint16_t i=0;i<layout.tail_fields.size();++i){
    const auto a=image.Tail(i),b=before->Tail(i);
    if(a.size()!=b.size()||(!a.empty()&&!simd.equal(reinterpret_cast<const unsigned char *>(a.data()),
        reinterpret_cast<const unsigned char *>(b.data()),a.size())))Set(fields,layout.tail_fields[i]);
  }
  if(image.links!=before->links){
    for(const auto &link:image.links)if(!link.is_resource){
      // 比较实际身份；同一线上号也可能在换目录后指向另一形态。
      if(std::none_of(before->links.begin(),before->links.end(),[&](const auto &prior){
        return !prior.is_resource&&prior.field==link.field&&prior.index==link.index&&prior.member==link.member;
      }))Set(fields,link.field);
    }
  }
  for(std::size_t i=0;i<fields.size();++i){
    auto word=fields[i];
    while(word){const auto field=i*32+std::countr_zero(word);Set(groups,layout.fields[field].group);word&=word-1;}
  }
}

std::span<const std::byte> ClientFlatNamed(const FlatLayout &layout,const ClientFlatImage &image,std::string_view name) noexcept {
  const auto at=std::find_if(layout.fields.begin(),layout.fields.end(),[&](const auto &f){return f.name==name;});
  return at==layout.fields.end()?ByteView{}:image.Bytes().subspan(at->offset,at->size);
}
}
