#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client_player_roles.h"
#include "game/client_player_state.h"
#include "game/client_player_equipment.h"
#include "game/client_entity_adapters.h"
#include "game/client_entity_draw_native.h"
#include "game/bridge.h"
#include "game/context.h"
#include "runtime.h"
#include <cmath>
#include <cstring>
#include <list>
#include <stdexcept>

namespace cs::game {
namespace {
// 82E1C0分配0x28节点；前8字节next/prev，draw读取data的position及两对动画/帧。
// data+8/+C的语义未核定，当前draw/析构不读取；Client不Update，显式中性化而不冒称已复制。
struct NativeRecord final {Vector position;int authority_a=0,authority_b=0,animation=-1,frame=0,second_animation=-1,second_frame=0;};
using NativeRecords=std::list<NativeRecord>;
struct RecordNode final {RecordNode *next,*previous;NativeRecord value;};
static_assert(_ITERATOR_DEBUG_LEVEL==0&&sizeof(NativeRecord)==32&&sizeof(NativeRecords)==8&&sizeof(RecordNode)==40,"J273可选动画序列list ABI");
NativeRecords &Records(Entity_Player &p){return *reinterpret_cast<NativeRecords *>(reinterpret_cast<unsigned char *>(&p)+0x1c44);}
RecordNode *&Cursor(Entity_Player &p){return *reinterpret_cast<RecordNode **>(reinterpret_cast<unsigned char *>(&p)+0x1c4c);}
RecordNode *Head(NativeRecords &list){return *reinterpret_cast<RecordNode **>(&list);}
ANM2 &RecordSprite(Entity_Player &p){return *reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(&p)+0x1c50);}
// J273 ctor构造两个Image strong，Player原生析构唯一释放；绝不复制Source的地址/计数。
KAGE_SmartPointer_ImageBase &Image(Entity_Player &p,unsigned slot){return *reinterpret_cast<KAGE_SmartPointer_ImageBase *>(reinterpret_cast<unsigned char *>(&p)+0x1ff8+slot*8);}
void Int(lua_State *L,const char *name,unsigned value){lua_pushinteger(L,value);lua_setfield(L,-2,name);}
bool Format(const ClientPlayerSpecialState::Image &s){return (s.shader==1&&s.stride==36)||(s.shader==3&&(s.stride==44||s.stride==88));}
bool Matches(const KAGE_SmartPointer_ImageBase &pair,const ClientPlayerSpecialState::Image &s){
  if(!s.present)return !pair.image&&!pair.counter;const auto *i=pair.image;
  return i&&pair.counter&&i->_name&&s.path==i->_name&&i->_predefinedShader==s.shader&&i->_vertexFormatStride==s.stride&&
      i->_minFilterMode==s.min_filter&&i->_magFilterMode==s.mag_filter&&i->_wrapSMode==s.wrap_s&&i->_wrapTMode==s.wrap_t&&
      i->_vertexFormat&&i->_numVertexAttributes==s.attributes.size()&&std::memcmp(i->_vertexFormat,s.attributes.data(),s.attributes.size())==0;
}
}
// 只读palette leaf的输入是Game已选controller色表；传出最终颜色，Client保持controller=-1。
void ClientPlayerDrawCapture(void *lua,void *pointer){
  auto *L=static_cast<lua_State *>(lua);auto &p=*static_cast<Entity_Player *>(pointer);
  if(!IsMain()||!g_Game)throw std::runtime_error("player_draw_capture_context");
  ColorMod color;
  using Palette=ColorMod *(__thiscall *)(void *,ColorMod *,Entity_Player *);
  reinterpret_cast<Palette>(Base+0x29440)(reinterpret_cast<unsigned char *>(g_Game)+0x1baa8,&color,&p);
  lua_newtable(L);unsigned n=0;
  for(float x:color._tint){if(!std::isfinite(x))throw std::runtime_error("player_palette_finite");lua_pushnumber(L,x);lua_rawseti(L,-2,++n);}
  for(float x:color._colorize){if(!std::isfinite(x))throw std::runtime_error("player_palette_finite");lua_pushnumber(L,x);lua_rawseti(L,-2,++n);}
  for(float x:color._offset){if(!std::isfinite(x))throw std::runtime_error("player_palette_finite");lua_pushnumber(L,x);lua_rawseti(L,-2,++n);}lua_setfield(L,-2,"palette");
  lua_newtable(L);
  for(unsigned slot=0;slot<2;++slot){const auto &pair=Image(p,slot);lua_newtable(L);lua_pushboolean(L,pair.image!=nullptr);lua_setfield(L,-2,"present");
    if(pair.image){const auto &i=*pair.image;if(!pair.counter||!i._name||!i._vertexFormat||i._numVertexAttributes>8)throw std::runtime_error("player_image_source_owner");
      std::size_t size=0;for(;size<1024&&i._name[size];++size){}if(!size||size==1024)throw std::runtime_error("player_image_name_budget");
      lua_pushlstring(L,i._name,size);lua_setfield(L,-2,"image");Int(L,"shader",i._predefinedShader);Int(L,"stride",i._vertexFormatStride);
      Int(L,"min_filter",i._minFilterMode);Int(L,"mag_filter",i._magFilterMode);Int(L,"wrap_s",i._wrapSMode);Int(L,"wrap_t",i._wrapTMode);
      lua_newtable(L);for(unsigned j=0;j<i._numVertexAttributes;++j){lua_pushinteger(L,i._vertexFormat[j]);lua_rawseti(L,-2,j+1);}lua_setfield(L,-2,"attributes");
    }lua_rawseti(L,-2,slot+1);
  }lua_setfield(L,-2,"images");
  auto &list=Records(p);if(list.size()>16384)throw std::runtime_error("player_record_sequence_budget");
  auto *head=Head(list),*cursor=Cursor(p);if(!head||!head->next||!head->previous)throw std::runtime_error("player_record_head");
  lua_newtable(L);unsigned index=0;int selected=cursor==head?-1:-2;auto *previous=head;
  for(auto *node=head->next;node!=head;node=node->next){
    if(!node||node->previous!=previous||index>=list.size())throw std::runtime_error("player_record_native_chain");
    if(node==cursor)selected=int(index);const auto &r=node->value;lua_newtable(L);
    lua_newtable(L);if(!std::isfinite(r.position.x)||!std::isfinite(r.position.y))throw std::runtime_error("player_record_position_finite");
    lua_pushnumber(L,r.position.x);lua_setfield(L,-2,"x");lua_pushnumber(L,r.position.y);lua_setfield(L,-2,"y");lua_setfield(L,-2,"position");
    const auto integer=[&](const char *key,int value){lua_pushinteger(L,value);lua_setfield(L,-2,key);};
    integer("animation",r.animation);integer("frame",r.frame);integer("second_animation",r.second_animation);integer("second_frame",r.second_frame);
    lua_rawseti(L,-2,++index);previous=node;
  }
  if(index!=list.size()||head->previous!=previous||selected== -2)throw std::runtime_error("player_record_count_or_cursor");lua_setfield(L,-2,"records");
  lua_pushinteger(L,selected);lua_setfield(L,-2,"record_cursor");
  lua::luabridge::UserdataPtr::push(L,&RecordSprite(p),lua::Metatables::SPRITE);lua_setfield(L,-2,"record_sprite");
}
// 直接安装有界列表及cursor；不调用录像/回放Update，不使用另一身体的初始化历史。
bool ClientPlayerDrawRecords(void *pointer,const ClientPlayerDescription &d,bool check,const char *&reason){
  if(!pointer||!d.special||!IsMain()||!ClientBusy()||!HasOwner(Owner::Manager)){reason="player_record_install_context";return false;}
  auto &p=*static_cast<Entity_Player *>(pointer);auto &list=Records(p);const auto &s=*d.special;
  if(check){if(list.size()!=s.records.size()){reason="player_record_count_postcondition";return false;}
    int selected=-1;unsigned i=0;auto *head=Head(list),*node=head->next;
    for(const auto &r:list){if(node==Cursor(p))selected=int(i);const auto &x=s.records[i++];
      if(r.position.x!=x.position.x||r.position.y!=x.position.y||r.animation!=x.animation||r.frame!=x.frame||r.second_animation!=x.second_animation||r.second_frame!=x.second_frame){reason="player_record_values_postcondition";return false;}node=node->next;}
    if((selected<0&&Cursor(p)!=head)||selected!=s.record_cursor){reason="player_record_cursor_postcondition";return false;}
  }else{
    NativeRecords candidate;for(const auto &r:s.records)candidate.push_back({Vector(r.position.x,r.position.y),0,0,r.animation,r.frame,r.second_animation,r.second_frame});
    list.swap(candidate);auto *head=Head(list);auto *cursor=head;
    if(s.record_cursor>=0){cursor=head->next;for(int i=0;i<s.record_cursor;++i)cursor=cursor->next;}Cursor(p)=cursor;
  }reason=check?"player_record_current_native_checked":"player_record_current_list_installed";return true;
}
bool ClientPlayerDrawPreflight(const ClientPlayerDescription &d,const char *&reason) noexcept {
  if(!d.special||!d.entity.derived||!d.entity.derived->player_full_draw||!d.equipment||!d.equipment->main.exact_phase){reason="player_complete_draw_source_required";return false;}
  if(d.render_member&&!d.equipment->main.loaded){reason="player_render_main_sprite_unloaded";return false;}
  if((d.player_type==29||d.player_type==38)&&d.state)for(const auto &f:d.state->fields)
    if(f.member.find("Scale")!=std::string::npos&&f.kind==ClientEntityValueKind::Real&&f.real==0){reason="player_flip_draw_scale_divisor_zero";return false;}
  for(const auto &image:d.special->images)if(image.present&&(!Format(image)||image.attributes.empty())){reason="player_draw_image_format_unsupported";return false;}
  reason="player_draw_values_resources_ready";return true;
}
// 只在资源阶段、CPU借用已退尽后换strong；所有图身份/格式从同T读取，不按角色重新选图。
bool ClientPlayerDrawResources(void *pointer,const ClientPlayerDescription &d,bool check,const char *&reason){
  if(!IsMain()||!ClientBusy()||!pointer||!HasOwner(Owner::Manager)||!ClientNativeCpuQueuesEmpty()||!ClientEntityDrawDrained()||!ClientPlayerDrawPreflight(d,reason))return false;
  auto &p=*static_cast<Entity_Player *>(pointer);
  auto &record_sprite=RecordSprite(p);
  if(check){if(!ClientResourcePoseMatches(&record_sprite,d.special->record_sprite)){reason="player_record_sprite_postcondition";return false;}}
  else {const auto applied=ClientApplyResourcePose(&record_sprite,d.special->record_sprite);if(!applied.complete){reason=applied.reason;return false;}}
  for(const auto &r:d.special->records)for(int animation:{r.animation,r.second_animation})if(animation>=0&&(!record_sprite._loaded||animation>=record_sprite._animCount)){reason="player_record_local_animation_range";return false;}
  for(unsigned slot=0;slot<2;++slot){auto &pair=Image(p,slot);const auto &s=d.special->images[slot];if(Matches(pair,s))continue;
    if(check){reason="player_draw_image_native_postcondition";return false;}
    if(!s.present){pair=KAGE_SmartPointer_ImageBase{};continue;}
    KAGE_SmartPointer_ImageBase candidate;
    const auto path=s.path.rfind("resources/gfx/",0)==0?s.path.substr(10):s.path;
    Manager::LoadImage(&candidate,path.c_str(),__ptr_g_VertexAttributeDescriptor_Position,false);
    if(!candidate.image||!candidate.counter||!candidate.image->_name||s.path!=candidate.image->_name){reason="player_draw_image_load";return false;}
    const auto *format=s.shader==1?__ptr_g_VertexAttributeDescriptor_Position:
        reinterpret_cast<const KAGE_Graphics_ImageBase_VertexAttributeDescriptor *>(Base+(s.stride==44?0x788734:0x788760));
    candidate.image->SetVertexFormat(*format);auto **table=*reinterpret_cast<void ***>(candidate.image);
    using Parameters=void (__thiscall *)(KAGE_Graphics_ImageBase *,unsigned,unsigned);
    reinterpret_cast<Parameters>(table[16])(candidate.image,s.min_filter,s.mag_filter);reinterpret_cast<Parameters>(table[17])(candidate.image,s.wrap_s,s.wrap_t);
    if(!Matches(candidate,s)){reason="player_draw_image_loaded_format";return false;}pair=std::move(candidate);
  }reason=check?"player_draw_images_checked":"player_draw_images_installed";return true;
}
// 唯一常驻Hook只替代已提交Player的纯色查询；不把Source设备索引装进本地Player。
bool ClientPlayerDrawPalette(void *output,void *pointer) noexcept {
  if(!IsMain()||!ClientBusy()||!HasOwner(Owner::Render)||!output)return false;
  const auto *d=ClientNativeObjectDrawDescription(pointer);if(!d||!d->derived||!d->derived->player_full_draw)return false;
  const auto &c=d->derived->player_palette;*static_cast<ColorMod *>(output)=ColorMod(c.tint[0],c.tint[1],c.tint[2],c.tint[3],c.offset[0],c.offset[1],c.offset[2],c.colorize[0],c.colorize[1],c.colorize[2],c.colorize[3]);return true;
}
ClientPlayerAdapter ClientPlayerBuiltinAdapter(){
  ClientPlayerAdapter adapter;
  adapter.prepare=[](void *,const ClientPlayerDescription &d,const char *&reason) noexcept {return ClientPlayerDrawPreflight(d,reason);};
  adapter.install=[](void *,void *p,const ClientPlayerDescription &d,bool resources,bool check,const char *&reason){return resources?ClientPlayerDrawResources(p,d,check,reason):ClientPlayerSpecialFields(p,d,check,reason);};
  adapter.relations=[](void *,void *pointer,const ClientPlayerDescription &,const ClientEntityOwners &,const ClientNativeRoots &,bool,bool,const char *&reason){
    // 跨Player/Weapon/held/Entity边已由各唯一owner安装；本adapter的有限槽不拥有Entity指针。
    if(!IsMain()||!ClientBusy()||!pointer||static_cast<Entity_Player *>(pointer)->_controllerIndex!=-1){reason="player_client_must_not_bind_native_input";return false;}reason="player_special_has_no_duplicate_entity_owner";return true;};
  adapter.retire=[](void *,void *pointer,const ClientPlayerDescription &,unsigned,const char *&reason){
    if(!IsMain()||!ClientBusy()||!pointer||!ClientEntityDrawDrained()||!ClientNativeCpuQueuesEmpty()){reason="player_draw_retire_inflight";return false;}
    // deque/Image strong/内嵌ANM皆由真实Player析构；此处只撤控制资格，不重复析构。
    static_cast<Entity_Player *>(pointer)->_controllerIndex=-1;reason="player_draw_native_owned_until_unique_Free";return true;};
  return adapter;
}
}
