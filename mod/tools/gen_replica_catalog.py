"""从09—14实际目录生成逐成员复制归属；不运行游戏、测试或文件摘要。"""
import argparse
import json
import pathlib
import re
from native_entity_contract import application as native_entity_application, PROPERTIES, SETTERS, READONLY

ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE = ROOT / "mod/observation/isaac-cs-observation"
TASKS = {"entity": [19, 29], "grid": [18, 29], "room": [18, 23, 26],
         "level": [24, 26], "exploration": [24], "room_save": [23, 24, 31],
         "player": [20, 28], "run": [25, 28, 31], "random": [25, 29],
         "presentation": [27], "mod": [27, 29]}


def classify(m, domain):
    # 操作和裁决只属于房主；读取得到的最终世界／显示结果另由状态项负责。
    owner = m.get("owner", m.get("class", ""))
    member = m.get("member", "")
    category = m.get("category", "")
    if m.get("id") in ("Entity.Index", "Entity.GetHitListIndex") or (
            owner.startswith("Entity") and member == "GetHitList"):
        return "ClientLocal", "原生列表／碰撞池索引只属于本端；不把Authority槽号写入本地池或命中集合"
    if owner in ("Input", "Keyboard", "Mouse", "Options", "ImGui"):
        return "ClientLocal", "本地窗口／输入／设置只影响该端，不能覆写Authority世界"
    if category in ("operation", "cast", "container_operator"):
        return "AuthorityOnly", "原生写入／裁决／转换不是Replica RPC；复制其最终状态"
    if domain == "random" or owner in ("RNG", "MTRNG", "LevelGenerator") or "RNG" in member or "Random" in member:
        return "AuthorityOnly", "玩法随机状态／流／生成器由唯一Authority执行，结果由世界状态负责"
    if category == "actual_call":
        if owner in ("SFXManager", "MusicManager"):
            return "PresentationEvent", "瞬时音画为已裁决结果，去重／过期；持续播放状态另入Baseline"
        return "AuthorityOnly", "实际调用是裁决历史，不能重放玩法；持久结果由对应状态负责"
    if category == "canonical_reference":
        return "ReplicatedState", "引用同窗口规范Entity／Player状态，不复制第二份身体"
    return "ReplicatedState", "公开纯值／关联／配置／姿态由显式域状态复制，缺口保留原状态"


def generate():
    rows, callbacks, gaps = [], [], []
    for path in sorted(SOURCE.glob("*catalog.json")):
        catalog = json.loads(path.read_text(encoding="utf-8-sig"))
        for m in catalog.get("members", []):
            domain = m.get("domain") or m.get("schema", "isaac.cs.entity").split(".")[2]
            ownership, reason = classify(m, domain)
            row = {"source_catalog": path.name, "id": m["id"], "domain": domain,
                         "category": m.get("category", ""), "ownership": ownership,
                         "reason": reason, "application_tasks": TASKS[domain],
                         "availability": "observed_status_required;catalog_is_not_runtime_support",
                         "returns": m.get("returns", ""), "parameters": m.get("parameters"),
                         "units": m.get("units", "API原单位"), "source_schema": m.get("schema", ""),
                         "source_version": m.get("schema_version", m.get("version", 2))}
            native = native_entity_application(m, ownership) if path.name == "entity-catalog.json" else None
            if native is not None:
                row["native_application"] = native
            rows.append(row)
        for i, callback in enumerate(catalog.get("callbacks", [])):
            signature = callback.get("signature", str(callback)) if isinstance(callback, dict) else str(callback)
            # 回调本身不是通用远程命令；render只提供独立来源的展示状态。
            callbacks.append({"source_catalog": path.name, "ordinal": i, "signature": signature,
                              "ownership": "ReplicatedState" if "RENDER" in signature else "AuthorityOnly",
                              "reason": "独立Render姿态保留scope" if "RENDER" in signature else "不在Client重发玩法回调；最终结果由状态负责",
                              "application_tasks": [17, 27] if "RENDER" in signature else [19, 25, 29]})
        if catalog.get("unexposed"):
            gaps.append({"source_catalog": path.name, "description": catalog["unexposed"],
                         "status": "Unavailable", "application_tasks": [17, 29, 31]})
    return {"format": "isaac.cs.replication.catalog", "version": 1, "members": rows,
            "callbacks": callbacks, "gaps": gaps,
            "structural_values": {"ownership": "ReplicatedState", "reason": "域schema结构值继承所在component合同；身份经Registry转换；随机域仅声明房主所有权",
                                  "application_tasks": TASKS},
            "resources": {"fields": ["logical_path", "content_version", "category", "layers", "pose", "status"],
                          "content_version": "explicit installation/mod version;no file digest",
                          "application_tasks": [17, 18, 19, 20, 27, 29]},
            "runtime_support": "native-room-grid:tx-04-18;native-nonplayer-state:tx-04-19-R01-R05/unified-e2e-pending;minimal-player-owner;full-player:20;world-network:21"}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", type=pathlib.Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    catalog = generate()
    text = json.dumps(catalog, ensure_ascii=False, separators=(",", ":"))
    (args.out / "replication-catalog.json").write_text(text, encoding="utf-8")
    # C++原始字符串嵌入产品目录，正式adapter不依赖磁盘录制或测试包。
    chunks = [text[i:i+8000] for i in range(0, len(text), 8000)]
    (args.out / "replication_catalog.inc").write_text('std::string{}' + ''.join('+std::string(R"ICSR(' + part + ')ICSR")' for part in chunks), encoding="utf-8")
    # 同一白名单同时驱动产品目录与主线程Lua恢复，避免声明能力与执行成员发生漂移。
    def lua(value):
        if isinstance(value, str):
            return json.dumps(value, ensure_ascii=True)
        if isinstance(value, list):
            return "{" + ",".join(lua(v) for v in value) + "}"
        return "{" + ",".join("[" + lua(k) + "]=" + lua(v) for k, v in sorted(value.items())) + "}"
    (args.out / "native_entity_rules.inc").write_text(
        'R"cs_rules(' + lua({"properties": PROPERTIES, "setters": SETTERS,"readonly":READONLY}) + ')cs_rules"', encoding="utf-8")


if __name__ == "__main__":
    main()
