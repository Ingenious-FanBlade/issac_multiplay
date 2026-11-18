#include "state/resource_protocol.h"
#include "state/budget.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <tuple>

namespace cs::state {
namespace {
struct ResourceDispatch { std::uint8_t table,kind; std::uint16_t layout; bool recipe; };
struct BlendProfile { std::array<std::uint32_t,5> values; };
struct SamplerProfile { std::array<std::uint32_t,4> values; };
struct GraphicsRule { std::uint32_t shaders, blends, samplers; };
#include "state_resource_protocol.inc"
[[noreturn]] void Fail(const char *reason) { throw Gap("resource_protocol",reason); }
std::uint64_t Bits(std::span<const std::byte> bytes) {
  if (bytes.empty() || bytes.size()>8) Fail("integer_width");
  std::uint64_t value=0;
  for (std::size_t i=0;i<bytes.size();++i) value|=std::uint64_t(std::to_integer<unsigned>(bytes[i]))<<(8*i);
  return value;
}
const ResourceDispatch &Dispatch(const ResourceEntryView &entry) {
  if (entry.key.index==kNullIndex) Fail("null_resource_table_entry");
  const auto it=std::find_if(kResourceDispatch.begin(),kResourceDispatch.end(),[&](const auto &d){
    return d.table==entry.key.table && d.kind==entry.kind;
  });
  if (it==kResourceDispatch.end()) Fail("unregistered_table_kind_or_disabled_media");
  return *it;
}
void Limits(ResourceCodecLimits limits) {
  if (!limits.body.reliable || !limits.body.bytes || limits.body.bytes>kRLimit_directory_bytes ||
      !limits.body.depth || limits.body.depth>kCodecMaxDepth) Fail("invalid_resource_codec_limits");
}
bool Overlap(std::span<const std::byte> a,std::span<const std::byte> b) {
  if (a.empty() || b.empty()) return false;
  const auto x=reinterpret_cast<std::uintptr_t>(a.data()),y=reinterpret_cast<std::uintptr_t>(b.data());
  return x<=y ? y-x<a.size() : x-y<b.size();
}
class Cursor {
public:
  explicit Cursor(std::span<const std::byte> bytes):bytes_(bytes) {}
  std::span<const std::byte> Take(std::size_t size) {
    if (size>bytes_.size()-position_) Fail("truncated_resource_payload");
    auto value=bytes_.subspan(position_,size);position_+=size;return value;
  }
  std::size_t Position() const { return position_; }
private:
  std::span<const std::byte> bytes_;std::size_t position_=0;
};
template<class E> using Values=std::array<std::uint64_t,std::size_t(E::count)>;
template<class E> std::uint64_t &At(Values<E> &values,E field) { return values[std::size_t(field)]; }
class FixedSource final : public BodyValueSource {
public:
  explicit FixedSource(std::span<const std::uint64_t> values):values_(values) {}
  std::span<const std::byte> Scalar(const CodecLocation &loc,ScalarKind kind,std::uint32_t count) const override {
    const auto width=kind==ScalarKind::U8?1u:kind==ScalarKind::U16?2u:kind==ScalarKind::U32?4u:kind==ScalarKind::U64?8u:0u;
    if (!width || count!=1 || !loc.parents.empty() || loc.field>=values_.size()) Fail("fixed_resource_header_plan");
    const auto value=values_[loc.field];
    if (width<8 && value>>(8*width)) Fail("fixed_resource_header_narrowing");
    for (unsigned i=0;i<width;++i) bytes_[i]=std::byte(value>>(8*i));
    return std::span(bytes_).first(width);
  }
  std::uint32_t Count(const CodecLocation &) const override { Fail("fixed_container"); }
  bool SharedPresent(const CodecLocation &) const override { Fail("fixed_shared"); }
  std::uint32_t SlotMask(const CodecLocation &) const override { Fail("fixed_choice"); }
  std::uint16_t SlotLayout(const CodecLocation &,std::uint8_t) const override { Fail("fixed_choice"); }
private:
  std::span<const std::uint64_t> values_;mutable std::array<std::byte,8> bytes_{};
};
template<class E> Values<E> ReadFixed(std::uint16_t layout,std::span<const std::byte> bytes) {
  std::array<BodyFieldView,std::size_t(E::count)> fields{};
  BodyCodecScratch scratch;scratch.root_fields=fields;
  const auto body=DecodeValue(layout,bytes,scratch);
  if (body.fields.size()!=fields.size()) Fail("fixed_resource_header_fields");
  Values<E> out{};
  for (const auto &f:body.fields) {
    if (!f.present || f.field>=out.size()) Fail("fixed_resource_header_field");
    out[f.field]=Bits(f.bytes);
  }
  return out;
}
template<class E> void WriteFixed(std::uint16_t layout,const Values<E> &values,std::span<std::byte> output) {
  FixedSource source(values);BodyCodecScratch scratch;
  if (EncodeValueInto(layout,source,output,scratch).bytes.size()!=output.size()) Fail("fixed_resource_header_size");
}
std::span<const std::byte> FieldBytes(std::span<const BodyFieldView> fields,std::uint16_t id) {
  const auto it=std::find_if(fields.begin(),fields.end(),[&](const auto &f){return f.field==id && f.present;});
  if (it==fields.end()) Fail("missing_resource_value_field");
  return it->bytes;
}
template<class E> std::uint64_t Number(std::span<const BodyFieldView> fields,E id) { return Bits(FieldBytes(fields,std::uint16_t(id))); }
template<class E> std::uint64_t FixedNumber(std::uint16_t layout,std::span<const std::byte> bytes,E id) {
  const auto *field=FindField(layout,std::uint16_t(id));
  if (!field || field->min_size!=field->max_size || field->wire_offset>bytes.size() ||
      field->max_size>bytes.size()-field->wire_offset) Fail("fixed_resource_value_field");
  return Bits(bytes.subspan(field->wire_offset,field->max_size));
}
void Geometry(std::uint64_t w,std::uint64_t h,std::uint64_t stored_w,std::uint64_t stored_h,bool surface) {
  if (!w || !h || w>stored_w || h>stored_h || stored_w>UINT16_MAX || stored_h>UINT16_MAX ||
      (surface && (stored_w>kRLimit_surface_dimension || stored_h>kRLimit_surface_dimension))) Fail("invalid_resource_geometry");
  if (stored_w>UINT32_MAX/(4*stored_h)) Fail("resource_geometry_storage_overflow");
}
// 路径为官方资源VFS相对名，禁止根目录、Windows分隔符／设备／URL及点组件。
// UTF-8按标量检查，不能将任意媒体字节或控制码伪装成本地路径。
void Utf8(std::span<const std::byte> bytes) {
  for (std::size_t i=0;i<bytes.size();) {
    const auto lead=std::to_integer<unsigned>(bytes[i++]);std::uint32_t value=lead,minimum=0;unsigned more=0;
    if (lead>=0xc2 && lead<=0xdf) {value=lead&31;minimum=0x80;more=1;}
    else if (lead>=0xe0 && lead<=0xef) {value=lead&15;minimum=0x800;more=2;}
    else if (lead>=0xf0 && lead<=0xf4) {value=lead&7;minimum=0x10000;more=3;}
    else if (lead>0x7f) Fail("invalid_resource_path_utf8");
    if (more>bytes.size()-i) Fail("truncated_resource_path_utf8");
    while (more) {--more;const auto next=std::to_integer<unsigned>(bytes[i++]);if ((next&0xc0)!=0x80) Fail("invalid_resource_path_utf8");value=(value<<6)|(next&63);}
    if (value<minimum || value>0x10ffff || value<32 || (value>=0x7f && value<=0x9f) || (value>=0xd800 && value<=0xdfff)) Fail("invalid_resource_path_codepoint");
  }
}
std::string_view Text(std::span<const std::byte> encoded) {
  if (encoded.size()<2 || Bits(encoded.first(2))!=encoded.size()-2) Fail("resource_text_length");
  return {reinterpret_cast<const char *>(encoded.data()+2),encoded.size()-2};
}
char Lower(char c) { return c>='A' && c<='Z'?char(c-'A'+'a'):c; }
bool Ends(std::string_view text,std::string_view suffix) {
  return text.size()>=suffix.size() && std::equal(suffix.begin(),suffix.end(),text.end()-suffix.size(),[](char a,char b){return a==Lower(b);});
}
bool EqualAscii(std::string_view text,std::string_view other) {
  return text.size()==other.size() && std::equal(text.begin(),text.end(),other.begin(),[](char a,char b){return Lower(a)==b;});
}
void Path(std::span<const std::byte> bytes,std::uint8_t kind) {
  const auto text=Text(bytes);Utf8(bytes.subspan(2));
  if (text.empty() || text.front()=='/' || text.back()=='/' || text.find_first_of("\\:<>\"|?*")!=text.npos) Fail("noncanonical_resource_path");
  for (std::size_t begin=0;begin<text.size();) {
    auto end=text.find('/',begin);if (end==text.npos) end=text.size();const auto part=text.substr(begin,end-begin);
    if (part.empty() || part=="." || part==".." || part.back()=='.' || part.front()==' ' || part.back()==' ') Fail("invalid_resource_path_component");
    const auto stem=part.substr(0,part.find('.'));
    if (EqualAscii(stem,"con") || EqualAscii(stem,"prn") || EqualAscii(stem,"aux") || EqualAscii(stem,"nul") ||
        (stem.size()==4 && stem[3]>='1' && stem[3]<='9' && (EqualAscii(stem.substr(0,3),"com") || EqualAscii(stem.substr(0,3),"lpt"))))
      Fail("resource_path_device_component");
    begin=end+1;
  }
  const bool gfx=text.starts_with("gfx/"),music=text.starts_with("music/"),sfx=text.starts_with("sfx/");
  const bool image=kind==1||kind==kResourceImagePathKind;
  if (kind==0 ? !(gfx && Ends(text,".anm2")) : image ? !(gfx && (Ends(text,".png") || Ends(text,".pcx"))) :
      kind==9 ? !((sfx || music) && (Ends(text,".ogg") || Ends(text,".wav") || Ends(text,".mp3"))) : true)
    Fail("resource_path_kind_or_namespace_mismatch");
}
// 借用scratch只在当前调用生效，异常也恢复observer和root字段区，禁止悬空回调。
class ScratchLease {
public:
  ScratchLease(BodyCodecScratch &scratch,BodyNodeObserver *observer,std::span<BodyFieldView> fields={})
      :scratch_(scratch),saved_(scratch) {scratch_.observer=observer;scratch_.root_fields=fields;}
  ~ScratchLease() {scratch_=saved_;}
private:
  BodyCodecScratch &scratch_;BodyCodecScratch saved_;
};
class ResourceObserver final : public BodyNodeObserver {
public:
  ResourceObserver(const ResourceEntryView *entry,const ResourceDirectoryView *directory=nullptr,
      std::span<ResourceEdge> edges={},const ResourceReadyContext *ready=nullptr)
      :entry_(entry),directory_(directory),edges_(edges),ready_(ready) {}
  void Node(const BodyNodeView &node) override {
    if (entry_ && node.layout==kR_ResourcePath) Path(node.bytes,entry_->kind);
    if (!node.layout && node.operation==Operation::Resource) {
      const auto index=Bits(node.bytes);
      if (index!=kNullIndex) {
        if (index>UINT16_MAX) Fail("resource_reference_width");
        const ResourceKey key{node.resource_table,std::uint16_t(index)};
        if (collect_) {
          if (used>=edges_.size() || used>=kRLimit_edges) Fail("resource_edge_scratch_budget");
          edges_[used++].target=key;
        }
        if (directory_ && ready_) {
          const auto *target=FindResource(*directory_,key);
          if (!target || !ready_->Materialized(*directory_,*target)) Fail("resource_binding_not_ready");
        }
      }
    }
    if (directory_ && node.layout==kR_AudioVoiceParameters) {
      const auto asset=std::uint16_t(FixedNumber(node.layout,node.bytes,AudioVoiceParametersField::asset));
      if (const auto *audio=FindResource(*directory_,{7,asset})) {
        if (audio->kind!=9) Fail("voice_asset_kind_mismatch");
        const auto audio_class=FixedNumber(kR_ResourceAudio,audio->payload,ResourceAudioField::audio_class);
        const auto volume_class=FixedNumber(node.layout,node.bytes,AudioVoiceParametersField::volume_class);
        const auto segment=FixedNumber(node.layout,node.bytes,AudioVoiceParametersField::segment);
        const auto intro=FixedNumber(kR_ResourceAudio,audio->payload,ResourceAudioField::has_intro);
        if (audio_class!=volume_class || (audio_class==0 ? segment!=0 : segment==0) || (segment==1 && !intro))
          Fail("voice_asset_class_or_segment_mismatch");
      }
    }
  }
  void Collect() {collect_=true;}
  std::uint32_t used=0;
private:
  const ResourceEntryView *entry_;const ResourceDirectoryView *directory_;
  std::span<ResourceEdge> edges_;const ResourceReadyContext *ready_;
  bool collect_=false;
};
// 纯值容器保持文件实际顺序；重复layer ID不能伪造一个可接受的动画目录。
void AnimationLayers(std::span<const std::byte> bytes) {
  Cursor cursor(bytes);const auto count=Bits(cursor.Take(2));
  if (count>256 || count*kR_ResourceAnimationLayerBytes!=bytes.size()-2) Fail("animation_layer_metadata_length");
  for (std::size_t i=0;i<count;++i) {
    const auto node=cursor.Take(kR_ResourceAnimationLayerBytes);
    const auto id=FixedNumber(kR_ResourceAnimationLayer,node,ResourceAnimationLayerField::layer_id);
    for (std::size_t j=0;j<i;++j) if (id==FixedNumber(kR_ResourceAnimationLayer,bytes.subspan(2+j*kR_ResourceAnimationLayerBytes,kR_ResourceAnimationLayerBytes),ResourceAnimationLayerField::layer_id))
      Fail("duplicate_animation_layer_id");
  }
}
std::uint32_t Value(std::uint16_t layout,std::span<const std::byte> bytes,ResourceCodecScratch &scratch,
    ResourceCodecLimits limits,ResourceObserver &observer) {
  std::array<BodyFieldView,kResourceMaxRootFields> fields{};ScratchLease lease(scratch.body,&observer,fields);
  const auto body=DecodeValue(layout,bytes,scratch.body,nullptr,limits.body);
  if (body.needs_base_checks) Fail("incomplete_resource_value");
  if (layout==kR_ResourceImageFile)
    Geometry(Number(body.fields,ResourceImageFileField::logical_w),Number(body.fields,ResourceImageFileField::logical_h),
             Number(body.fields,ResourceImageFileField::stored_w),Number(body.fields,ResourceImageFileField::stored_h),false);
  if (layout==kR_ResourceAnimation) {
    AnimationLayers(FieldBytes(body.fields,std::uint16_t(ResourceAnimationField::layers)));
    AnimationLayers(FieldBytes(body.fields,std::uint16_t(ResourceAnimationField::null_layers)));
  }
  if (layout==kR_ResourceAudio && Number(body.fields,ResourceAudioField::audio_class)==0 && Number(body.fields,ResourceAudioField::has_intro))
    Fail("sample_resource_has_music_intro");
  return body.field_visits;
}
std::uint32_t ValidateEntry(const ResourceEntryView &entry,ResourceCodecScratch &scratch,ResourceCodecLimits limits,ResourceObserver &observer) {
  const auto &dispatch=Dispatch(entry);
  if (entry.payload.size()>limits.body.bytes) Fail("resource_payload_byte_budget");
  if (!dispatch.recipe) return Value(dispatch.layout,entry.payload,scratch,limits,observer);
  Cursor input(entry.payload);const auto header=ReadFixed<SurfaceRecipeHeaderField>(kR_SurfaceRecipeHeader,input.Take(kR_SurfaceRecipeHeaderBytes));
  const auto n=[&](SurfaceRecipeHeaderField f){return header[std::size_t(f)];};
  if (n(SurfaceRecipeHeaderField::recipe_layout)!=kRLimit_recipe_layout || n(SurfaceRecipeHeaderField::format) ||
      n(SurfaceRecipeHeaderField::origin) || n(SurfaceRecipeHeaderField::alpha_encoding) || n(SurfaceRecipeHeaderField::color_encoding)) Fail("unknown_recipe_layout_or_geometry_encoding");
  const auto width=n(SurfaceRecipeHeaderField::logical_w),height=n(SurfaceRecipeHeaderField::logical_h);
  Geometry(width,height,n(SurfaceRecipeHeaderField::stored_w),n(SurfaceRecipeHeaderField::stored_h),true);
  // 只接受本机形状及程序图集所需的单次 Clear，像素和房间合成由原生流程拥有。
  if (n(SurfaceRecipeHeaderField::operation_count)!=1 ||
      entry.payload.size()!=kR_SurfaceRecipeHeaderBytes+kR_SurfaceRecipeClearBytes ||
      entry.payload[kR_SurfaceRecipeHeaderBytes]!=std::byte{0}) Fail("client_local_recipe_not_single_clear");
  return Value(kR_SurfaceRecipeClear,input.Take(kR_SurfaceRecipeClearBytes),scratch,limits,observer);
}
void Scope(const StateScope &scope) { if (!scope.session || !scope.run || !scope.scene) Fail("missing_resource_scope"); }
void Sorted(std::span<const ResourceEntryView> entries) {
  if (entries.size()>kRLimit_entries) Fail("resource_table_count_budget");
  for (std::size_t i=1;i<entries.size();++i) if (!(entries[i-1].key<entries[i].key)) Fail("resource_entries_not_strictly_sorted");
}
bool SameEntry(const ResourceEntryView &a,const ResourceEntryView &b) {
  return a.key==b.key && a.kind==b.kind && std::equal(a.payload.begin(),a.payload.end(),b.payload.begin(),b.payload.end());
}
// 此函数只做经校验的固定封装；调用者已检查整表源与输出不会相互覆盖。
std::span<const std::byte> PackEntry(const ResourceEntryView &entry,std::span<std::byte> output) {
  const auto total=kR_ResourceEntryHeaderBytes+entry.payload.size();
  if (total>output.size()) Fail("resource_entry_output_budget");
  Values<ResourceEntryHeaderField> h{};
  At(h,ResourceEntryHeaderField::table)=entry.key.table;At(h,ResourceEntryHeaderField::index)=entry.key.index;
  At(h,ResourceEntryHeaderField::kind)=entry.kind;At(h,ResourceEntryHeaderField::payload_bytes)=entry.payload.size();
  WriteFixed<ResourceEntryHeaderField>(kR_ResourceEntryHeader,h,output.first(kR_ResourceEntryHeaderBytes));
  if (!entry.payload.empty()) std::memcpy(output.data()+kR_ResourceEntryHeaderBytes,entry.payload.data(),entry.payload.size());
  return output.first(total);
}
} // namespace

ResourceProtocolBounds StateResourceBounds() noexcept {return {kRLimit_entries,kRLimit_edges,kRLimit_directory_bytes,kRLimit_dependency_depth};}
bool StateImagePathValid(std::span<const std::byte> encoded) noexcept {
  try{Path(encoded,1);return true;}catch(...){return false;}
}
ResourceLocalDescription DescribeResourceLocal(const ResourceEntryView &entry,ResourceCodecScratch &scratch) {
  ResourceLocalDescription out;out.table=entry.key.table;out.index=entry.key.index;out.kind=entry.kind;
  ResourceObserver observer(&entry);ValidateEntry(entry,scratch,{},observer);
  const auto &dispatch=Dispatch(entry);
  std::array<BodyFieldView,kResourceMaxRootFields> fields{};ScratchLease lease(scratch.body,nullptr,fields);
  if(dispatch.recipe) {
    // 正式本地分配只接一次Clear；图集内容由当前程序定义在Client绘制。
    const auto header=ReadFixed<SurfaceRecipeHeaderField>(kR_SurfaceRecipeHeader,entry.payload.first(kR_SurfaceRecipeHeaderBytes));
    for(std::size_t i=0;i<header.size();++i)out.numbers[i]=header[i];
    if(out.numbers[9]!=1||entry.payload.size()!=kR_SurfaceRecipeHeaderBytes+kR_SurfaceRecipeClearBytes||
        entry.payload[kR_SurfaceRecipeHeaderBytes]!=std::byte{0})Fail("client_local_recipe_not_single_clear");
    // 多标量 rgba 由可信字段位置读取，不把四个分量当成一个整数。
    const auto *rgba=FindField(kR_SurfaceRecipeClear,std::uint16_t(SurfaceRecipeClearField::rgba));
    const auto bytes=entry.payload.subspan(kR_SurfaceRecipeHeaderBytes);
    for(unsigned i=0;i<4;++i)out.numbers[10+i]=Bits(bytes.subspan(rgba->wire_offset+4*i,4));
    out.numbers[14]=FixedNumber(kR_SurfaceRecipeClear,bytes,SurfaceRecipeClearField::color_space);return out;
  }
  const auto value=DecodeValue(dispatch.layout,entry.payload,scratch.body,nullptr,{32u<<20,262144,32,true});
  const auto at=[&](unsigned f){return FieldBytes(value.fields,std::uint16_t(f));};
  if(entry.kind==0||entry.kind==kResourceImagePathKind){out.text[0]=Text(entry.payload);return out;}
  for(const auto &f:value.fields)if(f.present&&f.field<out.numbers.size()) {
    const auto *definition=FindField(dispatch.layout,f.field);
    if(definition&&definition->operation!=Operation::Container&&definition->operation!=Operation::Nested&&f.bytes.size()<=8)
      out.numbers[f.field]=Bits(f.bytes);
  }
  if(entry.kind==1)out.text[0]=Text(at(unsigned(ResourceImageFileField::path)));
  if(entry.kind==9){out.text[0]=Text(at(unsigned(ResourceAudioField::main)));
    if(out.numbers[unsigned(ResourceAudioField::has_intro)])out.text[1]=Text(at(unsigned(ResourceAudioField::intro)));}
  if(entry.kind==6){out.text[0]=Text(at(unsigned(ResourceAnimationField::name)));
    for(unsigned group=0;group<2;++group){Cursor cursor(at(unsigned(ResourceAnimationField::layers)+group));
      out.layer_count[group]=std::uint16_t(Bits(cursor.Take(2)));
      for(unsigned i=0;i<out.layer_count[group];++i){const auto bytes=cursor.Take(kR_ResourceAnimationLayerBytes);
        out.layers[group][i]={std::uint16_t(FixedNumber(kR_ResourceAnimationLayer,bytes,ResourceAnimationLayerField::layer_id)),
          std::uint32_t(FixedNumber(kR_ResourceAnimationLayer,bytes,ResourceAnimationLayerField::frame_count))};}}
  }
  return out;
}
ResourceEntryView DecodeResourceEntryPrefix(std::span<const std::byte> encoded,ResourceCodecScratch &scratch,ResourceCodecLimits limits) {
  Limits(limits);Cursor input(encoded);
  const auto h=ReadFixed<ResourceEntryHeaderField>(kR_ResourceEntryHeader,input.Take(kR_ResourceEntryHeaderBytes));
  const auto v=[&](ResourceEntryHeaderField f){return h[std::size_t(f)];};
  const auto size=std::size_t(v(ResourceEntryHeaderField::payload_bytes));
  if (size>limits.body.bytes || kR_ResourceEntryHeaderBytes>limits.body.bytes-size) Fail("resource_entry_byte_budget");
  ResourceEntryView entry{{std::uint8_t(v(ResourceEntryHeaderField::table)),std::uint16_t(v(ResourceEntryHeaderField::index))},
      std::uint8_t(v(ResourceEntryHeaderField::kind)),input.Take(size)};
  ResourceObserver observer(&entry);entry.field_visits=ValidateEntry(entry,scratch,limits,observer);entry.consumed=std::uint32_t(input.Position());return entry;
}
std::span<const std::byte> EncodeResourceEntryInto(const ResourceEntryView &entry,std::span<std::byte> output,ResourceCodecScratch &scratch,ResourceCodecLimits limits) {
  Limits(limits);const auto total=std::uint64_t(kR_ResourceEntryHeaderBytes)+entry.payload.size();
  if (total>output.size() || total>limits.body.bytes) Fail("resource_entry_output_budget");
  const auto destination=std::span<const std::byte>(output.first(std::size_t(total)));
  if (Overlap(destination,entry.payload) || Overlap(destination,std::as_bytes(std::span(&entry,1)))) Fail("overlapping_resource_encode_input");
  ResourceObserver observer(&entry);ValidateEntry(entry,scratch,limits,observer);return PackEntry(entry,output);
}
ResourceTableView DecodeResourceTable(const StateScope &expected,std::span<const std::byte> encoded,
    std::span<ResourceEntryView> entries,ResourceCodecScratch &scratch,ResourceCodecLimits limits) {
  Limits(limits);Scope(expected);
  if (encoded.size()>limits.body.bytes || encoded.size()>StateMessageBodyLimit(StateLane::Context)) Fail("resource_table_byte_budget");Cursor input(encoded);
  const auto common=ReadFixed<StateCommonHeaderField>(kR_StateCommonHeader,input.Take(kR_StateCommonHeaderBytes));
  const auto c=[&](StateCommonHeaderField f){return common[std::size_t(f)];};
  const StateScope scope{c(StateCommonHeaderField::session),c(StateCommonHeaderField::run),c(StateCommonHeaderField::floor),
      c(StateCommonHeaderField::dimension),c(StateCommonHeaderField::room),c(StateCommonHeaderField::scene)};
  if (c(StateCommonHeaderField::layout_version)!=LayoutVersion() || scope!=expected) Fail("resource_table_version_or_scope_mismatch");
  const auto table=ReadFixed<StateTableHeaderField>(kR_StateTableHeader,input.Take(kR_StateTableHeaderBytes));
  const auto t=[&](StateTableHeaderField f){return table[std::size_t(f)];};
  const auto epoch=t(StateTableHeaderField::epoch),previous=t(StateTableHeaderField::previous_epoch),count=t(StateTableHeaderField::entry_count);
  if (!epoch || previous>=epoch || count>entries.size() || count>kRLimit_entries || count*kR_ResourceEntryHeaderBytes>encoded.size()-input.Position()) Fail("resource_table_header");
  auto remaining=limits;
  for (std::size_t i=0;i<count;++i) {
    remaining.body.bytes=limits.body.bytes-std::uint32_t(input.Position());
    entries[i]=DecodeResourceEntryPrefix(encoded.subspan(input.Position()),scratch,remaining);
    input.Take(entries[i].consumed);remaining.body.field_visits-=entries[i].field_visits;
  }
  if (input.Position()!=encoded.size()) Fail("trailing_resource_table_bytes");
  Sorted(entries.first(std::size_t(count)));
  return {scope,LayoutVersion(),std::uint32_t(epoch),std::uint32_t(previous),entries.first(std::size_t(count))};
}
std::span<const std::byte> EncodeResourceTableInto(const ResourceTableView &table,std::span<std::byte> output,ResourceCodecScratch &scratch,ResourceCodecLimits limits) {
  Limits(limits);Scope(table.scope);Sorted(table.entries);
  if (table.layout_version!=LayoutVersion() || !table.epoch || table.previous_epoch>=table.epoch) Fail("resource_table_header");
  std::uint64_t total=kR_StateCommonHeaderBytes+kR_StateTableHeaderBytes;
  for (const auto &e:table.entries) total+=kR_ResourceEntryHeaderBytes+std::uint64_t(e.payload.size());
  if (total>output.size() || total>limits.body.bytes || total>StateMessageBodyLimit(StateLane::Context)) Fail("resource_table_output_budget");
  const auto destination=std::span<const std::byte>(output.first(std::size_t(total)));
  if (Overlap(destination,std::as_bytes(table.entries)) || Overlap(destination,std::as_bytes(std::span(&table,1)))) Fail("overlapping_resource_table_metadata");
  auto remaining=limits;
  for (const auto &e:table.entries) {
    if (Overlap(destination,e.payload)) Fail("overlapping_resource_table_payload");
    ResourceObserver observer(&e);remaining.body.field_visits-=ValidateEntry(e,scratch,remaining,observer);
  }
  Values<StateCommonHeaderField> h{};
  At(h,StateCommonHeaderField::layout_version)=table.layout_version;At(h,StateCommonHeaderField::session)=table.scope.session;
  At(h,StateCommonHeaderField::run)=table.scope.run;At(h,StateCommonHeaderField::floor)=table.scope.floor;
  At(h,StateCommonHeaderField::dimension)=table.scope.dimension;At(h,StateCommonHeaderField::room)=table.scope.room;At(h,StateCommonHeaderField::scene)=table.scope.scene;
  WriteFixed<StateCommonHeaderField>(kR_StateCommonHeader,h,output.first(kR_StateCommonHeaderBytes));
  Values<StateTableHeaderField> t{};At(t,StateTableHeaderField::epoch)=table.epoch;
  At(t,StateTableHeaderField::previous_epoch)=table.previous_epoch;At(t,StateTableHeaderField::entry_count)=table.entries.size();
  WriteFixed<StateTableHeaderField>(kR_StateTableHeader,t,output.subspan(kR_StateCommonHeaderBytes,kR_StateTableHeaderBytes));
  std::size_t position=kR_StateCommonHeaderBytes+kR_StateTableHeaderBytes;
  for (const auto &e:table.entries) position+=PackEntry(e,output.subspan(position)).size();
  return output.first(position);
}

const ResourceEntryView *FindResource(const ResourceDirectoryView &directory,ResourceKey key) noexcept {
  const auto it=std::lower_bound(directory.entries.begin(),directory.entries.end(),key,[](const auto &e,auto k){return e.key<k;});
  return it==directory.entries.end() || it->key!=key?nullptr:&*it;
}
ResourceDirectoryView MergeResourceTableInto(const ResourceDirectoryView &current,const ResourceTableView &incoming,
    std::span<ResourceEntryView> storage,ResourceCodecScratch &scratch,ResourceCodecLimits limits) {
  Limits(limits);Scope(incoming.scope);Sorted(current.entries);Sorted(incoming.entries);
  if (!current.epoch && (!current.entries.empty() || !current.update.empty() || current.previous_epoch)) Fail("invalid_empty_resource_directory");
  if (incoming.layout_version!=LayoutVersion() || !incoming.epoch || incoming.previous_epoch>=incoming.epoch ||
      (current.epoch && (current.session!=incoming.scope.session || current.run!=incoming.scope.run))) Fail("resource_directory_scope_or_epoch");
  auto remaining=limits;std::uint64_t bytes=0;
  for (const auto &e:incoming.entries) {
    bytes+=e.payload.size()+kR_ResourceEntryHeaderBytes;
    if (bytes>limits.body.bytes) Fail("resource_directory_update_budget");
    ResourceObserver observer(&e);remaining.body.field_visits-=ValidateEntry(e,scratch,remaining,observer);
  }
  if (current.epoch==incoming.epoch) {
    // 同一不可变快照可以在新scene／恢复时重发完整表达；不能只因最初是delta便拒绝。
    if (!incoming.previous_epoch) {
      if (current.entries.size()!=incoming.entries.size() ||
          !std::equal(current.entries.begin(),current.entries.end(),incoming.entries.begin(),SameEntry)) Fail("conflicting_full_resource_epoch");
      return current;
    }
    if (current.previous_epoch!=incoming.previous_epoch || current.update.size()!=incoming.entries.size() ||
        !std::equal(current.update.begin(),current.update.end(),incoming.entries.begin(),SameEntry)) Fail("conflicting_resource_epoch_retransmission");
    return current;
  }
  if (incoming.epoch<current.epoch || (incoming.previous_epoch && incoming.previous_epoch!=current.epoch) ||
      (!current.epoch && incoming.previous_epoch)) Fail("resource_previous_epoch_mismatch_or_rewind");
  const auto destination=std::as_bytes(storage);
  if (Overlap(destination,std::as_bytes(current.entries)) || Overlap(destination,std::as_bytes(incoming.entries)) ||
      Overlap(destination,std::as_bytes(std::span(&current,1))) || Overlap(destination,std::as_bytes(std::span(&incoming,1)))) Fail("overlapping_resource_merge_storage");
  for (const auto &e:current.entries) if (Overlap(destination,e.payload)) Fail("overlapping_resource_merge_payload");
  for (const auto &e:incoming.entries) if (Overlap(destination,e.payload)) Fail("overlapping_resource_merge_payload");
  std::array<std::uint16_t,8> maximum{};std::array<bool,8> present{};
  for (const auto &e:current.entries) {if (e.key.table>7 || e.key.index==kNullIndex) Fail("invalid_existing_resource_key");maximum[e.key.table]=e.key.index;present[e.key.table]=true;}
  std::size_t old=0,added=0,used=0;bytes=0;
  const auto emit=[&](const ResourceEntryView &e){
    if (used>=storage.size() || used>=kRLimit_entries) Fail("resource_directory_storage_exhausted");
    bytes+=e.payload.size()+kR_ResourceEntryHeaderBytes;
    if (bytes>kRLimit_directory_bytes) Fail("resource_directory_byte_budget");
    storage[used++]=e;
  };
  while (old<current.entries.size() || added<incoming.entries.size()) {
    if (added==incoming.entries.size() || (old<current.entries.size() && current.entries[old].key<incoming.entries[added].key)) {
      if (!incoming.previous_epoch) Fail("full_resource_snapshot_omits_known_entry");
      emit(current.entries[old++]);
    } else if (old<current.entries.size() && current.entries[old].key==incoming.entries[added].key) {
      if (!SameEntry(current.entries[old],incoming.entries[added])) Fail("immutable_resource_content_changed");
      emit(current.entries[old++]);++added;
    } else {
      const auto &e=incoming.entries[added++];
      if (present[e.key.table] && e.key.index<=maximum[e.key.table]) Fail("resource_index_reused_or_not_monotonic");
      emit(e);
    }
  }
  return {incoming.scope.session,incoming.scope.run,incoming.epoch,incoming.previous_epoch,storage.first(used),incoming.entries};
}

ResourceGraphPlan BuildResourceGraph(const ResourceDirectoryView &directory,ResourceGraphScratch &graph,
    ResourceCodecScratch &scratch,ResourceCodecLimits limits) {
  Limits(limits);Sorted(directory.entries);
  const auto count=directory.entries.size();
  if (!directory.session || !directory.run || !directory.epoch || graph.state.size()<count || graph.heights.size()<count || graph.offsets.size()<count+1 || graph.order.size()<count)
    Fail("resource_graph_identity_or_scratch_budget");
  auto remaining=limits;std::uint32_t edges=0;std::uint64_t bytes=0;
  for (std::size_t i=0;i<count;++i) {
    const auto &e=directory.entries[i];bytes+=e.payload.size()+kR_ResourceEntryHeaderBytes;
    if (bytes>kRLimit_directory_bytes || edges>graph.edges.size()) Fail("resource_graph_budget");
    graph.offsets[i]=edges;graph.state[i]=0;graph.heights[i]=0;
    ResourceObserver observer(&e,&directory,graph.edges.subspan(edges));observer.Collect();
    remaining.body.field_visits-=ValidateEntry(e,scratch,remaining,observer);
    edges+=observer.used;if (edges>kRLimit_edges) Fail("resource_graph_edge_budget");
  }
  graph.offsets[count]=edges;
  ResourceGraphPlan plan{directory.session,directory.run,directory.epoch,directory.entries,
      std::span<const ResourceEdge>(graph.edges.first(edges)),std::span<const std::uint32_t>(graph.offsets.first(count+1))};
  // 未到的声明可以等待；整个已知目录的环和最长路径仍在安装元数据前明确失败。
  (void)PrepareResourceClosure(directory,plan,graph);
  return plan;
}
ResourceGraphView PrepareResourceClosure(const ResourceDirectoryView &directory,const ResourceGraphPlan &plan,
    ResourceGraphScratch &graph,const ResourceReadyContext *ready,std::span<const ResourceKey> roots) {
  const auto count=directory.entries.size();
  if (!directory.session || !directory.run || !directory.epoch || count>kRLimit_entries || plan.session!=directory.session || plan.run!=directory.run || plan.epoch!=directory.epoch ||
      plan.entries.data()!=directory.entries.data() || plan.entries.size()!=count || plan.offsets.size()!=count+1 ||
      graph.state.size()<count || graph.heights.size()<count || graph.order.size()<count ||
      plan.edges.size()>kRLimit_edges || roots.size()>kRLimit_edges) Fail("resource_graph_plan_identity_or_scratch_budget");
  if (plan.offsets.front()!=0 || plan.offsets.back()!=plan.edges.size()) Fail("resource_graph_plan_offsets");
  for (std::size_t i=0;i<count;++i) {
    if (plan.offsets[i]>plan.offsets[i+1]) Fail("resource_graph_plan_offsets");
    graph.state[i]=0;graph.heights[i]=0;
  }
  std::uint32_t missing=0,ordered=0;bool materialized=ready!=nullptr;
  const auto visit=[&](auto &self,std::size_t index,unsigned depth)->void {
    if (depth>kRLimit_dependency_depth) Fail("resource_dependency_depth_budget");
    if (graph.state[index]==1) Fail("resource_dependency_cycle");
    if (graph.state[index]==2) return;
    graph.state[index]=1;
    unsigned height=1;
    for (auto edge=plan.offsets[index];edge<plan.offsets[index+1];++edge) {
      const auto *target=FindResource(directory,plan.edges[edge].target);
      if (!target) {++missing;materialized=false;continue;}
      const auto child=std::size_t(target-directory.entries.data());self(self,child,depth+1);
      height=std::max(height,unsigned(graph.heights[child])+1);
      if (height>kRLimit_dependency_depth) Fail("resource_dependency_depth_budget");
    }
    if (ready && !ready->Materialized(directory,directory.entries[index])) materialized=false;
    graph.heights[index]=std::uint8_t(height);graph.state[index]=2;graph.order[ordered++]=std::uint32_t(index);
  };
  if (roots.empty()) for (std::size_t i=0;i<count;++i) visit(visit,i,1);
  else for (const auto key:roots) {
    if (key.table>7 || key.index==kNullIndex) Fail("invalid_resource_graph_root");
    const auto *entry=FindResource(directory,key);
    if (entry) visit(visit,std::size_t(entry-directory.entries.data()),1);
    else {++missing;materialized=false;}
  }
  const bool closed=!missing;
  return {closed,closed && materialized,std::uint32_t(plan.edges.size()),missing,
      closed?std::span<const std::uint32_t>(graph.order.first(ordered)):std::span<const std::uint32_t>{}};
}
ResourceGraphView ValidateResourceGraph(const ResourceDirectoryView &directory,ResourceGraphScratch &graph,
    ResourceCodecScratch &scratch,const ResourceReadyContext *ready,ResourceCodecLimits limits,std::span<const ResourceKey> roots) {
  const auto plan=BuildResourceGraph(directory,graph,scratch,limits);
  return PrepareResourceClosure(directory,plan,graph,ready,roots);
}
std::span<const std::uint32_t> ResourceBlendProfile(std::uint8_t profile) {
  if (profile>=kResource_blend_profiles.size()) Fail("unknown_blend_profile");return kResource_blend_profiles[profile].values;
}
std::span<const std::uint32_t> ResourceSamplerProfile(std::uint8_t profile) {
  if (profile>=kResource_sampler_profiles.size()) Fail("unknown_sampler_profile");return kResource_sampler_profiles[profile].values;
}
std::optional<std::uint8_t> FindResourceBlendProfile(std::span<const std::uint32_t> values) {
  if (values.size()!=5) return {};
  for (std::size_t i=0;i<kResource_blend_profiles.size();++i)
    if (std::equal(values.begin(),values.end(),kResource_blend_profiles[i].values.begin())) return std::uint8_t(i);
  return {};
}
std::uint8_t MakeResourceSamplerProfile(std::uint32_t min_filter,std::uint32_t mag_filter,
    std::uint32_t wrap_s,std::uint32_t wrap_t) noexcept {
  return std::uint8_t((min_filter!=0) | ((mag_filter!=0)<<1) | ((wrap_s!=0)<<2) | ((wrap_t!=0)<<3));
}
bool ResourceShaderAllowed(std::uint8_t use,std::uint8_t shader_type) noexcept {
  return use<kResourceGraphicsRules.size() && shader_type<20 && (kResourceGraphicsRules[use].shaders&(1u<<shader_type));
}
void ValidateResourceBindings(std::uint16_t layout,std::uint32_t mask,BodyStream stream,std::span<const std::byte> bytes,
    const ResourceDirectoryView &directory,const ResourceReadyContext &ready,BodyCodecScratch &scratch,
    const BodyCodecContext *context,BodyCodecLimits limits) {
  if (!directory.session || !directory.run || !directory.epoch) Fail("resource_binding_directory_missing");
  ResourceObserver observer(nullptr,&directory,{},&ready);ScratchLease lease(scratch,&observer);
  const auto body=stream==BodyStream::Value?DecodeValue(layout,bytes,scratch,context,limits):DecodeBody(layout,mask,stream,bytes,scratch,context,limits);
  // 投影／base完整性由第12步合并候选检查，不因只核资源引用而给body增加world等待。
  (void)body;
}
void ValidateResourceBindingNode(const BodyNodeView &node,const ResourceDirectoryView &directory,const ResourceReadyContext &ready) {
  ResourceObserver observer(nullptr,&directory,{},&ready);observer.Node(node);
}
std::optional<CodecItemConfig> ResourceBodyContext::Item(std::uint8_t table,std::uint16_t index) const {
  if (table!=3) return {};
  const auto *entry=FindResource(directory_,{table,index});
  if (!entry || entry->kind!=7) return {};
  return CodecItemConfig{std::uint32_t(FixedNumber(kR_ResourceItemConfig,entry->payload,ResourceItemConfigField::type)),
      std::uint32_t(FixedNumber(kR_ResourceItemConfig,entry->payload,ResourceItemConfigField::id)),
      std::bit_cast<std::int32_t>(std::uint32_t(FixedNumber(kR_ResourceItemConfig,entry->payload,ResourceItemConfigField::cooldown)))};
}
} // namespace cs::state
