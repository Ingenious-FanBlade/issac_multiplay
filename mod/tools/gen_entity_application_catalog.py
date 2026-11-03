"""tx-04-19 全成员应用责任目录；资料生成不执行游戏或测试。"""
import argparse
import json
from pathlib import Path
from gen_replica_catalog import classify
from replica_projection_policy import consumer_review

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "mod/observation/isaac-cs-observation/entity-catalog.json"
PARSER = "mod/src/replica/entity_target.cpp:ParseEntityTarget"
RESEARCH = ROOT / "docs/design/tx-04-19/entity-native-contracts.json"
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


def route(m):
    """公共继承只定义一次；类别字段与关联值分别声明安装/纯值/外部责任。"""
    owner, member = m["class"], m["member"]
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
        return task, "class_state", "类别最终值/容器；具体原生存储由该adapter实现，当前纯值保留不等于原生应用", e2e
    return "I08/I09", "associated_value", "配置/PathFinder/命中集合等关联纯值；禁止寻路、抽奖或复制原生Index为本地引用", "E01/E15"


def generate():
    catalog = json.loads(SOURCE.read_text(encoding="utf-8-sig"))
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
            target_field, storage = "EntityTarget.objects/" + section + "/fields/" + m["id"], "owned-pure-value;native-storage-pending-" + task
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
            "native_status": "authority_not_replayed" if ownership == "AuthorityOnly" else "not_applied_by_I01_I03",
            "e2e": e2e,
        })
    # 结构型补充与公开行使用同一责任目录，不能靠测试夹具补表现源。
    return {"format": "isaac.cs.entity.application-catalog", "version": 1, "model_version": 2,
            "target_contract": "entity-target-v1", "source_contract": "entity-source-v1", "members": rows,
            # I04研究索引是资料与采用约束，不把静态审阅升级为原生支持或运行通过。
            "static_research": json.loads(RESEARCH.read_text(encoding="utf-8-sig")),
            "classes": [{"source_class": c, "category": d[0], "task": d[1], "contract": d[2], "e2e": d[3], "native_apply": False} for c, d in CLASSES.items()],
            "classification": {"method": "public-derived-interface-or-J273-RTTI;never-Type-range", "rejected_legacy": ["NativeDarkness"], "unknown": "explicit-UnknownEntity;never-NPC", "delirium": "EntityNPC+EntityDelirium-special"},
            "slot_variants": {"source": "mod/observation/isaac-cs-observation/domains/entity/slot_variants.lua", "values": [
                {"variant": i+1, "name": name, "contract": "C11;read actual Prize/DonationValue/Timeout/PrizeSprite;no local payout"}
                for i, name in enumerate(("SlotMachine", "BloodDonation", "FortuneTelling", "Beggar", "DemonBeggar", "ShellGame", "KeyMaster", "DonationMachine", "BombBum", "RestockMachine", "GreedDonation", "DressingTable", "BatteryBum", "HomeClosetPlayer", "HellGame", "CraneGame", "Confessional", "RottenBeggar"))], "unknown": "Unavailable;never-default-slot"},
            "structures": [
                {"id": "membership", "task": "I02/I06", "handling": "original count and Player keys retained;partial never deletes"},
                {"id": "application_source", "task": "I03", "handling": "ordinary same-window exact member poses;independent render remains separate/throttled"},
                {"id": "Sprite.pose_runtime", "task": "I03/I10", "handling": "resource-pose-v1;actual source member;nullable unloaded auxiliary"},
                {"id": "EntityLaser.HomingLaser", "task": "I03/I15", "handling": "owner-qualified opaque identity;public optimized/nonoptimized sample vectors retained;private geometry versioned separately"},
                {"id": "EntityLaser.sample_mode_raw/start_offset_mode_opaque32", "task": "I03/I15", "handling": "initialized-owner J273 supplement;signed int8 preserves -1 early-return state;int32 opaque preserves 4F0<=0 branch without guessing semantic name"},
                {"id": "NativeDummy", "task": "I03/I22", "handling": "J273 RTTI native supplement;common only;not NativeDarkness"},
                {"id": "NativeText.draw_color", "task": "I03/I21", "handling": "20-byte KColor;RGBA+opaque32;observed creation owner;never-public-Color-substitution"},
                {"id": "NativeText.text", "task": "I03/I21", "handling": "current-use native writer witness;Init alone insufficient;unwitnessed=NotCaptured;never read pool leftovers"},
                {"id": "NativeText.draw_phase", "task": "I03/I21/I26", "handling": "independent native-text-render-v1 entry inputs;Manager frame/pause/subframe/font;match source state, never merge into ordinary"},
                {"id": "EntityEffect.GetGridEntityDesc", "task": "I24", "handling": "grid.lifted_descriptors owns nullable full description;not generic VarData;no fabricated source Grid key"},
            ], "runtime_acceptance": "pending-full-tx-04-19-I34;catalog-and-target-are-not-native-support"}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--docs", action="store_true")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    data = generate()
    text = json.dumps(data, ensure_ascii=False, separators=(",", ":"))
    (args.out / "entity-application.json").write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (args.out / "entity_application.inc").write_text('std::string{}' + ''.join('+std::string(R"ICSEA(' + text[i:i+8000] + ')ICSEA")' for i in range(0, len(text), 8000)), encoding="utf-8")
    if args.docs:
        (ROOT / "docs/design/tx-04-19/entity-application.json").write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
