"""tx-04-21逐签名实时责任目录；生成产品数据，不执行测试或读取游戏。"""
import argparse
import json
from cpp_catalog import string_expression
import re
from pathlib import Path
from gen_replica_catalog import classify
from replica_projection_policy import consumer_review

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "mod/observation/isaac-cs-observation"
APPLICATIONS = ["tx-04-18/room-grid-application.json", "tx-04-19/entity-application.json",
                "tx-04-20/player-application.json"]
PLAYER_OWNERS = {"EntityPlayer", "ActiveItemDesc", "PocketItem", "QueueItemData", "Weapon",
                 "CostumeSpriteDesc", "TemporaryEffect", "TemporaryEffects", "History", "HistoryItem"}
POSE_OWNERS = {"Sprite", "LayerState", "BlendMode", "Color", "KColor", "Vector", "PosVel"}
RESOURCE_OWNERS = {"AnimationData", "AnimationLayer", "AnimationFrame", "Image", "NullFrame"}
# 构造、引用和资源定义必须普通联合提交；其余具名最终量保守地每个合格窗口刷新。
STRUCTURE = re.compile(r"(?:^|\.)(?:Type|Variant|SubType|InitSeed|GetPlayerType|GetPlayerIndex|GetMainTwin|GetOtherTwin|GetSubPlayer|IsSubPlayer|GetFlippedForm|IsHologram|GetHeldEntity|Parent|Child|SpawnerEntity|Target|GetWeapon|GetEffects|GetHistory|GetCostumeSpriteDescs|GetCostumeLayerMap|GetCollectiblesList|GetVoidedCollectiblesList|GetWispCollectiblesList|GetSpoofedCollectiblesList|GetSmeltedTrinkets|GetSmeltedTrinketDesc|QueuedItem|GetPocketItem|GetActiveItemDesc)$")
RESOURCE = re.compile(r"(?:Filename|SpritesheetPath|GetSpritesheetPath|GetAllAnimationData|GetLayerID|GetName|GetTexture|Shader|Path)$")


def capture_freshness():
    """本轮不宣称writer闭包：字段仍同窗重读，范围复用只发生在后台精确比较之后。"""
    return {"mode": "FreshRead", "stable_reuse": False,
            "source": "qualified-main-thread-window;sealed-owned-values;no-Lua-or-native-borrow-in-worker",
            "changes": "all declared getters reread;resource/save/config writer closure not established",
            "failure": "incomplete/window-change/loss/budget => no new complete publication",
            "version_reuse": "exact full-range equality;original acquired sample plus current checked sample"}


def protocol_names(catalog):
    """构建时展开固定名称表；握手线程不解析数MiB目录，也不动态建立名称字典。"""
    fixed = json.loads((ROOT / "mod/src/replica/stream_fixed_names.json").read_text(encoding="utf-8"))
    names, codes = {}, {}

    def add(code, name):
        if name in names:
            return
        if not code or code in codes:
            raise ValueError("compact_name_definition")
        names[name], codes[code] = code, name

    for index, name in enumerate(fixed, 1):
        add(index * 4, name)
    members = {}
    for row in catalog["members"]:
        members[row["id"]] = min(members.get(row["id"], row["field"]), row["field"])
    # 保留退休签名的旧编号，新增/修改签名不能重排已有名称。
    for wire, field in catalog["field_ids"].items():
        parts = wire.split("|", 2)
        if len(parts) == 3 and "." in parts[1]:
            ident = parts[1]
            members[ident] = min(members.get(ident, field), field)
    owners = {}
    for name, field in members.items():
        owner = name.split(".", 1)[0]
        owners[owner] = min(owners.get(owner, field), field)
    for name, field in sorted(members.items()):
        add(field * 4 + 1, name)
    for name, field in sorted(owners.items()):
        add(field * 4 + 2, name)
    return [(code, name) for name, code in names.items()]


def disposition(m, domain, ownership):
    """归属由实际语义/owner决定；共享姿态按身体与非身体拆成互斥绑定。"""
    owner = m.get("owner", m.get("class", ""))
    ident = m["id"]
    if ownership in {"AuthorityOnly", "ClientLocal"}:
        return "AuthorityOnly", "Authority/local-input", "never-replay-operation"
    if ownership == "PresentationEvent" or domain in {"presentation", "mod"}:
        return "Presentation", "tx-04-27", "independent-scope;not-world-deletion"
    if ownership == "NotReplicated":
        return "Context", "CapturedReadOnly/local-versioned-asset", "projection-omitted;see-consumer-review"
    if STRUCTURE.search(ident) or owner in {"PlayerManager", "EntityList", "VectorList", "EffectList"}:
        return "Structure", "ordinary-joint", "revoke-on-use/shape/membership/relation/equipment-change"
    if owner.startswith(("ItemConfig", "EntityConfig", "RoomConfig")) or owner in RESOURCE_OWNERS or RESOURCE.search(ident):
        return "Resource", "versioned-local-asset-and-owner", "revoke-on-reload/config/mod/context/owner-change"
    if owner in POSE_OWNERS or owner == "Entity":
        return "SubjectDynamic", "body:PlayerFast;other:WorldDynamic", "exact-pose;resource-version-and-instance-required"
    if owner in PLAYER_OWNERS:
        # 装备/容器的成员仍是完整动态最终量，容器资格和关系属于Structure。
        return "PlayerFast", "body-final-value", "ordinary-joint-until-R03-safe-body-schema;never-replay-gameplay"
    if domain in {"entity", "grid", "room"} or owner.startswith(("Entity", "GridEntity")) or owner in {"Camera", "FXParams", "FXLayers", "ColorModifier"}:
        return "WorldDynamic", "ordinary-final-value", "every-qualified-ordinary;no-assumed-dirty-coverage"
    return "Context", "retained-context", "bounded-full-refresh;invalidate-on-loss/root/mutation/Lua/owner-change"


def generate():
    """保留每份原目录行及重载；应用器的逐成员责任作为可追溯链接，不覆盖原始来源。"""
    # 沿用已提交的显式编号，不使用摘要，也不让插入新Wiki成员重排既有wire字段号。
    id_source = ROOT / "docs/design/tx-04-21/stream-catalog.json"
    previous = json.loads(id_source.read_text(encoding="utf-8")) if id_source.exists() else {}
    ids = dict(previous.get("field_ids", {}))
    ids.update({"|".join((r["source_catalog"], r["id"], str(r.get("parameters")), r.get("type", ""))): r["field"]
                for r in previous.get("members", [])})
    ids.update({r["wire_name"]: r["field"] for r in previous.get("native_members", []) if "field" in r})
    next_id = max(ids.values(), default=0)

    def field(name):
        """新增成员只追加序号；退休编号不重新分配。"""
        nonlocal next_id
        if name not in ids:
            next_id += 1
            ids[name] = next_id
        return ids[name]

    apps = {}
    supplements = []
    for name in APPLICATIONS:
        data = json.loads((ROOT / "docs/design" / name).read_text(encoding="utf-8-sig"))
        for row in data["members"]:
            apps.setdefault(row["id"], []).append({"catalog": "docs/design/" + name,
                "parameters": row.get("parameters"), "returns": row.get("returns"),
                "consumer": row.get("consumer") or row.get("local_storage") or row.get("installation", ""),
                "implementation": row.get("implementation", ""),
                "source_path": row.get("source_path", "component-local/public-member"),
                "status": row.get("implementation_status", row.get("status", row.get("native_status", "captured-only")))})
        for key in ("structural", "structures", "native_members", "common_native", "class_native", "associated"):
            if data.get(key):
                supplements.append({"catalog": "docs/design/" + name, "section": key, "value": data[key],
                                    "scope": "ordinary-joint;native-final-dynamic+explicit-resource-version",
                                    "tasks": "I06-I09/I23-I24;never-omit-private-supplement"})
    rows = []
    for path in sorted(SOURCE.glob("*catalog.json")):
        data = json.loads(path.read_text(encoding="utf-8-sig"))
        for ordinal, m in enumerate(data["members"]):
            domain = m.get("domain", path.name.replace("-catalog.json", ""))
            ownership, reason = classify(m, domain)
            scope, owner, invalidation = disposition(m, domain, ownership)
            links = [a for a in apps.get(m["id"], []) if a["parameters"] == m.get("parameters") and a["returns"] == m.get("returns")]
            wire_name = "|".join((path.name, m["id"], str(m.get("parameters")), m.get("returns", "")))
            rows.append({"field": field(wire_name), "wire_name": wire_name, "id": m["id"], "parameters": m.get("parameters"),
                "parameter_set": "original-signature+captured-parameters;NotScheduled-is-not-empty",
                "type": m.get("returns", ""), "precision": "lossless-source-width/sign/float;no-quantization",
                "units": m.get("units", "API原单位"), "source_catalog": path.name, "source_ordinal": ordinal,
                "source_schema": m.get("schema", ""), "source_version": m.get("schema_version", m.get("version", 2)),
                "sources": m.get("sources", []), "source_phase": m.get("phase", "explicit-source-phase"),
                "read_basis": "current-qualified-use;normal-owner-return;stable-ordinary-commit;PlayerPost-needs-I08-witness",
                "ownership": ownership, "ownership_reason": reason, "scope": scope, "canonical_owner": owner,
                "baseline": scope not in {"AuthorityOnly", "Presentation"} and ownership != "NotReplicated",
                "completeness": "Value/Null/Empty/NotApplicable/Missing/NotScheduled/ReadFailed/Unsupported/Truncated-distinct",
                "default_legal": False, "invalidation": invalidation,
                "capture_freshness": capture_freshness(),
                "joint_commit": "membership/shape/equipment/references/held/twin/weapon-owner;ordinary-only",
                "applications": links, "readonly_consumer": "ClientPlayerDisplayView::Captured" if domain == "player" else "retained-model-context",
                "projection": consumer_review(m["id"]), "tasks": "I06-I09/I11/I23-I24", "research": "R01/R03/R06",
                "e2e": "E02/E05/E09/E18", "runtime": "not-tested-tx-04-21"})
    # 私有补源和schema结构也是实际消费者输入，逐条登记，不能只覆盖Wiki签名。
    native = []
    for section in supplements:
        value = section["value"]
        entries = value.items() if isinstance(value, dict) else enumerate(value) if isinstance(value, list) else [(section["section"], value)]
        for key, entry in entries:
            ident = entry.get("id", str(key)) if isinstance(entry, dict) else str(key)
            wire_name = "|".join((section["catalog"], section["section"], ident))
            native.append({"field": field(wire_name), "wire_name": wire_name, "id": ident, "source_catalog": section["catalog"], "section": section["section"],
                "contract": entry, "scope": "Structure" if any(x in ident for x in ("membership", "physical_generation", "links", "relation", "backup")) else "SubjectDynamic",
                "binding": "body:PlayerFast-only-after-I08/R03;other:WorldDynamic;resource-definition:Resource",
                "joint_commit": "all cross-object and owner/layout changes ordinary-joint",
                "invalidation": "root/use/shape/borrow/resource/Lua/loss/mutation;no-unproven-cache",
                "capture_freshness": capture_freshness(),
                "tasks": "I06-I09/I23-I24", "research": "R01/R03/R04/R06", "e2e": "E02/E05/E09/E18"})
    return {"format": "isaac.cs.stream.catalog", "version": 1, "members": rows, "supplements": supplements,
        "native_members": native,
        "field_ids": ids,
        "player_common_owner": "ordinary body-joint preserves complete final values;newer witnessed PlayerFast overlays only its declared safe public range;shared getters never become per-body backing writes;WorldDynamic records exclude Player bodies",
        "projection_review": "no new omissions;existing omitted queries keep exact documented consumers/local-asset equivalence;new consumers must restore source",
        "low_frequency_policy": "no proven complete dirty coverage => bounded full read;no cached-complete claim from notification alone",
        "capture_profiles": {
            "ordinary": "main-thread FreshRead -> immutable owned values/64-item blocks -> single worker stream-source-v1 partition;no formal Lua JSON;complete ordinary body-joint;world.v2 carries all eleven real capture scopes;no cold-scope retagging",
            "player_post": "player-fast-source-v1;actual-body-witness;main+13-inline+ordered-costume-render-poses;exact ordinary resource/config/map dependency;29/38 cross-body standalone gap;no weapons/inventory promotion",
            "validity": "immutable acquired sample + target checked sample;bounded full reads for unclosed dirty coverage",
            "handoff": "one building+one latest ordinary;independent pinned cold;8 latest body slots;256 ordered controls;generation+watermark revoke;exact required_capture never rebound",
            "retirement": "bounded 16384 live-or-retired owners;final value destruction on reclaimer;all retained values and metadata charged to existing pools",
            "freshness": capture_freshness(),
            "layouts": ["stream.context.v1", "stream.structure.v1", "stream.resource.v1", "stream.world.v2", "stream.player.v1"]},
        "installation": {
            "prepare": "net/live.cpp::Client::Prepare;old-route ScenePrepared echo;main-thread BeginStream;input/history revoked",
            "cold": "ClientScene::AdmitStream/PumpStream -> ColdModel -> AdmitTarget/Pump;network values only",
            "ordinary": "SubjectValues + four-domain Room/Grid/Level/Run DTO;existing native owners;complete step then range commit",
            "body": "ClientPlayers::CompileFast;ClientNativePlayersFast;complete same-body render group;exact Resource/config/map + actual binding + same Scene;29/38 cross-body standalone gap;query facts retained, shared backing ordinary only",
            "resources": "same ANM/image pair across frame/animation;preflight selectors;false reset only for equal layer/null counts",
            "structure": "StructuralModel explicit closure;complete preflight then safe Scene rebuild;no gameplay RPC",
            "ack": "native-range-commit-v1;per-result attempt/instance/resource/domain/watermark;dynamic 20Hz;baseline/structure immediate",
            "acceptance": "I39 P2 full E01-E26 then P4 full suite;not run"},
        "runtime": "I01-I25 implemented;I26-I40 and I39 unified runtime acceptance pending;no measured Hz/bandwidth/native network loop claim"}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--json-only", action="store_true")
    parser.add_argument("--lua", type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    catalog = generate()
    # 与C++和CMake读取同一版本表，不把生成器的副本当作协议定义。
    version_text = (ROOT / "mod/src/replica/stream_version.h").read_text(encoding="utf-8")
    framing, schema, compact = map(int, re.search(r"kFraming=(\d+),kSchema=(\d+),kCompactSchema=(\d+)", version_text).groups())
    catalog["protocol"] = {"framing": framing, "stream_schema": schema, "compact_schema": compact}
    text = json.dumps(catalog, ensure_ascii=False, separators=(",", ":"))
    (args.out / "stream-catalog.json").write_text(text, encoding="utf-8")
    if not args.json_only:
        (args.out / "stream_catalog.inc").write_text(string_expression(text, "ICSS"), encoding="utf-8")
        names = protocol_names(catalog)
        for order, key in (("text", lambda row: row[1]), ("code", lambda row: row[0])):
            (args.out / f"stream_names_by_{order}.inc").write_text("".join(
                "{" + str(code) + "ULL," + json.dumps(name, ensure_ascii=False) + "},\n"
                for code, name in sorted(names, key=key)), encoding="utf-8")
    # C++与Lua共用同一份已消歧策略；后台分层不为查scope解析整份研究/说明目录。
    policies = {}
    for row in catalog["members"]:
        scope = row["scope"]
        ident = row["id"]
        policies[ident] = scope if ident not in policies or policies[ident] == scope else "Context"
    if not args.json_only:
        layers = {"Structure": "Structure", "Resource": "Resource", "WorldDynamic": "WorldLayer",
                  "PlayerFast": "WorldLayer", "SubjectDynamic": "WorldLayer"}
        (args.out / "stream_field_layers.inc").write_text("".join(
            "{" + json.dumps(ident, ensure_ascii=False) + "," + layers.get(scope, "Context") + "},\n"
            for ident, scope in sorted(policies.items())), encoding="utf-8")
    if args.lua:
        args.lua.write_text("-- 由gen_stream_catalog.py生成的逐成员来源层；编号与目录同源。\nreturn {\n" +
            "".join("[" + json.dumps(k, ensure_ascii=False) + "]=" + json.dumps(v) + ",\n"
                    for k, v in sorted(policies.items())) + "}\n", encoding="utf-8")


if __name__ == "__main__":
    main()
