#pragma once
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
namespace cs::game::player_sdk {
// 官方1.1.3公开源码的Release容器布局；不引入其带全局初始化的配置/文件写入实现头。
// 这里只读动态类型并借用已存在容器，绝不构造/析构此header；原始Plus仍归官方owner。
struct PlusHeader {virtual ~PlusHeader()=default;std::optional<bool> flying;std::optional<unsigned> water;};
struct Timed {int id,count,timer;};
struct Innate {std::unordered_map<int,int> persistent;std::vector<Timed> temporary;std::unordered_map<int,int> totals;};
struct Spoof {void *vptr;std::unordered_set<int> blocked;std::map<std::string,Innate> groups;};
struct PlusItems {Spoof collectible,trinket;int last_type;};
static_assert(sizeof(PlusHeader)==16,"J273 official Plus base layout");
}
