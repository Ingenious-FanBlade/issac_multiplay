#pragma once
#include "net/worker.h"
#include "replica/codec.h"
#include "replica/stream_codec.h"
#include <memory>

namespace cs::net::live {
// 真实游戏使用ICSR/ICRF v2；等待世界仍用小预算，不因协议支持而提前开放世界传输。
replica::Limits Limits() noexcept;
std::uint64_t Random();
bool IsPacket(std::span<const std::uint8_t>) noexcept;
using Packets = std::vector<std::vector<std::uint8_t>>;
Packets Pack(const replica::Message &, std::uint64_t &transfer);
replica::Message Control(const replica::Scope &, std::uint64_t sequence,
                         const char *operation, replica::Json parameters);
// 当前输入桥的内部DTO适配v2显式消息；旧v1只供离线工具，真实socket不接受v1混合正文。
class Reassembler final {
public:
  void Scope(replica::Scope s) noexcept {scope_=s;authority_=true;}
  void Configure(replica::Limits l) noexcept {wire_.Configure(l);}
  bool Receive(std::span<const std::uint8_t>,std::uint64_t,replica::Message &);
private:
  replica::stream::Reassembler wire_{Limits()};
  replica::Scope scope_{};
  bool authority_=false;
};

// 单网络owner维护一个真实游戏Peer；主线程仍经原Worker队列处理输入和发布进度。
class HostPeer final {
public:
  HostPeer(std::uint64_t session, unsigned ordinal);
  // 仅完整协商后交付Input；Hello重复只重发同一Welcome，不重新分配身份。
  bool Receive(std::span<const std::uint8_t>, std::uint64_t now,
               ReceivedInput &, const HostIdentity &);
  Packets Reply();
  Packets Progress(const protocol::Ack &);
  Packets Bye(const char *reason);
  Packets Reject(const char *reason);
  bool Established() const noexcept { return established_; }
  bool Stopped() const noexcept { return stopped_; }
  std::uint64_t LastReceive() const noexcept { return last_receive_; }
  std::uint64_t Nonce() const noexcept { return nonce_; }
private:
  Reassembler assembler_;
  replica::Scope scope_{};
  unsigned ordinal_;
  bool established_ = false, stopped_ = false;
  std::uint64_t nonce_ = 0, server_nonce_ = 0, transfer_ = 0,
                control_ = 0, roster_ = 0, last_receive_ = 0, client_control_ = 0;
  // ready回显只估计传输时间，不信任或应用远端墙钟到游戏对象。
  std::uint64_t echo_client_ms_ = 0, echo_server_ms_ = 0;
  protocol::Ack sent_{};
  replica::Message welcome_;
  bool reply_ = false;
};

// 固定大小的跨线程通知；网络线程不持有Player、设备、Lua或SDK对象。
struct Event final {
  enum class Type { Welcome, Progress, InputAck, Stopped, Failed } type = Type::Failed;
  replica::Scope scope{};
  unsigned ordinal = 0;
  protocol::Ack progress{};
  // 本机收包时刻、往返传输时间和服务器发布排队时间，均为有界纯值。
  std::uint64_t received_ms = 0, round_trip_ms = 0, publication_age_ms = 0;
  char reason[128]{};
};
struct Intent final {
  core::InputStateDraftV2 state{};
  std::uint64_t generation = 0, sequence = 0, target = 0;
};
// 每次连接使用新的worker；停止只请求，主线程轮询完成，不在DLL卸载里等待。
class Client final {
public:
  Client() noexcept = default;
  ~Client();
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;
  const char *Start(const std::string &ipv4, std::uint16_t port,
                    std::uint32_t timeout_ms) noexcept;
  bool Send(const Intent &) noexcept;
  bool Poll(Event &) noexcept;
  void Stop() noexcept;
  bool Finished() const noexcept;
private:
  struct State;
  std::shared_ptr<State> state_;
  void *thread_ = nullptr;
};
} // namespace cs::net::live
