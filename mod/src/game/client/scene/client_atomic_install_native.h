#pragma once
#include "game/client/scene/client_field_staging_native.h"
#include "game/client/scene/client_read_window.h"
#include <cstddef>
#include <cstdint>
#include <array>

namespace cs::game {
// 只由主线程可信字段绑定器产生，不接受网络地址、函数或偏移。extent 是实际承载范围。
enum class ClientAtomicWriteKind:unsigned {Bytes,Masked32,EntityReference,FieldAttachment};
struct ClientAtomicWrite {
  ClientAtomicWriteKind kind=ClientAtomicWriteKind::Bytes;
  void *destination=nullptr;
  std::size_t extent=0,size=0;
  const std::byte *value=nullptr;
  void *reference=nullptr;
  std::uint32_t mask=0;
  ClientStagedField *field=nullptr;
  bool retained_attachment=false;
};
// SDK 边界不解析正文；引用通过真实 EntityPtr 协议安装。副本缓冲由本Peer安装器持有。
// 只在不释放现有承载的连续读写片内复用页查询；自有header安装保管旧承载，
// 原生回调、EntityPtr setter及可能释放承载的操作仍必须清空。
struct ClientAtomicReadRegion {
  std::uintptr_t begin=0,end=0;
  // 同一纯写片常交替访问对象和容器，有序定长缓存避免逐项扫描和小环反复淘汰。
  // 仍随整片跨回调清空，不延长任何原生借用或页保护见证的寿命。
  std::array<std::array<std::uintptr_t,2>,64> recent{};
  // 引用目标可能重复出现；只复用本连续检查段的可读页，vptr仍每项实际读取。
  // 与写区见证同时撤销，不能穿过EntityPtr setter、附件操作或其它原生回调。
  ClientNativeReadWindow references;
  unsigned used=0;
  // 回调边界立即撤销全部页见证；旧槽位不在有效前缀内，后续必须重新查询。
  void Clear()noexcept {begin=end=0;used=0;references.Clear();}
};
// original 为空时仅核写区和 ABI；有回滚责任的写入紧邻执行时才取原值。
bool ClientAtomicWritePreflight(const ClientAtomicWrite &,ClientAtomicReadRegion &) noexcept;
bool ClientAtomicWriteCapture(const ClientAtomicWrite &,std::byte *original,ClientAtomicReadRegion &) noexcept;
// 片入口核主线程安装作用域和借用；以下纯内存操作仅供片内使用。
bool ClientAtomicCopyWindow() noexcept;
bool ClientAtomicClearRetiringReference(void *slot,const void *target,bool entity_ptr) noexcept;
// CapturedFailure表示原值完整而写入可能不完整，调用者必须登记done并撤销。
// attempted 非空时回报是否已进入原生写入：false 表示形状／作用域检查未通过，原生对象未改动，
// 调用者不得把这类失败当作“写了一半”的不确定状态。
bool ClientAtomicWriteApply(const ClientAtomicWrite &,bool *attempted=nullptr) noexcept;
// 只读诊断：当前是否处于有效主线程安装作用域、原生图像提交队列是否为空。
bool ClientAtomicScopeActive() noexcept;
bool ClientAtomicQueuesEmpty() noexcept;
bool ClientAtomicWriteCheck(const ClientAtomicWrite &) noexcept;
bool ClientAtomicDetachField(const ClientAtomicWrite &) noexcept;
// 快速路径纯值写入（设计 10.6 节）。调用者已在写段入口核主线程安装作用域（ClientAtomicCopyWindow）、
// 对象安装实例和承载链；这里不逐页查询，只用 SEH 收住访问异常。mask 非 0 时按 32 位掩码合并。
// 返回 false 表示写入触发了结构异常，写区内容不确定。
bool ClientAtomicFastWrite(void *destination,const std::byte *value,std::size_t size,std::uint32_t mask) noexcept;
bool ClientAtomicInstallWindow(const ClientNativeRoots &,void *vm) noexcept;
// 菜单 Game／Room 跨连接存在，记录只包含生成计划确认的根标量原值。
bool ClientAtomicRootScalar(const ClientNativeRoots &,const void *,std::size_t) noexcept;
bool ClientAtomicRestoreRootScalar(const ClientNativeRoots &,void *,const void *,std::size_t) noexcept;
} // namespace cs::game
