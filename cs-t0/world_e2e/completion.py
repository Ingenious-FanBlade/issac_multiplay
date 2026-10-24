"""tx-01-86 remaining-scope orchestration; human checkpoints, one invocation."""
from __future__ import annotations

import shutil
import time
from pathlib import Path

from .artifacts import append,load,rows,save,utc
from .completion_evidence import latest_reference,primary_proof
from .instrumented import FAULTS,InstrumentedSuite
from .instrumented_evidence import evaluate
from .runner import ROOT,Suite

TARGETS = ("W01.native_results","W02.blocked_windows","W03.retained_unknown",
    "W04.identity_history","W05.transitions","W12.fault_isolation")


class CompletionSuite(InstrumentedSuite):
    plan_stages = ("remaining","fault-event-overflow")
    plan_faults = ("object-budget","reference-ambiguity","event-overflow")
    plan_changes = "E2E Window/ReferenceFault/DiagnosticDelivery; actual ready premise; short Pickup and public SpawnerEntity fixture"
    def __init__(self,args):
        super().__init__(args,None)
        self.ledger.transaction = "tx-01-86"
        self.ledger.scope = "Complete six remaining authorized checks; 25 historical PASS retained with provenance; this legacy entry does not run replay"
        self.prior = ROOT / "artifacts/cs-t0/tx-01-83-instrumented-e2e-01"
        self.retained = ROOT / "artifacts/cs-t0/tx-01-84-trigger-e2e-01"
        self.grid = ROOT / "artifacts/cs-t0/tx-01-85-gridfree-e2e-01"
        self.catalogue = self.prior / "captures/primary/capture/restoration.json"
        self.pause = None
        self.proof = None
        self.readiness_count = 0

    def prepare(self):
        super().prepare()
        for name in ("cs-t0/accept-world-remaining-completion.py","cs-t0/world_e2e/completion.py",
            "cs-t0/world_e2e/completion_evidence.py","cs-t0/native/include/isaac_cs_t0/coop_active_runtime.hpp",
            "cs-t0/native/src/coop_active_runtime.cpp","docs/refactor/cs-t0/t0-06/tx-01-86-remaining-completion.md",
            "artifacts/cs-t0/tx-01-86-static/ida-evidence.json"):
            destination = self.root / "sources" / name
            destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(ROOT / name,destination)
        prior_copy = self.root / "historical-results"
        prior_copy.mkdir()
        for label,origin in (("tx-01-83",self.prior),("tx-01-84",self.retained),("tx-01-85",self.grid)):
            for name in ("result.json","finalization.json"):
                shutil.copyfile(origin / name,prior_copy / (label+"-"+name))
        for case in load(self.prior / "result.json")["cases"]:
            for item in case["checks"]:
                if item["status"] != "PASS" or item["id"] not in self.ledger.items: continue
                self.ledger.items[item["id"]]["method"] = "CARRIED_FORWARD"
                self.ledger.record(item["id"],"PASS","Historical tx-01-83 or its cited predecessor, not rerun on new Native: "+item["reason"],item["evidence"])
        for key in ("W08.replay",):
            self.ledger.record(key,"NOT_COVERED","USER_EXCLUDED; prior conversation preference remains in effect",[prior_copy])
        save(self.root / "completion-plan.json",{"transaction":self.ledger.transaction,"targets":list(TARGETS),
            "historical_roots":[str(self.prior),str(self.retained),str(self.grid)],
            "new_live_stages":list(self.plan_stages),
            "new_faults":list(self.plan_faults),
            "changes":self.plan_changes,
            "prior_failures_preserved":True,"automatic_retry":False,"game_log":str(self.log)})

    def healthy_premise(self,role,name):
        before = self.observers[role].snapshot()
        time.sleep(.75)
        after = self.observers[role].snapshot()
        fresh = latest_reference(self.root / "captures" / role / "capture/world/captures.jsonl") if name == "reference-ambiguity" else None
        healthy = (before["coop"]["state"] == after["coop"]["state"] == 2
            and before["coop"]["error"] == after["coop"]["error"] == 0
            and before["world"]["error"] == after["world"]["error"] == 0 and after["world"]["enabled"] == 1
            and int(after["coop"]["tick"]) >= int(before["coop"]["tick"])+8
            and int(after["world"]["captures"]) > int(before["world"]["captures"]))
        if name == "reference-ambiguity":
            healthy = healthy and fresh is not None and fresh["tick"] is not None and 0 <= int(after["coop"]["tick"])-int(fresh["tick"]) <= 120
        self.readiness_count += 1
        result = {"name":name,"role":role,"at_utc":utc(),"before":before,"after":after,
            "saved_reference":fresh,"ready":bool(healthy),"fault_sent":False}
        append(self.root / "fault-premises.jsonl",result)
        return result

    def handle_operation(self,role,operation):
        if operation == "pause-check":
            if role != "remaining" or self.pause is not None: raise ValueError("Pause observation is single-use in remaining stage")
            before = self.observers[role].snapshot()
            time.sleep(1.25)
            after = self.observers[role].snapshot()
            self.pause = {"before":before,"after":after,"human_checkpoint":"User confirmed actual pause menu; no tool-sent game input",
                "tick_and_capture_stopped":before["coop"]["tick"] == after["coop"]["tick"]
                    and before["world"]["captures"] == after["world"]["captures"]
                    and before["coop"]["error"] == after["coop"]["error"] == before["world"]["error"] == after["world"]["error"] == 0}
            save(self.root / "pause-observation.json",self.pause)
            self.send(role,"mark actual-pause-held")
            print("Pause tick/capture stopped="+str(self.pause["tick_and_capture_stopped"]),flush=True)
            return True
        if operation.startswith("ready "):
            value = self.healthy_premise(role,operation[6:])
            print("Actual ready premise="+str(value["ready"])+"; no fault sent",flush=True)
            return True
        if not operation.startswith("fault "): return super().handle_operation(role,operation)
        name = operation[6:]
        if name not in ("object-budget","reference-ambiguity","event-overflow"):
            raise ValueError("Only the three newly prepared fault cases belong to this invocation")
        if (name == "event-overflow") != (role == "fault-event-overflow"):
            raise ValueError("Event capacity failure needs its own fresh stage")
        if any(value["name"] == name for value in self.faults): raise ValueError("Fault already attempted; no retry")
        premise = self.healthy_premise(role,name)
        if not premise["ready"]:
            print("Fault NOT sent: actual simulation or nonempty saved reference premise is absent. Preserve this observation; restore normal foreground simulation before proceeding.",flush=True)
            return True
        outcome = {"name":name,"role":role,"premise":premise,"before":premise["after"],"observations":[]}
        self.faults.append(outcome)
        self.send(role,"e2e-fault "+name)
        until = time.monotonic()+12
        while time.monotonic() < until and self.child.poll() is None:
            sample = {"headers":self.observers[role].snapshot(),"probe":self.probe_snapshot(role)}
            outcome["observations"].append(sample)
            if sample["probe"]["ack"] == FAULTS[name][0] and int(sample["headers"]["coop"]["tick"]) >= int(outcome["before"]["coop"]["tick"])+8: break
            time.sleep(.1)
        after = outcome["observations"][-1] if outcome["observations"] else None
        outcome["scheduler_continues"] = bool(after and after["probe"]["ack"] == FAULTS[name][0]
            and after["headers"]["coop"]["error"] == 0 and after["headers"]["coop"]["state"] == 2
            and int(after["headers"]["coop"]["tick"]) >= int(outcome["before"]["coop"]["tick"])+8
            and after["headers"]["world"]["error"] == FAULTS[name][1])
        save(self.root / ("fault-"+name+".json"),outcome)
        if not outcome["scheduler_continues"]:
            self.ledger.record("W12.fault_isolation","FAIL","New "+name+" failed its original 12-second window; no retry",[self.root / ("fault-"+name+".json")])
            if after and after["headers"]["coop"]["error"]: self.live_blocked = True
        print("Fault acknowledged with actual scheduler continuation="+str(outcome["scheduler_continues"]),flush=True)
        return True

    def run_completion(self):
        self.stage("remaining",[
            "Fresh isolated seeded run, A present, fixture v1.3. Wait for B and actual ordinary samples.",
            "enable / references first; retain both objects before short-pickup so a removed pool slot cannot become the reference parent. Then short-pickup once.",
            "object-budget once only after actual ready premise. Wait for saved KNOWN_CURRENT field 0019, then reference-ambiguity once; preserve bad and recovered samples. references-clear.",
            "Human actual pause menu -> pause-check -> resume. Normal door transition, room settled, then return to main menu without exiting the game.",
            "observe/fixture-log mark actual boundaries, then normal stop. No Esau/GridFree/replay/old passed cases repeated."])
        if "remaining" in self.captures:
            cap = self.captures["remaining"]["directory"]
            self.evaluations["remaining"] = evaluate(cap,self.root / "remaining-evidence")
            value = self.evaluations["remaining"]
            if not value["trace_integrity"] or value["errors"]:
                self.live_blocked = True
        self.stage("fault-event-overflow",[
            "Fresh process and isolated local seeded run with A. No fixture world commands required.",
            "Wait for real samples and tick progress. event-overflow once delivers a collector-owned diagnostic to actual Emit capacity failure.",
            "Observe scheduler continuing with world error 13, then normal stop and 70-slot restoration."])
        if "fault-event-overflow" in self.captures:
            self.evaluations["event-overflow"] = evaluate(self.captures["fault-event-overflow"]["directory"],self.root / "event-overflow-evidence")
        self.finish_checks()

    def finish_checks(self):
        old = load(self.prior / "primary-evidence/evaluation.json")
        replace = load(self.retained / "trigger-evidence/evaluation.json")
        grid = load(self.grid / "new-gridfree-evidence/evaluation.json")
        prior_scene = load(self.grid / "result.json")
        scoped_prior_ok = prior_scene["status"] == "PASS"
        cumulative = sorted(set().union(*(set(v["hooks_covered"]) for v in (old,replace,grid))))
        native_ok = cumulative == list(range(19)) and all(v["trace_integrity"] and not v["errors"] for v in (old,replace,grid))
        current = self.evaluations.get("remaining")
        if not current: return
        capture = self.captures["remaining"]["directory"]
        fixture = load(self.root / "captures/remaining/fixture/final.json")
        self.proof = primary_proof(capture,self.root / "completion-evidence",current,fixture,self.pause)
        proof = self.proof
        safe = current["trace_integrity"] and not current["errors"] and not proof["fixture_errors"]
        sources = [self.root / "completion-evidence",self.root / "remaining-evidence",self.prior,self.retained,self.grid]
        save(self.root / "cumulative-native-coverage.json",{"hooks":cumulative,"sources":[str(self.prior),str(self.retained),str(self.grid)],
            "single_new_binary_all_19_claimed":False,"old_failures_unchanged":True,"basis":"Saved real entry/original/return/ABI/FX witnesses; new E2E instrumentation does not change entry bridges or target ABI"})
        conditions = {
            "W01.native_results":(safe and native_ok,"Cumulative 19 actual ABI/native call witnesses with original build provenance; no new single-capture all-19 claim"),
            "W02.blocked_windows":(safe and proof["window_ok"],"Every new gap paired to actual same-token Window facts; odd, actual pause, loading/lifecycle and normal stages remain distinct"),
            "W03.retained_unknown":(safe and scoped_prior_ok and old["unknown"] and proof["reference_ok"],"Actual retained Player and unknown-class historical evidence plus fresh nonempty KNOWN_CURRENT -> AMBIGUOUS_REUSE -> recovered reference; raw pointer unchanged"),
            "W04.identity_history":(safe and old["nested_spawn"] and old["reuse_groups"] and not old["duplicate_births"]
                and any(p["count"] >= 2 and p["births"] == 1 for p in replace["repeat_publish"]) and proof["short_ok"],
                "Historical nested/reuse/repeated-publish facts plus new short Pickup birth -> actual Remove -> ClearReferences -> complete-source absence; no physical destructor inferred"),
            "W05.transitions":(safe and scoped_prior_ok and proof["rooms_preserve"] and proof["teardown_ok"]
                and old["lifecycle_hooks"].get("NpcMorph:True",0) and old["lifecycle_hooks"].get("PickupMorph:True",0)
                and old["lifecycle_hooks"].get("PlayerRestore:True",0),
                "Saved Morph/Restore/Replace/GridFree with current room physical identity preservation and actual run retirement; current Restore UNKNOWN remains unknown"),
        }
        for key,(passed,reason) in conditions.items():
            self.ledger.items[key]["method"] = "CUMULATIVE_EVIDENCE_AND_NEW_E2E"
            self.ledger.require(key,bool(passed),reason,sources)
        historic_faults = {name:load(self.prior / ("fault-"+name+".json"))["scheduler_continues"] for name in
            ("membership-budget","relation-budget","field-read","source-changed","wrong-thread","root-changed")}
        delivered, fault_marker = [],[]
        if "fault-event-overflow" in self.captures:
            for offset,_,row in rows(self.captures["fault-event-overflow"]["directory"] / "e2e/trace.jsonl"):
                if row["kind"] == 21: delivered.append({"offset":offset,"event":row})
                if row["kind"] == 13 and row["tag"] == 6: fault_marker.append({"offset":offset,"event":row})
        budget_records = []
        for offset,_,row in rows(capture / "world/captures.jsonl"):
            if row["reason"] == "BUDGET": budget_records.append({"offset":offset,"attempt":row["attempt_seq"],"complete":row["complete"]})
        outcome = {"historical_faults":historic_faults,"new_faults":self.faults,"diagnostic_deliveries":delivered,
            "event_capacity_fault_markers":fault_marker,"budget_records":budget_records,"reference_proof":proof["reference_ok"],
            "historical_backpressure":"tx-01-79-live-remaining-01; previously PASS, not rerun","old_W12_FAIL_preserved":True}
        save(self.root / "fault-completion.json",outcome)
        fault_ok = (safe and all(historic_faults.values()) and len(self.faults) == 3 and all(f["scheduler_continues"] for f in self.faults)
            and proof["reference_ok"] and budget_records and all(not r["complete"] for r in budget_records)
            and len(delivered) == len(fault_marker) == 1 and self.evaluations.get("event-overflow",{}).get("trace_integrity")
            and not self.evaluations.get("event-overflow",{}).get("errors",["missing"]))
        if self.ledger.items["W12.fault_isolation"]["status"] != "FAIL":
            self.ledger.items["W12.fault_isolation"]["method"] = "HISTORICAL_BRANCHES_PLUS_NEW_PRECONDITIONED_E2E"
            self.ledger.require("W12.fault_isolation",bool(fault_ok),"Old failures preserved; new object/reference/event-capacity cases exercise corrected prerequisites/delivery and independently observed scheduler continuation",[self.root / "fault-completion.json",*sources])

    def record_completed_results(self):
        save(self.root / "completion-summary.json",{"evaluations":self.evaluations,"faults":self.faults,"proof":self.proof,
            "targets":{key:self.ledger.items[key] for key in TARGETS},"excluded":["W08.replay"],"automatic_retry":False})

    def inventory(self):
        Suite.inventory(self)
