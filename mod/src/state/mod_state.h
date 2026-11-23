#pragma once
#include "core/replication_memory.h"
#include <cstddef>
#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

namespace cs::state {
// kModGlobal：global 作用域的目标值，也表示"实体引用为空或未解析"；kModPublic：公开记录的接收者。
// kModValueBytes 为单条值上限 64 KiB；kModRecordHeaderBytes 为线上记录头长度（布局见 mod_codec.h）。
inline constexpr std::uint16_t kModGlobal=0xffff;
inline constexpr std::uint8_t kModPublic=0xff;
inline constexpr std::size_t kModValueBytes=65536,kModRecordHeaderBytes=11;

// 冻结后的前序程序供两个线程共用。key 仅在主线程解释为对应 VM 的 Lua 引用。
enum class ModNodeKind { Bool, Int, Float, Number, String, Vector, Color, KColor, Entity, Player, Struct, List, Map };
struct ModNode {
  ModNodeKind kind{};
  unsigned off=0,fixed=0,worst=0,end=0,children=0,max=0,keymax=0,key=0;
  bool string_key=false;
};
// 记录键：Mod 序号（握手清单中的顺序）、声明序号（Mod 内登记顺序）、目标。
// 目标按作用域为 kModGlobal、参与者序号 0—3 或实体成员号。比较顺序即线上记录的排列顺序。
struct ModRecordKey {
  std::uint16_t mod=0,declaration=0,target=kModGlobal;
  bool operator<(const ModRecordKey &b) const noexcept {
    return std::tie(mod,declaration,target)<std::tie(b.mod,b.declaration,b.target);
  }
};
// 一条声明冻结后的布局：作用域标记、是否私有（仅 player 作用域）及前序节点数组。
struct ModLayout {
  bool player=false,entity=false,private_value=false;
  std::vector<ModNode> nodes;
};
// 握手冻结的声明表，键的 target 固定为 kModGlobal，按 (Mod, 声明) 查布局。两端按同一清单生成。
struct ModSchema {
  memory::Lease memory;
  bool declared=false;
  std::map<ModRecordKey,std::shared_ptr<const ModLayout>> layouts;
};
using ModSchemaRef=std::shared_ptr<const ModSchema>;

// 实体声明暂存以完整物理身份为键，编码线程才将其换成本帧成员号。
struct ModEntityId {
  std::uint64_t id=0,use=0;
  bool operator<(const ModEntityId &b)const noexcept{return std::tie(id,use)<std::tie(b.id,b.use);}
  bool operator==(const ModEntityId &b)const noexcept{return id==b.id&&use==b.use;}
};
// Host 暂存键：记录键加实体物理身份（非实体作用域时身份为零）。
struct ModRawKey {
  ModRecordKey record;
  ModEntityId entity;
  bool operator<(const ModRawKey &b)const noexcept{return std::tie(record,entity)<std::tie(b.record,b.entity);}
};

// 主线程只交接字节及索引。实体引用使用采集身份，待同包成员目录分配编号后再填写。
struct ModRawValue {
  struct Reference {std::size_t offset;std::uint64_t id,use;};
  struct Item {std::size_t first,last;};
  struct Map {std::size_t node;std::vector<Item> items;};
  memory::Lease memory;
  std::shared_ptr<const ModLayout> layout;
  std::uint64_t mark=0,sample=0;
  std::vector<std::byte> bytes;
  std::vector<Reference> references;
  std::vector<Map> maps;
};
struct ModRawSnapshot {
  memory::Lease memory;
  std::map<ModRawKey,std::shared_ptr<const ModRawValue>> records;
};
using ModRawRef=std::shared_ptr<const ModRawSnapshot>;

// 规范化后的不可变值字节；空字节表示顶层 nil。
struct ModValue {
  memory::Lease memory;
  std::vector<std::byte> bytes;
};
using ModValueRef=std::shared_ptr<const ModValue>;
// 影子中的一条记录：值、最后变化版本（Host 用于按 Peer 补发）、接收者，以及实体记录的物理身份。
struct ModRecord {
  ModValueRef value;
  std::uint64_t changed=0;
  std::uint8_t receiver=kModPublic;
  // Host 用纯数字见证跳过重复的后台排序；客机这两项保持零值。
  std::uint64_t sample=0;
  std::uint32_t member_epoch=0;
  ModEntityId entity;
};
// 一份完整的 Mod 影子（Host 为编码结果，客机为合并结果），封存后在线程间只读共享。
struct ModSnapshot {
  memory::Lease memory;
  std::map<ModRecordKey,ModRecord> records;
  // 仅客机后台填写，和同一份影子一起交给主线程诊断。
  std::map<std::uint16_t,std::uint64_t> rejected_by_mod;
};
using ModShadowRef=std::shared_ptr<const ModSnapshot>;

std::uint64_t ModRead(const std::byte *,std::size_t) noexcept;
void ModWrite(std::byte *,std::size_t,std::uint64_t) noexcept;
bool ModSame(const ModValueRef &,const ModValueRef &) noexcept;
// 意图和 World 状态复用声明的同一个 codec，实体号由调用者所属目录解析。
using ModMemberNumber=std::uint16_t (*)(void *,ModEntityId) noexcept;
void CanonicalModValue(const ModRawValue &,std::vector<std::byte> &,void *,ModMemberNumber,std::uint64_t &missing);
bool ValidModBytes(const ModLayout &,const std::byte *,std::size_t);
}
