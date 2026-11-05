"""tx-04-21逐签名实时责任目录；生成产品数据，不执行测试或读取游戏。"""
import argparse
import json
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
                "tasks": "I06-I09/I23-I24", "research": "R01/R03/R04/R06", "e2e": "E02/E05/E09/E18"})
    return {"format": "isaac.cs.stream.catalog", "version": 1, "members": rows, "supplements": supplements,
        "native_members": native,
        "field_ids": ids,
        "player_common_owner": "Entity public dynamic fields belong to PlayerFast for body keys;WorldDynamic excludes those fields;relations ordinary-joint",
        "projection_review": "no new omissions;existing omitted queries keep exact documented consumers/local-asset equivalence;new consumers must restore source",
        "low_frequency_policy": "no proven complete dirty coverage => bounded full read;no cached-complete claim from notification alone",
        "capture_profiles": {
            "ordinary": "stream-source-v1;pre-JSON scopes;full declared membership;private player state stays body-joint",
            "player_post": "player-fast-source-v1;domains/common/stream.lua::Fast;actual-body-witness;main-Sprite-token;no relations/containers/parameter queries",
            "validity": "immutable acquired sample + target checked sample;bounded full reads for unclosed dirty coverage",
            "layouts": ["stream.context.v1", "stream.structure.v1", "stream.resource.v1", "stream.world.v1", "stream.player.v1"]},
        "runtime": "I01-I15 capture/transport implemented;I19 publication/I23 application and I39 acceptance pending"}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--json-only", action="store_true")
    parser.add_argument("--lua", type=Path)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    catalog = generate()
    text = json.dumps(catalog, ensure_ascii=False, separators=(",", ":"))
    (args.out / "stream-catalog.json").write_text(text, encoding="utf-8")
    if not args.json_only:
        (args.out / "stream_catalog.inc").write_text("std::string{}" + "".join(
            '+std::string(R"ICSS(' + text[i:i+8000] + ')ICSS")' for i in range(0, len(text), 8000)), encoding="utf-8")
    if args.lua:
        # 运行时来源只按显式字段目录分层；同名别名冲突保守交普通联合Context。
        policies = {}
        for row in catalog["members"]:
            scope = row["scope"]
            ident = row["id"]
            policies[ident] = scope if ident not in policies or policies[ident] == scope else "Context"
        args.lua.write_text("-- 由gen_stream_catalog.py生成的逐成员来源层；编号与目录同源。\nreturn {\n" +
            "".join("[" + json.dumps(k, ensure_ascii=False) + "]=" + json.dumps(v) + ",\n"
                    for k, v in sorted(policies.items())) + "}\n", encoding="utf-8")


if __name__ == "__main__":
    main()
