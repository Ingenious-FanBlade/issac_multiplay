"""tx-04-20：生成逐签名玩家应用责任目录；只生成产品数据，不执行验收。"""
import argparse
import json
import re
from pathlib import Path
from gen_entity_application_catalog import PLAYER_RELATED, RESOURCE_CLASSES
from gen_replica_catalog import classify
from replica_projection_policy import PLAYER_PROJECTION_REVIEW, consumer_review

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "mod/observation/isaac-cs-observation"
CLASSES = PLAYER_RELATED | RESOURCE_CLASSES | {
    "Entity", "EntityPlayer", "PlayerManager", "ItemConfigItem", "ItemConfigCostume",
    "ItemConfigCard", "ItemConfigPillEffect", "EffectList", "PosVel", "BitSet128"}
# 每一族指向实际实施责任；未实施的安装明确保留 pending，不能自动当作已处理。
FAMILIES = [
    (r"History", "P10", "I17", "R06", "E12"),
    (r"Weapon", "P13", "I20", "R08", "E14/E15"),
    (r"Costume", "P12", "I19", "R07", "E13"),
    (r"Costume|Skin|Layer|Sprite|Animation|NullFrame|Image|Color|Vector|PosVel", "P11", "I18/I19", "R07", "E13"),
    (r"Pocket|Card|Pill|QueueItem", "P08", "I15", "R05", "E11"),
    (r"ActiveItem|ActiveCharge|BatteryCharge|SubCharge|PartialCharge", "P07", "I15", "R05", "E11"),
    (r"TemporaryEffect|EffectList|PlayerForm|Transformation|GetEffects", "P09", "I16", "R06", "E12"),
    (r"Collectible|Trinket|Inventory|Voided|Wisp", "P06", "I14", "R05", "E10"),
    (r"Heart|Health|Dead|CoopGhost|DamageCooldown", "P03", "I12", "R04", "E09/E21"),
    (r"Coin|Key|Bomb|BloodCharge|SoulCharge|PoopMana", "P04", "I13", "R04", "E09/E16"),
    (r"Twin|SubPlayer|Esau|Lazarus|PlayerManager|Controller|PlayerIndex", "P01", "I09/I10", "R03", "E04/E05/E06/E07/E08"),
    (r"Held|Suplex|Minecart|ItemState", "P14", "I21", "R09", "E18"),
    (r"FireDirection|AimDirection|Shooting|Damage|Speed|Range|Delay|Luck|HeadDirection", "P05", "I11", "R04", "E09/E17"),
    (r"EntityConfig|ItemConfig", "P16", "I18/I22", "R12", "E16/E22"),
    (r"Entity\.", "P02", "I11", "R04", "E09/E17"),
]


def route(member):
    """未命名的Player专用状态交I27逐字段核定，绝不静默归为纯值完成。"""
    for pattern, *responsibility in FAMILIES:
        if re.search(pattern, member["id"]):
            return responsibility
    return ["P16", "I27", "R12", "E16"]


def generate():
    rows = {}
    for filename in ("entity-catalog.json", "continuity-catalog.json"):
        catalog = json.loads((SOURCE / filename).read_text(encoding="utf-8-sig"))
        for m in catalog["members"]:
            owner = m.get("class", m.get("owner"))
            if owner not in CLASSES:
                continue
            # 相同继承成员只定义一次，重载以完整参数/返回类型区分，保留两目录的来源。
            key = (owner, m["member"], m.get("parameters"), m["returns"])
            if key in rows:
                rows[key]["source_mappings"].append({"catalog": filename, "category": m["category"], "sources": m.get("sources", [])})
                continue
            family, task, research, e2e = route(m)
            ownership, reason = classify(m, m.get("domain", "entity"))
            category = m["category"]
            authority = category in {"operation", "cast", "container_operator", "actual_call"} or ownership == "AuthorityOnly"
            if authority:
                family = "P20"
            rows[key] = {
                "id": m["id"], "owner": owner, "parameters": m.get("parameters"), "returns": m["returns"],
                "source_mappings": [{"catalog": filename, "category": category, "sources": m.get("sources", [])}],
                "source_version": m.get("schema_version", 2), "family": family,
                "parameter_range": {"GetWeapon": "0..4;explicit-Null", "GetActiveItemDesc": "0..3", "GetPocketItem": "0..3", "GetCollectibleNum": "actual-config-IDs x only-true-items{false,true} x ignore-spoof{false,true}", "GetSmeltedTrinketDesc": "actual-trinket-config-IDs;normal/golden-counts-in-descriptor", "GetActiveItemSlot": "actual-collectible-config-IDs"}.get(m["member"], "signature-specific;unscheduled-query-is-not-complete-enumeration"),
                "character_applicability": "source-API-conditions;never-assume-Isaac-heart-or-roster-rules",
                "lifetime": "current-use/shape;resource-owner-generation;revoke-on-root-or-relation-change",
                "same_window": "entity+player ordinary capture;History graph stays player-component-local",
                "sharing": "per-body;explicit-source-relation-required;equal-values-do-not-prove-sharing",
                "projection": {"ownership": ownership, "reason": reason, "review_task": "I05", "consumer_review": consumer_review(m["id"])},
                "handling": "Authority-only-operation;install-final-result-through-state-member" if authority else "named-native-consumer-pending",
                "consumer": "Authority" if authority else "ClientPlayers/" + task,
                "installation": "never-replay-operation" if authority else "I02-owned-value;native-install-contract=" + research,
                "cleanup": "no-Client-operation-owner" if authority else "I33;body/child-resource owner;borrow-generation invalidation",
                "implementation": "mod/src/replica/player_target.cpp" if not authority else "Authority-only;no-Replica-RPC",
                "implementation_status": "classified-not-native-installed", "runtime_status": "not-run;I39",
                "task": task, "research": research, "e2e": e2e,
                "absence": "Missing/Null/Empty/Unavailable/ReadFailed/NotCaptured remain distinct",
            }
    research_path = ROOT / "docs/design/tx-04-20/player-native-contracts.json"
    return {"format": "isaac.cs.player.application-catalog", "version": 1,
            "target_contract": "player-target-v2", "source_contract": "player-source-v2",
            "projection_review": PLAYER_PROJECTION_REVIEW,
            "native_apply": False, "runtime_acceptance": "I39-not-run",
            "structure_implementation": {"plan": "mod/src/replica/player_plan.cpp", "owner": "mod/src/game/client_players.cpp", "native": "mod/src/game/client_players_native.cpp", "tasks": "I05-I10", "scope": "base-entity-initialized/role-uninitialized until named I11-I29 adapters;preflight rejects absent adapters", "scene_commit": "ClientScene only;native_apply remains false"},
            "members": list(rows.values()),
            "native_members": [{"id": "NativePlayer." + ("blood_gush" if i == 10 else "held" if i == 11 else "inline_" + str(i)),
                                "source": "world_layout:player_inline_sprite_first_offset+i*player_inline_sprite_stride",
                                "physical_slot": i, "returns": "Sprite-or-Null", "family": "P11/P12",
                                "evidence": "J273 ctor VA0x74DC70;13 physical ANM constructors",
                                "read_basis": "same-use KnownCurrent/readiness>=2;ctor-constructed is not Load proof",
                                "source_lifetime": "current entity ordinary window;revoke on use/owner loss",
                                "sharing": "independent-physical-member;no-filename-alias", "same_window": "entity+player",
                                "source_implementation": "mod/src/game/world_sources/lua_entity.cpp:EntityApplicationSprites",
                                "consumer": "ClientPlayers resource owner;I18/I19/I29", "installation": "pending-named-native-resource-owner",
                                "cleanup": "I33;native-Player owns embedded-ANM;do-not-free-borrow", "research": "R07/R13/R15",
                                "e2e": "E13/E20/E23", "runtime_status": "I39-not-run"} for i in range(1, 13)],
            "structures": [
                {"id": n, "source": source, "consumer": consumer, "task": task, "status": "I05-I10:typed-target-plan-and-structural-owner;final-state-adapters-pending-I11-I29"}
                for n, source, consumer, task in [
                    ("roster_order", "player.roster", "ClientPlayers roster owner", "I08"),
                    ("body_membership", "entity+player subject/control-group membership", "ClientPlayers role-qualified borrow", "I09/I10"),
                    ("backup_slots", "PlayerManager.GetEsauJrState:0..3", "ClientPlayers explicit backup owner", "I26"),
                    ("equipment_slots", "entity.application_player:active0..3/pocket0..3/weapon0..4", "ClientPlayers equipment child owners", "I14/I15/I20"),
                    ("private_presentation", "entity.application_player.native_presentation", "Player draw;private fields require named current witness", "I18/I29"),
                    ("native_membership", "player.bodies.native_membership:v1;source-v2;room/render/update/valid+four typed storage links", "ClientPlayers role-qualified borrow and relation owner", "I07/I09/I10"),
                ]],
            "handoff_from_entity19": "docs/design/tx-04-19/entity-application.json:Player/PLAYER_RELATED;single-Isaac support retained",
            "static_research": json.loads(research_path.read_text(encoding="utf-8-sig")) if research_path.exists() else {"status": "I04-in-progress"}}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--docs", action="store_true")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    catalog = generate()
    data = json.dumps(catalog, ensure_ascii=False, separators=(",", ":"))
    (args.out / "player-application.json").write_text(data + "\n", encoding="utf-8")
    (args.out / "player_application.inc").write_text("std::string{}" + "".join('+std::string(R"ICSP(' + data[i:i+8000] + ')ICSP")' for i in range(0, len(data), 8000)), encoding="utf-8")
    if args.docs:
        (ROOT / "docs/design/tx-04-20/player-application.json").write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
