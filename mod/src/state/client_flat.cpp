#include "state/client_flat.h"
#include "state/client_shadow.h"
#include "state/flat_encode.h"
#include "state/flat_simd.h"
#include "state/entity_counters.h"
#include <algorithm>
#include <bit>
#include <cmath>

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
  FlatReader(const StateRecordPayload &record,const ClientMemberSnapshotRef &members,
             const ClientResourceSnapshotRef &resources,std::uint32_t &remaining)
    :record_(record),members_(members),resources_(resources),remaining_(remaining) {}

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
    if(patch.header==FlatHeader::Owner){
      const auto tag=FlatWord(slot);
      if(!tag)return;
      if(tag!=(kFlatOwnerPresent|patch.child_layout))Bad("flat_owner_layout",patch.field);
      AdvanceChildren(patch.child_layout,1,cursor,depth);
      return;
    }
    std::uint32_t count=0,size=0;
    switch(patch.header){
      case FlatHeader::String:
        count=FlatWord(slot,16);size=count>15?count:0;
        if(FlatWord(slot,20)||!Zero(slot.subspan(count<=15?count:0,16-(count<=15?count:0))))
          Bad("flat_string_template",patch.field);
        break;
      case FlatHeader::LayerArray:count=FlatWord(slot,4);size=FlatWord(slot);break;
      case FlatHeader::Array:case FlatHeader::Vector:
        count=FlatWord(slot);size=FlatWord(slot,4);
        if(patch.header==FlatHeader::Vector&&FlatWord(slot,8))Bad("flat_vector_template",patch.field);
        break;
      default:Bad("flat_tail_header",patch.field);
    }
    if(count>patch.limit||(patch.max_bytes&&size>patch.max_bytes))Bad("flat_tail_limit",patch.field);
    auto child=Take(cursor,size,patch.field);
    if(patch.child_layout){
      AdvanceChildren(patch.child_layout,count,child,depth);
      if(!child.empty())Bad("flat_child_tail_length",patch.field);
    }else if(patch.header!=FlatHeader::String&&std::uint64_t(count)*patch.stride!=size)
      Bad("flat_tail_length",patch.field);
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
    for(std::uint16_t i=0;check_scalars&&i<layout.fields.size();++i){
      const auto &field=layout.fields[i];
      if(!selected.empty()&&!Bit(selected,i))continue;
      if((field.kind==FlatPatchKind::Copy||field.kind==FlatPatchKind::Convert)&&
         !ValidScalar(field,main.subspan(field.offset,field.size)))Bad("flat_scalar",i,layout.id);
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
        count+=FlatWord(slot,patch->header==FlatHeader::LayerArray?4:patch->header==FlatHeader::String?16:0);
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
      if(!ClientFlatSelected(layout,patch,main)){
        if(!Zero(slot))Bad("flat_inactive_field",patch.field,layout.id);
        continue;
      }
      const auto &field=layout.fields[patch.field];
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
          if(selectors!=(animation?animation->layer_count[group]:0))Bad("flat_selector_count",patch.field,layout.id);
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
        if(depth)tail=segment;
        else if(!segment.empty())Bad("flat_recursive_tail_length",patch.field,layout.id);
      }else if(patch.value_mask && affected && (Word(slot)&~std::uint64_t(patch.value_mask)))
        Bad("flat_mask_bits",patch.field,layout.id);
    }
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
           (patch.resource_table==4&&entry->kind!=8) ||
           (read=="layer_spritesheet_path_to_resource"&&entry->kind!=kResourceImagePathKind) ||
           (read=="image_base_name_to_resource"&&entry->kind!=1&&entry->kind!=4))
          Bad("flat_resource_type",patch.field,layout.id,StateReadiness::InvalidReference);
      }else{
        const auto *member=FindStateMember(members_->Directory(),link.index);
        if(!member||member->identity.space!=1)Bad("flat_reference_type",patch.field,layout.id,StateReadiness::InvalidReference);
        link.member=member->identity;
      }
      image.links.push_back(link);
    }
    return entry;
  }

  void ValidateTail(const FlatPatch &patch,ByteView slot,ByteView &tail,ClientFlatImage &image,
                    std::uint16_t root_field,unsigned depth,bool affected) {
    if(!patch.child_layout){Advance(patch,slot,tail,depth);return;}
    const auto *child=FindFlatLayout(patch.child_layout);
    if(!child||depth+1>=kFlatMaxDepth)Bad("flat_child_layout",patch.field);
    if(patch.header==FlatHeader::Owner && !FlatWord(slot))
      Bad("flat_required_plus",patch.field);
    const auto count=patch.header==FlatHeader::Owner?(FlatWord(slot)?1u:0u):
      FlatWord(slot,patch.header==FlatHeader::LayerArray?4:0);
    auto segment=patch.header==FlatHeader::Owner?tail:Take(tail,FlatWord(slot,patch.header==FlatHeader::LayerArray?0:4),patch.field);
    if(std::uint64_t(count)*child->mirror_size>segment.size())Bad("flat_child_main_bounds",patch.field);
    const auto mains=Take(segment,std::size_t(count)*child->mirror_size,patch.field);
    // 子记录随父字段整体携带。未更新的子记录只重新解析依赖，不重复标量校验。
    for(unsigned i=0;i<count;++i)
      Validate(*child,mains.subspan(std::size_t(i)*child->mirror_size,child->mirror_size),segment,image,
               {},{},depth+1,root_field,affected);
    if(patch.header==FlatHeader::Owner)tail=segment;
    else if(!segment.empty())Bad("flat_child_tail_length",patch.field);
  }

private:
  const StateRecordPayload &record_;
  const ClientMemberSnapshotRef &members_;
  const ClientResourceSnapshotRef &resources_;
  std::uint32_t &remaining_;
};
}

ClientFlatImage::ClientFlatImage(const std::shared_ptr<ClientStateMemory> &memory)
  :storage(ClientStateAllocator<Block>(memory)),tails(ClientStateAllocator<FlatTailSpan>(memory)),
   links(ClientStateAllocator<ClientFlatLink>(memory)) {}
ByteView ClientFlatImage::Tail(std::uint16_t index) const noexcept {
  if(index>=tails.size())return {};
  const auto &range=tails[index];
  return Bytes().subspan(range.offset,range.size);
}
const ClientFlatLink *ClientFlatImage::Link(std::uint32_t offset) const noexcept {
  const auto at=std::lower_bound(links.begin(),links.end(),offset,[](const auto &l,auto value){return l.offset<value;});
  return at!=links.end()&&at->offset==offset?&*at:nullptr;
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
  FlatReader read(record,members,resources,remaining);
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
  const auto type=ClientFlatNamed(layout,*next,"type");
  if(type.size()!=4||FlatWord(type)!=2)read.Bad("flat_tear_factory_type");
  std::sort(next->links.begin(),next->links.end(),[](const auto &a,const auto &b){return a.offset<b.offset;});
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
