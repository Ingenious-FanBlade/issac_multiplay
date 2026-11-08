#include "replica/stream_materialize.h"
#include <algorithm>
#include <cstring>
#include <functional>
#include <set>

namespace cs::replica::stream {
namespace {
[[noreturn]] void Fail(const char *s) { throw ContractError(s); }
bool Is(const Value &v, const char *key, const char *s) {
  return v.IsObject() && v.HasMember(key) && v[key].IsString() &&
         !std::strcmp(v[key].GetString(), s);
}
const Value &At(const Value &v, const char *key) {
  if (!v.IsObject() || !v.HasMember(key))
    throw ContractError(std::string("stream_materialize_missing:")+key+
                        (v.IsObject()?":object":v.IsArray()?":array":":scalar"));
  return v[key];
}
const Value &Tree(const Record &r) {
  if (r.fields.empty())
    Fail("stream_materialize_field");
  auto p = std::get_if<std::shared_ptr<const Json>>(&r.fields[0].value);
  if (!p || !*p)
    Fail("stream_materialize_tree");
  return **p;
}
SubjectKey Key(const Value &v) {
  const auto &k = At(v, "target");
  return {Unsigned(At(k, "run")), Unsigned(At(k, "id")),
          unsigned(Unsigned(At(k, "space")))};
}
bool Sequence(const Value &v) {
  return v.IsArray() || (v.IsObject() && v.ObjectEmpty());
}
// Context可以共享同封套中的表定义；抽取主体前先闭合所需图，避免借另一身体补洞。
using GraphDefinitions = std::map<std::uint64_t, const Value *>;
GraphDefinitions GraphIndex(const Value &root) {
  GraphDefinitions out;
  const auto walk = [&](const auto &self, const Value &v) -> void {
    if (v.IsArray())
      for (const auto &x : v.GetArray())
        self(self, x);
    else if (v.IsObject()) {
      if (Is(v, "type", "table") && v.HasMember("node"))
        out.emplace(Unsigned(v["node"]), &v);
      for (auto i = v.MemberBegin(); i != v.MemberEnd(); ++i)
        self(self, i->value);
    }
  };
  walk(walk, root);
  return out;
}
Json ClosedPiece(const Value &piece, const GraphDefinitions &definitions) {
  std::set<std::uint64_t> local;
  const auto index = [&](const auto &self, const Value &v) -> void {
    if (v.IsArray())
      for (const auto &x : v.GetArray())
        self(self, x);
    else if (v.IsObject()) {
      if (Is(v, "type", "table") && v.HasMember("node"))
        local.insert(Unsigned(v["node"]));
      for (auto i = v.MemberBegin(); i != v.MemberEnd(); ++i)
        self(self, i->value);
    }
  };
  index(index, piece);
  Json out;
  auto &a = out.GetAllocator();
  std::uint64_t nodes = 0;
  const auto clone = [&](const auto &self, const Value &v,
                         unsigned depth) -> Value {
    if (depth > 64 || ++nodes > world::observation::kNodeBudget)
      Fail("stream_piece_graph_budget");
    if (Is(v, "type", "graph_ref")) {
      const auto id = Unsigned(At(v, "node"));
      if (!local.contains(id)) {
        auto d = definitions.find(id);
        if (d == definitions.end())
          Fail("stream_piece_graph_missing");
        local.insert(id);
        return self(self, *d->second, depth + 1);
      }
    }
    Value copy;
    if (v.IsArray()) {
      copy.SetArray();
      for (const auto &x : v.GetArray())
        copy.PushBack(self(self, x, depth + 1), a);
    } else if (v.IsObject()) {
      copy.SetObject();
      for (auto i = v.MemberBegin(); i != v.MemberEnd(); ++i) {
        Value key;
        key.CopyFrom(i->name, a);
        copy.AddMember(key, self(self, i->value, depth + 1), a);
      }
    } else
      copy.CopyFrom(v, a);
    return copy;
  };
  auto value = clone(clone, piece, 0);
  static_cast<Value &>(out).Swap(value);
  return out;
}
// 合并前每张闭合图各分配独立号；相同路径的table合并后重定向其别名。
struct Merger {
  Allocator &a;
  std::uint64_t next = 0;
  std::map<std::uint64_t, std::uint64_t> aliases;
  void Index(const Value &v) {
    if (v.IsArray())
      for (const auto &x : v.GetArray())
        Index(x);
    else if (v.IsObject()) {
      if ((Is(v, "type", "table") || Is(v, "type", "graph_ref")) &&
          v.HasMember("node"))
        next = (std::max)(next, Unsigned(v["node"]));
      for (auto i = v.MemberBegin(); i != v.MemberEnd(); ++i)
        Index(i->value);
    }
  }
  Value Own(const Value &v, std::map<std::uint64_t, std::uint64_t> &ids) {
    Value out;
    out.CopyFrom(v, a);
    const auto walk = [&](const auto &self, Value &x) -> void {
      if (x.IsArray())
        for (auto &y : x.GetArray())
          self(self, y);
      else if (x.IsObject()) {
        if ((Is(x, "type", "table") || Is(x, "type", "graph_ref")) &&
            x.HasMember("node")) {
          auto [it, inserted] = ids.emplace(Unsigned(x["node"]), 0);
          if (inserted) {
            if (next == UINT64_MAX)
              Fail("stream_graph_exhausted");
            it->second = ++next;
          }
          x["node"].SetUint64(it->second);
        }
        for (auto i = x.MemberBegin(); i != x.MemberEnd(); ++i)
          self(self, i->value);
      }
    };
    walk(walk, out);
    return out;
  }
  void Merge(Value &out, Value &piece, unsigned depth = 0) {
    if (depth > 64)
      Fail("stream_merge_depth");
    // 片段占位只表示本层没有这个槽的值；真实Null和显式空集合仍按原合同合并。
    if(Is(piece,"type","stream_absent_piece"))return;
    if(Is(out,"type","stream_absent_piece")){out.Swap(piece);return;}
    if (Is(out, "type", "table") && Is(piece, "type", "table")) {
      if (out.HasMember("node") && piece.HasMember("node"))
        aliases[Unsigned(piece["node"])] = Unsigned(out["node"]);
      auto &left = out["entries"], &right = piece["entries"];
      if (!Sequence(left) || !Sequence(right))
        Fail("stream_table_entries");
      if (!left.IsArray())
        left.SetArray();
      if (right.IsArray())
        for (auto &entry : right.GetArray()) {
          Value *found = nullptr;
          for (auto &old : left.GetArray())
            if (At(old, "key") == At(entry, "key")) {
              found = &old;
              break;
            }
          if (found)
            Merge((*found)["value"], entry["value"], depth + 1);
          else
            left.PushBack(entry, a);
        }
      return;
    }
    const bool terminal =
        Is(piece, "type", "reference") || Is(piece, "type", "graph_ref") ||
        piece.IsObject() && piece.HasMember("status") &&
            !piece.HasMember("fields") && !Is(piece, "status", "Value");
    if (out.IsObject() && piece.IsObject() && !terminal) {
      for (auto i = piece.MemberBegin(); i != piece.MemberEnd(); ++i) {
        if (out.HasMember(i->name))
          Merge(out[i->name], i->value, depth + 1);
        else {
          Value key;
          key.CopyFrom(i->name, a);
          out.AddMember(key, i->value, a);
        }
      }
    } else if (out.IsArray() && piece.IsArray()) {
      if (out.Size() != piece.Size())
        Fail("stream_fixed_array_layout_changed");
      for (unsigned i = 0; i < out.Size(); ++i)
        Merge(out[i], piece[i], depth + 1);
    } else
      out.Swap(piece);
  }
  void Redirect(Value &v) {
    if (v.IsArray())
      for (auto &x : v.GetArray())
        Redirect(x);
    else if (v.IsObject()) {
      if ((Is(v, "type", "table") || Is(v, "type", "graph_ref")) &&
          v.HasMember("node")) {
        auto n = Unsigned(v["node"]);
        std::set<std::uint64_t> seen;
        while (aliases.contains(n)) {
          if (!seen.insert(n).second)
            Fail("stream_graph_alias_cycle");
          n = aliases.at(n);
        }
        v["node"].SetUint64(n);
      }
      for (auto i = v.MemberBegin(); i != v.MemberEnd(); ++i)
        Redirect(i->value);
    }
  }
};
// Context中的条目带subject；其它范围已按主体分组。所有分支均来自同一依赖闭包。
void Pieces(const Store &store, const RangeData &root, const char *domain,
            const std::function<void(SubjectKey, const Value &)> &put,
            const SubjectKey *only = nullptr) {
  auto ranges = Closure(store, root);
  std::stable_sort(
      ranges.begin(), ranges.end(),
      [](const Range &a, const Range &b) { return a->id.kind < b->id.kind; });
  for (const auto &range : ranges) {
    if (range->components || range->id.kind == RangeKind::Manifest ||
        range->id.kind == RangeKind::PlayerFast)
      continue;
    const bool context = range->id.name == std::string("context/") + domain;
    for (const auto &record : range->records) {
      if (!context && only && record.member.key != *only)
        continue;
      const auto &tree = Tree(record);
      if (context) {
        if (!Sequence(tree))
          Fail("stream_context_entries");
        std::optional<GraphDefinitions> graph;
        if (tree.IsArray())
          for (const auto &row : tree.GetArray()) {
            const auto key =
                row.HasMember("subject") ? Key(row["subject"]) : SubjectKey{};
            if (only && key != *only)
              continue;
            if (!graph)
              graph = GraphIndex(tree);
            auto closed = ClosedPiece(At(row, "value"), *graph);
            put(key, closed);
          }
      } else if (tree.IsObject() && tree.HasMember(domain)) {
        const auto &items = tree[domain];
        if (!Sequence(items))
          Fail("stream_subject_entries");
        const auto graph = GraphIndex(tree);
        if (items.IsArray())
          for (const auto &value : items.GetArray()) {
            auto closed = ClosedPiece(value, graph);
            put(record.member.key.space == 7 ? SubjectKey{} : record.member.key,
                closed);
          }
      }
    }
  }
}
} // namespace
std::vector<Range> Closure(const Store &store, const RangeData &root) {
  std::vector<Range> out;
  std::set<Version> seen, active;
  const auto visit = [&](const auto &self, const Version &v) -> void {
    if (active.contains(v))
      Fail("stream_dependency_cycle");
    if (seen.contains(v))
      return;
    if (active.size() >= 64)
      Fail("stream_dependency_depth");
    const auto r = store.Find(v);
    if (!r)
      Fail("stream_missing_dependency");
    active.insert(v);
    for (const auto &d : r->dependencies)
      self(self, d);
    for (const auto &d : r->required)
      self(self, d);
    active.erase(v);
    seen.insert(v);
    out.push_back(r);
  };
  visit(visit, {root.id, root.revision});
  return out;
}
bool DependenciesReady(const Store &store, const RangeData &r) noexcept {
  try {
    Closure(store, r);
    return true;
  } catch (...) {
    return false;
  }
}
void MergePiece(Value &out, const Value &v, Allocator &a) {
  Merger m{a};
  m.Index(out);
  std::map<std::uint64_t, std::uint64_t> ids;
  auto own = m.Own(v, ids);
  m.Merge(out, own);
  m.Redirect(out);
}
void FinishPieces(Value &v, Allocator &a, std::uint64_t revision) {
  if(Is(v,"type","stream_absent_piece"))Fail("stream_incomplete_array_piece");
  // 仅在失败回溯时追加路径，不让正常逐字段重组为诊断反复创建字符串。
  const auto descend=[&](Value &child,const auto &label){
    try{FinishPieces(child,a,revision);}catch(const ContractError &e){
      if(e.code.starts_with("stream_materialize_missing:")&&e.code.size()<1024)
        throw ContractError(e.code+"/"+label());
      throw;
    }
  };
  if (v.IsArray())
    for (rapidjson::SizeType i=0;i<v.Size();++i)
      descend(v[i],[&]{return std::to_string(i);});
  else if (v.IsObject()) {
    // 装备固定成员与当前值先按同一普通目标合并，再递归完成Sprite等跨层子片段。
    if(v.HasMember("application_player_equipment_dynamic")){
      if(!v.HasMember("application_player_equipment"))Fail("stream_equipment_structure_missing");
      MergePiece(v["application_player_equipment"],v["application_player_equipment_dynamic"],a);
      v.RemoveMember("application_player_equipment_dynamic");
    }
    for (auto i = v.MemberBegin(); i != v.MemberEnd(); ++i)
      descend(i->value,[&]{return std::string(i->name.GetString(),i->name.GetStringLength());});
    if (Is(v, "type", "stream_sprite_piece") ||
        Is(v, "type", "stream_sprite_pose")) {
      Put(v, "type", "resource_pose", a);
      Value resource(rapidjson::kObjectType);
      Put(resource, "category", "anm2", a);
      Put(resource, "content_version",
          "REPENTOGON-1.1.3/J273;mod-version-required", a);
      Put(resource, "status", "Unverified", a);
      const auto &fields = At(v, "fields");
      Value path;
      path.CopyFrom(At(fields, "Sprite.GetFilename"), a);
      Put(resource, "logical_path", std::move(path), a);
      Put(v, "resource", std::move(resource), a);
    }
    if (v.HasMember("schema") && v["schema"].IsString()) {
      std::string schema = v["schema"].GetString();
      if (schema.starts_with("isaac.cs.")) {
        Put(v, "component", schema.substr(9), a);
        Put(v, "required_revision", Decimal(revision, a), a);
      }
    }
    if (v.HasMember("required_revision"))
      Put(v, "required_revision", Decimal(revision, a), a);
  }
}
Json SubjectValue(const Store &store, const RangeData &root, SubjectKey key,
                  const char *domain) {
  Json out(rapidjson::kObjectType);
  Pieces(
      store, root, domain,
      [&](SubjectKey k, const Value &v) {
        if (k == key)
          MergePiece(out, v, out.GetAllocator());
      },
      &key);
  FinishPieces(out, out.GetAllocator(), root.revision);
  return out;
}
std::map<SubjectKey, Json>
SubjectValues(const Store &store, const RangeData &root, const char *domain) {
  std::map<SubjectKey, Json> out;
  Pieces(store, root, domain, [&](SubjectKey key, const Value &v) {
    if (key == SubjectKey{})
      return;
    auto [it, inserted] = out.try_emplace(key);
    if (inserted)
      it->second.SetObject();
    MergePiece(it->second, v, it->second.GetAllocator());
  });
  for (auto &[key, v] : out)
    FinishPieces(v, v.GetAllocator(), root.revision);
  return out;
}
Json DomainValue(const Store &store, const RangeData &root,
                 const char *domain) {
  Json out(rapidjson::kObjectType);
  auto &a = out.GetAllocator();
  std::map<SubjectKey, Json> subjects;
  Pieces(store, root, domain, [&](SubjectKey key, const Value &v) {
    if (key == SubjectKey{})
      MergePiece(out, v, a);
    else {
      auto [it, added] = subjects.try_emplace(key);
      if (added)
        it->second.SetObject();
      MergePiece(it->second, v, it->second.GetAllocator());
    }
  });
  if (!subjects.empty() || !std::strcmp(domain, "entity") ||
      !std::strcmp(domain, "grid")) {
    Value objects(rapidjson::kArrayType);
    for (auto &[key, value] : subjects) {
      Merger m{a};
      m.Index(out);
      m.Index(objects);
      std::map<std::uint64_t, std::uint64_t> ids;
      auto own = m.Own(value, ids);
      objects.PushBack(own, a);
    }
    Put(out, "objects", std::move(objects), a);
  }
  FinishPieces(out, a, root.revision);
  return out;
}
Json StructuralModel(const Store &store, const RangeData &root,
                     const Value &cold, std::uint64_t connection) {
  Json out = Copy(cold);
  auto &a = out.GetAllocator();
  Put(out, "scope", ScopeValue(Connected(root.world, connection), a), a);
  Put(out, "revision", Decimal(root.revision, a), a);
  const Value *captures = nullptr;
  for (const auto &r : root.records)
    if (r.member.key.space == 7 && r.fields.size() == 2)
      captures = std::get<std::shared_ptr<const Json>>(r.fields[1].value).get();
  if (!captures)
    Fail("stream_ordinary_captures_required");
  for (auto i = out["components"].MemberBegin();
       i != out["components"].MemberEnd(); ++i) {
    const std::string domain = i->name.GetString();
    auto &c = i->value;
    Put(c, "version", Decimal(root.revision, a), a);
    Value capture;
    capture.CopyFrom(At(*captures, domain.c_str()), a);
    Put(c, "capture", std::move(capture), a);
    if (domain != "random") {
      auto value = DomainValue(store, root, domain.c_str());
      Value state;
      state.CopyFrom(value, a);
      Put(c, "state", std::move(state), a);
    }
  }
  // 此适配只在结构切换使用；各组件保留真实源scope，ordinary的共同采样时刻单独记录。
  FinishPieces(out, a, root.revision);
  ValidateModel(out);
  return out;
}
Value WorldValue(WorldScope w, Allocator &a) {
  Value v(rapidjson::kObjectType);
  const std::uint64_t ns[] = {w.session,   w.run,  w.floor,
                              w.dimension, w.room, w.scene};
  unsigned i = 0;
  for (auto key : {"session", "run", "floor", "dimension", "room", "scene"})
    Put(v, key, Decimal(ns[i++], a), a);
  return v;
}
Json DomainModel(const Store &store, const RangeData &root,
                 const Value &skeleton,
                 std::initializer_list<const char *> domains) {
  Json out(rapidjson::kObjectType);
  auto &a = out.GetAllocator();
  for (auto i = skeleton.MemberBegin(); i != skeleton.MemberEnd(); ++i)
    if (std::strcmp(i->name.GetString(), "components")) {
      Value key, value;
      key.CopyFrom(i->name, a);
      value.CopyFrom(i->value, a);
      out.AddMember(key, value, a);
    }
  Put(out, "publishable", Value(false), a);
  Put(out, "revision", Decimal(root.revision, a), a);
  Put(out, "event_watermark", Decimal(0, a), a);
  const Value *captures = nullptr;
  for (const auto &r : root.records)
    if (r.member.key.space == 7 && r.fields.size() == 2)
      captures = std::get<std::shared_ptr<const Json>>(r.fields[1].value).get();
  if (!captures)
    Fail("stream_ordinary_captures_required");
  Value components(rapidjson::kObjectType);
  for (const auto *domain : domains) {
    Value c;
    c.CopyFrom(At(At(skeleton, "components"), domain), a);
    Put(c, "version", Decimal(root.revision, a), a);
    Value capture;
    capture.CopyFrom(At(*captures, domain), a);
    Put(c, "capture", std::move(capture), a);
    auto state = DomainValue(store, root, domain);
    Value own;
    own.CopyFrom(state, a);
    Put(c, "state", std::move(own), a);
    Put(components, domain, std::move(c), a);
  }
  Put(out, "components", std::move(components), a);
  Value missing(rapidjson::kArrayType);
  for (const char *name :
       {"entity", "grid", "room", "level", "exploration", "room_save", "player",
        "run", "random", "presentation", "mod"})
    if (!out["components"].HasMember(name))
      missing.PushBack(Value(name, a), a);
  Put(out, "missing_domains", std::move(missing), a);
  FinishPieces(out, a, root.revision);
  ValidateModel(out);
  return out;
}
Value VersionValues(const std::vector<Version> &versions, Allocator &a) {
  Value out(rapidjson::kArrayType);
  for (const auto &v : versions) {
    Value x(rapidjson::kObjectType), body;
    Put(x, "kind", Decimal(unsigned(v.id.kind), a), a);
    Put(x, "name", v.id.name, a);
    if (v.id.body != SubjectKey{}) {
      body.SetObject();
      Put(body, "run", Decimal(v.id.body.run, a), a);
      Put(body, "id", Decimal(v.id.body.id, a), a);
      Put(body, "space", Decimal(v.id.body.space, a), a);
    }
    Put(x, "body", std::move(body), a);
    Put(x, "revision", Decimal(v.revision, a), a);
    out.PushBack(x, a);
  }
  return out;
}
} // namespace cs::replica::stream
