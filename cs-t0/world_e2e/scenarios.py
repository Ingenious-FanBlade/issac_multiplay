"""One user-authorized invocation for newly defined live remaining scenes."""
from __future__ import annotations

import shutil
import time
from pathlib import Path

from .artifacts import CLASSES, load, restoration, save, scan_world, utc
from .runner import Suite
from .win_observer import Api, Game, Observer


class SceneSuite(Suite):
    def __init__(self, args, plan):
        super().__init__(args, plan)
        self.ledger.transaction = "tx-01-79"
        self.ledger.scope = "Historical PASS is carried with original provenance; new manual scenes, metadata and controlled-fault evidence execute once"
        self.api = None
        self.games, self.observers, self.loss_baselines = {}, {}, {}
        self.reader_pause_done = False

    def run_remaining(self):
        source = Path(self.args.continue_from).resolve()
        if source == self.root or source.is_relative_to(self.root) or self.root.is_relative_to(source):
            raise ValueError("Prior evidence and this output must be separate")
        previous = load(source / "result.json")
        if previous["status"] != "INCOMPLETE" or previous["error"]:
            raise ValueError("New live scenes require the completed incomplete report, not a failed/interrupted rerun")
        if load(source / "finalization.json")["status"] != "COMPLETED":
            raise ValueError("Source report did not finish")
        if (Path(self.args.native_dll).resolve() != source / "binaries" / "IsaacCsT0.Native.dll"
                or Path(self.args.helper).resolve() != source / "binaries" / "IsaacCsT0.Helper.exe"):
            raise ValueError("This metadata-only controller invocation requires the unchanged prior Native/Helper")
        prior = self.root / "prior-evidence"
        prior.mkdir()
        carried = []
        for name in ("result.json", "finalization.json", "continuation.json", "supplemental-coverage.json"):
            shutil.copyfile(source / name, prior / name)
        shutil.copyfile(source / "prior-evidence" / "primary-coverage.json", prior / "original-primary-coverage.json")
        self.readable = set(load(prior / "supplemental-coverage.json")["readable_classes"]) | set(load(prior / "original-primary-coverage.json")["readable_classes"])
        self.catalogue = source / "captures" / "supplemental" / "capture" / "restoration.json"
        shutil.copyfile(self.catalogue, prior / "restoration-catalogue.json")
        for group in previous["cases"]:
            for item in group["checks"]:
                if item["status"] == "FAIL":
                    raise ValueError("Prior FAIL must not be hidden by new live scenes")
                if item["status"] == "PASS" and item["id"] in self.ledger.items:
                    self.ledger.items[item["id"]]["method"] = "CARRIED_FORWARD"
                    self.ledger.record(item["id"], "PASS", "Historical evidence, not rerun on the metadata-only controller: " + item["reason"],
                                       [prior / "result.json", *item["evidence"]])
                    carried.append(item["id"])
        failed_observer = source.parent / "tx-01-77-live-remaining-01"
        for name in ("result.json", "finalization.json"):
            shutil.copyfile(failed_observer / name, prior / ("tx-01-77-" + name))
        failed_iat = source.parent / "tx-01-78-live-remaining-01"
        for name in ("result.json", "finalization.json"):
            shutil.copyfile(failed_iat / name, prior / ("tx-01-78-" + name))
        save(self.root / "continuation.json", {"transaction": "tx-01-79", "source": str(source), "at_utc": utc(),
            "observer_previous_failures": [str(failed_observer), str(failed_iat)],
            "observer_fix": "PSAPI module identity and native-matching per-slot ownership: three USER32 IAT slots; other function-pointer slots in game image; original bytes captured for this process",
            "carried_forward_pass": carried, "controller_change": "Write independent read-only observation metadata only",
            "native_helper": "Unchanged frozen tx-01-75 artifacts", "full_replay": "USER_EXCLUDED", "offline_cases": "ALREADY_COMPLETED_NOT_REPEATED",
            "new_scenes": ["Familiar pickup after recording starts", "manual pause/resume and room/run markers", "one 1000ms own-reader pause", "one owned-controller loss in a fresh process"],
            "special_lifecycle_faults": "Undefined Morph/Restore/Replace/reuse/native read/resource faults remain uncovered", "automatic_retry": False})
        self.ledger.record("W08.replay", "NOT_COVERED", "User continues to exclude full replay; completed offline cases are not rerun", [self.root / "continuation.json"])
        print(f"Carried {len(carried)} historical PASS; no replay or offline CLI cases in this invocation.", flush=True)
        self.capture({"role": "primary", "script": str(self.root / "inputs" / "primary.json"), "instructions": [
            "Fresh isolated local run; A exists. Prepare an UNCOLLECTED Familiar-generating item; pick it up only after the recording-start marker.",
            "Use mark before/after actual pickup, pause/resume, ordinary room transition and optional menu teardown as directed by the human checkpoints.",
            "pause-reader suspends exactly one owned world-reader for 1000ms and resumes it in finally; no game thread is suspended.",
            "Keep the game simulating during pause-reader; stop normally after all intended scenes. No new 10,000-sample requirement.",
        ]})
        self.scan_new_primary()
        self.capture({"role": "controller-loss", "script": str(self.root / "inputs" / "controller-loss.json"), "instructions": [
            "Last stage: fully restart into a NEW isolated local run with A present; no reinjection into the prior resident process.",
            "Wait for actual samples and the explicit loss checkpoint. lose-controller terminates only this Popen-owned controller once.",
            "The independent observer holds read-only mappings and checks 70 actual bytes, closed publication, callback drain and resident Native.",
            "No helper recovery attempt, unload, reinjection or game termination follows the failure. Any missing/failed recovery is preserved.",
        ]})

    def before_capture(self, role, process, capture):
        if self.api is None:
            self.api = Api()
        directory = capture.parent / "external"
        directory.mkdir()
        self.games[role] = Game(self.api, process, self.args.game_exe, self.catalogue, directory)

    def controller_started(self, role, capture):
        metadata = capture / "observation-metadata.json"
        deadline = time.monotonic() + 10
        while not metadata.is_file() and self.child.poll() is None and time.monotonic() < deadline:
            time.sleep(0.05)
        if not metadata.is_file():
            raise RuntimeError("Controller did not publish independent observation metadata")
        self.observers[role] = Observer(self.api, metadata, self.games[role], self.child, self.controller, capture.parent / "external")
        print("Read-only observer attached. Extra command: pause-reader (primary only, once).", flush=True)

    def handle_operation(self, role, operation):
        if operation != "pause-reader":
            return False
        if role != "primary" or self.reader_pause_done:
            raise ValueError("Reader fault is limited to once in primary; no automatic retry")
        self.reader_pause_done = True
        self.send(role, "mark controlled_reader_pause_begin")
        outcome = self.observers[role].pause_reader()
        self.send(role, "mark controlled_reader_pause_end")
        self.event("reader_pause_observed", role=role, observed_backpressure_and_tick_progress=outcome["observed_backpressure_and_tick_progress"])
        self.ledger.record("W12.fault_isolation", "NOT_COVERED", "Reader-pause evidence saved; native resource/relationship budgets, read failures and observer fault branches were not all exercised", [self.root / "captures" / role / "external"])
        print("Reader resumed; backpressure and simultaneous tick progress=" + str(outcome["observed_backpressure_and_tick_progress"]), flush=True)
        return True

    def before_controller_loss(self, role):
        observer = self.observers[role]
        sample = observer.snapshot()
        if (sample["coop"]["state"] != 2 or sample["coop"]["error"] or sample["world"]["error"]
                or not sample["world"]["enabled"] or int(sample["world"]["captures"]) == 0):
            raise ValueError("Controller-loss requires healthy actual active capture")
        installed = self.games[role].snapshot("installed-before-loss")
        if installed["restored_count"] != 0:
            raise ValueError("Not all 70 targets are observably installed; loss was not triggered")
        save(observer.directory / "before-controller-loss.json", sample)
        self.loss_baselines[role] = sample

    def capture_finished(self, role):
        captured = self.captures[role]
        if captured["controller_lost"]:
            outcome = self.observers[role].after_loss(self.loss_baselines[role], self.native)
            self.ledger.items["W13.controller_loss"]["method"] = "AUTOMATED"
            self.ledger.require("W13.controller_loss", outcome["status"] == "PASS",
                "Actual owned-controller termination; independent stable closed headers, 70 original bytes, restoration events and resident module; controller final flush remains unconfirmed",
                [self.observers[role].directory, self.root / "operator-actions.jsonl"])
        else:
            raw = self.games[role].snapshot("after-normal-stop")
            if raw["restored_count"] != 70:
                self.ledger.record("W01.optional_70", "FAIL", "Independent post-stop bytes did not restore all 70 targets", [self.games[role].directory])

    def scan_new_primary(self):
        primary = self.captures.get("primary")
        if not primary or primary["controller_lost"]:
            return
        world = primary["directory"] / "world"
        summary, _ = scan_world(world, self.root / "primary-record-index.jsonl")
        self.summaries["primary"] = summary
        save(self.root / "primary-coverage.json", summary)
        witness = summary["class_witnesses"].get("Familiar")
        observed_birth = (witness is not None and witness["object"]["birth_event"] is not None
                          and witness["object"]["init_evidence"] == 2 and "birth_event_evidence" in witness)
        combined = self.readable | set(summary["readable_classes"])
        clean = (primary["exit_code"] == 0 and primary.get("same_process_after") is True
                 and restoration(primary["directory"], 70) and not summary["profile_failures"]
                 and not summary["invariant_failures"] and not summary["event_gaps"]
                 and summary["producer_result"]["native_error"] == "NONE")
        save(self.root / "new-scene-outcome.json", {"familiar_birth_init_witness": observed_birth,
            "combined_readable_classes": sorted(combined), "clean_capture_and_restore": clean,
            "reader_pause_executed": self.reader_pause_done, "full_prior_and_current_sources_remain_separate": True})
        self.ledger.items["W03.eight_classes"]["method"] = "AUTOMATED"
        evidence = [self.root / "primary-coverage.json", self.root / "prior-evidence", self.root / "new-scene-outcome.json"]
        if not clean:
            self.ledger.record("W03.eight_classes", "FAIL", "New scene capture/profile/restore failed; no retry", evidence)
        elif observed_birth and set(CLASSES[:8]).issubset(combined):
            self.ledger.record("W03.eight_classes", "PASS", "Readable eight classes across explicit historical/new sources; Familiar has actual observed birth and completed Init after recording started", evidence)
        else:
            self.ledger.record("W03.eight_classes", "NOT_COVERED", "Still missing readable class or actual Familiar birth/Init witness", evidence)

    def close_observers(self):
        for observer in self.observers.values():
            observer.close()
        for game in self.games.values():
            game.close()
