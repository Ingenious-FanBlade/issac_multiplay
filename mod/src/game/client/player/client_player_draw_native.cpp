#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "game/client/player/client_player_roles.h"
#include "game/client/player/client_player_state.h"
#include "game/client/player/client_player_equipment.h"
#include "game/client/entity/client_entity_adapters.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
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
} // namespace cs::game
