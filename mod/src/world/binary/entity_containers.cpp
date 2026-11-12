#include "world/binary/entity_source_internal.h"
#include <limits>

namespace cs::world::binary {
namespace {
std::uint64_t Sequence(std::uint32_t stride,NativeSequence kind=NativeSequence::Contiguous) noexcept {return stride|(std::uint64_t(kind)<<32);}
}
// 读取真实MSVC树的有界中序节点，不读取组名、nil value或节点填隙。
bool EntitySource::Impl::NativeTree(std::uint32_t address,std::span<std::uint32_t> nodes,std::uint32_t &count,CaptureDiagnostic &d) noexcept {
  std::array<std::uint32_t,2> header{};if(!Tracked(address,header,d))return false;
  const auto head=header[0];count=header[1];std::uint8_t nil=0;std::array<std::uint32_t,3> sentinel{};
  if(!head || count>nodes.size() || !Tracked(head,sentinel,d) || !Tracked(std::uint64_t(head)+13,nil,d) || nil!=1)
    return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
  if(!count)return sentinel[0]==head && sentinel[1]==head && sentinel[2]==head || FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});
  struct Node {std::uint32_t address=0,parent=0,right=0;};std::array<Node,64> stack{};unsigned depth=0;
  auto current=sentinel[1],parent=head;std::uint32_t used=0;
  while(current!=head || depth){
    while(current!=head){std::array<std::uint32_t,3> node{};
      if(!current || depth==stack.size() || used+depth>=count || !Tracked(current,node,d) ||
         !Tracked(std::uint64_t(current)+13,nil,d) || nil || node[1]!=parent ||
         (node[0]==node[2] && node[0]!=head))return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});
      stack[depth++]={current,parent,node[2]};parent=current;current=node[0];
    }
    const auto node=stack[--depth];if(used==count)return FailCapture(d,CaptureFailure::CountMismatch,{facts.subject});
    nodes[used++]=node.address;current=node.right;parent=node.address;
  }
  if(used!=count || nodes[0]!=sentinel[0] || nodes[count-1]!=sentinel[2])return FailCapture(d,CaptureFailure::CountMismatch,{facts.subject});
  return true;
}
// 无序容器按真实双链表有界遍历；桶和管理值不复制，集合／聚合稍后只排序自有值。
bool EntitySource::Impl::HashItems(std::uint32_t address,bool pairs,std::span<std::byte> output,
    std::uint32_t &used,std::uint32_t &visited,CaptureDiagnostic &d) noexcept {
  std::array<std::uint32_t,2> list{},head{};if(!Tracked(std::uint64_t(address)+4,list,d))return false;
  const auto count=list[1],stride=pairs?8u:4u;
  if(!list[0] || count>16384-visited || std::uint64_t(count)*stride>output.size()-used || !Tracked(list[0],head,d))
    return FailCapture(d,CaptureFailure::ValueBudget,{facts.subject});
  visited+=count;auto node=head[0],previous=list[0];
  for(std::uint32_t i=0;i<count;++i){std::array<std::uint32_t,2> links{};
    if(!node || node==list[0] || !Tracked(node,links,d) || links[1]!=previous ||
       !Track(std::uint64_t(node)+8,output.subspan(used,stride),d))return FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});
    used+=stride;previous=node;node=links[0];
  }
  return node==list[0] && head[1]==previous || FailCapture(d,CaptureFailure::SourceChanged,{facts.subject});
}
bool EntitySource::Impl::Aggregate(std::uint32_t address,std::span<std::byte> output,std::uint32_t &count,CaptureDiagnostic &d) noexcept {
  auto storage=Allocate(16384u*4,4,d);if(d.reason!=CaptureFailure::None)return false;
  auto nodes=std::span(reinterpret_cast<std::uint32_t *>(storage.data()),storage.size()/4);std::uint32_t groups=0,used=0,visited=0;
  if(!NativeTree(address,nodes,groups,d))return false;
  for(std::uint32_t i=0;i<groups;++i){
    // std::map节点value+10h，键是24B string；只取其后的官方Innate当前两个计数表。
    const auto innate=std::uint64_t(nodes[i])+0x28;
    if(innate+0x4c>UINT32_MAX || !HashItems(std::uint32_t(innate),true,output,used,visited,d) ||
       !HashItems(std::uint32_t(innate+0x2c),true,output,used,visited,d))return false;
  }
  struct Count {std::int32_t id,count;};auto values=std::span(reinterpret_cast<Count *>(output.data()),used/8);
  std::sort(values.begin(),values.end(),[](auto a,auto b){return a.id<b.id;});count=0;
  for(const auto &item:values){if(item.count<0)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});if(!item.count)continue;
    if(count && values[count-1].id==item.id){const auto sum=std::int64_t(values[count-1].count)+item.count;
      if(sum>INT32_MAX)return FailCapture(d,CaptureFailure::InvalidValue,{facts.subject});values[count-1].count=std::int32_t(sum);}
    else values[count++]=item;
  }
  return true;
}

// open先读取数量／头和来源证言；执行器确认同组上限后才进入Element。
bool EntitySource::Impl::Open(const CaptureFieldAccess &a,CaptureTraversalSource &out,CaptureDiagnostic &d) noexcept {
  if(Recipe(a.field)==EntityRead::Presentation)return PresentationOpen(a,out,d);
  if(Recipe(a.field)==EntityRead::RockRubble || Recipe(a.field)==EntityRead::FireEmbers)return GridOpen(a,out,d);
  if(Recipe(a.field)==EntityRead::Scene || Recipe(a.field)==EntityRead::RoomConfig || Recipe(a.field)==EntityRead::Inline)return SceneOpen(a,out,d);
  const auto start=witness_used;out={};out.identity=a.root.address;
  const auto recipe=Recipe(a.field);const auto address=std::uint64_t(a.root.address)+a.field.native_offset;
  const auto contiguous=[&](std::uint32_t data,std::uint32_t count,std::uint32_t stride,NativeSequence kind=NativeSequence::Contiguous){
    if((count && !data) || !stride || std::uint64_t(data)+std::uint64_t(count)*stride>(std::uint64_t{1}<<32))return Reject(a,d,CaptureFailure::AddressRange);
    out.opaque[0]=data;out.opaque[1]=Sequence(stride,kind);out.count=count;return true;};
  if(recipe==EntityRead::Plus || recipe==EntityRead::Spoofs){std::uint16_t upper=0,lower=0;std::uint32_t holder=0,data=0;
    if(!Tracked(std::uint64_t(a.root.address)+0x18e,upper,d) || !Tracked(std::uint64_t(a.root.address)+0x192,lower,d))return false;
    holder=(std::uint32_t(upper)<<16)|lower;
    if(!holder || !Tracked(holder,data,d) || !data)return Reject(a,d,CaptureFailure::MissingValue);
    // PlayerPlus类型由真实EntityPlayer RTTI和Init owner决定；不调用可分配GetEntityPlayerPlus。
    if(recipe==EntityRead::Spoofs && (!facts.player_ready || a.root.address!=facts.address))return Reject(a,d,CaptureFailure::MissingQualification);
    const auto payload=std::uint64_t(data)+(recipe==EntityRead::Spoofs?16u:0u);
    if(payload>UINT32_MAX || !contiguous(std::uint32_t(payload),1,FindLayout(a.field.nested_layout)->native_size))return false;
    out.identity=(std::uint64_t(holder)<<32)|data;
  }else if(recipe==EntityRead::Shared){std::array<std::uint32_t,2> holder{};if(!Tracked(address,holder,d))return false;
    out.identity=address;out.present=holder[0]!=0;out.count=out.present?1u:0u;out.opaque[0]=holder[0];
    const auto *layout=FindLayout(a.field.nested_layout);if(!layout)return Reject(a,d,CaptureFailure::UnknownLayout);
    out.opaque[1]=Sequence(layout->native_size);
    if(out.present){if(!holder[1])return Reject(a,d,CaptureFailure::InvalidValue);
      CaptureRoot payload{a.root.subject,holder[0],layout->native_size,layout->id};
      if(!plan->Identify(a.window,payload,d) || payload.layout!=layout->id)return Reject(a,d,CaptureFailure::UnknownLayout);}
  }else if(recipe==EntityRead::Vector || recipe==EntityRead::TrinketPresence){std::array<std::uint32_t,3> header{};
    if(a.field.source!=FieldSource::Object || !Tracked(address,header,d))return false;
    const auto stride=a.field.native_stride;
    if(!stride || header[0]>header[1] || header[1]>header[2] || (!header[0] && (header[1]||header[2])) ||
       (header[1]-header[0])%stride || (header[2]-header[0])%stride)return Reject(a,d,CaptureFailure::InvalidValue);
    if(!contiguous(header[0],(header[1]-header[0])/stride,stride))return false;out.identity=address;
  }else if(recipe==EntityRead::Fixed){
    if(!a.field.fixed_count || address>UINT32_MAX || std::uint64_t(a.field.fixed_count)*a.field.native_stride>a.field.native_size ||
       a.field.native_offset>a.root.extent || a.field.native_size>a.root.extent-a.field.native_offset)return Reject(a,d,CaptureFailure::AddressRange);
    if(!contiguous(std::uint32_t(address),a.field.fixed_count,a.field.native_stride))return false;out.identity=address;
  }else if(recipe==EntityRead::String){
    auto bytes=Allocate(std::uint64_t(a.field.limit)+1,1,d);if(d.reason!=CaptureFailure::None)return false;std::uint32_t count=0;
    if(address>UINT32_MAX || !(a.format.storage==BodyStorage::CString?CStringAt(std::uint32_t(address),a.field.limit+1,bytes,count,d,true):StringAt(std::uint32_t(address),bytes.first(a.field.limit),count,d,true)))return false;
    if(!contiguous(NativeAddress(bytes),count,1,NativeSequence::Owned))return false;out.identity=address;
  }else if(recipe==EntityRead::Layers || recipe==EntityRead::LayerFrames || recipe==EntityRead::NullFrames){
    std::uint32_t data=0,count=0,owner=a.root.address;std::uint8_t loaded=0;
    if(recipe!=EntityRead::Layers && (!Tracked(a.root.address,owner,d) || !owner))return Reject(a,d,CaptureFailure::MissingValue);
    if(!Tracked(std::uint64_t(owner)+0x109,loaded,d) || loaded>1)return Reject(a,d,CaptureFailure::InvalidValue);
    if(loaded){
      if(recipe==EntityRead::Layers){if(!Tracked(std::uint64_t(a.root.address)+0x80,count,d))return false;}
      else{std::uint32_t animation=0;if(!Tracked(std::uint64_t(a.root.address)+4,animation,d))return false;
        if(animation && !Tracked(std::uint64_t(animation)+(recipe==EntityRead::LayerFrames?0x1cu:0x24u),count,d))return false;}
      if(count && !Tracked(address,data,d))return false;
    }
    if(!contiguous(data,count,a.field.native_stride))return false;out.identity=address;
  }else if(recipe==EntityRead::Ghost){std::int32_t count=0;if(!Tracked(std::uint64_t(a.root.address)+0x454,count,d) || count<0 || count>4)return Reject(a,d,CaptureFailure::CountMismatch);
    if(!contiguous(std::uint32_t(address),std::uint32_t(count),a.field.native_stride))return false;out.identity=address;
  }else if(recipe==EntityRead::Deque || recipe==EntityRead::Rope || recipe==EntityRead::Trail){
    std::array<std::uint32_t,5> header{};if(!Tracked(address,header,d))return false;
    const auto proxy=header[0],map=header[1],map_capacity=header[2],offset=header[3],count=header[4];
    const auto stride=a.field.native_stride;if(!stride)return Reject(a,d,CaptureFailure::InvalidValue);
    const auto per_block=stride<=1?16u:stride<=2?8u:stride<=4?4u:stride<=8?2u:1u;
    if(!proxy || count>a.field.limit || (!map_capacity?(map||offset||count):
       (!map || map_capacity<8 || map_capacity>32768 || (map_capacity&(map_capacity-1)) || count>std::uint64_t(map_capacity)*per_block ||
        std::uint64_t(map)+std::uint64_t(map_capacity)*4>UINT32_MAX)) || (recipe==EntityRead::Rope && !count))return Reject(a,d,CaptureFailure::CountMismatch);
    auto pointers=Allocate(std::uint64_t(count)*4,4,d);if(d.reason!=CaptureFailure::None)return false;
    for(std::uint32_t i=0;i<count;++i){const auto logical=std::uint64_t(offset)+i;std::uint32_t block=0;
      if(!Tracked(std::uint64_t(map)+((logical/per_block)&(map_capacity-1))*4,block,d) || !block ||
         std::uint64_t(block)+(logical%per_block)*stride+stride>(std::uint64_t{1}<<32))return Reject(a,d,CaptureFailure::AddressRange);
      const auto element=std::uint32_t(std::uint64_t(block)+(logical%per_block)*stride);std::memcpy(pointers.data()+i*4,&element,4);
    }
    if(!contiguous(NativeAddress(pointers),count,4,NativeSequence::Addresses))return false;out.identity=address;
  }else if(recipe==EntityRead::Tree){auto data=Allocate(std::uint64_t(a.field.limit)*4,4,d);if(d.reason!=CaptureFailure::None)return false;std::uint32_t count=0;
    auto nodes=std::span(reinterpret_cast<std::uint32_t *>(data.data()),data.size()/4);
    if(!NativeTree(std::uint32_t(address),nodes,count,d))return false;
    for(std::uint32_t i=0;i<count;++i){if(nodes[i]>UINT32_MAX-16)return Reject(a,d,CaptureFailure::AddressRange);nodes[i]+=16;}
    if(!contiguous(NativeAddress(data),count,4,NativeSequence::Addresses))return false;out.identity=address;
  }else if(recipe==EntityRead::Blocked || recipe==EntityRead::Innate){auto data=Allocate(16384u*8,8,d);if(d.reason!=CaptureFailure::None)return false;
    std::uint32_t count=0;
    if(recipe==EntityRead::Blocked){std::uint32_t used=0,visited=0;if(!HashItems(std::uint32_t(address),false,data,used,visited,d))return false;
      count=used/4;auto items=std::span(reinterpret_cast<std::int32_t *>(data.data()),count);std::sort(items.begin(),items.end());
      if(std::adjacent_find(items.begin(),items.end())!=items.end())return Reject(a,d,CaptureFailure::InvalidValue);
    }else if(!Aggregate(std::uint32_t(address),data,count,d))return false;
    if(!contiguous(NativeAddress(data),count,recipe==EntityRead::Blocked?4u:8u,NativeSequence::Owned))return false;out.identity=address;
  }else if(recipe==EntityRead::Weapon){auto data=Allocate(5*sizeof(NativeWeaponSlot),alignof(NativeWeaponSlot),d);if(d.reason!=CaptureFailure::None)return false;
    auto slots=reinterpret_cast<NativeWeaponSlot *>(data.data());
    for(unsigned slot=0;slot<5;++slot){std::uint32_t pointer=0,type=0;if(!Tracked(address+slot*4,pointer,d))return false;
      slots[slot]={};if(!pointer)continue;if(!Weapon(pointer,slots[slot],type,d,true))return false;out.slot_mask|=1u<<slot;}
    out.opaque[0]=NativeAddress(data);out.opaque[1]=Sequence(sizeof(NativeWeaponSlot),NativeSequence::Weapon);out.identity=address;
  }else if(recipe==EntityRead::DrawNode || recipe==EntityRead::DrawSprite){std::array<std::uint32_t,3> header{};
    if(!Tracked(a.root.address,header,d) || !header[0] || !header[1] || !header[2] || header[2]==header[0])return Reject(a,d,CaptureFailure::InvalidValue);
    if(!contiguous(recipe==EntityRead::DrawNode?header[2]:a.root.address+12,1,FindLayout(a.field.nested_layout)->native_size))return false;
  }else return Reject(a,d,CaptureFailure::MissingHandler);
  out.opaque[2]=start;out.opaque[3]=witness_used-start;
  if(a.field.operation==Operation::Container && out.count>a.field.limit)return Reject(a,d,CaptureFailure::CountMismatch);
  return true;
}
// 逻辑顺序在open期间已封闭，Element不重新读取count或猜测动态类。
bool EntitySource::Impl::Element(const CaptureFieldAccess &a,const CaptureTraversalSource &source,std::uint16_t index,
    CaptureRoot &out,CaptureDiagnostic &d) noexcept {
  const auto kind=NativeSequence(source.opaque[1]>>32);const auto stride=std::uint32_t(source.opaque[1]);
  const auto element=index==kNullIndex?0u:unsigned(index);
  if(kind==NativeSequence::Weapon){if(element>=5 || !(source.slot_mask&(1u<<element)))return Reject(a,d,CaptureFailure::InvalidValue);
    NativeWeaponSlot slot{};if(!Value(source.opaque[0]+element*sizeof(slot),slot,d))return false;
    out={a.root.subject,slot.address,slot.extent,slot.layout};return true;}
  if(element>=source.count)return Reject(a,d,CaptureFailure::CountMismatch);
  auto address=source.opaque[0]+std::uint64_t(element)*stride;
  if(kind==NativeSequence::Addresses){std::uint32_t pointer=0;if(!Value(address,pointer,d))return false;address=pointer;}
  const auto *layout=FindLayout(a.field.nested_layout);
  if(!layout || !address || address>UINT32_MAX || layout->native_size>(std::uint64_t{1}<<32)-address)return Reject(a,d,CaptureFailure::AddressRange);
  out={a.root.subject,std::uint32_t(address),layout->native_size,layout->id};return true;
}
bool EntitySource::Impl::Close(const CaptureFieldAccess &a,const CaptureTraversalSource &source,CaptureDiagnostic &d) noexcept {
  if(source.opaque[2]>witness_used || source.opaque[3]>witness_used-source.opaque[2])return Reject(a,d,CaptureFailure::InvalidTransaction);
  std::array<std::byte,32> bytes{};
  for(std::uint64_t i=source.opaque[2];i<source.opaque[2]+source.opaque[3];++i){const auto &mark=witnesses[std::size_t(i)];
    if(!Read(mark.address,std::span(bytes).first(mark.size),d) || !std::equal(bytes.begin(),bytes.begin()+mark.size,mark.bytes.begin()))
      return Reject(a,d,CaptureFailure::SourceChanged);}
  return true;
}
// 标量容器消费当前已经预检的游标；读出的原始指针仍在当前同步栈转为自有handle。
bool EntitySource::Impl::ScalarContainer(const CaptureFieldAccess &a,CaptureValueWriter &writer,CaptureDiagnostic &d) noexcept {
  if(!a.traversal || a.field.nested_layout)return Reject(a,d,CaptureFailure::InvalidTransaction);
  const auto &source=*a.traversal;const auto count=std::uint64_t(source.count)*a.format.components;
  const auto bytes=count*CaptureScalarWidth(a.format.scalar);
  const auto sequence=NativeSequence(source.opaque[1]>>32);const auto stride=std::uint32_t(source.opaque[1]);
  const auto element_address=[&](std::uint32_t index,std::uint64_t &address){address=source.opaque[0]+std::uint64_t(index)*stride;
    if(sequence==NativeSequence::Addresses){std::uint32_t pointer=0;if(!Value(address,pointer,d))return false;address=pointer;}return true;};
  // 原生Entity*数组先解析同窗身份；deque的逻辑地址表不能作为指针值本身发送。
  if(a.format.linked){if(a.format.resource_table!=255 || a.format.components!=1 ||
      (sequence!=NativeSequence::Addresses && stride!=4))return Reject(a,d,CaptureFailure::InvalidValue);
    auto storage=Allocate(std::uint64_t(source.count)*sizeof(CaptureLink),alignof(CaptureLink),d);if(d.reason!=CaptureFailure::None)return false;
    auto links=std::span(reinterpret_cast<CaptureLink *>(storage.data()),source.count);
    for(std::uint32_t i=0;i<source.count;++i){std::uint64_t address=0;std::uint32_t pointer=0;
      if(!element_address(i,address) || !Value(address,pointer,d))return false;
      links[i]=services.reference(services.owner,pointer);if(links[i].kind==CapturedReferenceKind::Invalid)return Reject(a,d,CaptureFailure::ReferencePending);}
    return writer.PutLinks(a.location,false,255,links);
  }
  const auto width=a.format.components*CaptureScalarWidth(a.format.scalar);
  if(count>UINT32_MAX || (sequence!=NativeSequence::Addresses && stride!=width))return Reject(a,d,CaptureFailure::InvalidValue);
  auto values=Allocate(bytes,8,d);if(d.reason!=CaptureFailure::None)return false;
  if(sequence==NativeSequence::Addresses){for(std::uint32_t i=0;i<source.count;++i){std::uint64_t address=0;
      if(!element_address(i,address) || !Read(address,values.subspan(std::size_t(i)*width,width),d))return false;}}
  else if(bytes && !Read(source.opaque[0],values,d))return false;
  if(Recipe(a.field)==EntityRead::TrinketPresence)for(auto &b:values)b=b==std::byte{}?std::byte{}:std::byte{1};
  return writer.PutScalar(a.location,a.format.scalar,std::uint32_t(count),values);
}
}
