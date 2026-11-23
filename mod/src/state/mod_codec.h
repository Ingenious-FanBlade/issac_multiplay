#pragma once
#include "state/mod_state.h"
#include "net/mod_intent.h"
#include <span>

namespace cs::state {
struct StateMemberEntry;
// 只处理纯值。Host 成员号来自同一帧的目录，客机按冻结的声明校验记录。

// Host 编码线程：把主线程暂存的原始值转成规范字节。实体作用域记录的目标不在同帧成员目录时
// 整条跳过；值内的实体引用换成成员号（找不到写 0xFFFF 并计入 missing），map 按键排序。
// 与上一份逐字节相同的记录沿用旧值和"最后变化版本"，否则版本记为 version。
// raw 为空时沿用 before；raw 没有记录时返回空影子。
ModShadowRef EncodeModValues(const ModRawRef &,const ModShadowRef &,std::span<const StateMemberEntry>,
    std::uint64_t version,std::uint32_t member_epoch,std::uint64_t &missing);

// 在 World 帧 Mod 段末尾追加一条记录。记录头 11 字节，小端：
// +0 u16 Mod 序号 | +2 u16 声明序号 | +4 u16 目标（global=0xFFFF、参与者序号或实体成员号）
// | +6 u8 接收者（公开=0xFF，私有记录为参与者序号）| +7 u32 值字节数；其后紧跟值字节。
void AppendModRecord(std::vector<std::byte> &,ModRecordKey,const ModRecord &);

// 客机后台：把一帧的 Mod 段并入上一份影子，返回新的不可变影子。本帧没有 Mod 段、
// 又不是完整基线且没有记录需要删除时，直接返回原指针。
// - full（base=0 完整基线）：从空表开始，只保留本帧带来的记录；没出现的记录即被删除。
// - 增量帧：在旧影子上覆盖本帧记录。增量帧不携带删除，记录只经 gone 删除——实体记录的
//   成员号已不在目录或身份变了、参与者记录的序号已不在名册 players 位图里。
// - 逐条校验：声明存在、目标与作用域相符、私有记录发给本机 receiver；不符的丢弃本条
//   （增量帧里旧值仍在，完整基线不补回）。其余再查记录键严格递增、接收者字节与声明一致、
//   值通过 ValidModBytes；不符的在两种帧里都沿用旧值。
//   两类都计入 rejected，声明所属 Mod 存在时另计入 rejected_by_mod。
// - 记录头声明的长度超出剩余字节（截断）或尾部剩下不足一个记录头时，后续边界未知：
//   已验证的前缀照常合并；full 时未读到的旧记录沿用。
ModShadowRef MergeModShadow(const ModShadowRef &,const ModSchema &,std::span<const std::byte>,
    unsigned receiver,unsigned players,std::span<const StateMemberEntry>,bool full,std::uint64_t &rejected);

// 复制一份可写快照并按"已有记录数 + extra"预记内存账；base 为空时得到空表。
std::shared_ptr<ModSnapshot> CopyModSnapshot(const ModShadowRef &,std::size_t extra);

// 意图结果头 15 字节，小端：+0 u64 调用序号 | +8 u16 Mod 序号 | +10 u16 声明序号
// | +12 u8 ok（0 失败、1 成功）| +13 u16 结果字节数（≤256，0 表示 nil）；其后紧跟结果字节。
inline constexpr std::size_t kModResultHeaderBytes=15;
// 在 World 帧私有结果区末尾追加一条结果，只发给发起该意图的 Peer。
void AppendModResult(std::vector<std::byte> &,const net::ModIntentResult &);
// 解析私有结果区。要求调用序号严格递增、ok 只取 0/1、长度不越界且条数不超过在途上限；
// 任一条不合格即整区作废（返回 false），已写入 out 的条目不可使用。
bool ReadModResults(std::span<const std::byte>,net::ModIntentResults &) noexcept;
}
