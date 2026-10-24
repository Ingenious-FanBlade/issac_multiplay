"""tx-01-80: one invocation for the ten remaining authorized obligations."""
from __future__ import annotations
import json
import shutil
import time
from pathlib import Path
from .artifacts import append, load, restoration, rows, save, utc
from .runner import ROOT, Suite, prompt
from .win_observer import Api, Game, Observer, ReadMapping
from .instrumented_evidence import evaluate, frame

FAULTS = {"object-budget":(1,0),"membership-budget":(2,0),"relation-budget":(3,0),
    "reference-ambiguity":(8,0),"field-read":(4,4),"source-changed":(5,17),
    "event-overflow":(6,13),"wrong-thread":(7,9),"root-changed":(9,12)}
TERMINAL = ("field-read","source-changed","event-overflow","wrong-thread","root-changed")
TARGETS=("W01.native_results","W02.phase_order","W02.blocked_windows","W03.retained_unknown",
    "W04.identity_history","W05.transitions","W06.revocation","W07.readonly_calls","W11.scenario_basis","W12.fault_isolation")


class InstrumentedSuite(Suite):
    def __init__(self,args,plan):
        super().__init__(args,plan)
        self.ledger.transaction = "tx-01-83"
        self.ledger.scope = "Legacy remaining-check entry with explicit E2E Native; this entry does not run full replay"
        self.api = None
        self.games, self.observers, self.probes = {}, {}, {}
        self.faults, self.evaluations = [], {}
        self.log = Path(args.game_log).resolve()

    def prepare(self):
        super().prepare()
        extra = ("cs-t0/src/world_e2e.rs","cs-t0/world_e2e/instrumented.py","cs-t0/world_e2e/instrumented_evidence.py",
            "cs-t0/native/src/world_e2e.cpp","cs-t0/native/src/world_e2e_bridges.inc",
            "cs-t0/native/include/isaac_cs_t0/world_e2e.hpp","cs-t0/native/IsaacCsT0.Native.vcxproj",
            "test-mod/isaac-cs-world-e2e/main.lua","test-mod/isaac-cs-world-e2e/metadata.xml","test-mod/isaac-cs-world-e2e/README.md",
            "docs/refactor/cs-t0/t0-06/tx-01-80-remaining-test-design.md",
            "docs/refactor/cs-t0/t0-06/tx-01-80-e2e-results.md","docs/refactor/cs-t0/t0-06/tx-01-81-process-observer-fix.md",
            "docs/refactor/cs-t0/t0-06/tx-01-81-e2e-results.md","docs/refactor/cs-t0/t0-06/tx-01-82-host-execution.md",
            "docs/refactor/cs-t0/t0-06/tx-01-82-e2e-results.md","docs/refactor/cs-t0/t0-06/tx-01-83-diagnostics-and-finalization.md")
        for name in extra:
            destination = self.root / "sources" / name
            destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(ROOT / name,destination)

    def controller_arguments(self,role):
        return ["--world-e2e-probe","v1"]

    def run_remaining(self):
        source = Path(self.args.continue_from).resolve()
        previous = load(source / "result.json")
        if source == self.root or source.is_relative_to(self.root) or self.root.is_relative_to(source):
            raise ValueError("Prior and new outputs must be separate")
        if previous["error"] or load(source / "finalization.json")["status"] != "COMPLETED":
            raise ValueError("Prior invocation did not finish")
        prior = self.root / "prior-evidence";prior.mkdir()
        shutil.copyfile(source / "result.json",prior / "result.json")
        shutil.copyfile(source / "finalization.json",prior / "finalization.json")
        self.catalogue = source / "captures" / "primary" / "capture" / "restoration.json"
        shutil.copyfile(self.catalogue,prior / "restoration-catalogue.json")
        failed=source.parent / "tx-01-80-instrumented-e2e-01"
        for name in ("result.json","finalization.json","authorized-scope-result.json"):
            shutil.copyfile(failed / name,prior / ("tx-01-80-"+name))
        failed81=source.parent / "tx-01-81-instrumented-e2e-01"
        for name in ("result.json","finalization.json","authorized-scope-result.json"):
            shutil.copyfile(failed81 / name,prior / ("tx-01-81-"+name))
        failed82=source.parent / "tx-01-82-instrumented-e2e-01"
        if failed82.is_dir():
            for name in ("result.json","finalization.json","authorized-scope-result.json"):
                if (failed82 / name).is_file():shutil.copyfile(failed82 / name,prior / ("tx-01-82-"+name))
        for group in previous["cases"]:
            for item in group["checks"]:
                if item["status"] == "PASS" and item["id"] in self.ledger.items:
                    self.ledger.items[item["id"]]["method"] = "CARRIED_FORWARD"
                    self.ledger.record(item["id"],"PASS","Historical tx-01-79 evidence; not rerun or claimed for the newly instrumented binary: " + item["reason"],
                        [prior / "result.json",*item["evidence"]])
        save(self.root / "continuation.json",{"transaction":"tx-01-83","source":str(source),"previous_failures":[str(failed),str(failed81)]+([str(failed82)] if failed82.is_dir() else []),
            "at_utc":utc(),"automatic_retry":False,"user_excluded":["W08.replay"],
            "game_log":str(self.log),"new_native":"WorldE2E=true, original 70 target slots, separate E2E mapping",
            "fault_scope":"Observer-owned branch injection; never corrupt game memory or claim OS access faults"})
        for key in ("W08.replay",):
            self.ledger.record(key,"NOT_COVERED","USER_EXCLUDED by current conversation",[self.root / "continuation.json"])
        self.stage("primary",[
            "Fresh seeded isolated local run; fixture enabled, A exists, no Online/Isaac Sync. Install/remove the reviewed fixture manually before this invocation.",
            "After B creation, use the documented cswe2e scene/grid/Esau Jr./Hour Glass console actions with markers; normal pause/resume, room transition and menu teardown remain human checkpoints.",
            "budget/reference faults use 'fault <name>'; terminal observer faults run later in separate fresh processes.",
            "Commands: observe <label>, fixture-log, fault <name>; no automatic game input, game launch, reinjection or full replay."])
        if "primary" in self.captures:
            self.evaluations["primary"] = evaluate(self.captures["primary"]["directory"],self.root / "primary-evidence")
            if not self.evaluations["primary"]["trace_integrity"] or self.evaluations["primary"]["errors"]:
                self.live_blocked=True
                self.event("instrumentation_invariant_failed",no_further_live_changes=True)
        for role in ("align-left","align-right"):
            self.stage(role,[
                "Fresh process and NEW seeded run, same seed/character/difficulty/starting room and neutral gameplay input from run start; no scene actions.",
                "Enable fixture and arm the SAME future absolute Game frame with cswe2e align <frame>; preserve startup raw-input plus canonical controller events.",
                "Stop after the chosen frame was observed. The comparison uses exact native game-frame equality only and requires explicit human mapping confirmation."])
        try:self.strict_compare()
        except (OSError,ValueError,RuntimeError,KeyError,TypeError) as exc:
            self.ledger.record("W11.scenario_basis","FAIL",str(exc)+"; no retry",[self.root / "captures",self.root / "timeline.jsonl"])
        for name in TERMINAL:
            self.stage("fault-"+name,[
                "Fresh isolated seeded run with A present; unique new game process. Wait for real healthy samples and B creation.",
                "Keep game simulating and enter fault "+name+" once. Only the observer's own state/branch is affected.",
                "The executor saves independent headers while Authority tick continues; then stop cooperatively, restore 70 targets, keep resident module."])

    def stage(self,role,instructions):
        if self.live_blocked:
            self.event("stage_not_run",role=role,reason="Prior restoration uncertain")
            return
        self.capture({"role":role,"script":str(self.root / "inputs" / "primary.json"),"instructions":instructions})

    def before_capture(self,role,process,capture):
        if self.api is None:self.api=Api()
        directory=capture.parent / "external";directory.mkdir()
        self.games[role]=Game(self.api,process,self.args.game_exe,self.catalogue,directory)

    def controller_started(self,role,capture):
        metadata=capture / "observation-metadata.json"
        until=time.monotonic()+10
        while time.monotonic()<until and self.child.poll() is None:
            try:
                meta=load(metadata)
                break
            except (FileNotFoundError,json.JSONDecodeError):time.sleep(.05)
        else:raise RuntimeError("Controller metadata did not finish publication")
        self.observers[role]=Observer(self.api,metadata,self.games[role],self.child,self.controller,capture.parent / "external")
        meta=load(metadata)
        if not meta.get("e2e_mapping"):raise ValueError("No explicit E2E probe transport")
        self.probes[role]=ReadMapping(self.api,meta["e2e_mapping"],8388736)
        print("Independent observer and E2E probe mapping opened. No scene action is automatic.",flush=True)

    def probe_snapshot(self,role):
        import struct
        raw=self.probes[role].read(0,128)
        meta=self.observers[role].meta
        if (struct.unpack_from("<Q",raw)[0]!=0x3145324557534349 or struct.unpack_from("<4I",raw,8)!=(1,8388736,meta["game_pid"],meta["controller_pid"])
                or raw[24:56]!=bytes.fromhex(meta["nonce"])):raise ValueError("E2E mapping identity/layout changed")
        return {"command":struct.unpack_from("<i",raw,56)[0],"ack":struct.unpack_from("<i",raw,60)[0],
            "error":struct.unpack_from("<i",raw,64)[0],"closed":struct.unpack_from("<i",raw,68)[0],
            "worker":list(struct.unpack_from("<3I",raw,104)),
            "collector_owned_noaccess_page":struct.unpack_from("<I",raw,116)[0],
            "raw_header":raw.hex()}

    def handle_operation(self,role,operation):
        if operation.startswith("observe "):
            append(self.root / "scene-observations.jsonl",{"role":role,"label":operation[8:],
                "at_utc":utc(),"snapshot":self.observers[role].snapshot(),"probe":self.probe_snapshot(role)})
            self.send(role,"mark "+operation[8:]);return True
        if operation=="fixture-log":
            self.copy_log(role,"checkpoint-"+str(time.monotonic_ns()));return True
        if not operation.startswith("fault "):return False
        name=operation[6:]
        if name not in FAULTS:raise ValueError("Unknown authorized observer fault")
        if any(f["name"]==name for f in self.faults):raise ValueError("Fault already attempted; no retry")
        if (name in TERMINAL)!=(role=="fault-"+name):raise ValueError("Terminal faults require their own fresh stage")
        before=self.observers[role].snapshot()
        if before["coop"]["state"]!=2 or before["coop"]["error"] or before["world"]["error"] or int(before["world"]["captures"])==0:
            raise ValueError("Fault needs actual healthy active samples")
        outcome={"role":role,"name":name,"before":before,"observations":[]}
        self.faults.append(outcome)
        self.send(role,"e2e-fault "+name)
        until=time.monotonic()+12
        while time.monotonic()<until and self.child.poll() is None:
            sample={"headers":self.observers[role].snapshot(),"probe":self.probe_snapshot(role)}
            outcome["observations"].append(sample)
            if (sample["probe"]["ack"]==FAULTS[name][0] and int(sample["headers"]["coop"]["tick"])>=int(before["coop"]["tick"])+8):break
            time.sleep(.1)
        after=outcome["observations"][-1] if outcome["observations"] else None
        outcome["scheduler_continues"]=(after is not None and after["probe"]["ack"]==FAULTS[name][0]
            and after["headers"]["coop"]["error"]==0 and after["headers"]["coop"]["state"]==2
            and int(after["headers"]["coop"]["tick"])>=int(before["coop"]["tick"])+8
            and after["headers"]["world"]["error"]==FAULTS[name][1])
        if name=="wrong-thread" and after is not None:
            main,worker,result=after["probe"]["worker"]
            outcome["scheduler_continues"] &= bool(main and worker and main!=worker and result==0)
        save(self.root / ("fault-"+name+".json"),outcome)
        if not outcome["scheduler_continues"]:
            self.ledger.record("W12.fault_isolation","FAIL","Controlled "+name+" lacks required independent continuation/error evidence; no retry",[self.root / ("fault-"+name+".json")])
            if after is not None and after["headers"]["coop"]["error"]:self.live_blocked=True
        print("Fault acknowledged; scheduler and expected observer status="+str(outcome["scheduler_continues"]),flush=True)
        return True

    def copy_log(self,role,name):
        directory=self.root / "captures" / role / "fixture";directory.mkdir(exist_ok=True)
        target=directory / (name+".log")
        shutil.copyfile(self.log,target)
        records=[]
        with target.open("r",encoding="utf-8-sig",errors="replace") as stream:
            for line in stream:
                at=line.find("[CSWORLD-E2E-v1] ")
                if at>=0:
                    record=json.loads(line[at+len("[CSWORLD-E2E-v1] "):])
                    if record["kind"]=="run_start":records=[]
                    records.append(record)
        save(directory / (name+".json"),records)
        return records

    def capture_finished(self,role):
        raw=self.games[role].snapshot("after-normal-stop")
        self.copy_log(role,"final")
        if raw["restored_count"]!=70:
            self.live_blocked=True
            raise RuntimeError("Actual bytes did not restore all 70 targets")
        capture=self.captures[role]["directory"]
        report=load(capture / "result.json")
        expected=FAULTS.get(role.removeprefix("fault-"),(0,0))[1] if role.startswith("fault-") else 0
        if report["nativeError"] or report["transportFailure"] or not restoration(capture,70):
            self.live_blocked=True
            raise RuntimeError("Unexpected coop/restoration failure; preserve and stop")
        world=load(capture / "world" / "result.json")
        probe=load(capture / "e2e" / "result.json")
        if not probe["closed"] or probe["error"] or probe["overflow"]!="0" or probe["produced"]!=probe["consumed"]:
            self.live_blocked=True
            raise RuntimeError("E2E evidence overflow/closure/consumption failed; no further live changes")
        # Numeric expected errors are checked independently above; named error
        # and controlled controller exit remain raw artifacts, never rewritten.
        if not expected and (world["native_error"]!="NONE" or self.captures[role]["exit_code"]!=0):
            self.live_blocked=True
            raise RuntimeError("Unexpected observer/consumer failure in healthy stage")

    def strict_compare(self):
        if not all(role in self.captures for role in ("align-left","align-right")):return
        candidates=[]
        for role in ("align-left","align-right"):
            logs=load(self.root / "captures" / role / "fixture" / "final.json")
            marks=[r for r in logs if r["kind"]=="aligned_frame"]
            if len(marks)!=1:raise ValueError("Missing unique actual exact-frame fixture witness")
            target=marks[0]["frame"]
            inputs=[r for r in logs if r["kind"]=="raw_input" and r["frame"]<=target]
            if (not inputs or inputs[0]["frame"]>1 or any(b["frame"]!=a["frame"]+1 for a,b in zip(inputs,inputs[1:]))
                or inputs[-1]["frame"]!=target or any(any(v!=0 for v in p["values"]) for r in inputs for p in r["observed"])):
                raise ValueError("Startup raw input missing/nonneutral; no strict alignment inference")
            found=[(offset,record) for offset,_,record in rows(self.captures[role]["directory"] / "world" / "captures.jsonl")
                if record["record"]=="capture" and record["authority_kind"]==1 and frame(record)==target]
            if len(found)!=1:raise ValueError("Exact native frame has no unique ordinary capture; never choose a neighbor")
            resolved=set()
            for _,_,event in rows(self.captures[role]["directory"] / "events.jsonl"):
                if event["kind"] not in (4,5):continue
                state=event["state"]
                if any(state[k]!=0 for k in ("held_flags","edge_flags","move_x","move_y","aim_x","aim_y")):
                    raise ValueError("Actual canonical input was not neutral; no strict alignment")
                if event["kind"]==5:resolved.add(event["ordinal"])
            if not {0,1}.issubset(resolved):raise ValueError("No canonical Resolve evidence for both players")
            candidates.append({"role":role,"fixture":marks[0],"offset":found[0][0],"record":found[0][1],"canonical_resolve_ordinals":sorted(resolved)})
        left,right=candidates
        if left["fixture"]["seed"]!=right["fixture"]["seed"] or left["fixture"]["room"]!=right["fixture"]["room"]:
            raise ValueError("Actual seed/room differ")
        directory=self.root / "cross-process";directory.mkdir()
        alignment={"version":1,"left":{k:left["record"][k] for k in ("run_epoch","room_epoch")},
            "right":{k:right["record"][k] for k in ("run_epoch","room_epoch")},
            "ticks":[{"left":left["record"]["authority_tick"],"right":right["record"]["authority_tick"]}],
            "births":[],"players":[{"left":0,"right":0},{"left":1,"right":1}]}
        left_b=[o for o in left["record"]["objects"] if o["class"]=="Player" and o["ordinal"]==1]
        right_b=[o for o in right["record"]["objects"] if o["class"]=="Player" and o["ordinal"]==1]
        if len(left_b)!=1 or len(right_b)!=1 or left_b[0]["birth_event"] is None or right_b[0]["birth_event"] is None:
            raise ValueError("No unique actual B creation events for explicit mapping")
        from .instrumented_evidence import fields
        birth_counters={"left":fields(left_b[0])["001c"],"right":fields(right_b[0])["001c"]}
        alignment["births"]=[{"left":left_b[0]["birth_event"],"right":right_b[0]["birth_event"]}]
        save(directory / "alignment-proposal.json",alignment)
        save(directory / "basis.json",{"scope":"Same actual seed and neutral input, exact current Game frame; does not assert identical initial worlds or join timing",
            "left":{k:v for k,v in left.items() if k!="record"},"right":{k:v for k,v in right.items() if k!="record"},
            "b_birth_counters":birth_counters,"b_birth_counter_equal":birth_counters["left"]==birth_counters["right"],
            "earlier_scope":"Different B join timing, unobserved pre-injection world history and unmapped objects remain explicit; no global-first or deterministic-whole-world claim",
            "unmapped_births":"Preserved as unknown, no nearest-frame/address/position mapping"})
        answer=prompt("Review exact-frame alignment-proposal.json and input/birth basis. Enter 'confirm exact alignment' or 'skip <reason>'.")
        if answer!="confirm exact alignment":
            self.event("strict_alignment_not_confirmed",answer=answer);return
        shutil.copyfile(directory / "alignment-proposal.json",directory / "alignment.json")
        save(directory / "alignment-provenance.json",{"confirmed_at_utc":utc(),"answer":answer,"basis":"basis.json"})
        code=self.command("strict-exact-frame-compare",[str(self.controller),"world-compare",
            "--left",str(self.captures["align-left"]["directory"] / "world"),"--right",str(self.captures["align-right"]["directory"] / "world"),
            "--alignment",str(directory / "alignment.json"),"--artifact-dir",str(directory / "comparison")])
        if code:raise RuntimeError("Production exact-frame comparison failed; no retry")

    def record_completed_results(self):
        # This method is invoked once by Suite.finalize_evidence, before normal
        # manual review or from finally after an exception. It never reruns analysis.
        save(self.root / "remaining-review-evidence.json",{"evaluations":self.evaluations,"faults":self.faults,
            "required_faults":list(FAULTS),"all_fault_scheduler_checks":len(self.faults)==len(FAULTS) and all(f.get("scheduler_continues") is True for f in self.faults),
            "faults_not_attempted":[name for name in FAULTS if not any(f["name"]==name for f in self.faults)],
            "manual_items":["W02.blocked_windows","W03.retained_unknown","W04.identity_history","W05.transitions","W07.readonly_calls","W11.scenario_basis","W12.fault_isolation"],
            "excluded":["W08.replay"]})
        value=self.evaluations.get("primary")
        if not value:return
        evidence=[self.root / "primary-evidence",self.root / "captures" / "primary",self.root / "sources"]
        if not value["trace_integrity"] or value["errors"]:
            self.ledger.record("W01.native_results","FAIL","E2E trace/ABI/RNG/phase invariant failed; no retry",evidence)
            return
        self.ledger.items["W01.native_results"]["method"]="AUTOMATED_WITH_STATIC_ABI_REVIEW"
        if self.ledger.items["W01.native_results"]["status"]!="FAIL":
            self.ledger.record("W01.native_results","PASS" if len(value["hooks_covered"])==19 else "NOT_COVERED",
                f"Observed {len(value['hooks_covered'])}/19 entry/return/ABI witnesses; missing calls are not a preservation failure",evidence)
        self.ledger.items["W02.phase_order"]["method"]="AUTOMATED"
        if self.ledger.items["W02.phase_order"]["status"]!="FAIL":
            self.ledger.record("W02.phase_order","NOT_COVERED" if not value["ordinary"] else (
                "PASS" if value["phase_matches"]==value["ordinary"] else "FAIL"),
                f"Same-token owner/Room/tail/commit/TopExit matches {value['phase_matches']}/{value['ordinary']} saved ordinary captures",evidence)
        self.ledger.items["W06.revocation"]["method"]="AUTOMATED_WITH_SCENE_REVIEW"
        if self.ledger.items["W06.revocation"]["status"]!="FAIL":
            self.ledger.record("W06.revocation","PASS" if value["first_observed_preserved"] and value["readiness_revoked_reacquired"] else "NOT_COVERED",
                "Requires observed FIRST_OBSERVED preservation and A readiness 0->3->0->3; absence is uncovered, not inferred success",evidence)

    def close_observers(self):
        errors=[]
        for label,resources in (("probe",self.probes),("observer",self.observers),("game",self.games)):
            for role,resource in resources.items():
                try:resource.close()
                except Exception as exc:errors.append(f"{label}/{role}: {type(exc).__name__}: {exc}")
        if errors:raise RuntimeError("; ".join(errors))

    def inventory(self):
        checks={key:self.ledger.items[key]["status"] for key in TARGETS}
        save(self.root / "authorized-scope-result.json",{"scope":"User-requested ten remaining checks",
            "status":"COMPLETE" if all(v=="PASS" for v in checks.values()) else "INCOMPLETE",
            "checks":checks,"user_excluded":["W08.replay"],
            "global_ledger":"result.json retains excluded NOT_COVERED and historical provenance"})
        super().inventory()
