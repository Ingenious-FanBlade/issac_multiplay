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
# 运行状态独立于具名安装器；最终验收结束时统一更新，不把每个目录成员标作参数穷尽通过。
RUNTIME_ACCEPTANCE = "tx-04-20:I39-complete;114-logical-Source/cold-Client;18-49+5/19-65-regressions;finite-matrix;see-docs/design/tx-04-20/tx-04-20-e2e.md"
RUNTIME_MEMBER = "named-scene coverage only;captured queries are not independent native query replay;not every parameter or nonempty optional branch"
CLASSES = PLAYER_RELATED | RESOURCE_CLASSES | {
    "Entity", "EntityPlayer", "PlayerManager", "ItemConfigItem", "ItemConfigCostume",
    "ItemConfigCard", "ItemConfigPillEffect", "EffectList", "PosVel", "BitSet128"}
# I11—I15与生产端同表取成员；目录不因任务编号完成而把其它查询/资源标成已安装。
STATE_FIELDS = set(re.findall(r'CS_PLAYER_FIELD\("([^"]+)"',
    (ROOT / "mod/src/game/client_player_fields.inc").read_text(encoding="utf-8")))
STATE_CONTAINERS = {"EntityPlayer." + name for name in (
    "GetCollectiblesList", "GetVoidedCollectiblesList", "GetWispCollectiblesList", "GetSpoofedCollectiblesList",
    "GetSmeltedTrinkets", "GetSmeltedTrinketDesc", "GetTrinket", "IsCollectibleBlocked", "IsTrinketBlocked",
    "GetInnateCollectibleCount", "GetInnateTrinketCount", "GetInnateCollectibleGroup", "GetInnateTrinketGroup")}
STATE_SLOTS = {"EntityPlayer.GetActiveItemDesc", "EntityPlayer.SecondaryActiveItem", "EntityPlayer.GetPocketItem", "EntityPlayer.QueuedItem"}
# 只把资源解析器实际消费的具名成员标为原生pose后端；其余动画元数据／任意查询仍保留只读处置。
RESOURCE_POSE_MEMBERS = set(re.findall(r'"((?:Sprite|LayerState|BlendMode)\.[^"]+)"',
    (ROOT / "mod/src/game/client_resource_model.cpp").read_text(encoding="utf-8")))
# 只把已经存在的具名安装器登记为实现；操作接口始终保持Authority-only。
EQUIPMENT_MEMBERS = {
    "EntityPlayer.GetEffects", "EntityPlayer.HasPlayerForm", "EntityPlayer.GetPlayerFormCounter",
    "EntityPlayer.GetHistory", "History.GetCollectiblesHistory", "EntityPlayer.GetBodySprite",
    "EntityPlayer.GetHeldSprite", "EntityPlayer.GetCostumeSpriteDescs", "EntityPlayer.GetCostumeLayerMap",
    "EntityPlayer.GetWeapon", "EntityPlayer.FireDelay",
}
EQUIPMENT_OWNERS = {"TemporaryEffect", "HistoryItem", "CostumeSpriteDesc", "Weapon"}
# 与产品角色注册表共源；专用状态/绘制已接入，运行结果仍只由I39填写。
VANILLA_ROLES = [
    {"type": int(t), "name": n, "family": "C" + f"{int(f):02}", "task": "I" + task,
     "resources": "I18-I20:per-body-final-state;no-starting-item-or-random-replay",
     "draw": "I29-native-pass+frame-resource-borrow", "runtime_status": RUNTIME_ACCEPTANCE + ";basic type final state;not all natural combinations"}
    for t, n, f, task in re.findall(r'CS_PLAYER_ROLE\((\d+),\s*"([^"]+)",\s*(\d+),\s*(\d+)\)',
        (ROOT / "mod/src/game/client_player_roles.inc").read_text(encoding="utf-8"))
]
ROLE_MEMBERS = {"EntityPlayer." + name for name in (
    "GetPlayerType", "GetPlayerIndex", "GetHeldEntity", "GetItemState", "GetSuplexState",
    "GetSuplexAimCountdown", "GetSuplexTargetPosition", "GetSuplexLandPosition",
    "GetMainTwin", "GetOtherTwin", "GetSubPlayer", "IsSubPlayer", "GetFlippedForm", "IsHologram")}
SPECIAL_MEMBERS = {"EntityPlayer." + name for name in (
    "GetBagOfCraftingContent", "GetBagOfCraftingSlot", "GetBagOfCraftingOutput", "GetBagOfCraftingOutputItemPool",
    "GetPoopSpell", "GetPoopMana", "GetInventoryHistoryIndex", "GetInventoryCollectible", "GetMaxInventorySize",
    "GetSoulCharge", "GetBloodCharge", "GetEveSumptoriumCharge", "SamsonBerserkCharge", "IBSCharge")}
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
                "handling": "Authority-only-operation;install-final-result-through-state-member" if authority else "I31:captured-readonly-query;native-storage-only-where-named-below",
                "consumer": "Authority" if authority else "ClientPlayerDisplayView::Captured;native installer only for named dispositions",
                "installation": "never-replay-operation" if authority else "ClientPlayerDisplayView::Captured;original-parameters/status/graph-root;no-local-query-replay",
                "cleanup": "no-Client-operation-owner" if authority else "I33;body/child-resource owner;borrow-generation invalidation",
                "implementation": "mod/src/replica/player_target.cpp;mod/src/game/client_player_view.cpp" if not authority else "Authority-only;no-Replica-RPC",
                "implementation_status": "readonly-captured-result;not-a-native-storage-claim" if not authority else "Authority-only;no-Client-operation", "runtime_status": "Authority-only;no Client operation" if authority else RUNTIME_MEMBER,
                "task": task, "research": research, "e2e": e2e,
                "absence": "Missing/Null/Empty/Unavailable/ReadFailed/NotCaptured remain distinct",
            }
            if not authority and (m["id"] in STATE_FIELDS | STATE_CONTAINERS | STATE_SLOTS or owner in {"ActiveItemDesc", "PocketItem", "QueueItemData"}):
                row = rows[key]
                row.update(handling="I11-I15:typed-final-native-storage;no-gameplay-replay",
                           consumer="ClientPlayers/ClientPlayerStateFields;HUD keeps original public query semantics",
                           installation="application_player_state-v1;per-body native backing;local-config lookup;complete containers;role-NotApplicable is distinct",
                           implementation="mod/src/game/client_player_model.cpp;mod/src/game/client_player_state_native.cpp;mod/src/game/client_player_fields.inc",
                           implementation_status="implemented-named-final-native-storage;integrated-with-complete-Player-owner",
                           cleanup="Player/official-Plus own containers;queue config borrow revoked before Free;no double destruction")
            if not authority and (m["id"] in EQUIPMENT_MEMBERS or owner in EQUIPMENT_OWNERS):
                rows[key].update(
                    handling="I16-I20:typed-body-equipment;public-query-and-native-storage-semantics-kept-distinct",
                    consumer="ClientPlayers/ClientPlayerEquipment;History exposes committed immutable body view",
                    installation="application_player_equipment-v1;effects/forms;Player-domain History;main+13-inline poses;costume vector/map;five typed Weapon owners",
                    implementation="mod/src/game/client_player_equipment_model.cpp;mod/src/game/client_player_equipment_native.cpp;mod/src/game/client_player_weapon_native.cpp",
                    implementation_status="implemented-named-equipment-and-frame-resource-owner",
                    cleanup="unlink actual EntityPtr slots and weapon owner before virtual Free;costume ANM/native Player each destroyed once;unknown prefixes retained")
            if not authority and m["id"] in ROLE_MEMBERS:
                role_task = "I21" if m["id"] in {"EntityPlayer.GetHeldEntity", "EntityPlayer.GetItemState"} or "Suplex" in m["id"] else "I22" if m["id"] == "EntityPlayer.GetPlayerType" else "I23-I25"
                rows[key].update(
                    task=role_task, research="R09/R10/R11", e2e="E03-E07/E18/E22/E23",
                    handling="I21-I25:final-composite-values+whole-target-vanilla-body-graph",
                    consumer="ClientPlayerRolesPrepare/ClientPlayerRoleFields/ClientPlayerRoleRelations;general-held-frame-borrow",
                    installation="application_player_role-v1;native storage then all-body links then public native getter checks;no InitTwin/Swap/Flip/Hold/Throw/Suplex",
                    implementation="mod/src/game/client_player_roles.cpp;mod/src/game/client_player_roles_native.cpp;mod/src/game/client_players_native.cpp;mod/src/game/client_entity_draw_native.cpp",
                    implementation_status="implemented-role-graph;Birthright active twins and structurally dormant backups distinguished",
                    cleanup="held raw borrow and twin/sub/backup/replaced links detach for all bodies before unique child/native owner retirement")
            if not authority and m["id"] in SPECIAL_MEMBERS:
                rows[key].update(task="I27", research="R05/R06/R12/R13", e2e="E10-E16/E18/E21",
                    handling="I27:finite typed final state;public query semantics remain distinct",
                    consumer="ClientPlayerSpecialFields;ClientPlayerStateFields;Player/Entity19/Weapon unique owners",
                    installation="special-state-v1:history-index16/crafting8+output+pool/poop6;final soul/blood/Eve fields;never recipe/refill/spawn",
                    implementation="mod/src/game/client_player_roles.cpp;mod/src/game/client_player_roles_native.cpp;mod/src/game/client_player_draw_native.cpp",
                    implementation_status="implemented-special-state-and-native-draw;full HUD layout=tx27",
                    cleanup="native Player owns finite slots/deque/Image;Entity19 owns clots/wisps/locusts/DarkEsau;no duplicate Player")
            if not authority and m["id"] in {"PlayerManager.GetEsauJrState", "ActiveItemDesc.VarData"}:
                rows[key].update(task="I26", research="R11", e2e="E08/E21/E23",
                    handling="I26:explicit four-slot backup identity and per-active703 VarData association",
                    consumer="ClientPlayers::Backup;ClientNativePlayers backup instance mirror;ClientPlayerStateFields",
                    installation="VarData0 unassigned;1..4 source slot0..3;all local bodies acquired before mirror binding;never write Authority global slots",
                    implementation="mod/src/game/client_players.cpp;mod/src/game/client_players_native.cpp;mod/src/game/client_player_model.cpp",
                    implementation_status="implemented-four-real-backup-slots;source-qualified-state-owner",
                    cleanup="revoke mirror raw borrows then unique Player owner;Authority slot reclaim remains outside Client ownership")
            if not authority and m["id"] in RESOURCE_POSE_MEMBERS:
                rows[key].update(
                    handling="named-native-resource-pose-backing;public getter and local asset metadata remain distinct",
                    consumer="ClientParseResourcePose/ClientApplyResourcePose/ClientResourcePoseMatches;Player frame borrow",
                    installation="complete ANM/layer pose and selectors;validated local resource identity;no Update",
                    implementation="mod/src/game/client_resource_model.cpp;mod/src/game/client_resources_native.cpp;mod/src/game/client_entity_draw_native.cpp",
                    implementation_status="implemented-shared-resource-owner;nullable and zero-length animation semantics preserved",
                    cleanup="own ANM/Image/selector generations;CPU/GPU drain;explicit scalar restoration after native copy/Swap")
    research_path = ROOT / "docs/design/tx-04-20/player-native-contracts.json"
    return {"format": "isaac.cs.player.application-catalog", "version": 1,
            "target_contract": "player-target-v2", "source_contract": "player-source-v2",
            "projection_review": PLAYER_PROJECTION_REVIEW,
            "native_apply": True, "runtime_acceptance": RUNTIME_ACCEPTANCE, "runtime_coverage_semantics": RUNTIME_MEMBER,
            "acceptance_document": "docs/design/tx-04-20/tx-04-20-e2e.md",
            "supplementary_research": "docs/design/tx-04-20/player-i39-source.md;historical R01-R16 unknowns are not rewritten as runtime proof",
            "source_qualifications": {"2": "CreationOwnerCompleted", "3": "user-accepted current exact-pair PlayerUpdate witness only", "4": "observed ctor/base Init and named child/full-role owner completed", "5": "named state-slot owner completed;input binding separate;lifecycle revokes"},
            "i36_i37_implementation": {
                "status": "implemented;product and E2E built;runtime result separately reported",
                "diagnostics": "cs status player_status_version=1;admitted target/valid applied revisions;participant/body/backup budgets;role masks;last step and first failure stable Key/dependency/field/instance/shape/body-mask;retirement cause/failure and recovery;pure bounded frame-end copies;no Player addresses or per-tick recording dependency",
                "inspect": "ISAACCS_E2E only;Scene committed objects plus every native Player;read-only actual sets/state/inventory/effects/ordered History/slots/costume/Weapon/role/special/ANM/Image/records/relations/HUD checks;diagnostic failure does not mutate fault or publication",
                "faults": "ISAACCS_E2E only;token and Manager arm;one-shot normal boundary stops;actual body masks, weapon resource-stage+slot/generation and draw pass explicitly distinguished;no unknown SEH simulation",
                "engineering": "C++20 pure models and C++17 SDK boundary retained;all production modules plus formal same-window observation;existing permanent Hook registry reused;Player catalog embedded/generated/copied/allowlisted in package",
                "outputs": "product=out/repentogon;E2E DLL/helpers/catalogs/Lua/harness=out/repentogon/e2e;separate build trees;release allowlist excludes test bridge/fixture/harness/fault implementation"},
            "i31_i35_implementation": {
                "hud": "Scene-only committed participant/body/slot immutable view;all heart/resource/active/pocket/charge/History/special/explicit sharing;captured queries preserve original semantics;required player-hud-state-v1 uses same-window dynamic HUD heart24/color/flash/mantle;native980020 factory owns real HUD+control pair;7D2B20 releases;full HUD layout remains tx27",
                "isolation": "Player purpose frames;unexpected RNG FaultSkip before original;full Client SFX Play/Stop deny;existing callback/Lua persistence/native writers and shutdown protection remain installed until safe retirement",
                "retirement": "Player stop-borrow/all-outgoing-links and dynamic HUD before non-Player retirement;per-body detach/count/Free prefixes;ANM copy/swap/dtor owners registered before entry and retained on uncertainty;one automatic complete-target recovery only after safe retirement;stop/foreign/VM/context failures do not grant retries",
                "commit": "single Scene attempt;source-complete plus checked Player owner and matching full entity set;separate body/relation/resource/Player domain flags;legacy support cannot set complete Entity/Player bits;no network ACK",
                "budget": "4 participants/64 group members/256 bodies/global4 backups;per-container and aggregate budgets;32MiB source/64Mi nodes;4096 resources/65536 layers/262144 references/65536 plan cost;bounded graph expansion bytes;immutable decoded values and native resource reuse",
                "cost": "E2E aggregate source/parse/apply/draw/retire durations;status process working-set/private bytes separate from owned-value estimate;no realtime bandwidth claim",
                "status": "implemented;integrated Scene ownership;runtime result separately reported"},
            "i26_i30_implementation": {"backup": "four source slots;typed703 VarData;native mirror instance table;no Authority global slot writes", "special": "complete finite inventory-History/crafting/poop and final role/effect/Weapon state;Samson+2070/IBS+2074 distinct from CraftingUseCharge+1F54", "lifecycle": "Scene-committed Key tombstones survive native rebuild;dead/ghost/inactive distinct;complete membership only", "draw": "native Player pass+Image source+Source palette;typed optional list/cursor+record Sprite;full ANM strong copies through CPU drain;hologram RNG render-only;no Pre/Update;multi-body presentation member cache", "control": "committed pure group view;one intent stream per ordinal;binding generation independent from shape;native controller=-1", "status": "implemented;native presentation and immutable control view;runtime result separately reported"},
            "role_implementation": {"tasks": "I21-I25", "source": "application_player_role-v1;same-use ordinary window;source-v2.role_state=player-role-state-v1", "model": "mod/src/game/client_player_roles.cpp", "native": "mod/src/game/client_player_roles_native.cpp", "registry": VANILLA_ROLES, "internal": "possessor -1/variant2 and other Baby/internal variants explicitly unsupported;undeclared Mod characters=tx-04-27", "relationships": "source storage and public main/other/sub/flipped remain distinct;whole-target group and shape validation;all-links-before-all-checks", "hologram": "actual native exists/type29-or38/twin/lower-player-index predicate;no invented flag", "status": "implemented-complete-role-owner;native_apply=true;runtime result separately reported"},
            "structure_implementation": {"plan": "mod/src/replica/player_plan.cpp", "owner": "mod/src/game/client_players.cpp", "native": "mod/src/game/client_players_native.cpp", "tasks": "I05-I10", "scope": "base-entity-initialized/role-uninitialized until named I11-I29 adapters;preflight rejects absent adapters", "scene_commit": "ClientScene only;complete body/relation/resource checks before Player+full Entity domain bits;legacy remains separate"},
            "final_state_implementation": {"tasks": "I11-I15", "source": "application_player_state-v1;same-use ordinary-window;no addresses or headers", "model": "mod/src/game/client_player_model.cpp", "native": "mod/src/game/client_player_state_native.cpp", "fields": sorted(STATE_FIELDS), "inventory": "counts/voided/wisps/smelted/trinkets;official Plus blocked+all innate groups including legacy and temporary timers", "slots": "active4/pocket4/queue;PartialCharge=f32 despite ZHL int;EsauJr typed backup slot", "status": "implemented-named-final-state;native_apply=true;runtime result separately reported"},
            "equipment_implementation": {"tasks": "I16-I20", "source": "application_player_equipment-v1;current-use ordinary window;public Sprite/Costume/Weapon and independent Player History component graph", "model": "mod/src/game/client_player_equipment_model.cpp", "native": "mod/src/game/client_player_equipment_native.cpp;mod/src/game/client_player_weapon_native.cpp", "effects": "ordered local-config/count/cooldown;cacheFlags/disabled/evaluate;15 form counters;no Add/Remove/EvaluateItems", "history": "ordered native vector plus Scene-committed immutable per-body HUD view;no equality-based sharing inference", "resources": "main and 13 physical inline ANM;complete exact pose;headAnim bytes/string assignment from named 773990 writer;ordered costume vector and 15 layer map;empty clears old", "weapons": "five explicit slots;types1..15;actual dynamic vtables;per-slot generations;EntityPtr links;detach owner before native Free;no Fire/Update/ClearReferences replay", "status": "implemented-complete-equipment-owner;native_apply=true;runtime result separately reported"},
            "retained_candidate_capacity": 1056,
            "capacity_semantics": "guard limits are not tested maxima;actual matrix max15 bodies,8 active,4 global backups;not realtime bandwidth",
            "members": list(rows.values()),
            "native_members": [{"id": "NativePlayer." + ("blood_gush" if i == 10 else "held" if i == 11 else "inline_" + str(i)),
                                "source": "world_layout:player_inline_sprite_first_offset+i*player_inline_sprite_stride",
                                "physical_slot": i, "returns": "Sprite-or-Null", "family": "P11/P12",
                                "evidence": "J273 ctor VA0x74DC70;13 physical ANM constructors",
                                "read_basis": "same-use current public readiness;completed creation owner2, named child/full-init4 or state-owner5;current exact-pair witness3 stays separate;ctor is not Load proof",
                                "source_lifetime": "current entity ordinary window;revoke on use/owner loss",
                                "sharing": "independent-physical-member;no-filename-alias", "same_window": "entity+player",
                                "source_implementation": "mod/src/game/world_sources/lua_entity.cpp:EntityApplicationSprites",
                                "consumer": "ClientPlayerEquipmentResources;I18/I19/I29", "installation": "implemented-independent-exact-pose;native-Player-owns-ANM;integrated-I29-draw-admission",
                                "cleanup": "I33;native-Player owns embedded-ANM;do-not-free-borrow", "research": "R07/R13/R15",
                                "e2e": "E13/E20/E23", "runtime_status": RUNTIME_MEMBER} for i in range(1, 13)] + [
                {"id": "NativePlayer.RecordSprite", "physical_offset": "0x1C50", "source": "application_player_special.record_sprite;ctor82E1C0;current ordinary window", "consumer": "ClientPlayerDrawResources;native82E340;whole-frame strong borrow", "installation": "independent complete exact ANM pose;record list/cursor separately typed;no Update", "cleanup": "native Player unique owner;restore after CPU drain", "task": "I29", "research": "R13", "runtime_status": RUNTIME_MEMBER},
                {"id": "NativePlayer.DynamicHUD", "physical_offset": "0x20A8/0x20AC", "source": "application_player_special.hud;player-hud-state-v1;same ordinary window;actual dynamic owner not Lua getter fallback", "consumer": "ClientPlayerHudInstall;Player.Render78573F/83AC80/834930", "installation": "native980020(HUD,outPair,Player);only own ctor/baseInit empty pair;24 hearts+index/color/flash/mantle;no HUD Update;Game8 slots separate", "cleanup": "7D2B20 -> ReferenceCount<PlayerHUD>7D33A0 -> dtor6EC2F0;both pair words cleared before Player Free;entered exception retained", "task": "I31/I33", "research": "R14/R15", "runtime_status": RUNTIME_MEMBER}],
            "structures": [
                {"id": n, "source": source, "consumer": consumer, "task": task, "status": "I05-I10:typed-target-plan-and-complete-structural-owner;named final-state/role/resource adapters integrated"}
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
