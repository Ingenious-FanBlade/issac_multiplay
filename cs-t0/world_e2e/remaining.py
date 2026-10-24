"""Explicit continuation of interrupted E2E evidence; never retry prior commands."""
from __future__ import annotations

import collections
import shutil
from pathlib import Path

from .artifacts import (CLASSES, MAX_RECORD, append, decode, full, load, mapped_key,
                        restoration, rows, save, scan_world, select_target, utc)


def import_previous(suite, source):
    source = Path(source).resolve()
    if source == suite.root or source.is_relative_to(suite.root) or suite.root.is_relative_to(source):
        raise ValueError("Continuation output must be separate from the immutable prior evidence")
    result = load(source / "result.json")
    if result["status"] != "INTERRUPTED":
        raise ValueError("Continuation accepts interrupted evidence only; failed executions are not automatically retried")
    load(source / "finalization.json")
    prior = suite.root / "prior-evidence"
    prior.mkdir()
    for name in ("result.json", "manifest.json", "finalization.json", "binary-provenance.json", "primary-coverage.json"):
        shutil.copyfile(source / name, prior / name)
    # Existing binary copies are the actual target, not a new source build.
    expected = {"controller": "IsaacCsT0.exe", "native_dll": "IsaacCsT0.Native.dll", "helper": "IsaacCsT0.Helper.exe"}
    for argument, name in expected.items():
        if Path(getattr(suite.args, argument)).resolve() != source / "binaries" / name:
            raise ValueError("Continuation must use the prior invocation's frozen binaries")
    carried = []
    for group in result["cases"]:
        for item in group["checks"]:
            if item["status"] == "PASS" and item["id"] in suite.ledger.items:
                carried.append(item["id"])
                suite.ledger.items[item["id"]]["method"] = "CARRIED_FORWARD"
                suite.ledger.record(item["id"], "PASS", "Prior invocation only: " + item["reason"],
                                    [prior / "result.json", *item["evidence"]])
    for role in ("primary", "secondary"):
        directory = source / "captures" / role
        before = load(directory / "process-before.json")
        after = load(directory / "process-after.json")
        ended = load(directory / "controller-exit.json")
        if ended["controller_loss_requested"] or ended["exit_code"] != 0 or suite.process_key(before) != suite.process_key(after):
            raise ValueError("Prior capture did not complete in the same process")
        suite.captures[role] = {"directory": directory / "capture", "process": before,
                                "process_after": after, "same_process_after": True,
                                "exit_code": ended["exit_code"], "controller_lost": False}
        suite.processes.add(suite.process_key(before))
    suite.summaries["primary"] = load(prior / "primary-coverage.json")
    # Select before running any case. Preserve the full profile and object set.
    # Prefer a full ordinary sample with a second uniquely mapped reference owner.
    world = suite.captures["primary"]["directory"] / "world"
    selected = None
    with (world / "captures.jsonl").open("rb") as stream:
        for _, _, entry in rows(source / "primary-record-index.jsonl"):
            if not entry["full_profile"]:
                continue
            stream.seek(entry["offset"])
            raw = stream.readline(MAX_RECORD + 1)
            if len(raw) > MAX_RECORD:
                raise ValueError("Frozen sample exceeds record bound")
            record = decode(raw)
            target = select_target(record)
            if not full(record) or target is None or record["attempt_seq"] != entry["attempt_seq"]:
                raise ValueError("Prior index does not describe the selected record")
            keys = collections.Counter((obj["class"], mapped_key(obj)) for obj in record["objects"])
            referrer = any(index != target[0] and mapped_key(obj) is not None
                           and keys[(obj["class"], mapped_key(obj))] == 1
                           and any(field["encoding"] == "REFERENCE" for field in obj["fields"])
                           for index, obj in enumerate(record["objects"]))
            if selected is None:
                selected = entry, record
            if referrer:
                selected = entry, record
                break
    if selected is None:
        raise ValueError("No full ordinary frozen capture available; no case was executed")
    entry, suite.primary_sample = selected
    save(suite.root / "frozen-sample.json", suite.primary_sample)
    save(suite.root / "continuation.json", {
        "transaction": "tx-01-76", "target_transaction": "tx-01-75", "source": str(source),
        "at_utc": utc(), "prior_status": result["status"], "carried_forward_pass": carried,
        "full_replay": "USER_EXCLUDED", "single_record_replay": "USER_ALLOWED",
        "sample_source": str(world / "captures.jsonl"), "sample_index": entry,
        "selection_rule": "First full ordinary sample with a unique finite position target and another unique mapped REFERENCE owner; otherwise first full ordinary sample",
        "original_evidence_modified": False, "automatic_retry": False,
    })
    suite.event("continuation_imported", source=str(source), carried_forward=carried, sample_index=entry)
    print(f"Carried {len(carried)} prior PASS; full replay excluded. Frozen sample tick={suite.primary_sample['authority_tick']}", flush=True)


def supplemental(suite):
    stage = {"role": "supplemental", "script": str(suite.root / "inputs" / "primary.json"), "instructions": [
        "Fully restart into a fresh isolated local run yourself; A exists, no Online/Isaac Sync session.",
        "Prepare a reachable Familiar and enemy projectile scene using your own permitted game operations.",
        "Bracket actual scenes with mark, leave enough normal updates, then stop cooperatively.",
        "No new 10,000-sample run, special lifecycle or controller-loss action is required in this stage.",
    ]}
    save(suite.root / "supplemental-plan.json", stage)
    suite.capture(stage)
    captured = suite.captures.get("supplemental")
    if not captured:
        return
    world = captured["directory"] / "world"
    summary, _ = scan_world(world, suite.root / "supplemental-record-index.jsonl")
    save(suite.root / "supplemental-coverage.json", summary)
    readable = set(suite.summaries["primary"]["readable_classes"]) | set(summary["readable_classes"])
    evidence = [suite.root / "prior-evidence" / "primary-coverage.json", suite.root / "supplemental-coverage.json", world]
    clean = (captured["exit_code"] == 0 and captured.get("same_process_after") is True
             and restoration(captured["directory"], 70) and not summary["profile_failures"]
             and not summary["invariant_failures"] and not summary["event_gaps"]
             and summary["producer_result"]["native_error"] == "NONE")
    save(suite.root / "supplemental-outcome.json", {"clean_capture_and_restore": clean,
        "combined_readable_classes": sorted(readable), "prior_and_current_are_separate_process_captures": True})
    if not clean:
        suite.ledger.record("W03.eight_classes", "FAIL", "Supplemental capture/profile/restore evidence failed; no retry", evidence)
    elif set(CLASSES[:8]).issubset(readable):
        suite.ledger.record("W03.eight_classes", "PASS", "Eight readable classes covered across identified prior and supplemental captures using the same frozen binaries", evidence)
    else:
        suite.ledger.record("W03.eight_classes", "NOT_COVERED", "Still missing: " + ", ".join(sorted(set(CLASSES[:8]) - readable)), evidence)
