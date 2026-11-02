#pragma once
// tx-04-15 独立复制合同：只拥有纯值，不依赖SDK、Lua、游戏地址或录制状态。
#include <rapidjson/document.h>
#include <cstdint>
#include <exception>
#include <map>
#include <string>
#include <vector>
#include "world/common/contracts.h"
#include "core/participant.h"

namespace cs::replica {
using Json=rapidjson::Document;
using Value=rapidjson::Value;
using Allocator=rapidjson::Document::AllocatorType;
// 协议边界预算；协商只能收紧，不允许来自报文的长度扩大分配上限。
struct Limits final {
  // 用户要求压缩优化前先放宽至64M节点；字节/深度约束独立，不按报文扩大。
  std::uint32_t bytes=32u<<20, nodes=world::observation::kNodeBudget, depth=64, string_bytes=2u<<20;
  std::uint32_t datagram=1200, candidates=2, fragments=32768, timeout_ms=5000;
};
enum class Kind : std::uint8_t { Hello=1, Welcome, Baseline, Delta, Event, Control, InputAck, AppliedAck, Error, ReceiveAck, Component, Input };
enum class Lane : std::uint8_t { Input=1, Control, Recovery, State, Presentation };
// scope各代次显式携带；connection与控制组、run与观测epoch分别独立。
struct Scope final {
  std::uint64_t session=0, connection=0, run=0, floor=0, dimension=0, room=0, scene=0;
  bool operator==(const Scope &) const = default;
};
struct Message final { Kind kind=Kind::Error; std::uint64_t sequence=0; Json body; };
struct ContractError final : std::exception {
  std::string code;
  explicit ContractError(std::string c):code(std::move(c)){}
  const char *what() const noexcept override { return code.c_str(); }
};
Json Parse(const std::string &,Limits={});
std::string Text(const Value &);
Json Copy(const Value &);
void CheckValue(const Value &,Limits={});
void ValidateModel(const Value &);
Value Decimal(std::uint64_t,Allocator &);
std::uint64_t Unsigned(const Value &);
void Put(Value &,const char *,Value,Allocator &);
void Put(Value &,const char *,const std::string &,Allocator &);
Value ScopeValue(const Scope &,Allocator &);
Scope ReadScope(const Value &);
// 目录生成物为每个成员／回调／未公开边界提供归属、理由和应用任务。
const Json &Catalog();
const Value *Policy(const std::string &);

// 稳定身份只从Authority已登记的源主体映射；每个实例有界，不回绕、不复用ID。
class Registry final {
public:
  explicit Registry(std::uint64_t session,std::uint64_t run):session_(session),run_(run){}
  void BindSource(std::uint64_t session,std::uint64_t epoch,const std::string &run);
  Value Key(unsigned space,const std::string &local,const std::string &use,Allocator &);
  Value Project(const Value &,Allocator &,std::uint64_t epoch=0,unsigned depth=0);
  std::size_t Size() const noexcept { return keys_.size(); }
private:
  std::uint64_t session_,run_,next_=0;
  std::uint64_t source_epoch_=0;
  std::string source_run_;
  std::map<std::string,std::uint64_t> keys_;
  std::map<std::string,std::string> current_uses_;
};
// 每个component保留自己的capture kind、tick、scope和依赖；不把独立回调拼成原子世界。
Json Capture(const Scope &,std::uint64_t revision,
             const std::vector<world::observation::ObservationEnvelope> &,Registry &);
Json CaptureRows(const Scope &,std::uint64_t revision,const std::vector<Json> &,Registry &);
Message Independent(const Scope &,const Value &observation,Registry &);
Json Difference(const Value &baseline,const Value &target);
Json Reconstruct(const Value &baseline,const Value &delta);
// 模型DTO确认与原生应用确认分开；只有调用者完成实际应用才能建立Applied ACK。
Message Applied(const Scope &,std::uint64_t snapshot,std::uint64_t watermark,bool native_committed);
Message InputProgress(const Scope &,std::uint64_t participant,std::uint64_t generation,std::uint64_t sequence,std::uint64_t tick);
Message InputIntent(const Scope &,unsigned participant,std::uint64_t generation,std::uint64_t sequence,std::uint64_t target,const core::InputStateDraftV2 &);
Json Capabilities();
Json Negotiate(const Value &local,const Value &remote);
} // namespace cs::replica
