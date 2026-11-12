#pragma once
#include "state/capture_values.h"
#include <optional>

namespace cs::state {
enum class CaptureLane : std::uint8_t { Ordinary, Body };
enum class CaptureControlKind : std::uint8_t { Begin, Stop, Invalidate, ColdRequest, RetireBody };
struct CaptureControl {
  CaptureControlKind kind=CaptureControlKind::Invalidate;
  CaptureFailure reason=CaptureFailure::None;
  std::uint32_t generation=0,watermark=0,order=0,cold_request=0;
  CaptureSubject subject;
};
// 所有原子状态均使用Win32上保证无锁的u32；不依赖atomic<shared_ptr>或64位原子锁实现。
struct CaptureExchangeConfig {
  std::uint32_t buffers=20,body_lanes=8;
  CaptureCapacity capacity;
  // 非零时前N份专供ordinary大包，其余专供body；避免为每个快槽重复预留整房容量。
  std::uint32_t ordinary_buffers=0;
  CaptureCapacity ordinary_capacity;
};
struct CaptureExchangeStats {
  std::uint32_t published=0,replaced=0,revoked=0,joint_requests=0;
  bool failed=false;
  CaptureFailure failure=CaptureFailure::None;
};
// 没有内置线程或游戏依赖。预分配的buffer在Free→Writing→Ready→Reading→Free之间转移，
// 生命周期owner须在安全退休期释放exchange；票据只保留自有内存，不保留原生对象。
class CaptureExchange final {
  struct State;
  struct Key {std::uint32_t slot=UINT32_MAX,serial=0,generation=0,watermark=0,cold_request=0;CaptureLane lane=CaptureLane::Ordinary;CaptureSubject subject;
    std::uint32_t body_slot=UINT32_MAX,body_generation=0;};
public:
  class WriteTicket final {
  public:
    WriteTicket()=default;
    ~WriteTicket();
    WriteTicket(WriteTicket &&) noexcept;
    WriteTicket &operator=(WriteTicket &&) noexcept;
    WriteTicket(const WriteTicket &)=delete;
    WriteTicket &operator=(const WriteTicket &)=delete;
    CaptureValueWriter *Values() noexcept;
    // 只在真实外层提交后调用；window.current在此再次核对完整来源，未提交候选不得入队。
    bool Publish(const CaptureWindow &,const CaptureRoot &anchor,std::uint32_t expected_roots) noexcept;
    void Reset() noexcept;
  private:
    friend class CaptureExchange;
    WriteTicket(std::shared_ptr<State>,Key,CaptureWitness) noexcept;
    std::shared_ptr<State> state_;
    Key key_;
    std::optional<CaptureValueWriter> writer_;
  };
  class ReadTicket final {
  public:
    ReadTicket()=default;
    ~ReadTicket();
    ReadTicket(ReadTicket &&) noexcept;
    ReadTicket &operator=(ReadTicket &&) noexcept;
    ReadTicket(const ReadTicket &)=delete;
    ReadTicket &operator=(const ReadTicket &)=delete;
    const CapturePacket *Packet() const noexcept;
    bool Current() const noexcept;
    void Reset() noexcept;
    std::uint32_t ColdRequest() const noexcept {return key_.cold_request;}
  private:
    friend class CaptureExchange;
    ReadTicket(std::shared_ptr<State>,Key) noexcept;
    std::shared_ptr<State> state_;
    Key key_;
  };
  // 构造在冷准备期完成；两个真实线程身份由owner显式提供，不由首次随机调用抢占。
  CaptureExchange(CaptureExchangeConfig,std::thread::id producer,std::thread::id consumer);
  CaptureExchange(const CaptureExchange &)=delete;
  CaptureExchange &operator=(const CaptureExchange &)=delete;
  bool Begin() noexcept;
  bool Stop(CaptureFailure=CaptureFailure::Revoked) noexcept;
  bool Invalidate(CaptureFailure) noexcept;
  bool RequestCold() noexcept;
  bool RetireBody(CaptureSubject) noexcept;
  WriteTicket Acquire(CaptureLane,CaptureWitness,CaptureSubject,CaptureFailure &failure) noexcept;
  // 控制FIFO先消费；有未取控制时Take不交付候选。错误／溢出撤销全部发布资格。
  bool PopControl(CaptureControl &) noexcept;
  ReadTicket Take() noexcept;
  CaptureExchangeStats Statistics() const noexcept;
  bool Drained() const noexcept;
  std::uint64_t RetainedBytes() const noexcept;
private:
  bool Control(CaptureControlKind,CaptureFailure,CaptureSubject={}) noexcept;
  std::shared_ptr<State> state_;
};
}
