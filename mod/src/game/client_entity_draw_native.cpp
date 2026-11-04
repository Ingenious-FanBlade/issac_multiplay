#include "game/client_entity_e2e.h"
#include "IsaacRepentance.h"
#include "binding/world_layout.h"
#include "game/client_entity_draw_native.h"
#include "game/client_entity_adapters.h"
#include "game/bridge.h"
#include "game/context.h"
#include "runtime.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <memory>
#include <set>
#include <stdexcept>
#include <utility>

namespace cs::game {
namespace {
// ANM拥有的字符串/图片/引用计数不复制。仅深借用可能被SetFrame重写的选择器，
// 以及已核的显示标量；游戏allocator负责工作数组，离栈恢复原owner。
void Release(int *p){if(p)reinterpret_cast<void (__cdecl *)(void *)>(Base+0x686bc9)(p);}
int *Selectors(const int *p,unsigned count){if(count>65536||(!p&&count))throw std::runtime_error("entity_draw_selector_shape");if(!count)return nullptr;
  auto *out=static_cast<int *>(reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(count*4));if(!out)throw std::bad_alloc();std::memcpy(out,p,count*4);return out;}
struct AnimationBorrow final {
  AnimationState &state;AnimationState previous,working;bool attached=false;
  explicit AnimationBorrow(AnimationState &s):state(s),previous(s),working(s){working._layerFrames=working._nullLayerFrames=nullptr;
    try{if(s._animData){working._layerFrames=Selectors(s._layerFrames,s._animData->_layerCount);working._nullLayerFrames=Selectors(s._nullLayerFrames,s._animData->_nullLayerCount);}}
    catch(...){Release(working._layerFrames);throw;}}
  void Attach(){state=working;attached=true;}
  ~AnimationBorrow(){if(attached){Release(state._layerFrames);Release(state._nullLayerFrames);state=previous;}else{Release(working._layerFrames);Release(working._nullLayerFrames);}}
};
struct SpriteBorrow final {
  ANM2 &sprite;AnimationBorrow main,overlay;bool first;
  std::array<unsigned char,0x80> scalars{};
  std::vector<std::array<unsigned char,0x78>> layers;
  explicit SpriteBorrow(ANM2 &s):sprite(s),main(s._animState),overlay(s._overlayAnimState),first(s._overlayRenderFirst){
    if(s._layerCount>65536||(s._layerCount&&!s._layerState))throw std::runtime_error("entity_draw_layer_shape");
    std::memcpy(scalars.data(),reinterpret_cast<unsigned char *>(&s)+0x94,scalars.size());layers.resize(s._layerCount);
    for(unsigned i=0;i<s._layerCount;++i)std::memcpy(layers[i].data(),reinterpret_cast<unsigned char *>(&s._layerState[i])+0x20,layers[i].size());}
  void Attach(){main.Attach();overlay.Attach();}
  ~SpriteBorrow(){sprite._overlayRenderFirst=first;std::memcpy(reinterpret_cast<unsigned char *>(&sprite)+0x94,scalars.data(),scalars.size());
    for(unsigned i=0;i<layers.size();++i)std::memcpy(reinterpret_cast<unsigned char *>(&sprite._layerState[i])+0x20,layers[i].data(),layers[i].size());}
};
// 965:10、Familiar和持物借用当前Player；状态/选择器持有至最外层CPU队列退尽。
struct PlayerBorrow final {
  Entity_Player &player;Entity *minecart=nullptr,*desired=nullptr;
  Entity *held=nullptr,*desired_held=nullptr;std::int32_t item_state=0,held_pose=0;
  Vector position,position_offset,cart_offset;ColorMod color;bool visible=false,attached=false;
  std::uint32_t frame_base=0;const ClientObjectDescription &description;
  std::vector<std::unique_ptr<SpriteBorrow>> sprites;
  PlayerBorrow(Entity_Player &p,const ClientObjectDescription &d):player(p),minecart(p.GetMinecart()),position(p._pos),position_offset(p._posOffset),visible(p._visible),description(d){
    held=p.GetHeldEntity();held_pose=p._suplexState;std::memcpy(&item_state,reinterpret_cast<unsigned char *>(&p)+0x15f4,4);
    std::memcpy(&cart_offset,reinterpret_cast<unsigned char *>(&p)+0x205c,8);std::memcpy(&color,reinterpret_cast<unsigned char *>(&p)+0x1b8,sizeof(color));
    std::memcpy(&frame_base,reinterpret_cast<unsigned char *>(&p)+0x328,4);
    sprites.push_back(std::make_unique<SpriteBorrow>(p._sprite));sprites.push_back(std::make_unique<SpriteBorrow>(p._bodySprite));
    if(d.derived->player_composite_v1){const auto &layout=world::layout::LayoutConstants();
      for(unsigned i=1;i<layout.player_inline_sprite_count;++i)sprites.push_back(std::make_unique<SpriteBorrow>(*reinterpret_cast<ANM2 *>(reinterpret_cast<unsigned char *>(&p)+layout.player_inline_sprite_first_offset+i*layout.player_inline_sprite_stride)));}
    if(p._costumeSpriteDescs.size()>16384)throw std::runtime_error("entity_player_draw_costume_budget");
    for(auto &costume:p._costumeSpriteDescs)sprites.push_back(std::make_unique<SpriteBorrow>(costume._sprite));
    for(const auto &r:d.relations)if(r.member=="Entity.GetMinecart"&&!r.is_null){desired=static_cast<Entity *>(ClientNativeObjectDrawAddress(r.target));if(!desired)throw std::runtime_error("entity_player_draw_minecart_key");}
    if(d.derived->player_minecart_draw&&(!desired||desired->_type!=965||desired->_variant!=10))throw std::runtime_error("entity_player_draw_minecart_shape");
    if(d.derived->player_held_draw){
      for(const auto &r:d.relations)if(r.member=="EntityPlayer.GetHeldEntity"&&!r.is_null)desired_held=static_cast<Entity *>(ClientNativeObjectDrawAddress(r.target));
      const auto *source=ClientNativeObjectDrawDescription(desired_held);
      if(!desired_held||!source||source->shape_revision!=d.derived->player_held_shape||!desired_held->_exists)throw std::runtime_error("entity_player_draw_held_current_key_or_shape");
      if(!d.derived->player_composite_v1&&(source->category!=9||source->variant!=136||desired_held->_visible||desired_held->GetParent()!=&p))throw std::runtime_error("entity_player_draw_held136_owner_or_pose");
      // Player Render临时变换持物并调用其原生Render；资源留在原owner，选择器/标量工作副本保护到外层drain。
      sprites.push_back(std::make_unique<SpriteBorrow>(desired_held->_sprite));
      if(d.derived->player_composite_v1&&source->derived)for(const auto &resource:source->derived->resources){
        auto *sprite=static_cast<ANM2 *>(ClientEntityAuxiliarySprite(desired_held,resource));if(!sprite)throw std::runtime_error("entity_player_held_auxiliary_resource");sprites.push_back(std::make_unique<SpriteBorrow>(*sprite));}
    }
  }
  void Attach(){for(auto &s:sprites)s->Attach();attached=true;player._minecart.SetReference(desired);
    player._heldEntity=desired_held;
    if(description.derived->player_held_draw){player._suplexState=description.derived->player_held_pose;std::memcpy(reinterpret_cast<unsigned char *>(&player)+0x15f4,&description.derived->player_held_item_state,4);}
    if(description.derived->player_minecart_draw){const Vector offset(description.derived->player_minecart_offset.x,description.derived->player_minecart_offset.y);std::memcpy(reinterpret_cast<unsigned char *>(&player)+0x205c,&offset,8);std::memcpy(reinterpret_cast<unsigned char *>(&player)+0x2060,&description.derived->player_minecart_sort_bits,4);}
    for(const auto &f:description.common_fields)if(f.member=="Entity.FrameCount"){const auto base=description.derived->draw.game_frame-std::uint32_t(f.integer);std::memcpy(reinterpret_cast<unsigned char *>(&player)+0x328,&base,4);}
  }
  ~PlayerBorrow(){if(attached){player._minecart.SetReference(minecart);player._pos=position;player._posOffset=position_offset;player._visible=visible;
    player._heldEntity=held;player._suplexState=held_pose;std::memcpy(reinterpret_cast<unsigned char *>(&player)+0x15f4,&item_state,4);
    std::memcpy(reinterpret_cast<unsigned char *>(&player)+0x205c,&cart_offset,8);std::memcpy(reinterpret_cast<unsigned char *>(&player)+0x1b8,&color,sizeof(color));std::memcpy(reinterpret_cast<unsigned char *>(&player)+0x328,&frame_base,4);}}
};
struct Frame final {
  ClientEntityClassState::DrawInputs source{};
  Game *game=nullptr;Manager *manager=nullptr;
  std::uint32_t manager_frame=0,game_frame=0;
  std::array<std::uint32_t,4> pickup{},familiar{};
  bool installed=false,uncertain=false;
  int exceptions=std::uncaught_exceptions();
  std::vector<std::unique_ptr<PlayerBorrow>> players;
  ~Frame(){if(installed){manager->_framecount=manager_frame;std::memcpy(reinterpret_cast<unsigned char *>(game)+0x25a70,&game_frame,4);
    std::memcpy(reinterpret_cast<void *>(Base+0x7d0ca4),pickup.data(),16);std::memcpy(reinterpret_cast<void *>(Base+0x7d0674),familiar.data(),16);}}
};
thread_local Frame *frame=nullptr;
struct Draw;
thread_local Draw *draw=nullptr;
// 只保存具名值列及原生屏幕缓存，不复制含指针/容器的Entity对象。
struct Draw final {
  struct Scalar final {void *address;unsigned size;std::array<unsigned char,44> bytes{};};
  Entity *entity=nullptr;Draw *previous=nullptr;
  unsigned pass=0;
  std::vector<Scalar> scalars;
  std::vector<std::pair<Entity *,std::uint32_t>> child_frames;
  std::vector<std::unique_ptr<SpriteBorrow>> sprites;
  bool installed=false;
  template<class T> void Keep(T &value){Scalar s{&value,unsigned(sizeof(T))};static_assert(sizeof(T)<=44);std::memcpy(s.bytes.data(),&value,sizeof(T));scalars.push_back(s);}
  void Sprite(ANM2 &s){for(const auto &p:sprites)if(&p->sprite==&s)return;sprites.push_back(std::make_unique<SpriteBorrow>(s));}
  Draw(Entity &e,const ClientObjectDescription &d,unsigned kind):entity(&e),previous(draw),pass(kind){
    for(const auto &f:d.common_fields){
#define CS_ENTITY_FIELD(id,field,kind) if(f.member==id)Keep(e.field);
#define CS_ENTITY_RAW_FIELD(id,offset,type,kind) if(f.member==id)Keep(*reinterpret_cast<type *>(reinterpret_cast<unsigned char *>(&e)+offset));
#define CS_ENTITY_SPECIAL_FIELD(id,kind)
#include "game/client_entity_fields.inc"
#undef CS_ENTITY_FIELD
#undef CS_ENTITY_RAW_FIELD
#undef CS_ENTITY_SPECIAL_FIELD
    }
    for(const auto &f:d.derived->fields){
#define CS_CLASS_FIELD(cat,id,offset,type,kind) if(d.category==cat&&f.member==id)Keep(*reinterpret_cast<type *>(reinterpret_cast<unsigned char *>(&e)+offset));
#include "game/client_entity_class_fields.inc"
#undef CS_CLASS_FIELD
    }
    Keep(*reinterpret_cast<Vector *>(reinterpret_cast<unsigned char *>(&e)+0x1a4));
    Keep(*reinterpret_cast<std::uint32_t *>(reinterpret_cast<unsigned char *>(&e)+0x328));
    Sprite(e._sprite);for(const auto &r:d.derived->resources){auto *s=static_cast<ANM2 *>(ClientEntityAuxiliarySprite(&e,r));if(!s)throw std::runtime_error("entity_draw_auxiliary_owner");Sprite(*s);}
    // 906/920等直接借子体动画，不一定递归调用子体Render；共同保护其真实独立Sprite。
    if(d.category==1)for(const auto &r:d.relations)if(r.member.rfind("NativeNPC.Composite:",0)==0&&!r.is_null){auto *child=static_cast<Entity *>(ClientNativeObjectDrawAddress(r.target));
      const auto *description=ClientNativeObjectDrawDescription(child);if(!child||!description)throw std::runtime_error("entity_draw_composite_child_use");Sprite(child->_sprite);
      Keep(*reinterpret_cast<Vector *>(reinterpret_cast<unsigned char *>(child)+0x1a4));Keep(child->_pos);Keep(child->_posOffset);
      Keep(*reinterpret_cast<Vector *>(reinterpret_cast<unsigned char *>(child)+0xdc));Keep(*reinterpret_cast<Vector *>(reinterpret_cast<unsigned char *>(child)+0xe4));Keep(*reinterpret_cast<float *>(reinterpret_cast<unsigned char *>(child)+0xec));
      Keep(*reinterpret_cast<std::uint32_t *>(reinterpret_cast<unsigned char *>(child)+0x328));
      for(const auto &f:description->common_fields)if(f.member=="Entity.FrameCount")child_frames.emplace_back(child,description->derived->draw.game_frame-std::uint32_t(f.integer));
      if(description->derived)for(const auto &resource:description->derived->resources){auto *s=static_cast<ANM2 *>(ClientEntityAuxiliarySprite(child,resource));if(s)Sprite(*s);}}
    // Familiar复合绘制只借当前具名Player，保护其颜色/可见性/位置及主Sprite；不拥有或退休身体。
    if(d.category==8&&(d.variant==68||d.variant==78||d.variant==243)){
      auto *p=static_cast<Entity_Familiar &>(e)._player;if(!p||!p->_valid||!p->_exists||!p->ToPlayer())throw std::runtime_error("entity_draw_player_owner");
      Keep(p->_visible);Keep(p->_pos);Keep(p->_posOffset);Keep(*reinterpret_cast<ColorMod *>(reinterpret_cast<unsigned char *>(p)+0x1b8));Sprite(p->_sprite);
    }
  }
  void Attach(const ClientObjectDescription &d){for(auto &p:sprites)p->Attach();installed=true;draw=this;
    for(const auto &p:child_frames)std::memcpy(reinterpret_cast<unsigned char *>(p.first)+0x328,&p.second,4);
    for(const auto &f:d.common_fields)if(f.member=="Entity.FrameCount"){const auto base=d.derived->draw.game_frame-std::uint32_t(f.integer);std::memcpy(reinterpret_cast<unsigned char *>(entity)+0x328,&base,4);}}
  ~Draw(){if(installed){for(auto i=scalars.rbegin();i!=scalars.rend();++i)std::memcpy(i->address,i->bytes.data(),i->size);draw=previous;}}
};
Vector Screen(Entity &e,const Vector &offset,float height=0,bool use_offset=true){const float scale=*reinterpret_cast<float *>(Base+0x788730)* *reinterpret_cast<float *>(Base+0x7883e8);
  if(!std::isfinite(scale)||scale<=0)throw std::runtime_error("entity_draw_pixel_scale");
  return Vector(std::floor((((e._pos.x+(use_offset?e._posOffset.x:0)-60)*0.65f+(g_WIDTH-338)*0.5f)*scale)+0.5f)/scale+offset.x,
    std::floor((((e._pos.y+(use_offset?e._posOffset.y:0)+height-140)*0.65f+(g_HEIGHT-182)*0.5f)*scale)+0.5f)/scale+offset.y);}
void Text(Entity &e,const ClientObjectDescription &d,const Vector &offset){
  if(!e._exists)return;const auto &s=*d.derived;auto &font=g_Manager->_font6_PFTempestaSevenCondensed;
  if(!font._isLoaded)throw std::runtime_error("entity_text_font_not_loaded");
  Vector position=Screen(e,offset),scale(1,1);FontSettings settings;
  KColor color(s.text_color[0],s.text_color[1],s.text_color[2],s.text_color[3],s.text_color_extra);
  if(d.variant==2){std::int32_t counter=0;std::memcpy(&counter,reinterpret_cast<unsigned char *>(&e)+0x410,4);
    const float size=4-2.5f*(std::max)(0.0f,(std::min)(1.0f,(float(s.draw.subframe)+2.0f*float(counter))/8));
    position.x-=font.GetStringWidth(s.text.c_str())*0.5f*size;position.y-=4*size;scale=Vector(size,size);}
  font.DrawString(s.text.c_str(),position,scale,&color,&settings);
}
}
ClientEntityFrameScope::ClientEntityFrameScope(const ClientNativeRoots &roots){
  if(!IsMain()||!ClientBusy()||TopOwner()!=Owner::Render||frame||!g_Game||roots.game!=g_Game||roots.room!=g_Game->_room||!g_Manager||!ClientEntityDrawIsolationInstalled()||!ClientEntityOrdinaryTypeContext())return;
  auto state=std::make_unique<Frame>();state->game=g_Game;state->manager=g_Manager;bool found=false;
  const auto &list=g_Game->_room->_entityList._renderEL;
  for(unsigned i=0;i<list._size;++i){const auto *d=ClientNativeObjectDrawDescription(list._data[i]);if(!d||!d->full_entity||!d->derived)continue;const auto &inputs=d->derived->draw;
    if(!inputs.present)return;
    if(found&&(inputs.manager_frame!=state->source.manager_frame||inputs.game_frame!=state->source.game_frame||inputs.paused!=state->source.paused||inputs.pickup_random!=state->source.pickup_random||inputs.familiar_random!=state->source.familiar_random))return;
    state->source=inputs;found=true;
  }
  // 17代表场景没有19的输入，不扩充其声明；原有表现流程仍可使用。
  if(!found){ready_=true;return;}
  std::set<Entity_Player *> borrowed;
  const auto player=[&](Entity_Player *p){if(!p||!borrowed.insert(p).second)return;const auto *d=ClientNativeObjectDrawDescription(p);
    if(!d||!d->derived)throw std::runtime_error("entity_frame_player_ready_source");state->players.push_back(std::make_unique<PlayerBorrow>(*p,*d));};
  for(auto *p:g_Game->_playerManager._playerList)player(p);
  // 非roster可见身体及被举起Player同样借用，不因列表位置失去完整Body/内嵌资源保护。
  for(unsigned i=0;i<list._size;++i)if(list._data[i]&&list._data[i]->_type==1)player(static_cast<Entity_Player *>(list._data[i]));
  for(std::size_t i=0;i<state->players.size();++i){auto *held=state->players[i]->desired_held;if(held&&held->_type==1)player(static_cast<Entity_Player *>(held));}
  state->manager_frame=g_Manager->_framecount;std::memcpy(&state->game_frame,reinterpret_cast<unsigned char *>(g_Game)+0x25a70,4);
  std::memcpy(state->pickup.data(),reinterpret_cast<void *>(Base+0x7d0ca4),16);std::memcpy(state->familiar.data(),reinterpret_cast<void *>(Base+0x7d0674),16);
  state->installed=true;g_Manager->_framecount=state->source.manager_frame;std::memcpy(reinterpret_cast<unsigned char *>(g_Game)+0x25a70,&state->source.game_frame,4);
  std::memcpy(reinterpret_cast<void *>(Base+0x7d0ca4),state->source.pickup_random.data(),16);std::memcpy(reinterpret_cast<void *>(Base+0x7d0674),state->source.familiar_random.data(),16);
  // 首次原生关系安装前把owner交给常驻借用；Attach异常不自动析构未知在途前缀。
  state_=state.release();frame=static_cast<Frame *>(state_);
  for(auto &p:frame->players)p->Attach();ready_=true;
}
ClientEntityFrameScope::~ClientEntityFrameScope(){
  auto *state=static_cast<Frame *>(state_);
  if(state&&(state->uncertain||std::uncaught_exceptions()!=state->exceptions||!ClientNativeCpuQueuesEmpty())){
    // 未知draw/队列异常保留全部owner和隔离；不能在未消费raw使用上释放选择器或强交还Off。
    state->uncertain=true;ClientCallbackFault("entity_frame_inflight_owner_retained");return;
  }
  if(frame==state_)frame=nullptr;delete state;
}
ClientEntityDrawScope::ClientEntityDrawScope(void *pointer,void *offset,unsigned pass){
  ClientEntityTestMeasure measure("draw_prepare");
  if(!IsMain()||!ClientBusy()||!frame)return;
  if(frame->uncertain){original_=false;return;}
  for(auto *p=draw;p;p=p->previous)if(p->entity==pointer){
    // 派生Shadow/Post正常调用同一对象的base入口，外层借用已覆盖；不能当成主体递归。
    if(pass==1||pass==2||(p->pass==3&&pass==0))return;
    original_=false;ClientCallbackFault("entity_draw_recursive_subject");return;
  }
  const auto *d=ClientNativeObjectDrawDescription(pointer);
  if(!d||!d->full_entity||!d->derived){if(static_cast<Entity *>(pointer)->_type!=1){original_=false;ClientCallbackFault("entity_draw_unowned_subject");}return;}
  if(ClientEntityTestFault("draw-before")){original_=false;ClientCallbackFault("draw-before");return;}
  auto state=std::make_unique<Draw>(*static_cast<Entity *>(pointer),*d,pass);state->Attach(*d);state_=state.release();
  if(pass==0&&d->category==11){original_=false;if(!offset)throw std::runtime_error("entity_text_draw_offset");Text(*static_cast<Entity *>(pointer),*d,*static_cast<Vector *>(offset));}
}
ClientEntityDrawScope::~ClientEntityDrawScope(){
  if(!state_)return;
  // RenderLayer已同步把姿态读成顶点；队列只借Image/Batch，不借选择器或Entity标量。
  // 此处恢复工作值，不在原生Shadow等未收口batch中Present；资源owner由整帧scope保持到外层drain。
  delete static_cast<Draw *>(state_);
  // 原调用已返回且工作值已归还；外层RenderState仍负责消费本帧队列，再进入失败退休。
  if(ClientEntityTestFault("draw-after"))ClientCallbackFault("draw-after");
}
bool ClientEntityDrawPaused(bool &value) noexcept {if(!IsMain()||!frame)return false;value=frame->source.paused;return true;}
bool ClientEntityDrawDrained() noexcept {return IsMain()&&!frame&&!draw;}
bool ClientEntityOrdinaryTypeContext() noexcept {
  if(!IsMain()||!g_Game)return false;std::int32_t state=0;
  // 55546F的真实条件是Deathmatch状态>=2，不是Player拥有NPC路径。只读，不改原版模式。
  std::memcpy(&state,reinterpret_cast<unsigned char *>(g_Game)+0x25b8c,4);return state<2;
}
bool ClientEntityDrawOwnedFinal(void *pointer,void *sprite,void *offset){
  if(!IsMain()||!frame)return false;
  const auto *d=ClientNativeObjectDrawDescription(pointer);
  if(!d||d->category!=8||!d->derived||!sprite||!offset)throw std::runtime_error("entity_owned_draw_contract");
  bool owned=false;for(const auto &r:d->derived->resources)if(r.member=="owned_sprite"&&ClientEntityAuxiliarySprite(pointer,r)==sprite)owned=true;
  if(!owned)throw std::runtime_error("entity_owned_draw_sprite_owner");
  auto &s=*static_cast<ANM2 *>(sprite);if(s._loaded){Vector position=Screen(*static_cast<Entity *>(pointer),*static_cast<Vector *>(offset),-25,false);Vector zero(0,0);s.Render(&position,&zero,&zero);}return true;
}
} // namespace cs::game
