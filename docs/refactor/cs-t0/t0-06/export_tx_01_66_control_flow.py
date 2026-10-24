"""Supplement normal-path call cardinality and the corrected transition target.

Only reads the open IDB; preserves both earlier evidence files. Execute using
IDA MCP py_exec_file. This is static graph/data extraction, not game execution.
"""

import json
import runpy
from pathlib import Path

import ida_funcs
import ida_gdl
import ida_hexrays
import ida_lines
import idaapi
import idautils


def export(output_file):
    helpers = runpy.run_path(str(Path(__file__).with_name("export_tx_01_66_review.py")),
                             run_name="tx_01_66_control_flow_helpers")
    instructions = helpers["instructions"]
    flow = ida_gdl.FlowChart(ida_funcs.get_func(0x954CD0))
    blocks = {block.start_ea: block for block in flow}
    calls = {}
    graph = []
    for block in flow:
        for call in (0x955231, 0x955241):
            if block.start_ea <= call < block.end_ea:
                calls[call] = block.start_ea
        graph.append({"startVa": hex(block.start_ea), "endVaExclusive": hex(block.end_ea),
                      "successors": [hex(succ.start_ea) for succ in block.succs()]})
    summaries = []
    for call, block in calls.items():
        seen = set()
        pending = [succ.start_ea for succ in blocks[block].succs()]
        while pending:
            current = pending.pop()
            if current in seen:
                continue
            seen.add(current)
            pending.extend(succ.start_ea for succ in blocks[current].succs())
        summaries.append({"callVa": hex(call), "blockVa": hex(block),
                          "canReturnToOwnBlock": block in seen,
                          "otherSimulationCallBlocksReachable": [hex(other) for key, other in calls.items()
                                                                 if key != call and other in seen]})
    target = 0x8318A0
    func = ida_funcs.get_func(target)
    raw = []
    for begin, end in idautils.Chunks(func.start_ea):
        raw.extend(instructions(begin, end))
    result = {
        "tx": "tx-01-66", "kind": "STATIC_NORMAL_CONTROL_FLOW_AND_TRANSITION_TARGET",
        "imageBase": hex(idaapi.get_imagebase()),
        "driver": {"functionVa": "0x954cd0", "simulationCallSummaries": summaries, "blocks": graph},
        "correctedTransitionTarget": {
            "callsiteVa": "0x6fb51c", "targetVa": hex(target), "returnVa": "0x6fb521",
            "pseudocode": ida_lines.tag_remove(str(ida_hexrays.decompile(target))),
            "instructions": raw,
            "note": "Original export selected 6FE2F0 but the actual 6FB51C call target is 8318A0. Future exporter includes the correct target; original JSON is preserved."},
        "callsiteContext": instructions(0x6FB4E1, 0x6FB526),
        "limitations": ["Normal recovered CFG excludes exceptional/unresolved indirect reentry.",
                        "At-most-one ordinary branch call is not proof against arbitrary Mod reentry; observation tokens must track nesting/counts.",
                        "No game, debugger, test, build, hash computation or IDB mutation."]}
    with Path(output_file).open("x", encoding="utf-8", newline="\n") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)
        handle.write("\n")
    print(json.dumps({"output": str(output_file), "functionCount": 1,
                      "driverGraphBlocks": len(graph), "status": "EXPORTED_STATIC_EVIDENCE"}))


if __name__ == "__main__":
    repository = Path(__file__).resolve().parents[4]
    export(repository / "artifacts" / "cs-t0" / "tx-01-66-static" / "control-flow-evidence.json")
