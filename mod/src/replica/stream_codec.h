#pragma once
#include "replica/codec.h"
#include "replica/stream_model.h"

namespace cs::replica::stream {
enum class Kind : unsigned { Hello=1, Welcome, Ready, ScenePrepare, ScenePrepared, Range, ReceiveAck, ContextReady, AppliedAck, Resync, Cancel, Stop, Error, Roster, TickProgress, InputAck, Input, Disconnect };
enum class Lane : unsigned { Control=1, Baseline, Context, WorldDynamic, PlayerFast, Input };
enum class Direction { ToAuthority, ToClient };
enum class State { Handshake, Waiting, Preparing, Active, Retiring, Stopped };
enum class Codec : unsigned { None, Zstd };
// 封套按连接/路由持有；范围正文中的WorldScope仍然可在全部Peer间共享。
struct Message final {
  Kind kind=Kind::Error;
  std::uint64_t sequence=0,connection=0,route=0;
  Json body;
};
struct Header final {
  std::uint64_t connection=0,route=0,transfer=0;
  std::uint32_t wire_bytes=0,decoded_bytes=0,index=0,count=0;
  Kind kind=Kind::Error;
  Lane lane=Lane::Control;
  Codec codec=Codec::None;
  std::uint16_t payload=0;
};
Lane MessageLane(const Message &);
void ValidateMessage(const Message &);
void ValidateReceive(const Message &,Direction,State);
Json Capabilities();
Json Negotiate(const Value &,const Value &);
std::vector<std::uint8_t> Encode(const Message &,Limits={});
Message Decode(std::span<const std::uint8_t>,Limits={});
Message RangeMessage(const RangeData &,std::uint64_t connection,std::uint64_t route);
Range ReadRange(const Message &,const Layouts &,std::uint64_t value_budget=256ull<<20);
// 先做完整嵌套形状检查；本地必需字段布局仍由ReadRange的可信Layouts约束。
void ValidateRangePayload(const Message &);
// I04固定64字节路由格式；授权预算/选择补片/大消息调度由I13—I17使用同一个头接入。
Header ReadHeader(std::span<const std::uint8_t>,Limits={});
std::vector<std::vector<std::uint8_t>> Fragment(const Message &,std::uint64_t transfer,Limits={});
class Reassembler final {
public:
  explicit Reassembler(Limits l):limits_(l){}
  // 协商在完整握手消息后生效；退休旧候选，不能让旧预算下的分配越过新上限。
  void Configure(Limits limits) noexcept {pending_.clear();limits_=limits;}
  bool Receive(std::span<const std::uint8_t>,std::uint64_t now,Message &);
private:
  struct Candidate {Header header;std::uint64_t started=0;std::size_t bytes=0;std::vector<std::vector<std::uint8_t>> parts;};
  Limits limits_;
  std::map<std::tuple<std::uint64_t,std::uint64_t,std::uint64_t>,Candidate> pending_;
};
} // namespace cs::replica::stream
