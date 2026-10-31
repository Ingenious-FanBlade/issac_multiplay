#include "IsaacRepentance.h"
#include "game/room_grid_original.h"
#include "game/client.h"
#include "LuaCore.h"
#include "runtime.h"
#include <GL/gl.h>
#include <compressapi.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace cs::game {
// C++20拓扑单元的纯标量桥；SDK单元保持官方要求的C++17布局。
bool RunActive() noexcept;
namespace {
using J=rapidjson::Document;using V=rapidjson::Value;using A=J::AllocatorType;
[[noreturn]] void Fail(const char *s){throw std::runtime_error(s);}
const V &At(const V &v,const char *k){if(!v.IsObject()||!v.HasMember(k))Fail("original_missing_field");return v[k];}
std::string Str(const V &v){if(!v.IsString())Fail("original_string");return {v.GetString(),v.GetStringLength()};}
V S(const std::string &v,A &a){return V(v.data(),unsigned(v.size()),a);}
void Put(V &v,const char *k,V x,A &a){v.AddMember(V(k,a),std::move(x),a);}
void Put(V &v,const char *k,const std::string &x,A &a){Put(v,k,S(x,a),a);}
std::string Text(const V &v){rapidjson::StringBuffer b;rapidjson::Writer<rapidjson::StringBuffer>w(b);if(!v.Accept(w))Fail("original_json");return {b.GetString(),b.GetSize()};}
J Parse(const std::string &s){if(s.size()>32u<<20)Fail("original_bytes_budget");J j;j.Parse<rapidjson::kParseIterativeFlag|rapidjson::kParseValidateEncodingFlag>(s.data(),s.size());if(j.HasParseError()||!j.IsObject())Fail("original_json");return j;}
unsigned U(const V &v,unsigned limit){if(!v.IsUint()||v.GetUint()>=limit)Fail("original_index");return v.GetUint();}
V VectorValue(const Vector &v,A &a){if(!std::isfinite(v.x)||!std::isfinite(v.y))Fail("original_nonfinite");V x(rapidjson::kArrayType);x.PushBack(v.x,a);x.PushBack(v.y,a);return x;}
V FloatValues(const float *p,std::size_t n,A &a){V v(rapidjson::kArrayType);for(std::size_t i=0;i<n;++i){if(!std::isfinite(p[i]))Fail("original_nonfinite_vertex");v.PushBack(p[i],a);}return v;}
std::string Hex(const std::vector<unsigned char> &bytes){static const char *h="0123456789abcdef";std::string s;s.resize(bytes.size()*2);for(std::size_t i=0;i<bytes.size();++i){s[2*i]=h[bytes[i]>>4];s[2*i+1]=h[bytes[i]&15];}return s;}
std::vector<unsigned char> Unhex(const V &value){const auto s=Str(value);if(s.size()>16u<<20||s.size()%2)Fail("original_image_budget");std::vector<unsigned char> out(s.size()/2);
 auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a'+10);Fail("original_image_hex");};
 for(std::size_t i=0;i<out.size();++i)out[i]=static_cast<unsigned char>((digit(s[2*i])<<4)|digit(s[2*i+1]));return out;}
// Windows公开无损压缩API；传值资源保留真实RGBA8，编码没有摘要或原生地址。
std::vector<unsigned char> Pack(const std::vector<unsigned char> &raw){COMPRESSOR_HANDLE c=nullptr;if(!CreateCompressor(COMPRESS_ALGORITHM_XPRESS_HUFF,nullptr,&c))Fail("original_compressor");
 SIZE_T needed=0;Compress(c,raw.data(),raw.size(),nullptr,0,&needed);std::vector<unsigned char> out(needed);
 const bool ok=needed&&Compress(c,raw.data(),raw.size(),out.data(),out.size(),&needed);CloseCompressor(c);if(!ok)Fail("original_compression");out.resize(needed);return out;}
std::vector<unsigned char> Unpack(const V &image){const auto w=U(At(image,"width"),8193),h=U(At(image,"height"),8193);if(!w||!h||std::uint64_t(w)*h>8u*1024u*1024u)Fail("original_image_dimensions");
 auto packed=Unhex(At(image,"data"));std::vector<unsigned char> raw(std::size_t(w)*h*4);DECOMPRESSOR_HANDLE d=nullptr;if(!CreateDecompressor(COMPRESS_ALGORITHM_XPRESS_HUFF,nullptr,&d))Fail("original_decompressor");
 SIZE_T actual=0;const bool ok=Decompress(d,packed.data(),packed.size(),raw.data(),raw.size(),&actual);CloseDecompressor(d);if(!ok||actual!=raw.size())Fail("original_image_data");return raw;}
// GPU读回显式保存PACK布局；PBO不属于本模块，绑定时整候选拒绝。
struct ReadPixelsScope final{
 GLint alignment=0,row=0,rows=0,pixels=0;
 ReadPixelsScope(){GLint pbo=0;glGetIntegerv(0x88ED,&pbo);if(pbo)Fail("original_pack_pbo");glGetIntegerv(GL_PACK_ALIGNMENT,&alignment);glGetIntegerv(GL_PACK_ROW_LENGTH,&row);glGetIntegerv(GL_PACK_SKIP_ROWS,&rows);glGetIntegerv(GL_PACK_SKIP_PIXELS,&pixels);glPixelStorei(GL_PACK_ALIGNMENT,4);glPixelStorei(GL_PACK_ROW_LENGTH,0);glPixelStorei(GL_PACK_SKIP_ROWS,0);glPixelStorei(GL_PACK_SKIP_PIXELS,0);}
 ~ReadPixelsScope(){glPixelStorei(GL_PACK_ALIGNMENT,alignment);glPixelStorei(GL_PACK_ROW_LENGTH,row);glPixelStorei(GL_PACK_SKIP_ROWS,rows);glPixelStorei(GL_PACK_SKIP_PIXELS,pixels);}
};
bool Context(){if(!IsMain()||CurrentOwner()!=Owner::Render||!g_KAGE_Graphics_Manager._window)return false;
 auto *w=reinterpret_cast<const unsigned char *>(g_KAGE_Graphics_Manager._window);HDC dc=nullptr;HGLRC rc=nullptr;SIZE_T bytes=0;
 return ReadProcessMemory(GetCurrentProcess(),w+0x234,&dc,sizeof(dc),&bytes)&&bytes==sizeof(dc)&&ReadProcessMemory(GetCurrentProcess(),w+0x238,&rc,sizeof(rc),&bytes)&&bytes==sizeof(rc)&&dc&&rc&&dc==wglGetCurrentDC()&&rc==wglGetCurrentContext();}
// 按官方ShaderUtils/ImageUtils的描述符算法适配；只用SDK Image ABI，不引入RGON的GLAD全局加载器。
struct NativeShaderLayout final{
 std::vector<std::uint8_t> formats;unsigned stride=0;
 explicit NativeShaderLayout(const KAGE_Graphics_Shader &shader){
  for(unsigned i=0;i<shader._numVertexAttributes;++i){const auto f=shader._vertexAttributes[i].format;unsigned size=0;
   switch(f){case 1:case 8:size=1;break;case 2:case 7:size=2;break;case 3:case 5:size=3;break;case 4:case 6:size=4;break;default:Fail("original_shader_format");}
   formats.push_back(std::uint8_t(f));stride+=size*sizeof(float);
  }
 }
};
// J273原生Image的三个基础shader嵌在GraphicsManager；官方AllShaders仅列游戏扩展shader。
// 9E3220选择Manager+48/74/A0，均为2c字节Shader对象，值报文仅携带索引。
constexpr unsigned NativeShaderCount=SHADER_MAX+3;
KAGE_Graphics_Shader *NativeShader(unsigned index){
 if(index<SHADER_MAX)return __ptr_g_AllShaders[index];
 if(index>=NativeShaderCount)Fail("original_shader_unsupported");
 return reinterpret_cast<KAGE_Graphics_Shader *>(reinterpret_cast<unsigned char *>(&g_KAGE_Graphics_Manager)+0x48+(index-SHADER_MAX)*0x2c);
}
float *SubmitNative(KAGE_Graphics_ImageBase &image,KAGE_Graphics_Shader &shader,NativeShaderLayout &layout,const SourceQuad &source,const DestinationQuad &dest,const std::vector<float> &captured){
 auto &m=g_KAGE_Graphics_Manager;auto *old_shader=m._shader;auto *old_format=image._vertexFormat;const auto old_count=image._numVertexAttributes;const auto old_predefined=image._predefinedShader;const auto old_stride=image._vertexFormatStride;
 image._predefinedShader=3;image._vertexFormat=layout.formats.data();image._numVertexAttributes=std::uint8_t(layout.formats.size());image._vertexFormatStride=std::uint16_t(layout.stride);m._shader=&shader;
 unsigned color_offset=0;for(unsigned i=0;i<shader._numVertexAttributes&&shader._vertexAttributes[i].format!=6;++i){const auto f=shader._vertexAttributes[i].format;color_offset+=(f==1||f==8)?1:(f==2||f==7)?2:(f==3||f==5)?3:4;}
 if(color_offset+4>layout.stride/4)Fail("original_color_layout");KColor colors[4];for(unsigned i=0;i<4;++i){const auto at=i*(layout.stride/4)+color_offset;colors[i]=KColor(captured[at],captured[at+1],captured[at+2],captured[at+3]);}
 auto *v=image.Render(source,dest,colors[0],colors[1],colors[2],colors[3]);
 m._shader=old_shader;image._predefinedShader=old_predefined;image._vertexFormat=old_format;image._numVertexAttributes=old_count;image._vertexFormatStride=old_stride;return v;
}
struct Owner final{unsigned kind=0;void *object=nullptr;std::string key;};
// native借用仅保留到同帧safe point；在访问前复核run/room/mutation，队列排空后全部清除。
struct SourceImage final{KAGE_Graphics_ImageBase *image=nullptr;unsigned width=0,height=0,logical_width=0,logical_height=0,min=0,mag=0,wrap_s=0,wrap_t=0;std::string name;};
struct Pending final{float *vertices=nullptr;unsigned stride=0,image=0,shader=0;BlendMode blend;Owner owner;KAGE_Graphics_ImageBase *target=nullptr;};
struct Draw final{unsigned stride=0,image=0,shader=0;BlendMode blend;Owner owner;KAGE_Graphics_ImageBase *target=nullptr;std::vector<float> vertices;};
struct Source final{
 bool requested=false,active=false,awaiting=false,returned=false,failed=false,pre_render_prepared=false;Room *room=nullptr;HGLRC context=nullptr;
 KAGE_Graphics_ImageBase *main_target=nullptr;
 OriginalRoomSource source;std::vector<Owner> owners;std::vector<SourceImage> images;std::vector<Draw> draws;std::map<KAGE_Graphics_ImageBase *,unsigned> image_ids;
 Pending pending;unsigned foreign_layers=0,layer_depth=0,excluded=0,unknown=0;std::uint64_t published=0,rejected=0;
 std::string snapshot,error,output_name;J geometry;float width=0,height=0;
};
Source &State(){static auto *s=new Source;return *s;}
// 仅识别J273房间专用资源槽；不依赖名称，不接收报文中的native指针。
KAGE_Graphics_ImageBase *RoomImage(unsigned rva){KAGE_Graphics_ImageBase *image=nullptr;SIZE_T copied=0;
 if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void *>(Base+rva),&image,sizeof(image),&copied)||copied!=sizeof(image))Fail("original_room_image_slot");return image;}
bool RoomTarget(KAGE_Graphics_ImageBase *image,bool &screen){
 if(!image)return false;
 // Shadow按纹理逻辑尺寸，LightOverlay及房间后处理按屏幕正交尺寸，Water V2按自身尺寸。
 for(const auto &slot:std::vector<std::pair<unsigned,bool>>{{0x7e7360,false},{0x7e7398,true},{0x7e7388,false},{0x7e7340,true},{0x7e7330,true},{0x7e73a0,true},{0x7e7380,true},{0x7e7378,true}})
  if(image==RoomImage(slot.first)){screen=slot.second;return true;}return false;
}
unsigned SourceImageId(KAGE_Graphics_ImageBase *image){auto &s=State();auto it=s.image_ids.find(image);if(it!=s.image_ids.end())return it->second;
 if(s.images.size()>=256)Fail("original_images_budget");SourceImage r;r.image=image;r.width=unsigned(image->GetPaddedWidth());r.height=unsigned(image->GetPaddedHeight());r.logical_width=unsigned(image->GetWidth());r.logical_height=unsigned(image->GetHeight());
 r.min=image->_minFilterMode;r.mag=image->_magFilterMode;r.wrap_s=image->_wrapSMode;r.wrap_t=image->_wrapTMode;
 if(!r.width||!r.height||std::uint64_t(r.width)*r.height>8u*1024u*1024u)Fail("original_image_dimensions");
 if(image->_name){const auto name=std::string(image->_name);if(name.size()<512&&name.find("gfx/")!=name.npos&&name.find("0x")==name.npos)r.name=name;}
 const auto id=unsigned(s.images.size());s.images.push_back(std::move(r));s.image_ids.emplace(image,id);return id;
}
// J273 FXLayers的同一76-byte条目被普通FX及Light(type4)/Water(type5)共用。
// 以当前Room真实容器的Image成员归属识别，避免为每个FX pass继续添加Hook。
bool RoomFxImage(KAGE_Graphics_ImageBase *image){auto &fx=State().room->_fxLayers;if(!fx._loaded)return false;std::uintptr_t range[3]{};SIZE_T copied=0;
 if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const unsigned char *>(&fx)+0x18,range,sizeof(range),&copied)||copied!=sizeof(range)||range[1]<range[0]||range[2]<range[1]||(range[1]-range[0])%76||(range[1]-range[0])/76>256)Fail("original_fx_membership");
 for(auto at=range[0];at<range[1];at+=76){KAGE_Graphics_ImageBase *member=nullptr;if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void *>(at+4),&member,sizeof(member),&copied)||copied!=sizeof(member))Fail("original_fx_image");if(member==image)return true;}return false;
}
void RejectSource(const char *reason){auto &s=State();s.failed=true;s.error=reason;s.pending={};}
void Flush(){auto &s=State();if(!s.pending.vertices)return;auto p=std::move(s.pending);s.pending={};if(s.failed)return;
 if(s.draws.size()>=16384||p.stride<7||p.stride>128)Fail("original_draw_budget");
 Draw d;d.stride=p.stride;d.image=p.image;d.shader=p.shader;d.blend=p.blend;d.owner=std::move(p.owner);d.target=p.target;d.vertices.resize(p.stride*4);
 SIZE_T copied=0;if(!ReadProcessMemory(GetCurrentProcess(),p.vertices,d.vertices.data(),d.vertices.size()*sizeof(float),&copied)||copied!=d.vertices.size()*sizeof(float))Fail("original_vertices_unreadable");
 for(float f:d.vertices)if(!std::isfinite(f))Fail("original_nonfinite_vertex");s.draws.push_back(std::move(d));
}
// 从真实当前布局读inside及遮挡；坐标/碰撞/path/rail仍由正式12域模型提供。
void Geometry(Room *room){auto &s=State();s.geometry.SetObject();auto &a=s.geometry.GetAllocator();V cells(rapidjson::kArrayType);
 for(unsigned i=0;i<448;++i){V c(rapidjson::kObjectType);Put(c,"index",V(i),a);
  Put(c,"inside",V(room->IsValidGridIndex(int(i),true)),a);Put(c,"occlusion",V(-1),a);cells.PushBack(std::move(c),a);}
 Put(s.geometry,"cells",std::move(cells),a);Put(s.geometry,"size",VectorValue(Vector(float(room->_gridWidth)*40,float(room->_gridHeight)*40),a),a);
 s.width=g_OrthographicWidth;s.height=g_OrthographicHeight;
}
Owner ImageOwner(KAGE_Graphics_ImageBase *image){auto &s=State();if(s.owners.empty())return {};auto owner=s.owners.back();if(owner.kind)return owner;
 if(s.layer_depth&&!s.foreign_layers)return {8,s.room,{}};
 auto &b=s.room->_backdrop;if(image==b.floorBuffer.image||image==b.floorSurface2.image)return {3,s.room,{}};
 if(image==b.wallsBuffer.image)return {4,s.room,{}};
 if(image==b.waterGfx.image)return {5,s.room,{}};
 if(image==b.gfx.image||image==b.nlfloorGfx.image||image==b.tempBackdropSurface.image)return {8,s.room,{}};
 bool screen=false;if(RoomTarget(image,screen))return {8,s.room,{}};
 if(image==RoomImage(0x7e7390))return {5,s.room,{}};
 // Backdrop形状遮罩与原生纯色quad图像由Room的准备／绘制路径使用。
 if(image==RoomImage(0x7e7370)||image==RoomImage(0x7e743c))return {8,s.room,{}};
 if(RoomFxImage(image))return {8,s.room,{}};
 return {};
}
const char *Role(const Owner &o){if(o.kind==9)return "outline";if(o.kind==3)return "floor";if(o.kind==4)return "wall";if(o.kind==5)return "water";if(o.kind==2)return "top";if(o.kind==6)return "light";return o.key.empty()?"environment":"main";}
void ClearBorrows(){auto &s=State();s.pending={};s.draws.clear();s.images.clear();s.image_ids.clear();s.owners.clear();s.room=nullptr;s.context=nullptr;s.main_target=nullptr;s.pre_render_prepared=false;s.active=s.awaiting=false;}
// 只服务实景E2E的实际Authority framebuffer参考；产品资源仍是逐Image数据，绝非整帧视频。
void ReferenceImage(const std::filesystem::path &path){
 GLint view[4]{},align=0,row=0,rows=0,pixels=0,pbo=0;glGetIntegerv(GL_VIEWPORT,view);glGetIntegerv(GL_PACK_ALIGNMENT,&align);glGetIntegerv(GL_PACK_ROW_LENGTH,&row);glGetIntegerv(GL_PACK_SKIP_ROWS,&rows);glGetIntegerv(GL_PACK_SKIP_PIXELS,&pixels);glGetIntegerv(0x88ED,&pbo);
 if(pbo||view[2]<=0||view[3]<=0||std::uint64_t(view[2])*unsigned(view[3])>8u*1024u*1024u)Fail("original_reference_viewport");
 std::vector<unsigned char> data(std::size_t(view[2])*unsigned(view[3])*4);glPixelStorei(GL_PACK_ALIGNMENT,4);glPixelStorei(GL_PACK_ROW_LENGTH,0);glPixelStorei(GL_PACK_SKIP_ROWS,0);glPixelStorei(GL_PACK_SKIP_PIXELS,0);glFinish();glReadPixels(view[0],view[1],view[2],view[3],0x80E1,GL_UNSIGNED_BYTE,data.data());
 glPixelStorei(GL_PACK_ALIGNMENT,align);glPixelStorei(GL_PACK_ROW_LENGTH,row);glPixelStorei(GL_PACK_SKIP_ROWS,rows);glPixelStorei(GL_PACK_SKIP_PIXELS,pixels);
 BITMAPFILEHEADER h{};h.bfType=0x4d42;h.bfOffBits=sizeof(h)+sizeof(BITMAPINFOHEADER);h.bfSize=h.bfOffBits+DWORD(data.size());BITMAPINFOHEADER i{};i.biSize=sizeof(i);i.biWidth=view[2];i.biHeight=view[3];i.biPlanes=1;i.biBitCount=32;
 std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char *>(&h),sizeof(h));file.write(reinterpret_cast<const char *>(&i),sizeof(i));file.write(reinterpret_cast<const char *>(data.data()),std::streamsize(data.size()));if(!file)Fail("original_reference_write");
}
}

bool RequestOriginalRoomGrid() noexcept{if(!IsMain()||ClientBusy()||!RunActive())return false;auto &s=State();if(s.requested||s.active||s.awaiting)return false;s.requested=true;s.error.clear();s.failed=false;return true;}
std::string OriginalRoomGridSnapshot(){return IsMain()?State().snapshot:std::string{};}
OriginalRoomGridScope::OriginalRoomGridScope(unsigned kind,void *owner) noexcept:kind_(kind){
 auto &s=State();if(!IsMain()||ClientBusy())return;
 try{
  if(kind==0||kind==11){if(!s.requested||s.active||s.awaiting||!g_Game||g_Game->_room!=owner||!Context())return;
   const bool reuse=kind==0&&s.pre_render_prepared&&s.room==owner&&s.context==wglGetCurrentContext()&&OriginalRoomSourceStable(s.source);
   if(!reuse){
   OriginalRoomSource source;if(!ReadOriginalRoomSource(source))return;
   s.source=std::move(source);s.room=static_cast<Room *>(owner);s.context=wglGetCurrentContext();s.failed=false;s.returned=false;s.draws.clear();s.images.clear();s.image_ids.clear();s.owners.clear();s.pending={};s.foreign_layers=s.layer_depth=s.excluded=s.unknown=0;s.pre_render_prepared=false;Geometry(s.room);
   }
   s.active=true;if(kind==0)s.main_target=g_KAGE_Graphics_Manager.currentRenderTarget;
  }else if(!s.active)return;
  if(kind==8&&owner!=&s.room->_fxLayers)return;
  previous_=s.owners.size();Owner o{kind==11?0:kind,owner,{}};
  // 实体嵌套基类调用保留最外层归属；Room拥有的红门轮廓仍是Room展示资源。
  if(kind==7){
   if(!s.owners.empty()&&s.owners.back().kind!=0)o=s.owners.back();
   else for(auto *e:s.room->_redDoorOutlines)if(e==owner)o.kind=9;
  }
  if(kind==1||kind==2||kind==6){o.key=OriginalGridKey(owner);if(o.key.empty())Fail("original_grid_identity");}
  s.owners.push_back(std::move(o));active_=true;
 }catch(const std::exception &e){RejectSource(e.what());if(kind==0||kind==11){s.active=false;s.awaiting=true;}}catch(...){RejectSource("original_scope_failed");if(kind==0||kind==11){s.active=false;s.awaiting=true;}}
}
void OriginalRoomGridScope::Returned() noexcept{if(!active_)return;returned_=true;try{Flush();if(kind_==0)State().returned=true;}catch(const std::exception &e){RejectSource(e.what());}catch(...){RejectSource("original_flush_failed");}}
OriginalRoomGridScope::~OriginalRoomGridScope() noexcept{if(!active_)return;auto &s=State();if(!returned_)RejectSource("original_native_exception");if(s.owners.size()>previous_)s.owners.resize(std::size_t(previous_));if(kind_==0){s.active=false;s.awaiting=true;}else if(kind_==11){s.active=false;s.pre_render_prepared=true;}}
OriginalLayerScope::OriginalLayerScope(void *animation) noexcept{auto &s=State();if(!s.active)return;previous_=s.foreign_layers;active_=true;++s.layer_depth;
 if(s.owners.empty()||s.owners.back().kind==0){bool room_owned=false;auto &b=s.room->_backdrop;
  for(auto *a:{&b.floorANM2,&b._wallDetailANM2,&b._controlsANM2,&b._controlsButtonsANM2})if(a==animation)room_owned=true;
  // 红门轮廓为Room拥有的Effect，作为门展示的一部分；普通Entity交给19。
  for(auto *e:s.room->_redDoorOutlines)if(e&&&e->_sprite==animation)room_owned=true;
  if(!room_owned)++s.foreign_layers;
 }
}
OriginalLayerScope::~OriginalLayerScope() noexcept{if(!active_)return;try{Flush();}catch(const std::exception &e){RejectSource(e.what());}catch(...){RejectSource("original_layer_failed");}auto &s=State();s.foreign_layers=previous_;if(s.layer_depth)--s.layer_depth;}
void OriginalImageBefore() noexcept{auto &s=State();if(!s.active)return;try{Flush();}catch(const std::exception &e){RejectSource(e.what());}catch(...){RejectSource("original_flush_failed");}}
void OriginalImageReturned(void *object,float *vertices) noexcept{
 auto &s=State();if(!s.active||s.failed||!vertices)return;
 try{
  if(!s.owners.empty()&&s.owners.back().kind==7){++s.excluded;return;}
  // 无明确owner的ANM2不能猜作实体而丢弃；整候选拒绝，避免静默漏掉Room特效。
  if(s.foreign_layers||s.owners.empty()){++s.unknown;if(!Settings.world_e2e_token.empty()){auto *image=static_cast<KAGE_Graphics_ImageBase *>(object);Trace("original_unknown_layer","name=%s",image->_name?image->_name:"<unnamed>");}return;}
  auto *image=static_cast<KAGE_Graphics_ImageBase *>(object);auto owner=ImageOwner(image);
  if(!owner.kind){++s.unknown;if(!Settings.world_e2e_token.empty()){auto &b=s.room->_backdrop;Trace("original_unknown_image","name=%s known_backdrop=%p/%p/%p/%p/%p/%p/%p observed=%p",image->_name?image->_name:"<unnamed>",b.floorBuffer.image,b.wallsBuffer.image,b.floorSurface2.image,b.tempBackdropSurface.image,b.gfx.image,b.waterGfx.image,b.nlfloorGfx.image,image);}return;}
  auto *batch=image->_lastRenderBatch;auto *transparent=image->_lastTransparentRenderBatch;
  // J273 Render核心9E2460用+60保存当前批次；坑等flags&10的手动batch不进入last列表。
  KAGE_Graphics_RenderBatch *current=nullptr;SIZE_T copied=0;
  if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const unsigned char *>(image)+0x60,&current,sizeof(current),&copied)||copied!=sizeof(current))Fail("original_current_batch");
  auto contains=[&](KAGE_Graphics_RenderBatch *b){if(!b||!b->_vertexBuffer.container)return false;auto &v=b->_vertexBuffer.container[b->_vertexBuffer.activeBufferIdx];auto at=reinterpret_cast<std::uintptr_t>(vertices),begin=reinterpret_cast<std::uintptr_t>(v.data);return at>=begin&&at<begin+std::size_t(v.capacity)*b->_vertexBuffer.elementSize;};
  if(!contains(batch)){
   batch=contains(current)?current:contains(transparent)?transparent:nullptr;
   if(!batch)for(auto *b:image->_renderBatches)if(contains(b)){batch=b;break;}
   if(!batch)for(auto *b:image->_reusableTransparentRenderBatches)if(contains(b)){batch=b;break;}
   if(!batch){if(!Settings.world_e2e_token.empty())Trace("original_batch_missing","name=%s flags=%llu",image->_name?image->_name:"<unnamed>",image->_flags);Fail("original_batch_owner");}
  }
  unsigned shader=NativeShaderCount;for(unsigned i=0;i<NativeShaderCount;++i)if(NativeShader(i)==batch->shader)shader=i;
  if(shader==NativeShaderCount||!batch->shader||!batch->shader->_initialized)Fail("original_shader_unsupported");
  if(shader==SHADER_MAX+2)Fail("original_palette_material_unsupported");
  // 原版纯色quad只有Position3+Color4；纹理quad才额外带TexCoord2，不能统一要求九项。
  const unsigned stride=batch->_vertexBuffer.elementSize/sizeof(float);if(batch->_vertexBuffer.elementSize%4||stride<7||stride>128||NativeShaderLayout(*batch->shader).stride!=batch->_vertexBuffer.elementSize)Fail("original_vertex_layout");
  const auto id=SourceImageId(image);
  BlendMode blend;SIZE_T blend_bytes=0;if(!ReadProcessMemory(GetCurrentProcess(),batch,&blend,sizeof(blend),&blend_bytes)||blend_bytes!=sizeof(blend))Fail("original_batch_blend");
  s.pending={vertices,stride,id,shader,blend,std::move(owner),g_KAGE_Graphics_Manager.currentRenderTarget};
 }catch(const std::exception &e){RejectSource(e.what());}catch(...){RejectSource("original_image_failed");}
}

void OriginalRoomGridRenderReturned() noexcept{
 auto &s=State();if(!s.awaiting)return;
 try{
  if(!s.returned||s.failed)Fail(s.error.empty()?"original_room_failed":s.error.c_str());
  if(!Context()||wglGetCurrentContext()!=s.context||!g_Game||g_Game->_room!=s.room||!OriginalRoomSourceStable(s.source))Fail("original_scene_changed");
  if(!g_KAGE_Graphics_ImageManager._frameImages.empty()||!g_KAGE_Graphics_ImageManager._transparentBatches.empty())Fail("original_cpu_queue_pending");
  if(s.draws.empty())Fail("original_no_draws");
  if(s.unknown)Fail("original_unclassified_room_primitive");
  if(!s.output_name.empty()){const auto root=std::filesystem::u8path(Settings.world_output_dir)/"original";std::filesystem::create_directories(root);ReferenceImage(root/(s.output_name+"-authority.bmp"));}
  J packet(rapidjson::kObjectType);auto &a=packet.GetAllocator();Put(packet,"format",std::string("isaac.cs.room-grid-original"),a);Put(packet,"version",V(1),a);
  Put(packet,"connection",std::string("1"),a);
  auto model=Parse(s.source.model);Put(packet,"model",V(model,a),a);Put(packet,"geometry",V(s.geometry,a),a);
  Put(packet,"orthographic",VectorValue(Vector(s.width,s.height),a),a);Put(packet,"excluded_entity_draws",V(s.excluded),a);Put(packet,"unclassified_draws",V(s.unknown),a);
  // 读取Authority实际已插值的显示色彩；Client不调用Game::SetColorModifier或Room计算。
  const auto &color=*g_Game->GetCurrentColorModifier();const float values[]={color.r,color.g,color.b,color.a,color.brightness,color.contrast};Put(packet,"color_modifier",FloatValues(values,6,a),a);
  Put(packet,"color_modifier_enabled",V(g_Manager&&g_Manager->GetOptions()->_enableColorModifier&&!g_Game->GetRoomTransition()->IsRenderingBossIntro()),a);
  // 生成目标即使没有可见Room/Grid绘制也须清为透明；其Entity内容不进入资源字节。
  for(const auto &d:s.draws)if(d.target!=s.main_target){bool screen=false;if(!RoomTarget(d.target,screen))Fail("original_render_target_unsupported");SourceImageId(d.target);}
  V images(rapidjson::kArrayType);std::uint64_t total=0;ReadPixelsScope pixel_layout;
  for(unsigned i=0;i<s.images.size();++i){auto &r=s.images[i];total+=std::uint64_t(r.width)*r.height*4;if(total>64u<<20)Fail("original_texture_budget");
   V im(rapidjson::kObjectType);Put(im,"id",V(i),a);Put(im,"width",V(r.width),a);Put(im,"height",V(r.height),a);Put(im,"logical_width",V(r.logical_width),a);Put(im,"logical_height",V(r.logical_height),a);Put(im,"logical_path",r.name,a);
   bool screen=false;if(RoomTarget(r.image,screen)){
    // Shadow/Light/Water复用同一目标重建路径，绝不读回混有Entity的原生表面。
    if(!s.pre_render_prepared)Fail("original_target_not_prepared");Put(im,"encoding",std::string("room-grid-target-v1"),a);Put(im,"target_screen",V(screen),a);
   }else{std::vector<unsigned char> pixels(std::size_t(r.width)*r.height*4);glFinish();r.image->GetTexelRegion(0,0,r.width,r.height,pixels.data());Put(im,"encoding",std::string("rgba8-xpress-huff"),a);Put(im,"data",Hex(Pack(pixels)),a);}
   V sampler(rapidjson::kArrayType);for(auto v:{r.min,r.mag,r.wrap_s,r.wrap_t})sampler.PushBack(v,a);Put(im,"sampler",std::move(sampler),a);images.PushBack(std::move(im),a);
  }Put(packet,"images",std::move(images),a);
  V draws(rapidjson::kArrayType);for(unsigned i=0;i<s.draws.size();++i){const auto &d=s.draws[i];V draw(rapidjson::kObjectType);Put(draw,"id",S(std::to_string(i+1),a),a);
   if(d.target==s.main_target)Put(draw,"target",V(rapidjson::kNullType),a);
   else if(s.image_ids.count(d.target))Put(draw,"target",V(s.image_ids.at(d.target)),a);
   else Fail("original_render_target_unsupported");
   Put(draw,"image",V(d.image),a);Put(draw,"shader",V(d.shader),a);Put(draw,"stride",V(d.stride),a);Put(draw,"role",std::string(Role(d.owner)),a);
   if(d.owner.key.empty())Put(draw,"grid",V(rapidjson::kNullType),a);else{auto k=Parse(d.owner.key);Put(draw,"grid",V(k,a),a);}
   V blend(rapidjson::kArrayType);for(auto v:{d.blend._equation,d.blend._rgbSourceFactor,d.blend._rgbDestFactor,d.blend._alphaSourceFactor,d.blend._alphaDestFactor})blend.PushBack(v,a);
   Put(draw,"blend",std::move(blend),a);Put(draw,"vertices",FloatValues(d.vertices.data(),d.vertices.size(),a),a);draws.PushBack(std::move(draw),a);
  }Put(packet,"draws",std::move(draws),a);
  auto snapshot=Text(packet);if(snapshot.size()>32u<<20)Fail("original_packet_budget");s.snapshot=std::move(snapshot);++s.published;s.error.clear();s.requested=false;
  if(!s.output_name.empty()){const auto root=std::filesystem::u8path(Settings.world_output_dir)/"original";std::filesystem::create_directories(root);std::ofstream f(root/(s.output_name+".json"),std::ios::binary);f<<s.snapshot;if(!f)Fail("original_export_write");s.output_name.clear();}
 }catch(const std::exception &e){s.error=e.what();++s.rejected;s.requested=false;}catch(...){s.error="original_readback_failed";++s.rejected;s.requested=false;}
 ClearBorrows();
}

// 同一个私有Scene拥有全部native smartpointer。析构只由17的队列排空/GL完成退休路径触发。
struct OriginalRoomGraphics final{
 struct Command{unsigned image=0,shader=0,stride=0;int target=-1;BlendMode blend;std::vector<float> vertices;};
 std::vector<KAGE_SmartPointer_ImageBase> images;std::vector<Command> commands;
 std::map<unsigned,bool> generated;
 KAGE_SmartPointer_ImageBase composite;std::vector<float> color;
 float width=0,height=0;
};
std::shared_ptr<OriginalRoomGraphics> PrepareOriginalRoomGraphics(const std::string &descriptor){
 if(!Context())Fail("original_graphics_context");auto doc=Parse(descriptor);auto result=std::make_shared<OriginalRoomGraphics>();
 static std::uint64_t serial=0;auto *color_shader=NativeShader(SHADER_COLOR_MOD);if(!color_shader||!color_shader->_initialized||NativeShaderLayout(*color_shader).stride!=12*sizeof(float))Fail("original_color_shader_layout");
 const auto &ortho=At(doc,"orthographic");if(!ortho.IsArray()||ortho.Size()!=2||!ortho[0].IsNumber()||!ortho[1].IsNumber())Fail("original_orthographic");
 result->width=float(ortho[0].GetDouble());result->height=float(ortho[1].GetDouble());if(!std::isfinite(result->width)||!std::isfinite(result->height)||result->width<=0||result->height<=0)Fail("original_orthographic");
 const auto &color=At(doc,"color_modifier");if(!color.IsArray()||color.Size()!=6)Fail("original_room_color");for(const auto &v:color.GetArray()){if(!v.IsNumber()||!std::isfinite(v.GetDouble())||std::abs(v.GetDouble())>1000000)Fail("original_room_color");result->color.push_back(float(v.GetDouble()));}
 // v1早期快照隐式启用；显式关闭时使用ColorModState的中性值，不修改本地Game。
 if(doc.HasMember("color_modifier_enabled")){if(!doc["color_modifier_enabled"].IsBool())Fail("original_room_color_enabled");if(!doc["color_modifier_enabled"].GetBool())result->color={1,1,1,0,0,1};}
 GLint viewport[4]{};glGetIntegerv(GL_VIEWPORT,viewport);if(viewport[2]<=0||viewport[3]<=0||std::uint64_t(viewport[2])*unsigned(viewport[3])>8u*1024u*1024u)Fail("original_composite_dimensions");
 KColor empty(0,0,0,0);const auto composite_name="isaac.cs.room-grid-composite."+std::to_string(++serial);result->composite=KAGE_Graphics_ImageManager::CreateProceduralImage(unsigned(viewport[2]),unsigned(viewport[3]),composite_name.c_str(),empty);if(!result->composite.image)Fail("original_composite_create");
 const auto &images=At(doc,"native_images");if(!images.IsArray()||images.Size()>256)Fail("original_images_budget");std::uint64_t total=std::uint64_t(result->composite.image->GetPaddedWidth())*unsigned(result->composite.image->GetPaddedHeight())*4;
 for(const auto &im:images.GetArray()){
  const auto encoding=Str(At(im,"encoding"));const bool generated=encoding=="room-grid-target-v1";
  if((encoding!="rgba8-xpress-huff"&&!generated)||U(At(im,"id"),256)!=result->images.size())Fail("original_image_identity");
  const unsigned width=U(At(im,"width"),8193),height=U(At(im,"height"),8193);if(!width||!height||std::uint64_t(width)*height>8u*1024u*1024u)Fail("original_image_dimensions");
  auto raw=generated?std::vector<unsigned char>{}:Unpack(im);total+=std::uint64_t(width)*height*4;if(total>64u<<20)Fail("original_texture_budget");
  KColor transparent(0,0,0,0);const auto name="isaac.cs.original."+std::to_string(GetCurrentProcessId())+"."+std::to_string(++serial);
  const auto logical_width=im.HasMember("logical_width")?U(im["logical_width"],width+1):width,logical_height=im.HasMember("logical_height")?U(im["logical_height"],height+1):height;if(!logical_width||!logical_height)Fail("original_image_dimensions");
  auto owned=KAGE_Graphics_ImageManager::CreateProceduralImage(logical_width,logical_height,name.c_str(),transparent);
  if(!owned.image||!owned.counter||owned.image->GetPaddedWidth()!=int(width)||owned.image->GetPaddedHeight()!=int(height))Fail("original_image_create");
  // J273私有布局由SetTexel/Unload核定；只访问本方新建的format=2图像，不接收报文中的GL名。
  const auto *object=reinterpret_cast<const unsigned char *>(owned.image);unsigned format=0,texture=0;SIZE_T copied=0;
  if(!ReadProcessMemory(GetCurrentProcess(),object+0x18,&format,4,&copied)||copied!=4||format!=2||!ReadProcessMemory(GetCurrentProcess(),object+0x78,&texture,4,&copied)||copied!=4||!texture)Fail("original_owned_image_layout");
  GLint binding=0,alignment=0,row=0,rows=0,pixels=0,pbo=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding);glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);glGetIntegerv(GL_UNPACK_ROW_LENGTH,&row);glGetIntegerv(GL_UNPACK_SKIP_ROWS,&rows);glGetIntegerv(GL_UNPACK_SKIP_PIXELS,&pixels);glGetIntegerv(0x88EF,&pbo);if(pbo)Fail("original_unpack_pbo");
  glBindTexture(GL_TEXTURE_2D,texture);glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  if(!generated)glTexSubImage2D(GL_TEXTURE_2D,0,0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,raw.data());
  glBindTexture(GL_TEXTURE_2D,binding);glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);glPixelStorei(GL_UNPACK_ROW_LENGTH,row);glPixelStorei(GL_UNPACK_SKIP_ROWS,rows);glPixelStorei(GL_UNPACK_SKIP_PIXELS,pixels);
  const auto &sampler=At(im,"sampler");if(!sampler.IsArray()||sampler.Size()!=4)Fail("original_sampler");
  owned.image->_minFilterMode=U(sampler[0],65536);owned.image->_magFilterMode=U(sampler[1],65536);owned.image->_wrapSMode=U(sampler[2],65536);owned.image->_wrapTMode=U(sampler[3],65536);
  // J273 SetFilterMode/SetWrapMode(A4C590/A4C600)会立即写driver；只改CPU成员不会生效。
  glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,owned.image->_minFilterMode?GL_LINEAR:GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,owned.image->_magFilterMode?GL_LINEAR:GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,owned.image->_wrapSMode?0x812F:GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,owned.image->_wrapTMode?0x812F:GL_REPEAT);glBindTexture(GL_TEXTURE_2D,binding);
  if(generated){if(!im.HasMember("target_screen")||!im["target_screen"].IsBool())Fail("original_target_screen");result->generated.emplace(unsigned(result->images.size()),im["target_screen"].GetBool());}result->images.push_back(std::move(owned));
 }
 const auto &draws=At(doc,"native_draws");if(!draws.IsArray()||draws.Size()>16384)Fail("original_draw_budget");std::set<unsigned> completed,written;int previous_target=-1;
 for(const auto &d:draws.GetArray())if(d.HasMember("target")&&!d["target"].IsNull())written.insert(U(d["target"],unsigned(result->images.size())));
 for(const auto &value:draws.GetArray()){OriginalRoomGraphics::Command c;c.image=U(At(value,"image"),unsigned(result->images.size()));c.shader=U(At(value,"shader"),NativeShaderCount);c.stride=U(At(value,"stride"),129);
  if(value.HasMember("target")&&!value["target"].IsNull()){c.target=int(U(value["target"],unsigned(result->images.size())));if(!result->generated.count(unsigned(c.target))||unsigned(c.target)==c.image)Fail("original_target_dependency");}
  // 每个生成目标只写一个连续阶段，采样前必须完成；不允许跨帧残留、读写反馈或环。
  if(c.target!=previous_target){if(previous_target>=0)completed.insert(unsigned(previous_target));if(c.target>=0&&completed.count(unsigned(c.target)))Fail("original_target_reentered");previous_target=c.target;}
  if(result->generated.count(c.image)&&written.count(c.image)&&!completed.count(c.image))Fail("original_target_not_ready");
  auto *shader=NativeShader(c.shader);if(c.shader==SHADER_MAX+2)Fail("original_palette_material_unsupported");
  if(!shader||!shader->_initialized||!shader->_glProgram||shader->_glProgram==unsigned(-1)||NativeShaderLayout(*shader).stride!=c.stride*sizeof(float))Fail("original_shader_layout");
  const auto &vertices=At(value,"vertices");if(!vertices.IsArray()||vertices.Size()!=c.stride*4)Fail("original_vertex_layout");
  for(const auto &v:vertices.GetArray()){if(!v.IsNumber()||!std::isfinite(v.GetDouble()))Fail("original_nonfinite_vertex");c.vertices.push_back(float(v.GetDouble()));}
  const auto &blend=At(value,"blend");if(!blend.IsArray()||blend.Size()!=5)Fail("original_blend");
  c.blend=BlendMode(int(U(blend[0],unsigned(eBlendEquation::NUM_BLEND_EQUATIONS))),int(U(blend[1],unsigned(eBlendFactor::NUM_BLEND_FACTORS))),int(U(blend[2],unsigned(eBlendFactor::NUM_BLEND_FACTORS))),int(U(blend[3],unsigned(eBlendFactor::NUM_BLEND_FACTORS))),int(U(blend[4],unsigned(eBlendFactor::NUM_BLEND_FACTORS))));result->commands.push_back(std::move(c));
 }return result;
}
void DrawOriginalRoomGraphics(OriginalRoomGraphics &scene){
 if(!Context())Fail("original_draw_context");auto &manager=g_KAGE_Graphics_Manager;const auto old=manager._blendMode;
 auto *old_target=manager.currentRenderTarget;const bool old_screen=manager.renderTargetUsesScreenSize;auto *old_shader=manager._shader;int target=-1;std::set<unsigned> cleared;bool offscreen=true;
 manager.SetCurrentRenderTarget(scene.composite.image,true);GLfloat previous_clear[4]{};glGetFloatv(GL_COLOR_CLEAR_VALUE,previous_clear);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glClearColor(previous_clear[0],previous_clear[1],previous_clear[2],previous_clear[3]);
 // 官方RenderToSurface使用Present：目标非空时只提交并清深度，不交换窗口。
 auto restore=[&](){if(offscreen){manager.Present();manager.SetCurrentRenderTarget(old_target,old_screen);}manager._blendMode=old;manager._shader=old_shader;};
 try{for(const auto &c:scene.commands){
   if(c.target!=target){manager.Present();target=c.target;manager.SetCurrentRenderTarget(target<0?scene.composite.image:scene.images[unsigned(target)].image,target<0?true:scene.generated.at(unsigned(target)));
    if(target>=0&&cleared.insert(unsigned(target)).second){GLfloat clear[4]{};glGetFloatv(GL_COLOR_CLEAR_VALUE,clear);glClearColor(0,0,0,0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glClearColor(clear[0],clear[1],clear[2],clear[3]);}}
   auto &image=*scene.images[c.image].image;auto *shader=NativeShader(c.shader);NativeShaderLayout layout(*shader);SourceQuad source{};DestinationQuad destination{};source._topRight=destination._topRight=Vector(1,0);source._bottomLeft=destination._bottomLeft=Vector(0,1);source._bottomRight=destination._bottomRight=Vector(1,1);manager._blendMode=c.blend;
   auto *vertices=SubmitNative(image,*shader,layout,source,destination,c.vertices);if(!vertices)Fail("original_draw_vertices");std::memcpy(vertices,c.vertices.data(),c.vertices.size()*sizeof(float));
   // Source和Client的原生目标可使用不同正交尺寸；只转换自有顶点，屏幕比例保持一致。
   if(target<0||scene.generated.at(unsigned(target)))for(unsigned v=0;v<4;++v){vertices[v*c.stride]*=g_OrthographicWidth/scene.width;vertices[v*c.stride+1]*=g_OrthographicHeight/scene.height;}}
  // 仅一个房间专用后处理步骤，复用原版SHADER_COLOR_MOD的12-float布局。
  // J273 Game.Render同样用RGB/1、Amount、Brightness、Contrast；这里只画本方组合图。
  manager.Present();manager.SetCurrentRenderTarget(old_target,old_screen);offscreen=false;manager._blendMode=BlendMode(0,1,0,1,0);
  auto &image=*scene.composite.image;auto *shader=NativeShader(SHADER_COLOR_MOD);NativeShaderLayout layout(*shader);if(layout.stride!=12*sizeof(float))Fail("original_color_shader_layout");
  const float w=g_OrthographicWidth,h=g_OrthographicHeight,u=float(image.GetWidth())/image.GetPaddedWidth(),v=float(image.GetHeight())/image.GetPaddedHeight();
  std::vector<float> post;for(unsigned i=0;i<4;++i){const float vertex[]={i%2?w:0,i/2?h:0,-1,scene.color[0],scene.color[1],scene.color[2],1,i%2?u:0,i/2?v:0,scene.color[3],scene.color[4],scene.color[5]};post.insert(post.end(),vertex,vertex+12);}
  SourceQuad source{};DestinationQuad dest{};source._topRight=dest._topRight=Vector(1,0);source._bottomLeft=dest._bottomLeft=Vector(0,1);source._bottomRight=dest._bottomRight=Vector(1,1);auto *vertices=SubmitNative(image,*shader,layout,source,dest,post);if(!vertices)Fail("original_color_vertices");std::memcpy(vertices,post.data(),post.size()*sizeof(float));
 }catch(...){restore();throw;}restore();
}
unsigned OriginalGraphicsImages(const OriginalRoomGraphics &s) noexcept{return unsigned(s.images.size())+unsigned(s.composite.image!=nullptr);}
unsigned OriginalGraphicsDraws(const OriginalRoomGraphics &s) noexcept{return unsigned(s.commands.size());}
std::string OriginalRoomGridFields(){auto &s=State();std::ostringstream o;o<<"original.requested="<<s.requested<<"\noriginal.active="<<s.active<<"\noriginal.published="<<s.published<<"\noriginal.rejected="<<s.rejected<<"\noriginal.error="<<s.error<<"\n";return o.str()+OriginalRoomSourceFields();}
std::string OriginalRoomGridTest(const std::string &token,const std::string &action){
 if(!IsMain()||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token)return "ok=0\nerror=original_test_disabled\n";
 if(action=="status")return "ok=1\n"+OriginalRoomGridFields();
 if(action.rfind("action-",0)==0&&!ClientBusy()){
  auto *L=g_LuaEngine?g_LuaEngine->_state:nullptr;if(!L)return "ok=0\nerror=original_fixture_unavailable\n";
  const int top=lua_gettop(L);lua_getglobal(L,"IsaacCS_OriginalAction");lua_pushlstring(L,action.data()+7,action.size()-7);
  if(!lua_isfunction(L,-2)||lua_pcall(L,1,1,0)!=LUA_OK){const auto *error=lua_tostring(L,-1);const std::string reason=error?error:"original_fixture_error";lua_settop(L,top);return "ok=0\nerror="+reason.substr(0,256)+"\n";}
  std::size_t n=0;const char *value=lua_tolstring(L,-1,&n);const std::string result=value?std::string(value,n):"{}";lua_settop(L,top);return "ok=1\noriginal.fixture="+result+"\n";
 }
 if(action.rfind("capture-",0)==0&&action.size()<64&&action.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")==action.npos){
  if(!RequestOriginalRoomGrid())return "ok=0\nerror=original_not_ready\n";State().output_name=action.substr(8);return "ok=1\n";
 }return "ok=0\nerror=original_test_action\n";
}
} // namespace cs::game
