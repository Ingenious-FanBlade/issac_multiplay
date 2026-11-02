#include "IsaacRepentance.h"
#include "game/client_resources.h"
#include "game/client_lifetime.h"
#include "game/bridge.h"
#include "game/context.h"
#include "runtime.h"
#include <cmath>
#include <new>
#include <windows.h>

namespace cs::game {
namespace {
// 失败方式：错误 context、资源缺失、层映射不符、半套 Load、构造/释放异常。
// 候选先登记寿命，完成前从不交给 draw；原生异常后不重试析构或伪造回滚。
struct ResourceCandidate final {
  ANM2 *sprite=nullptr;
  bool constructor_entered=false,constructed=false,release_entered=false;
};
bool ResourceWindow() noexcept {
  void *dc=nullptr,*context=nullptr;
  return IsMain()&&ClientBusy()&&ContextHealthy()&&!InBlockedContext()&&
      TopOwner()==Owner::Manager&&ClientRendererHandles(dc,context)&&
      dc&&context&&wglGetCurrentDC()==dc&&wglGetCurrentContext()==context;
}
bool ReleaseCandidate(void *pointer) noexcept {
  auto *owner=static_cast<ResourceCandidate *>(pointer);
  if(!owner||!ResourceWindow()||!ClientNativeCpuQueuesEmpty()||owner->release_entered)return false;
  if(owner->constructor_entered&&!owner->constructed)return false;
  owner->release_entered=true;
  try {
    if(owner->constructed)owner->sprite->destructor();
    ::operator delete(owner->sprite);delete owner;return true;
  }catch(...){return false;}
}
ColorMod Color(const ClientPoseColor &v) {
  return ColorMod(v.tint[0],v.tint[1],v.tint[2],v.tint[3],v.offset[0],v.offset[1],v.offset[2],
                  v.colorize[0],v.colorize[1],v.colorize[2],v.colorize[3]);
}
bool AnimationReady(ANM2 &sprite,const std::string &name,int frame,bool overlay) {
  if(name.empty()||frame<0)return false;
  auto *data=sprite.GetAnimationData(name);
  return data&&data->_length>0&&frame<data->_length&&data->_layerCount>=0&&data->_layerCount<=64&&
      (!data->_layerCount||data->_animLayers)&&(!overlay||!name.empty());
}
// layer selector 经当前资源的真实帧数校验；null selector 只认可原生定位出的结果。
// 不猜测未声明的 null-layer 内部布局，也不通过 Update 消费事件。
bool ExactPhase(AnimationState &state,float phase,const std::vector<int> &layers,const std::vector<int> &nulls) {
  const auto *data=state._animData;
  if(!data)return phase==0&&layers.empty()&&nulls.empty();
  if(!data||!std::isfinite(phase)||phase<0||phase>=data->_length||
     data->_layerCount!=int(layers.size())||data->_nullLayerCount!=int(nulls.size())||
     (!layers.empty()&&(!data->_animLayers||!state._layerFrames))||(!nulls.empty()&&!state._nullLayerFrames))return false;
  for(unsigned i=0;i<layers.size();++i){
    const auto count=data->_animLayers[i].GetFrameCount();
    // 原生普通Player动画含无关键帧的层；这些层没有可索引帧，只认可SetFrame留下的哨兵。
    // 不能把实际采样到的空层selector=0误判为越界，也不能接纳任意空层selector。
    if(count<0||(count==0?layers[i]!=state._layerFrames[i]:(layers[i]<0||layers[i]>=count)))return false;
  }
  for(unsigned i=0;i<nulls.size();++i)if(state._nullLayerFrames[i]!=nulls[i])return false;
  for(unsigned i=0;i<layers.size();++i)state._layerFrames[i]=layers[i];
  state._animFrame=phase;return true;
}
// 不使用 ANM2::Update；setter 定位时间帧后显式停止播放，不消费动画事件。
bool SetPose(ANM2 &sprite,const ClientResourcePose &pose,const char *&reason) {
  if(sprite._layerCount!=pose.layers.size()||sprite._layerCount>64||!sprite._layerState){reason="resource_layer_count";return false;}
  if((!pose.animation.empty()&&!AnimationReady(sprite,pose.animation,pose.frame,false))||
      (pose.overlay_present&&!AnimationReady(sprite,pose.overlay_animation,pose.overlay_frame,true))){reason="resource_animation_frame";return false;}
  for(const auto &layer:pose.layers){
    auto *native=sprite.GetLayer(int(layer.id));
    if(!native||!native->_layerData||native->_layerData->ID!=layer.id||native->_layerData->name!=layer.name){reason="resource_layer_identity";return false;}
    if(native->_spriteSheetPath!=layer.sheet){auto path=layer.sheet;sprite.ReplaceSpritesheet(int(layer.id),path);}
  }
  sprite.LoadGraphics(false);
  // 每个所需图像都检查实际 pair 与尺寸；loaded flag 本身不代表全部层成功。
  for(const auto &layer:pose.layers){
    auto *native=sprite.GetLayer(int(layer.id));
    if(!native||!native->_spriteSheetLoaded||!native->_spriteSheet.image||!native->_spriteSheet.counter||
       native->_spriteSheetPath!=layer.sheet||native->_spriteSheet.image->GetWidth()<=0||native->_spriteSheet.image->GetHeight()<=0){reason="resource_image_pair";return false;}
  }
  if(pose.animation.empty()){
    // 原生 Fire 的历史资源可能加载成功而没有选中 FireStage00；保留来源的无动画状态。
    // 这是已加载资源的显式状态，不选 Idle 代替，也不推进 Update 试图恢复。
    sprite._animState._animData=nullptr;
  }else{
    if(!sprite.SetAnimation(pose.animation.c_str(),true)){reason="resource_main_animation";return false;}
    auto animation=pose.animation;sprite.SetFrame(&animation,pose.frame);
  }
  if(pose.overlay_present){
    if(!sprite.SetOverlayAnimation(pose.overlay_animation.c_str(),true)){reason="resource_overlay_animation";return false;}
    sprite.SetOverlayFrame(pose.overlay_animation.c_str(),pose.overlay_frame);
  }else{
    // 和公开 RemoveOverlay 一样，只撤销动画选择，不析构内嵌 AnimationState。
    sprite._overlayAnimState._animData=nullptr;
  }
  if(!pose.exact_phase||!ExactPhase(sprite._animState,pose.main_phase,pose.main_selectors,pose.main_null_selectors)||
     (pose.overlay_present&&!ExactPhase(sprite._overlayAnimState,pose.overlay_phase,pose.overlay_selectors,pose.overlay_null_selectors))){reason="resource_exact_phase";return false;}
  sprite._animState.Stop();sprite._overlayAnimState.Stop();
  sprite._animState._currentlyTriggeredEvents=0;sprite._animState._previouslyTriggeredEvents=0;
  sprite._overlayAnimState._currentlyTriggeredEvents=0;sprite._overlayAnimState._previouslyTriggeredEvents=0;
  for(const auto &layer:pose.layers){
    auto *native=sprite.GetLayer(int(layer.id));
    if(!pose.animation.empty()&&layer.main_frame>=0)sprite.SetLayerFrame(int(layer.id),layer.main_frame);
    if(pose.overlay_present&&layer.overlay_frame>=0)sprite._overlayAnimState.SetLayerFrame(int(layer.id),layer.overlay_frame);
    native->_size=Vector(layer.size.x,layer.size.y);native->_pos=Vector(layer.position.x,layer.position.y);
    native->_cropOffset=Vector(layer.crop.x,layer.crop.y);native->_rotation=layer.rotation;
    native->_color=Color(layer.color);native->_visible=layer.visible;
    native->_flipX=layer.flip_x;native->_flipY=layer.flip_y;native->_bitflags=layer.flags;
    native->_wrapSMode=layer.wrap_s;native->_wrapTMode=layer.wrap_t;
    native->_blendMode=BlendMode(int(layer.blend[0]),int(layer.blend[1]),int(layer.blend[2]),int(layer.blend[3]),int(layer.blend[4]));
  }
  sprite._offset=Vector(pose.offset.x,pose.offset.y);sprite._scale=Vector(pose.scale.x,pose.scale.y);
  sprite._rotation=pose.rotation;sprite._color=Color(pose.color);sprite._flipX=pose.flip_x;sprite._flipY=pose.flip_y;
  sprite._playbackSpeed=pose.playback_speed;sprite._bitflags=pose.flags;sprite._overlayRenderFirst=pose.overlay_first;
  reason="resource_complete";return true;
}
}
bool ClientNativeCpuQueuesEmpty() noexcept {
  if(!IsMain()||!__ptr_g_KAGE_Graphics_ImageManager)return false;
  const auto &manager=*__ptr_g_KAGE_Graphics_ImageManager;
  // ImageManager 真实普通和透明 raw 提交队列；不把此结果解释成 GPU idle。
  return manager._frameImages.empty()&&manager._transparentBatches.empty();
}
ClientResourceResult ClientApplyResourcePose(void *pointer,const ClientResourcePose &pose) noexcept {
  ClientResourceResult result;
  if(!pointer||!ResourceWindow()||!ClientNativeCpuQueuesEmpty()){result.reason="resource_context_or_inflight";return result;}
  if(!pose.loaded){
    if(!pose.path.empty()||!pose.layers.empty()||!pose.animation.empty()||pose.overlay_present||!pose.exact_phase||pose.main_phase!=0||!pose.main_selectors.empty()||!pose.main_null_selectors.empty()){
      result.reason="resource_unloaded_description";return result;}
    try{auto &sprite=*static_cast<ANM2 *>(pointer);result.written=true;
      if(sprite._loaded||!sprite._filename.empty()||sprite._layerCount||sprite._animState._animData||sprite._overlayAnimState._animData){sprite.destructor();sprite.constructor();}
      sprite._offset=Vector(pose.offset.x,pose.offset.y);sprite._scale=Vector(pose.scale.x,pose.scale.y);sprite._rotation=pose.rotation;sprite._color=Color(pose.color);
      sprite._flipX=pose.flip_x;sprite._flipY=pose.flip_y;sprite._playbackSpeed=pose.playback_speed;sprite._bitflags=pose.flags;sprite._overlayRenderFirst=pose.overlay_first;
      result.complete=true;result.reason="resource_unloaded_state_installed";
    }catch(...){result.reason="resource_native_exception";}return result;
  }
  if(pose.path.empty()||pose.layers.empty()||pose.layers.size()>64){result.reason="resource_description";return result;}
  auto *owner=new(std::nothrow) ResourceCandidate;
  if(!owner){result.reason="resource_owner_budget";return result;}
  owner->sprite=static_cast<ANM2 *>(::operator new(sizeof(ANM2),std::nothrow));
  if(!owner->sprite){delete owner;result.reason="resource_allocation";return result;}
  ClientRetainedTicket ticket{};
  if(!ClientRetainGraphics(owner,ReleaseCandidate,ticket)){
    ::operator delete(owner->sprite);delete owner;result.reason="resource_retained_budget";return result;
  }
  try {
    owner->constructor_entered=true;owner->sprite->constructor();owner->constructed=true;
    auto path=pose.path;owner->sprite->Load(path,false);
    if(!SetPose(*owner->sprite,pose,result.reason)){ClientRetireGraphics(ticket);return result;}
    auto &sprite=*static_cast<ANM2 *>(pointer);
    result.written=true;
    if(sprite._filename!=pose.path)sprite.Load(path,false);
    result.complete=SetPose(sprite,pose,result.reason);
    result.layers=result.complete?unsigned(pose.layers.size()):0;
  }catch(...){result.reason="resource_native_exception";}
  ClientRetireGraphics(ticket);return result;
}
bool ClientResourcePoseMatches(void *pointer,const ClientResourcePose &pose) noexcept {
  if(!pointer||!ResourceWindow())return false;
  const auto &sprite=*static_cast<ANM2 *>(pointer);
  if(sprite._loaded!=pose.loaded||sprite._filename!=pose.path||sprite._layerCount!=pose.layers.size()||bool(sprite._animState._animData)!=!pose.animation.empty()||
     (sprite._animState._animData&&(sprite._animState._animData->_name!=pose.animation||sprite._animState._animFrame!=pose.main_phase||sprite._animState._isPlaying)))return false;
  if(bool(sprite._overlayAnimState._animData)!=pose.overlay_present)return false;
  if(pose.overlay_present&&(sprite._overlayAnimState._animData->_name!=pose.overlay_animation||sprite._overlayAnimState._animFrame!=pose.overlay_phase||sprite._overlayAnimState._isPlaying))return false;
  const auto sameState=[](const AnimationState &state,const std::vector<int> &layers,const std::vector<int> &nulls){
    if(!state._animData)return layers.empty()&&nulls.empty()&&state._currentlyTriggeredEvents==0&&state._previouslyTriggeredEvents==0;
    if(!state._animData||state._animData->_layerCount!=int(layers.size())||state._animData->_nullLayerCount!=int(nulls.size()))return false;
    for(unsigned i=0;i<layers.size();++i)if(!state._layerFrames||state._layerFrames[i]!=layers[i])return false;
    for(unsigned i=0;i<nulls.size();++i)if(!state._nullLayerFrames||state._nullLayerFrames[i]!=nulls[i])return false;
    return state._currentlyTriggeredEvents==0&&state._previouslyTriggeredEvents==0;
  };
  if(!sameState(sprite._animState,pose.main_selectors,pose.main_null_selectors)||
     (pose.overlay_present&&!sameState(sprite._overlayAnimState,pose.overlay_selectors,pose.overlay_null_selectors)))return false;
  if(sprite._offset.x!=pose.offset.x||sprite._offset.y!=pose.offset.y||sprite._scale.x!=pose.scale.x||sprite._scale.y!=pose.scale.y||
     sprite._rotation!=pose.rotation||!(sprite._color==Color(pose.color))||sprite._flipX!=pose.flip_x||sprite._flipY!=pose.flip_y||
     sprite._playbackSpeed!=pose.playback_speed||sprite._bitflags!=pose.flags||sprite._overlayRenderFirst!=pose.overlay_first)return false;
  for(const auto &layer:pose.layers){
    if(layer.id>=sprite._layerCount)return false;
    const auto &native=sprite._layerState[layer.id];
    if(native._spriteSheetPath!=layer.sheet||!native._spriteSheet.image||!native._spriteSheet.counter)return false;
    if(!native._layerData||native._layerData->ID!=layer.id||native._layerData->name!=layer.name||
       native._size.x!=layer.size.x||native._size.y!=layer.size.y||native._pos.x!=layer.position.x||native._pos.y!=layer.position.y||
       native._cropOffset.x!=layer.crop.x||native._cropOffset.y!=layer.crop.y||native._rotation!=layer.rotation||
       !(native._color==Color(layer.color))||native._visible!=layer.visible||native._flipX!=layer.flip_x||native._flipY!=layer.flip_y||
       native._bitflags!=layer.flags||native._wrapSMode!=layer.wrap_s||native._wrapTMode!=layer.wrap_t||
       native._blendMode._equation!=layer.blend[0]||native._blendMode._rgbSourceFactor!=layer.blend[1]||
       native._blendMode._rgbDestFactor!=layer.blend[2]||native._blendMode._alphaSourceFactor!=layer.blend[3]||native._blendMode._alphaDestFactor!=layer.blend[4])return false;
  }
  return true;
}
} // namespace cs::game
