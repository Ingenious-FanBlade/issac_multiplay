"""tx-01-87: corrected frozen evidence plus one actual Continue/teardown scene."""
from __future__ import annotations

import shutil
from .artifacts import load,rows,save
from .completion import CompletionSuite
from .completion_evidence import primary_proof
from .instrumented_evidence import evaluate
from .runner import ROOT


class FinalWindowsSuite(CompletionSuite):
    plan_stages = ("continue-window",)
    plan_faults = ()
    plan_changes = "Correct optional target/reason/Spawn-return decoding of saved evidence; actual main-menu Continue and run teardown; unchanged Native/Mod"
    def __init__(self,args):
        super().__init__(args)
        self.ledger.transaction = "tx-01-87"
        self.ledger.scope = "Correct schema/Spawn/reason decoding of preserved tx-01-86 evidence; only new Continue-loading and run-teardown scene; prior failures unchanged"
        self.completed = ROOT / "artifacts/cs-t0/tx-01-86-remaining-e2e-01"

    def prepare(self):
        super().prepare()
        for name in ("cs-t0/accept-world-final-windows.py","cs-t0/world_e2e/final_windows.py",
            "docs/refactor/cs-t0/t0-06/tx-01-86-e2e-results.md","docs/refactor/cs-t0/t0-06/tx-01-87-final-windows.md"):
            destination=self.root / "sources" / name
            destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(ROOT / name,destination)
        for name in ("result.json","finalization.json","finalization-diagnostics.json"):
            shutil.copyfile(self.completed / name,self.root / "historical-results" / ("tx-01-86-"+name))
        save(self.root / "final-windows-plan.json",{"transaction":"tx-01-87","frozen_raw_source":str(self.completed),
            "new_stage":"continue-window","initial_A_B":"ABSENT_IN_MAIN_MENU; user Continue only after installation",
            "sequence":["install in main menu","human Continue","actual normal A/B samples","human return to main menu","normal stop"],
            "new_faults":[],"new_fixture_world_actions":[],"native_and_mod":"Unchanged tx-01-86/v1.3",
            "schema_fixes":["optional reference.target","BUDGET_EXCEEDED label","Spawn request seed -> actual normal return pointer -> birth"],
            "old_reports_modified":False,"automatic_retry":False})

    def handle_operation(self,role,operation):
        if operation.startswith(("fault ","ready ")) or operation == "pause-check":
            raise ValueError("This continuation has no new fault or repeated pause cases")
        return super().handle_operation(role,operation)

    def run_completion(self):
        self.stage("continue-window",[
            "Fresh game process in main menu with the authorized seeded run available to Continue. Do NOT enter the run before installation.",
            "After controller/observer installation, user manually chooses Continue. Actual AlternateStateApply and owner return must supply Loading/LoadingExit.",
            "Wait for A, single B Join and normal samples; no fixture scene, fault, Esau, GridFree or repeated pause.",
            "User returns to main menu within this capture; observe actual run retirement and immediately stop normally."])
        if "continue-window" not in self.captures: return
        current=self.captures["continue-window"]
        if not current.get("same_process_after"): raise RuntimeError("Current game process identity changed")
        evaluation=evaluate(current["directory"],self.root / "continue-evidence")
        self.evaluations["continue"] = evaluation
        old_eval=load(self.completed / "remaining-evidence/evaluation.json")
        old_capture=self.completed / "captures/remaining/capture"
        old_proof=primary_proof(old_capture,self.root / "preserved-primary-proof",old_eval,
            load(self.completed / "captures/remaining/fixture/final.json"),load(self.completed / "pause-observation.json"))
        new_proof=primary_proof(current["directory"],self.root / "continue-window-proof",evaluation,
            load(self.root / "captures/continue-window/fixture/final.json"),None)
        self.proof={"preserved_primary":old_proof,"new_continue":new_proof}
        self.final_checks(old_eval,evaluation,old_proof,new_proof)

    def final_checks(self,old_eval,evaluation,old_proof,new_proof):
        old=load(self.prior / "primary-evidence/evaluation.json")
        replace=load(self.retained / "trigger-evidence/evaluation.json")
        grid=load(self.grid / "new-gridfree-evidence/evaluation.json")
        scenes_ok=load(self.grid / "result.json")["status"] == "PASS"
        safe=all(v["trace_integrity"] and not v["errors"] for v in (old_eval,evaluation))
        safe = safe and not old_proof["fixture_errors"] and not new_proof["fixture_errors"]
        coverage=sorted(set().union(*(set(v["hooks_covered"]) for v in (old,replace,grid))))
        native_ok=coverage == list(range(19)) and all(v["trace_integrity"] and not v["errors"] for v in (old,replace,grid))
        windows_ok=(old_proof["window_pairs_ok"] and new_proof["window_pairs_ok"]
            and old_proof["pause"]["tick_and_capture_stopped"] and old_proof["blocked_counts"].get("pause",0)
            and old_proof["window_counts"].get("ODD_PLAYER_ONLY",0) and new_proof["loading_observed"])
        repeat_ok=any(p["count"] >= 2 and p["births"] == 1 for p in replace["repeat_publish"])
        sources=[self.root / "preserved-primary-proof",self.root / "continue-window-proof",self.completed,self.prior,self.retained,self.grid]
        save(self.root / "cumulative-native-coverage.json",{"hooks":coverage,"single_new_build_all_19_claimed":False,
            "sources":[str(self.prior),str(self.retained),str(self.grid)],"old_failures_unchanged":True})
        checks={
            "W01.native_results":(safe and native_ok,"Cumulative actual 19 entry/original/return ABI witnesses across cited builds; current subset also healthy; not a single-new-build all-19 claim"),
            "W02.blocked_windows":(safe and windows_ok,"Same-token/attempt pairing for every saved old/new gap; actual prior pause/odd/room and new AlternateStateApply loading; no fabricated UI/load label"),
            "W03.retained_unknown":(safe and scenes_ok and old["unknown"] and old_proof["reference_ok"],"Saved real retained Player/unknown class and nonempty KNOWN_CURRENT -> AMBIGUOUS_REUSE -> recovery; omitted target stays unknown"),
            "W04.identity_history":(safe and old["nested_spawn"] and old["reuse_groups"] and not old["duplicate_births"] and repeat_ok and old_proof["short_ok"],
                "Real Spawn seed/normal return/birth association completes saved short Pickup Remove/ClearReferences/membership retirement; nested/reuse/repeated-publish history cited without rerun"),
            "W05.transitions":(safe and scenes_ok and old_proof["rooms_preserve"] and new_proof["teardown_ok"]
                and old["lifecycle_hooks"].get("NpcMorph:True",0) and old["lifecycle_hooks"].get("PickupMorph:True",0)
                and old["lifecycle_hooks"].get("PlayerRestore:True",0),
                "Historical Morph/Restore/Replace/GridFree and same-process room identity, plus newly observed run teardown with no post-retirement capture; Restore UNKNOWN retained"),
        }
        for key,(passed,reason) in checks.items():
            self.ledger.items[key]["method"]="PRESERVED_RAW_EVIDENCE_PLUS_ACTUAL_CONTINUE_E2E"
            self.ledger.require(key,bool(passed),reason,sources)
        self.faults=[load(self.completed / ("fault-"+name+".json")) for name in ("object-budget","reference-ambiguity","event-overflow")]
        historic={name:load(self.prior / ("fault-"+name+".json"))["scheduler_continues"] for name in
            ("membership-budget","relation-budget","field-read","source-changed","wrong-thread","root-changed")}
        backpressure=ROOT / "artifacts/cs-t0/tx-01-79-live-remaining-01/captures/primary/external/reader-pause-outcome.json"
        backpressure_ok=load(backpressure)["observed_backpressure_and_tick_progress"]
        budget=[]
        for offset,_,row in rows(self.completed / "captures/remaining/capture/world/captures.jsonl"):
            if row["reason"] == "BUDGET_EXCEEDED": budget.append({"offset":offset,"attempt":row["attempt_seq"],"complete":row["complete"]})
        delivery,markers=[],[]
        terminal=self.completed / "captures/fault-event-overflow/capture"
        for offset,_,row in rows(terminal / "e2e/trace.jsonl"):
            if row["kind"] == 21: delivery.append({"offset":offset,"event":row})
            if row["kind"] == 13 and row["tag"] == 6: markers.append({"offset":offset,"event":row})
        terminal_eval=load(self.completed / "event-overflow-evidence/evaluation.json")
        fault_ok=(safe and all(historic.values()) and backpressure_ok and all(f["scheduler_continues"] for f in self.faults)
            and old_proof["reference_ok"] and budget and all(not r["complete"] for r in budget)
            and len(delivery) == len(markers) == 1 and terminal_eval["trace_integrity"] and not terminal_eval["errors"])
        save(self.root / "fault-completion.json",{"historical":historic,"backpressure_source":str(backpressure),"backpressure_ok":backpressure_ok,
            "preserved_new_faults":self.faults,"budget_records":budget,"reference_proof":old_proof["reference_ok"],
            "diagnostic_delivery":delivery,"capacity_fault_markers":markers,"new_fault_commands_this_invocation":[],
            "original_FAIL_records_unchanged":True})
        self.ledger.items["W12.fault_isolation"]["method"]="CORRECTED_SAVED_BRANCH_EVIDENCE_NO_FAULT_RERUN"
        self.ledger.require("W12.fault_isolation",bool(fault_ok),"Actual saved nine controlled branches and backpressure with independent scheduler evidence; corrected production reason/optional reference schema; no faults rerun",[self.root / "fault-completion.json",*sources])
