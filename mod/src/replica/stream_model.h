#pragma once
#include "replica/player_target.h"
#include <variant>
#include <optional>

namespace cs::replica::stream {
inline constexpr unsigned kSchema=1;
enum class RangeKind : unsigned { Manifest=1, Context, Structure, Resource, WorldDynamic, PlayerFast };
enum class FieldStatus : unsigned { Value, Null, Empty, NotApplicable, Missing, NotScheduled, ReadFailed, Unsupported, Truncated };
enum class CaptureKind : unsigned { Ordinary, OddPlayer };
// 世界身份不携带连接；同一个不可变内容由三个Peer共享，路由只属于传输封套。
struct WorldScope final {
  std::uint64_t session=0,run=0,floor=0,dimension=0,room=0,scene=0;
  auto operator<=>(const WorldScope &) const = default;
};
struct RangeId final {
  RangeKind kind=RangeKind::Context;
  std::string name;
  SubjectKey body{};
  auto operator<=>(const RangeId &) const = default;
};
struct Version final { RangeId id; std::uint64_t revision=0; auto operator<=>(const Version &) const = default; };
// 普通来源与逐身体来源独立计序；相同采样不能通过重编码变成新频率。
struct Sample final {
  std::uint64_t epoch=0,sequence=0,tick=0,outer=0,owner=0,mutation=0,loss=0;
  std::uint64_t last_native_update_tick=0,required_world_sequence=0,binding_revision=0;
  CaptureKind kind=CaptureKind::Ordinary;
};
struct Member final {
  SubjectKey key{};
  std::uint64_t use=0,shape=0;
  std::string category;
  auto operator<=>(const Member &) const = default;
};
// 热字段已经解析为自有标量；复杂完整值只持有const树，不重新解析JSON或借用游戏内存。
using Atom=std::variant<std::monostate,bool,std::uint64_t,std::int64_t,double,std::string,std::shared_ptr<const Json>>;
struct Field final {
  std::uint32_t id=0;
  FieldStatus status=FieldStatus::Missing;
  Atom value;
  bool operator==(const Field &) const;
};
struct Record final {
  Member member;
  std::string layout;
  std::vector<Field> fields;
  bool operator==(const Record &) const = default;
};
// 布局由本地编码/应用合同注册，报文不能自行删去必需字段；不同类别、形态可使用不同完整布局。
struct Layout final {
  RangeKind kind=RangeKind::WorldDynamic;
  std::vector<std::uint32_t> fields;
  std::vector<std::uint32_t> nullable,empty,not_applicable;
  // Atom的具名类型索引1..6；status不能把标量字段伪造成合法空集合。
  std::map<std::uint32_t,unsigned> types;
};
using Layouts=std::map<std::string,Layout>;
struct RangeData final {
  WorldScope world;
  RangeId id;
  std::uint64_t revision=0;
  Sample sample;
  bool complete=false,membership_complete=false;
  std::vector<Version> dependencies;
  std::vector<Member> members;
  std::vector<Record> records;
  // Manifest声明全部冷依赖/域/历史水位；components仅用于冷构建的18—20适配，不是热路径。
  std::vector<Version> required;
  std::uint64_t native_domains=0,retained_domains=0,event_watermark=0;
  std::shared_ptr<const Json> components;
};
using Range=std::shared_ptr<const RangeData>;
enum class Admission { Complete, WaitingDependencies, Duplicate, Stale, Conflict, MissingField, IncompleteMembership, Invalid, OverBudget };
struct Decision final { Admission status=Admission::Invalid; const char *reason="stream_invalid"; explicit operator bool() const noexcept {return status==Admission::Complete;} };
const Json &Catalog();
// Freeze不接收调用者仍可修改的shared_ptr，深复制复杂值后才公开const所有权。
Range Freeze(RangeData,const Layouts &,std::uint64_t byte_limit=256ull<<20);
Decision Validate(const RangeData &,const Layouts &,std::uint64_t byte_limit=256ull<<20) noexcept;
std::uint64_t Footprint(const RangeData &);
bool Equal(const RangeData &,const RangeData &);
WorldScope World(const Scope &) noexcept;
Scope Connected(const WorldScope &,std::uint64_t connection);
// 范围仓库仅接纳纯值，不产生原生ACK；一个逻辑视图按显式版本组合，永不隐式继承旧动态字段。
class Store final {
public:
  explicit Store(WorldScope scope,std::uint64_t budget=256ull<<20):world_(scope),budget_(budget){}
  Decision Admit(Range,const Layouts &) noexcept;
  Range Find(const Version &) const noexcept;
  std::vector<Range> Compose(const RangeData &manifest) const;
  // 安装器真实成功后单独推进；普通提交不覆盖较新的身体子帧，结构换代撤销旧子帧。
  Decision Commit(const Version &) noexcept;
  const std::map<RangeId,Range> &Applied() const noexcept {return applied_;}
  void Reset(WorldScope) noexcept;
  // 根、已安装值与外部共享值共同保留依赖闭包；传输退尽后才回收旧版本。
  void Collect(const std::vector<Version> &roots);
private:
  WorldScope world_;
  std::uint64_t budget_;
  std::map<Version,Range> values_;
  std::map<RangeId,Range> applied_;
};
// 只在冷构建中生成一次连接视图；共享源内容本身从不嵌入某Peer的connection。
Json ColdModel(const Store &,const RangeData &manifest,std::uint64_t connection);
} // namespace cs::replica::stream
