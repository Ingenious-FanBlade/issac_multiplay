#pragma once
#include "game/client_objects_native.h"
#include "replica/entity_plan.h"
namespace cs::game {
// 共同字段只在模型线程解析为自有值；分类adapter之后在主线程消费具名派生payload。
ClientObjectDescription ClientDescribeEntity(const replica::EntityObjectTarget &,const std::vector<replica::EntityDependency> &,
    std::shared_ptr<const std::string>,bool player_structure=false);
const char *ClientEntityCommonDisposition(const std::string &) noexcept;
} // namespace cs::game
