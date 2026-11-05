#include "replica/stream_transport.h"
#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <new>
#include <cstddef>

namespace cs::replica::stream {
namespace {
[[noreturn]] void Fail(const char *s){throw ContractError(s);}
std::uint64_t Micros(){return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
void U(std::vector<std::uint8_t> &out,std::uint64_t n,unsigned width){for(unsigned i=0;i<width;++i)out.push_back(std::uint8_t(n>>(8*i)));}
std::uint64_t Read(std::span<const std::uint8_t> b,std::size_t &p,unsigned n){if(n>b.size()-p)Fail("bundle_truncated");std::uint64_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint64_t(b[p++])<<(8*i);return v;}
void Set(std::vector<std::uint8_t> &b,std::size_t p,std::uint64_t n){for(unsigned i=0;i<8;++i)b[p+i]=std::uint8_t(n>>(8*i));}
bool Same(const Header &a,const Header &b){return a.connection==b.connection&&a.route==b.route&&a.transfer==b.transfer&&a.kind==b.kind&&a.lane==b.lane&&a.codec==b.codec&&a.wire_bytes==b.wire_bytes&&a.decoded_bytes==b.decoded_bytes&&a.count==b.count;}
void Z(size_t result,const char *reason){if(ZSTD_isError(result))Fail(reason);}
// zstd每笔实际分配在malloc之前计费，含上下文/窗口；回调异常转换为C API的分配失败。
struct alignas(std::max_align_t) Allocation {memory::Lease lease;};
void *Allocate(void *,size_t n) noexcept {
  try {if(n>SIZE_MAX-sizeof(Allocation))return nullptr;auto lease=memory::Reserve(memory::Pool::Workspace,n+sizeof(Allocation));auto *p=std::malloc(n+sizeof(Allocation));if(!p)return nullptr;new(p) Allocation{std::move(lease)};return static_cast<Allocation *>(p)+1;}catch(...){return nullptr;}
}
void Release(void *,void *p) noexcept {if(!p)return;auto *block=static_cast<Allocation *>(p)-1;block->~Allocation();std::free(block);}
ZSTD_customMem Allocator(){return {Allocate,Release,nullptr};}
void Decompress(const std::vector<std::uint8_t> &wire,std::vector<std::uint8_t> &out,std::uint32_t expected){
  ZSTD_FrameHeader header{};const auto status=ZSTD_getFrameHeader(&header,wire.data(),wire.size());Z(status,"zstd_frame_invalid");if(status)Fail("zstd_frame_truncated");
  if(header.frameType!=ZSTD_frame||header.dictID)Fail("zstd_frame_dictionary_or_skippable");
  if(header.windowSize>(8ull<<20))Fail("zstd_window_limit");if(header.frameContentSize!=expected)Fail("zstd_decoded_length");
  const auto size=ZSTD_findFrameCompressedSize(wire.data(),wire.size());if(ZSTD_isError(size))Fail(ZSTD_getErrorCode(size)==ZSTD_error_srcSize_wrong?"zstd_frame_truncated":"zstd_frame_invalid");if(size!=wire.size())Fail("zstd_trailing_frame");
  std::unique_ptr<ZSTD_DCtx,decltype(&ZSTD_freeDCtx)> ctx(ZSTD_createDCtx_advanced(Allocator()),ZSTD_freeDCtx);if(!ctx)Fail("zstd_workspace_exhausted");Z(ZSTD_DCtx_setParameter(ctx.get(),ZSTD_d_windowLogMax,23),"zstd_window_parameter");
  out.resize(expected);const auto n=ZSTD_decompressDCtx(ctx.get(),out.data(),out.size(),wire.data(),wire.size());Z(n,"zstd_decompress_failed");if(n!=expected)Fail("zstd_decoded_length");
}
bool WorldLane(Lane lane){return lane==Lane::Baseline||lane==Lane::Context||lane==Lane::WorldDynamic||lane==Lane::PlayerFast;}
bool WorldValid(const WorldScope &w){return w.session&&w.run&&w.floor&&w.dimension&&w.room&&w.scene;}
void RouteHeader(const Header &h,const Route &route,bool routed){
  if(!routed){if(WorldLane(h.lane)||h.codec!=Codec::None)Fail("stream_world_not_authorized");return;}
  if(!route.connection||!route.epoch||route.epoch==UINT64_MAX)Fail("stream_local_route_invalid");
  if(h.kind!=Kind::Hello&&(h.connection!=route.connection||h.route!=route.epoch))Fail("stream_route_not_authorized");
  if(h.kind==Kind::Hello&&route.direction!=Direction::ToAuthority)Fail("stream_handshake_route");
  if(WorldLane(h.lane)&&(!route.negotiated||!route.world))Fail("stream_world_not_authorized");
  if(WorldLane(h.lane)&&!WorldValid(route.scope))Fail("stream_world_scope_not_authorized");
  if(WorldLane(h.lane)&&route.direction!=Direction::ToClient)Fail("stream_world_wrong_direction");
  if(h.codec==Codec::Zstd&&(!route.zstd||!route.world||!Reliable(h.lane)))Fail("stream_codec_not_negotiated");
}
}
Limits HandshakeLimits() noexcept {Limits l;l.bytes=65536;l.nodes=4096;l.depth=24;l.string_bytes=8192;l.fragments=64;l.candidates=2;l.timeout_ms=1000;return l;}
Limits WorldLimits(bool fast) noexcept {Limits l;l.timeout_ms=2000;if(fast){l.bytes=65536;l.nodes=32768;l.fragments=64;}return l;}
Limits NegotiatedWorldLimits(const Value &caps){
  auto agreed=Negotiate(Capabilities(),caps);const auto &v=agreed["world_limits"];auto l=WorldLimits();
  l.bytes=unsigned(Unsigned(v["bytes"]));l.nodes=unsigned(Unsigned(v["nodes"]));l.depth=unsigned(Unsigned(v["depth"]));l.string_bytes=unsigned(Unsigned(v["string_bytes"]));l.fragments=unsigned(Unsigned(v["fragments"]));l.candidates=unsigned(Unsigned(v["candidates"]));l.value_bytes=Unsigned(v["value_bytes"]);return l;
}
bool Reliable(Lane lane) noexcept {return lane==Lane::Baseline||lane==Lane::Context;}
SharedBlob EncodeBlob(const Message &m,bool compress,Limits limits){
  if(limits.datagram<=64||limits.datagram>1200)Fail("stream_datagram_budget");
  const auto lane=MessageLane(m);if(!Reliable(lane)){const auto cap=WorldLane(lane)?WorldLimits(true):HandshakeLimits();limits.bytes=(std::min)(limits.bytes,cap.bytes);limits.nodes=(std::min)(limits.nodes,cap.nodes);limits.depth=(std::min)(limits.depth,cap.depth);limits.string_bytes=(std::min)(limits.string_bytes,cap.string_bytes);limits.fragments=(std::min)(limits.fragments,cap.fragments);}
  if(compress&&!Reliable(lane))Fail("stream_compression_lane");
  // Encode同时持有payload与完整wire；工作区在函数退出/异常时归还。
  auto workspace=memory::Reserve(memory::Pool::Workspace,2ull*limits.bytes);const auto started=Micros();auto wire=Encode(m,limits);const auto encoded=Micros();workspace->Resize(wire.capacity());
  // 路由封套不属于共享内容；接收端按已经授权的分片头恢复这16字节再做严格解码。
  Set(wire,24,0);Set(wire,32,0);
  auto blob=std::make_shared<Blob>();blob->decoded_bytes=std::uint32_t(wire.size());blob->sequence=m.sequence;blob->kind=m.kind;blob->lane=lane;if(m.range)blob->range_id=m.range->id;blob->encode_us=encoded-started;
  if(WorldLane(lane)){if(m.range)blob->world=m.range->world;else {const auto &w=m.body["world"];blob->world={Unsigned(w["session"]),Unsigned(w["run"]),Unsigned(w["floor"]),Unsigned(w["dimension"]),Unsigned(w["room"]),Unsigned(w["scene"])};}}
  if(compress){
    const auto bound=ZSTD_compressBound(wire.size());Z(bound,"zstd_compress_bound");blob->memory=memory::Reserve(memory::Pool::Wire,bound+sizeof(Blob));blob->bytes.resize(bound);
    std::unique_ptr<ZSTD_CCtx,decltype(&ZSTD_freeCCtx)> ctx(ZSTD_createCCtx_advanced(Allocator()),ZSTD_freeCCtx);if(!ctx)Fail("zstd_workspace_exhausted");
    Z(ZSTD_CCtx_setParameter(ctx.get(),ZSTD_c_compressionLevel,3),"zstd_level_parameter");Z(ZSTD_CCtx_setParameter(ctx.get(),ZSTD_c_windowLog,23),"zstd_window_parameter");Z(ZSTD_CCtx_setParameter(ctx.get(),ZSTD_c_contentSizeFlag,1),"zstd_size_parameter");
    const auto n=ZSTD_compress2(ctx.get(),blob->bytes.data(),blob->bytes.size(),wire.data(),wire.size());Z(n,"zstd_compress_failed");if(n>limits.bytes)Fail("zstd_wire_budget");blob->bytes.resize(n);blob->codec=Codec::Zstd;
  }else {blob->memory=memory::Reserve(memory::Pool::Wire,wire.capacity()+sizeof(Blob));blob->bytes=std::move(wire);}
  blob->compression_us=compress?Micros()-encoded:0;return blob;
}
std::vector<std::uint8_t> Datagram(const Blob &blob,TransferKey key,std::uint32_t index,Limits l){
  if(l.datagram<=64||l.datagram>1200||!key.transfer||key.transfer==UINT64_MAX||blob.bytes.empty()||blob.bytes.size()>l.bytes||blob.decoded_bytes>l.bytes)Fail("stream_send_description");
  const auto chunk=l.datagram-64,count=(blob.bytes.size()+chunk-1)/chunk;if(!count||count>l.fragments||index>=count)Fail("stream_fragment_count");
  const auto offset=std::size_t(index)*chunk,n=(std::min)(std::size_t(chunk),blob.bytes.size()-offset);
  std::vector<std::uint8_t> out={'I','C','R','F'};out.reserve(64+n);U(out,2,2);U(out,64,2);U(out,key.connection,8);U(out,key.route,8);U(out,key.transfer,8);U(out,blob.bytes.size(),4);U(out,blob.decoded_bytes,4);U(out,index,4);U(out,count,4);U(out,unsigned(blob.kind),2);U(out,unsigned(blob.lane),1);U(out,unsigned(blob.codec),1);U(out,n,2);U(out,0,2);U(out,0,8);out.insert(out.end(),blob.bytes.begin()+offset,blob.bytes.begin()+offset+n);ReadHeader(out,l);return out;
}
bool IsBundle(std::span<const std::uint8_t> b) noexcept {return b.size()>=4&&!std::memcmp(b.data(),"ICRB",4);}
std::vector<std::vector<std::uint8_t>> Unbundle(std::span<const std::uint8_t> b){
  if(!IsBundle(b)||b.size()>1200||b.size()<28)Fail("bundle_header");std::size_t p=4;if(Read(b,p,2)!=2||Read(b,p,2)!=28)Fail("bundle_version");const auto connection=Read(b,p,8),route=Read(b,p,8),count=Read(b,p,2);if(Read(b,p,2)||!count||count>16||!connection||!route||route==UINT64_MAX)Fail("bundle_description");std::vector<std::vector<std::uint8_t>> out;out.reserve(std::size_t(count));
  for(unsigned i=0;i<count;++i){const auto n=Read(b,p,2),kind=Read(b,p,2),lane=Read(b,p,1);if(Read(b,p,1))Fail("bundle_reserved");const auto sequence=Read(b,p,8),transfer=Read(b,p,8);if(n>b.size()-p||n<2||n>1096||!sequence||sequence==UINT64_MAX||kind<1||kind>20)Fail("bundle_length_or_sequence");
    // 共用路由从容器恢复；子消息只传完整compact payload，不重复64/40字节封套。
    Blob blob;blob.kind=Kind(kind);blob.lane=Lane(lane);blob.sequence=sequence;blob.decoded_bytes=unsigned(n+40);blob.bytes={'I','C','S','R'};blob.bytes.reserve(std::size_t(n+40));U(blob.bytes,2,2);U(blob.bytes,40,2);U(blob.bytes,kind,2);U(blob.bytes,lane,1);U(blob.bytes,0,1);U(blob.bytes,n,4);U(blob.bytes,sequence,8);U(blob.bytes,0,8);U(blob.bytes,0,8);blob.bytes.insert(blob.bytes.end(),b.begin()+p,b.begin()+p+std::size_t(n));out.push_back(Datagram(blob,{connection,route,transfer},0));p+=std::size_t(n);
  }if(p!=b.size())Fail("bundle_trailing_bytes");return out;
}
std::vector<std::vector<std::uint8_t>> Bundle(const std::vector<std::vector<std::uint8_t>> &packets){
  std::vector<std::vector<std::uint8_t>> out;std::vector<std::uint8_t> current;std::uint64_t connection=0,route=0;unsigned count=0;
  const auto flush=[&]{if(!count)return;current[24]=std::uint8_t(count);out.push_back(std::move(current));current.clear();count=0;};
  for(const auto &p:packets){const auto h=ReadHeader(p);if(h.count!=1||h.codec!=Codec::None||!h.connection||p.size()<106){flush();out.push_back(p);continue;}
    const auto payload=p.size()-104;
    if(count&&(h.connection!=connection||h.route!=route||current.size()+22+payload>1200||count==16))flush();
    if(!count){connection=h.connection;route=h.route;current={'I','C','R','B'};U(current,2,2);U(current,28,2);U(current,connection,8);U(current,route,8);U(current,0,2);U(current,0,2);}
    U(current,payload,2);U(current,unsigned(h.kind),2);U(current,unsigned(h.lane),1);U(current,0,1);std::size_t at=80;U(current,Read(p,at,8),8);U(current,h.transfer,8);current.insert(current.end(),p.begin()+104,p.end());++count;
  }flush();return out;
}
Message AckMessage(const ReceiveBitmap &ack,std::uint64_t sequence,Route route){
  Message m;m.memory=memory::Reserve(memory::Pool::Values,131072);m.kind=Kind::ReceiveAck;m.sequence=sequence;m.connection=route.connection;m.route=route.epoch;m.body.SetObject();auto &a=m.body.GetAllocator();Put(m.body,"transfer",Decimal(ack.transfer,a),a);Put(m.body,"part_count",Decimal(ack.part_count,a),a);Put(m.body,"window",Decimal(ack.window,a),a);Value bits(rapidjson::kArrayType);for(auto n:ack.bits)bits.PushBack(Value(n),a);Put(m.body,"bitmap",std::move(bits),a);Put(m.body,"complete",Value(ack.complete),a);ValidateMessage(m);return m;
}
ReceiveBitmap ReadAck(const Message &m){ValidateMessage(m);if(m.kind!=Kind::ReceiveAck)Fail("stream_not_receive_ack");ReceiveBitmap a;a.transfer=Unsigned(m.body["transfer"]);a.part_count=unsigned(Unsigned(m.body["part_count"]));a.window=unsigned(Unsigned(m.body["window"]));for(unsigned i=0;i<4;++i)a.bits[i]=Unsigned(m.body["bitmap"][i]);a.complete=m.body["complete"].GetBool();return a;}
void RttEstimator::Observe(std::uint64_t ms) noexcept {
  ms=(std::min)(ms,10000ull);
  if(!initialized_){mean_=ms;variation_=ms/2;initialized_=true;}
  else {const auto delta=mean_>ms?mean_-ms:ms-mean_;variation_=(3*variation_+delta)/4;mean_=(7*mean_+ms)/8;}
  rto_=std::clamp(mean_+(std::max)(1ull,4*variation_),100ull,1000ull);
}
std::uint64_t TransferDeadline(std::uint64_t started,std::uint64_t now,std::uint64_t remaining,std::uint64_t rate) noexcept {
  const auto absolute=started>UINT64_MAX-120000?UINT64_MAX:started+120000;
  const auto duration=2000+(remaining*1000+(std::max)(1ull,rate)-1)/(std::max)(1ull,rate);
  return (std::min)(absolute,now>UINT64_MAX-duration?UINT64_MAX:now+duration);
}
void Reassembler::Configure(Limits l) noexcept {pending_.clear();retired_.clear();retired_before_=0;limits_=HandshakeLimits();limits_.bytes=(std::min)(limits_.bytes,l.bytes);limits_.nodes=(std::min)(limits_.nodes,l.nodes);limits_.depth=(std::min)(limits_.depth,l.depth);limits_.string_bytes=(std::min)(limits_.string_bytes,l.string_bytes);limits_.fragments=(std::min)(limits_.fragments,l.fragments);limits_.candidates=(std::min)(limits_.candidates,l.candidates);limits_.timeout_ms=(std::min)(limits_.timeout_ms,l.timeout_ms);limits_.value_bytes=(std::min)(limits_.value_bytes,l.value_bytes);}
void Reassembler::Authorize(Route r) noexcept {if(!routed_||r.connection!=route_.connection||r.epoch!=route_.epoch||r.world!=route_.world||r.zstd!=route_.zstd||r.negotiated!=route_.negotiated||r.scope!=route_.scope){pending_.clear();retired_.clear();retired_before_=0;}route_=r;routed_=true;}
void Reassembler::Retire(std::uint64_t transfer) noexcept {try{retired_.push_back(transfer);if(retired_.size()>128){retired_before_=(std::max)(retired_before_,retired_.front());retired_.pop_front();}}catch(...){retired_before_=(std::max)(retired_before_,transfer);}}
void Reassembler::Cancel(std::uint64_t transfer) noexcept {for(auto i=pending_.begin();i!=pending_.end();)if(i->first.transfer==transfer)i=pending_.erase(i);else ++i;Retire(transfer);}
void Reassembler::Expire(std::uint64_t now){for(auto i=pending_.begin();i!=pending_.end();){auto &c=i->second;const auto age=now>=c.started?now-c.started:UINT64_MAX,stall=now>=c.progress?now-c.progress:UINT64_MAX;const auto timeout=WorldLane(c.header.lane)?2000:limits_.timeout_ms;if(age>=120000||(!c.complete&&(stall>=timeout||now>=c.deadline))){Retire(i->first.transfer);i=pending_.erase(i);++statistics_.expired;}else ++i;}}
bool Reassembler::Receive(std::span<const std::uint8_t> b,std::uint64_t now,Message &out){
  // 小预算只读头后，才根据本地授权选择世界上限；每个数据报的lane不可自授额度。
  const auto h=ReadHeader(b,WorldLimits());RouteHeader(h,route_,routed_);const bool world=WorldLane(h.lane);auto limits=world?WorldLimits(h.lane==Lane::WorldDynamic||h.lane==Lane::PlayerFast):limits_;
  if(world){const auto &p=route_.world_limits;limits.bytes=(std::min)(limits.bytes,p.bytes);limits.nodes=(std::min)(limits.nodes,p.nodes);limits.depth=(std::min)(limits.depth,p.depth);limits.string_bytes=(std::min)(limits.string_bytes,p.string_bytes);limits.fragments=(std::min)(limits.fragments,p.fragments);limits.candidates=(std::min)(limits.candidates,p.candidates);limits.value_bytes=(std::min)(limits.value_bytes,p.value_bytes);}
  ReadHeader(b,limits);Expire(now);++statistics_.fragments;statistics_.payload_bytes+=b.size();
  const TransferKey key{h.connection,h.route,h.transfer};auto i=pending_.find(key);
  if(i==pending_.end()){
    if(h.transfer<=retired_before_||std::find(retired_.begin(),retired_.end(),h.transfer)!=retired_.end())return false;
    unsigned active=0,fast=0;for(const auto &[k,c]:pending_)if(!c.complete){if(Reliable(c.header.lane))++active;else ++fast;}
    if(Reliable(h.lane)?active>=limits.candidates:fast>=limits.candidates)Fail("stream_candidate_budget");
    // 完成历史允许淘汰，活动可靠依赖绝不为新目标让位；外部已借出的DTO租约独立存活。
    auto trim=[&]{for(auto x=pending_.begin();x!=pending_.end();++x)if(x->second.complete){Retire(x->first.transfer);pending_.erase(x);return true;}return false;};
    while(pending_.size()>=130&&trim()){}
    Candidate c;c.header=h;c.started=c.progress=now;c.ack_due=now+50;c.deadline=TransferDeadline(now,now,h.wire_bytes,rate_);
    const auto charge=std::uint64_t(h.wire_bytes)+(std::uint64_t(h.count)+63)/64*8+(h.count+255)/256+sizeof(Candidate)+128;
    for(;;){try{c.memory=memory::Reserve(memory::Pool::Wire,charge);break;}catch(const std::runtime_error &){if(!trim())Fail("stream_reassembly_pool_exhausted");}}
    c.bytes.resize(h.wire_bytes);c.bits.resize((h.count+63)/64);c.dirty.resize((h.count+255)/256);i=pending_.emplace(key,std::move(c)).first;
  }
  auto &c=i->second;if(!Same(c.header,h)){Cancel(h.transfer);++statistics_.conflicts;Fail("stream_fragment_conflict");}
  const auto offset=std::size_t(h.index)*(limits.datagram-64);const auto bit=1ull<<(h.index%64);auto &word=c.bits[h.index/64];
  if(word&bit){if(std::memcmp(c.bytes.data()+offset,b.data()+64,h.payload)){Cancel(h.transfer);++statistics_.conflicts;Fail("stream_fragment_conflict");}++statistics_.duplicates;if(Reliable(h.lane)){c.dirty[h.index/256]=true;if(c.complete)c.ack_due=now;}return false;}
  std::memcpy(c.bytes.data()+offset,b.data()+64,h.payload);word|=bit;++c.received;c.progress=now;c.deadline=TransferDeadline(c.started,now,std::uint64_t(h.count-c.received)*1228,rate_);c.dirty[h.index/256]=true;
  if(h.index+1==h.count||h.index>c.cursor)c.ack_due=now;while(c.cursor<h.count&&(c.bits[c.cursor/64]&(1ull<<(c.cursor%64))))++c.cursor;
  if(c.received!=h.count)return false;
  try {
    const auto decode_started=Micros();
    auto workspace=memory::Reserve(memory::Pool::Workspace,std::uint64_t(h.decoded_bytes)+128);std::vector<std::uint8_t> decoded;
    if(h.codec==Codec::Zstd)Decompress(c.bytes,decoded,h.decoded_bytes);else decoded=c.bytes;
    if(decoded.size()<40)Fail("stream_decoded_header");for(unsigned j=24;j<40;++j)if(decoded[j])Fail("stream_shared_envelope");Set(decoded,24,h.connection);Set(decoded,32,h.route);
    auto message=Decode(decoded,limits);if(message.kind!=h.kind||MessageLane(message)!=h.lane)Fail("stream_fragment_envelope_conflict");
    if(world&&(!message.range||message.range->world!=route_.scope))Fail("stream_world_scope_conflict");
    ValidateReceive(message,routed_?route_.direction:Direction::ToClient,route_.world?State::Preparing:route_.negotiated?State::Waiting:State::Handshake);
    c.complete=true;c.ack_due=now;out=std::move(message);++statistics_.decoded;statistics_.decoded_bytes+=h.decoded_bytes;statistics_.decode_us+=Micros()-decode_started;
    if(!Reliable(h.lane)){if(world)Retire(h.transfer);pending_.erase(i);}return true;
  }catch(...){Cancel(h.transfer);throw;}
}
std::vector<ReceiveBitmap> Reassembler::Acks(std::uint64_t now,unsigned max_windows){
  Expire(now);std::vector<ReceiveBitmap> out;max_windows=(std::min)(max_windows,8u);
  for(auto &[key,c]:pending_){if(!Reliable(c.header.lane)||now<c.ack_due)continue;
    if(c.complete&&out.size()<max_windows){ReceiveBitmap a;a.transfer=key.transfer;a.part_count=c.header.count;a.window=((c.header.count-1)/256)*256;a.complete=true;for(unsigned j=0;j<4;++j){const auto index=a.window/64+j;if(index<c.bits.size())a.bits[j]=c.bits[index];}out.push_back(a);std::fill(c.dirty.begin(),c.dirty.end(),false);c.ack_due=now+200;continue;}
    for(unsigned window=0;window<c.dirty.size()&&out.size()<max_windows;++window)if(c.dirty[window]){ReceiveBitmap a;a.transfer=key.transfer;a.part_count=c.header.count;a.window=window*256;for(unsigned j=0;j<4;++j)if(window*4+j<c.bits.size())a.bits[j]=c.bits[window*4+j];out.push_back(a);c.dirty[window]=false;}
    c.ack_due=now+50;
  }statistics_.ack_windows+=out.size();return out;
}
std::uint64_t Reassembler::NextWake(std::uint64_t now) const noexcept {auto next=now+1000;for(const auto &[key,c]:pending_){next=(std::min)(next,c.started+120000);if(Reliable(c.header.lane))next=(std::min)(next,c.ack_due);if(!c.complete)next=(std::min)(next,c.progress+(WorldLane(c.header.lane)?2000:limits_.timeout_ms));}return (std::max)(now,next);}
void ReliableSender::Authorize(Route r) noexcept {if(r.connection!=route_.connection||r.epoch!=route_.epoch||!r.negotiated||!r.world||r.zstd!=route_.zstd||r.scope!=route_.scope){transfers_.clear();last_transfer_=0;results_.clear();}route_=r;}
void ReliableSender::Start(SharedBlob blob,std::uint64_t id,std::uint64_t now){
  if(!route_.negotiated||!route_.world||!route_.connection||!route_.epoch||!blob||!Reliable(blob->lane)||id<=last_transfer_||id==UINT64_MAX||!WorldValid(route_.scope)||blob->world!=route_.scope)Fail("stream_send_not_authorized");if(blob->codec==Codec::Zstd&&!route_.zstd)Fail("stream_codec_not_negotiated");if(transfers_.size()>=route_.world_limits.candidates)Fail("stream_send_candidate_budget");
  const auto count=(blob->bytes.size()+1135)/1136;if(!count||count>route_.world_limits.fragments||blob->bytes.size()>route_.world_limits.bytes||blob->decoded_bytes>route_.world_limits.bytes)Fail("stream_fragment_count_or_peer_budget");
  Transfer t;t.blob=std::move(blob);t.id=id;t.started=t.progress=now;t.deadline=TransferDeadline(now,now,count*1228,rate_);t.count=unsigned(count);t.memory=memory::Reserve(memory::Pool::Wire,count*11+sizeof(Transfer)+128);t.sent.resize(count);t.acked.resize(count);t.sent_at.resize(count);t.retransmitted.resize(count);transfers_.emplace(id,std::move(t));last_transfer_=id;
}
bool ReliableSender::Acknowledge(const Message &m,std::uint64_t now){
  if(m.connection!=route_.connection||m.route!=route_.epoch)Fail("stream_ack_route");const auto a=ReadAck(m);auto i=transfers_.find(a.transfer);if(i==transfers_.end())return false;auto &t=i->second;if(a.part_count!=t.count)Fail("stream_ack_count");
  for(unsigned j=0;j<256&&a.window+j<t.count;++j)if((a.bits[j/64]&(1ull<<(j%64)))&&!t.sent[a.window+j])Fail("stream_ack_unsent");
  if(a.complete&&std::find(t.sent.begin(),t.sent.end(),false)!=t.sent.end())Fail("stream_ack_unsent_completion");bool progress=false;
  for(unsigned j=0;j<256&&a.window+j<t.count;++j)if(a.bits[j/64]&(1ull<<(j%64))){if(!t.acked[a.window+j]){progress=true;if(!t.retransmitted[a.window+j]&&now>=t.sent_at[a.window+j])rtt_.Observe(now-t.sent_at[a.window+j]);}t.acked[a.window+j]=true;}
  if(a.complete){if(results_.size()>=128)Fail("stream_result_queue_full");results_.push_back({a.transfer,true,"received_complete"});transfers_.erase(i);return true;}if(progress){t.progress=now;const auto remaining=std::count(t.acked.begin(),t.acked.end(),false);t.deadline=TransferDeadline(t.started,now,std::uint64_t(remaining)*1228,rate_);}return progress;
}
void ReliableSender::Cancel(std::uint64_t id) noexcept {transfers_.erase(id);}
bool ReliableSender::Poll(Result &out){if(results_.empty())return false;out=results_.front();results_.pop_front();return true;}
bool ReliableSender::AllSent(std::uint64_t id) const noexcept {auto i=transfers_.find(id);return i!=transfers_.end()&&std::find(i->second.sent.begin(),i->second.sent.end(),false)==i->second.sent.end();}
void ReliableSender::Confirm(std::uint64_t id){auto i=transfers_.find(id);if(i==transfers_.end())return;if(!AllSent(id))Fail("stream_semantic_ack_unsent");if(results_.size()>=128)Fail("stream_result_queue_full");results_.push_back({id,true,"semantic_receipt_complete"});transfers_.erase(i);}
unsigned ReliableSender::Pump(std::uint64_t now,unsigned budget,const Send &send){
  unsigned sent=0,remaining=unsigned(transfers_.size());auto i=transfers_.upper_bound(round_robin_);
  while(remaining&&sent<budget){if(i==transfers_.end())i=transfers_.begin();auto current=i++;auto &t=current->second;round_robin_=t.id;
    const auto quota=sent+(std::max)(1u,(budget-sent)/remaining);--remaining;
    if(now<t.started||now-t.started>=120000||now>=t.deadline||now<t.progress||now-t.progress>=2000){if(results_.size()>=128)Fail("stream_result_queue_full");results_.push_back({t.id,false,"stream_transfer_no_progress_or_deadline"});transfers_.erase(current);++statistics_.expired;continue;}
    // 轮转寻找未发送/到期缺片；仅缺片RTO到期重发，已确认位永不重发。
    const bool probe=std::find(t.acked.begin(),t.acked.end(),false)==t.acked.end();
    unsigned examined=0;while(examined++<t.count&&sent<quota){const auto part=t.cursor++%t.count;if((t.acked[part]&&!(probe&&part+1==t.count))||(t.sent[part]&&now>=t.sent_at[part]&&now-t.sent_at[part]<rtt_.Rto()))continue;
      auto lease=memory::Reserve(memory::Pool::Wire,1200);const auto packet=Datagram(*t.blob,{route_.connection,route_.epoch,t.id},part);const auto result=send(packet);if(result==SendResult::WouldBlock){t.cursor=part;return sent;}if(result==SendResult::Failed)Fail("stream_socket_send_failed");if(t.sent[part]){++statistics_.retransmitted;t.retransmitted[part]=true;}t.sent[part]=true;t.sent_at[part]=now;++sent;++statistics_.fragments;statistics_.payload_bytes+=packet.size();
    }
  }return sent;
}
std::uint64_t ReliableSender::NextWake(std::uint64_t now) const noexcept {std::uint64_t next=now+1000;for(const auto &[id,t]:transfers_){bool missing=false;next=(std::min)(next,(std::min)(t.progress+2000,t.deadline));for(unsigned i=0;i<t.count;++i)if(!t.acked[i]){missing=true;next=(std::min)(next,t.sent[i]?t.sent_at[i]+rtt_.Rto():now);}if(!missing)next=(std::min)(next,t.sent_at.back()+rtt_.Rto());}return (std::max)(now,next);}
} // namespace cs::replica::stream
