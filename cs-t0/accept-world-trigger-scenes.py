"""tx-01-84: one live E2E invocation for Replace/retained and GridFree scenes."""
from __future__ import annotations

import argparse
import os
import shutil
import sys
from pathlib import Path

from world_e2e.artifacts import Ledger, append, load, rows, save, utc
from world_e2e.instrumented import InstrumentedSuite
from world_e2e.instrumented_evidence import evaluate
from world_e2e.runner import ROOT, Suite


class TriggerLedger(Ledger):
    def __init__(self, root):
        super().__init__(root)
        self.transaction = "tx-01-84"
        self.scope = "Only native Replace/retained Player and owned-rock D12/GridFree; no prior result is rejudged"
        self.items = {key: {"id":key, "title":title, "status":"NOT_COVERED", "reason":"Not observed",
            "evidence":[], "method":"AUTOMATED_WITH_STATIC_TRIGGER_REVIEW"} for key,title in (
            ("S84.replace_retained", "Three native item uses, actual Replace and preserved retained identity"),
            ("S84.gridfree", "Owned fixture rock passes actual GridFree and retires its slot membership"))}

    def export(self, failure=None, interrupted=False):
        statuses = {row["status"] for row in self.items.values()}
        status = "FAIL" if failure or "FAIL" in statuses else "PASS" if statuses == {"PASS"} else "INCOMPLETE"
        if interrupted: status = "INTERRUPTED"
        case = {"id":next(iter(self.items)).split(".")[0], "status":status, "checks":list(self.items.values())}
        append(self.root / "cases.jsonl", case)
        result = {"transaction":self.transaction,"status":status,"ended_at_utc":utc(),"error":failure,
            "cases":[case],"scope":self.scope,"automatic_retry":False,"hashes":"NOT_USED",
            "required_completion_record":"finalization.json"}
        save(self.root / "result.json", result)
        return result


def key(obj):
    return obj["local_id"], obj["use_generation"]


class TriggerSuite(InstrumentedSuite):
    def __init__(self, args):
        super().__init__(args, None)
        self.ledger = TriggerLedger(self.root)
        self.catalogue = Path(args.catalogue).resolve()

    def prepare(self):
        super().prepare()
        for name in ("cs-t0/accept-world-trigger-scenes.py", "docs/refactor/cs-t0/t0-06/tx-01-84-trigger-methods.md",
                     "artifacts/cs-t0/tx-01-84-static/ida-evidence.json"):
            target = self.root / "sources" / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, target)
        save(self.root / "trigger-plan.json", {"transaction":"tx-01-84", "catalogue_source":str(self.catalogue),
            "prior_result":"artifacts/cs-t0/tx-01-83-instrumented-e2e-01/result.json",
            "sequence":["enable", "esau-equip", "esau-use-1 + physical item press/release",
                "esau-use-2 + physical item press/release", "esau-use-3 + physical item press/release",
                "gridfree-spawn", "gridfree-use", "stop"],
            "no_prior_passes_rerun":True, "no_fault_commands":True, "automatic_retry":False})
        shutil.copyfile(ROOT / "artifacts/cs-t0/tx-01-83-instrumented-e2e-01/result.json",
                        self.root / "prior-result-unchanged.json")

    def handle_operation(self, role, operation):
        if operation.startswith("fault "): raise ValueError("This scene-only invocation has no fault commands")
        return super().handle_operation(role, operation)

    def run_scenes(self):
        self.stage("triggers", [
            "Fresh isolated seeded local run, fixture v1.1 enabled, A exists. Wait for healthy samples and B.",
            "Enable fixture and equip Esau Jr. Arm use-1, close console and physically press/release the normal item key once; wait for native_use_settled. Repeat use-2 and use-3 once each.",
            "Each arm reacquires current A and preserves the native slot VarData. Never call Lua UseActiveItem(703). Native callback alone is not Replace proof.",
            "Then gridfree-spawn in the same empty room; wait a few natural frames, gridfree-use once. D12 refuses any non-fixture rerollable Grid.",
            "Use observe <label>/fixture-log for evidence. Stop after settled logs; do not run other scenes, faults, replay or comparisons."])
        if "triggers" not in self.captures: return
        capture = self.captures["triggers"]["directory"]
        analysis = evaluate(capture, self.root / "trigger-evidence")
        self.evaluations["triggers"] = analysis
        proof = self.analyze_scenes(capture, analysis)
        save(self.root / "trigger-evidence" / "scene-proof.json", proof)
        for suffix in ("replace_retained", "gridfree"):
            self.ledger.require("S84."+suffix, proof[suffix]["passed"], proof[suffix]["reason"],
                [self.root / "trigger-evidence", capture, self.root / "captures/triggers/fixture/final.json"])

    def analyze_scenes(self, capture, analysis, fixture_file=None, allow_prior_grid_refusal=False):
        logs = load(fixture_file or self.root / "captures/triggers/fixture/final.json")
        errors = [row for row in logs if row["kind"] == "fixture_error"]
        blocking_errors = [row for row in errors if not (allow_prior_grid_refusal
            and row.get("action") == "gridfree-use" and "GridFree fixture changed; refusing D12" in row.get("message", ""))]
        arms = [row for row in logs if row["kind"] == "native_use_armed"]
        settled = [row for row in logs if row["kind"] == "native_use_settled"]
        created = [row for row in logs if row["kind"] == "gridfree_fixture_created"]
        grid_settled = [row for row in logs if row["kind"] == "gridfree_settled"]
        pairs, frees = [], []
        for _,_,row in rows(capture / "e2e/trace.jsonl"):
            if row["kind"] == 18 and row["tag"] == 9 and row["subject"] == "0":
                pairs.append({"outer":int(row["outer"]),"source":f"{row['words'][0]:08x}",
                    "target":f"{row['words'][1]:08x}","before":None,"after":None})
            elif row["kind"] == 18 and row["tag"] == 17 and row["subject"] == "0":
                frees.append({"outer":int(row["outer"]),"index":row["words"][0]})
        owned_index = created[0]["grid"]["index"] if len(created) == 1 else None
        owned, grid_after = None, None
        for offset,_,record in rows(capture / "world/captures.jsonl"):
            if record["record"] != "capture" or record["authority_kind"] != 1: continue
            token = int(record["outer_token"])
            players = [obj for obj in record["objects"] if obj["class"] == "Player"]
            for pair in pairs:
                source = next((obj for obj in players if obj["diagnostics"]["address"] == pair["source"]),None)
                target = next((obj for obj in players if obj["diagnostics"]["address"] == pair["target"]),None)
                if token < pair["outer"] and source and source["ordinal"] == 0 and any(m["source"] == 8 for m in source["memberships"]):
                    pair["before"] = {"offset":offset,"key":key(source),"birth_event":source["birth_event"]}
                if token > pair["outer"] and pair["before"] and pair["after"] is None and source and target:
                    memberships = {m["source"] for m in source["memberships"]}
                    if 9 in memberships and 8 not in memberships and target["ordinal"] == 0 and any(m["source"] == 8 for m in target["memberships"]):
                        pair["after"] = {"offset":offset,"source_key":key(source),"target_key":key(target),
                            "source_birth_event":source["birth_event"],"target_birth_event":target["birth_event"]}
            if owned_index is not None:
                for obj in record["objects"]:
                    if obj["class"] != "Grid" or not any(m["source"] == 11 and m["index"] == owned_index for m in obj["memberships"]): continue
                    if owned is None and frees and token < min(free["outer"] for free in frees) and any(
                        f["field_id"] == "0805" and f.get("raw32") is not None
                        and int(f["raw32"],16) == 9730012 for f in obj["fields"]):
                        owned = {"key":key(obj),"address":obj["diagnostics"]["address"],"outer":token,"offset":offset}
                if owned and frees and token > max(free["outer"] for free in frees):
                    still_member = any(key(obj) == owned["key"] and any(m["source"] == 11 and m["index"] == owned_index for m in obj["memberships"]) for obj in record["objects"])
                    if not still_member and grid_after is None: grid_after = {"outer":token,"offset":offset}
        retired = []
        replace_returns, historical_births = [], {}
        for offset,_,event in rows(capture / "world/lifecycle-events.jsonl"):
            if event["local_id"] is not None and event["birth_event"] is not None:
                historical_births.setdefault(key(event),set()).add(event["birth_event"])
            if event["hook"] == "Replace" and not event["entering"]:
                replace_returns.append({"offset":offset,"outer":int(event["outer_token"]),"key":key(event),
                    "birth_event":event["birth_event"],"address":event["diagnostics"]["address"],"result":event["diagnostics"]["result"]})
            if owned and event["hook"] == "GridFree" and event["event_kind"] == 4 and key(event) == owned["key"]:
                retired.append({"offset":offset,"event":event})
        edges = {4:[],5:[]}
        for _,_,event in rows(capture / "events.jsonl"):
            if event["kind"] in edges and event["ordinal"] == 0 and event["state"]["edge_flags"] & 2:
                edges[event["kind"]].append({"tick":event["tick"],"generation":event["generation"],
                    "command":event["command"],"target":event["target"],"decision":event["decision"]})
        def birth_consistent(pair):
            before, after = pair["before"], pair["after"]
            source_births = historical_births.get(before["key"],set())
            target_births = historical_births.get(after["target_key"],set())
            # Restore can revoke the *current mapping*. Do not fill a null in a
            # saved sample; immutable physical birth facts remain in events.
            return (len(source_births) == len(target_births) == 1
                and before["birth_event"] in source_births | {None}
                and after["source_birth_event"] in source_births | {None}
                and after["target_birth_event"] in target_births | {None})
        counts = analysis["counts"]
        native_calls = all(counts.get(f"{kind}:9",0) == 3 for kind in (3,4,5))
        identity = native_calls and len(pairs) == len(replace_returns) == 3 and all(pair["before"] and pair["after"]
            and pair["before"]["key"] == pair["after"]["source_key"]
            and birth_consistent(pair)
            and any(row["outer"] == pair["outer"] and row["key"] == pair["before"]["key"]
                and row["birth_event"] in historical_births.get(row["key"],set())
                and int(row["result"],16) & 0xff == 1 for row in replace_returns) for pair in pairs)
        stable = analysis["trace_integrity"] and not analysis["errors"] and not blocking_errors
        # Submit carries target; Resolve uses tick (its target field is unused).
        # CurrentSequence can be a newer continuous command merged at that tick.
        submitted = {(edge["generation"],edge["target"]) for edge in edges[4] if edge["decision"] == 1}
        resolved = {(edge["generation"],edge["tick"]) for edge in edges[5]}
        replace_ok = stable and identity and len(arms) == len(settled) == 3 and len(submitted & resolved) == 3
        grid_ok = stable and len(created) == len(grid_settled) == 1 and bool(frees) and owned is not None
        grid_ok = grid_ok and all(free["index"] == owned_index for free in frees) and bool(retired) and grid_after is not None
        return {"fixture_errors":errors,"blocking_fixture_errors":blocking_errors,"canonical_item_edges":edges,
            "matched_item_targets_by_generation":sorted(submitted & resolved),"replace_pairs":pairs,"replace_returns":replace_returns,
            "historical_births":[{"key":k,"birth_events":sorted(v)} for k,v in historical_births.items()],
            "restore_scope":"Sample birth_event=null remains UNKNOWN; event history does not establish cross-Restore state equality",
            "grid_calls":frees,"owned_grid":owned,"grid_retired":retired,"grid_after":grid_after,
            "replace_retained":{"passed":bool(replace_ok),"reason":"Require three successful Replace calls, roster->retained with preserved physical ID/use and historical birth plus canonical item edges; current Restore mapping can remain UNKNOWN"},
            "gridfree":{"passed":bool(grid_ok),"reason":"Require actual hook 17 for the owned index, native retirement of the explicitly seeded old Grid, later slot membership removal and healthy trace/restoration"}}

    def record_completed_results(self):
        save(self.root / "trigger-summary.json", {"checks":self.ledger.items,"evaluations":self.evaluations,
            "scope":self.ledger.scope,"prior_global_fail_preserved":True,"automatic_retry":False})

    def inventory(self):
        Suite.inventory(self)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("game-exe","controller","native-dll","helper","artifact-dir","authorization","game-log","catalogue"):
        parser.add_argument("--"+option, required=True)
    parser.add_argument("--powershell", default="C:\\Program Files\\PowerShell\\7\\pwsh.exe")
    args = parser.parse_args()
    if os.name != "nt" or not sys.stdin.isatty(): parser.error("Windows interactive terminal required")
    if not args.authorization.strip(): parser.error("Record the actual authorized operation scope")
    args.plan = str(ROOT / "cs-t0/testdata/tx-01-74-world-record-plan.json")
    args.b_character, args.instrumented_remaining = "native", True
    suite = TriggerSuite(args)
    failure, interrupted = None, False
    result = {"status":"FAIL"}
    cleanup_errors = []
    try:
        suite.prepare()
        suite.run_scenes()
    except KeyboardInterrupt:
        interrupted = True
    except Exception as exc:
        failure = f"{type(exc).__name__}: {exc}"
    finally:
        for label,operation in (("controller",suite.interrupted_cleanup),("summary",suite.finalize_evidence),("observers",suite.close_observers)):
            try: operation()
            except (Exception,KeyboardInterrupt) as exc: cleanup_errors.append(f"{label}: {type(exc).__name__}: {exc}")
        failure = "; ".join(([failure] if failure else []) + suite.command_failures + cleanup_errors) or None
        save(suite.root / "finalization-diagnostics.json", {"errors":cleanup_errors,"original_error":failure,
            "evidence_summary_completed":suite.evidence_finalization_complete,"automatic_retry":False})
        result = suite.ledger.export(failure, interrupted)
        suite.inventory()
        save(suite.root / "finalization.json", {"status":"COMPLETED","result_status":result["status"],"at_utc":utc(),"automatic_retry":False})
    print(result["status"]+": "+str(suite.root / "result.json"),flush=True)
    return 130 if interrupted else 0 if result["status"] == "PASS" else 2 if result["status"] == "INCOMPLETE" else 1


if __name__ == "__main__":
    raise SystemExit(main())
