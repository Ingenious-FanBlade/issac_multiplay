#pragma once
#include "game/client/entity/client_objects_native.h"
#include "replica/entity_plan.h"
namespace cs::game {
// 共同字段只在模型线程解析为自有值；分类adapter之后在主线程消费具名派生payload。
ClientObjectDescription ClientDescribeEntity(const replica::EntityObjectTarget &,const std::vector<replica::EntityDependency> &,
    std::shared_ptr<const std::string>,bool player_structure=false);
const char *ClientEntityCommonDisposition(const std::string &) noexcept;
// 已核结构上的完整局部值只解析一次；关系复用的是精确Structure版本，不是上一帧动态值。
replica::EntityObjectTarget ClientStreamEntity(const ClientObjectDescription &,const replica::Value &);
ClientObjectDescription ClientDescribeStreamEntity(const ClientObjectDescription &,const replica::EntityObjectTarget &);
std::vector<ClientEntityField> ClientStreamCommonFields(const replica::Value &);
std::vector<ClientEntityField> ClientStreamPlayerFields(const replica::Value &,int player_type);
} // namespace cs::game
