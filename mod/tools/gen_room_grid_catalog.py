"""I01逐成员应用目录：生成产品数据和可审阅JSON；不运行游戏/测试，不计算摘要。"""
import argparse
import json
import pathlib
from cpp_catalog import string_expression
from gen_replica_catalog import classify
from replica_projection_policy import consumer_review

ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE = ROOT / "mod/observation/isaac-cs-observation"
PARSER = "mod/src/replica/room_grid_target.cpp:ParseRoomGridTarget"
PLAN = "mod/src/replica/room_grid_plan.cpp:PlanRoomGridTarget"
NATIVE = "mod/src/game/client/scene/client_native_world.cpp:PrepareImpl/RetireImpl"
ROOM_NATIVE = {
    "I07": "mod/src/replica/room_grid_target.cpp:ConfigValue;mod/src/game/client/scene/client_native_world.cpp:ResolveConfiguration/OwnConfiguration",
    "I08": "mod/src/replica/room_geometry.h;mod/src/game/client/room/client_room_native.cpp:ClientRoomStateApply",
    "I09": "mod/src/game/client/scene/client_scene.cpp:RoomState;mod/src/game/client/room/client_room_native.cpp:ClientRoomStateApply/ClientRoomFrameMaintain",
    "I10": "mod/src/game/client/scene/client_native_world.cpp:FinalizeImpl;mod/src/game/client/presentation/client_presentation_native.cpp:DrawImpl",
    "I11": "mod/src/game/client/presentation/client_presentation.cpp:PrepareTarget;mod/src/game/client/presentation/client_presentation_native.cpp:LuaCaptureAux/BuildAuxCandidate/DrawImpl/ColorComposite",
    "I12": "mod/src/game/client/room/client_room_native.cpp:ClientRoomStateCaptureLua/ClientRoomRailsDraw;mod/src/replica/room_geometry.h",
    "I13": "mod/src/game/client/room/client_map_native.cpp;mod/src/game/client/scene/client_scene.cpp:MapState;mod/src/game/client/presentation/client_presentation_native.cpp:PrepareImpl",
}
GRID_NATIVE = {
    "I14": "mod/src/game/client/grid/client_grids.cpp:PrepareTarget/Step/Retire;mod/src/game/client/grid/client_grid_native.cpp:Entry",
    "I15": "mod/src/game/client/grid/client_grid_native.cpp:Execute/Remove/ClientGridWaterClip;mod/binding/hooks.json",
    "I16": "mod/src/game/client/grid/client_grids.cpp:Parse;mod/src/game/client/grid/client_grid_native.cpp:Common/FieldsThunk",
    "I17": "mod/src/game/client/grid/client_grids.cpp:Parse;mod/src/game/client/grid/client_grid_native.cpp:Runtime/Resources",
    "I18": "mod/src/replica/room_grid_target.cpp;mod/src/game/client/grid/client_grid_native.cpp:ClientNativeGridsBindDoors/Unpublish",
    "I19": "mod/src/game/client/grid/client_grids.cpp:ResolveReferences/VisitLifted",
    "I20": "mod/src/game/client/scene/client_scene.cpp:RoomState;mod/src/game/client/room/client_room_native.cpp:ClientRoomStateApply;mod/src/game/client/scene/client_native_world.cpp:ClientNativeWorldCheckCommit",
    "I21": "mod/src/game/client/resources/client_resources_native.cpp;mod/src/game/client/grid/client_grid_native.cpp:ClientGridDraw;mod/src/game/client/presentation/client_presentation_native.cpp:DrawImpl",
    "I22": "mod/src/game/client/grid/client_grid_events.cpp;mod/src/game/client/scene/client_scene.cpp:AdmitGridIndependent",
    "I23": "mod/src/game/client/grid/client_grids.cpp:Retire;mod/src/game/client/grid/client_grid_native.cpp:Remove;mod/src/game/client/scene/client_scene.cpp:Pump",
}
# 仍发送的事实保留在T；参数查询是否发送以模型v2清单为准，未来任务说明不是已有消费者。
ROOM_FACT_CONSUMERS = {
    "Room.CanSpawnObstacleAtPosition": "I20 collision/placement query",
    "Room.GetAliveEnemiesCount": "tx-04-19 Entity membership",
    "Room.GetAliveBossesCount": "tx-04-19 Entity membership",
    "Room.GetSecondBossID": "tx-04-19 boss Entity/context",
    "Room.GetBossVictoryJingle": "tx-04-27 audio",
    "Room.GetBrokenWatchState": "tx-04-25 run effects",
    "Room.GetShopLevel": "tx-04-25 run/shop rules",
    "Room.GetShopItemPrice": "tx-04-25 authoritative shop price",
    "Room.GetDeliriumDistance": "tx-04-25 boss selection;tx-04-26 topology",
    "Room.GetDungeonRockIdx": "I17 dungeon Grid layout",
    "Room.GetTintedRockIdx": "I17 authoritative rock Grid state",
    "Room.GetEnemyDamageInflicted": "tx-04-25 room damage rules",
    "Room.GetLaserTarget": "tx-04-19 laser Entity/reference",
    "Room.IsCurrentRoomLastBoss": "tx-04-25 boss rules;tx-04-26 route",
    "Room.IsMirrorWorld": "I13 mirror Camera;tx-04-26 dimension flow",
    "Room.IsPersistentRoomEntity": "tx-04-24 room-save membership;tx-04-19 Entity",
    "Room.GetEffects": "tx-04-25 room temporary gameplay effects",
    "Room.HasCurseMist": "tx-04-19 environment Entity membership",
    "Room.HasWaterPits": "I17/I21 authoritative GridPit/water draw",
    "RoomDescriptor.GridIndex": "I06普通值核验；dim1 1x1异常raw仅诊断，native原点取实际offset，不冒认raw等价",
    "RoomDescriptor.BossDeathSeed": "tx-04-25 authoritative reward RNG",
    "RoomDescriptor.DeliriumDistance": "tx-04-25 boss selection;tx-04-26 topology",
    "RoomDescriptor.GetErrorTrinketEffect": "tx-04-25 authoritative room rules",
    "RoomDescriptor.GetGroup": "tx-04-26 level group/topology",
    "RoomDescriptor.GetGroupByName": "tx-04-26 level group/topology",
    "RoomDescriptor.GetRestrictedGridIndexes": "I17/I20 authoritative Grid placement",
    "RoomDescriptor.GetValidNeighborPlacementLocations": "tx-04-26 level placement",
    "Room.IsFirstEnemyDead": "tx-04-25 room rules;retained fact",
    "Room.HasTriggerPressurePlates": "I17/I20 Grid membership",
    "Room.KeepDoorsClosed": "I18 authoritative Door states",
}


def route(m):
    """每个原始目录行都有处理；参数查询保留结果，操作绝不转为网络RPC。"""
    owner, name = m["owner"], m["member"]
    if owner.startswith("GridEntity"):
        if owner == "GridEntityDesc": return "I16", "GridDescriptorTarget及原始Desc字段", "E04/E05"
        if owner == "GridEntityDoor": return "I18", "DoorBinding及G16专用主体/额外Sprite", "E06/E11"
        if name.startswith("To"): return "I03", "GridTarget原始casts（类型派生见证，不执行远程cast）", "E04/E13"
        if name in ("GetType", "GetVariant", "GetGridIndex"): return "I03", "GridTarget类型/variant/index与common交叉语义", "E04/E05"
        if owner == "GridEntity" and name in ("State", "VarData", "CollisionClass", "Position", "GetSaveState"): return "I16", "GridTarget公共值/Desc；Position保留纯值", "E04/E05"
        if name in ("GetSprite", "GetRenderPosition", "GetWaterClipFlags", "RenderTop", "Render"): return "I21", "Grid资源/姿态/裁切/绘制pass", "E04/E08"
        if name in ("Init", "PostInit"): return "I15", "Grid构造/初始化owner（非RPC）", "E04/E13"
        return "I17", owner + "专用字段/查询结果；对应G01—G27", "E04/E07/E11"
    if owner in ("RoomConfig", "RoomConfigRoom", "RoomConfigSpawn", "RoomConfigEntry", "RoomConfigStage"):
        return "I07", "RoomConfiguration及完整配置资源子树（候选不等于最终成员）", "E03/E13"
    if owner == "RoomDescriptor":
        if name in ("GridIndex", "SafeGridIndex", "ListIndex", "GetDimension"): return "I06", "RoomDescriptorTarget.location；Game两数组/各维度offset", "E02/E03"
        if name in ("Data", "OverrideData"): return "I07", "RoomDescriptorTarget.data/override_data独立可空配置", "E03/E13"
        if name == "Doors" or name == "AllowedDoors": return "I18", "候选索引/启用位与实际门主体分开", "E06"
        if "SaveState" in name: return "tx-04-24", "离房保存域；18实际展示所需当前Desc仍归I16/I19", "E11"
        if name in ("DisplayFlags", "GetNeighboringRooms"): return "I13", "当前楼层拓扑/地图显示上下文；完整探索历史归24", "E10"
        return "I09", "RoomDescriptorTarget完整原始字段；最终进度值", "E03/E09"
    if owner in ("LRoomAreaDesc", "LRoomTileDesc", "RailManager"): return "I12", "Room专用结构/轨道及关联Sprite", "E09"
    if owner == "Backdrop": return "I10", "背景资源与真实构建流；controls资源按Player依赖收尾", "E01/E08"
    if owner in ("FXParams", "FXLayers", "ColorModifier"): return "I11", "EnvironmentTarget及完整持续环境纯值", "E08"
    if owner == "Camera": return "I13", "Camera目标与真实地图/scroll展示上下文", "E10"
    if owner == "RoomTransition": return "tx-04-27", "过渡展示；18当前稳定房间需明确transition=0", "E08/E14"
    if owner == "Level":
        if name in ("GetCurrentRoomIndex", "GetDimension", "GetCurrentRoomDesc", "GetRoomByIdx", "GetRooms", "GetRoomCount", "GetLastRoomDesc", "GetPreviousRoomIndex", "EnterDoor", "LeaveDoor"):
            return "I06", "RoomTarget.current_index/descriptor及正式已采相关descriptor", "E02/E03"
        if name in ("GetCurses", "GetCanSeeEverything", "GetName", "GetNeighboringRooms"): return "I13", "必要当前楼层地图上下文；完整探索归24", "E10"
        if name in ("GetStage", "GetStageType", "GetAbsoluteStage", "GetStartingRoomIndex"): return "I06", "原生Game当前展示上下文；完整楼层重构归26", "E03"
        return "tx-04-25", "整局裁决/路线/随机最终状态；18保留来源，不本地执行", "E16"
    if owner == "Room":
        if any(s in name for s in ("Water", "Color", "Lighting", "Lightning", "Lava", "Rain", "CurseMist")) or name in ("GetFXParams", "GetFXLayers", "GetEffects"): return "I11", "EnvironmentTarget持续状态及完整原始环境树", "E08"
        if "Backdrop" in name or name == "GetRoomConfigStage": return "I10", "背景构建与资源依赖", "E08"
        if "Rail" in name or "LRoom" in name: return "I12", "专用Room结构/轨道", "E09"
        if name in ("GetDoor", "IsDoorSlotAllowed", "GetDoorSlotPosition", "RemoveDoor") or "Door" in name: return "I18", "Room门槽/允许/候选/实际主体关系", "E06"
        if "Grid" in name or name in ("CheckLine", "FindFreeTilePosition", "IsPositionInRoom", "GetClampedPosition", "TryMakeBridge", "TryPlaceLadder"): return "I20", "最终格子几何/碰撞/路径查询及维护；Grid主体归I14—I19", "E02/E07"
        if name in ("GetRoomShape", "GetCenterPos", "GetTopLeftPos", "GetBottomRightPos", "IsLShapedRoom"): return "I08", "RoomLayout几何/边界及原始查询结果", "E02"
        if any(s in name for s in ("Camera", "Render", "Screen")): return "I13", "Camera及具名绘制上下文；完整Render不用于补状态", "E10"
        if name == "GetEntities": return "tx-04-19", "唯一Entity域主体引用；18不造替代Entity", "E11"
        if any(s in name for s in ("Random", "Seeded", "Champion", "ItemPool", "DevilRoomChance")): return "tx-04-25", "Authority已裁决结果；Client不重新随机", "E16"
        return "I09", "Room进度/机关/计数及纯值查询结果（不把Getter强行变setter）", "E09"
    # 共享资源沿用17的canonical资源合同；操作仍由下方归属覆盖。
    return "I21", owner + "共享资源/层/帧/颜色/向量纯值，owner仍为Room/Grid", "E08/E11/E13"


def generate():
    rows, callbacks, gaps = [], [], []
    grid = json.loads((SOURCE / "grid-catalog.json").read_text(encoding="utf-8-sig"))
    resources = set(grid["shared_resources"]["canonical_classes"])
    for name in ("grid-catalog.json", "room-level-catalog.json", "entity-catalog.json"):
        catalog = json.loads((SOURCE / name).read_text(encoding="utf-8-sig"))
        for ordinal, m in enumerate(catalog["members"]):
            m = {**m, "owner": m.get("owner", m.get("class", ""))}
            if name == "entity-catalog.json" and m["owner"] not in resources: continue
            task, destination, e2e = route(m)
            domain = m.get("domain", "grid" if name == "grid-catalog.json" else "room")
            ownership, reason = classify(m, domain)
            category = m.get("category", "")
            omitted = ownership == "NotReplicated"
            operation = ownership in ("AuthorityOnly", "ClientLocal")
            if category == "cast": ownership, operation, reason = "DerivedWitness", False, "派生接口结果是类型见证，不分配身份，不在Client重放cast调用"
            handling = "仅完整观测保留；复制投影按逐成员清单省略，不能据此补造状态或掩盖来源失败" if omitted else "不作为Client远程调用；最终结果由状态项安装" if operation else "仅已声明消费者需要的参数与结果保留；NotScheduled仅传状态" if category == "parameter_query" else "保留具名纯值/状态/关系，按具体adapter安装"
            implemented = task in ("I03", "I06") or task in ROOM_NATIVE or task in GRID_NATIVE or operation
            consumer = "" if omitted else ROOM_FACT_CONSUMERS.get(m["id"], "tx-04-27 stage UI/audio" if m["owner"] == "RoomConfigStage" else "")
            rows.append({"source_catalog": name, "source_ordinal": ordinal, "id": m["id"], "member": m["member"],
                         "category": category, "parameters": m.get("parameters"), "returns": m.get("returns"),
                         "units": m.get("units", "API原单位"), "source_absence": m.get("absence", "状态显式保留"),
                         "sources": m.get("sources", []), "source_schema": m.get("schema"), "source_version": m.get("schema_version", 2),
                         "target": m.get("target", "REPENTOGON 1.1.3 / J273"), "ownership": ownership, "ownership_reason": reason,
                         "consumer_review": consumer_review(m["id"]),
                         "application_task": task, "handling": handling, "local_storage": "observation_only;not_on_wire" if omitted else destination,
                         "source_path": f"components/{'{room|grid}' if name == 'entity-catalog.json' else domain}/state/**/fields/{m['id']}",
                         "dependencies": ["same Scope/component capture", "complete membership before deletion", "I05 plan;I23 retirement"],
                         "missing": "成员按目录不发送；来源coverage/失败原因不变，不能将省略解释为成功读取" if omitted else "保留Null/Empty/Unknown/Unavailable/Partial/Truncated；required缺失则字段级拒绝，不补零",
                         "implementation": "mod/src/replica/model.cpp:Registry::ProjectValue" if omitted else NATIVE if task == "I06" else ROOM_NATIVE[task] if task in ROOM_NATIVE and not operation else GRID_NATIVE[task] if task in GRID_NATIVE and not operation else PARSER if implemented else f"{PARSER}（保留来源）；{task}原生adapter待实现",
                         "status": "not_replicated_model_v2;runtime_regression_pending_tx19" if omitted else "retained_fact;consumer="+consumer if consumer and not operation else "implemented_grid_adapter;scene_integrated_I25;runtime_representative_I29_pass" if task in GRID_NATIVE and not operation else "implemented_room_adapter;runtime_representative_I29_pass" if task in ROOM_NATIVE and not operation else "implemented_foundation;runtime_representative_I29_pass" if implemented else "native_pending_"+task,
                         "consumer": consumer,
                         "e2e": e2e})
        if name != "entity-catalog.json":
            for ordinal, c in enumerate(catalog.get("callbacks", [])):
                render = "RENDER" in c["signature"]
                callbacks.append({"source_catalog": name, "source_ordinal": ordinal, **c,
                                  "ownership": "IndependentPresentationSource" if render else "AuthorityOnly",
                                  "handling": "保留独立scope，不拼成普通快照；Client不重发玩法回调", "application_task": "I22", "implementation": PARSER+"（普通目标不接独立回调）；I22 ClientGridEvents已接入；独立效果消费/音画归27，不由普通状态冒认完成", "e2e": "E12/E16"})
            for gap in catalog.get("unexposed", []):
                gaps.append({**gap, "source_catalog": name, "application_task": "I11", "handling": "正式RoomPresentation已提供采用的真实环境缓存；其余私有容器仍Unavailable，不补造", "e2e": "E08/E13"})
    structures = [
        ("grid.physical_generation", "I14", "统一清单追加GridCtor=19；直接墙构造正常返回换新物理代次，工厂内去重，不伪造派生Init或外层出生；原生槽与退休身份不一致则来源Partial"),
        ("room.application_source.descriptor_placement", "I06", "原生数组存储/实际offset占用；地址只在主线程转换为值"),
        ("reference.construction_revision", "I03", "保留descriptor当前构建修订；不是出生"),
        ("grid.membership/objects.memberships", "I14", "一份主体，多格子/门别名；完整集合才规划删除"),
        ("grid.lifted_descriptors", "I19", "Effect136所有，不推断来源Grid身份"),
        ("GridEntityPressurePlate.TimerPlate.Null", "I21", "合法Null沿用read.Value；不可把nil当空路径Sprite"),
        ("grid.member_sprite_pose", "I21", "来源v5：Door用REP GetExtraSprite指针Getter，TimerPlate读真实内嵌成员；原版ANM2值拷贝会清owner Offset/render flags，不能作为实际绘制姿态"),
        ("private_grid_bridge_parent/neighbors", "I19/I20", "J273 MakeBridge不保留parent指针；最终State/HasLadder是状态输入，邻居/路径维护仍归I20"),
        ("grid.application_runtime/application_sprites", "I15/I17", "spawnFrame、Door动画模式、Rock碎屑/Fire三Sprite与有界Ember，正式只读来源；不调用Init/PostInit"),
        ("room.map_sprites", "I13/I21", "Minimap两份原生ANM2语义pose，Refresh后安装；不复制owner或用IconsSprite代替边框"),
        ("room.backdrop_wall_trace", "I10/I24", "来源v8记录真实墙面构建/不适用见证和最多4096项独立RNG轨迹；Client保持墙纹采样，省去915890/两处variant7/917420尾部null标记实体生成，不推进共享MT"),
        ("room.application_runtime.render_flag_7220", "I09", "J273仅采用已核定byte绘制条件；不写SDK误标slowdown的未知高位"),
        ("descriptor.grid_index/raw_grid_index_adopted", "I06", "raw保留；dim1 1x1的native原点取唯一真实占用格，缺口通过聚合状态显式报告"),
        ("room.map_application/descriptor.map_icons", "I13", "Camera数值、地图config、DisplayFlags与保存记录图标投影；不替代24完整保存/探索"),
        ("resource_pose/pose_runtime", "I21", "17正式资源姿态与内容版本；不创建第二套Grid身份"),
        ("room.config.application_config", "I07", "独立可空完整配置及MergedData树；只读复用或自有深副本，不执行候选"),
        ("room.application_runtime", "I08/I09/I12", "真实数组尺寸/边界/12形矩形、进度、448字节轨道；不经截断Lua Getter"),
        ("room.presentation_runtime.auxiliary", "I11", "poop最终pose/Image及vein索引图；不传指针、不调用随机构建"),
        ("room_grid.target_budget", "I26", "完整目标32MiB/64M节点（用户要求先放宽，压缩优化后另行收敛）、456主槽及门别名、4096资源/65536层/32768FX；单T、16借用/retained owner及每步一格"),
    ]
    return {"format": "isaac.cs.room-grid.application-catalog", "version": 2, "model_version": 2,
            "members": rows, "callbacks": callbacks, "gaps": gaps,
            "interfaces": grid.get("classes", []),
            "associated": [{**row, "application_task": "I19", "missing": "未解析Entity owner则拒绝依赖；不推断source Grid", "e2e": "E11"} for row in grid.get("associated", [])],
            "structural": [{"id": key, "application_task": task, "handling": text, "implementation": PARSER+";"+PLAN, "missing": "required缺失写前拒绝", "e2e": "E01/E03/E11/E13"} for key, task, text in structures],
            "native_scope": "I01—I30已完成；统一E2E为49个真实Source目标、49个冷Client目标及12项检查，代表画面已主审；不外推所有内容组合、Entity19/Player20/HUD27或实时整局联机"}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=pathlib.Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    data = generate()
    (args.out / "room-grid-application.json").write_text(json.dumps(data, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    text = json.dumps(data, ensure_ascii=False, separators=(",", ":"))
    (args.out / "room_grid_application.inc").write_text(string_expression(text, "ICSA"), encoding="utf-8")


if __name__ == "__main__":
    main()
