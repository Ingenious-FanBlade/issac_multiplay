#!/usr/bin/env python3
"""tx-01-98 operation 9: analyze one real-Authority B-source-stop run.

This reader never contacts the game. It consumes the controller/Native event
stream, the independently written world-record JSONL, and the restoration
report from the same invocation. No hash is computed or validated.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import struct
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path

# Stable values from the current production event ABI. They are names here,
# not a second implementation of admission or scheduling.
EV_SUBMIT = 4
EV_RESOLVE = 5
EV_AUTHORITY_COMMIT = 8
EV_JOINED = 12
EV_RETIRED = 14
EV_RESTORED = 15
EV_FAILURE = 16
EV_SCRIPT_COMPLETE = 18
EV_AUTHORITY_SESSION_TRANSITION = 31
EV_AUTHORITY_TICK_BOUNDARY = 32
EV_CLIENT_INPUT_PROGRESS = 33
EV_AUTHORITY_TICK_RECEIPT = 34
EV_CLIENT_INPUT_AVAILABILITY = 35
EV_INPUT_RUNTIME_CONFIG = 36

DECISION_ACCEPTED = 1
TICK_STARTED = 1
TICK_COMMITTED = 2
TICK_STANDARD = 1
TICK_ODD_PLAYER = 2
POST_COMPLETE = 1
POST_GAP = 2
POST_BACKPRESSURE = 3
POST_UNAVAILABLE = 4
SOURCE_ACTIVE = 1
SOURCE_SILENT = 2
SOURCE_TIMED_OUT = 3
MISSING_NO_TARGET = 1
MISSING_FUTURE_ONLY = 2
SESSION_ACTIVE = 3
SESSION_FAILED = 6
STANDARD_RETURN_RVA = 0x2FBA52
ODD_RETURN_RVA = 0x555246
ODD_PLAYER_RETURN_RVA = 0x2FD60C


def load_json(path: Path):
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def write_json(path: Path, value) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write("\n")


def iter_jsonl(path: Path):
    malformed = 0
    with path.open("r", encoding="utf-8", errors="strict") as stream:
        for line_number, line in enumerate(stream, 1):
            text = line.strip()
            if not text:
                continue
            try:
                yield json.loads(text)
            except json.JSONDecodeError as exc:
                malformed += 1
                if malformed <= 8:
                    yield {"__malformed__": True, "line": line_number, "error": str(exc)}
    if malformed:
        yield {"__malformed_summary__": True, "count": malformed}


def state_axes_zero(state: dict) -> bool:
    return all(int(state.get(name, 0)) == 0 for name in
               ("held_flags", "edge_flags", "move_x", "move_y", "aim_x", "aim_y", "reserved"))


def state_nonzero(state: dict) -> bool:
    return not state_axes_zero(state)


def ordinary_state_equal(state: dict, expected: dict) -> bool:
    return all(int(state.get(name, 0)) == int(expected.get(name, 0)) for name in
               ("held_flags", "edge_flags", "move_x", "move_y", "aim_x", "aim_y"))


def runs_of(values) -> list[tuple[int, int, int]]:
    ordered = sorted(set(int(value) for value in values))
    if not ordered:
        return []
    result = []
    start = previous = ordered[0]
    for value in ordered[1:]:
        if value == previous + 1:
            previous = value
            continue
        result.append((start, previous, previous - start + 1))
        start = previous = value
    result.append((start, previous, previous - start + 1))
    return result


def interior_gaps(values) -> list[tuple[int, int, int]]:
    ordered = sorted(set(int(value) for value in values))
    result = []
    for left, right in zip(ordered, ordered[1:]):
        if right > left + 1:
            result.append((left + 1, right - 1, right - left - 1))
    return result


def field_float(obj: dict, field_id: str):
    for field in obj.get("fields", []):
        if field.get("field_id") != field_id:
            continue
        if field.get("status") != "VALUE" or "raw32" not in field:
            return None
        try:
            bits = int(field["raw32"], 16)
            return struct.unpack("<f", bits.to_bytes(4, "little"))[0]
        except (TypeError, ValueError, OverflowError, struct.error):
            return None
    return None


def player_position(record: dict, ordinal: int):
    for obj in record.get("objects", []):
        if obj.get("class") != "Player" or obj.get("ordinal") != ordinal:
            continue
        x = field_float(obj, "000b")
        y = field_float(obj, "000c")
        if x is None or y is None or not math.isfinite(x) or not math.isfinite(y):
            return None
        return (x, y)
    return None


def distance(left, right) -> float:
    return math.hypot(left[0] - right[0], left[1] - right[1])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--artifact-dir", required=True)
    args = parser.parse_args()
    root = Path(args.artifact_dir).resolve()
    report_dir = root / "report"
    report_dir.mkdir(parents=True, exist_ok=True)
    checks = []
    evidence = {}
    details = {}

    def check(check_id: str, description: str, passed: bool, detail=None, required=True):
        row = {"id": check_id, "description": description,
               "status": "PASS" if passed else ("FAIL" if required else "NOT_REQUIRED"),
               "required": bool(required)}
        if detail is not None:
            row["detail"] = detail
        checks.append(row)
        return passed

    try:
        required_files = [
            root / "plan.json", root / "script.json", root / "input-config.json",
            root / "orchestrator.json", root / "controller.stdout.log",
            root / "capture" / "events.jsonl", root / "capture" / "result.json",
            root / "capture" / "restoration.json",
            root / "capture" / "observation-metadata.json",
            root / "capture" / "world" / "captures.jsonl",
            root / "capture" / "world" / "result.json",
        ]
        missing_files = [str(path.relative_to(root)) for path in required_files if not path.is_file()]
        if missing_files:
            raise RuntimeError("missing required artifact files: " + ", ".join(missing_files))

        plan = load_json(root / "plan.json")
        script = load_json(root / "script.json")
        configured = load_json(root / "input-config.json")
        orchestrator = load_json(root / "orchestrator.json")
        controller_result = load_json(root / "capture" / "result.json")
        restoration = load_json(root / "capture" / "restoration.json")
        world_result = load_json(root / "capture" / "world" / "result.json")
        observation = load_json(root / "capture" / "observation-metadata.json")

        malformed_events = []
        malformed_world = []
        b_submits = []
        b_submit_rejected = []
        b_resolve = defaultdict(list)
        a_resolve = defaultdict(list)
        b_availability = {}
        a_receipts = defaultdict(list)
        b_receipts = defaultdict(list)
        committed_boundaries = defaultdict(list)
        authority_commits = defaultdict(list)
        config_events = []
        joined_events = []
        retired_events = []
        restored_events = []
        failure_events = []
        script_complete_events = []
        session_transitions = []
        lifecycle_events = []
        progress_rows = []
        duplicate_availability = 0
        duplicate_receipts = 0

        for event in iter_jsonl(root / "capture" / "events.jsonl"):
            if event.get("__malformed__") or event.get("__malformed_summary__"):
                malformed_events.append(event)
                continue
            kind = int(event.get("kind", 0))
            ordinal = int(event.get("ordinal", 0xFFFFFFFF))
            tick = int(event.get("tick", 0))
            if kind == EV_SUBMIT and ordinal == 1:
                if int(event.get("decision", 0)) == DECISION_ACCEPTED:
                    b_submits.append(event)
                else:
                    b_submit_rejected.append(event)
            elif kind == EV_RESOLVE and ordinal == 1:
                b_resolve[tick].append(event)
            elif kind == EV_RESOLVE and ordinal == 0:
                a_resolve[tick].append(event)
            elif kind == EV_CLIENT_INPUT_AVAILABILITY and ordinal == 1:
                if tick in b_availability:
                    duplicate_availability += 1
                b_availability[tick] = event
            elif kind == EV_AUTHORITY_TICK_RECEIPT and ordinal == 0:
                a_receipts[tick].append(event)
            elif kind == EV_AUTHORITY_TICK_RECEIPT and ordinal == 1:
                b_receipts[tick].append(event)
                duplicate_receipts += 1 if len(b_receipts[tick]) > 1 else 0
            elif kind == EV_AUTHORITY_TICK_BOUNDARY and int(event.get("decision", 0)) == TICK_COMMITTED:
                committed_boundaries[tick].append(event)
            elif kind == EV_AUTHORITY_COMMIT:
                authority_commits[tick].append(event)
            elif kind == EV_INPUT_RUNTIME_CONFIG:
                config_events.append(event)
            elif kind == EV_JOINED:
                joined_events.append(event)
            elif kind == EV_RETIRED and ordinal == 1:
                retired_events.append(event)
            elif kind == EV_RESTORED:
                restored_events.append(event)
            elif kind == EV_FAILURE:
                failure_events.append(event)
            elif kind == EV_SCRIPT_COMPLETE and ordinal == 1:
                script_complete_events.append(event)
            elif kind == EV_AUTHORITY_SESSION_TRANSITION:
                session_transitions.append(event)
            elif kind == 13:
                lifecycle_events.append(event)
            elif kind == EV_CLIENT_INPUT_PROGRESS:
                progress_rows.append(event)

        world_positions = {"A": {}, "B": {}}
        world_capture_rows = 0
        world_capture_records = 0
        complete_world_capture_rows = 0
        partial_world_capture_rows = 0
        world_bad_completeness_flags = 0
        world_capture_reasons = Counter()
        world_player_ready = Counter()
        for record in iter_jsonl(root / "capture" / "world" / "captures.jsonl"):
            if record.get("__malformed__") or record.get("__malformed_summary__"):
                malformed_world.append(record)
                continue
            world_capture_rows += 1
            if record.get("record") != "capture":
                continue
            world_capture_records += 1
            complete = record.get("complete") is True
            reason = record.get("reason")
            if not isinstance(record.get("complete"), bool) or not isinstance(reason, str):
                world_bad_completeness_flags += 1
            world_capture_reasons[str(reason)] += 1
            if complete:
                complete_world_capture_rows += 1
            else:
                partial_world_capture_rows += 1
            tick_text = record.get("authority_tick")
            if tick_text is None:
                continue
            tick = int(tick_text)
            # Incomplete world records are not promoted to complete snapshots,
            # but an available Player field may still be used as explicitly
            # partial world evidence for the A action in this operation.
            for label, ordinal in (("A", 0), ("B", 1)):
                position = player_position(record, ordinal)
                if position is None:
                    world_player_ready[label] += 1
                else:
                    world_positions[label][tick] = position

        # 1. Invocation/controller independently complete.
        controller_ok = (
            orchestrator.get("status") == "STOPPED" and
            orchestrator.get("phase") == "script_complete_and_final_target_effective" and
            orchestrator.get("controller_stop_sent") is True and
            orchestrator.get("final_target_was_effective") is True and
            int(orchestrator.get("last_committed_tick", -1)) >
                int(orchestrator.get("max_b_accepted_target", 0)) and
            orchestrator.get("wait_timed_out") is False and
            orchestrator.get("extra_signal_sent") is False and
            not (orchestrator.get("parse_errors") or []) and
            orchestrator.get("failure_event") is None and
            int(orchestrator.get("controller_exit", -1)) == 0 and
            controller_result.get("status") == "CAPTURE_CLOSED")
        check("controller_completed", "The single controller stopped only after the final accepted target became effective, exited 0, and reported CAPTURE_CLOSED.",
              controller_ok, {"orchestrator_status": orchestrator.get("status"),
                              "phase": orchestrator.get("phase"),
                              "controller_exit": orchestrator.get("controller_exit"),
                              "final_target_was_effective": orchestrator.get("final_target_was_effective"),
                              "last_committed_tick": orchestrator.get("last_committed_tick"),
                              "max_b_accepted_target": orchestrator.get("max_b_accepted_target"),
                              "controller_status": controller_result.get("status")})

        # 2. Native 70-slot transaction.
        valid_restored = [event for event in restored_events
                          if int(event.get("action", 0xFFFFFFFF)) != 0xFFFFFFFF and
                          int(event.get("query", 0)) != 0 and
                          int(event.get("boolean", 0)) == 1 and
                          len(event.get("before") or []) == 4 and
                          event.get("before") == event.get("after")]
        restored_keys = {(int(event.get("action", -1)), int(event.get("query", -1)))
                         for event in valid_restored}
        catalog_rows = [event for event in restored_events
                        if int(event.get("action", 0)) == 0xFFFFFFFF and int(event.get("detail", 0)) == 70
                        and int(event.get("boolean", 0)) == 1]
        restoration_ok = (restoration.get("nativeRestored") is True and
                          restoration.get("pinned") is True and
                          restoration.get("dllResidentExpected") is True and
                          len(valid_restored) == 70 and len(restored_keys) == 70 and
                          len(catalog_rows) == 1 and
                          int(controller_result.get("restoredTargets", -1)) == 70 and
                          controller_result.get("nativeRestored") is True and
                          controller_result.get("transportFailure") is None and
                          int(controller_result.get("nativeError", -1)) == 0)
        check("native_70_slot_restoration", "All 70 unique Native targets restored with unchanged bytes; the resident DLL remains expected.",
              restoration_ok, {"valid_restored_rows": len(valid_restored),
                               "unique_restored_targets": len(restored_keys),
                               "catalog_rows": len(catalog_rows),
                               "restored_targets_report": controller_result.get("restoredTargets")})

        # 3. World consumer closed without a hidden gap/failure.
        consumer = world_result.get("consumer") or {}
        world_ok = (world_result.get("status") == "CAPTURE_CLOSED" and
                    world_result.get("native_error") == "NONE" and
                    world_result.get("producer_closed") is True and
                    consumer.get("failure") is None and
                    consumer.get("reader_done") is True and
                    int(world_result.get("captures", 0)) > 0 and
                    int(world_result.get("events_consumed", -1)) == int(world_result.get("events_produced", -2)))
        check("world_consumer_closed", "The same run's world-record mapping/consumer closed with no producer/consumer failure.",
              world_ok, {"world_status": world_result.get("status"),
                         "captures": world_result.get("captures"),
                         "backpressure": world_result.get("backpressure"),
                         "events_consumed": world_result.get("events_consumed"),
                         "events_produced": world_result.get("events_produced")})
        world_completeness_explicit = (
            world_capture_records > 0 and world_bad_completeness_flags == 0 and
            all(reason not in ("None", "") for reason in world_capture_reasons))
        check("world_capture_completeness_recorded", "Partial/complete world capture status and reasons are explicit; no incomplete sample is promoted to a complete world snapshot.",
              world_completeness_explicit,
              {"capture_records": world_capture_records,
               "complete_captures": complete_world_capture_rows,
               "partial_captures": partial_world_capture_rows,
               "reasons": dict(world_capture_reasons)})

        # 4. Actual configuration entered Native.
        expected_config = plan["input_config"]
        config_ok = len(config_events) == 1
        config_detail = {}
        if config_ok:
            event = config_events[0]
            before = event.get("before") or []
            after = event.get("after") or []
            config_detail = {
                "mapping_version": event.get("query"),
                "input_lead_ticks": event.get("command"),
                "future_window_ticks": event.get("target"),
                "max_input_age_ticks": event.get("generation"),
                "script_drain_budget": event.get("action"),
                "admission_attempt_budget": event.get("decision"),
                "config_version": event.get("detail"),
                "boolean": event.get("boolean"),
                "ordinal": event.get("ordinal"),
                "before": before,
                "after": after,
            }
            config_ok = (
                configured == expected_config and
                int(event.get("ordinal", 0)) == 0xFFFFFFFF and
                int(event.get("query", 0)) == 5 and
                int(event.get("command", 0)) == int(expected_config["input_lead_ticks"]) and
                int(event.get("target", 0)) == int(expected_config["future_window_ticks"]) and
                int(event.get("generation", 0)) == int(expected_config["max_input_age_ticks"]) and
                int(event.get("action", 0)) == int(expected_config["script_drain_budget"]) and
                int(event.get("decision", 0)) == int(expected_config["admission_attempt_budget"]) and
                int(event.get("detail", 0)) == 1 and int(event.get("boolean", 0)) == 1 and
                len(before) == 4 and int(before[0]) == 1 and int(before[1]) == 48 and
                int(before[2]) == 0 and int(before[3]) == 0 and
                len(after) == 4 and all(int(value) == 0 for value in after))
        check("native_runtime_config", "EventKind36 proves the explicit v5 configuration was frozen and used by Native.",
              config_ok, config_detail)

        # 5. Session/B identity stayed active across the experiment.
        active_transition = any(int(event.get("decision", -1)) == SESSION_ACTIVE for event in session_transitions)
        failed_transition = any(int(event.get("decision", -1)) == SESSION_FAILED for event in session_transitions)
        failure_ok = not failure_events and not failed_transition
        check("authority_session_healthy", "AuthoritySession became Active and no failure transition/EventKind16 occurred.",
              active_transition and failure_ok and bool(joined_events),
              {"joined_events": len(joined_events), "failure_events": len(failure_events),
               "failed_transition": failed_transition})

        # Establish the four intended stop windows from independent production rows.
        b_submits.sort(key=lambda event: int(event.get("published", 0)))
        accepted_source_ticks = [int(event["tick"]) for event in b_submits]
        unique_source_ticks = sorted(set(accepted_source_ticks))
        accepted_targets = [int(event["target"]) for event in b_submits]
        source_gaps = interior_gaps(unique_source_ticks)
        target_gaps = interior_gaps(accepted_targets)
        expected_gap_lengths = [int(value) for value in plan["expected_source_stop_lengths"]]

        # The normal command target/source-tick relation is measured, not assumed.
        deltas = [int(event["target"]) - int(event["tick"]) for event in b_submits]
        delta_counts = Counter(deltas)
        expected_lead = int(expected_config["input_lead_ticks"])
        normal_delta = delta_counts.most_common(1)[0][0] if delta_counts else None
        # The first startup command is target completed+1 (one tick closer)
        # while every later normal/script command uses the configured lead.
        delta_consistent = (
            normal_delta == expected_lead and len(deltas) >= 2 and
            delta_counts.get(expected_lead, 0) == len(deltas) - 1 and
            delta_counts.get(expected_lead - 1, 0) == 1)
        startup_delta_rows = [row for row, value in zip(b_submits, deltas) if value != expected_lead]

        expected_source_ticks = len(script.get("steps", []))
        expected_accepted_commands = expected_source_ticks + 1  # startup emits completed+1 plus the nominal target once
        source_gap_ok = (
            [run[2] for run in source_gaps] == expected_gap_lengths and
            len(source_gaps) == len(expected_gap_lengths) and
            bool(b_submits) and len(unique_source_ticks) == expected_source_ticks and
            len(set(accepted_targets)) == expected_accepted_commands and
            (source_gaps[0][0] - min(unique_source_ticks)) >= int(plan["minimum_stable_prefix_ticks"]) and
            (max(unique_source_ticks) - source_gaps[-1][1]) >= int(plan["minimum_stable_recovery_ticks"]) and
            delta_consistent)
        check("b_source_stop_windows", "B's production script source emitted no command for exactly four interior windows of 1/5/30/120 Authority ticks.",
              source_gap_ok, {"source_gaps": source_gaps, "expected": expected_gap_lengths,
                              "accepted_commands": len(accepted_source_ticks),
                              "unique_source_ticks": len(unique_source_ticks),
                              "unique_targets": len(set(accepted_targets)),
                              "target_submit_delta": normal_delta,
                              "expected_lead_ticks": expected_lead,
                              "startup_delta_rows": len(startup_delta_rows)})

        # Effective missing is derived from the committed B availability snapshots.
        if b_availability:
            analysis_start = min(tick for tick, event in b_availability.items()
                                 if int(event.get("detail", 0)) & 1)
            analysis_end = max(accepted_targets)
        else:
            analysis_start = analysis_end = 0
        in_window = [tick for tick in b_availability if analysis_start <= tick <= analysis_end]
        availability_shape_failures = []
        for tick in in_window:
            event = b_availability[tick]
            flags = int(event.get("detail", 0))
            effective = bool(flags & 1)
            neutral = bool(flags & 2)
            decision = int(event.get("decision", -1))
            if effective and (neutral or decision != 0):
                availability_shape_failures.append({"tick": tick, "reason": "effective_shape",
                                                    "flags": flags, "decision": decision})
            if not effective and (not neutral or decision not in (MISSING_NO_TARGET, MISSING_FUTURE_ONLY)):
                availability_shape_failures.append({"tick": tick, "reason": "neutral_shape",
                                                    "flags": flags, "decision": decision})
        missing_ticks = [tick for tick in in_window
                         if (int(b_availability[tick].get("detail", 0)) & 1) == 0 and
                         (int(b_availability[tick].get("detail", 0)) & 2) != 0]
        effective_gaps = runs_of(missing_ticks)
        # With normal lead, the last missing tick can already have received the
        # resumed command for the next target. That is FutureOnly, never a
        # current-tick effective input. Earlier missing ticks have no target and
        # no future command. For a one-tick gap, that one tick is FutureOnly.
        missing_reason_shapes = []
        missing_reason_ok = True
        for start, end, length in effective_gaps:
            reasons = [int(b_availability[tick].get("decision", -1))
                       for tick in range(start, end + 1)]
            expected_reasons = [MISSING_NO_TARGET] * (length - 1) + [MISSING_FUTURE_ONLY]
            missing_reason_shapes.append({"start": start, "end": end, "length": length,
                                          "reasons": reasons, "expected": expected_reasons})
            missing_reason_ok = missing_reason_ok and reasons == expected_reasons
        neutral_flags_ok = all(
            (int(event.get("detail", 0)) & 1) == 0 and
            (int(event.get("detail", 0)) & 2) != 0
            for tick, event in b_availability.items() if tick in set(missing_ticks))
        effective_gap_ok = (
            [run[2] for run in effective_gaps] == expected_gap_lengths and
            len(effective_gaps) == len(expected_gap_lengths) and
            missing_reason_ok and neutral_flags_ok and
            duplicate_availability == 0 and not availability_shape_failures and
            len(missing_ticks) == sum(expected_gap_lengths))
        check("effective_b_missing_windows", "The production tick receipts show exactly four effective neutral windows of 1/5/30/120 ticks; NoTargetCommand and the lead-induced final FutureOnly are distinct.",
              effective_gap_ok, {"analysis_start": analysis_start, "analysis_end": analysis_end,
                                 "effective_gaps": effective_gaps, "missing_ticks": len(missing_ticks),
                                 "expected": expected_gap_lengths,
                                 "missing_reason_shapes": missing_reason_shapes,
                                 "availability_shape_failures": availability_shape_failures[:8],
                                 "duplicate_availability_rows": duplicate_availability})
        lifecycle_in_window = [event for event in lifecycle_events
                               if analysis_start <= int(event.get("tick", 0)) <= analysis_end]
        check("stable_run_and_room", "No pause/loading/room/death/respawn/rebind/teardown lifecycle invalidation intersects the B-stop and A-action window.",
              not lifecycle_in_window,
              {"lifecycle_events": lifecycle_in_window[:8]})

        # Align source and effective gaps; prove the queued leading command drained naturally.
        alignment_rows = []
        alignment_ok = len(source_gaps) == len(effective_gaps) == len(expected_gap_lengths) and normal_delta is not None
        for source_gap, effective_gap in zip(source_gaps, effective_gaps):
            source_start, source_end, source_length = source_gap
            effective_start, effective_end, effective_length = effective_gap
            last_before = [event for event in b_submits if int(event["tick"]) < source_start]
            first_after = [event for event in b_submits if int(event["tick"]) > source_end]
            last_before = max(last_before, key=lambda event: int(event["tick"])) if last_before else None
            first_after = min(first_after, key=lambda event: int(event["tick"])) if first_after else None
            prior_availability = b_availability.get(effective_start - 1)
            prior_effective = prior_availability is not None and bool(int(prior_availability.get("detail", 0)) & 1)
            row = {
                "source_start": source_start, "source_end": source_end, "source_length": source_length,
                "effective_start": effective_start, "effective_end": effective_end,
                "effective_length": effective_length,
                "source_to_effective_shift": effective_start - source_start,
                "last_source_tick_before_gap": int(last_before["tick"]) if last_before else None,
                "last_target_before_gap": int(last_before["target"]) if last_before else None,
                "first_source_tick_after_gap": int(first_after["tick"]) if first_after else None,
                "first_target_after_gap": int(first_after["target"]) if first_after else None,
                "prior_effective_available": prior_effective,
                "sequence_continuous": bool(last_before and first_after and
                                            int(first_after["command"]) == int(last_before["command"]) + 1),
            }
            row["aligned"] = (
                source_length == effective_length and
                effective_start - source_start == normal_delta and
                last_before is not None and first_after is not None and
                int(last_before["target"]) == effective_start - 1 and
                int(first_after["target"]) == effective_end + 1 and
                prior_effective and row["sequence_continuous"])
            alignment_ok = alignment_ok and row["aligned"]
            alignment_rows.append(row)
        check("source_to_effective_alignment", "Each source stop maps by the measured lead to the same-length effective gap; the one queued leading command is consumed before neutrality.",
              alignment_ok, alignment_rows)

        # Target stream gaps must equal effective missing ticks and source gap lengths.
        target_gap_ok = (target_gaps == effective_gaps and
                         [run[2] for run in target_gaps] == expected_gap_lengths)
        check("target_stream_gaps", "The accepted B target stream has only the four scheduled gaps and those targets are the effective missing ticks.",
              target_gap_ok, {"target_gaps": target_gaps, "effective_gaps": effective_gaps})

        # 6. Every tick, including each missing B tick, has a committed Authority boundary.
        boundary_failures = []
        boundary_kinds = Counter()
        for tick in in_window:
            rows = committed_boundaries.get(tick, [])
            if len(rows) != 1:
                boundary_failures.append({"tick": tick, "reason": "boundary_count", "count": len(rows)})
                continue
            event = rows[0]
            kind = int(event.get("query", 0))
            boundary_kinds[kind] += 1
            expected_rva = STANDARD_RETURN_RVA if kind == TICK_STANDARD else ODD_RETURN_RVA
            expected_player_rva = 0 if kind == TICK_STANDARD else ODD_PLAYER_RETURN_RVA
            after = event.get("after") or []
            before = event.get("before") or []
            completed = ((int(before[0]) | (int(before[1]) << 32)) if len(before) == 4 else -1)
            current = ((int(before[2]) | (int(before[3]) << 32)) if len(before) == 4 else -1)
            if (kind not in (TICK_STANDARD, TICK_ODD_PLAYER) or
                    int(event.get("tick", -1)) != tick or completed != tick or current != 0 or
                    int(event.get("action", -1)) != expected_rva or
                    int(event.get("detail", -1)) != expected_player_rva or
                    len(after) != 4 or int(after[0]) == 0 or int(after[0]) != int(after[1]) or
                    int(after[2]) == 0 or int(after[3]) != 0):
                boundary_failures.append({"tick": tick, "reason": "boundary_fields", "event": event})
        check("real_authority_ticks_continue", "Every tick in the B-stop interval has one committed standard/odd Native boundary with exact source RVA and main-thread authority.",
              bool(in_window) and not boundary_failures,
              {"ticks": len(in_window), "kinds": dict(boundary_kinds),
               "failures": boundary_failures[:8]})

        # 7. Commit/post-state and both local receipts exist independently of B input.
        commit_failures = []
        receipt_failures = []
        post_counts = Counter()
        for tick in in_window:
            boundaries = committed_boundaries.get(tick, [])
            boundary_kind = int(boundaries[0].get("query", 0)) if boundaries else 0
            commits = authority_commits.get(tick, [])
            commit_status = None
            commit_reason = None
            if len(commits) != 1:
                commit_failures.append({"tick": tick, "reason": "commit_count", "count": len(commits)})
            else:
                event = commits[0]
                after = event.get("after") or []
                commit_status = int(after[0]) if len(after) == 4 else -1
                commit_reason = int(after[1]) if len(after) == 4 else -1
                post_counts[commit_status] += 1
                if (int(event.get("tick", -1)) != tick or int(event.get("detail", 0)) != boundary_kind or
                        commit_status not in (POST_COMPLETE, POST_GAP, POST_BACKPRESSURE) or
                        commit_status == POST_UNAVAILABLE):
                    commit_failures.append({"tick": tick, "reason": "post_state", "event": event})
                if boundary_kind == TICK_ODD_PLAYER and commit_status != POST_GAP:
                    commit_failures.append({"tick": tick, "reason": "odd_not_gap", "status": commit_status})
            if (len(a_receipts.get(tick, [])) != 1 or len(b_receipts.get(tick, [])) != 1 or
                    duplicate_receipts != 0):
                receipt_failures.append({"tick": tick, "a": len(a_receipts.get(tick, [])),
                                         "b": len(b_receipts.get(tick, [])),
                                         "duplicate_b_receipts": duplicate_receipts})
            for label, rows in (("A", a_receipts.get(tick, [])), ("B", b_receipts.get(tick, []))):
                if len(rows) != 1:
                    continue
                receipt = rows[0]
                if (int(receipt.get("tick", -1)) != tick or
                        int(receipt.get("query", -1)) != boundary_kind or
                        int(receipt.get("detail", -1)) != commit_status or
                        int(receipt.get("action", -1)) != commit_reason or
                        int(receipt.get("boolean", 0)) != 1 or
                        len(receipt.get("before") or []) != 4 or
                        len(receipt.get("after") or []) != 4):
                    receipt_failures.append({"tick": tick, "client": label, "reason": "receipt_fields",
                                             "receipt": receipt})
        check("post_state_commit_and_receipts", "Each Authority tick has a commit/post-state result and exactly one A plus one B local receipt; B missing never blocks A or the tick.",
              bool(in_window) and not commit_failures and not receipt_failures,
              {"post_state_counts": dict(post_counts), "commit_failures": commit_failures[:8],
               "receipt_failures": receipt_failures[:8], "duplicate_b_receipts": duplicate_receipts,
               "duplicate_availability_rows": duplicate_availability})

        # 8. B effective neutrality, no old edge retention/replay.
        neutral_failures = []
        for tick in missing_ticks:
            resolves = b_resolve.get(tick, [])
            availability = b_availability.get(tick)
            if len(resolves) != 1:
                neutral_failures.append({"tick": tick, "reason": "resolve_count", "count": len(resolves)})
                continue
            state = resolves[0].get("state") or {}
            if not state_axes_zero(state):
                neutral_failures.append({"tick": tick, "reason": "nonzero_state", "state": state})
            if (availability is None or (int(availability.get("detail", 0)) & 1) != 0 or
                    (int(availability.get("detail", 0)) & 2) == 0):
                neutral_failures.append({"tick": tick, "reason": "availability_not_neutral",
                                         "availability": availability})
        check("b_immediate_neutrality", "For every effective B-missing tick, the resolved B state is zero and availability reports neutral_applied with no effective command.",
              bool(missing_ticks) and not neutral_failures, {"missing_ticks": len(missing_ticks),
                                                             "failures": neutral_failures[:8]})

        # 9. Recovery uses new source sequence/target and the planned fresh state.
        stable_segments = []
        cursor = analysis_start
        for start, end, _length in effective_gaps:
            if cursor <= start - 1:
                stable_segments.append((cursor, start - 1))
            cursor = end + 1
        if cursor <= analysis_end:
            stable_segments.append((cursor, analysis_end))
        planned_stable = [segment for segment in plan["segments"] if segment["kind"] == "stable"]
        recovery_failures = []
        edge_starts = []
        for index, (start, end) in enumerate(stable_segments):
            length = end - start + 1
            expected = planned_stable[index] if index < len(planned_stable) else None
            minimum_length = (int(plan["minimum_stable_prefix_ticks"]) if index == 0
                              else int(plan["minimum_stable_recovery_ticks"]))
            if length < minimum_length:
                recovery_failures.append({"segment": index, "reason": "too_short", "length": length,
                                          "minimum": minimum_length})
            for tick in range(start, end + 1):
                rows = b_resolve.get(tick, [])
                if len(rows) != 1:
                    recovery_failures.append({"segment": index, "tick": tick, "reason": "resolve_count",
                                              "count": len(rows)})
                    continue
                if expected is None:
                    recovery_failures.append({"segment": index, "tick": tick,
                                              "reason": "unexpected_stable_segment"})
                    continue
                state = rows[0].get("state") or {}
                expected_edge = int(expected["new_edge_at_first_tick"] or 0) if tick == start else 0
                expected_state = {
                    "held_flags": 0, "edge_flags": expected_edge,
                    "move_x": int(expected["b_move_x"]), "move_y": 0, "aim_x": 0, "aim_y": 0,
                }
                if not ordinary_state_equal(state, expected_state):
                    recovery_failures.append({"segment": index, "tick": tick, "reason": "state_not_recovered",
                                              "state": state, "expected": expected_state})
                availability = b_availability.get(tick)
                if availability is None or (int(availability.get("detail", 0)) & 1) == 0:
                    recovery_failures.append({"segment": index, "tick": tick,
                                              "reason": "not_effective_on_recovery"})
            edge_starts.append(start)
        check("b_recovery_from_new_commands", "Every stable prefix/recovery interval is long enough and B's resolved movement plus fresh edge is restored only from its new command.",
              bool(stable_segments) and not recovery_failures,
              {"stable_segments": stable_segments, "planned_stable_segments": len(planned_stable),
               "failures": recovery_failures[:8]})

        # 10. Source sequence/generation stays monotonic and same-generation.
        b_generations = sorted(set(int(event.get("generation", 0)) for event in b_submits))
        sequences = [int(event.get("command", 0)) for event in b_submits]
        sequence_ok = (bool(sequences) and all(right == left + 1 for left, right in zip(sequences, sequences[1:])) and
                       sequences[0] >= 1 and len(b_generations) == 1 and b_generations[0] != 0)
        gap_sequence_ok = all(row["sequence_continuous"] for row in alignment_rows)
        check("b_source_sequence_generation", "All B source commands are accepted, retain one generation, use strictly continuous non-reset sequences, and do not allocate during silence.",
              sequence_ok and gap_sequence_ok and not b_submit_rejected,
              {"generation": b_generations, "first_sequence": sequences[0] if sequences else None,
               "last_sequence": sequences[-1] if sequences else None,
               "accepted_commands": len(sequences), "rejected_b_submits": len(b_submit_rejected)})

        # 11. Edge exactly-once accounting from the retirement progress snapshot.
        accepted_edges = [event for event in b_submits if int((event.get("state") or {}).get("edge_flags", 0)) & 8]
        edge_rows = [event for event in progress_rows
                     if int(event.get("ordinal", -1)) == 1 and int(event.get("query", -1)) == 7 and
                     int(event.get("action", -1)) == 3]
        if edge_rows:
            words = edge_rows[-1].get("before") or []
            processed_edge_count = (int(words[2]) | (int(words[3]) << 32)) if len(words) == 4 else -1
        else:
            processed_edge_count = -1
        expected_edges = sum(1 for segment in planned_stable
                             if int(segment.get("new_edge_at_first_tick") or 0) != 0)
        edge_ok = (len(accepted_edges) == expected_edges == processed_edge_count and
                   len(edge_starts) == expected_edges)
        check("edge_exactly_once", "The five planned drop edges are each accepted and processed once; no gap tick resolves an edge.",
              edge_ok, {"accepted_edge_commands": len(accepted_edges),
                        "processed_edge_count": processed_edge_count,
                        "retirement_edge_progress_rows": len(edge_rows)})

        # 12. The 120-tick silence reaches the diagnostic timeout and recovers without retirement.
        timeout_ticks = []
        silent_values = []
        for tick in missing_ticks:
            event = b_availability[tick]
            if int(event.get("query", 0)) == SOURCE_TIMED_OUT:
                timeout_ticks.append(tick)
            after = event.get("after") or []
            if len(after) >= 3:
                silent_values.append(int(after[2]))
        recovery_active = all(
            (b_availability.get(end + 1) is not None and
             int(b_availability[end + 1].get("detail", 0)) & 1 and
             int(b_availability[end + 1].get("query", 0)) == SOURCE_ACTIVE)
            for _start, end, _length in effective_gaps)
        query9 = [event for event in progress_rows
                  if int(event.get("ordinal", -1)) == 1 and int(event.get("query", -1)) == 9]
        q9 = query9[-1] if query9 else None
        if q9 is not None:
            timeout_entries = int(q9.get("detail", 0))
            timeout_recoveries = int(q9.get("action", 0))
            silent_entries = int(q9.get("target", 0))
            silence_recoveries = int(q9.get("decision", 0))
        else:
            timeout_entries = timeout_recoveries = silent_entries = silence_recoveries = -1
        # A one-tick source gap can be entirely FutureOnly at its only effective
        # tick, so it need not create a Silent entry. The longer gaps do; require
        # at least one actual silence and exactly the 120-tick timeout transition
        # without asserting a particular number of entries beyond that.
        timeout_ok = (bool(timeout_ticks) and max(silent_values or [0]) >= 120 and recovery_active and
                      timeout_entries >= 1 and timeout_recoveries >= 1 and
                      silent_entries >= 1 and silence_recoveries >= 1)
        check("timeout_diagnostic_and_recovery", "The 120-tick source silence reaches TimedOut, later becomes Active from fresh input, and does not retire B or fail the session.",
              timeout_ok, {"timeout_ticks": len(timeout_ticks), "max_silent_ticks": max(silent_values or [0]),
                           "timeout_entries": timeout_entries, "timeout_recoveries": timeout_recoveries,
                           "silent_entries": silent_entries, "silence_recoveries": silence_recoveries,
                           "all_gaps_recover_active": recovery_active})

        # 13. A has real physical input through every B stop and actual movement in the world.
        a_value_ticks = []
        a_gap_evidence = []
        for start, end, length in effective_gaps:
            nonzero = []
            for tick in range(start, end + 1):
                rows = a_resolve.get(tick, [])
                if len(rows) == 1 and state_nonzero(rows[0].get("state") or {}):
                    nonzero.append(tick)
            nearby_nonzero = []
            for tick in range(start - 1, end + 2):
                rows = a_resolve.get(tick, [])
                if len(rows) == 1 and state_nonzero(rows[0].get("state") or {}):
                    nearby_nonzero.append(tick)
            a_value_ticks.extend(nonzero)
            a_gap_evidence.append({"start": start, "end": end, "length": length,
                                   "nonzero_resolve_ticks": len(nonzero),
                                   "nearby_nonzero_resolve_ticks": len(nearby_nonzero),
                                   "resolved_ticks": end - start + 1})
        # A one-tick window can straddle a keyboard release/repress edge. For
        # that window only, adjacent physical ticks are sufficient; every longer
        # stop window must contain an actual non-zero A resolve sample.
        a_physical_ok = all(row["nearby_nonzero_resolve_ticks"] > 0 if row["length"] == 1
                            else row["nonzero_resolve_ticks"] > 0 for row in a_gap_evidence)
        a_directions = set()
        for rows in a_resolve.values():
            if len(rows) != 1:
                continue
            state = rows[0].get("state") or {}
            mx, my = int(state.get("move_x", 0)), int(state.get("move_y", 0))
            if mx or my:
                a_directions.add((1 if mx > 0 else (-1 if mx < 0 else 0),
                                  1 if my > 0 else (-1 if my < 0 else 0)))
        a_physical_ok = a_physical_ok and len(a_directions) >= 2

        position_ticks = sorted(world_positions["A"])
        a_changes = []
        for left, right in zip(position_ticks, position_ticks[1:]):
            a_changes.append({"left": left, "right": right,
                              "distance": distance(world_positions["A"][left], world_positions["A"][right])})
        significant_changes = [row for row in a_changes if row["distance"] > 0.25]
        world_gap_evidence = []
        for start, end, length in effective_gaps:
            before_positions = [tick for tick in position_ticks if tick < start]
            during_positions = [tick for tick in position_ticks if start <= tick <= end]
            after_positions = [tick for tick in position_ticks if tick > end]
            baseline = world_positions["A"][before_positions[-1]] if before_positions else None
            candidates = [world_positions["A"][tick] for tick in during_positions]
            if not candidates and after_positions:
                candidates = [world_positions["A"][after_positions[0]]]
            movement = None
            if baseline is not None and candidates:
                movement = max(distance(baseline, candidate) for candidate in candidates)
            world_gap_evidence.append({"start": start, "end": end, "length": length,
                                       "complete_world_captures_in_window": len(during_positions),
                                       "a_position_movement_from_pre_gap": movement})
        movement_required = [row for row in world_gap_evidence if row["length"] >= 5 and
                             row["complete_world_captures_in_window"] > 0]
        movement_observed_gaps = sum(1 for row in world_gap_evidence
                                     if row["a_position_movement_from_pre_gap"] is not None and
                                     row["a_position_movement_from_pre_gap"] > 0.25)
        actual_motion_ok = (bool(position_ticks) and len(position_ticks) >= 3 and
                            len(significant_changes) >= 3 and movement_observed_gaps >= 3 and
                            all(row["a_position_movement_from_pre_gap"] is not None and
                                row["a_position_movement_from_pre_gap"] > 0.25 for row in movement_required))
        check("a_physical_action_during_b_silence", "A has non-zero resolved physical input during every B source stop and actual Player position change in the explicitly partial-or-complete world record.",
              a_physical_ok and actual_motion_ok,
              {"a_gap_evidence": a_gap_evidence, "world_gap_evidence": world_gap_evidence,
               "complete_world_captures": complete_world_capture_rows,
               "partial_world_captures": partial_world_capture_rows,
               "a_position_ticks": len(position_ticks),
               "significant_world_moves": len(significant_changes),
               "world_motion_observed_gaps": movement_observed_gaps,
               "a_direction_states": sorted(a_directions),
               "world_player_position_not_ready": dict(world_player_ready)})

        # 14. B neutrality is not confused with physical immobility.
        b_position_changes = 0
        b_ticks = sorted(world_positions["B"])
        for left, right in zip(b_ticks, b_ticks[1:]):
            if distance(world_positions["B"][left], world_positions["B"][right]) > 0.25:
                b_position_changes += 1
        check("neutrality_semantics_preserved", "The world record contains B Player position samples, recorded separately from the input-neutrality decision.",
              bool(b_ticks),
              {"b_world_position_ticks": len(b_ticks), "b_observed_position_changes": b_position_changes,
               "definition": "neutral means zero effective canonical input, not frozen position"})

        # 15. B remains bound until the cooperative stop; retirement only follows it.
        valid_retirements = [event for event in retired_events if int(event.get("boolean", 0)) == 1]
        retired_before_end = [event for event in valid_retirements if int(event.get("tick", 0)) <= analysis_end]
        late_retirement = (len(valid_retirements) == 1 and
                           int(valid_retirements[0].get("tick", 0)) > analysis_end)
        b_binding_values = [int(event.get("boolean", 0)) for tick, event in b_availability.items()
                            if analysis_start <= tick <= analysis_end]
        binding_ok = bool(b_binding_values) and all(value == 1 for value in b_binding_values)
        check("b_binding_preserved", "B stays bound through all stop windows; one valid cooperative retirement occurs only after the final scripted target.",
              binding_ok and not retired_before_end and late_retirement and
              len(retired_events) == len(valid_retirements),
              {"all_availability_binding_true": binding_ok,
               "retired_events": len(retired_events),
               "valid_retirements": len(valid_retirements),
               "retired_before_analysis_end": len(retired_before_end),
               "late_retirement": late_retirement})

        # 16. Script completion marker and native source stop, never network shutdown.
        expected_script_steps = len(script.get("steps", []))
        script_complete_ok = (len(script_complete_events) == 1 and
                              int(script_complete_events[0].get("detail", -1)) == expected_script_steps)
        check("script_completion_marker", "The B script source reached its once-only completion marker after consuming the full preloaded step set.",
              script_complete_ok,
              {"script_complete_events": len(script_complete_events),
               "accepted_script_steps": expected_script_steps,
               "limit": 256,
               "network_claim": "NOT_MADE: this is a scripted source stop, not a real disconnect"})

        # 17. Process health after cooperative restoration.
        game_pid = int(observation.get("game_pid", 0))
        process_health = None
        process_alive = False
        if game_pid > 0:
            try:
                completed = subprocess.run(
                    ["tasklist", "/FI", f"PID eq {game_pid}", "/FO", "CSV", "/NH"],
                    capture_output=True, text=True, timeout=15, check=False)
                rows = list(csv.reader(completed.stdout.splitlines()))
                process_health = {
                    "returncode": completed.returncode,
                    "rows": rows,
                    "stderr": completed.stderr.strip(),
                }
                process_alive = any(len(row) >= 2 and row[1].strip() == str(game_pid) for row in rows)
            except (OSError, subprocess.SubprocessError) as exc:
                process_health = {"error": str(exc)}
        check("game_process_health", "The game process from the recorded invocation is still alive after the controller's cooperative restoration.",
              process_alive, {"game_pid": game_pid, "tasklist": process_health})

        # Preserve structured evidence for the gaps and A/world observations.
        write_json(report_dir / "gaps.json", {
            "source_gaps": source_gaps,
            "target_gaps": target_gaps,
            "effective_gaps": effective_gaps,
            "expected_lengths": expected_gap_lengths,
            "normal_target_minus_source_tick": normal_delta,
            "alignment": alignment_rows,
            "missing_reason_shapes": missing_reason_shapes,
        })
        write_json(report_dir / "a-world-observation.json", {
            "a_gap_evidence": a_gap_evidence,
            "world_gap_evidence": world_gap_evidence,
            "a_direction_states": sorted(a_directions),
            "a_position_sample_count": len(position_ticks),
            "b_position_sample_count": len(b_ticks),
            "world_capture_records": world_capture_records,
            "complete_world_captures": complete_world_capture_rows,
            "partial_world_captures": partial_world_capture_rows,
            "world_capture_reasons": dict(world_capture_reasons),
            "world_rows_seen": world_capture_rows,
            "malformed_world_rows": malformed_world,
        })
        write_json(report_dir / "runtime-observations.json", {
            "analysis_window": [analysis_start, analysis_end],
            "in_window_ticks": len(in_window),
            "boundary_kinds": dict(boundary_kinds),
            "post_state_counts": dict(post_counts),
            "b_submits": len(b_submits),
            "b_submit_rejected": len(b_submit_rejected),
            "b_generations": b_generations,
            "b_sequence_first_last": [sequences[0], sequences[-1]] if sequences else [],
            "expected_lead_ticks": expected_lead,
            "measured_normal_target_minus_source_tick": normal_delta,
            "source_gaps": source_gaps,
            "effective_gaps": effective_gaps,
            "timeout_ticks": len(timeout_ticks),
            "retired_events": len(retired_events),
            "script_complete_events": len(script_complete_events),
            "duplicate_b_receipts": duplicate_receipts,
            "duplicate_availability_rows": duplicate_availability,
            "config_event": config_detail,
            "world_complete_captures": complete_world_capture_rows,
            "world_partial_captures": partial_world_capture_rows,
            "world_capture_reasons": dict(world_capture_reasons),
        })

        malformed_ok = not malformed_events and not malformed_world
        check("event_stream_integrity", "All controller event and world JSONL records parsed as complete records.",
              malformed_ok, {"malformed_events": malformed_events[:8],
                             "malformed_world": malformed_world[:8]})

        failed = [row for row in checks if row["status"] == "FAIL"]
        status = "PASS" if not failed else "FAIL"
        result = {
            "tx": "tx-01-98",
            "operation": 9,
            "status": status,
            "checks": checks,
            "passed": sum(1 for row in checks if row["status"] == "PASS"),
            "failed": len(failed),
            "required_checks": sum(1 for row in checks if row["required"]),
            "scope": "One real game process/run; scripted B production-source silence, not real network loss.",
            "automatic_retry": False,
            "hashes": "NOT_USED",
            "artifacts": {
                "events": "capture/events.jsonl",
                "world": "capture/world/",
                "restoration": "capture/restoration.json",
                "gaps": "report/gaps.json",
                "a_world": "report/a-world-observation.json",
                "runtime": "report/runtime-observations.json",
            },
        }
        write_json(report_dir / "result.json", result)
        with (report_dir / "checks.jsonl").open("w", encoding="utf-8", newline="\n") as stream:
            for row in checks:
                stream.write(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n")
        print(json.dumps({"status": status, "passed": result["passed"], "failed": len(failed),
                          "checks": len(checks)}, sort_keys=True))
        return 0 if status == "PASS" else 1
    except Exception as exc:  # Preserve a machine-readable failed acceptance, never retry.
        result = {
            "tx": "tx-01-98",
            "operation": 9,
            "status": "FAIL",
            "error": str(exc),
            "checks": checks,
            "passed": sum(1 for row in checks if row["status"] == "PASS"),
            "failed": sum(1 for row in checks if row["status"] == "FAIL") + 1,
            "automatic_retry": False,
            "hashes": "NOT_USED",
        }
        write_json(report_dir / "result.json", result)
        with (report_dir / "checks.jsonl").open("w", encoding="utf-8", newline="\n") as stream:
            for row in checks:
                stream.write(json.dumps(row, sort_keys=True, separators=(",", ":")) + "\n")
            stream.write(json.dumps({"id": "analysis_exception", "status": "FAIL", "detail": str(exc)},
                                    sort_keys=True, separators=(",", ":")) + "\n")
        print(json.dumps({"status": "FAIL", "error": str(exc)}, sort_keys=True))
        return 1


if __name__ == "__main__":
    sys.exit(main())
