#pragma once
#include "replica/codec.h"
#include "replica/stream_model.h"
namespace cs::state {struct HostStateBytes;}

namespace cs::replica::stream {
enum class Kind : unsigned { Hello=1, Welcome, Ready, ScenePrepare, ScenePrepared, ReceiveAck=7, AppliedAck=9, Resync, Cancel, Stop, Error, Roster, TickProgress, InputAck, Input, Disconnect, OperationAck, Retire, StateFrame, StateResources=23, LobbySelection, LobbyRoster };
enum class Lane : unsigned { Control=1, Baseline, Context, WorldDynamic, PlayerFast, Input };
enum class Direction { ToAuthority, ToClient };
enum class State { Handshake, Waiting, Preparing, Active, Retiring, Stopped };
enum class Codec : unsigned { None, Zstd };
// 可选能力独立于原zstd-v1；旧Peer的可靠压缩支持不自动允许world/body压缩。
inline constexpr const char *kWorldBodyZstdCapability="world-body-zstd-v1";
// 同布局的旧接收端要等 Resources 才会回报；只有双方声明后才可省略该消息。
inline constexpr const char *kStateTableDeltaCapability="state-table-delta-v1";
// v2：LobbyRoster 增加 unlocked（房主解锁位图）；与 v1 不兼容，混用时按 live_lobby_required 拒绝。
inline constexpr const char *kLobbyCapability="lobby-v2";
// 封套按连接/路由持有；范围正文中的WorldScope仍然可在全部Peer间共享。
struct Message final {
  memory::Lease memory;
  Kind kind=Kind::Error;
  std::uint64_t sequence=0,connection=0,route=0;
  Json body;
  // ICSR flags=1专用于现行二进制状态正文；控制信封仍用现有固定槽编码。
  std::shared_ptr<const state::HostStateBytes> state_body;
  Lane state_lane=Lane::Control;
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
// 已有自有二进制公共头的scope；不解析typed状态或授予应用资格。
WorldScope StateMessageScope(const Message &);
void ValidateMessage(const Message &);
void ValidateReceive(const Message &,Direction,State);
Json Capabilities();
Json Negotiate(const Value &,const Value &);
// 可传入调用者工作区租约，返回后继续覆盖wire至压缩/所有权交接完成。
std::vector<std::uint8_t> Encode(const Message &,Limits={},memory::Lease workspace={});
Message Decode(std::span<const std::uint8_t>,Limits={});
// 固定payload schema=2；具名结构/字段编号与最短整数保持原值，未知形状仍保留完整树。
std::vector<std::uint8_t> EncodePayload(const Message &,Limits,std::uint64_t expected_bytes=0);
// 调用者先完成消息形状校验；计量与正式编码采用同一节点/深度/字符串约束。
std::uint64_t MeasurePayload(const Message &,Limits);
// 对已持有的纯值计量同一编码器的真实长度；不创建网络消息、不提升发送上限。
void DecodePayload(std::span<const std::uint8_t>,Message &,Limits);
// 先做完整嵌套形状检查；本地必需字段布局仍由ReadRange的可信Layouts约束。
// 固定64字节路由格式；授权预算/补片共用stream_transport的同一描述。
Header ReadHeader(std::span<const std::uint8_t>,Limits={});
std::vector<std::vector<std::uint8_t>> Fragment(const Message &,std::uint64_t transfer,Limits={});
} // namespace cs::replica::stream
#include "replica/stream_transport.h"
