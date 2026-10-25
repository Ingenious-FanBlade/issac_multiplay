"""只读追踪已定位 owner 到持久写入/回调函数的有限调用路径，不运行游戏。"""

import json
from collections import deque
from pathlib import Path

import ida_funcs
import ida_nalt
import ida_ua
import idautils
import idc


def direct_calls(rva):
    """仅记录实际 call 指令的静态目标；虚调用缺口显式保留。"""
    function = ida_funcs.get_func(base+rva)
    if not function:
        return [], []
    edges, indirect = [], []
    for ea in idautils.FuncItems(function.start_ea):
        if idc.print_insn_mnem(ea) != "call":
            continue
        targets = list(idautils.CodeRefsFrom(ea, False))
        if not targets:
            indirect.append(hex(ea-base))
        for target in targets:
            target_function = ida_funcs.get_func(target)
            if target_function and base <= target_function.start_ea < base+0x6B0000:
                edges.append({"site": hex(ea-base), "target": hex(target_function.start_ea-base)})
    return edges, indirect


# 本图只收录直接边；工厂的虚拟 Init 边须由原始指令另行人工复核。
base = ida_nalt.get_imagebase()
roots = [0x590250, 0x590560, 0x591360, 0x34F6A0]
sinks = {0x503930: "TryUnlock", 0x503A50: "IncreaseEventCounter",
         0x503460: "SaveLocally", 0x502F00: "SaveToSteamCloud", 0x44A7E0: "PostPlayerInit"}
graph = {}
queue = deque((rva, 0) for rva in roots)
max_depth, max_nodes = 7, 2200
while queue and len(graph) < max_nodes:
    rva, depth = queue.popleft()
    if rva in graph:
        continue
    edges, unresolved = direct_calls(rva)
    graph[rva] = {"depth": depth, "calls": edges, "unresolved_calls": unresolved}
    if depth < max_depth and rva < 0x680000 and rva not in sinks:
        queue.extend((int(edge["target"], 0), depth+1) for edge in edges)


def find_path(root, sink):
    """返回有界图中一条最短路径；未命中不等于不可达或无副作用。"""
    pending = deque([(root, [])])
    visited = set()
    while pending:
        node, path = pending.popleft()
        if node == sink:
            return path
        if node in visited:
            continue
        visited.add(node)
        for edge in graph.get(node, {}).get("calls", []):
            pending.append((int(edge["target"], 0), path+[{"owner": hex(node), **edge}]))
    return None


paths = {hex(root): {name: find_path(root, sink) for sink, name in sinks.items()} for root in roots}
out = Path(r"F:\projects_isaac\issac_multiplay\artifacts\repentogon\tx-04-06-static\st2-st9-primary-callpaths.json")
out.write_text(json.dumps({"imagebase": hex(base), "roots": [hex(r) for r in roots],
                          "max_depth": max_depth, "max_nodes": max_nodes, "nodes": len(graph),
                          "pending_nodes": len(queue), "paths": paths,
                          "graph": {hex(r): data for r, data in graph.items()}}, indent=2), encoding="utf-8")
print(json.dumps({"nodes": len(graph), "pending": len(queue), "paths": paths}))
result = None
