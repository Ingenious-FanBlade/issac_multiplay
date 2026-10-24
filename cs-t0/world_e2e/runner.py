"""Human-checkpoint, single-invocation acceptance. No work occurs at import time."""
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

from .artifacts import (CLASSES, CONTINUITY_RULE, MANUAL, Ledger, LiveProgress, append, load, number,
                        restoration, rows, save, scan_world, utc)
from .offline import Offline

ROLE_ORDER = ("default", "primary", "secondary", "controller-loss")
FORWARD = {"status", "script", "queries", "conflict-b", "retire-b", "stop"}
ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    "cs-t0/run-world-record-e2e.py", "cs-t0/accept-world-record-e2e.ps1",
    "cs-t0/world_e2e/__init__.py", "cs-t0/world_e2e/artifacts.py",
    "cs-t0/world_e2e/offline.py", "cs-t0/world_e2e/runner.py",
    "cs-t0/world_e2e/remaining.py",
    "cs-t0/world_e2e/scenarios.py", "cs-t0/world_e2e/win_observer.py",
    "cs-t0/src/world_record.rs", "cs-t0/src/world_record_live.rs", "cs-t0/src/coop_active.rs", "cs-t0/src/main.rs",
    "cs-t0/native/src/world_record.cpp", "cs-t0/native/src/hooks.cpp",
    "cs-t0/native/src/loop_runtime.cpp", "cs-t0/native/src/coop_active_runtime.cpp",
    "cs-t0/native/include/isaac_cs_t0/world_record.hpp",
    "cs-t0/native/include/isaac_cs_t0/coop_active_ipc.hpp",
    "cs-t0/native/include/isaac_cs_t0/hooks.hpp", "cs-t0/native/include/isaac_cs_t0/loop_runtime.hpp",
    "docs/refactor/project-conventions.md",
    "docs/refactor/cs-t0/t0-06/tx-01-73-e2e-checklist.md",
    "docs/refactor/cs-t0/t0-06/tx-01-74-unified-executor.md",
    "docs/refactor/cs-t0/t0-06/tx-01-75-immediate-consumer.md",
    "docs/refactor/cs-t0/t0-06/tx-01-76-remaining-e2e.md",
    "docs/refactor/cs-t0/t0-06/tx-01-77-live-remaining.md",
    "docs/refactor/cs-t0/t0-06/tx-01-77-e2e-results.md",
    "docs/refactor/cs-t0/t0-06/tx-01-78-observer-fix.md",
    "docs/refactor/cs-t0/t0-06/tx-01-78-e2e-results.md",
    "docs/refactor/cs-t0/t0-06/tx-01-79-iat-observer-fix.md",
)

def prompt(message):
    print(message, flush=True)
    try:
        return input("> ").strip()
    except EOFError as exc:
        raise RuntimeError("Interactive input closed; preserve artifacts, do not retry") from exc


def path_from(base, value):
    path = Path(value)
    return (path if path.is_absolute() else base / path).resolve()


def read_plan(path):
    plan = load(path)
    if set(plan) != {"version", "captures"} or type(plan["version"]) is not int or plan["version"] != 1 or not isinstance(plan["captures"], list):
        raise ValueError("plan requires exactly version=1 and captures array")
    roles = []
    for stage in plan["captures"]:
        if set(stage) != {"role", "script", "instructions"} or stage["role"] not in ROLE_ORDER:
            raise ValueError("capture requires known role, script and instructions only")
        if not isinstance(stage["instructions"], list) or not stage["instructions"] or any(not isinstance(text, str) for text in stage["instructions"]):
            raise ValueError("capture instructions must be a nonempty string array")
        stage["script"] = str(path_from(path.parent, stage["script"]))
        load(stage["script"])  # Preserve input before launching; production validates actual input semantics.
        roles.append(stage["role"])
    if roles != list(ROLE_ORDER):
        raise ValueError("captures must contain default, primary, secondary, controller-loss once in that order")
    return plan


class Suite:
    def __init__(self, args, plan):
        self.args, self.plan = args, plan
        self.root = Path(args.artifact_dir).resolve()
        self.root.mkdir(parents=True, exist_ok=False)
        self.ledger = Ledger(self.root)
        self.controller = self.root / "binaries" / "IsaacCsT0.exe"
        self.native = self.root / "binaries" / "IsaacCsT0.Native.dll"
        self.helper = self.root / "binaries" / "IsaacCsT0.Helper.exe"
        self.processes, self.captures, self.summaries = set(), {}, {}
        self.primary_sample = None
        self.live_blocked = False
        self.evidence_finalization_started = False
        self.evidence_finalization_complete = False
        self.child = None
        self.stop_sent = False
        self.finished_commands = 0
        self.command_failures = []
        self.process_query_count = 0

    def event(self, kind, **detail):
        append(self.root / "timeline.jsonl", {"at_utc": utc(), "kind": kind, **detail})

    def prepare(self):
        save(self.root / "manifest.json", {
            "transaction": self.ledger.transaction, "started_at_utc": utc(), "authorization": self.args.authorization,
            "game_exe": str(Path(self.args.game_exe).resolve()), "plan_source": str(Path(self.args.plan).resolve()),
            "python": sys.executable, "python_version": sys.version, "powershell": self.args.powershell,
            "source_root": str(ROOT), "scenario_mode": "USER_ACCEPTED_MANUAL_CHECKPOINTS",
            "execution_context":os.environ.get("ISAAC_WORLD_E2E_EXECUTION_CONTEXT","WORKSPACE_EXEC_DEFAULT"),
            "w07_continuity_rule": CONTINUITY_RULE,
            "automatic_retry": False, "hashes": "NOT_USED", "build_performed": False,
            "required_completion_record": "finalization.json",
            "semantics": "One invocation, multiple fresh user-started processes, offline production CLI and explicit manual reviews",
        })
        shutil.copyfile(self.args.plan, self.root / "original-plan.json")
        self.plan = read_plan(Path(self.args.plan).resolve())
        save(self.root / "resolved-plan.json", self.plan)
        binaries = []
        for source, destination in ((self.args.controller, self.controller), (self.args.native_dll, self.native), (self.args.helper, self.helper)):
            origin = Path(source).resolve()
            if not origin.is_file():
                raise ValueError(f"binary does not exist: {origin}")
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(origin, destination)
            stat = origin.stat()
            binaries.append({"source": str(origin), "copy": str(destination.relative_to(self.root)),
                             "bytes": stat.st_size, "last_write_ns": str(stat.st_mtime_ns)})
        save(self.root / "binary-provenance.json", binaries)
        for source in SOURCES:
            destination = self.root / "sources" / source
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / source, destination)
        for stage in self.plan["captures"]:
            destination = self.root / "inputs" / (stage["role"] + ".json")
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(stage["script"], destination)
            stage["script"] = str(destination)
        save(self.root / "manual-review-template.json", {
            "version": 1, "reviews": [
                {"id": key, "status": "NOT_COVERED", "observer": "", "reason": "Not yet reviewed", "evidence": []}
                for key in sorted(MANUAL)]})
        self.event("prepared")

    def command(self, name, argv):
        """Launch exactly once. A nonzero exit can be the expected decoder result."""
        self.finished_commands += 1
        directory = self.root / "commands" / f"{self.finished_commands:03}-{name}"
        directory.mkdir(parents=True)
        save(directory / "invocation.json", {"argv": argv, "at_utc": utc(), "automatic_retry": False})
        self.event("command_start", name=name, argv=argv)
        with (directory / "output.log").open("xb") as output:
            child = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=output, stderr=subprocess.STDOUT,
                                     creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
            # Do not terminate or rerun a production command after an interruption.
            try:
                code = child.wait()
            except KeyboardInterrupt:
                self.event("offline_command_interrupted", name=name, pid=child.pid,
                           note="Child was not automatically killed or rerun")
                raise
        save(directory / "exit.json", {"exit_code": code, "ended_at_utc": utc()})
        self.event("command_end", name=name, exit_code=code)
        print(f"[offline] {name}: exit={code}", flush=True)
        return code

    def process_snapshot(self):
        from .win_observer import discover_unique
        self.process_query_count += 1
        return discover_unique(self.args.game_exe,self.root / "process-queries" / f"{self.process_query_count:03}")

    @staticmethod
    def process_key(item):
        return item["pid"], item["creation_time_100ns"]

    def send(self, role, command):
        append(self.root / "operator-actions.jsonl", {"at_utc": utc(), "role": role, "command": command})
        self.child.stdin.write(command + "\n")
        self.child.stdin.flush()
        if command == "stop":
            self.stop_sent = True

    def before_capture(self, role, process, capture):
        pass

    def controller_started(self, role, capture):
        pass

    def handle_operation(self, role, operation):
        return False

    def before_controller_loss(self, role):
        pass

    def capture_finished(self, role):
        pass

    def controller_arguments(self, role):
        return []

    def capture(self, stage):
        role = stage["role"]
        print(f"\n[{role}] Prepare a fresh user-started game process. No game is launched by this executor.")
        for instruction in stage["instructions"]:
            print("  " + instruction)
        if self.live_blocked:
            self.event("capture_not_run", role=role, reason="Earlier restoration/process identity uncertain; no further live changes")
            return
        while True:
            action = prompt(f"Enter 'start {role}' to capture, or 'skip <reason>'. Scene preparation is manual.")
            if action.startswith("skip ") and action[5:].strip():
                self.event("capture_skipped", role=role, reason=action[5:])
                obligations = (["W01.default_51"] if role == "default" else
                               ["W11.production_compare", "W11.scenario_basis"] if role == "secondary" else
                               ["W13.controller_loss"] if role == "controller-loss" else
                               [key for key in self.ledger.items if key not in
                                {"W01.default_51", "W11.production_compare", "W11.scenario_basis", "W13.controller_loss"}])
                for obligation in obligations:
                    if self.ledger.items[obligation]["status"] not in ("PASS", "FAIL"):
                        self.ledger.record(obligation, "NOT_COVERED", f"{role} skipped: {action[5:]}", [self.root / "timeline.jsonl"])
                return
            if action == f"start {role}":
                break
        before = self.process_snapshot()
        key = self.process_key(before)
        if key in self.processes or any(name.lower() == "isaaccst0.native.dll" for name in before["modules"]):
            raise RuntimeError("A fresh process is required; no reinjection into an earlier/resident instance")
        self.processes.add(key)
        directory = self.root / "captures" / role
        directory.mkdir(parents=True)
        save(directory / "process-before.json", before)
        capture = directory / "capture"  # Created exclusively by the controller.
        self.before_capture(role, before, capture)
        world = role != "default"
        argv = [str(self.controller), "coop-active", "--game-exe", str(Path(self.args.game_exe).resolve()),
                "--native-dll", str(self.native), "--helper", str(self.helper), "--artifact-dir", str(capture),
                "--script", stage["script"], "--authorization", self.args.authorization,
                "--b-character", self.args.b_character]
        if world:
            argv += ["--world-record", "min-read-v1"]
        argv += self.controller_arguments(role)
        save(directory / "controller-invocation.json", {"argv": argv, "at_utc": utc(), "process": before})
        self.event("capture_start", role=role, process=before)
        self.stop_sent = False
        lost = False
        progress = LiveProgress(capture / "world") if world else None
        with (directory / "controller.log").open("xb") as output:
            self.child = subprocess.Popen(argv, stdin=subprocess.PIPE, stdout=output, stderr=subprocess.STDOUT,
                                          text=True, encoding="utf-8",
                                          creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
            self.controller_started(role, capture)
            print(f"Controller pid={self.child.pid}; live output: {directory / 'controller.log'}", flush=True)
            print("Commands: mark <scene>, note <text>, progress, status, script, queries, conflict-b, retire-b, stop.")
            if role == "controller-loss":
                print("This last stage also accepts lose-controller: terminate only this controller child; recovery stays unconfirmed until evidenced.")
            while self.child.poll() is None:
                operation = prompt(f"[{role}] Perform only authorized game actions; enter a marker/command.")
                if self.child.poll() is not None:
                    break
                if operation.startswith("note ") and operation[5:].strip():
                    self.event("operator_note", role=role, note=operation[5:])
                elif operation == "progress":
                    observed = progress.read() if progress else {"status": "DEFAULT_MODE_NO_WORLD"}
                    self.event("progress", role=role, observation=observed)
                    print(observed, flush=True)
                elif (operation.startswith("mark ") and operation[5:].strip()) or operation in FORWARD:
                    self.send(role, operation)
                    if operation == "stop":
                        break
                elif operation == "lose-controller" and role == "controller-loss":
                    observed = progress.read()
                    if observed.get("ordinary_captures", 0) == 0:
                        print("No saved ordinary capture yet; controller loss was not triggered. Wait for initialization, then inspect progress.", flush=True)
                        self.event("controller_loss_not_triggered", reason="No saved ordinary capture")
                        continue
                    self.before_controller_loss(role)
                    self.event("controller_loss_requested", role=role, pid=self.child.pid)
                    self.child.terminate()  # Only the Popen-owned controller; never the game/helper.
                    lost = True
                    break
                elif self.handle_operation(role, operation):
                    continue
                else:
                    print("Unknown command; nothing sent.", flush=True)
            try:
                code = self.child.wait(timeout=120)
            except subprocess.TimeoutExpired as exc:
                self.live_blocked = True
                self.event("controller_wait_timeout", role=role, pid=self.child.pid, stop_sent=self.stop_sent,
                           note="No kill, second stop, reinjection or unload attempted")
                raise RuntimeError("Controller has not completed; recovery is unconfirmed") from exc
            self.child.stdin.close()
            self.child = None
        save(directory / "controller-exit.json", {"exit_code": code, "controller_loss_requested": lost, "at_utc": utc()})
        self.event("capture_end", role=role, exit_code=code, controller_loss_requested=lost)
        self.captures[role] = {"directory": capture, "process": before, "exit_code": code, "controller_lost": lost}
        try:
            after = self.process_snapshot()
            save(directory / "process-after.json", after)
            same = self.process_key(after) == key
            self.captures[role]["process_after"] = after
        except (OSError, ValueError, RuntimeError, KeyError) as exc:
            same = False
            save(directory / "process-after-error.json", {"error": str(exc), "at_utc": utc()})
        if lost:
            self.live_blocked = True
            self.event("loss_recovery_unconfirmed", role=role,
                       note="No final controller result/raw restoration is inferred; attach actual external recovery evidence for W13")
            self.capture_finished(role)
            return
        restored = restoration(capture, 70 if world else 51)
        self.live_blocked |= not same or not restored
        self.captures[role]["same_process_after"] = same
        report = load(capture / "result.json")
        if role == "default":
            invocation = load(capture / "invocation.json")
            # The legacy controller also evaluates a larger historical input
            # suite. Preserve NOT_OBSERVED/exit!=0; don't turn it into that suite's PASS.
            passed = (restored and same and not (capture / "world").exists() and invocation["coopIpcVersion"] == 3
                      and report.get("nativeError") == 0 and report.get("transportFailure") is None
                      and report["status"] in ("PASS", "NOT_OBSERVED"))
            self.ledger.require("W01.default_51", passed,
                                f"Default-mode subset only; legacy full-suite status={report['status']}, exit={code} retained", [capture])
        elif role == "primary":
            invocation = load(capture / "invocation.json")
            manifest = load(capture / "world" / "manifest.json")
            self.ledger.items["W01.optional_70"]["method"] = "AUTOMATED"
            self.ledger.require("W01.optional_70",
                                restored and same and code == 0 and report["status"] == "CAPTURE_CLOSED"
                                and invocation["coopIpcVersion"] == 4 and manifest["hook_slots"] == 70,
                                "Optional capture must close and restore all 70 unique targets", [capture])
        elif role == "secondary" and (not restored or not same or code != 0 or report["status"] != "CAPTURE_CLOSED"):
            self.ledger.record("W11.production_compare", "FAIL", "Second capture failed or restoration/process identity is uncertain", [capture])
        self.capture_finished(role)

    def scan(self):
        primary = self.captures.get("primary")
        if not primary or primary["controller_lost"]:
            return
        world = primary["directory"] / "world"
        summary, self.primary_sample = scan_world(world, self.root / "primary-record-index.jsonl")
        self.summaries["primary"] = summary
        save(self.root / "primary-coverage.json", summary)
        evidence = [self.root / "primary-coverage.json", world]
        self.ledger.require("W02.record_invariants",
                            summary["counts"].get("records", 0) > 0 and not summary["invariant_failures"]
                            and len(summary["threads"]) == 1 and not summary["event_gaps"],
                            "Saved sample/event sequence, ordinary/odd kind and single-thread facts", evidence)
        self.ledger.require("W03.profile", not summary["profile_failures"] and summary["counts"].get("records", 0) > 0,
                            "Known-class objects retain every fixed-profile field, including unavailable states", evidence)
        if set(CLASSES[:8]).issubset(summary["readable_classes"]):
            self.ledger.record("W03.eight_classes", "PASS", "All eight classes observed with current VALUE fields", evidence)
        else:
            self.ledger.record("W03.eight_classes", "NOT_COVERED",
                               "Missing readable classes: " + ", ".join(sorted(set(CLASSES[:8]) - set(summary["readable_classes"]))), evidence)
        if set(range(13)).issubset(summary["sources"]) and summary["grid_door_alias"]:
            self.ledger.record("W03.sources_alias", "PASS", "All source statuses and a merged Grid/door object observed", evidence)
        for key, condition, reason in (
            ("W04.creation_events", all(summary["hook_enters"].get(hook, 0) for hook in ("Factory", "Init", "Publish", "Spawn"))
             and summary["event_kinds"].get("1", 0) > 0, "Factory/Init/Publish/Spawn and birth events actually present"),
            ("W06.bootstrap_witness", summary["bootstrap_witness"], "A retains FIRST_OBSERVED/birth unavailable with current-read evidence 3"),
        ):
            if condition:
                self.ledger.record(key, "PASS", reason, evidence)
        producer = summary["producer_result"]
        consumer_path = world / "consumer.json"
        consumer_config = load(world / "manifest.json").get("consumer", {})
        if consumer_config.get("world_mapping_version") == 2:
            consumer = load(consumer_path)
            consumer_ok = (
                consumer["world_mapping_version"] == 2 and consumer["reader"] == "DEDICATED_NAMED_EVENT"
                and consumer["shared_release"] == "RAW_COPY_COMPLETE_BEFORE_ENCODING"
                and consumer["queue_limit_bytes"] == 256 * 1024 * 1024 and consumer["queue_limit_items"] == 256
                and 0 <= consumer["queue_peak_bytes"] <= consumer["queue_limit_bytes"]
                and 0 <= consumer["queue_peak_items"] <= consumer["queue_limit_items"]
                and consumer["outstanding_bytes"] == 0 and consumer["outstanding_items"] == 0
                and consumer["reader_done"] is True and consumer["files_flushed"] is True and consumer["failure"] is None
                and number(consumer["notifications"]) > 0
                and number(consumer["batches_copied"]) == number(consumer["batches_written"]) == summary["counts"].get("records", 0) > 0
                and number(consumer["batches_written"]) == number(producer["attempts"]) - number(producer["backpressure"])
                and consumer["events_copied"] == consumer["events_written"] == producer["events_produced"] == producer["events_consumed"]
            )
            self.ledger.require("W07.consumer_pipeline", consumer_ok,
                                "Saved notification/copy/write counters and queue/flush/drain facts only; no latency or read-only proof inferred",
                                [consumer_path, world / "manifest.json", *evidence])
        else:
            self.ledger.record("W07.consumer_pipeline", "NOT_COVERED", "Capture has no mapping v2 consumer evidence", evidence)
        persisted = (number(producer["captures"]) == summary["counts"].get("capture", 0)
                     and producer["captures"] == summary["last_capture"]
                     and producer["events_produced"] == summary["last_event"]
                     and number(producer["attempts"]) >= number(summary["last_attempt"]))
        # Backpressure leaves an attempt hole, which resets OrdinaryWindow.
        # An explicitly recorded fault outside the selected interval cannot
        # erase a later complete ordinary interval; event/counter loss still fails.
        clean = producer["native_error"] == "NONE" and not summary["event_gaps"] and persisted
        self.ledger.require("W07.continuous_10000", summary["longest_full_profile"] >= 10000 and clean,
                            f"Longest full-profile ordinary interval={summary['longest_full_profile']}; "
                            f"explicit odd gaps excluded={summary['interval']['odd_gaps_not_counted']}; "
                            "saved event/counter coverage must remain complete", evidence)
        balanced = (producer["producer_closed"] is True and producer["callbacks"] == 0
                    and producer["native_error"] == "NONE" and producer["events_produced"] == producer["events_consumed"]
                    and isinstance(producer["hook_enters"], list) and len(producer["hook_enters"]) == 19
                    and producer["hook_enters"] == producer["hook_returns"]
                    and persisted and restoration(primary["directory"], 70)
                    and primary.get("same_process_after") is True
                    and any(name.lower() == "isaaccst0.native.dll" for name in primary.get("process_after", {}).get("modules", [])))
        self.ledger.require("W13.normal_restore", balanced, "Producer closed, callbacks/events drained, 19 entry counters balanced and 70 raw targets restored", evidence)

    def offline(self):
        primary = self.captures.get("primary")
        if not primary or primary["controller_lost"]:
            return
        runner = Offline(self, primary["directory"] / "world")
        if not self.args.skip_full_replay:
            runner.replay()
        else:
            self.ledger.record("W08.replay", "NOT_COVERED", "Full replay explicitly excluded by user; single-record decoder/reencoding remains enabled", [self.root / "manifest.json"])
        if self.primary_sample is not None:
            runner.mutations(self.primary_sample)
        else:
            self.event("owned_copy_cases_not_covered", reason="No unique mapped object with a readable finite position field in an ordinary capture")

    def cross_process(self):
        left, right = self.captures.get("primary"), self.captures.get("secondary")
        if (not left or not right or right["controller_lost"] or "primary" not in self.summaries
                or self.ledger.items["W11.production_compare"]["status"] == "FAIL"):
            return
        if self.process_key(left["process"]) == self.process_key(right["process"]):
            self.ledger.record("W11.production_compare", "FAIL", "Both captures refer to the same process instance", [])
            return
        summary, _ = scan_world(right["directory"] / "world", self.root / "secondary-record-index.jsonl")
        save(self.root / "secondary-coverage.json", summary)
        save(self.root / "alignment-candidates.json", {
            "scope": "Diagnostic candidates only; not an alignment",
            "left": {"process": left["process"], "scopes": self.summaries["primary"]["scopes"], "births": self.summaries["primary"]["births"]},
            "right": {"process": right["process"], "scopes": summary["scopes"], "births": summary["births"]},
            "rule": "Supply actual run/room/tick/creation mapping; no nearest-frame/address/position inference"})
        answer = prompt("Provide alignment.json path for these two actual captures, or 'skip <reason>'. See alignment-candidates.json and raw records.")
        if answer.startswith("skip "):
            self.event("comparison_skipped", reason=answer[5:])
            return
        source = Path(answer).expanduser().resolve()
        alignment = load(source)
        basis = prompt("Record the actual alignment basis (input/scenario/event evidence); this note alone is not W11.scenario_basis PASS.")
        if not basis:
            raise ValueError("Alignment basis must not be empty")
        directory = self.root / "cross-process"
        directory.mkdir()
        shutil.copyfile(source, directory / "alignment.json")
        save(directory / "alignment-provenance.json", {"source": str(source), "basis": basis,
                                                       "left_process": left["process"], "right_process": right["process"]})
        output = directory / "comparison"
        code = self.command("cross-process", [str(self.controller), "world-compare",
            "--left", str(left["directory"] / "world"), "--right", str(right["directory"] / "world"),
            "--alignment", str(directory / "alignment.json"), "--artifact-dir", str(output)])
        result = load(output / "result.json") if (output / "result.json").is_file() else {}
        legal = ("DIFFERENT", "INCOMPARABLE", "EQUAL_WITHIN_PROFILE")
        compared_count = 0
        statuses_valid = True
        if code == 0:
            for _, _, row in rows(output / "comparisons.jsonl"):
                compared_count += 1
                statuses_valid &= row["status"] in legal
        passed = (code == 0 and result.get("status") in legal and compared_count == len(alignment["ticks"])
                  and compared_count > 0 and statuses_valid and result.get("tick_pairs") == len(alignment["ticks"]))
        self.ledger.require("W11.production_compare", passed,
                            "CLI accepts only the explicit alignment; DIFFERENT/INCOMPARABLE are valid comparison conclusions, not global equality", [directory])

    def manual_review(self):
        template = self.root / "manual-review-template.json"
        print(f"\nManual review template: {template}")
        answer = prompt("Edit the template and enter 'review', provide another review JSON path, or 'skip <reason>'. PASS requires an observer, rationale and saved evidence.")
        if answer.startswith("skip "):
            self.event("manual_review_skipped", reason=answer[5:])
            return
        path = template if answer == "review" else Path(answer).expanduser().resolve()
        with path.open("rb") as source, (self.root / "manual-review-submitted.json").open("xb") as destination:
            shutil.copyfileobj(source, destination)
        reviews = load(path)
        if (set(reviews) != {"version", "reviews"} or type(reviews["version"]) is not int
                or reviews["version"] != 1 or not isinstance(reviews["reviews"], list)):
            raise ValueError("Manual review requires version=1 and reviews")
        seen = set()
        normalized = []
        for row in reviews["reviews"]:
            if set(row) != {"id", "status", "observer", "reason", "evidence"} or row["id"] not in MANUAL or row["id"] in seen:
                raise ValueError("Unknown/duplicate manual obligation or extra fields; automatic results cannot be overridden")
            seen.add(row["id"])
            if row["status"] not in ("PASS", "FAIL", "NOT_COVERED") or not isinstance(row["reason"], str) or not row["reason"].strip():
                raise ValueError("Manual status/rationale missing")
            if not isinstance(row["observer"], str):
                raise ValueError("Manual observer must be a string")
            if not isinstance(row["evidence"], list) or any(not isinstance(item, str) for item in row["evidence"]):
                raise ValueError("Manual evidence must be an array of file paths")
            if row["status"] == "PASS" and (not row["observer"].strip() or not row["evidence"]):
                raise ValueError("Manual PASS requires identified observer and evidence")
            if row["status"] == "PASS":
                if row["id"] == "W13.controller_loss":
                    if not self.captures.get("controller-loss", {}).get("controller_lost"):
                        raise ValueError("Controller-loss PASS requires the actual controller-loss action in this invocation")
                elif "primary" not in self.captures:
                    raise ValueError("Manual runtime PASS requires this invocation's actual primary capture")
                if row["id"] == "W11.scenario_basis" and not (self.root / "cross-process" / "alignment-provenance.json").is_file():
                    raise ValueError("Scenario/alignment PASS requires this invocation's explicit cross-process comparison")
            copied = []
            for item in row["evidence"]:
                origin = path_from(self.root, item)
                if not origin.is_file():
                    raise ValueError(f"Missing evidence file: {origin}")
                if origin.is_relative_to(self.root):
                    destination = origin
                else:
                    destination = self.root / "manual-evidence" / f"{len(normalized):03}-{len(copied):03}-{origin.name}"
                    destination.parent.mkdir(exist_ok=True)
                    shutil.copyfile(origin, destination)
                copied.append(str(destination.relative_to(self.root)))
            normalized.append({**row, "evidence": copied})
        save(self.root / "manual-review-accepted.json", {"source": str(path), "reviews": normalized, "at_utc": utc()})
        for row in normalized:
            self.ledger.record(row["id"], row["status"], f"MANUAL_REVIEW by {row['observer']}: {row['reason']}", row["evidence"])

    def interrupted_cleanup(self):
        if self.child is None or self.child.poll() is not None:
            return
        pid = self.child.pid
        self.event("interrupted_with_controller", pid=pid, stop_already_sent=self.stop_sent)
        if not self.stop_sent:
            try:
                self.send("interrupted", "stop")  # One cooperative shutdown request, no force unload.
            except (OSError, ValueError):
                pass
        try:
            self.child.wait(timeout=120)
        except (subprocess.TimeoutExpired, KeyboardInterrupt):
            self.event("cleanup_unconfirmed", controller_pid=pid,
                       note="No further stop, kill, helper, unload or reinjection attempted")

    def record_completed_results(self):
        pass

    def finalize_evidence(self):
        """Summarize existing artifacts once, including stages interrupted before registration."""
        if self.evidence_finalization_started:
            return
        self.evidence_finalization_started = True
        errors = []

        def snapshot(path, required=False):
            relative = str(path.relative_to(self.root))
            try:
                if path.is_file():
                    return {"path": relative, "value": load(path)}
                if required:
                    errors.append({"path": relative, "error": "Required artifact missing"})
                return {"path": relative, "missing": True}
            except (OSError, ValueError) as exc:
                errors.append({"path": relative, "error": str(exc)})
                return {"path": relative, "error": str(exc)}

        directories = {role: value["directory"] for role, value in self.captures.items()}
        for directory in sorted((self.root / "captures").glob("*")):
            if directory.is_dir():
                directories.setdefault(directory.name, directory / "capture")
        stages = []
        instrumented = bool(getattr(self.args, "instrumented_remaining", False))
        for role, directory in directories.items():
            captured = self.captures.get(role, {})
            controller_lost = captured.get("controller_lost") is True
            stages.append({
                "role": role, "capture_registration_completed": role in self.captures,
                "exit_code": captured.get("exit_code"),
                "controller_lost": controller_lost,
                "controller_final_flush": "UNCONFIRMED_AFTER_CONTROLLED_LOSS" if controller_lost else "See raw results",
                "controller_exit": snapshot(directory.parent / "controller-exit.json"),
                "capture_result": snapshot(directory / "result.json", not controller_lost),
                "world_result": snapshot(directory / "world" / "result.json", role != "default" and not controller_lost),
                "probe_result": snapshot(directory / "e2e" / "result.json", instrumented and not controller_lost),
                "independent_restoration": snapshot(directory.parent / "external" / "after-normal-stop.json"),
            })
        commands = [{"name": directory.name, "exit": snapshot(directory / "exit.json")}
                    for directory in sorted((self.root / "commands").glob("*")) if directory.is_dir()]
        comparison = snapshot(self.root / "cross-process" / "comparison" / "result.json")
        provenance = snapshot(self.root / "cross-process" / "alignment-provenance.json")
        save(self.root / "completed-evidence.json", {
            "at_utc": utc(), "stages": stages, "commands": commands,
            "comparison": comparison, "alignment_provenance": provenance, "errors": errors,
            "scope": "Existing files only; raw errors/exit codes retained; no scene, validation, CLI or capture rerun",
            "manual_review": "No manual PASS is inferred from file existence or command exit alone",
        })
        self.command_failures.extend(f"Evidence summary {row['path']}: {row['error']}" for row in errors)
        self.record_completed_results()
        self.evidence_finalization_complete = True

    def inventory(self):
        inventory = self.root / "file-inventory.jsonl"
        for path in sorted(self.root.rglob("*")):
            if path.is_file() and path != inventory:
                stat = path.stat()
                append(inventory, {"path": str(path.relative_to(self.root)), "bytes": stat.st_size,
                                   "last_write_ns": str(stat.st_mtime_ns)})


def main(argv=None):
    parser = argparse.ArgumentParser(description="One T0-06 E2E invocation with explicit human scene checkpoints; never run as a preflight.")
    for option in ("controller", "native-dll", "helper", "game-exe", "artifact-dir", "authorization", "plan"):
        parser.add_argument("--" + option, required=True)
    parser.add_argument("--b-character", default="native")
    parser.add_argument("--powershell", default="pwsh")
    parser.add_argument("--continue-from", help="Read-only evidence from a prior interrupted invocation; no live stages are repeated")
    parser.add_argument("--skip-full-replay", action="store_true")
    parser.add_argument("--supplemental-capture", action="store_true", help="One fresh manual Familiar/Projectile scene after offline work")
    parser.add_argument("--live-remaining", action="store_true", help="New manual scenes and independent controlled-fault evidence; no offline cases repeated")
    parser.add_argument("--instrumented-remaining", action="store_true", help="tx-01-80 E2E Native, removable fixture and remaining ten obligations, one invocation")
    parser.add_argument("--game-log", help="Actual game log.txt containing the fixture startup/scene/input witness")
    args = parser.parse_args(argv)
    if not args.authorization.strip():
        parser.error("--authorization must record the actual user's operation scope")
    if os.name != "nt":
        parser.error("The live unified executor requires Windows")
    if not sys.stdin.isatty():
        parser.error("Interactive terminal required for the user-accepted human checkpoints")
    if args.continue_from and not args.skip_full_replay:
        parser.error("Continuation requires the explicitly scoped --skip-full-replay")
    if args.supplemental_capture and not args.continue_from:
        parser.error("Supplemental capture is a continuation option")
    if args.live_remaining and (not args.continue_from or args.supplemental_capture):
        parser.error("Live remaining requires --continue-from and cannot use --supplemental-capture")
    if args.instrumented_remaining and (not args.continue_from or not args.game_log or args.live_remaining or args.supplemental_capture):
        parser.error("Instrumented remaining requires prior tx-01-79 evidence and --game-log, with no other scene modes")
    if args.instrumented_remaining:
        from .instrumented import InstrumentedSuite
        suite = InstrumentedSuite(args, None)
    elif args.live_remaining:
        from .scenarios import SceneSuite
        suite = SceneSuite(args, None)
    else:
        suite = Suite(args, None)
    if args.continue_from and not (args.live_remaining or args.instrumented_remaining):
        suite.ledger.transaction = "tx-01-76"
        suite.ledger.scope = "Prior PASS evidence is explicitly carried forward; remaining cases execute once in this invocation"
    failure, interrupted, finalized = None, False, False
    result = {"status": "FAIL"}
    try:
        suite.prepare()
        if args.instrumented_remaining:
            suite.run_remaining()
        elif args.live_remaining:
            suite.run_remaining()
        elif args.continue_from:
            from .remaining import import_previous
            import_previous(suite, Path(args.continue_from).resolve())
        for stage in ([] if args.continue_from else suite.plan["captures"]):
            try:
                suite.capture(stage)
            except (OSError, ValueError, RuntimeError, KeyError, TypeError) as exc:
                suite.live_blocked = True
                suite.command_failures.append(f"{stage['role']}: {exc}")
                suite.event("capture_error", role=stage["role"], error=str(exc), no_further_live_changes=True)
                print(f"Capture error: {exc}. Continuing only available offline work.", flush=True)
                break
        operations = ([] if args.continue_from else [("scan", suite.scan)])
        if not (args.live_remaining or args.instrumented_remaining):
            operations += [("offline", suite.offline), ("cross-process", suite.cross_process)]
        if args.supplemental_capture:
            from .remaining import supplemental
            operations.append(("supplemental", lambda: supplemental(suite)))
        for name, operation in operations:
            try:
                operation()
            except (OSError, ValueError, RuntimeError, KeyError, TypeError) as exc:
                suite.command_failures.append(f"{name}: {exc}")
                suite.event("stage_error", stage=name, error=str(exc))
                print(f"{name}: {exc}; saved artifacts retained, no retry.", flush=True)
        suite.finalize_evidence()
        suite.manual_review()
    except KeyboardInterrupt:
        interrupted = True
    except Exception as exc:
        failure = f"{type(exc).__name__}: {exc}"
    finally:
        finalization_errors = []

        def finish_step(name, operation):
            nonlocal interrupted
            try:
                operation()
            except KeyboardInterrupt:
                interrupted = True
                finalization_errors.append({"stage": name, "type": "KeyboardInterrupt", "error": "Interrupted; no retry"})
            except Exception as exc:
                # Failure reporting must not prevent the other cleanup/report steps.
                finalization_errors.append({"stage": name, "type": type(exc).__name__, "error": str(exc)})

        if suite.child is not None:
            finish_step("controller_cleanup", suite.interrupted_cleanup)
        if not suite.evidence_finalization_started:
            finish_step("completed_evidence", suite.finalize_evidence)
        if args.live_remaining or args.instrumented_remaining:
            finish_step("observer_close", suite.close_observers)
        reasons = ([failure] if failure else []) + suite.command_failures + [
            f"{row['stage']}: {row['type']}: {row['error']}" for row in finalization_errors]
        failure = "; ".join(reasons) or None
        try:
            save(suite.root / "finalization-diagnostics.json", {
                "at_utc": utc(), "errors": finalization_errors,
                "evidence_summary_attempted": suite.evidence_finalization_started,
                "evidence_summary_completed": suite.evidence_finalization_complete,
                "original_or_stage_errors_preserved": failure, "automatic_retry": False,
            })
        except Exception as exc:
            detail = f"finalization_diagnostics: {type(exc).__name__}: {exc}"
            finalization_errors.append({"stage": "finalization_diagnostics", "type": type(exc).__name__, "error": str(exc)})
            failure = "; ".join(value for value in (failure, detail) if value)
        try:
            result = suite.ledger.export(failure, interrupted)
            suite.inventory()
            save(suite.root / "finalization.json", {"status": "COMPLETED", "at_utc": utc(), "result_status": result["status"],
                                                  "inventory": "file-inventory.jsonl", "automatic_retry": False})
            finalized = True
        except Exception as exc:
            print(f"Report finalization failed: {exc}; partial artifacts retained.", file=sys.stderr, flush=True)
            try:
                save(suite.root / "finalization-failure.json", {"status": "FAIL", "error": str(exc),
                    "original_or_stage_errors_preserved": failure, "finalization_errors": finalization_errors, "automatic_retry": False})
            except OSError:
                pass
    displayed_status = result["status"] if finalized else "FAIL_REPORT_FINALIZATION"
    print(f"\n{displayed_status}: {suite.root / 'result.json'}", flush=True)
    return 130 if interrupted else 1 if not finalized or result["status"] == "FAIL" else 2 if result["status"] == "INCOMPLETE" else 0
