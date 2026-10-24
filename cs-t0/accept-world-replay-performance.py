"""tx-01-88 single offline E2E: full production replay, performance and current ledger."""
from __future__ import annotations

import argparse
import copy
import itertools
import json
import math
import shutil
import subprocess
import time
from pathlib import Path

from world_e2e.artifacts import (Ledger, MANUAL, canonical, first_value_difference,
                                  load, rows, same_file_bytes, save, utc)

ROOT = Path(__file__).resolve().parent.parent
MIB = 1024 * 1024


class Acceptance:
    def __init__(self, args):
        self.args = args
        self.root = Path(args.artifact_dir).resolve()
        self.root.mkdir(parents=True, exist_ok=False)
        self.world = Path(args.world_dir).resolve()
        self.ledger = Ledger(self.root)
        self.ledger.transaction = "tx-01-88"
        self.ledger.scope = "W08 full replay performance and exact data preservation; W01.isolation permanently removed; other checks carried from tx-01-87"
        self.commands = []
        self.proof = {"automatic_retry": False, "game_operations": False, "hashes": "NOT_USED"}
        self.frozen = self.root / "frozen-source"

    def prepare(self):
        prior = load(Path(self.args.prior) / "result.json")
        historical = self.root / "historical-results"
        historical.mkdir()
        for name in ("result.json", "finalization.json", "authorized-scope-result.json"):
            shutil.copyfile(Path(self.args.prior) / name, historical / ("tx-01-87-" + name))
        for group in prior["cases"]:
            for item in group["checks"]:
                if item["id"] in self.ledger.items and item["status"] == "PASS":
                    self.ledger.items[item["id"]]["method"] = "CARRIED_FORWARD"
                    self.ledger.record(item["id"], "PASS", "Historical tx-01-87/cited predecessors, not rerun on new controller: " + item["reason"],
                                       [historical / "tx-01-87-result.json", *item["evidence"]])
        save(self.root / "scope-change.json", {"removed": "W01.isolation", "reason": "User permanently removed real Online/Isaac Sync testing: design never enters those modes",
             "current_checks": len(self.ledger.items), "replay_resumed_by_user": True, "old_reports_unchanged": True})
        if len(self.ledger.items) != 32 or "W01.isolation" in self.ledger.items or "W01.isolation" in MANUAL:
            raise ValueError("Current checklist still contains removed online obligation")
        binaries = self.root / "binaries"
        binaries.mkdir()
        origins = []
        for role, source in (("new", self.args.controller), ("baseline", self.args.baseline)):
            origin = Path(source).resolve()
            destination = binaries / (role + "-IsaacCsT0.exe")
            shutil.copyfile(origin, destination)
            setattr(self, role, destination)
            origins.append({"role": role, "source": str(origin), "copy": str(destination), "bytes": origin.stat().st_size})
        save(self.root / "binary-provenance.json", origins)
        for name in ("cs-t0/src/world_record.rs", "cs-t0/Cargo.toml", "cs-t0/Cargo.lock",
                     "cs-t0/accept-world-replay-performance.py", "cs-t0/world_e2e/artifacts.py",
                     "cs-t0/world_e2e/instrumented.py", "cs-t0/world_e2e/completion.py",
                     "cs-t0/world_e2e/remaining.py", "cs-t0/world_e2e/scenarios.py",
                     "cs-t0/accept-world-final-windows.py", "cs-t0/accept-world-remaining-completion.py",
                     "out/cs-t0/tx-01-88-build/rust-build.log", "docs/refactor/project-conventions.md",
                     "docs/refactor/cs-t0/t0-06/tx-01-73-e2e-checklist.md",
                     "docs/refactor/cs-t0/t0-06/tx-01-88-replay-performance.md"):
            destination = self.root / "sources" / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, destination)
        self.frozen.mkdir()
        for name in ("manifest.json", "captures.jsonl"):
            shutil.copyfile(self.world / name, self.frozen / name)
        self.manifest = load(self.frozen / "manifest.json")
        save(self.root / "invocation.json", {"arguments": vars(self.args), "started_at_utc": utc(),
             "source_bytes": (self.frozen / "captures.jsonl").stat().st_size,
             "minimum_mib_per_second": 5, "minimum_speedup": 5,
             "process_timeout_seconds": 600, "automatic_retry": False, "game_operations": False, "hashes": "NOT_USED"})

    def cli(self, label, binary, source):
        destination = self.root / label
        argv = [str(binary), "world-replay", "--capture", str(source), "--artifact-dir", str(destination)]
        command = {"name": label, "argv": argv, "started_at_utc": utc()}
        self.commands.append(command)
        print("Starting " + label, flush=True)
        started = time.perf_counter()
        with (self.root / (label + ".log")).open("xb") as log:
            child = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
                                     creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
            command["pid"] = child.pid
            try:
                code = child.wait(timeout=600)
            except (KeyboardInterrupt, subprocess.TimeoutExpired):
                # Only this owned offline CLI, never a game/controller attachment.
                child.terminate()
                code = child.wait()
                command.update(exit_code=code, interrupted_or_timed_out=True, seconds=time.perf_counter()-started)
                save(self.root / (label + "-command.json"), command)
                raise
        command.update(exit_code=code, seconds=time.perf_counter()-started, ended_at_utc=utc())
        save(self.root / (label + "-command.json"), command)
        print(f"Finished {label}: exit={code}, {command['seconds']:.3f}s", flush=True)
        return destination, command

    def owned_input(self, label, raw):
        directory = self.root / "inputs" / label
        directory.mkdir(parents=True)
        save(directory / "manifest.json", self.manifest)
        (directory / "captures.jsonl").write_bytes(raw)
        return directory

    def run(self):
        self.prepare()
        prefix = self.root / "baseline-prefix"
        prefix.mkdir()
        shutil.copyfile(self.frozen / "manifest.json", prefix / "manifest.json")
        prefix_records, prefix_bytes, sample = 0, 0, None
        with (prefix / "captures.jsonl").open("xb") as output:
            for _, raw, record in rows(self.frozen / "captures.jsonl"):
                output.write(raw)
                prefix_records += 1
                prefix_bytes += len(raw)
                if sample is None and record["objects"]:
                    sample = record
                if prefix_bytes >= 4*MIB and sample is not None:
                    break
        if sample is None:
            raise ValueError("Actual input has no object sample")
        baseline, baseline_command = self.cli("baseline-prefix-output", self.baseline, prefix)
        if baseline_command["exit_code"] != 0 or load(baseline / "result.json")["records"] != str(prefix_records):
            raise ValueError("Original production baseline failed")
        outputs, timings = [], []
        for label in ("replay-a", "replay-b"):
            output, command = self.cli(label, self.new, self.frozen)
            result = load(output / "result.json") if command["exit_code"] == 0 else None
            if not result or result["status"] != "REENCODED":
                raise ValueError("Full production replay failed: " + label)
            io = result["io"]
            size = (output / "captures.jsonl").stat().st_size
            if not io["flushed"] or int(io["output_bytes"]) != size or int(io["file_write_calls"]) > 2*math.ceil(size/MIB)+1:
                raise ValueError("Output not fully flushed or file writes remain unbuffered")
            outputs.append(output)
            timings.append({**command, "result": result})
        input_bytes = (self.frozen / "captures.jsonl").stat().st_size
        baseline_rate = prefix_bytes / baseline_command["seconds"] / MIB
        rates = [input_bytes / command["seconds"] / MIB for command in timings]
        self.proof["performance"] = {"input_bytes": input_bytes, "baseline_prefix_bytes": prefix_bytes,
             "baseline_prefix_records": prefix_records, "baseline_mib_per_second": baseline_rate,
             "full_mib_per_second": rates, "speedup_against_prefix": [rate/baseline_rate for rate in rates],
             "full_commands": timings, "baseline_scope": "same actual input prefix, not a full old replay",
             "passed": all(rate >= 5 and rate >= 5*baseline_rate for rate in rates)}
        save(self.root / "performance.json", self.proof["performance"])
        print("Checking every canonical record and exact repeated output bytes", flush=True)
        equal, baseline_equal, count, compared_fields = True, True, 0, 0
        report = self.root / "record-comparison.jsonl"
        with report.open("x", encoding="utf-8", newline="\n") as log, (baseline / "captures.jsonl").open("rb") as legacy:
            for source, left, right in itertools.zip_longest(rows(self.frozen / "captures.jsonl"),
                    rows(outputs[0] / "captures.jsonl"), rows(outputs[1] / "captures.jsonl")):
                count += 1
                present = all(value is not None for value in (source, left, right))
                values_ok = present and canonical(source[2]) == left[2] == right[2]
                bytes_ok = left is not None and right is not None and left[1] == right[1]
                legacy_ok = count > prefix_records or (left is not None and legacy.readline() == left[1])
                fields = sum(len(obj["fields"]) for obj in source[2]["objects"]) if source else 0
                compared_fields += fields
                difference = None if values_ok or not present else first_value_difference(canonical(source[2]), left[2])
                log.write(json.dumps({"record_index": count, "source_offset": source[0] if source else None,
                     "same_canonical_values": values_ok, "same_reencoded_bytes": bytes_ok,
                     "legacy_prefix_bytes_equal": legacy_ok, "compared_fields": fields,
                     "first_value_difference": difference}, separators=(",", ":")) + "\n")
                equal &= bool(values_ok and bytes_ok)
                baseline_equal &= legacy_ok
                if count % 5000 == 0:
                    print(f"Compared {count} records", flush=True)
            baseline_equal &= not legacy.read(1)
        counts_ok = count > 0 and all(str(count) == load(output / "result.json")["records"] for output in outputs)
        unchanged = all(same_file_bytes(self.world / name, self.frozen / name) for name in ("manifest.json", "captures.jsonl"))
        manifests_ok = all(load(output / "manifest.json") == self.manifest for output in outputs)
        self.proof["records"] = {"count": count, "compared_fields": compared_fields, "equal": equal,
             "counts_match": counts_ok, "legacy_prefix_equal": baseline_equal, "source_unchanged_exact_bytes": unchanged,
             "manifest_equal": manifests_ok, "method": "STREAMED_VALUES_AND_EXACT_BYTES_NO_DIGEST"}

        # Reorder a real owned sample and retain the maximum u64 decimal string.
        variant = copy.deepcopy(sample)
        variant["outer_token"] = "18446744073709551615"
        variant["sources"].reverse()
        variant["objects"].reverse()
        for obj in variant["objects"]:
            obj["fields"].reverse()
            obj["memberships"].reverse()
        raw = json.dumps(variant, separators=(",", ":")).encode() + b"\n"
        wide_source = self.owned_input("wide-reordered", raw)
        wide_output, wide_command = self.cli("wide-reordered-output", self.new, wide_source)
        wide_rows = list(rows(wide_output / "captures.jsonl")) if wide_command["exit_code"] == 0 else []
        wide_ok = (canonical(variant) != variant and len(wide_rows) == 1 and wide_rows[0][2] == canonical(variant))
        self.proof["wide_reordered"] = {"passed": wide_ok, "outer_token": variant["outer_token"], "source": str(wide_source)}
        failures = []
        invalid_width = copy.deepcopy(sample)
        invalid_width["objects"][0]["fields"][0]["width"] += 1
        invalid_decimal = copy.deepcopy(sample)
        invalid_decimal["outer_token"] = "18446744073709551616"
        unknown_key = copy.deepcopy(sample)
        unknown_key["unexpected"] = True
        bad_inputs = {"width": json.dumps(invalid_width).encode(), "u64-overflow": json.dumps(invalid_decimal).encode(),
             "extra-key": json.dumps(unknown_key).encode(), "truncated-after-valid": raw + b'{"record":'}
        for name, invalid in bad_inputs.items():
            source = self.owned_input(name, invalid)
            output, command = self.cli("reject-" + name, self.new, source)
            passed = command["exit_code"] != 0 and (output / "failure.json").is_file() and not (output / "result.json").exists()
            failures.append({"name": name, "passed": passed, "exit_code": command["exit_code"], "output": str(output)})
        self.proof["rejections"] = failures
        replay_ok = all((equal, counts_ok, baseline_equal, unchanged, manifests_ok, wide_ok,
                         self.proof["performance"]["passed"], all(case["passed"] for case in failures)))
        self.ledger.items["W08.replay"]["method"] = "FULL_PRODUCTION_REPLAY_E2E"
        self.ledger.require("W08.replay", replay_ok, f"{count} full records/{compared_fields} fields; two full production replays, legacy prefix byte oracle, bounded buffered writes and measured speed",
                            [report, self.root / "performance.json", *outputs])
        self.ledger.items["W08.wide_integer"]["method"] = "PRODUCTION_CLI_OWNED_ACTUAL_SAMPLE"
        self.ledger.require("W08.wide_integer", wide_ok, "Maximum u64 survives production parsing/reencoding and reordered full fields canonicalize", [wide_source, wide_output])
        save(self.root / "replay-proof.json", self.proof)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("controller", "baseline", "world-dir", "prior", "artifact-dir"):
        parser.add_argument("--"+name, required=True)
    suite = Acceptance(parser.parse_args())
    failure, interrupted = None, False
    try:
        suite.run()
    except KeyboardInterrupt:
        interrupted = True
        failure = "User interrupted owned offline acceptance"
    except Exception as exc:
        failure = f"{type(exc).__name__}: {exc}"
    finally:
        if failure and suite.ledger.items["W08.replay"]["status"] != "FAIL":
            suite.ledger.record("W08.replay", "FAIL", failure, [suite.root])
        save(suite.root / "commands.json", suite.commands)
        save(suite.root / "partial-proof.json", suite.proof)
        result = suite.ledger.export(failure, interrupted)
        save(suite.root / "finalization.json", {"status": "COMPLETED", "result_status": result["status"],
             "at_utc": utc(), "error": failure, "automatic_retry": False, "prior_reports_modified": False})
    print(f"{result['status']}: {suite.root / 'result.json'}", flush=True)
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
