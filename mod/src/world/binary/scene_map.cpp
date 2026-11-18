#include "world/binary/entity_source_internal.h"
#include <cmath>

namespace cs::world::binary {
// 每描述只聚合一次，读取最小地图consumer视图；不读取隐藏保存集合或构造保存对象。
bool EntitySource::Impl::SavedMapIcons(std::uint32_t descriptor,std::uint16_t &mask,
    std::array<std::uint8_t,16> &frames,CaptureDiagnostic &d) noexcept {
  mask=0;frames={};std::uint32_t display=0;if(!Tracked(std::uint64_t(descriptor)+0x3c,display,d))return false;
  if(!display)return FailCapture(d,CaptureFailure::InvalidTransaction,{facts.subject});
  const auto mark=[&](unsigned bit,unsigned frame=0){mask|=std::uint16_t(1u<<bit);frames[bit]=std::uint8_t(std::max(unsigned(frames[bit]),frame));};
  // 两个容器各核宽头／步长及当前whole-world扫描配额；超额拒绝，不裁成可接受前缀。
  for(unsigned collection=0;collection<2;++collection){std::array<std::uint32_t,3> header{};
    const auto stride=collection?0x20u:0x78u,offset=collection?0x68u:0x74u;
    if(!Tracked(std::uint64_t(descriptor)+offset,header,d) || header[0]>header[1] || header[1]>header[2] || (!header[0]&&(header[1]||header[2])) ||
       (header[1]-header[0])%stride || (header[2]-header[0])%stride)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
    const auto count=(header[1]-header[0])/stride;
    if(count>16384 || count>scene_scan_remaining)return FailCapture(d,CaptureFailure::TraversalBudget,{facts.subject});scene_scan_remaining-=count;
    for(std::uint32_t i=0;i<count;++i){const auto element=std::uint64_t(header[0])+std::uint64_t(i)*stride;
      // 保存实体的 type/variant/subtype 是已初始化的连续标识前缀。保留保存 Grid 的独立类型读取。
      std::array<std::uint32_t,3> identity{};
      if(collection?!Tracked(element,identity[0],d):!Tracked(element,identity,d))return false;
      const auto type=identity[0];
      if(collection){if(type==18)mark(14);continue;}
      if(type!=5 && type!=6 && type!=1000)continue;
      const auto variant=identity[1];
      const auto subtype=std::bit_cast<std::int32_t>(identity[2]);
      if(type==1000){if(variant==161&&subtype==980)mark(15);continue;}
      if(type==6){switch(variant){
        case 14:break;case 2:mark(11,3);break;case 3:mark(11,4);break;case 4:case 6:mark(13,0);break;
        case 5:case 15:mark(13,2);break;case 7:mark(13,3);break;case 8:case 11:mark(11,1);break;
        case 9:mark(13,4);break;case 10:mark(11,6);break;case 12:mark(11,0);break;case 13:mark(13,5);break;
        case 16:mark(11,5);break;case 17:mark(11,7);break;case 18:mark(13,1);break;case 19:mark(11,8);break;default:mark(11,2);break;}continue;}
      std::int32_t excluded=0;if(!Tracked(element+0x14,excluded,d))return false;if(excluded)continue;
      std::array<float,2> target{};if(!Tracked(element+0x28,target,d) || !std::isfinite(target[0]) || !std::isfinite(target[1]))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
      if(target[0]==0 && target[1]==0)continue;
      if(variant==42){mark(2);continue;}if(variant==100){mark(6);continue;}if(variant==350){mark(10);continue;}
      const bool chest=variant==50 || variant==60 || variant==360 || variant==51 || variant==53 || variant==52 || variant==54 || variant==390 ||
        variant==55 || variant==56 || variant==57 || variant==58 || variant==69;
      if(chest && variant!=69){unsigned frame=4;switch(variant){case 51:frame=1;break;case 52:frame=0;break;case 53:frame=7;break;
        case 56:frame=6;break;case 57:frame=9;break;case 60:frame=8;break;case 360:frame=5;break;default:break;}mark(9,frame);continue;}
      if(variant!=10 && variant!=20 && variant!=30 && variant!=40 && variant!=70 && variant!=90 && variant!=300 && variant!=69)continue;
      if(variant==69){mark(9,subtype==2?3u:2u);continue;}
      if(variant==10){unsigned frame=1;switch(subtype){case 2:frame=0;break;case 3:frame=5;break;case 4:frame=9;break;case 6:frame=6;break;
        case 7:frame=8;break;case 8:frame=3;break;case 10:frame=4;break;case 11:frame=7;break;case 12:frame=2;break;default:break;}mark(0,frame);}
      else if(variant==20)mark(1,subtype==2||subtype==6?1u:subtype==3?2u:subtype==7?3u:0u);
      else if(variant==30)mark(3,subtype==2?1u:subtype==4?2u:0u);
      else if(variant==40)mark(4,subtype==4?1u:0u);
      else if(variant==70)mark(7,(std::uint32_t(subtype)&0x7ff)==14?1u:0u);
      else if(variant==90)mark(8,subtype==4?1u:0u);
      else if(variant==300){std::uint32_t manager=0,config=0;std::array<std::uint32_t,3> cards{};bool rune=false;
        if(!Value(std::uint64_t(memory.image_base)+0x7e4a1c,manager,d) || !manager || !Tracked(std::uint64_t(manager)+0x2a258,cards,d) ||
           cards[0]>cards[1] || cards[1]>cards[2] || (cards[1]-cards[0])%4)return FailCapture(d,CaptureFailure::MissingQualification,{facts.subject});
        if(subtype>=0 && std::uint32_t(subtype)<(cards[1]-cards[0])/4){if(!Tracked(std::uint64_t(cards[0])+std::uint32_t(subtype)*4u,config,d))return false;
          if(config){std::int32_t card_type=0;if(!Tracked(std::uint64_t(config)+88,card_type,d))return false;rune=card_type==2;}}
        mark(rune?12u:5u);}
    }
  }
  return true;
}
}
