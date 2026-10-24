"""tx-01-85: one GridFree E2E plus corrected review of frozen tx-01-84 Replace evidence."""
from __future__ import annotations

import argparse
import importlib.util
import os
import shutil
import sys
from pathlib import Path

from world_e2e.artifacts import load, save, utc
from world_e2e.instrumented_evidence import evaluate
from world_e2e.runner import ROOT

_spec = importlib.util.spec_from_file_location("world_trigger_scenes", Path(__file__).with_name("accept-world-trigger-scenes.py"))
_scenes = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_scenes)
TriggerLedger, TriggerSuite = _scenes.TriggerLedger, _scenes.TriggerSuite


class GridFreeSuite(TriggerSuite):
    def __init__(self, args):
        super().__init__(args)
        self.prior = Path(args.prior_evidence).resolve()
        if self.prior == self.root or self.prior.is_relative_to(self.root) or self.root.is_relative_to(self.prior):
            raise ValueError("Prior and new outputs must be separate")
        self.ledger = TriggerLedger(self.root)
        self.ledger.transaction = "tx-01-85"
        self.ledger.scope = "Fresh GridFree scene and corrected read-only Replace review of tx-01-84; old FAIL results unchanged"
        self.ledger.items = {k.replace("S84.","S85."): {**v,"id":k.replace("S84.","S85.")}
                             for k,v in self.ledger.items.items()}

    def prepare(self):
        super().prepare()
        for name in ("cs-t0/accept-gridfree-scene.py", "docs/refactor/cs-t0/t0-06/tx-01-85-gridfree-correction.md",
                     "docs/refactor/cs-t0/t0-06/tx-01-84-e2e-results.md"):
            target = self.root / "sources" / name
            target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(ROOT / name,target)
        for name in ("result.json","finalization.json","trigger-evidence/scene-proof.json"):
            target = self.root / "prior-tx-01-84" / name
            target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(self.prior / name,target)
        save(self.root / "gridfree-plan.json", {"transaction":"tx-01-85","prior_evidence":str(self.prior),
            "prior_replace_live_rerun":False,"new_sequence":["enable","gridfree-spawn","gridfree-use","stop"],
            "fixture":"v1.2 observed descriptor baseline","native":"Unchanged tx-01-84",
            "analysis_fixes":["subject=0 only","ActiveItem=2","Accepted=1; Resolve tick matches Submit target",
                "historical birth vs current Restore mapping availability"],"automatic_retry":False})

    def run_scenes(self):
        self.stage("gridfree", [
            "Fresh isolated seeded local run, fixture v1.2 enabled, A present. Wait for B and healthy samples.",
            "Enable fixture. gridfree-spawn once, close console and wait natural frames; gridfree-use once, close console and wait for settled.",
            "No Esau, scene, old grid commands, faults, replay or comparison. Keep the current room and normal stop."])
        if "gridfree" not in self.captures: return
        if not self.captures["gridfree"].get("same_process_after"):
            raise RuntimeError("New process identity changed; restoration not inferred")
        if load(self.prior / "finalization.json")["status"] != "COMPLETED":
            raise ValueError("Prior evidence did not finish")
        previous = self.prior / "captures/triggers/capture"
        current = self.captures["gridfree"]["directory"]
        for role,capture,fixture,allow in (
            ("prior-replace",previous,self.prior / "captures/triggers/fixture/final.json",True),
            ("new-gridfree",current,self.root / "captures/gridfree/fixture/final.json",False)):
            directory = self.root / (role+"-evidence")
            analysis = evaluate(capture,directory)
            self.evaluations[role] = analysis
            proof = self.analyze_scenes(capture,analysis,fixture,allow)
            save(directory / "scene-proof.json",proof)
            suffix = "replace_retained" if allow else "gridfree"
            self.ledger.require("S85."+suffix,proof[suffix]["passed"],proof[suffix]["reason"],
                [directory,capture,fixture])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("game-exe","controller","native-dll","helper","artifact-dir","authorization","game-log","catalogue","prior-evidence"):
        parser.add_argument("--"+option,required=True)
    parser.add_argument("--powershell",default="C:\\Program Files\\PowerShell\\7\\pwsh.exe")
    args = parser.parse_args()
    if os.name != "nt" or not sys.stdin.isatty(): parser.error("Windows interactive terminal required")
    if not args.authorization.strip(): parser.error("Record the actual authorized operation scope")
    args.plan = str(ROOT / "cs-t0/testdata/tx-01-74-world-record-plan.json")
    args.b_character,args.instrumented_remaining = "native",True
    suite = GridFreeSuite(args)
    failure,interrupted,cleanup_errors = None,False,[]
    result = {"status":"FAIL"}
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
        failure = "; ".join(([failure] if failure else [])+suite.command_failures+cleanup_errors) or None
        save(suite.root / "finalization-diagnostics.json", {"errors":cleanup_errors,"original_error":failure,
            "evidence_summary_completed":suite.evidence_finalization_complete,"automatic_retry":False})
        result = suite.ledger.export(failure,interrupted)
        suite.inventory()
        save(suite.root / "finalization.json", {"status":"COMPLETED","result_status":result["status"],"at_utc":utc(),"automatic_retry":False})
    print(result["status"]+": "+str(suite.root / "result.json"),flush=True)
    return 130 if interrupted else 0 if result["status"] == "PASS" else 2 if result["status"] == "INCOMPLETE" else 1


if __name__ == "__main__":
    raise SystemExit(main())
