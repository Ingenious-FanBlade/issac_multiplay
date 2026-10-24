"""tx-01-87 unified E2E: real Continue/teardown and corrected saved completion evidence."""
from __future__ import annotations

import argparse
import os
import sys

from world_e2e.artifacts import save,utc
from world_e2e.completion import TARGETS
from world_e2e.final_windows import FinalWindowsSuite
from world_e2e.runner import ROOT


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ("game-exe","controller","native-dll","helper","artifact-dir","authorization","game-log"):
        parser.add_argument("--"+name,required=True)
    parser.add_argument("--powershell",default="C:\\Program Files\\PowerShell\\7\\pwsh.exe")
    args=parser.parse_args()
    if os.name != "nt" or not sys.stdin.isatty(): parser.error("Windows interactive terminal required")
    if not args.authorization.strip(): parser.error("Record actual authorized operation scope")
    args.plan=str(ROOT / "cs-t0/testdata/tx-01-74-world-record-plan.json")
    args.b_character,args.instrumented_remaining="native",True
    suite=FinalWindowsSuite(args)
    failure,interrupted,cleanup_errors=None,False,[]
    result={"status":"FAIL"}
    scoped={"status":"FAIL"}
    try:
        suite.prepare()
        suite.run_completion()
    except KeyboardInterrupt: interrupted=True
    except Exception as exc: failure=f"{type(exc).__name__}: {exc}"
    finally:
        for label,operation in (("controller",suite.interrupted_cleanup),("summary",suite.finalize_evidence),("observers",suite.close_observers)):
            try: operation()
            except (Exception,KeyboardInterrupt) as exc: cleanup_errors.append(f"{label}: {type(exc).__name__}: {exc}")
        failure="; ".join(([failure] if failure else [])+suite.command_failures+cleanup_errors) or None
        save(suite.root / "finalization-diagnostics.json",{"errors":cleanup_errors,"original_error":failure,
            "evidence_summary_completed":suite.evidence_finalization_complete,"automatic_retry":False})
        result=suite.ledger.export(failure,interrupted)
        states={key:suite.ledger.items[key]["status"] for key in TARGETS}
        scoped={"transaction":"tx-01-87","status":"PASS" if not failure and not interrupted and all(v=="PASS" for v in states.values()) else "INCOMPLETE",
            "checks":states,"excluded":["W08.replay"],"global_ledger":"result.json",
            "prior_FAIL_records_unchanged":True,"automatic_retry":False}
        save(suite.root / "authorized-scope-result.json",scoped)
        suite.inventory()
        save(suite.root / "finalization.json",{"status":"COMPLETED","result_status":result["status"],"scope_status":scoped["status"],"at_utc":utc(),"automatic_retry":False})
    print("Scope "+scoped["status"]+"; global "+result["status"]+": "+str(suite.root / "result.json"),flush=True)
    return 130 if interrupted else 0 if scoped["status"]=="PASS" else 1 if failure or result["status"]=="FAIL" else 2


if __name__ == "__main__":
    raise SystemExit(main())
