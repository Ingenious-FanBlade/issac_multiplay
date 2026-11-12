#include "world/binary/entity_source_internal.h"
#include <cmath>

namespace cs::world::binary {
// 伴随读取不借type数值猜Player；同窗初始化证言与冷准备的真实RTTI必须同时成立。
bool EntitySource::Impl::QualifiedPlayer(std::uint32_t player,CaptureDiagnostic &d) noexcept {
  std::uint32_t vtable=0;if(!services.player_current || !services.player_current(services.owner,player) || !Tracked(player,vtable,d))
    return FailCapture(d,CaptureFailure::MissingQualification,{facts.subject});
  const auto found=std::lower_bound(vtables.begin(),vtables.end(),vtable,[](const auto &entry,std::uint32_t p){return entry.vtable<p;});
  return (found!=vtables.end() && found->vtable==vtable && found->layout==197) || FailCapture(d,CaptureFailure::UnknownVtable,{facts.subject});
}
// 原生有序计数表只走查找路径；nil不读value，循环／越界／过深不冒充未持有。
bool EntitySource::Impl::NativeCounter(std::uint32_t address,std::int32_t key,std::int32_t &count,CaptureDiagnostic &d) noexcept {
  std::array<std::uint32_t,2> header{};std::uint32_t node=0;std::uint8_t nil=0;count=0;
  if(!Tracked(address,header,d) || !header[0] || header[1]>16384 || !Tracked(std::uint64_t(header[0])+13,nil,d) || nil!=1 ||
     !Tracked(std::uint64_t(header[0])+4,node,d))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  std::array<std::uint32_t,128> visited{};unsigned depth=0;
  while(node!=header[0]){if(!node || depth==visited.size() || std::find(visited.begin(),visited.begin()+depth,node)!=visited.begin()+depth)
      return FailCapture(d,CaptureFailure::TraversalBudget,{facts.subject});visited[depth++]=node;
    std::int32_t candidate=0;if(!Tracked(std::uint64_t(node)+13,nil,d) || nil || !Tracked(std::uint64_t(node)+16,candidate,d))
      return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
    if(candidate==key)return Tracked(std::uint64_t(node)+20,count,d);
    if(!Tracked(std::uint64_t(node)+(key<candidate?0u:8u),node,d))return false;
  }
  return true;
}

// 7699D0／9950B0中425、460及其191派生路径的只读投影。原版固定角色／变换／局部随机
// 候选表均不含这三个ID，因此不重跑那些查询或局部随机；官方现有blocked／innate仍优先。
// 第三方自定义角色／rework不在当前0—15步原版内容范围内，不能据此声称支持自定义查询语义。
bool EntitySource::Impl::LightingCollectible(std::uint32_t player,std::int32_t id,bool &present,CaptureDiagnostic &d,unsigned depth,bool spoof) noexcept {
  present=false;if(depth>=16 || (id!=425 && id!=460 && id!=191))
    return FailCapture(d,CaptureFailure::MissingQualification,{facts.subject});
  if(!QualifiedPlayer(player,d))return false;
  const auto mark=scratch_used;struct Restore {std::uint32_t &cursor,value;~Restore(){cursor=value;}} restore{scratch_used,mark};
  if(spoof){std::uint16_t upper=0,lower=0;std::uint32_t plus=0;std::uint32_t holder=0;
  if(!Tracked(std::uint64_t(player)+0x18e,upper,d) || !Tracked(std::uint64_t(player)+0x192,lower,d))return false;
  holder=(std::uint32_t(upper)<<16)|lower;
  if(!holder || !Tracked(holder,plus,d) || !plus || plus>UINT32_MAX-0xa8)return FailCapture(d,CaptureFailure::MissingQualification,{facts.subject});
  auto storage=Allocate(16384u*8,8,d);if(d.reason!=CaptureFailure::None)return false;std::uint32_t used=0,visited=0,count=0;
  if(!HashItems(plus+0x14,false,storage,used,visited,d))return false;
  for(std::uint32_t i=0;i<used;i+=4){std::int32_t item=0;std::memcpy(&item,storage.data()+i,4);if(item==id)return true;}
  if(!Aggregate(plus+0x34,storage,count,d))return false;
  for(std::uint32_t i=0;i<count;++i){std::array<std::int32_t,2> item{};std::memcpy(item.data(),storage.data()+i*8,8);
    if(item[0]==id && item[1]>0){present=true;return true;}}}
  std::uint32_t manager=0,config=0;std::array<std::uint32_t,3> configs{};
  if(!Tracked(std::uint64_t(memory.image_base)+0x7e4a1c,manager,d) || !manager ||
     !Tracked(std::uint64_t(manager)+0x2a234,configs,d) || configs[0]>configs[1] || configs[1]>configs[2] ||
     (configs[1]-configs[0])%4 || std::uint32_t(id)>=(configs[1]-configs[0])/4 ||
     !Tracked(std::uint64_t(configs[0])+std::uint32_t(id)*4u,config,d) || !config)
    return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
  std::uint64_t tags=0;std::uint32_t variant=0;if(!Tracked(std::uint64_t(config)+184,tags,d) || !Tracked(std::uint64_t(player)+44,variant,d))return false;
  if(variant==1){std::uint32_t spawner=0,type=0;if(!Tracked(std::uint64_t(player)+956,spawner,d))return false;
    if(spawner && !Tracked(std::uint64_t(spawner)+40,type,d))return false;
    if(!spawner || type!=1){std::uint32_t first=0;if(!Tracked(std::uint64_t(facts.scene->game)+113320,first,d) || !first || !Tracked(first,spawner,d))return false;}
    // 这是原生super内部的while转向，不是又一次公开HasCollectible调用；不重复套用spawner的spoof。
    return LightingCollectible(spawner,id,present,d,depth+1,false);}
  std::uint8_t state=0;std::int32_t frame=0;
  if(!Tracked(std::uint64_t(player)+8317,state,d) || !Tracked(std::uint64_t(facts.scene->game)+154508,frame,d))return false;
  if(state && frame<2 && !(tags&0x8000))return true;
  if(!Tracked(std::uint64_t(player)+368,state,d))return false;
  if(state && (tags&0x40000000)){std::uint32_t twin=0;if(!Tracked(std::uint64_t(player)+7760,twin,d))return false;
    if(twin && (!LightingCollectible(twin,id,present,d,depth+1) || present))return d.reason==CaptureFailure::None;}
  if(!Tracked(std::uint64_t(player)+8208,state,d))return false;
  if(state && !(tags&0x8000))return true;
  std::int32_t wisp=0;std::uint32_t selected=0;
  if(!NativeCounter(player+8028,id,wisp,d) || !Tracked(std::uint64_t(player)+8084,selected,d))return false;
  if(wisp>0 || selected==std::uint32_t(id)){present=true;return true;}
  std::array<std::uint32_t,3> extras{};
  if(!Tracked(std::uint64_t(player)+8112,extras,d) || extras[0]>extras[1] || extras[1]>extras[2])return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  if(std::uint32_t(id)<extras[1]-extras[0]){std::uint8_t amount=0;if(!Tracked(std::uint64_t(extras[0])+std::uint32_t(id),amount,d))return false;
    if(amount){present=true;return true;}}
  for(unsigned i=0;i<4;++i){if(!Tracked(std::uint64_t(player)+5492+i*32u,selected,d))return false;if(selected==std::uint32_t(id)){present=true;return true;}}
  if(id!=191){if(!Tracked(std::uint64_t(player)+5008,selected,d))return false;
    if(selected==std::uint32_t(id) && (!LightingCollectible(player,191,present,d,depth+1) || present))return d.reason==CaptureFailure::None;}
  if(!Tracked(std::uint64_t(player)+5000,selected,d))return false;
  if(selected==std::uint32_t(id)){std::uint8_t disabled=0;std::array<std::uint32_t,3> effects{};
    if(!Tracked(std::uint64_t(player)+5393,disabled,d) || disabled>1)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
    if(!disabled){if(!Tracked(std::uint64_t(player)+5380,effects,d) || effects[0]>effects[1] || effects[1]>effects[2] || (effects[1]-effects[0])%16 ||
        (effects[1]-effects[0])/16>16384)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
      for(auto address=std::uint64_t(effects[0]);address<effects[1];address+=16){std::uint32_t item=0;std::array<std::uint32_t,2> choice{};
        if(!Tracked(address,item,d) || !item || !Tracked(item,choice,d))return false;
        if((choice[0]==1 || choice[0]==3 || choice[0]==4) && choice[1]==488){present=true;return true;}}}}
  std::array<std::uint32_t,3> items{};std::int32_t amount=0;
  if(!Tracked(std::uint64_t(player)+5816,items,d) || items[0]>items[1] || items[1]>items[2] || (items[1]-items[0])%4 ||
     std::uint32_t(id)>=(items[1]-items[0])/4 || !Tracked(std::uint64_t(items[0])+std::uint32_t(id)*4u,amount,d))
    return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});present=amount>0;return true;
}

// 803E20／8134C0的世界输入投影，在同一ordinary窗口计算一次；Options不决定世界资格。
bool EntitySource::Impl::Lighting(CaptureDiagnostic &d) noexcept {
  if(lighting_ready)return true;if(!facts.scene)return FailCapture(d,CaptureFailure::MissingQualification,{facts.subject});
  const auto &scene=*facts.scene;const auto game=std::uint64_t(scene.game),room=std::uint64_t(scene.room);NativeLighting result;
  std::int32_t stage=0,stage_type=0,dimension=0,difficulty=0;std::uint32_t backdrop=0,room_type=0,flags=0,game_flags=0,config=0,type=0,subtype=0;
  float darkness=0,target=0,base=0;
  if(!Tracked(game,stage,d) || !Tracked(game+4,stage_type,d) || !Tracked(game+0x1830c,dimension,d) || !Tracked(game+0x25f40,difficulty,d) ||
     !Tracked(game+38577u*4,game_flags,d) || !Tracked(game+38564u*4,darkness,d) || !Tracked(game+38565u*4,target,d) ||
     !Tracked(game+105063u*4,result.lightning,d) || !Tracked(room+7448,backdrop,d) || !Tracked(room+8,room_type,d) ||
     !Tracked(std::uint64_t(scene.current_descriptor)+68,flags,d) || !Tracked(std::uint64_t(scene.current_descriptor)+16,config,d) || !config ||
     !Tracked(std::uint64_t(config)+8,type,d) || !Tracked(std::uint64_t(config)+16,subtype,d) || !Tracked(room+7000,base,d))return false;
  for(unsigned i=0;i<5;++i)if(!Tracked(room+7004+i*4u,result.shadow[i],d))return false;
  if(!std::isfinite(darkness) || !std::isfinite(target) || !std::isfinite(base) || !std::isfinite(result.lightning))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  const bool mines=difficulty!=2 && difficulty!=3 && !(stage>=1 && stage<=6 && (game_flags&0x10000)) &&
    (stage_type==4 || stage_type==5) && (stage==4 || (stage==3 && (scene.effective_curses&2)));
  const bool womb=difficulty!=2 && difficulty!=3 && !(stage>=1 && stage<=6 && (game_flags&0x10000)) &&
    std::int64_t(stage)+((scene.effective_curses&2)?1:0)==6 && (stage_type==4 || stage_type==5) && (game_flags&(1u<<(47-32)));
  const bool pitch=(flags&0x200)!=0;
  if(pitch)result.shadow={0,0,0,std::bit_cast<std::uint32_t>(1.0f),0};
  if(std::bit_cast<float>(result.shadow[3])!=0)result.mask|=1;
  if(backdrop!=57 && !pitch){float factor=(scene.effective_curses&1)?1.0f:float(target*darkness);
    if(room_type==8 || room_type==29 || (mines && dimension==1) || (room_type==5 && subtype==89 && womb))factor=std::fmin(float(factor+0.6f),1.0f);
    if(stage==13 && stage_type==1 && !(flags&4))factor=1.0f;
    factor=std::fmax(float(factor-result.lightning),0.0f);result.tint[4]=0;
    for(unsigned i=0;i<4;++i){std::uint32_t raw=0;if(!Tracked(room+7024+i*4u,raw,d))return false;
      const auto to=std::bit_cast<float>(raw),from=std::bit_cast<float>(result.shadow[i]);
      if(!std::isfinite(from) || !std::isfinite(to))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
      result.tint[i]=i<3?std::bit_cast<std::uint32_t>(float(float(from-to)*factor)+to):raw;}result.mask|=2;}
  result.alpha=backdrop==57?float(target*darkness):pitch?1.0f:(scene.effective_curses&1)?1.0f:base;
  if(backdrop!=57 && !pitch){if(!(scene.effective_curses&1) && stage==1 && stage_type!=4 && stage_type!=5 && stage_type!=2)result.alpha=float(result.alpha*0.6f);
    std::array<std::uint32_t,3> roster{};if(!Tracked(game+113320,roster,d) || roster[0]>roster[1] || roster[1]>roster[2] ||
      (roster[1]-roster[0])%4 || (roster[1]-roster[0])/4>256)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
    std::array<bool,2> owns{};
    for(unsigned which=0;which<2;++which){const auto id=which?460:425;std::uint32_t manager=0,item=0;std::array<std::uint32_t,3> items{};std::uint64_t tags=0;
      if(!Tracked(std::uint64_t(memory.image_base)+0x7e4a1c,manager,d) || !manager || !Tracked(std::uint64_t(manager)+0x2a234,items,d) ||
         items[0]>items[1] || items[1]>items[2] || (items[1]-items[0])%4 || std::uint32_t(id)>=(items[1]-items[0])/4 ||
         !Tracked(std::uint64_t(items[0])+std::uint32_t(id)*4u,item,d) || !item || !Tracked(std::uint64_t(item)+184,tags,d))return FailCapture(d,CaptureFailure::ResourcePending,{facts.subject});
      for(auto address=std::uint64_t(roster[0]);address<roster[1]&&!owns[which];address+=4){std::uint32_t player=0,variant=0;
        if(!Tracked(address,player,d) || !player || !QualifiedPlayer(player,d) || !Tracked(std::uint64_t(player)+44,variant,d))return FailCapture(d,CaptureFailure::MissingQualification,{facts.subject});
        if(!variant){if(!LightingCollectible(player,id,owns[which],d))return false;
          if(!owns[which] && (tags&0x80000000)){std::uint32_t twin=0;if(!Tracked(std::uint64_t(player)+7760,twin,d))return false;
            if(twin && !LightingCollectible(twin,id,owns[which],d))return false;}}}}
    if(owns[0])result.alpha=float(result.alpha*0.5f);if(owns[1])result.alpha=std::fmin(float(result.alpha+0.1f),1.0f);
    result.alpha=float(float(float(target-result.alpha)*darkness)+result.alpha);
    if(room_type==8 || room_type==29 || (mines && dimension==1) || dimension==2)result.alpha=std::fmin(1.0f,float(result.alpha+0.33f));
    if(stage==9 && type==5){std::uint32_t list=0,count=0;if(!Tracked(room+4700,list,d) || !Tracked(room+4708,count,d) || count>16384 || (count&&!list))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
      for(std::uint32_t i=0;i<count;++i){std::uint32_t entity=0;std::array<std::uint32_t,2> kind{};
        if(!Tracked(std::uint64_t(list)+i*4u,entity,d) || !entity || !services.entity_current || !services.entity_current(services.owner,entity) ||
           !Tracked(std::uint64_t(entity)+40,kind,d))return FailCapture(d,CaptureFailure::MissingQualification,{facts.subject});
        if(kind[0]==407 && !kind[1]){std::array<float,2> hp{};if(!Tracked(std::uint64_t(entity)+896,hp,d) || !std::isfinite(hp[0]) || !std::isfinite(hp[1]) || hp[1]==0)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
          result.alpha=std::fmax(float(1.0f-float(hp[0]/hp[1])),result.alpha);break;}}}
    if(stage==9 && type!=5){std::int32_t index=0;if(!Tracked(game+24769u*4,index,d))return false;
      if(index==-9){std::array<float,4> bounds{};std::int32_t shape=0;std::uint32_t width=0;
        if(!Tracked(room+20,bounds,d) || !Tracked(std::uint64_t(config)+72,shape,d) || !Tracked(room+12,width,d) || !width || roster[0]==roster[1])return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
        std::array<float,2> center={float(float(bounds[2]-bounds[0])*0.5f)+bounds[0],float(float(bounds[3]-bounds[1])*0.5f)+bounds[1]};
        if(shape>=9 && shape<=12){constexpr unsigned indices[]={328,315,132,119};const auto grid=indices[shape-9];center={float(float(grid%width)*40.0f)+40.0f,float(float(grid/width)*40.0f)+120.0f};}
        float distance=INFINITY;for(auto address=std::uint64_t(roster[0]);address<roster[1];address+=4){std::uint32_t player=0;std::array<float,2> position{};
          if(!Tracked(address,player,d) || !QualifiedPlayer(player,d) || !Tracked(std::uint64_t(player)+828,position,d))return false;
          const float x=float(center[0]-position[0]),y=float(center[1]-position[1]),squared=float(float(y*y)+float(x*x));
          if(!std::isfinite(squared))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});distance=std::fmin(distance,squared);}
        result.alpha=float(1.0f-float(std::fmin(300.0f,float(std::sqrt(distance)))/300.0f));}}}
  result.mask|=4;
  const bool mirror=dimension==1 && (stage_type==4 || stage_type==5) && (stage==2 || (stage==1 && (scene.effective_curses&2)));
  // 特殊主题与镜面是独立pass，可同时成立，不能用互斥kind丢掉其中一个。
  result.special=std::uint8_t((backdrop==11?1u:0u)|(backdrop==57?2u:0u)|(mirror?4u:0u));if(result.special)result.mask|=8;
  if(!std::isfinite(result.alpha))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  for(unsigned i=0;i<4;++i)if(!std::isfinite(std::bit_cast<float>(result.shadow[i])))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  lighting=result;lighting_ready=true;return true;
}
}
