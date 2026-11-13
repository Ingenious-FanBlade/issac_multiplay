#pragma once
#include "game/client/scene/client_field_staging_native.h"
#include <cstddef>
#include <cstdint>

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
};
// SDK 边界不解析正文；引用通过真实 EntityPtr 协议安装。副本缓冲由本Peer安装器持有。
bool ClientAtomicWritePreflight(const ClientAtomicWrite &,std::byte *original,std::size_t) noexcept;
bool ClientAtomicWriteApply(const ClientAtomicWrite &) noexcept;
bool ClientAtomicWriteCheck(const ClientAtomicWrite &) noexcept;
bool ClientAtomicWriteRestore(const ClientAtomicWrite &,const std::byte *original,std::size_t) noexcept;
bool ClientAtomicInstallWindow(const ClientNativeRoots &,void *vm) noexcept;
// 菜单 Game／Room 跨连接存在，记录只包含生成计划确认的根标量原值。
bool ClientAtomicRootScalar(const ClientNativeRoots &,const void *,std::size_t) noexcept;
bool ClientAtomicRestoreRootScalar(const ClientNativeRoots &,void *,const void *,std::size_t) noexcept;
} // namespace cs::game
