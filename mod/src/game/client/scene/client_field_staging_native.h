#pragma once
#include "game/client/scene/client_preparation_native.h"
#include <array>

namespace cs::game {
// detached 表示只属于候选的原生承载；它们从未写入现有对象，也没有发布成员关系。
enum class ClientStagedFieldKind:unsigned {Group,Rows,Vector,String,CString,Deque,Map,Blocked,Innate,Seeds,Shared,Weapons,Records};
struct ClientStagedWeaponInput {std::uint16_t layout=0;std::uint32_t type=0,size=0;};
struct ClientStagedFieldInput {
  ClientStagedFieldKind kind=ClientStagedFieldKind::Group;
  std::uint16_t layout=0;
  std::uint32_t count=0,stride=0,slot_mask=0;
  bool present=false,payload_present=false;
  // 仅可信数字叶整理出的字符串／集合键／聚合值；不是变长正文或网络长度头。
  const std::byte *data=nullptr;std::size_t bytes=0;
  std::array<ClientStagedWeaponInput,5> weapons{};
};
struct ClientStagedField;
ClientStagedField *ClientStagedFieldCreate(ClientPreparationAllocator) noexcept;
ClientPreparationResult ClientStagedFieldConstruct(ClientStagedField *,const ClientStagedFieldInput &) noexcept;
bool ClientStagedFieldCurrent(const ClientStagedField *) noexcept;
bool ClientStagedFieldRelease(ClientStagedField *&) noexcept;
// 只在当前用途栈借用。容器／payload 的所有权、分配账目和析构前缀继续留在字段 owner。
// Group 是无需独立分配的数字路径；没有伪造原生对象。嵌套字段按数字 field 索引分别取承载。
const void *ClientStagedFieldContainer(const ClientStagedField *) noexcept;
void *ClientStagedFieldElement(ClientStagedField *,std::uint32_t) noexcept;
// 安装租约保留原header和全部分配账目。原生根退休前必须先Detach；禁止根与字段双重析构。
std::size_t ClientStagedFieldAttachmentSize(const ClientStagedField *) noexcept;
bool ClientStagedFieldCanAttach(const ClientStagedField *,void *,std::size_t) noexcept;
bool ClientStagedFieldAttach(ClientStagedField *,void *,std::size_t) noexcept;
bool ClientStagedFieldAttached(const ClientStagedField *,const void *) noexcept;
// 成功发布才折叠前一安装租约；失败时仍能拆下候选并归还前一header。
bool ClientStagedFieldCommitAttachment(ClientStagedField *) noexcept;
bool ClientStagedFieldDetach(ClientStagedField *) noexcept;
} // namespace cs::game
