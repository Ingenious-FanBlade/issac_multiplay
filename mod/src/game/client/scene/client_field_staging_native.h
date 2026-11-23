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
  bool present=false,payload_present=false,retained=false,update_live=false,resize_live_sequence=false;
  bool create_live_owner=false;
  // 仅可信数字叶整理出的字符串／集合键／聚合值；不是变长正文或网络长度头。
  const std::byte *data=nullptr;std::size_t bytes=0;
  std::array<ClientStagedWeaponInput,5> weapons{};
};
struct ClientStagedField;
ClientStagedField *ClientStagedFieldCreate(ClientPreparationAllocator) noexcept;
ClientPreparationResult ClientStagedFieldConstruct(ClientStagedField *,const ClientStagedFieldInput &) noexcept;
bool ClientStagedFieldMaterialize(ClientStagedField *,const ClientStagedFieldInput &) noexcept;
bool ClientStagedFieldCurrent(const ClientStagedField *) noexcept;
bool ClientStagedFieldRetain(ClientStagedField *) noexcept;
bool ClientStagedFieldRelease(ClientStagedField *&) noexcept;
// 只在当前用途栈借用。容器／payload 的所有权、分配账目和析构前缀继续留在字段 owner。
// Group 是无需独立分配的数字路径；没有伪造原生对象。嵌套字段按数字 field 索引分别取承载。
void *ClientStagedFieldElement(ClientStagedField *,std::uint32_t) noexcept;
// 仅供 InspectStaged 已核定后、尚未进入任何原生调用的描述片使用。
void *ClientInspectedFieldElement(ClientStagedField *,std::uint32_t) noexcept;
std::size_t ClientInspectedFieldAttachmentSize(const ClientStagedField *) noexcept;
bool ClientInspectedFieldCanAttach(const ClientStagedField *,void *,std::size_t) noexcept;
// 安装租约保留原header和全部分配账目。原生根退休前必须先Detach；禁止根与字段双重析构。
// 在已挂接的原生容器上改值或调整经核定的扁平标量 vector 长度。
bool ClientStagedFieldInstallValues(ClientStagedField *,const ClientStagedFieldInput &) noexcept;
bool ClientLiveVectorLayout(std::uint16_t layout,std::uint32_t stride) noexcept;
bool ClientStagedFieldAttach(ClientStagedField *,void *,std::size_t) noexcept;
bool ClientStagedFieldBorrowAttachment(ClientStagedField *,void *,std::size_t) noexcept;
bool ClientStagedFieldAttached(const ClientStagedField *,const void *) noexcept;
// 作用域失效时本次未改动附件链。保管链不一致需要继续保留原生租约。
enum class ClientAttachmentCommit { Committed, ScopeLost, InvalidChain };
ClientAttachmentCommit ClientStagedFieldCommitCheckedAttachment(ClientStagedField *,const void *destination) noexcept;
bool ClientStagedFieldDetach(ClientStagedField *) noexcept;
} // namespace cs::game
