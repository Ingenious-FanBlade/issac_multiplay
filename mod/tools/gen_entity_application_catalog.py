"""tx-04-19 全成员应用责任目录；资料生成不执行游戏或测试。"""
import argparse
import json
import re
from cpp_catalog import string_expression
from pathlib import Path
from gen_replica_catalog import classify
from replica_projection_policy import consumer_review, ENTITY_COMPACT_CONSUMERS

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "mod/observation/isaac-cs-observation/entity-catalog.json"
PARSER = "mod/src/replica/entity_target.cpp:ParseEntityTarget"
RESEARCH = ROOT / "docs/design/tx-04-19/entity-native-contracts.json"
COMMON_FIELDS = ROOT / "mod/src/game/client_entity_fields.inc"
CLASS_FIELDS = ROOT / "mod/src/game/client_entity_class_fields.inc"
# 分类来自公开转换/专用RTTI见证；数值Type只用于交叉检查，绝不按范围猜NPC。
CLASSES = {
    "EntityPlayer": ("Player", "tx-04-20", "external-player-owner", "E13"),
    "EntityNPC": ("NPC", "I11", "C01", "E02"),
    "EntityDelirium": ("Delirium", "I11", "C02:special-of-NPC", "E03"),
    "EntityTear": ("Tear", "I12", "C03", "E04/E18"),
    "EntityProjectile": ("Projectile", "I13", "C04", "E04"),
    "EntityBomb": ("Bomb", "I14", "C05", "E05"),
    "EntityLaser": ("Laser", "I15", "C06", "E06"),
    "EntityKnife": ("Knife", "I16", "C07", "E07"),
    "EntityPickup": ("Pickup", "I17", "C08", "E08"),
    "EntityFamiliar": ("Familiar", "I18", "C09", "E09"),
    "EntityEffect": ("Effect", "I19/I24", "C10;variant136=GridDesc", "E10/E14"),
    "EntitySlot": ("Slot", "I20", "C11:variants1..18", "E11"),
    "NativeText": ("NativeText", "I21", "C12", "E13"),
    "NativeDummy": ("NativeDummy", "I22", "C13", "E13"),
}
RESOURCE_CLASSES = {"Sprite", "LayerState", "AnimationData", "AnimationLayer", "AnimationFrame", "NullFrame", "Image", "BlendMode", "Shape", "Color", "KColor", "Vector"}
PLAYER_RELATED = {"Weapon", "TemporaryEffects", "TemporaryEffect", "CostumeSpriteDesc", "PocketItem", "QueueItemData", "ActiveItemDesc", "EntityConfigPlayer", "EntityConfigBaby", "History", "HistoryItem"}
RELATIONS = {"Entity.Parent", "Entity.Child", "Entity.Target", "Entity.SpawnerEntity", "Entity.GetLastChild", "Entity.GetLastParent", "Entity.GetMinecart", "EntityNPC.ParentNPC", "EntityNPC.ChildNPC", "EntityNPC.EntityRef", "EntityNPC.GetPlayerTarget", "EntityNPC.GetSirenPlayerEntity", "EntityTear.StickTarget", "EntityLaser.BounceLaser"}
# 类别实现与逐字段运行覆盖分别声明；验收文档保存具体场景和未覆盖分支。
ACCEPTANCE_DOC = "docs/design/tx-04-19/tx-04-19-acceptance.md"
NATIVE_CLASSES = set(CLASSES) - {"EntityPlayer"}
RUNTIME_ACCEPTANCE = "I34-passed:65-Source/65-cold-Client;20-invalid/2-compat/16-fault;named-scenes;see-acceptance-document"


def route(m):
    """公共继承只定义一次；类别字段与关联值分别声明安装/纯值/外部责任。"""
    owner, member = m["class"], m["member"]
    if m["id"] == "EntityPlayer.GetHeldEntity":
        return "I24/I25", "typed_reference", "仅Effect136所需的同T稳定Key/shape与Render栈持物借用；不执行TryHold/Throw，完整Player持物语义仍归20", "E14/E15/E22"
    if m["id"] == "EntityPlayer.GetBodySprite":
        return "I24/I25", "resource_pose", "普通Isaac窄支撑安装实际BodySprite及null-frame，供136原生持物draw；完整Player形态/装备仍归20", "E14/E18"
    if m["id"] in {"EntityPlayer.GetItemState", "EntityPlayer.GetSuplexState"}:
        return "I24/I25", "presentation_dependency", "仅136持物Render栈按来源临时安装并还原；公开状态不重放UseItem/Suplex/Throw，完整Player应用仍归20", "E14/E22"
    if m["id"] == "EntityEffect.GetGridEntityDesc":
        return "I24", "external_domain", "仅Effect136；同窗口Grid.lifted_descriptors提供完整可空描述，不是活动Grid身份或通用VarData", "E14"
    if owner == "EntityPlayer" or owner in PLAYER_RELATED:
        return "tx-04-20", "external_domain", "Player/装备/跨身体关系由具名Player owner负责；19保留值而不应用", "E13"
    if m["id"] in RELATIONS or (m["returns"].startswith("Entity") and "Config" not in m["returns"] and owner not in {"EntityConfigEntity", "EntityDesc"}):
        return "I09", "typed_reference", "稳定Key与scope；构造前置/后置关系由类别adapter决定，禁止首玩家回退", "E13/E15"
    if owner in RESOURCE_CLASSES or m["returns"] == "Sprite":
        return "I10/I25", "resource_pose", "同窗口真实成员资源/精确姿态；不Load/Update取值、不按路径合并实例", "E16"
    if owner == "Entity":
        if member in {"Index", "GetHitListIndex", "InitSeed", "DropSeed", "SpawnGridIndex", "SpawnerType", "SpawnerVariant"}:
            return "I08", "authority_fact", "保留Authority身份/来源事实；Index、命中编号与seed不写入本地身份", "E01/E15"
        return "I08", "common_state", "公共最终状态；写回与真实后态由I08逐字段实现，不能重放操作", "E01/E15"
    if owner in CLASSES:
        _, task, _, e2e = CLASSES[owner]
        return task, "class_state", "类别最终值/容器；由具名adapter安装、保留Authority事实或交接外部owner，具体分支见应用处置及验收文档", e2e
    return "I08/I09", "associated_value", "配置/PathFinder/命中集合等关联纯值；禁止寻路、抽奖或复制原生Index为本地引用", "E01/E15"


def generate():
    catalog = json.loads(SOURCE.read_text(encoding="utf-8-sig"))
    # 产品字段表同时供模型解析和SDK写/读使用；目录只读取声明，不执行原生程序。
    common_native = {}
    for line in COMMON_FIELDS.read_text(encoding="utf-8-sig").splitlines():
        match = re.match(r'CS_ENTITY_(FIELD|RAW_FIELD|SPECIAL_FIELD)\("([^"]+)",(.+)\)', line)
        if match:
            kind, member, storage = match.groups()
            common_native[member] = {"declaration": kind, "storage": storage, "scope": "non-Player;Player20-owner",
                                     "write_and_check": "mod/src/game/client_entity_common_native.cpp:ClientEntityCommonFields"}
    # 派生表与公共表同样来自产品声明，目录不执行任何原生检查。
    class_native = {}
    for line in CLASS_FIELDS.read_text(encoding="utf-8-sig").splitlines():
        match = re.match(r'CS_CLASS_FIELD\((\d+),"([^"]+)",([^,]+),([^,]+),([^\)]+)\)', line)
        if match:
            category, member, offset, native_type, kind = match.groups()
            class_native[member] = {"category": int(category), "offset": offset, "native_type": native_type, "kind": kind,
                                    "write_and_check": "mod/src/game/client_entity_adapters_native.cpp:Fields"}
    rows = []
    for ordinal, m in enumerate(catalog["members"]):
        task, responsibility, reason, e2e = route(m)
        ownership, projection_reason = classify(m, "entity")
        if m["category"] == "cast":
            responsibility, reason = "classification_witness", "来源分类见证；同一主体不创建第二份身份或执行远程cast"
        elif ownership == "NotReplicated":
            responsibility, reason = "observation_only", projection_reason
        elif ownership == "AuthorityOnly":
            responsibility, reason = "authority_only", projection_reason
        if responsibility == "classification_witness":
            target_field, storage = "EntityObjectTarget.category/source_class/delirium", "one shared Key;no second instance"
        elif responsibility in {"authority_only", "observation_only"}:
            target_field, storage = "none;operation-not-a-state-setter;actual-call-has-independent-scope", "Authority/complete-observation only;not native-applied"
        else:
            section = "common" if m["class"] == "Entity" else "special" if m["class"] == "EntityDelirium" else "payload"
            target_field, storage = "EntityTarget.objects/" + section + "/fields/" + m["id"], "owned-pure-value;named-disposition-" + task
        rows.append({
            "source_catalog": SOURCE.name, "source_ordinal": ordinal, "id": m["id"],
            "owner": m["class"], "member": m["member"], "signature": m["signature"],
            "source_schema": m["schema"], "source_version": m["schema_version"],
            "sources": m["sources"], "returns": m["returns"], "parameters": m["parameters"],
            "category": m["category"], "applicability": "all-class-common;Player-native-owner=tx-04-20" if m["class"] == "Entity" else CLASSES.get(m["class"], ("associated-owner-qualified",))[0],
            "ownership": ownership, "projection_reason": projection_reason,
            "consumer_review": consumer_review(m["id"]), "responsibility": responsibility,
            "reason": reason, "application_task": task,
            "target_field": target_field, "local_storage": storage,
            "source_path": "components/entity/state/objects/**/class|special" if responsibility == "classification_witness" else "components/entity/state/objects/**/fields/" + m["id"],
            "dependencies": ["same ordinary component capture", "entity-source-v1", "stable-keys-v1", "I05 Player/instance owner;I09 references;I10 resources"],
            "missing": "Missing/Null/Empty/NotApplicable/NotScheduled/Unknown/Unavailable/ReadFailed/Truncated分别保留；只有完整成员表允许缺席删除",
            "implementation": PARSER, "source_implementation": m["implementation"],
            "native_status": "authority_not_replayed" if ownership == "AuthorityOnly" else "external_Player20" if task == "tx-04-20" else responsibility + ";named-consumer-disposition",
            "e2e": e2e,
        })
        if m["id"] in common_native and ownership == "ReplicatedState":
            rows[-1]["native_status"] = "I08-common-path-implemented;non-Player-only;Player20"
            rows[-1]["local_storage"] = common_native[m["id"]]["storage"]
            rows[-1]["implementation"] = "mod/src/game/client_entity_model.cpp:ClientDescribeEntity;mod/src/game/client_entity_common_native.cpp:ClientEntityCommonFields"
        elif m["id"] in class_native and ownership == "ReplicatedState":
            rows[-1]["native_status"] = "I11-I25-final-field-path-implemented;I29-Scene-only-commit"
            rows[-1]["local_storage"] = class_native[m["id"]]
            rows[-1]["implementation"] = "mod/src/game/client_entity_class_model.cpp:ClientDescribeEntityClass;mod/src/game/client_entity_adapters_native.cpp:Fields"
        elif m["class"] in {"EntityNPC", "EntityDelirium", "EntityTear", "EntityProjectile", "EntityBomb", "EntityLaser", "EntityKnife", "EntityPickup", "EntityFamiliar", "EntityEffect", "EntitySlot", "NativeText", "NativeDummy"} and ownership == "ReplicatedState":
            rows[-1]["native_status"] = "I10-I25-explicit-field-disposition;I29-Scene-only-commit"
            rows[-1]["local_storage"] = "ClientEntityClassDisposition:resource/native-reference/Authority-fact/complete-typed-collection/optional-bool/removed-API;NPC-three-deques=stable-keys/value-streams;Siren-owned-gameplay-actor=Authority-fact;Effect136=checked-Grid18-handoff"
            rows[-1]["implementation"] = "mod/src/game/client_entity_class_model.cpp:ClientEntityClassDisposition;mod/src/game/client_entity_adapters_native.cpp"
        elif responsibility == "typed_reference":
            rows[-1]["native_status"] = "I09-typed-plan-and-common-EntityPtr-implemented;named-owner-required"
            rows[-1]["implementation"] = "mod/src/replica/entity_plan.cpp:DescribeEntityDependency;mod/src/game/client_objects_native.cpp:FormalReferences"
    # 结构型补充与公开行使用同一责任目录，不能靠测试夹具补表现源。
    return {"format": "isaac.cs.entity.application-catalog", "version": 1, "model_version": 2,
            "acceptance_scope": {"document": ACCEPTANCE_DOC, "scope": "category-scene;not-every-member-value-or-variant", "full_entity_domain_commit": False, "network_ack": False},
            "target_contract": "entity-target-v1", "source_contract": "entity-source-v1", "members": rows,
            "common_native": common_native, "class_native": class_native,
            "projection_consumer_review": {"task": "I31", "compact_sources": ENTITY_COMPACT_CONSUMERS,
                "retained_parameter_queries": ["EntityBomb.GetCostumeLayerSprite", "Sprite.HasCustomShader", "Sprite.HasCustomChampionShader", "LayerState.HasCustomShader", "LayerState.HasCustomChampionShader"],
                "graph": "one closed graph per complete component;no new Sprite alias", "runtime_acceptance": RUNTIME_ACCEPTANCE},
            "class_execution": {"registered": ["NPC", "Delirium-special", "Tear", "Projectile", "Bomb", "Laser", "Knife", "Pickup", "Familiar", "Effect", "Slot1..18", "NativeText", "NativeDummy"],
                                "model": "mod/src/game/client_entity_class_model.cpp", "native": "mod/src/game/client_entity_adapters_native.cpp",
                                "resources": "main/overlay/layers/exact-phase/champion-color/shader;NPC two ANM;Tear three ANM;Bomb five costumes;Knife shared DeadEye;Pickup price/alternate;Slot prize;five typed ExtraData families",
                                "native_links": "client_entity_links.inc;public class references;Familiar Player raw scoped borrow;Knife Plus leaf;Null-explicit;official-EntityPtr-helper",
                                "source_contract": "entity-class-application-v3;same-window draw inputs;current Player render borrow;full Player state=20",
                                "collections": "Laser four independent vectors and inline geometry;Pickup complete cycle;SpriteTrail owned deque and exact 64B records",
                                "owned_presentations": "Familiar48 IsaacsBody/231 AbyssLocust;Effect108 MrMe/166 SpriteTrail/191 PickupGhost;real native shared-owner and constructors/destructors",
                                "npc_compound_draw": "835 references+path;906/920 child ANM;921 C04 secondary path;C18 retained independently;951:3 full path;965:10 named Player minecart offset",
                                "native_type_context": "ordinary Source/preflight/frame require Game Deathmatch state <2;835 path reads only real NPC Type10..999;Player C28 belongs to ANM2 layerCount, never a path",
                                "scene_commit": "I29-single-Scene-commit;non-Player-range-only", "native_draw_dispatch": "I25 implemented;dynamic main/shadow/noarg post/Room-light;water indirect hooks;Authority frame RNG borrow;final Abyss ANM;Text font", "runtime_acceptance": RUNTIME_ACCEPTANCE},
            "common_execution": {"tasks": "I05-I09", "plan": "mod/src/replica/entity_plan.cpp:PlanEntityTarget",
                                 "executor": "mod/src/game/client_objects.cpp:ClientObjects;mod/src/game/client_objects_native.cpp:FormalExecute",
                                 "class_native_apply": True, "scope": "non-Player;full-Player20", "scene_commit": "I29-single-Scene-commit;non-Player-range-only", "runtime_acceptance": RUNTIME_ACCEPTANCE},
            # I04研究索引是资料与采用约束，不把静态审阅升级为原生支持或运行通过。
            "player_handoff": {"catalog": "docs/design/tx-04-20/player-application.json", "target": "player-target-v2", "native_apply": False, "implementation": "mod/src/replica/player_target.cpp;mod/src/game/client_players.cpp"},
            "static_research_context": {"kind": "pre-I34-static-record", "runtime_authority": ACCEPTANCE_DOC,
                                        "note": "原研究时点的pending/UNKNOWN保留；当前运行范围看顶层acceptance_scope，不把旧字段作为推进前置"},
            "static_research": json.loads(RESEARCH.read_text(encoding="utf-8-sig")),
            "classes": [{"source_class": c, "category": d[0], "task": d[1], "contract": d[2], "e2e": d[3], "adapter_registered": c in NATIVE_CLASSES, "native_apply": c in NATIVE_CLASSES,
                         "acceptance_document": ACCEPTANCE_DOC, "coverage": "named-category-scenes;not-all-content-combinations" if c in NATIVE_CLASSES else "one-ordinary-Isaac-support-only;full-Player20"} for c, d in CLASSES.items()],
            "classification": {"method": "public-derived-interface-or-J273-RTTI;never-Type-range", "rejected_legacy": ["NativeDarkness"], "unknown": "explicit-UnknownEntity;never-NPC", "delirium": "EntityNPC+EntityDelirium-special"},
            "slot_variants": {"source": "mod/observation/isaac-cs-observation/domains/entity/slot_variants.lua", "values": [
                {"variant": i+1, "name": name, "contract": "C11;read actual Prize/DonationValue/Timeout/PrizeSprite;no local payout"}
                for i, name in enumerate(("SlotMachine", "BloodDonation", "FortuneTelling", "Beggar", "DemonBeggar", "ShellGame", "KeyMaster", "DonationMachine", "BombBum", "RestockMachine", "GreedDonation", "DressingTable", "BatteryBum", "HomeClosetPlayer", "HellGame", "CraneGame", "Confessional", "RottenBeggar"))], "unknown": "Unavailable;never-default-slot"},
            "structures": [
                {"id": "membership", "task": "I02/I06", "handling": "original count and Player keys retained;partial never deletes"},
                {"id": "application_source", "task": "I03", "handling": "ordinary same-window exact member poses;independent render remains separate/throttled"},
                {"id": "Sprite.pose_runtime", "task": "I03/I10", "handling": "resource-pose-v1;actual source member;nullable unloaded auxiliary"},
                {"id": "Sprite.pose_runtime.champion_color/shaders", "task": "I10", "handling": "11-float native champion ColorMod;early observed successful official shader paths;current Has query must match;unobserved=ReadFailed"},
                {"id": "Entity.application_draw", "task": "I21/I25", "handling": "same ordinary-window manager/game frame/pause/subframe and two inline xorshift states;ordinary native type context excludes Deathmatch layout mode;single frame consumption and restore;never merged independent Render"},
                {"id": "EntityNPC.application_npc", "task": "I25", "handling": "complete BF0 EntityPtr deque and independent C04/C18 Vector2 streams;game allocator, original proxies, per-key links and triplet bounds"},
                {"id": "EntityPlayer.application_player_draw", "task": "I25", "handling": "ready Player Key only;965:10 minecart offset205C/2060;835 keeps Player references without NPC-path access;temporary native borrow, no second Player or full Player claim"},
                {"id": "NativeText.application_text_bytes", "task": "I21", "handling": "same-use writer-witnessed bytes, <=255 plus terminator, full KColor20;Font6 width/draw and ordinary subframe;independent Render unchanged"},
                {"id": "EntityNPC.application_resources", "task": "I10/I11", "handling": "observed completed owner;same-use native ANM +CD8/+DEC;independent resources;Null unload via candidate swap"},
                {"id": "Entity.application_links", "task": "I11-I19/I09", "handling": "fixed client_entity_links.inc plus complete indexed NPC composite slots including Null;stable Key;official EntityPtr registration;unlink before retirement"},
                {"id": "EntityNPC.application_runtime", "task": "I11/I25", "handling": "reference_count cross-checks application_npc complete EntityPtr slots;owned Siren actor is Authority-only update owner, not installed or borrowed from roster"},
                {"id": "EntityBomb.application_runtime", "task": "I14", "handling": "NativeBomb.CostumeDrawEnabled is the actual initialized byte +462;separate from source lazy pending +463"},
                {"id": "EntityLaser.HomingLaser", "task": "I03/I15", "handling": "application_geometry v1;four owned vectors/scalars;six-f32 Bezier;self backpointer retained;public aliases checked;no CalculateSamples"},
                {"id": "Entity.application_owned", "task": "I18/I19", "handling": "five typed ExtraData families;independent ANM poses;Source stable current owner;game allocator/shared control owner;unknown RTTI unavailable;no copied headers"},
                {"id": "EntityLaser.sample_mode_raw/start_offset_mode_opaque32", "task": "I03/I15", "handling": "initialized-owner J273 supplement;signed int8 preserves -1 early-return state;int32 opaque preserves 4F0<=0 branch without guessing semantic name"},
                {"id": "NativeDummy", "task": "I03/I22", "handling": "J273 RTTI native supplement;common only;not NativeDarkness"},
                {"id": "NativeText.draw_color", "task": "I03/I21", "handling": "20-byte KColor;RGBA+opaque32;observed creation owner;never-public-Color-substitution"},
                {"id": "NativeText.text", "task": "I03/I21", "handling": "current-use native writer witness;Init alone insufficient;unwitnessed=NotCaptured;never read pool leftovers"},
                {"id": "NativeText.draw_phase", "task": "I03/I21/I26", "handling": "independent native-text-render-v1 entry inputs;Manager frame/pause/subframe/font;match source state, never merge into ordinary"},
                {"id": "EntityEffect.GetGridEntityDesc", "task": "I24", "handling": "grid.lifted_descriptors owns nullable full description;not generic VarData;no fabricated source Grid key"},
            ], "runtime_acceptance": RUNTIME_ACCEPTANCE}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--docs", action="store_true")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    data = generate()
    text = json.dumps(data, ensure_ascii=False, separators=(",", ":"))
    (args.out / "entity-application.json").write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (args.out / "entity_application.inc").write_text(string_expression(text, "ICSEA"), encoding="utf-8")
    if args.docs:
        (ROOT / "docs/design/tx-04-19/entity-application.json").write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
