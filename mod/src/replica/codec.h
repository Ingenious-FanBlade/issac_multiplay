#pragma once
#include "replica/model.h"
#include <span>
namespace cs::replica {
// v1/v2共用无损值标签；顶层消息schema与传输版本分别由各协议检查。
std::vector<std::uint8_t> EncodeValue(const Value &,Limits={});
Json DecodeValue(std::span<const std::uint8_t>,Limits={});
// 独立ICSR v1协议；固定小端头+有类型值树，不发送JSON envelope或C++内存。
Lane MessageLane(Kind);
std::vector<std::uint8_t> Encode(const Message &,Limits={});
Message Decode(std::span<const std::uint8_t>,Limits={});
std::vector<std::vector<std::uint8_t>> Fragment(const Message &,std::uint64_t transfer,Limits={});
// 每Peer自有重组器，乱序可接纳，冲突副本失败；超时和容量只影响该Peer。
class Reassembler final {
public:
  explicit Reassembler(Limits limits={}):limits_(limits){}
  bool Receive(std::span<const std::uint8_t>,std::uint64_t now_ms,Message &out);
  void Expire(std::uint64_t now_ms);
  std::size_t Pending() const noexcept { return pending_.size(); }
private:
  struct Candidate { std::uint64_t started=0;std::uint32_t total=0,count=0;std::vector<std::vector<std::uint8_t>> parts;std::size_t bytes=0; };
  Limits limits_;
  std::map<std::uint64_t,Candidate> pending_;
};
// 接收模型窗口的版本/冲突检查不等于原生apply；ACK由上层主线程单独产生。
class ModelWindow final {
public:
  explicit ModelWindow(Scope scope):scope_(scope){}
  bool Stage(const Message &);
  const Json &Current() const noexcept {return current_;}
private:
  Scope scope_;Json current_;std::map<std::uint64_t,Json> baselines_;
  std::map<std::uint64_t,std::vector<std::uint8_t>> seen_;
};
enum class EventDecision { Ready, Duplicate, Waiting, Expired, WrongScene };
class EventWindow final {
public:
  explicit EventWindow(Scope s):scope_(s){}
  void Baseline(std::uint64_t watermark);
  EventDecision Accept(const Message &,std::uint64_t applied,std::uint64_t tick);
private:
  Scope scope_;std::uint64_t watermark_=0;std::map<std::uint64_t,std::string> played_;
};
// 可靠控制仅定义有序、幂等和跨lane版本依赖，不在15实现网络重传线程。
class ControlWindow final {
public:
  explicit ControlWindow(Scope s):scope_(s){}
  bool Accept(const Message &,std::uint64_t applied);
private:
  Scope scope_;std::uint64_t next_=1;std::map<std::uint64_t,std::string> history_;
};
} // namespace cs::replica
