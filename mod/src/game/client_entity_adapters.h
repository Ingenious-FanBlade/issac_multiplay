#pragma once
#include "game/client_objects_native.h"
namespace cs::replica { struct EntityObjectTarget; }
namespace cs::game {
// 类别目标只含自有值；raw存储偏移来自共同声明，不接受网络提供的偏移/函数地址。
struct ClientEntityClassState final {
  std::vector<ClientEntityField> fields;
  struct Resource final {std::string member;unsigned index=0;bool is_null=false;ClientResourcePose pose;};
  std::vector<Resource> resources;
  bool delirium=false;
  // 官方EntityPlus的optional<bool>：-1=Null，0=false，1=true，不能把Null压成false。
  int flying_override=-1;
};
// 模型阶段一次解释完整派生payload，原生步骤仅消费此不可变结果。
void ClientDescribeEntityClass(const replica::EntityObjectTarget &,ClientObjectDescription &);
std::vector<ClientEntityClassAdapter> ClientEntityBuiltinAdapters();
const char *ClientEntityClassDisposition(const std::string &) noexcept;
} // namespace cs::game
