#include "IsaacRepentance.h"
#include "game/client/scene/client_preparation_native.h"
#include "game/client/scene/client_lifetime.h"
#include "game/client/entity/client_entity_budget.h"
#include "game/client/isolation/client_player_isolation.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "runtime.h"
#include <algorithm>
#include <cstring>
#include <new>
#include <vector>
#include <windows.h>

namespace cs::game {
namespace {
using Status=ClientPreparationStatus;
ClientPreparationResult Ready(){return {Status::Ready,"binary_preparation_ready"};}
ClientPreparationResult Bad(const char *r){return {Status::Failed,r};}
ClientPreparationResult Missing(const char *r){return {Status::Missing,r};}
bool Window() noexcept {
  void *dc=nullptr,*gl=nullptr;
  return IsMain()&&ClientBusy()&&ClientPersistenceBlocked()&&TopOwner()==Owner::Manager&&ContextHealthy()&&!InBlockedContext()&&
    ClientRendererHandles(dc,gl)&&dc&&gl&&wglGetCurrentDC()==dc&&wglGetCurrentContext()==gl;
}
// 构造、释放都固定原 context／VM；异常前缀保留在原 owner，不用清零伪造恢复。
struct NativeContext {
  void *dc=nullptr,*gl=nullptr,*vm=nullptr,*manager=nullptr;
  void Capture(){ClientRendererHandles(dc,gl);vm=ClientLuaStateIdentity();manager=g_Manager;}
  bool Current()const noexcept {void *d=nullptr,*g=nullptr;return Window()&&ClientLuaStateIdentity()==vm&&
    g_Manager==manager&&ClientRendererHandles(d,g)&&d==dc&&g==gl;}
};
template<class T> T *Allocate(ClientPreparationAllocator a) noexcept {
  if(!a.allocate||!a.release)return nullptr;
  void *raw=nullptr;
  try {raw=a.allocate(a.context,sizeof(T),alignof(T));return raw?new(raw) T(a):nullptr;}
  catch(...){if(raw)a.release(a.context,raw,sizeof(T),alignof(T));return nullptr;}
}
template<class T> void Delete(T *value) noexcept {
  const auto a=value->allocator;value->~T();a.release(a.context,value,sizeof(T),alignof(T));
}
template<class O,class F> auto NativeCall(O &owner,F &&body) {
  owner.uncertain=true;auto value=body();owner.uncertain=false;return value;
}
template<class O,class F> void NativeVoid(O &owner,F &&body) {
  owner.uncertain=true;body();owner.uncertain=false;
}
template<class T> struct NativeAllocator {
  using value_type=T;ClientPreparationAllocator owner;
  explicit NativeAllocator(ClientPreparationAllocator a):owner(a){}
  template<class U> NativeAllocator(const NativeAllocator<U> &a):owner(a.owner){}
  T *allocate(std::size_t n){if(n>SIZE_MAX/sizeof(T))throw std::bad_alloc();return static_cast<T *>(owner.allocate(owner.context,n*sizeof(T),alignof(T)));}
  void deallocate(T *p,std::size_t n)noexcept{owner.release(owner.context,p,n*sizeof(T),alignof(T));}
  template<class U> bool operator==(const NativeAllocator<U> &a)const noexcept{return owner.context==a.owner.context;}
  template<class U> bool operator!=(const NativeAllocator<U> &a)const noexcept{return !(*this==a);}
};
std::string Path(std::string_view path){return std::string(path.data(),path.size());}
bool SamePath(std::string a,std::string_view b) {
  const auto normalize=[](std::string &p){std::replace(p.begin(),p.end(),'\\','/');
    for(const char *root:{"resources/","resources-dlc3/","resources-repentogon/"})if(p.rfind(root,0)==0){p.erase(0,std::strlen(root));break;}};
  auto second=Path(b);normalize(a);normalize(second);return a==second;
}
struct AudioPair {void *source=nullptr,*counter=nullptr;};
// J273 7D7AC2／906FD2：本地 SoundManager vtable+20h，hidden out + path + stream + flags。
// 只加载，不调用 Play、不抽 sample；VM／音频后端变化时不继续使用旧 pair。
bool LoadAudio(AudioPair &out,std::string_view path,bool stream) {
  if(!*reinterpret_cast<const unsigned char *>(Base+0x7ce5a4)||out.source||out.counter)return false;
  std::string logical=Path(path),resolved;
  g_Manager->_modManager.TryRedirectPath(&resolved,&logical);
  auto *manager=reinterpret_cast<void *>(Base+0x7ce5a0);auto **vtable=*reinterpret_cast<void ***>(manager);
  using Load=void *(__thiscall *)(void *,AudioPair *,const char *,bool,unsigned);
  reinterpret_cast<Load>(vtable[8])(manager,&out,resolved.c_str(),stream,0);
  return out.source&&out.counter;
}
bool ReleaseAudio(AudioPair &pair) {
  if(!pair.counter)return !pair.source;
  auto **vtable=*reinterpret_cast<void ***>(pair.counter);
  const bool last=reinterpret_cast<bool (__thiscall *)(void *)>(vtable[3])(pair.counter);
  if(last){auto release=*reinterpret_cast<void (__cdecl **)(AudioPair *)>(Base+0x7eca14);if(!release)return false;release(&pair);}
  pair={};return true;
}
}
struct ClientPreparedResource {
  explicit ClientPreparedResource(ClientPreparationAllocator a):allocator(a){}
  ClientPreparationAllocator allocator;
  NativeContext context;
  std::uint8_t kind=255;
  bool entered=false,constructed=false,ready=false,uncertain=false,release_entered=false;
  ANM2 *sprite=nullptr;
  KAGE_SmartPointer_ImageBase image;
  RoomConfig_Room *room=nullptr;
  ItemConfig_Item *item=nullptr;
  std::unique_ptr<ItemConfig_Item> procedural_item;
  EntityConfig_Entity *entity=nullptr;
  KAGE_Graphics_Shader *shader=nullptr;
  AnimationData *animation=nullptr;
  ClientPreparedResource *dependency=nullptr,*second=nullptr;
  AudioPair audio,intro;
};
namespace {
bool Geometry(KAGE_Graphics_ImageBase *image,const state::ResourceLocalDescription &d,unsigned first) {
  return image&&image->GetWidth()==int(d.numbers[first])&&image->GetHeight()==int(d.numbers[first+1])&&
    image->GetPaddedWidth()==int(d.numbers[first+2])&&image->GetPaddedHeight()==int(d.numbers[first+3]);
}
// 纯路径资源的宽高由本地解码确定，仍核对图像自身的逻辑和存储尺寸关系。
bool LocalImageGeometry(KAGE_Graphics_ImageBase &image) {
  const auto width=image.GetWidth(),height=image.GetHeight();
  const auto stored_width=image.GetPaddedWidth(),stored_height=image.GetPaddedHeight();
  return width>0&&height>0&&stored_width>=width&&stored_height>=height&&stored_width<=65535&&stored_height<=65535;
}
bool Animation(ClientPreparedResource &o,const state::ResourceLocalDescription &d,ClientPreparedResource *file) {
  if(!file||!file->ready||file->kind!=0||!file->sprite)return false;
  auto name=Path(d.text[0]);auto *a=file->sprite->GetAnimationData(name);
  bool owned=false;for(unsigned i=0;i<file->sprite->_animCount;++i)owned|=a==file->sprite->_animData+i;
  if(!owned||!a||a->_length!=d.numbers[2]||a->_layerCount!=d.layer_count[0]||a->_nullLayerCount!=d.layer_count[1]||
      (a->_layerCount&&!a->_animLayers))return false;
  for(unsigned i=0;i<d.layer_count[0];++i){auto &l=a->_animLayers[i];if(l.GetLayerID()!=d.layers[0][i].id||l.GetFrameCount()!=d.layers[0][i].frames||
      (l.GetFrameCount()&&!l._animFrames))return false;}
  const unsigned char *nulls=nullptr;std::memcpy(&nulls,a->_nullLayersPtr,4);if(a->_nullLayerCount&&!nulls)return false;
  for(unsigned i=0;i<d.layer_count[1];++i){unsigned id=0,count=0;void *frames=nullptr;
    std::memcpy(&id,nulls+i*16,4);std::memcpy(&frames,nulls+i*16+8,4);std::memcpy(&count,nulls+i*16+12,4);
    if(id!=d.layers[1][i].id||count!=d.layers[1][i].frames||(count&&!frames))return false;}
  o.animation=a;o.dependency=file;return true;
}
}
bool ClientPreparationWindow(const ClientNativeRoots &r,void *vm) noexcept {
  return Window()&&g_Game&&r.game==g_Game&&r.room==g_Game->_room&&r.room&&ClientLuaStateIdentity()==vm;
}
ClientNativeRoots ClientPreparationRoots() noexcept {
  if(!Window()||!g_Game||!g_Game->_room)return {};
  return {g_Game,g_Game->_room,g_Game->_room->_descriptor,g_Game->_room->_Camera};
}
ClientPreparedResource *ClientPreparedResourceCreate(ClientPreparationAllocator a) noexcept {
  return Window()&&ClientNativeCpuQueuesEmpty()?Allocate<ClientPreparedResource>(a):nullptr;
}
ClientPreparationResult ClientPreparedResourceLoad(ClientPreparedResource *o,const state::ResourceLocalDescription &d,
    ClientPreparedResource *first,ClientPreparedResource *second) noexcept {
  if(!o||!Window()||!ClientBinaryPreparationScopeActive(false))return Bad("binary_resource_context");
  if(!ClientNativeCpuQueuesEmpty())return {Status::Waiting,"binary_resource_cpu_borrow"};
  if(d.kind==9&&!*reinterpret_cast<const unsigned char *>(Base+0x7ce5a4))return {Status::Waiting,"binary_resource_audio_backend"};
  if(o->entered)return o->ready&&o->context.Current()?Ready():Bad("binary_resource_prefix_retained");
  o->kind=d.kind;o->context.Capture();o->entered=true;
  try {
    switch(d.kind) {
    case 0:{o->sprite=static_cast<ANM2 *>(o->allocator.allocate(o->allocator.context,sizeof(ANM2),alignof(ANM2)));
        NativeVoid(*o,[&]{o->sprite->constructor();});o->constructed=true;auto path=Path(d.text[0]);NativeVoid(*o,[&]{o->sprite->Load(path,false);});
        if(!o->sprite->_loaded||!SamePath(o->sprite->_filename,d.text[0])||o->sprite->_layerCount>64||
            (o->sprite->_layerCount&&!o->sprite->_layerState))return Missing("binary_resource_anm2_load");break;}
      case 1:case state::kResourceImagePathKind:{auto path=Path(d.text[0]);NativeVoid(*o,[&]{Manager::LoadImage(&o->image,path.c_str(),__ptr_g_VertexAttributeDescriptor_Position,false);});
        if(!o->image.image||!o->image.counter||!o->image.image->_name||!SamePath(o->image.image->_name,d.text[0]))return Missing("binary_resource_image_load");
        if(d.kind==1?!Geometry(o->image.image,d,1):!LocalImageGeometry(*o->image.image))return Bad("binary_resource_image_geometry");break;}
      case 2:{if(d.numbers[0]>=20||!__ptr_g_AllShaders)return Bad("binary_resource_shader_type");
        o->shader=__ptr_g_AllShaders[d.numbers[0]];if(!o->shader||!o->shader->_initialized||!o->shader->_glProgram||o->shader->_glProgram==UINT32_MAX)
          return Missing("binary_resource_shader_unavailable");break;}
      case 3:{auto *raw=o->allocator.allocate(o->allocator.context,sizeof(RoomConfig_Room),alignof(RoomConfig_Room));
        o->room=static_cast<RoomConfig_Room *>(raw);new(raw) RoomConfig_Room();o->constructed=true;o->room->StageId=int(d.numbers[0]);o->room->Type=int(d.numbers[1]);
        o->room->Subtype=int(d.numbers[2]);o->room->Width=char(d.numbers[3]);o->room->Height=char(d.numbers[4]);o->room->Shape=int(d.numbers[5]);break;}
      case 4:{
        const auto colors=d.numbers.begin()+10;
        const bool white=std::all_of(colors,colors+4,[](auto n){return n==0x3f800000;});
        const auto size=d.numbers[1];
        const bool atlas=(size==1024||size==512)&&d.numbers[2]==size&&d.numbers[3]==size&&d.numbers[4]==size&&
          std::all_of(colors,colors+4,[](auto n){return n==0;});
        if(d.numbers[0]!=1||d.numbers[9]!=1||d.numbers[14]||(!white&&!atlas))return Bad("binary_resource_surface_recipe");
        NativeVoid(*o,[&]{KAGE_Graphics_ImageManager::CreateProceduralImage_Internal(&o->image,unsigned(size),unsigned(d.numbers[2]),nullptr,
          atlas?"IsaacCS.ProceduralAtlas":"IsaacCS.Shape",0,0,white?KColor(1,1,1,1):KColor(0,0,0,0));});
        if(!o->image.counter||!Geometry(o->image.image,d,1))return Bad("binary_resource_shape_geometry");break;}
      case 6:if(!Animation(*o,d,first))return Bad("binary_resource_animation_shape");break;
      case 7:{
        const auto wire_id=std::uint32_t(d.numbers[1]);std::int32_t id=0;std::memcpy(&id,&wire_id,sizeof(id));
        const auto *gfx=first&&first->ready&&(first->kind==1||first->kind==state::kResourceImagePathKind)?first->image.image:nullptr;
        if(id<0){
          if(id< -1024||d.numbers[0]==0||d.numbers[0]==2)return Bad("binary_resource_procedural_id");
          o->procedural_item=std::make_unique<ItemConfig_Item>();o->item=o->procedural_item.get();
          o->item->id=id;o->item->type=int(d.numbers[0]);o->item->maxCooldown=std::uint32_t(d.numbers[2]);
          if(gfx)o->item->gfxFilename=gfx->_name;
          if(second&&second->ready&&second->kind==0)o->item->costume.anm2Path=second->sprite->_filename;
        }else o->item=NativeCall(*o,[&]{
          // 角色服装使用真实NullItem配置，不能用同号Collectible冒充；饰品同样按类型查目录。
          if(d.numbers[0]==0)return g_Manager->_itemConfig.GetNullItem(unsigned(id));
          return d.numbers[0]==2?g_Manager->_itemConfig.GetTrinket(unsigned(id)):g_Manager->_itemConfig.GetCollectible(id);
        });
        if(!o->item)return Missing("binary_resource_item_missing");
        if(o->item->type!=d.numbers[0]||o->item->id!=id||o->item->maxCooldown!=std::uint32_t(d.numbers[2]))return Bad("binary_resource_item_identity");
        if((d.numbers[3]!=UINT16_MAX&&(!gfx||!SamePath(o->item->gfxFilename,gfx->_name)))||
           (d.numbers[4]!=UINT16_MAX&&(!second||!second->ready||second->kind!=0||!SamePath(o->item->costume.anm2Path,second->sprite->_filename))))
          return Bad("binary_resource_item_paths");o->dependency=first;o->second=second;break;}
      case 8:{o->entity=NativeCall(*o,[&]{return g_Manager->_entityConfig.GetEntity(int(d.numbers[0]),int(d.numbers[1]),int(std::uint32_t(d.numbers[2])));});
        if(!o->entity)return Missing("binary_resource_entity_config_missing");std::uint32_t shadow=0;std::memcpy(&shadow,&o->entity->shadowSize,4);
        if(std::uint32_t(o->entity->id)!=d.numbers[0]||std::uint32_t(o->entity->variant)!=d.numbers[1]||std::uint32_t(o->entity->subtype)!=d.numbers[2]||
           std::uint32_t(o->entity->portrait)!=d.numbers[4]||shadow!=d.numbers[5]||
           (d.numbers[3]!=UINT16_MAX&&(!first||!first->ready||first->kind!=0||!SamePath(o->entity->anm2Path,first->sprite->_filename))))return Bad("binary_resource_entity_config_identity");
        o->dependency=first;break;}
      // 9F8170 的正常 SFX load-on-play 与音乐加载均采用 mode=1；准备不调用选样本／Play。
      case 9:if(!NativeCall(*o,[&]{return LoadAudio(o->audio,d.text[0],true);})||
          (!d.text[1].empty()&&!NativeCall(*o,[&]{return LoadAudio(o->intro,d.text[1],true); })))
        return Missing("binary_resource_audio_load");break;
      default:return Bad("binary_resource_kind_not_supported");
    }
    o->ready=true;return Ready();
  }catch(...){return Bad(o->uncertain?"binary_resource_native_exception_retained":"binary_resource_allocation");}
}
bool ClientPreparedResourceCurrent(const ClientPreparedResource *o) noexcept {
  return o&&o->ready&&!o->uncertain&&!o->release_entered&&o->context.Current()&&
    (o->kind!=9||*reinterpret_cast<const unsigned char *>(Base+0x7ce5a4))&&
    (o->kind!=2||(o->shader&&o->shader->_initialized&&o->shader->_glProgram&&o->shader->_glProgram!=UINT32_MAX))&&
    (!o->dependency||ClientPreparedResourceCurrent(o->dependency))&&(!o->second||ClientPreparedResourceCurrent(o->second));
}
void *ClientPreparedResourceBorrow(ClientPreparedResource *o) noexcept {
  if(!ClientPreparedResourceCurrent(o))return nullptr;
  switch(o->kind){case 0:return o->sprite;case 1:case 4:case state::kResourceImagePathKind:return &o->image;case 2:return o->shader;case 3:return o->room;
    case 6:return o->animation;case 7:return o->item;case 8:return o->entity;case 9:return &o->audio;default:return nullptr;}
}
bool ClientPreparedResourceRelease(ClientPreparedResource *&o) noexcept {
  if(!o)return true;if(!Window()||!ClientBinaryPreparationScopeActive(true)||!ClientNativeCpuQueuesEmpty()||o->uncertain||o->release_entered||
      (o->entered&&!o->context.Current())||(o->sprite&&!o->constructed))return false;
  o->release_entered=true;
  try {if(o->sprite){o->sprite->destructor();o->allocator.release(o->allocator.context,o->sprite,sizeof(ANM2),alignof(ANM2));o->sprite=nullptr;}
    if(!ReleaseAudio(o->intro)||!ReleaseAudio(o->audio))return false;
    if(o->room){if(o->constructed)o->room->~RoomConfig_Room();o->allocator.release(o->allocator.context,o->room,sizeof(RoomConfig_Room),alignof(RoomConfig_Room));o->room=nullptr;}
    Delete(o);o=nullptr;return true;
  }catch(...){o->uncertain=true;return false;}
}

// 音乐具名目录与 Boss 演出依赖只加载本地资产，不执行 Play／Show／StartBossIntro。
// Boss 的实际原生选择和副作用隔离仍归事件安装器；这里拥有其有限配置闭包的资源。
struct ClientPreparedNativeNeed {
  explicit ClientPreparedNativeNeed(ClientPreparationAllocator a):allocator(a),images(NativeAllocator<KAGE_SmartPointer_ImageBase>(a)),
    sprites(NativeAllocator<ClientPreparedResource *>(a)){}
  ClientPreparationAllocator allocator;NativeContext context;
  std::array<AudioPair,14> audio{};
  std::vector<KAGE_SmartPointer_ImageBase,NativeAllocator<KAGE_SmartPointer_ImageBase>> images;
  std::vector<ClientPreparedResource *,NativeAllocator<ClientPreparedResource *>> sprites;
  bool entered=false,ready=false,uncertain=false,release_entered=false;
};
namespace {
bool NeedImage(ClientPreparedNativeNeed &o,const std::string &path) {
  if(path.empty())return true;if(path.size()>4096||o.images.size()>=1024)return false;
  o.images.emplace_back();auto &image=o.images.back();
  NativeVoid(o,[&]{Manager::LoadImage(&image,path.c_str(),__ptr_g_VertexAttributeDescriptor_Position,false);});
  return image.image&&image.counter&&image.image->GetWidth()>0&&image.image->GetHeight()>0;
}
bool NeedSprite(ClientPreparedNativeNeed &o,const std::string &path) {
  if(path.empty())return true;if(path.size()>4096||o.sprites.size()>=256)return false;
  o.sprites.push_back(nullptr);auto *sprite=ClientPreparedResourceCreate(o.allocator);if(!sprite)return false;o.sprites.back()=sprite;
  state::ResourceLocalDescription d;d.kind=0;d.text[0]=path;
  if(ClientPreparedResourceLoad(sprite,d).status!=Status::Ready)return false;NativeVoid(*sprite,[&]{sprite->sprite->LoadGraphics(false);});
  for(unsigned i=0;i<sprite->sprite->_layerCount;++i){const auto &layer=sprite->sprite->_layerState[i];
    // 演出文件允许空图层占位；后续具名选择使用本请求已准备的肖像，不把空层补成图片。
    if(layer._spriteSheetPath.empty())continue;
    if(!layer._spriteSheetLoaded||!layer._spriteSheet.image||!layer._spriteSheet.counter||layer._spriteSheet.image->GetWidth()<=0||layer._spriteSheet.image->GetHeight()<=0)return false;}
  return true;
}
template<class T> const T &NativeAt(const void *p,unsigned offset){return *reinterpret_cast<const T *>(static_cast<const unsigned char *>(p)+offset);}
bool MusicNeed(ClientPreparedNativeNeed &o,const ClientNativeNeedInput &input) {
  const auto &entries=g_Manager->_musicmanager._entries;
  if(input.id<=0||input.id>=std::int64_t(entries.size()))return false;const auto &entry=entries[std::size_t(input.id)];
  if(NativeAt<std::int32_t>(&entry,0)!=input.id)return false;
  const auto &intro=NativeAt<std::string>(&entry,0x1c),&main=NativeAt<std::string>(&entry,0x34);
  if(main.empty()||main.size()>4096||intro.size()>4096)return false;
  if(!NativeCall(o,[&]{return LoadAudio(o.audio[0],main,true);})||(!intro.empty()&&!NativeCall(o,[&]{return LoadAudio(o.audio[1],intro,true); })))return false;
  const auto count=NativeAt<std::uint32_t>(&entry,0x5c);const auto *layers=NativeAt<const unsigned char *>(&entry,0x58);
  if(count>6||(count&&!layers))return false;
  for(unsigned i=0;i<count;++i){const auto &a=NativeAt<std::string>(layers+i*0x34,0),&b=NativeAt<std::string>(layers+i*0x34,0x18);
    if(a.size()>4096||b.empty()||b.size()>4096||!NativeCall(o,[&]{return LoadAudio(o.audio[2+i*2],b,true);})||
        (!a.empty()&&!NativeCall(o,[&]{return LoadAudio(o.audio[3+i*2],a,true); })))return false;}
  return true;
}
bool BossNeed(ClientPreparedNativeNeed &o,const ClientNativeNeedInput &input) {
  if(input.id<0||input.id>UINT32_MAX||input.second_id<0||input.second_id>UINT32_MAX||input.player_count>256)return false;
  const auto screen=input.id==99?"gfx/ui/boss/versusscreen_dogma.anm2":input.id==88?"gfx/ui/boss/versusscreen_mother.anm2":"gfx/ui/boss/versusscreen.anm2";
  if(!NeedSprite(o,screen))return false;
  const auto &bosses=g_Manager->_entityConfig._bosses;
  for(auto id:{input.id,input.second_id})if(id){if(id>=std::int64_t(bosses.size()))return false;const auto &boss=bosses[std::size_t(id)];
    if(boss.bossID!=id||boss.alts.size()>128||!NeedImage(o,boss.namePath)||!NeedImage(o,boss.portraitPath))return false;
    // 只闭合这两个实际请求 ID 的有限变体；不扫描整个 Boss 目录，不在 Client 重选变体。
    for(const auto &alt:boss.alts)if(!NeedImage(o,alt.portraitPath))return false;}
  for(std::size_t i=0;i<input.player_count;++i){auto *p=g_Manager->_entityConfig.GetPlayer(input.players[i]);
    if(!p||p->_id!=input.players[i]||!NeedImage(o,p->_nameImagePath)||!NeedImage(o,p->_portraitPath)||!NeedSprite(o,p->_extraPortraitPath))return false;}
  return true;
}
}
ClientPreparedNativeNeed *ClientPreparedNativeNeedCreate(ClientPreparationAllocator a) noexcept {
  return Window()&&ClientNativeCpuQueuesEmpty()?Allocate<ClientPreparedNativeNeed>(a):nullptr;
}
ClientPreparationResult ClientPreparedNativeNeedLoad(ClientPreparedNativeNeed *o,const ClientNativeNeedInput &in) noexcept {
  if(!o||!Window()||!ClientBinaryPreparationScopeActive(false))return Bad("binary_native_need_context");if(!ClientNativeCpuQueuesEmpty())return {Status::Waiting,"binary_native_need_cpu_borrow"};
  if(in.kind<2&&!*reinterpret_cast<const unsigned char *>(Base+0x7ce5a4))return {Status::Waiting,"binary_native_need_audio_backend"};
  if(o->entered)return o->ready&&o->context.Current()?Ready():Bad("binary_native_need_prefix_retained");
  o->entered=true;o->context.Capture();
  try {const bool ready=in.kind<2?MusicNeed(*o,in):in.kind==2&&BossNeed(*o,in);
    if(!ready)return Missing("binary_native_need_local_asset");o->ready=true;return Ready();
  }catch(...){return Bad(o->uncertain?"binary_native_need_exception_retained":"binary_native_need_allocation");}
}
bool ClientPreparedNativeNeedCurrent(const ClientPreparedNativeNeed *o) noexcept {
  return o&&o->ready&&!o->uncertain&&!o->release_entered&&o->context.Current()&&
    (!o->audio[0].source||*reinterpret_cast<const unsigned char *>(Base+0x7ce5a4));
}
bool ClientPreparedNativeNeedRelease(ClientPreparedNativeNeed *&o) noexcept {
  if(!o)return true;if(!Window()||!ClientBinaryPreparationScopeActive(true)||!ClientNativeCpuQueuesEmpty()||o->uncertain||o->release_entered||(o->entered&&!o->context.Current()))return false;
  // 子 sprite 各自保留释放前缀；排空可分次重入，已释放的句柄保持 Null。
  for(std::size_t i=o->sprites.size();i>0;--i)if(!ClientPreparedResourceRelease(o->sprites[i-1]))return false;
  o->release_entered=true;
  try {for(std::size_t i=o->audio.size();i>0;--i)if(!ReleaseAudio(o->audio[i-1]))return false;Delete(o);o=nullptr;return true;}
  catch(...){o->uncertain=true;return false;}
}

// 预构造池 use 只在本 owner 链保留；不伪造 _valid／_exists 占用位。Factory 返回重复地址即停止。
struct ClientStagedObject {
  explicit ClientStagedObject(ClientPreparationAllocator a):allocator(a){}
  ClientPreparationAllocator allocator;NativeContext context;ClientNativeRoots roots;ClientStagedObjectInput input;
  ClientStagedObject *next=nullptr;
  void *object=nullptr,*vptr=nullptr;
  bool entered=false,constructed=false,ready=false,uncertain=false,release_entered=false,pool=false,notified=false;
};
namespace {
ClientStagedObject *staged_objects=nullptr;
constexpr unsigned kLargestEntityPool=*std::max_element(kClientEntityPools.begin(),kClientEntityPools.end());
// 每次同步扫描先投影当前保留槽，避免“池中每个槽再遍历全部暂存owner”的平方成本。
// 位图只活到本次调用返回，不跨原生调用或帧缓存；地址仍须精确对齐真实池槽。
std::array<bool,kLargestEntityPool> ReservedPool(unsigned category,const unsigned char *base) noexcept {
  std::array<bool,kLargestEntityPool> reserved{};
  const auto start=reinterpret_cast<std::uintptr_t>(base);
  const auto stride=kClientEntityPoolStrides[category],count=kClientEntityPools[category];
  for(auto *o=staged_objects;o;o=o->next){const auto address=reinterpret_cast<std::uintptr_t>(o->object);
    if(address<start)continue;const auto offset=address-start;
    if(offset<std::uintptr_t(count)*stride&&offset%stride==0)reserved[offset/stride]=true;
  }
  return reserved;
}
// Factory选择器只看valid，无法识别本方尚未Init或已离开但仍被借用的槽。
// 有保留空槽时，按同一J273池和官方ESSM入口取得已经构造的可用槽。
Entity *AvailableEntity(unsigned category,bool &factory_safe) noexcept {
  auto *pool=reinterpret_cast<unsigned char *>(g_Game->_entityFactory)+kClientEntityPoolOffsets[category];
  const auto reserved=ReservedPool(category,pool);
  Entity *result=nullptr;factory_safe=category!=12;
  for(unsigned i=0;i<kClientEntityPools[category];++i){
    auto *entity=reinterpret_cast<Entity *>(pool+i*kClientEntityPoolStrides[category]);
    if(entity->_valid)continue;
    if(reserved[i]||entity->_exists){factory_safe=false;continue;}
    if(!result)result=entity;
  }
  return result;
}
}
ClientPreparationResult ClientStagedPoolNotificationsCurrent(const ClientNativeRoots &roots,
    const ClientStagedObjectInput *inputs,std::size_t count) noexcept {
  if((count&&!inputs)||!ClientPreparationWindow(roots,ClientLuaStateIdentity())||!g_Game->_entityFactory)
    return Bad("binary_stage_pool_context");
  std::array<bool,kClientEntityPools.size()> checked{};
  for(std::size_t i=0;i<count;++i){
    const auto &input=inputs[i];
    if(input.kind!=ClientStagedKind::Entity||input.borrowed)continue;
    const auto category=input.category;
    if(!category||category>=checked.size()||input.native_size!=kClientEntityPoolStrides[category])
      return Bad("binary_stage_entity_factory_abi");
    if(checked[category])continue;
    // 检查官方闭包身份，无需申请、读写或腾空这个槽。
    auto *slot=reinterpret_cast<std::byte *>(g_Game->_entityFactory)+kClientEntityPoolOffsets[category];
    const char *why="binary_stage_essm_preflight";
    if(!ClientObjectStagedPoolNotification(slot,false,false,why)||!ClientObjectStagedPoolNotification(slot,false,true,why))
      return Bad(why);
    checked[category]=true;
  }
  return Ready();
}
ClientStagedObject *ClientStagedObjectCreate(ClientPreparationAllocator a) noexcept {
  if(!Window()||!ClientNativeCpuQueuesEmpty())return nullptr;
  auto *o=Allocate<ClientStagedObject>(a);if(o){o->next=staged_objects;staged_objects=o;}return o;
}
ClientPreparationResult ClientStagedObjectConstruct(ClientStagedObject *o,const ClientNativeRoots &roots,const ClientStagedObjectInput &in) noexcept {
  if(!o||!ClientBinaryPreparationScopeActive(false)||!ClientPreparationWindow(roots,ClientLuaStateIdentity()))return Bad("binary_stage_context");
  if(!ClientNativeCpuQueuesEmpty())return {Status::Waiting,"binary_stage_cpu_borrow"};
  if(o->entered)return o->ready&&ClientStagedObjectCurrent(o,roots)?Ready():Bad("binary_stage_prefix_retained");
  o->context.Capture();o->roots=roots;o->input=in;o->entered=true;
  try {switch(in.kind) {
    case ClientStagedKind::Logical:break;
    case ClientStagedKind::Game:case ClientStagedKind::Level:o->object=roots.game;break;
    case ClientStagedKind::Room:o->object=roots.room;break;
    case ClientStagedKind::Sfx:
      // 布局437的基址是SoundEffects的音量标量；容器头从Manager+2A160开始。
      o->object=&g_Manager->_sfxManager._volume;break;
    case ClientStagedKind::Descriptor:{
      if(in.native_size!=sizeof(RoomDescriptor)||in.index>=527||in.index==525)return Bad("binary_stage_descriptor_slot");
      // Level 与地图直接索引 Game 的内嵌数组。这里借出真实槽，安装前不改槽内容。
      o->object=in.index<507?&g_Game->_gridRooms[in.index]:&g_Game->_negativeGridRooms[in.index-507];break;}
    case ClientStagedKind::Grid:{if(!in.type||in.type>27||in.index>=448)return Bad("binary_stage_grid_input");
      NativeVoid(*o,[&]{o->object=Isaac::CreateGridEntity(in.type,in.index);});o->constructed=bool(o->object);if(!o->object)return Bad("binary_stage_grid_factory_null");break;}
    case ClientStagedKind::Player:{if(in.native_size!=sizeof(Entity_Player)||in.type!=1||in.variant)return Bad("binary_stage_player_abi");
      ClientPlayerIsolationScope scope(ClientPlayerOperation::Construct);if(!scope.Ready())return Bad("binary_stage_player_isolation");
      o->object=reinterpret_cast<void *(__cdecl *)(std::size_t)>(Base+0x686de5)(sizeof(Entity_Player));if(!o->object)return Bad("binary_stage_player_allocation");
      std::memset(o->object,0,sizeof(Entity_Player));NativeVoid(*o,[&]{static_cast<Entity_Player *>(o->object)->constructor();});o->constructed=true;break;}
    case ClientStagedKind::Entity:{if(!g_Game->_entityFactory||!ClientEntityInitIsolationInstalled())return Bad("binary_stage_entity_factory");
      const char *why="binary_stage_essm";
      o->pool=true;
      if(!in.category||in.category>=kClientEntityPools.size())return Bad("binary_stage_entity_pool_category");
      bool factory_safe=false;auto *slot=AvailableEntity(in.category,factory_safe);
      if(!slot)return Missing("binary_stage_entity_pool_full");
      if(!factory_safe){
        if(in.category==12&&*reinterpret_cast<std::uintptr_t *>(slot)!=Base+0x7181b8)return Bad("binary_stage_dummy_type");
        o->object=slot;
        if(!ClientObjectStagedPoolNotification(o->object,true,true,why)){o->uncertain=true;return Bad(why);}
      }else NativeVoid(*o,[&]{o->object=g_Game->_entityFactory->Create(in.type,true);});
      if(!o->object)return Missing("binary_stage_entity_pool_full");o->notified=true;
      auto *e=static_cast<Entity *>(o->object);if(e->_valid||e->_exists){o->uncertain=true;return Bad("binary_stage_live_pool_use_retained");}
      for(auto *p=staged_objects;p;p=p->next)if(p!=o&&p->object==o->object){p->uncertain=o->uncertain=true;return Bad("binary_stage_duplicate_pool_use_retained");}
      o->constructed=true;break;}
  }
    if(o->object&&(in.kind==ClientStagedKind::Grid||in.kind==ClientStagedKind::Player||in.kind==ClientStagedKind::Entity))o->vptr=*static_cast<void **>(o->object);
    o->ready=true;return Ready();
  }catch(...){return Bad(o->uncertain?"binary_stage_constructor_exception_retained":"binary_stage_allocation");}
}
bool ClientStagedObjectCurrent(const ClientStagedObject *o,const ClientNativeRoots &roots) noexcept {
  if(!o||!o->ready||o->uncertain||o->release_entered||!o->context.Current()||!ClientPreparationWindow(roots,o->context.vm)||
      roots.game!=o->roots.game||roots.room!=o->roots.room)return false;
  if(o->vptr&&(!o->object||*static_cast<void **>(o->object)!=o->vptr))return false;
  if(o->input.kind==ClientStagedKind::Entity||o->input.kind==ClientStagedKind::Player){auto *e=static_cast<Entity *>(o->object);return e&&!e->_valid&&!e->_exists;}
  return true;
}
void *ClientStagedObjectBorrow(ClientStagedObject *o) noexcept {return o&&ClientStagedObjectCurrent(o,o->roots)?o->object:nullptr;}
void *ClientStagedObjectOwnedAddress(const ClientStagedObject *o) noexcept {
  __try {
    if(!o||!o->ready||o->uncertain||o->release_entered||!o->context.Current()||!g_Game||g_Game!=o->roots.game||g_Game->_room!=o->roots.room)return nullptr;
    return o->object&&(!o->vptr||*static_cast<void **>(o->object)==o->vptr)?o->object:nullptr;
  }__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
}
void *ClientStagedObjectLiveAddress(const ClientStagedObject *o) noexcept {
  __try {
    auto *address=ClientStagedObjectOwnedAddress(o);
    if(!address)return nullptr;
    if(o->input.kind==ClientStagedKind::Grid){
      const auto *grid=static_cast<const GridEntity *>(address);
      return grid->_gridIndex==o->input.index&&unsigned(grid->_desc._type)==o->input.type&&
        unsigned(grid->_desc._variant)==o->input.variant?address:nullptr;
    }
    if(o->input.kind!=ClientStagedKind::Entity&&o->input.kind!=ClientStagedKind::Player)return address;
    const auto *entity=static_cast<const Entity *>(address);
    // Player 使用独立分配，合法的未入册身体可无 valid；池对象必须同时存活且未退役。
    if((o->input.kind==ClientStagedKind::Entity&&(!entity->_valid||!entity->_exists))||
        entity->_type!=o->input.type||entity->_variant!=o->input.variant||
        entity->_subtype!=o->input.subtype||entity->_initSeed!=o->input.seed)return nullptr;
    return address;
  } __except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
}
bool ClientStagedObjectAdoptShape(ClientStagedObject *o,void *address,unsigned type,unsigned variant,
    unsigned subtype,unsigned seed) noexcept {
  __try {
    if(!o||!address||ClientStagedObjectOwnedAddress(o)!=address||o->input.kind!=ClientStagedKind::Entity)return false;
    const auto &entity=*static_cast<const Entity *>(address);
    if(!entity._valid||!entity._exists||entity._type!=type||entity._variant!=variant||
        entity._subtype!=subtype||entity._initSeed!=seed)return false;
    o->input.type=type;o->input.variant=variant;o->input.subtype=subtype;o->input.seed=seed;
    return true;
  }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ClientStagedObjectRelease(ClientStagedObject *&o) noexcept {
  if(!o)return true;if(!Window()||!ClientBinaryPreparationScopeActive(true)||!ClientNativeCpuQueuesEmpty()||o->uncertain||o->release_entered||
      (o->entered&&(!o->context.Current()||g_Game!=o->roots.game||!g_Game||g_Game->_room!=o->roots.room)))return false;
  if(o->object&&!o->constructed&&o->input.kind==ClientStagedKind::Player)return false;
  // Init／发布后的生命周期由安装owner先撤销，不能以本准备层的通知／析构替代Remove和拆边。
  if(o->object&&(o->input.kind==ClientStagedKind::Player||o->input.kind==ClientStagedKind::Entity)){
    const auto *entity=static_cast<const Entity *>(o->object);if(entity->_valid||entity->_exists)return false;}
  const char *why="binary_stage_release";
  if(o->pool&&o->object&&!ClientObjectStagedPoolNotification(o->object,false,false,why))return false;
  o->release_entered=true;
  try {if(o->object)switch(o->input.kind){
    case ClientStagedKind::Player:{ClientPlayerIsolationScope scope(ClientPlayerOperation::Retire);if(!scope.Ready())return false;
      // 只有 ctor、尚未 Init 的私有 Player：经官方完整析构链配平 New／Delete，再配对原生分配器。
      static_cast<Entity_Player *>(o->object)->destructor();reinterpret_cast<void (__cdecl *)(void *)>(Base+0x686bc9)(o->object);break;}
    case ClientStagedKind::Grid:{auto **table=*static_cast<void ***>(o->object);reinterpret_cast<void (__thiscall *)(void *,unsigned)>(table[0])(o->object,1);break;}
    case ClientStagedKind::Entity:if(o->notified&&!ClientObjectStagedPoolNotification(o->object,true,false,why))return false;break;
    default:break;
  }
    auto **where=&staged_objects;while(*where&&*where!=o)where=&(*where)->next;if(*where!=o)return false;*where=o->next;Delete(o);o=nullptr;return true;
  }catch(...){o->uncertain=true;return false;}
}
} // namespace cs::game
