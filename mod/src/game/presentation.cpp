#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/presentation.h"
#include "game/client.h"
#include "game/native_room_grid.h"
#include "game/mod_persistence.h"
#include "game/bridge.h"
#include "replica/room_grid.h"
#include "runtime.h"
#include <GL/gl.h>
#include <rapidjson/document.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

namespace cs::game {
namespace {
using Value=rapidjson::Value;
// 此处只保存本方所有权；不向Lua暴露registry root，不把borrowed userdata留给下一次调用。
struct Sprite final {
  ANM2 *value=nullptr;
  explicit Sprite(ANM2 *v):value(v){}
  Sprite(const Sprite &)=delete;
  ~Sprite() { if(value){value->destructor();::operator delete(value);} }
};
struct Node final {
  std::uint64_t id=0;
  unsigned pass=0;
  Vector position{0,0};
  bool screen=false;
  float order=0;
  Vector clamp_top{0,0},clamp_bottom{0,0};
  std::string text;
  Vector text_scale{1,1};
  KColor text_color{1,1,1,1};
  std::unique_ptr<Sprite> sprite;
};
// Room与Camera是独立呈现模型，不构造会使用共享RNG和玩法回调的原生世界。
struct Scene final {
  std::uint64_t epoch=0,revision=0;
  Vector size{0,0},camera{0,0};
  float zoom=1;
  std::vector<Node> nodes;
  std::string hud,descriptor;
  std::uint64_t drawn=0;
  std::unique_ptr<replica::RoomGrid> room_grid;
};
struct Service final {
  std::uint64_t connection=0,revision=0,epoch=0,prepared=0,published=0,rejected=0,
    draws=0,retired=0,context_checks=0,context_misses=0,applies=0,resets=0,
    draw_apply=0,draw_reset=0,drains=0,drain_waits=0;
  bool active=false,stopping=false,drawing=false,render_pending=false;
  std::uint64_t rng_checks=0,rng_changes=0;
  std::string capture;
  std::uint64_t captures=0;
  HDC dc=nullptr;
  HGLRC context=nullptr;
  std::string pending,error;
  std::vector<unsigned char> progress;
  bool progress_changed=false;
  std::unique_ptr<Scene> current;
  // 同时最多两个候选／旧代次，整个描述及纹理预算另行检查。
  std::vector<std::unique_ptr<Scene>> retiring;
};
// 进程退出不让CRT在context销毁后隐式析构资源；正常退休必须显式排空。
Service &State(){static auto *state=new Service;return *state;}
// 只在显式E2E动作后向临时夹具提供；关闭窗口保护仍在时可测试隐藏的原生保存入口。
int InternalPersistenceProbe(lua_State *L){
  const bool result=!Settings.world_e2e_token.empty()&&TestInternalModPersistence();lua_pushboolean(L,result);return 1;
}
[[noreturn]] void Reject(const char *s){throw std::runtime_error(s);}
const Value &Member(const Value &v,const char *key){
  if(!v.IsObject()||!v.HasMember(key))Reject("presentation_missing_member");return v[key];
}
float Number(const Value &v){if(!v.IsNumber()||!std::isfinite(v.GetDouble())||std::abs(v.GetDouble())>1000000)Reject("presentation_number");return float(v.GetDouble());}
std::uint64_t Id(const Value &v){if(!v.IsString()||!v.GetStringLength()||v.GetStringLength()>20||(v.GetStringLength()>1&&v.GetString()[0]=='0'))Reject("presentation_identity");std::uint64_t n=0;for(char c:std::string(v.GetString(),v.GetStringLength())){if(c<'0'||c>'9'||n>(UINT64_MAX-unsigned(c-'0'))/10)Reject("presentation_identity");n=n*10+unsigned(c-'0');}if(!n)Reject("presentation_identity");return n;}
std::string String(const Value &v,unsigned limit=256){if(!v.IsString()||v.GetStringLength()>limit||std::string(v.GetString(),v.GetStringLength()).find('\0')!=std::string::npos)Reject("presentation_string");return {v.GetString(),v.GetStringLength()};}
std::string Path(const Value &v){auto p=String(v);if(p.empty()||p.front()=='/'||p.find(':')!=p.npos||p.find("..")!=p.npos||p.find('\\')!=p.npos)Reject("presentation_resource_path");return p;}
Vector Vec(const Value &v){if(!v.IsArray()||v.Size()!=2)Reject("presentation_vector");return Vector(Number(v[0]),Number(v[1]));}
ColorMod Color(const Value &v){if(!v.IsArray()||v.Size()!=11)Reject("presentation_color");return ColorMod(Number(v[0]),Number(v[1]),Number(v[2]),Number(v[3]),Number(v[4]),Number(v[5]),Number(v[6]),Number(v[7]),Number(v[8]),Number(v[9]),Number(v[10]));}
unsigned Index(const Value &v,unsigned limit){if(!v.IsUint()||v.GetUint()>=limit)Reject("presentation_index");return v.GetUint();}
bool Flag(const Value &v){if(!v.IsBool())Reject("presentation_flag");return v.GetBool();}

// 仅查询OS当前context，绝不主动wglMakeCurrent修复外部线程/context错误。
bool Graphics() noexcept {
  auto &s=State();++s.context_checks;
  if(!IsMain()||CurrentOwner()!=Owner::Render||!g_KAGE_Graphics_Manager._window){++s.context_misses;return false;}
  const auto *window=reinterpret_cast<const unsigned char *>(g_KAGE_Graphics_Manager._window);
  HDC expected_dc=nullptr;HGLRC expected_context=nullptr;
  SIZE_T copied=0;
  if(!ReadProcessMemory(GetCurrentProcess(),window+0x234,&expected_dc,sizeof(expected_dc),&copied)||copied!=sizeof(expected_dc)||
     !ReadProcessMemory(GetCurrentProcess(),window+0x238,&expected_context,sizeof(expected_context),&copied)||copied!=sizeof(expected_context)||
     !expected_dc||!expected_context||wglGetCurrentDC()!=expected_dc||wglGetCurrentContext()!=expected_context) {
    ++s.context_misses;return false;
  }
  if(s.context&&(s.context!=expected_context||s.dc!=expected_dc)) {s.error="presentation_context_changed";++s.context_misses;return false;}
  s.dc=expected_dc;s.context=expected_context;return true;
}
bool DrawRandom(std::uint32_t &value) noexcept {
  SIZE_T copied=0;return IsMain()&&ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void *>(Base+0x7aa104),&value,sizeof(value),&copied)&&copied==sizeof(value);
}
// E2E直接读取实际native提交后的当前color target，不激活窗口、不依赖被遮挡的桌面像素。
void CaptureTarget(){
  auto &s=State();if(s.capture.empty())return;
  GLint viewport[4]{},alignment=0,row=0,rows=0,pixels=0,pbo=0;
  glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_PACK_ALIGNMENT,&alignment);glGetIntegerv(GL_PACK_ROW_LENGTH,&row);
  glGetIntegerv(GL_PACK_SKIP_ROWS,&rows);glGetIntegerv(GL_PACK_SKIP_PIXELS,&pixels);glGetIntegerv(0x88ED,&pbo);
  const int width=viewport[2],height=viewport[3];
  if(pbo||width<=0||height<=0||std::uint64_t(width)*unsigned(height)>8u*1024u*1024u)Reject("presentation_capture_target");
  std::vector<unsigned char> data(std::size_t(width)*unsigned(height)*4);
  glPixelStorei(GL_PACK_ALIGNMENT,4);glPixelStorei(GL_PACK_ROW_LENGTH,0);glPixelStorei(GL_PACK_SKIP_ROWS,0);glPixelStorei(GL_PACK_SKIP_PIXELS,0);
  glFinish();glReadPixels(viewport[0],viewport[1],width,height,0x80E1,GL_UNSIGNED_BYTE,data.data());
  glPixelStorei(GL_PACK_ALIGNMENT,alignment);glPixelStorei(GL_PACK_ROW_LENGTH,row);glPixelStorei(GL_PACK_SKIP_ROWS,rows);glPixelStorei(GL_PACK_SKIP_PIXELS,pixels);
  BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+DWORD(data.size());
  BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=width;info.biHeight=height;info.biPlanes=1;info.biBitCount=32;info.biSizeImage=DWORD(data.size());
  const auto path=std::filesystem::u8path(Settings.world_output_dir)/"presentation"/(s.capture+".bmp");
  // Authority参考截图也走此入口，不能依赖Client夹具预先创建输出目录。
  std::filesystem::create_directories(path.parent_path());
  std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char *>(&header),sizeof(header));file.write(reinterpret_cast<const char *>(&info),sizeof(info));
  file.write(reinterpret_cast<const char *>(data.data()),std::streamsize(data.size()));if(!file)Reject("presentation_capture_write");
  s.capture.clear();++s.captures;
}
// 场景拥有当前pass的背景，避免冻结前的菜单像素残留在Replica空白区域。
// 只清已由原生prelude绑定的color target，完整恢复GL clear/mask/scissor状态。
void ClearScene(){
  GLfloat color[4]{};GLboolean mask[4]{};glGetFloatv(GL_COLOR_CLEAR_VALUE,color);glGetBooleanv(GL_COLOR_WRITEMASK,mask);
  const auto scissor=glIsEnabled(GL_SCISSOR_TEST);if(scissor)glDisable(GL_SCISSOR_TEST);
  glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(color[0],color[1],color[2],color[3]);glColorMask(mask[0],mask[1],mask[2],mask[3]);
  if(scissor)glEnable(GL_SCISSOR_TEST);
}

// 调用安装版公开Sprite/LayerState shader方法。临时UserdataPtr只借用本方候选，
// pcall返回后立即撤销；不产生Image userdata，也不依赖GC释放native资源。
void Shader(lua_State *L,void *object,bool layer,const char *method,const std::string *path,int expected=-1) {
  if(!L)Reject("presentation_lua_unavailable");
  const int top=lua_gettop(L);
  if(layer){*static_cast<void **>(lua_newuserdata(L,sizeof(void *)))=object;luaL_setmetatable(L,lua::metatables::LayerStateMT);}
  else lua::luabridge::UserdataPtr::push(L,object,lua::Metatables::SPRITE);
  lua_getfield(L,-1,method);lua_pushvalue(L,-2);
  if(path)lua_pushlstring(L,path->data(),path->size());
  if(lua_pcall(L,path?2:1,expected<0?0:1,0)!=LUA_OK){lua_settop(L,top);Reject("presentation_shader_failed");}
  if(expected>=0&&(!lua_isboolean(L,-1)||(lua_toboolean(L,-1)!=0)!=(expected!=0))){lua_settop(L,top);Reject("presentation_shader_association");}
  lua_settop(L,top);
}
void Shaders(void *object,bool layer,const Value &v){
  auto *L=g_LuaEngine?g_LuaEngine->_state:nullptr;
  for(const auto *key:{"custom_shader","champion_shader"}) {
    const auto &p=Member(v,key);const bool champion=key[0]=='c'&&key[1]=='h';
    const char *set=champion?"SetCustomChampionShader":"SetCustomShader";
    const char *clear=champion?"ClearCustomChampionShader":"ClearCustomShader";
    const char *has=champion?"HasCustomChampionShader":"HasCustomShader";
    if(p.IsNull()){Shader(L,object,layer,clear,nullptr);Shader(L,object,layer,has,nullptr,0);}
    else{auto path=Path(p);Shader(L,object,layer,set,&path);Shader(L,object,layer,has,&path,1);}
  }
}
AnimationData *Animation(ANM2 *sprite,const Value &name,const Value &frame,bool overlay){
  const auto n=String(name);const auto found=sprite->GetAnimationByName(n);
  if(!found)Reject("presentation_animation_missing");
  auto *data=sprite->GetAnimationData(int(*found));
  if(!data||data->GetLength()<=0)Reject("presentation_animation_empty");
  if(data->_layerCount<0||data->_layerCount>64||(data->_layerCount&&!data->_animLayers))Reject("presentation_animation_tracks");
  std::set<int> ids;
  for(int i=0;i<data->_layerCount;++i){const auto &track=data->_animLayers[i];const int id=track._layerID;
    if(id<0||unsigned(id)>=sprite->_layerCount||!ids.insert(id).second||!sprite->_layerState[id]._layerData||
       sprite->_layerState[id].GetLayerID()!=id)Reject("presentation_native_render_layer_layout");
    if(!track._animFrames||track._numFrames<=0||track._numFrames>65536)Reject("presentation_animation_track_frames");
  }
  Index(frame,unsigned(data->GetLength()));
  if(overlay)sprite->SetOverlayFrame(n.c_str(),int(frame.GetUint()));
  else{std_string text=n;sprite->SetFrame(&text,int(frame.GetUint()));}
  return data;
}
struct ResolvedLayer final {LayerState *value=nullptr;unsigned ordinal=0;};
// native GetLayer/换图参数是数组ordinal，真实ID另在LayerData；越界getter返回非空sentinel。
ResolvedLayer Layer(ANM2 *sprite,unsigned id){
  if(id>=64||!sprite->_layerState||sprite->_layerCount>64)Reject("presentation_layer_missing");
  ResolvedLayer found{};
  for(unsigned i=0;i<sprite->_layerCount;++i){auto &layer=sprite->_layerState[i];
    if(!layer._layerData)continue;
    if(layer.GetLayerID()==int(id)){if(found.value)Reject("presentation_layer_duplicate");found={&layer,i};}
  }
  if(!found.value)Reject("presentation_layer_missing");return found;
}
// 稀疏ID不能传进原生SetLayerFrame；其错误map分支只log仍写数组。
// 仅在本方候选的已验证selected数组上复刻404030的end-exclusive选择，不写共享帧cache。
void SelectLayer(AnimationState *state,ANM2 *sprite,AnimationData *data,unsigned id,const Value &frame,const Value *key){
  if(!data||state->_animation!=sprite||state->_animData!=data||data->_layerCount<0||data->_layerCount>64)Reject("presentation_layer_state");
  if(data->_layerCount==0){if(frame.IsNull()&&(!key||key->IsNull()))return;Reject("presentation_layer_track_missing");}
  if(!state->_layerFrames||!data->_animLayers)Reject("presentation_layer_state");
  int order=-1;AnimationLayer *track=nullptr;
  for(int i=0;i<data->_layerCount;++i)if(data->_animLayers[i]._layerID==int(id)){
    if(track)Reject("presentation_track_duplicate");track=&data->_animLayers[i];order=i;
  }
  if(!track){if(frame.IsNull()&&!key)return;Reject("presentation_layer_track_missing");}
  // 既有native/RGON RenderFrame按track ID索引Sprite状态数组；先核对其实际可达布局。
  if(id>=sprite->_layerCount||!sprite->_layerState[id]._layerData||sprite->_layerState[id].GetLayerID()!=int(id))Reject("presentation_native_render_layer_layout");
  if(track->_numFrames==0&&frame.IsNull()&&(!key||key->IsNull())){state->_layerFrames[order]=-1;return;}
  if(!track->_animFrames||track->_numFrames<=0||track->_numFrames>65536)Reject("presentation_layer_track_missing");
  if(key){
    if(key->IsNull()){state->_layerFrames[order]=-1;return;}
    state->_layerFrames[order]=int(Index(*key,unsigned(track->_numFrames)));return;
  }
  if(frame.IsNull()){state->_layerFrames[order]=-1;return;}
  const float time=Number(frame);if(time<0||time>=float(data->GetLength()))Reject("presentation_layer_time");
  int selected=0,previous_end=0;
  for(int i=0;i<track->_numFrames;++i){const auto &f=track->_animFrames[i];
    if(f.startFrame<0||f.endFrame<f.startFrame||f.startFrame<previous_end)Reject("presentation_track_timeline");
    previous_end=f.endFrame;
    if(time>=float(f.endFrame)&&i+1<track->_numFrames)selected=i+1;
  }
  if(time<float(track->_animFrames[selected].startFrame))Reject("presentation_track_gap_requires_key");
  state->_layerFrames[order]=selected;
}
// 所有层先核ID、轨道与时间帧，再调用原生setter；不采用GetFrameData的clamp作为合法性证明。
// STATIC/GOLDEN的官方renderer读本地Game/Manager时间；以Authority给定shader及colorize相位替代。
unsigned FinalFlags(const Value &v,const Value &owner,bool random_uv){
  if(!v.IsUint())Reject("presentation_flags");const auto flags=v.GetUint();
  if((flags&2)&&!random_uv)Reject("presentation_unresolved_random_uv");
  if(flags&(32u|128u)){
    if(!owner.HasMember("resolved_time_effect")||!Flag(owner["resolved_time_effect"])||Member(owner,"custom_shader").IsNull())Reject("presentation_unresolved_time_effect");
  }
  return flags&~(2u|32u|64u|128u);
}
void Layers(ANM2 *sprite,AnimationData *main,AnimationData *overlay,const Value &layers,bool random_uv){
  if(!layers.IsArray()||layers.Size()>64)Reject("presentation_layers_budget");
  std::set<unsigned> seen;
  for(const auto &p:layers.GetArray()){
    const auto id=Index(Member(p,"id"),64);if(!seen.insert(id).second)Reject("presentation_layer_duplicate");
    auto *layer=Layer(sprite,id).value;
    for(unsigned o=0;o<2;++o){const char *key=o?"overlay_frame":"frame";const auto &f=Member(p,key);
      auto *data=o?overlay:main;const char *explicit_key=o?"overlay_key":"main_key";
      if(!data){if(!f.IsNull()||p.HasMember(explicit_key))Reject("presentation_overlay_absent");continue;}
      SelectLayer(o?sprite->GetOverlayAnimationState():sprite->GetAnimationState(),sprite,data,id,f,p.HasMember(explicit_key)?&p[explicit_key]:nullptr);
    }
    layer->_visible=Flag(Member(p,"visible"));layer->_pos=Vec(Member(p,"offset"));layer->_size=Vec(Member(p,"scale"));
    layer->_rotation=Number(Member(p,"rotation"));layer->_color=Color(Member(p,"color"));
    layer->_flipX=Flag(Member(p,"flip_x"));layer->_flipY=Flag(Member(p,"flip_y"));
    layer->_cropOffset=Vec(Member(p,"crop"));const auto &flags=Member(p,"flags");if(!flags.IsUint())Reject("presentation_layer_flags");
    // GLITCH会在GetSourceQuad前进全局RNG。只接受Authority已经bake到crop的UV结果，
    // native随机位清零；未给定结果时拒绝整个候选，不能在Client重抽。
    layer->_bitflags=FinalFlags(flags,p,random_uv);
    const auto &blend=Member(p,"blend");if(!blend.IsArray()||blend.Size()!=5)Reject("presentation_blend");
    layer->_blendMode=BlendMode(int(Index(blend[0],unsigned(eBlendEquation::NUM_BLEND_EQUATIONS))),int(Index(blend[1],unsigned(eBlendFactor::NUM_BLEND_FACTORS))),int(Index(blend[2],unsigned(eBlendFactor::NUM_BLEND_FACTORS))),int(Index(blend[3],unsigned(eBlendFactor::NUM_BLEND_FACTORS))),int(Index(blend[4],unsigned(eBlendFactor::NUM_BLEND_FACTORS))));
    Shaders(layer,true,p);
  }
}
std::unique_ptr<Scene> Prepare(const std::string &text){
  // 18的域元数据必须与所有native Sprite属于同一个候选，不能先发布几何再加载资源。
  std::unique_ptr<replica::RoomGrid> domain;
  rapidjson::Document root;root.Parse<rapidjson::kParseIterativeFlag|rapidjson::kParseValidateEncodingFlag>(text.data(),text.size());
  if(root.HasParseError()||!root.IsObject())Reject("presentation_descriptor_json");
  if(root.HasMember("format")&&root["format"].IsString()&&
     (std::string(root["format"].GetString())=="isaac.cs.room-grid"||std::string(root["format"].GetString())=="isaac.cs.room-grid-source")){
    domain=std::make_unique<replica::RoomGrid>(replica::PrepareRoomGrid(text));
    root.Parse<rapidjson::kParseIterativeFlag|rapidjson::kParseValidateEncodingFlag>(domain->descriptor.data(),domain->descriptor.size());
  }else if(text.size()>256u*1024u)Reject("presentation_descriptor_budget");
  std::vector<std::pair<const Value *,unsigned>> work{{&root,0}};unsigned count=0;
  while(!work.empty()){auto item=work.back();work.pop_back();if(++count>(domain?524288u:16384u)||item.second>32)Reject("presentation_value_budget");
    if(item.first->IsArray())for(const auto &v:item.first->GetArray())work.emplace_back(&v,item.second+1);
    if(item.first->IsObject()){std::set<std::string> names;for(auto it=item.first->MemberBegin();it!=item.first->MemberEnd();++it){if(!names.insert(String(it->name)).second)Reject("presentation_duplicate_member");work.emplace_back(&it->value,item.second+1);}}
  }
  if(Id(Member(root,"connection"))!=State().connection)Reject("presentation_connection_stale");
  auto scene=std::make_unique<Scene>();scene->descriptor=text;
  scene->room_grid=std::move(domain);
  scene->epoch=Id(Member(root,"epoch"));scene->revision=Id(Member(root,"revision"));
  const auto &room=Member(root,"room");scene->size=Vec(Member(room,"size"));
  if(scene->size.x<=0||scene->size.y<=0)Reject("presentation_room_geometry");
  const auto &camera=Member(root,"camera");scene->camera=Vec(Member(camera,"origin"));scene->zoom=Number(Member(camera,"zoom"));
  if(scene->zoom<=0||scene->zoom>16)Reject("presentation_camera_zoom");
  scene->hud=String(Member(root,"hud"),2048);
  const auto &nodes=Member(root,"nodes");if(!nodes.IsArray()||nodes.Size()>(scene->room_grid?4096u:512u))Reject("presentation_nodes_budget");
  std::set<std::uint64_t> ids;std::uint64_t pixels=0;std::set<const void *> images;
  for(const auto &p:nodes.GetArray()){
    Node node;node.id=Id(Member(p,"id"));if(!ids.insert(node.id).second)Reject("presentation_node_duplicate");
    node.pass=Index(Member(p,"pass"),8);node.screen=Flag(Member(p,"screen"));node.position=Vec(Member(p,"position"));
    if(p.HasMember("order"))node.order=Number(p["order"]);
    if(p.HasMember("clamp_top"))node.clamp_top=Vec(p["clamp_top"]);
    if(p.HasMember("clamp_bottom"))node.clamp_bottom=Vec(p["clamp_bottom"]);
    // 压力板倒计时等字形只显示Authority给定文本，不能读取本地Game计时器重新计算。
    if(p.HasMember("text")){
      node.text=String(p["text"],2048);if(node.text.empty())Reject("presentation_text_empty");
      node.text_scale=Vec(Member(p,"text_scale"));const auto &color=Member(p,"text_color");
      if(!color.IsArray()||color.Size()!=4)Reject("presentation_text_color");node.text_color=KColor(Number(color[0]),Number(color[1]),Number(color[2]),Number(color[3]));
      scene->nodes.push_back(std::move(node));continue;
    }
    // 不用delete ANM2：native destructor已经析构std_string和内部数组，避免C++再次析构成员。
    void *raw=::operator new(sizeof(ANM2));ANM2 *sprite=nullptr;
    try{sprite=new(raw) ANM2;}catch(...){::operator delete(raw);throw;}
    node.sprite=std::make_unique<Sprite>(sprite);
    auto anm=Path(Member(p,"anm2"));std_string filename=anm;sprite->Load(filename,false);
    if(!sprite->_loaded||!sprite->_layerCount||sprite->_layerCount>64||sprite->_animCount<=0||sprite->_animCount>4096)Reject("presentation_anm_missing");
    const auto &replacements=Member(p,"sheets");if(!replacements.IsArray()||replacements.Size()>64)Reject("presentation_sheets_budget");
    std::set<unsigned> replaced;
    for(const auto &r:replacements.GetArray()){
      auto id=Index(Member(r,"id"),64);if(!replaced.insert(id).second)Reject("presentation_sheet_layer");const auto layer=Layer(sprite,id);
      std_string path=Path(Member(r,"path"));sprite->ReplaceSpritesheet(int(layer.ordinal),path);
    }
    sprite->LoadGraphics(false);
    for(unsigned i=0;i<sprite->_layerCount;++i){auto &layer=sprite->_layerState[i];if(!layer._layerData)continue;const auto &image=layer._spriteSheet;
      if(!layer._spriteSheetLoaded||!image.image||!image.counter||image.image->GetWidth()<=0||image.image->GetHeight()<=0)Reject("presentation_image_missing");
      // 多格共享同一native image只计一次纹理；不把448格共享图误算成448张独立纹理。
      if(images.insert(image.image).second)pixels+=std::uint64_t(image.image->GetWidth())*unsigned(image.image->GetHeight());
      if(pixels>16u*1024u*1024u)Reject("presentation_texture_budget");
    }
    const auto &pose=Member(p,"pose");
    auto *main=Animation(sprite,Member(pose,"animation"),Member(pose,"frame"),false);
    AnimationData *overlay=nullptr;const auto &o=Member(pose,"overlay");
    if(!o.IsNull())overlay=Animation(sprite,Member(o,"animation"),Member(o,"frame"),true);
    // 只保留本方姿态的分数相位；key内插值已由资源bake，不能据此承诺额外连续插值。
    if(pose.HasMember("phase")){const float phase=Number(pose["phase"]);if(phase<0||phase>=1)Reject("presentation_phase");sprite->_animState._animFrame+=phase;}
    if(overlay&&o.HasMember("phase")){const float phase=Number(o["phase"]);if(phase<0||phase>=1)Reject("presentation_phase");sprite->_overlayAnimState._animFrame+=phase;}
    sprite->_overlayRenderFirst=Flag(Member(pose,"overlay_first"));
    sprite->_scale=Vec(Member(pose,"scale"));sprite->_offset=Vec(Member(pose,"offset"));
    sprite->_rotation=Number(Member(pose,"rotation"));sprite->_color=Color(Member(pose,"color"));
    sprite->_championColor=Color(Member(pose,"champion_color"));sprite->_flipX=Flag(Member(pose,"flip_x"));sprite->_flipY=Flag(Member(pose,"flip_y"));
    const bool random_uv=pose.HasMember("resolved_random_uv")&&Flag(pose["resolved_random_uv"]);
    const auto &flags=Member(pose,"flags");if(!flags.IsUint())Reject("presentation_flags");
    sprite->_bitflags=FinalFlags(flags,pose,random_uv);
    const auto &layers=Member(pose,"layers");
    if((flags.GetUint()&2)&&(!layers.IsArray()||layers.Size()!=sprite->_layerCount))Reject("presentation_random_uv_layers_incomplete");
    Shaders(sprite,false,pose);Layers(sprite,main,overlay,layers,random_uv);
    // ANM资源默认层也不能遗留随机位；有该位的每层都必须有显式crop结果。
    for(unsigned i=0;i<sprite->_layerCount;++i)if(sprite->_layerState[i]._bitflags&(2u|32u|128u))Reject("presentation_resource_effect_unresolved");
    // Client只展示指定后态。禁止RGON插值路径按本地奇偶帧改写共享AnimationFrame。
    sprite->_animState._isPlaying=false;sprite->_overlayAnimState._isPlaying=false;
    scene->nodes.push_back(std::move(node));
  }
  std::stable_sort(scene->nodes.begin(),scene->nodes.end(),[](const Node &a,const Node &b){return a.pass==b.pass?a.order<b.order:a.pass<b.pass;});
  return scene;
}
}

bool PresentationBegin(std::uint64_t connection) noexcept {
  if(!IsMain()||!connection)return false;auto &s=State();if(s.active||s.current||!s.retiring.empty())return false;
  if(!g_Manager)return false;
  try{s.progress.resize(sizeof(PersistentGameData));}catch(...){return false;}
  std::memcpy(s.progress.data(),g_Manager->GetPersistentGameData(),s.progress.size());s.progress_changed=false;
  s.active=true;s.stopping=false;s.connection=connection;s.revision=s.epoch=0;s.dc=nullptr;s.context=nullptr;s.pending.clear();s.capture.clear();s.error.clear();return true;
}
bool PresentationNativeGraphicsReady() noexcept {return Graphics();}
void PresentationNativeDrawStarted() noexcept {auto &s=State();s.render_pending=true;s.draw_apply=s.applies;s.draw_reset=s.resets;}
void PresentationClearTarget(){ClearScene();}
void PresentationCaptureNativeReference(const std::string &name){if(!Graphics()||!State().capture.empty())Reject("native_reference_context");State().capture=name;CaptureTarget();}
const char *PresentationSubmit(const std::string &descriptor) noexcept {
  auto &s=State();if(!IsMain()||!s.active||s.stopping)return "presentation_not_active";
  if(descriptor.size()>8u*1024u*1024u)return "presentation_descriptor_budget";
  if(!s.pending.empty())return s.pending==descriptor?"":"presentation_candidate_busy";
  if(s.current&&s.current->descriptor==descriptor)return "";
  try{s.pending=descriptor;return "";}catch(...){return "presentation_allocation";}
}
void PresentationStop() noexcept {if(!IsMain())return;auto &s=State();s.stopping=true;s.pending.clear();if(!s.current&&s.retiring.empty()&&!s.drawing)s.active=false;}
bool PresentationDrained() noexcept {auto &s=State();return !s.current&&s.retiring.empty()&&!s.drawing&&s.pending.empty();}
// 比较当前PGD内存值只作必要生命周期检查，绝不回滚dirty或覆写访客进度。
bool PresentationProgressUnchanged() noexcept {
  auto &s=State();if(!IsMain()||!g_Manager||s.progress.empty())return false;
  if(std::memcmp(s.progress.data(),g_Manager->GetPersistentGameData(),s.progress.size()))s.progress_changed=true;
  return !s.progress_changed;
}
const char *PresentationFailure() noexcept {return State().error.c_str();}
void PresentationDraw() noexcept {
  auto &s=State();if(!s.active)return;
  if(s.drawing){s.error="presentation_render_reentrant";return;}
  if(!Graphics())return;
  s.render_pending=true;
  s.draw_apply=s.applies;s.draw_reset=s.resets;
  s.drawing=true;
  // J273 404F60的全局XorShift状态；仅显式E2E读取，生产不做逐帧诊断。
  std::uint32_t rng_before=0;const bool watch_rng=!Settings.world_e2e_token.empty()&&DrawRandom(rng_before);
  try {
    // 只在本方draw窗口限定官方ANM2全局模式，正常/异常返回均恢复。反射/GLITCH
    // 由Authority已解析的部件表达，不能从冻结的本地Room读取水量或对象地址派生UV。
    struct Modes final {
      bool light=g_ANM2_LightRendering,reflection=g_ANM2_ReflectionRendering,glitch=g_ANM2_GlitchRendering;
      Modes(){g_ANM2_LightRendering=false;g_ANM2_ReflectionRendering=false;g_ANM2_GlitchRendering=false;}
      ~Modes(){g_ANM2_LightRendering=light;g_ANM2_ReflectionRendering=reflection;g_ANM2_GlitchRendering=glitch;}
    } modes;
    if(!g_ColorOverrideStack.empty())Reject("presentation_foreign_color_override");
    ClearScene();
    if(!s.pending.empty()&&!s.stopping){
      auto pending=std::move(s.pending);s.pending.clear();
      try{auto candidate=Prepare(pending);++s.prepared;
        if(candidate->epoch<s.epoch||(candidate->epoch==s.epoch&&candidate->revision<=s.revision))Reject("presentation_stale_revision");
        if(s.current&&s.current->room_grid&&candidate->room_grid&&candidate->epoch==s.epoch){
          const auto &old=*s.current->room_grid,&next=*candidate->room_grid;
          if(old.session!=next.session||old.connection!=next.connection||old.run!=next.run||old.floor!=next.floor||old.dimension!=next.dimension||old.room!=next.room)Reject("room_grid_scope_changed_without_scene");
        }
        if(s.retiring.size()>=2)Reject("presentation_retirement_budget");
        if(s.current)s.retiring.push_back(std::move(s.current));
        s.epoch=candidate->epoch;s.revision=candidate->revision;s.current=std::move(candidate);++s.published;
      }catch(const std::exception &e){++s.rejected;s.error=e.what();}
    }
    if(s.stopping&&s.current)s.retiring.push_back(std::move(s.current));
    if(s.current&&!s.stopping){auto &scene=*s.current;
      for(auto &node:scene.nodes){Vector at=node.position;
        g_ANM2_LightRendering=node.pass==6;
        if(!node.screen){at.x=(at.x-scene.camera.x)*scene.zoom;at.y=(at.y-scene.camera.y)*scene.zoom;}
        if(!node.sprite){if(g_Manager){auto scale=node.text_scale;if(!node.screen){scale.x*=scene.zoom;scale.y*=scene.zoom;}
          FontSettings settings;g_Manager->_font8_LuaMiniOutlined.DrawString(node.text.c_str(),at,scale,&node.text_color,&settings);}continue;}
        auto *sprite=node.sprite->value;const auto scale=sprite->_scale;
        if(!node.screen){sprite->_scale.x*=scene.zoom;sprite->_scale.y*=scene.zoom;}
        try{sprite->Render(&at,&node.clamp_top,&node.clamp_bottom);}catch(...){sprite->_scale=scale;throw;}
        sprite->_scale=scale;
      }
      if(g_Manager){KColor color(1,1,1,1);FontSettings settings;g_Manager->_font8_LuaMiniOutlined.DrawString(scene.hud.c_str(),Vector(20,15),Vector(1,1),&color,&settings);}
      ++scene.drawn;++s.draws;
    } else if(g_Manager) {
      // 连接／失败／退休提示属于本方呈现，不重新开放通用菜单Lua回调。
      KColor color(1,1,1,1);FontSettings settings;
      const auto text=std::string("Isaac CS - ")+(s.stopping?"Retiring resources":ClientStateName())+"\nWaiting for authority world. ESC: stop.";
      g_Manager->_font8_LuaMiniOutlined.DrawString(text.c_str(),Vector(20,20),Vector(1,1),&color,&settings);
    }
    ClientUiHeartbeat();
  }catch(const std::exception &e){s.error=e.what();}catch(...){s.error="presentation_native_exception";}
  if(watch_rng){std::uint32_t after=0;++s.rng_checks;
    if(!DrawRandom(after)||after!=rng_before)++s.rng_changes;
  }
  s.drawing=false;
}
void PresentationQueueReturned(bool reset) noexcept {
  NativeRoomGridQueueReturned(reset);
  if(!IsMain()||!ClientBusy())return;auto &s=State();
  if(reset)++s.resets;else ++s.applies;
}
// 正常render返回仍需本轮apply/reset见证及具体raw容器检查；不能以SwapBuffers冒充GPU完成。
void PresentationRenderReturned() noexcept {
  NativeRoomGridRenderReturned();
  auto &s=State();if(!s.active||!s.render_pending||!IsMain())return;s.render_pending=false;
  if(!s.capture.empty()&&s.applies>s.draw_apply&&s.resets>s.draw_reset&&Graphics()&&
     g_KAGE_Graphics_ImageManager._frameImages.empty()&&g_KAGE_Graphics_ImageManager._transparentBatches.empty()){
    try{CaptureTarget();}catch(const std::exception &e){s.capture.clear();s.error=e.what();}catch(...){s.capture.clear();s.error="presentation_capture_failed";}
  }
  if(s.retiring.empty()){if(s.stopping&&!s.current){s.active=false;}return;}
  if(s.applies<=s.draw_apply||s.resets<=s.draw_reset||!Graphics()){++s.drain_waits;return;}
  // frameImages与透明pair是待提交raw owner，两个具体容器必须都已排空。
  // tracked vector是image/cache bookkeeping，不能要求先空再DecrRef形成循环等待。
  // 共享图像仍由新Scene/cache的strong持有；递减到1的cache回调清索引，归零另走图像析构。
  if(!g_KAGE_Graphics_ImageManager._frameImages.empty()||!g_KAGE_Graphics_ImageManager._transparentBatches.empty()){
    ++s.drain_waits;return;
  }
  glFinish();
  s.retired+=s.retiring.size();s.retiring.clear();++s.drains;
  Trace("presentation_retired","connection=%llu revision=%llu frame_apply=%llu frame_reset=%llu os_current=1 cpu_queues_empty=1 gpu_finish=1",s.connection,s.revision,s.applies,s.resets);
  if(s.stopping&&!s.current)s.active=false;
}
std::string PresentationTest(const std::string &token,const std::string &name){
  if(!IsMain()||Settings.world_e2e_token.empty()||token!=Settings.world_e2e_token)return "ok=0\nerror=presentation_test_disabled\n";
  if(name=="callbacks"){
    auto *L=g_LuaEngine?g_LuaEngine->_state:nullptr;if(!L)return "ok=0\nerror=no_lua\n";
    const int top=lua_gettop(L);std::ostringstream out;out<<"ok=1\n";
    lua_getglobal(L,"IsaacCS_PresentationE2E");
    if(!lua_istable(L,-1)){lua_settop(L,top);return "ok=0\nerror=fixture_unavailable\n";}
    for(const char *key:{"MC_PRE_RENDER","MC_MAIN_MENU_RENDER","MC_POST_UPDATE","MC_POST_RENDER","MC_POST_HUD_RENDER"}){
      lua_pushstring(L,key);lua_rawget(L,-2);out<<key<<'='<<(lua_isinteger(L,-1)?lua_tointeger(L,-1):0)<<'\n';lua_pop(L,1);
    }
    lua_settop(L,top);return out.str();
  }
  if(!ClientBusy())return "ok=0\nerror=presentation_not_active\n";
  if(name.rfind("capture-",0)==0){
    if(name.size()>64||name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")!=name.npos)return "ok=0\nerror=capture_name\n";
    if(!State().capture.empty())return "ok=0\nerror=capture_busy\n";State().capture=name;return "ok=1\n";
  }
  if(name=="room-grid-state")return "ok=1\nroom_grid.model="+PresentationRoomGrid("all")+"\n";
  if(name.rfind("room-grid-object-",0)==0)return "ok=1\nroom_grid.object="+PresentationRoomGrid(name.substr(17))+"\n";
  if(name=="mod-persistence"){
    auto *L=g_LuaEngine?g_LuaEngine->_state:nullptr;if(!L||!ModPersistenceReady())return "ok=0\nerror=mod_persistence_unavailable\n";
    const int top=lua_gettop(L);lua_getglobal(L,"IsaacCS_PresentationTryPersistence");
    if(!lua_isfunction(L,-1)||lua_pcall(L,0,1,0)!=LUA_OK||!lua_toboolean(L,-1)){lua_settop(L,top);return "ok=0\nerror=mod_persistence_test_failed\n";}
    lua_settop(L,top);if(!TestInternalModPersistence())return "ok=0\nerror=internal_mod_persistence_test_failed\n";
    lua_pushcfunction(L,InternalPersistenceProbe);lua_setglobal(L,"IsaacCS_PresentationTestInternal");
    return "ok=1\n"+ModPersistenceFields();
  }
  if(name=="wrong-owner"){
    const auto before=State().draws;PresentationDraw();
    return before==State().draws?"ok=1\nwrong_owner_blocked=1\n":"ok=0\nerror=wrong_owner_draw\n";
  }
  if(name.empty()||name.size()>64||name.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789-")!=name.npos)return "ok=0\nerror=presentation_test_name\n";
  const auto path=std::filesystem::u8path(Settings.world_output_dir)/"presentation"/(name+".json");
  std::ifstream file(path,std::ios::binary);if(!file)return "ok=0\nerror=presentation_test_file\n";
  std::string text(8u*1024u*1024u+1,'\0');file.read(text.data(),std::streamsize(text.size()));text.resize(std::size_t(file.gcount()));
  if(text.size()>8u*1024u*1024u)return "ok=0\nerror=presentation_descriptor_budget\n";
  const auto *error=PresentationSubmit(text);return *error?"ok=0\nerror="+std::string(error)+"\n":"ok=1\n";
}
std::string PresentationFields(){
  const auto &s=State();std::ostringstream o;
  o<<"presentation.active="<<s.active<<"\npresentation.stopping="<<s.stopping<<"\npresentation.connection="<<s.connection
   <<"\npresentation.progress_changed="<<s.progress_changed
   <<"\npresentation.epoch="<<s.epoch<<"\npresentation.revision="<<s.revision<<"\npresentation.prepared="<<s.prepared
   <<"\npresentation.published="<<s.published<<"\npresentation.rejected="<<s.rejected<<"\npresentation.draws="<<s.draws
   <<"\npresentation.retired="<<s.retired<<"\npresentation.retiring="<<s.retiring.size()<<"\npresentation.context_checks="<<s.context_checks
   <<"\npresentation.rng_checks="<<s.rng_checks<<"\npresentation.rng_changes="<<s.rng_changes
   <<"\npresentation.captures="<<s.captures
   <<"\npresentation.context_misses="<<s.context_misses<<"\npresentation.nodes="<<(s.current?s.current->nodes.size():0)
   <<"\npresentation.applies="<<s.applies<<"\npresentation.resets="<<s.resets<<"\npresentation.drains="<<s.drains
   <<"\npresentation.drain_waits="<<s.drain_waits<<"\npresentation.drained="<<PresentationDrained()<<"\npresentation.error="<<s.error<<"\n";
  if(s.current&&s.current->room_grid){const auto &r=*s.current->room_grid;
    o<<"room_grid.revision="<<r.revision<<"\nroom_grid.room="<<r.room<<"\nroom_grid.shape="<<r.shape
     <<"\nroom_grid.cells="<<r.cells.size()<<"\nroom_grid.grids="<<r.grids<<"\nroom_grid.doors="<<r.doors
     <<"\nroom_grid.types="<<r.types<<"\nroom_grid.classes="<<r.classes<<"\nroom_grid.parts="<<r.parts
     <<"\nroom_grid.references="<<r.references<<"\n";
  }else o<<"room_grid.revision=0\nroom_grid.grids=0\nroom_grid.cells=0\n";
  // 仅在有效owner的当前自有Sprite上复制姿态值；不跨线程发布native借用或地址。
  if(s.current&&!s.current->nodes.empty()&&s.current->nodes.front().sprite){
    const auto *sprite=s.current->nodes.front().sprite->value;
    o<<"presentation.main_frame="<<sprite->_animState._animFrame<<"\npresentation.overlay_frame="<<sprite->_overlayAnimState._animFrame
     <<"\npresentation.main_event_mask="<<sprite->_animState._currentlyTriggeredEvents<<'\n';
    if(sprite->_animState._layerFrames&&sprite->_overlayAnimState._layerFrames)
      o<<"presentation.layer0_key="<<sprite->_animState._layerFrames[0]<<"\npresentation.overlay_layer0_key="<<sprite->_overlayAnimState._layerFrames[0]<<'\n';
  }
  return o.str();
}
std::string PresentationRoomGrid(const std::string &identity){
  if(!IsMain())return "null";const auto &s=State();
  return s.current&&s.current->room_grid?replica::RoomGridObject(*s.current->room_grid,identity):"null";
}
} // namespace cs::game
