#include "IsaacRepentance.h"
#include "game/client_resources.h"
#include "game/client_lifetime.h"
#include "game/bridge.h"
#include "game/context.h"
#include "runtime.h"
#include <cmath>
#include <cstring>
#include <new>
#include <windows.h>

namespace cs::game {
bool ClientResourceApplyShaders(void *,const ClientResourcePose &,bool) noexcept;
namespace {
ClientResourceStatistics resource_statistics;
// 失败方式：错误 context、资源缺失、层映射不符、半套 Load、构造/释放异常。
// 候选先登记寿命，完成前从不交给 draw；原生异常后不重试析构或伪造回滚。
struct ResourceCandidate final {
  ANM2 *sprite=nullptr;
  bool constructor_entered=false,constructed=false,release_entered=false,uncertain=false;
};
bool ResourceWindow() noexcept {
  void *dc=nullptr,*context=nullptr;
  return IsMain()&&ClientBusy()&&ContextHealthy()&&!InBlockedContext()&&
      TopOwner()==Owner::Manager&&ClientRendererHandles(dc,context)&&
      dc&&context&&wglGetCurrentDC()==dc&&wglGetCurrentContext()==context;
}
bool ReleaseCandidate(void *pointer) noexcept {
  auto *owner=static_cast<ResourceCandidate *>(pointer);
  if(!owner||!ResourceWindow()||!ClientNativeCpuQueuesEmpty()||owner->release_entered||owner->uncertain)return false;
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
  if(name.empty()||frame<-1)return false;
  auto *data=sprite.GetAnimationData(name);
  return data&&data->_length>=0&&(data->_length?frame<data->_length:frame==0)&&data->_layerCount>=0&&data->_layerCount<=64&&
      (!data->_layerCount||data->_animLayers)&&(!overlay||!name.empty());
}
// selector均按当前资源真实帧数校验；J273 null descriptor是16B、扩展frame stride54h。
// 只读取descriptor的frames/count，不解释其未知字，不通过Update消费事件。
bool NullSelectors(const AnimationState &state,const std::vector<int> &nulls) noexcept {
  const auto *data=state._animData;if(!data)return nulls.empty();if(data->_nullLayerCount!=int(nulls.size())||(!nulls.empty()&&!state._nullLayerFrames))return false;
  const unsigned char *descriptors=nullptr;std::memcpy(&descriptors,data->_nullLayersPtr,sizeof(descriptors));if(!nulls.empty()&&!descriptors)return false;
  for(unsigned i=0;i<nulls.size();++i){const void *frames=nullptr;int count=0;std::memcpy(&frames,descriptors+i*16+8,4);std::memcpy(&count,descriptors+i*16+12,4);
    if(count<0||count>1000000||(count&&!frames)||nulls[i]<-1||
       (count?nulls[i]>=count:nulls[i]>0))return false;}
  return true;
}
bool LayerSelectors(const AnimationState &state,const std::vector<int> &layers) noexcept {
  const auto *data=state._animData;if(!data)return layers.empty();if(data->_layerCount!=int(layers.size())||(!layers.empty()&&(!data->_animLayers||!state._layerFrames)))return false;
  for(unsigned i=0;i<layers.size();++i){auto &layer=data->_animLayers[i];const int count=layer.GetFrameCount();
    if(count<0||(count&&!layer._animFrames)||layers[i]<-1||(count==0?layers[i]>0:layers[i]>=count))return false;}
  return true;
}
// 原版Collectible的Empty长度为0，但可选中且停在phase0；不把它当无动画或可播放时间轴。
bool PhaseInAnimation(const AnimationData &data,float phase,bool playing){
  return data._length>=0&&std::isfinite(phase)&&
      (data._length==0?phase==0&&!playing:phase<data._length&&(phase>=0||(!playing&&phase==-1)));
}
// J273 405F50/406430只选本ANM动画。写前检查目标全部selector，避免写后才发现缺层。
bool TargetSelectors(ANM2 &sprite,const std::string &name,int frame,float phase,bool playing,const std::vector<int> &layers,const std::vector<int> &nulls){
  if(name.empty())return phase==0&&layers.empty()&&nulls.empty();auto *d=sprite.GetAnimationData(name);
  bool owned=false;for(int i=0;sprite._animData&&i<sprite._animCount;++i)if(d==sprite._animData+i){owned=true;break;}
  if(!owned||!d||!PhaseInAnimation(*d,phase,playing)||frame!=int(std::floor(phase))||d->_layerCount!=int(layers.size())||d->_nullLayerCount!=int(nulls.size())||(!layers.empty()&&!d->_animLayers))return false;
  for(unsigned i=0;i<layers.size();++i){auto &l=d->_animLayers[i];const int n=l.GetFrameCount();if(n<0||(n&&!l._animFrames)||layers[i]<-1||(n?layers[i]>=n:layers[i]>0))return false;}
  const unsigned char *descriptors=nullptr;std::memcpy(&descriptors,d->_nullLayersPtr,4);if(!nulls.empty()&&!descriptors)return false;
  for(unsigned i=0;i<nulls.size();++i){const void *frames=nullptr;int n=0;std::memcpy(&frames,descriptors+i*16+8,4);std::memcpy(&n,descriptors+i*16+12,4);if(n<0||n>1000000||(n&&!frames)||nulls[i]<-1||(n?nulls[i]>=n:nulls[i]>0))return false;}return true;
}
bool SameSelectorStorage(const AnimationState &old,const AnimationData *next){return old._animData&&next&&old._animData->_layerCount==next->_layerCount&&old._animData->_nullLayerCount==next->_nullLayerCount&&(!next->_layerCount||old._layerFrames)&&(!next->_nullLayerCount||old._nullLayerFrames);}
bool ExactPhase(AnimationState &state,float phase,bool playing,const std::vector<int> &layers,const std::vector<int> &nulls) {
  const auto *data=state._animData;
  if(!data)return phase==0&&layers.empty()&&nulls.empty();
  if(!PhaseInAnimation(*data,phase,playing)||
     data->_layerCount!=int(layers.size())||data->_nullLayerCount!=int(nulls.size())||
     (!layers.empty()&&(!data->_animLayers||!state._layerFrames))||(!nulls.empty()&&!state._nullLayerFrames))return false;
  // 空层0/-1均不会被原生RenderFrame解引用；有帧层允许独立的-1隐藏姿态。
  if(!LayerSelectors(state,layers)||!NullSelectors(state,nulls))return false;
  for(unsigned i=0;i<layers.size();++i)state._layerFrames[i]=layers[i];
  for(unsigned i=0;i<nulls.size();++i)state._nullLayerFrames[i]=nulls[i];
  state._animFrame=phase;return true;
}
// 不使用ANM2::Update。保留draw消费的播放标志，但不推进相位或消费动画事件。
bool SetPose(ANM2 &sprite,const ClientResourcePose &pose,const char *&reason,bool load_graphics=true) {
  if(!sprite._loaded||sprite._filename!=pose.path){reason="resource_load_failed";return false;}
  if(sprite._layerCount!=pose.layers.size()||sprite._layerCount>64||(sprite._layerCount&&!sprite._layerState)){reason="resource_layer_count";return false;}
  if((!pose.animation.empty()&&!AnimationReady(sprite,pose.animation,pose.frame,false))||
      (pose.overlay_present&&!AnimationReady(sprite,pose.overlay_animation,pose.overlay_frame,true))){reason="resource_animation_frame";return false;}
  // 价格Sprite用停止态-1全局相位及独立数字层；只接受该精确哨兵，播放负相位仍拒绝。
  const auto phase_valid=[](int frame,float phase,bool playing){return std::isfinite(phase)&&
      phase>=-1&&(phase>=0||(!playing&&phase==-1))&&double(frame)==std::floor(double(phase));};
  if((!pose.animation.empty()&&!phase_valid(pose.frame,pose.main_phase,pose.main_playing))||
      (pose.overlay_present&&!phase_valid(pose.overlay_frame,pose.overlay_phase,pose.overlay_playing))){reason="resource_exact_phase";return false;}
  if(!pose.exact_phase||!TargetSelectors(sprite,pose.animation,pose.frame,pose.main_phase,pose.main_playing,pose.main_selectors,pose.main_null_selectors)||
     (pose.overlay_present&&!TargetSelectors(sprite,pose.overlay_animation,pose.overlay_frame,pose.overlay_phase,pose.overlay_playing,pose.overlay_selectors,pose.overlay_null_selectors))){reason="resource_target_selectors";return false;}
  for(const auto &layer:pose.layers){
    auto *native=sprite.GetLayer(int(layer.id));
    if(!native||!native->_layerData||native->_layerData->ID!=layer.id||native->_layerData->name!=layer.name){reason="resource_layer_identity";return false;}
    if(native->_spriteSheetPath!=layer.sheet){if(!load_graphics){reason="resource_graphics_changed";return false;}auto path=layer.sheet;sprite.ReplaceSpritesheet(int(layer.id),path);}
  }
  if(load_graphics)sprite.LoadGraphics(false);
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
    // reset=false仅在两种selector数量完全相同时采用；403EC0的resize分支不用于本方。
    const bool reset=!SameSelectorStorage(sprite._animState,sprite.GetAnimationData(pose.animation));
    if(!sprite.SetAnimation(pose.animation.c_str(),reset)){reason="resource_main_animation";return false;}
  }
  if(pose.overlay_present){
    const bool reset=!SameSelectorStorage(sprite._overlayAnimState,sprite.GetAnimationData(pose.overlay_animation));
    if(!sprite.SetOverlayAnimation(pose.overlay_animation.c_str(),reset)){reason="resource_overlay_animation";return false;}
  }else{
    // 和公开 RemoveOverlay 一样，只撤销动画选择，不析构内嵌 AnimationState。
    sprite._overlayAnimState._animData=nullptr;
  }
  if(!pose.exact_phase||!ExactPhase(sprite._animState,pose.main_phase,pose.main_playing,pose.main_selectors,pose.main_null_selectors)||
     (pose.overlay_present&&!ExactPhase(sprite._overlayAnimState,pose.overlay_phase,pose.overlay_playing,pose.overlay_selectors,pose.overlay_null_selectors))){reason="resource_exact_phase";return false;}
  sprite._animState._isPlaying=pose.main_playing;sprite._overlayAnimState._isPlaying=pose.overlay_playing;
  sprite._animState._currentlyTriggeredEvents=0;sprite._animState._previouslyTriggeredEvents=0;
  sprite._overlayAnimState._currentlyTriggeredEvents=0;sprite._overlayAnimState._previouslyTriggeredEvents=0;
  for(const auto &layer:pose.layers){
    auto *native=sprite.GetLayer(int(layer.id));
    // 完整wire selector已经由ExactPhase安装；不再用时间帧重算并覆盖它。
    native->_size=Vector(layer.size.x,layer.size.y);native->_pos=Vector(layer.position.x,layer.position.y);
    native->_cropOffset=Vector(layer.crop.x,layer.crop.y);native->_rotation=layer.rotation;
    native->_color=Color(layer.color);native->_visible=layer.visible;
    native->_flipX=layer.flip_x;native->_flipY=layer.flip_y;native->_bitflags=layer.flags;
    native->_wrapSMode=layer.wrap_s;native->_wrapTMode=layer.wrap_t;
    native->_blendMode=BlendMode(int(layer.blend[0]),int(layer.blend[1]),int(layer.blend[2]),int(layer.blend[3]),int(layer.blend[4]));
  }
  sprite._offset=Vector(pose.offset.x,pose.offset.y);sprite._scale=Vector(pose.scale.x,pose.scale.y);
  sprite._rotation=pose.rotation;sprite._color=Color(pose.color);sprite._flipX=pose.flip_x;sprite._flipY=pose.flip_y;
  if(pose.champion_color_present)sprite._championColor=Color(pose.champion_color);
  sprite._playbackSpeed=pose.playback_speed;sprite._bitflags=pose.flags;sprite._overlayRenderFirst=pose.overlay_first;
  if(!ClientResourceApplyShaders(&sprite,pose,false)){reason="resource_shader_install";return false;}
  reason="resource_complete";return true;
}
// 同实例/文件/图像pair可跨帧与动画复用；所有目标selector在第一次写前验证。
// 这使位置/颜色/旋转等姿态变化不重复Load/LoadGraphics，资源或实例变化立即失效。
bool ReusableGraphics(ANM2 &sprite,const ClientResourcePose &pose) {
  if(!pose.loaded||!sprite._loaded||sprite._filename!=pose.path||sprite._layerCount!=pose.layers.size()||!pose.exact_phase)return false;
  if(!TargetSelectors(sprite,pose.animation,pose.frame,pose.main_phase,pose.main_playing,pose.main_selectors,pose.main_null_selectors)||
     (pose.overlay_present&&!TargetSelectors(sprite,pose.overlay_animation,pose.overlay_frame,pose.overlay_phase,pose.overlay_playing,pose.overlay_selectors,pose.overlay_null_selectors)))return false;
  for(const auto &layer:pose.layers){auto *v=sprite.GetLayer(int(layer.id));
    if(!v||!v->_layerData||v->_layerData->ID!=layer.id||v->_layerData->name!=layer.name||v->_spriteSheetPath!=layer.sheet||
      !v->_spriteSheetLoaded||!v->_spriteSheet.image||!v->_spriteSheet.counter||v->_spriteSheet.image->GetWidth()<=0||v->_spriteSheet.image->GetHeight()<=0)return false;}
  return true;
}
}
bool ClientResourcePoseReusable(void *pointer,const ClientResourcePose &pose) noexcept {
  if(!pointer||!ResourceWindow())return false;try{auto &sprite=*static_cast<ANM2 *>(pointer);return pose.loaded?ReusableGraphics(sprite,pose):!sprite._loaded;}catch(...){return false;}
}
ClientResourceStatistics ClientResourcesStatistics() noexcept {return IsMain()?resource_statistics:ClientResourceStatistics{};}
bool ClientNativeCpuQueuesEmpty() noexcept {
  if(!IsMain()||!__ptr_g_KAGE_Graphics_ImageManager)return false;
  const auto &manager=*__ptr_g_KAGE_Graphics_ImageManager;
  // ImageManager 真实普通和透明 raw 提交队列；不把此结果解释成 GPU idle。
  return manager._frameImages.empty()&&manager._transparentBatches.empty();
}
ClientResourceResult ClientApplyResourcePose(void *pointer,const ClientResourcePose &pose) noexcept {
  ClientResourceResult result;
  if(!pointer||!ResourceWindow()||!ClientNativeCpuQueuesEmpty()){result.reason="resource_context_or_inflight";return result;}
  // 同一原生实例上的完整资源/姿态相同则复用；换实例或任意姿态变化立即走正常安装。
  if(ClientResourcePoseMatches(pointer,pose)){++resource_statistics.unchanged;result.complete=true;result.layers=unsigned(pose.layers.size());result.reason="resource_unchanged";return result;}
  try{auto &sprite=*static_cast<ANM2 *>(pointer);if(ReusableGraphics(sprite,pose)){
    result.written=true;result.complete=SetPose(sprite,pose,result.reason,false);result.layers=result.complete?unsigned(pose.layers.size()):0;
    if(result.complete){++resource_statistics.reused_graphics;result.reason="resource_graphics_reused";}return result;
  }}catch(...){result.written=true;result.reason="resource_native_exception";return result;}
  if(!pose.loaded){
    if(!pose.path.empty()||!pose.layers.empty()||!pose.animation.empty()||pose.overlay_present||!pose.exact_phase||pose.main_phase!=0||!pose.main_selectors.empty()||!pose.main_null_selectors.empty()){
      result.reason="resource_unloaded_description";return result;}
    try{auto &sprite=*static_cast<ANM2 *>(pointer);result.written=true;
      // 空资源是已构造成员的合法状态。使用临时ANM交换/重绑layer owner，绝不析构后重建内嵌成员。
      if(sprite._loaded||!sprite._filename.empty()||sprite._layerCount||sprite._animState._animData||sprite._overlayAnimState._animData){
        auto *empty=new(std::nothrow) ResourceCandidate;if(!empty){result.reason="resource_owner_budget";return result;}
        empty->sprite=static_cast<ANM2 *>(::operator new(sizeof(ANM2),std::nothrow));if(!empty->sprite){delete empty;result.reason="resource_allocation";return result;}
        ClientRetainedTicket ticket{};if(!ClientRetainGraphics(empty,ReleaseCandidate,ticket)){::operator delete(empty->sprite);delete empty;result.reason="resource_retained_budget";return result;}
        // 原生异常保留ticket责任，不能在不明前缀中再次析构。
        try{empty->constructor_entered=true;empty->sprite->constructor();empty->constructed=true;Isaac::SwapANM2(&sprite,empty->sprite);}
        catch(...){empty->uncertain=true;ClientRetireGraphics(ticket);throw;}
        ClientRetireGraphics(ticket);
      }
      sprite._offset=Vector(pose.offset.x,pose.offset.y);sprite._scale=Vector(pose.scale.x,pose.scale.y);sprite._rotation=pose.rotation;sprite._color=Color(pose.color);
      sprite._flipX=pose.flip_x;sprite._flipY=pose.flip_y;sprite._playbackSpeed=pose.playback_speed;sprite._bitflags=pose.flags;sprite._overlayRenderFirst=pose.overlay_first;
      if(pose.champion_color_present)sprite._championColor=Color(pose.champion_color);
      sprite._animState.Stop();sprite._overlayAnimState.Stop();sprite._animState._currentlyTriggeredEvents=0;sprite._animState._previouslyTriggeredEvents=0;
      sprite._overlayAnimState._currentlyTriggeredEvents=0;sprite._overlayAnimState._previouslyTriggeredEvents=0;
      result.complete=ClientResourceApplyShaders(&sprite,pose,false);result.reason=result.complete?"resource_unloaded_state_installed":"resource_shader_install";
    }catch(...){result.reason="resource_native_exception";}return result;
  }
  if(pose.path.empty()||pose.layers.size()>64){result.reason="resource_description";return result;}
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
    auto path=pose.path;++resource_statistics.candidate_loads;owner->sprite->Load(path,false);
    if(!SetPose(*owner->sprite,pose,result.reason)){ClientRetireGraphics(ticket);return result;}
    auto &sprite=*static_cast<ANM2 *>(pointer);
    result.written=true;
    if(sprite._filename!=pose.path)sprite.Load(path,false);
    result.complete=SetPose(sprite,pose,result.reason);
    result.layers=result.complete?unsigned(pose.layers.size()):0;
  }catch(...){owner->uncertain=true;result.reason="resource_native_exception";}
  ClientRetireGraphics(ticket);return result;
}
bool ClientResourcePoseMatches(void *pointer,const ClientResourcePose &pose,const char **reason) noexcept {
  const auto mismatch=[&](const char *field){if(reason)*reason=field;return false;};
  if(!pointer||!ResourceWindow())return mismatch("resource_match_context");
  const auto &sprite=*static_cast<ANM2 *>(pointer);
  if(sprite._animState._isPlaying!=pose.main_playing||sprite._overlayAnimState._isPlaying!=pose.overlay_playing)return mismatch("resource_playing_postcondition");
  if(pose.champion_color_present&&!(sprite._championColor==Color(pose.champion_color)))return mismatch("resource_champion_color_postcondition");
  if(!ClientResourceApplyShaders(pointer,pose,true))return mismatch("resource_shader_postcondition");
  if(sprite._loaded!=pose.loaded||sprite._filename!=pose.path||sprite._layerCount!=pose.layers.size()||bool(sprite._animState._animData)!=!pose.animation.empty()||
     (sprite._animState._animData&&(sprite._animState._animData->_name!=pose.animation||sprite._animState._animFrame!=pose.main_phase||sprite._animState._isPlaying!=pose.main_playing)))return mismatch("resource_animation_postcondition");
  if(bool(sprite._overlayAnimState._animData)!=pose.overlay_present)return mismatch("resource_overlay_presence_postcondition");
  if(pose.overlay_present&&(sprite._overlayAnimState._animData->_name!=pose.overlay_animation||sprite._overlayAnimState._animFrame!=pose.overlay_phase||sprite._overlayAnimState._isPlaying!=pose.overlay_playing))return mismatch("resource_overlay_animation_postcondition");
  const auto sameState=[](const AnimationState &state,const std::vector<int> &layers,const std::vector<int> &nulls){
    if(!state._animData)return layers.empty()&&nulls.empty()&&state._currentlyTriggeredEvents==0&&state._previouslyTriggeredEvents==0;
    if(!state._animData||state._animData->_layerCount!=int(layers.size())||state._animData->_nullLayerCount!=int(nulls.size()))return false;
    if(!LayerSelectors(state,layers)||!NullSelectors(state,nulls))return false;
    for(unsigned i=0;i<layers.size();++i)if(!state._layerFrames||state._layerFrames[i]!=layers[i])return false;
    for(unsigned i=0;i<nulls.size();++i)if(!state._nullLayerFrames||state._nullLayerFrames[i]!=nulls[i])return false;
    return state._currentlyTriggeredEvents==0&&state._previouslyTriggeredEvents==0;
  };
  if(!sameState(sprite._animState,pose.main_selectors,pose.main_null_selectors)||
     (pose.overlay_present&&!sameState(sprite._overlayAnimState,pose.overlay_selectors,pose.overlay_null_selectors)))return mismatch("resource_animation_selectors_postcondition");
  if(sprite._offset.x!=pose.offset.x||sprite._offset.y!=pose.offset.y||sprite._scale.x!=pose.scale.x||sprite._scale.y!=pose.scale.y||
     sprite._rotation!=pose.rotation||!(sprite._color==Color(pose.color))||sprite._flipX!=pose.flip_x||sprite._flipY!=pose.flip_y||
     sprite._playbackSpeed!=pose.playback_speed||sprite._bitflags!=pose.flags||sprite._overlayRenderFirst!=pose.overlay_first)return mismatch("resource_pose_scalars_postcondition");
  for(const auto &layer:pose.layers){
    if(layer.id>=sprite._layerCount)return mismatch("resource_layer_count_postcondition");
    const auto &native=sprite._layerState[layer.id];
    if(native._spriteSheetPath!=layer.sheet||!native._spriteSheetLoaded||!native._spriteSheet.image||!native._spriteSheet.counter)return mismatch("resource_sheet_postcondition");
    if(!native._layerData||native._layerData->ID!=layer.id||native._layerData->name!=layer.name||
       native._size.x!=layer.size.x||native._size.y!=layer.size.y||native._pos.x!=layer.position.x||native._pos.y!=layer.position.y||
       native._cropOffset.x!=layer.crop.x||native._cropOffset.y!=layer.crop.y||native._rotation!=layer.rotation||
       !(native._color==Color(layer.color))||native._visible!=layer.visible||native._flipX!=layer.flip_x||native._flipY!=layer.flip_y||
       native._bitflags!=layer.flags||native._wrapSMode!=layer.wrap_s||native._wrapTMode!=layer.wrap_t||
       native._blendMode._equation!=layer.blend[0]||native._blendMode._rgbSourceFactor!=layer.blend[1]||
       native._blendMode._rgbDestFactor!=layer.blend[2]||native._blendMode._alphaSourceFactor!=layer.blend[3]||native._blendMode._alphaDestFactor!=layer.blend[4])return mismatch("resource_layer_pose_postcondition");
  }
  return true;
}
} // namespace cs::game
