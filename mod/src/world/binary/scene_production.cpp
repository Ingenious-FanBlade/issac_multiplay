#include "world/binary/entity_source_internal.h"
#include "state/resource_protocol.h"
#include <cmath>

namespace cs::world::binary {
// 只解释已核四种Image的尺寸getter对应成员，不执行虚函数或GPU读回。
bool EntitySource::Impl::ImageGeometry(std::uint32_t image,std::array<std::uint32_t,4> &geometry,CaptureDiagnostic &d) noexcept {
  std::uint32_t type=0;if(!Value(image,type,d))return false;
  if(type==memory.image_base+0x73ba80){std::array<std::uint16_t,4> values{};if(!Value(std::uint64_t(image)+128,values,d))return false;std::copy(values.begin(),values.end(),geometry.begin());}
  else if(type==memory.image_base+0x73b8ec){if(!Value(std::uint64_t(image)+256,geometry,d))return false;}
  else if(type==memory.image_base+0x73bcac){if(!Value(std::uint64_t(image)+128,geometry,d))return false;}
  else if(type==memory.image_base+0x73bc14){std::array<std::uint16_t,2> logical{};std::array<std::uint32_t,2> stored{};
    if(!Value(std::uint64_t(image)+128,logical,d) || !Value(std::uint64_t(image)+132,stored,d))return false;geometry={logical[0],logical[1],stored[0],stored[1]};}
  else return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  if(!geometry[0] || !geometry[1] || geometry[0]>geometry[2] || geometry[1]>geometry[3] || geometry[2]>65535 || geometry[3]>65535)
    return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});return true;
}
bool EntitySource::Impl::ProgramImage(std::uint32_t image,CaptureLink &out,CaptureDiagnostic &d) noexcept {
  if(!production_valid || (!production_read && (!facts.scene || production_depth || production_owner!=facts.scene->room+7112 || production_descriptor!=facts.scene->current_descriptor)))
    return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  for(const auto &surface:surfaces)if(surface.image==image && surface.resource){std::array<std::uint32_t,4> geometry{};
    const auto &resource=resources[std::size_t(surface.resource-1)];
    if(!ImageGeometry(image,geometry,d) || !std::equal(geometry.begin(),geometry.end(),resource.values.begin()))return FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});
    out={CapturedReferenceKind::Identity,surface.resource};return true;}
  return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
}
// 918A60／9191F0／919030：+20是追加的floorSurface2 pass，并非替换floorBuffer。
// gfx／nlfloor与temp仅为生产输入／scratch；它们的实际内容依赖已在不可变recipe中闭合。
bool EntitySource::Impl::BackdropBindings(std::array<std::array<std::uint32_t,2>,7> &out,std::uint32_t &count,CaptureDiagnostic &d) noexcept {
  if(!facts.scene)return FailCapture(d,CaptureFailure::MissingQualification,{facts.subject});const auto owner=std::uint64_t(facts.scene->room)+7112;
  std::uint32_t id=0,walls=0;std::uint8_t auxiliary=0;float water=0;count=0;
  if(!Tracked(owner+336,id,d) || id>=63 || !Tracked(owner+32,auxiliary,d) || auxiliary>1 ||
     !Tracked(owner+340+id*316u+100,walls,d) || !Tracked(std::uint64_t(facts.scene->room)+29248,water,d) || !std::isfinite(water))
    return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  const auto add=[&](unsigned slot,unsigned offset,bool required){std::array<std::uint32_t,2> pair{};if(!Tracked(owner+offset,pair,d))return false;
    if(!pair[0])return !required || FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});if(!pair[1])return FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});
    out[count++]={slot,pair[0]};return true;};
  if(id!=26 && id!=35 && !add(0,0,true))return false;
  if(id!=18 && id!=35 && !(id>=52 && id<=54) && walls && !add(1,8,true))return false;
  if(auxiliary && !add(2,16,true))return false;
  if(water>0 && !add(5,44,false))return false;return true;
}
// 下一次提交／切target／清理前，上一quad的调用者已经写完ColorMod属性，且顶点尚未回收。
bool EntitySource::Impl::FlushSurface(CaptureDiagnostic &d) noexcept {
  if(!pending_image)return true;pending_image=false;if(!pending_vertices)return true;
  if(!surface_valid || surface_used==surface_operations.size() || (pending_stride!=28 && pending_stride!=36 && pending_stride!=88))return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
  auto operation=pending_surface;std::array<std::array<float,22>,4> vertices{};
  for(unsigned vertex=0;vertex<4;++vertex){if(!Read(std::uint64_t(pending_vertices)+vertex*pending_stride,std::as_writable_bytes(std::span(vertices[vertex])).first(pending_stride),d))return false;
    operation.destination[vertex*2]=vertices[vertex][0];operation.destination[vertex*2+1]=vertices[vertex][1];
    for(unsigned part=0;part<2;++part)if(!std::isfinite(operation.destination[vertex*2+part]))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});}
  operation.modifier={1,1,1,1,0,0,0,0,0,0,0};
  if(pending_stride==88){for(unsigned i=0;i<7;++i)operation.modifier[4+i]=vertices[0][9+i];
    for(unsigned i=0;i<4;++i)operation.context[i]=vertices[0][18+i];
    for(unsigned v=0;v<4;++v){for(unsigned i=0;i<7;++i)if(!std::isfinite(vertices[v][9+i]) || vertices[v][9+i]!=vertices[0][9+i])return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
      for(unsigned i=0;i<4;++i)if(!std::isfinite(vertices[v][18+i]) || vertices[v][18+i]!=operation.context[i])return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});}}
  for(const auto coordinate:operation.source)if(!std::isfinite(coordinate))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  for(const auto &color:operation.colors)for(unsigned i=0;i<4;++i)if(!std::isfinite(std::bit_cast<float>(color[i])))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  surface_operations[surface_used++]=operation;surface_dirty=true;return true;
}
bool EntitySource::Impl::SealSurface(CaptureDiagnostic &d) noexcept {
  if(!FlushSurface(d))return false;if(!surface_dirty)return true;
  if(!surface_valid || !surface_target || !surface_used || surface_operations[0].op)return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  NativeResource value;value.table=1;value.kind=4;std::copy(surface_geometry.begin(),surface_geometry.end(),value.values.begin());value.values[4]=1;value.values[5]=surface_used;
  CaptureLink link;if(!Intern(value,std::as_bytes(std::span(surface_operations).first(surface_used)),{},link,d))return false;
  for(auto &surface:surfaces)if(surface.image==surface_target){surface.resource=link.handle;surface_dirty=false;return true;}
  return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
}

void EntitySource::SceneBuild(std::uint32_t backdrop,bool entering,bool normal,bool replace) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread || s.active)return;
  if(entering){if(!s.production_depth){if(replace || s.production_owner!=backdrop){s.surfaces={};s.production_owner=backdrop;}
      s.production_valid=backdrop && backdrop>=7112;s.surface_valid=s.surface_dirty=s.pending_image=false;s.surface_used=s.surface_target=0;
      s.production_depth=1;
      s.production_read=true;s.scratch_used=s.witness_used=0;CaptureDiagnostic d;
      if(s.production_valid&&!s.Value(std::uint64_t(backdrop)-7112+4,s.production_descriptor,d))s.production_valid=false;s.production_read=false;}
    else if(s.production_depth!=UINT32_MAX)++s.production_depth;else s.production_valid=false;
  }else {if(!s.production_depth)return;s.production_read=true;CaptureDiagnostic d;
    if(!normal || !s.SealSurface(d))s.production_valid=false;s.production_read=false;--s.production_depth;
    if(!s.production_depth){if(!s.production_valid)s.surfaces={};s.surface_valid=s.surface_dirty=s.pending_image=false;s.surface_target=s.surface_used=0;}}
}
void EntitySource::SceneGraphicsBoundary() noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread || !s.production_depth || !s.production_valid || s.active)return;
  s.production_read=true;CaptureDiagnostic d;if(!s.SealSurface(d))s.production_valid=false;s.production_read=false;
}
void EntitySource::SceneClear() noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread || !s.production_depth || !s.production_valid || s.active)return;
  s.production_read=true;CaptureDiagnostic d;std::uint32_t target=0;std::array<std::uint32_t,8> pairs{};
  if(!s.SealSurface(d) || !s.Value(std::uint64_t(s.memory.image_base)+0x7ae1ac,target,d) || !s.Value(s.production_owner,pairs,d))s.production_valid=false;
  s.surface_valid=s.surface_dirty=s.pending_image=false;s.surface_target=s.surface_used=0;
  if(s.production_valid && target){unsigned index=4;for(unsigned i=0;i<4;++i)if(pairs[i*2]==target){index=i;break;}
    if(index<4){s.surfaces[index]={target,0};s.surface_target=target;s.surface_valid=s.ImageGeometry(target,s.surface_geometry,d);s.surface_dirty=s.surface_valid;
      // 正常离屏Clear明确为RGBA零；不复制渲染器当前屏幕清色，也不继承旧surface。
      if(s.surface_valid){s.surface_operations[0]={};s.surface_used=1;}else s.production_valid=false;}}
  s.production_read=false;
}
void EntitySource::SceneImageBefore(std::uint32_t image,std::uint32_t source,std::uint32_t destination,const std::array<std::uint32_t,4> &colors) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread || !s.production_depth || !s.production_valid || s.active)return;
  s.production_read=true;CaptureDiagnostic d;if(!s.FlushSurface(d)){s.production_valid=false;s.production_read=false;return;}
  std::uint32_t target=0;if(!s.Value(std::uint64_t(s.memory.image_base)+0x7ae1ac,target,d)){s.production_valid=false;s.production_read=false;return;}
  if(target!=s.surface_target){if(!s.SealSurface(d))s.production_valid=false;s.surface_valid=false;s.surface_target=s.surface_used=0;
    // 目标恢复后允许继续追加已经完整封存的同一内容，引用仍是不可变旧版本。
    for(const auto &surface:s.surfaces)if(surface.image==target && surface.resource){const auto &value=s.resources[std::size_t(surface.resource-1)];
      if(value.kind!=4 || value.values[5]>s.surface_operations.size() || value.text_bytes!=value.values[5]*sizeof(NativeSurfaceOperation)){s.production_valid=false;break;}
      s.surface_target=target;s.surface_used=value.values[5];std::copy_n(value.values.begin(),4,s.surface_geometry.begin());
      std::memcpy(s.surface_operations.data(),s.resource_bytes.data()+value.text,value.text_bytes);s.surface_valid=true;break;}}
  if(s.production_valid && s.surface_valid){auto &operation=s.pending_surface;operation={};operation.op=1;CaptureLink input,shader;
    if(image==target || !s.ImageResource(image,input,d) || input.kind!=CapturedReferenceKind::Identity || !input.handle ||
       !s.Value(source,operation.source,d) ||
       !s.Value(destination,operation.destination,d))s.production_valid=false;
    // 9E2460是四族Render的共同leaf，调用者已完成pixel→UV转换；这里不再读不存在的Space尾字。
    operation.source_space=1;
    for(unsigned i=0;i<4 && s.production_valid;++i)if(!s.Value(colors[i],operation.colors[i],d))s.production_valid=false;
    std::array<std::uint32_t,4> sampler{};if(s.production_valid && !s.Value(std::uint64_t(image)+36,sampler,d))s.production_valid=false;
    if(s.production_valid){NativeResource builtin;builtin.table=6;builtin.kind=2;builtin.values[0]=0;
      if(!s.Intern(builtin,{},{},shader,d))s.production_valid=false;
      operation.image=input.handle;operation.shader=shader.handle;operation.sampler=MakeResourceSamplerProfile(sampler[0],sampler[1],sampler[2],sampler[3]);
      s.pending_image=true;s.pending_vertices=0;s.pending_native_image=image;
      // 28B实心形状、36B普通图和88B ColorOffset；未知格式不猜测。
      std::uint16_t stride=0;std::uint8_t count=0;std::uint32_t formats=0;
      if(!s.Value(std::uint64_t(image)+58,stride,d) || !s.Value(std::uint64_t(image)+56,count,d) || !s.Value(std::uint64_t(image)+52,formats,d))s.production_valid=false;
      constexpr std::array<std::uint8_t,7> expected={5,6,7,4,3,2,4};std::array<std::uint8_t,7> actual{};
      if((stride!=28 && stride!=36 && stride!=88) || count!=(stride==28?2u:stride==36?3u:7u) || !s.Read(formats,std::as_writable_bytes(std::span(actual)).first(count),d) ||
         !std::equal(actual.begin(),actual.begin()+count,expected.begin()))s.production_valid=false;s.pending_stride=stride;}}
  s.production_read=false;
}
void EntitySource::SceneImageAfter(std::uint32_t vertices) noexcept {
  auto &s=*impl_;if(std::this_thread::get_id()!=s.thread || !s.production_depth || !s.production_valid || !s.pending_image || s.active)return;
  s.pending_vertices=vertices;if(!vertices)return;s.production_read=true;CaptureDiagnostic d;
  // 当前真实batch记录实际blend与shader，不能取Render返回后已被恢复的全局blend。
  std::array<std::uint32_t,3> candidates{};
  // 原生Image仅在同一生产栈短借，绝不写入资源。先以真实顶点区归属选中实际batch。
  if(!s.Value(std::uint64_t(s.memory.image_base)+0x7ce594,candidates[0],d) ||
     !s.Value(std::uint64_t(s.pending_native_image)+76,candidates[1],d) || !s.Value(std::uint64_t(s.pending_native_image)+92,candidates[2],d))s.production_valid=false;
  bool found=false;
  for(const auto batch:candidates){if(!batch)continue;std::array<std::uint32_t,5> blend{};std::uint32_t shader=0,blocks=0,index=0;
    if(!s.Value(batch,blend,d) || !s.Value(std::uint64_t(batch)+20,shader,d) || !s.Value(std::uint64_t(batch)+36,blocks,d) || !s.Value(std::uint64_t(batch)+40,index,d) || index>65535)continue;
    std::array<std::uint32_t,4> block{};if(!s.Value(std::uint64_t(blocks)+index*16u,block,d) || vertices<block[0] ||
       std::uint64_t(vertices)+s.pending_stride*4u>std::uint64_t(block[0])+std::uint64_t(block[1])*s.pending_stride)continue;
    // g_AllShaders[AACAB0][0]=BAB44C才是公开ColorOffset；BAE1D8为普通贴图基础
    // shader，36B／28B绘制以单位ColorMod等价表达，不把它误认作另一个公开shader号。
    const bool color_offset=shader==s.memory.image_base+0x7ab44c;
    const bool basic=shader==s.memory.image_base+0x7ae1d8 && s.pending_stride!=88;
    const auto profile=FindResourceBlendProfile(blend);if(!profile || (!color_offset&&!basic)){s.production_valid=false;break;}
    s.pending_surface.blend=*profile;found=true;break;}
  if(!found)s.production_valid=false;s.production_read=false;
}
}
