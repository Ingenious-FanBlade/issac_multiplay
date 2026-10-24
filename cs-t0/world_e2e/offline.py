"""Production-CLI E2E using actual captures and explicitly labelled owned copies."""
from __future__ import annotations

import copy
import itertools
import collections
import json
import shutil
from pathlib import Path

from .artifacts import (append, canonical, first_value_difference, load, mapped_key,
                        number, rows, same_file_bytes, save, select_target)


class Offline:
    def __init__(self, suite, world):
        self.suite = suite
        self.world = Path(world)
        self.manifest = load(self.world / "manifest.json")
        self.root = suite.root / "offline"
        self.root.mkdir()
        self.case_index = 0

    def cli(self, name, command, arguments):
        destination = self.root / name
        exit_code = self.suite.command(name, [str(self.suite.controller), command, *arguments,
                                               "--artifact-dir", str(destination)])
        result = load(destination / "result.json") if (destination / "result.json").is_file() else None
        return exit_code, destination, result

    def replay(self):
        key = "W08.replay"
        try:
            frozen = self.root / "frozen-source"
            frozen.mkdir()
            for name in ("manifest.json", "captures.jsonl"):
                shutil.copyfile(self.world / name, frozen / name)
            save(frozen / "provenance.json", {"source_world": str(self.world), "scope": "EXACT_FROZEN_FILE_COPY"})
            a_exit, a, a_result = self.cli("replay-a", "world-replay", ["--capture", str(frozen)])
            unchanged_a = all(same_file_bytes(self.world / name, frozen / name) for name in ("manifest.json", "captures.jsonl"))
            b_exit, b, b_result = self.cli("replay-b", "world-replay", ["--capture", str(frozen)])
            unchanged_b = all(same_file_bytes(self.world / name, frozen / name) for name in ("manifest.json", "captures.jsonl"))
            if a_exit != 0 or b_exit != 0 or not a_result or not b_result:
                raise ValueError("production replay failed; inspect command logs and failure.json")
            count = 0
            equal = a_result["status"] == b_result["status"] == "REENCODED" and unchanged_a and unchanged_b
            save(self.root / "replay-source-comparison.json", {"unchanged_after_a": unchanged_a, "unchanged_after_b": unchanged_b,
                                                              "method": "STREAMED_EXACT_BYTES_NO_DIGEST"})
            comparisons = self.root / "replay-record-comparison.jsonl"
            for source, left, right in itertools.zip_longest(
                    rows(self.world / "captures.jsonl"), rows(a / "captures.jsonl"), rows(b / "captures.jsonl")):
                count += 1
                same_values = source is not None and left is not None and right is not None
                same_bytes = left is not None and right is not None and left[1] == right[1]
                field_count, difference = 0, None
                if same_values:
                    original = canonical(source[2])
                    field_count = sum(len(obj["fields"]) for obj in original["objects"])
                    same_values = original == left[2] == right[2]
                    if not same_values:
                        difference = (first_value_difference(original, left[2])
                                      or first_value_difference(left[2], right[2]))
                equal &= same_values and same_bytes
                append(comparisons, {"record_index": count, "same_canonical_values": same_values,
                                     "same_reencoded_bytes": same_bytes,
                                     "compared_fields": field_count, "first_value_difference": difference,
                                     "source_offset": source[0] if source else None})
            equal &= count > 0 and str(count) == a_result["records"] == b_result["records"]
            self.suite.ledger.require(key, equal, f"{count} records compared as values and exact bytes",
                                      [a, b, comparisons])
        except (OSError, ValueError, KeyError, TypeError) as exc:
            self.suite.ledger.record(key, "FAIL", str(exc), [self.root])

    def input(self, name, records, manifest=None):
        directory = self.root / "inputs" / name
        directory.mkdir(parents=True)
        save(directory / "manifest.json", self.manifest if manifest is None else manifest)
        with (directory / "captures.jsonl").open("x", encoding="utf-8", newline="\n") as stream:
            for record in records:
                stream.write(json.dumps(record, ensure_ascii=False, separators=(",", ":"), allow_nan=False) + "\n")
        return directory

    @staticmethod
    def alignment(record):
        births = sorted({obj["birth_event"] for obj in record["objects"] if obj["birth_event"] is not None}, key=number)
        players = sorted({obj["ordinal"] for obj in record["objects"] if obj["class"] == "Player" and obj["ordinal"] in (0, 1)})
        scope = {"run_epoch": record["run_epoch"], "room_epoch": record["room_epoch"]}
        return {"version": 1, "left": scope, "right": dict(scope),
                "ticks": [{"left": record["authority_tick"], "right": record["authority_tick"]}],
                "births": [{"left": birth, "right": birth} for birth in births],
                "players": [{"left": ordinal, "right": ordinal} for ordinal in players]}

    @staticmethod
    def target_path(record, index, alignment, field_id):
        obj = record["objects"][index]
        if obj["class"] == "Player" and obj["ordinal"] in (0, 1):
            ordinal = next(i for i, pair in enumerate(alignment["players"]) if pair["left"] == obj["ordinal"])
            key = f"player:{ordinal:02}"
        else:
            mapped = next(i for i, pair in enumerate(alignment["births"]) if pair["left"] == obj["birth_event"])
            birth = obj["birth"]
            key = f"birth:{mapped:020}:{obj['object_domain']}:{birth['kind']}:{birth['entity_type']}:{birth['variant']}:{birth['subtype']}:{birth['seed']}"
        name = "position.x" if field_id == "000b" else None
        return ["run", "room", obj["class"], key, name]

    @staticmethod
    def field(record, index, field_id):
        return next(field for field in record["objects"][index]["fields"] if field["field_id"] == field_id)

    def comparison(self, name, left, right, alignment, change):
        self.case_index += 1
        prefix = f"{self.case_index:02}-{name}"
        ld = self.input(prefix + "-left", left)
        rd = self.input(prefix + "-right", right)
        alignment_path = self.root / "inputs" / (prefix + "-alignment.json")
        save(alignment_path, alignment)
        save(self.root / "inputs" / (prefix + "-changes.json"),
             {"scope": "OWNED_VALUE_COPY_ONLY_NOT_NATIVE_BRANCH_COVERAGE",
              "source_world": str(self.world), "identity_basis": "Both sides are copies of the identical frozen capture; no cross-process inference",
              "change": change})
        code, output, result = self.cli(prefix, "world-compare",
                                        ["--left", str(ld), "--right", str(rd), "--alignment", str(alignment_path)])
        if code != 0 or result is None:
            raise ValueError(f"production comparison failed: {prefix}")
        for directory, expected in ((ld, left), (rd, right)):
            preserved = load(directory / "manifest.json") == self.manifest
            for actual, original in itertools.zip_longest(rows(directory / "captures.jsonl"), expected):
                preserved &= actual is not None and original is not None and actual[2] == original
            if not preserved:
                raise ValueError(f"production comparison changed its input: {prefix}")
        compared = [record for _, _, record in rows(output / "comparisons.jsonl")]
        if len(compared) != len(alignment["ticks"]):
            raise ValueError(f"unexpected comparison row count: {prefix}")
        return output, result, compared

    @staticmethod
    def unavailable(row, path, field_id):
        return any(unknown.get("reason") == "FIELD_UNAVAILABLE" and unknown.get("field_id") == field_id
                   and unknown.get("path") == path[:4] for unknown in row.get("unknown", []))

    def mutations(self, sample):
        """No reduced profile; every variant keeps the complete actual object set."""
        index, field_id = select_target(sample)
        alignment = self.alignment(sample)
        path = self.target_path(sample, index, alignment, field_id)
        before = self.field(sample, index, field_id)["raw32"]
        after = f"{int(before, 16) ^ 1:08x}"
        right = copy.deepcopy(sample)
        self.field(right, index, field_id)["raw32"] = after
        changed = {"local_id": sample["objects"][index]["local_id"], "use_generation": sample["objects"][index]["use_generation"],
                   "field_id": field_id, "before": before, "after": after, "expected_path": path}
        try:
            output, result, compared = self.comparison("one-field", [sample], [right], alignment, changed)
            row = compared[0]
            differences = row.get("differences", [])
            found = (result["status"] == row["status"] == "DIFFERENT" and len(differences) == 1
                     and differences[0]["path"] == path and differences[0]["field_id"] == field_id
                     and row["left_tick"] == sample["authority_tick"] == row["right_tick"]
                     and row["first_observed_difference"] == differences[0])
            self.suite.ledger.require("W09.single_field", found, "Exactly the edited frozen field must differ at the explicit tick", [output])
        except (OSError, ValueError, KeyError, TypeError) as exc:
            self.suite.ledger.record("W09.single_field", "FAIL", str(exc), [self.root])

        left_zero, right_zero = copy.deepcopy(sample), copy.deepcopy(sample)
        self.field(left_zero, index, field_id)["raw32"] = "00000000"
        self.field(right_zero, index, field_id)["raw32"] = "80000000"
        try:
            output, _, compared = self.comparison("signed-zero", [left_zero], [right_zero], alignment,
                                                  {**changed, "left": "00000000", "right": "80000000"})
            row = compared[0]
            zero_equal = (not row.get("differences") and not self.unavailable(row, path, field_id)
                          and row["status"] in ("EQUAL_WITHIN_PROFILE", "INCOMPARABLE"))
            self.suite.ledger.require("W09.signed_zero", zero_equal,
                                      "Only this field's comparison is equal; unrelated unknowns are retained", [output])
        except (OSError, ValueError, KeyError, TypeError) as exc:
            self.suite.ledger.record("W09.signed_zero", "FAIL", str(exc), [self.root])
        try:
            evidence, finite_ok = [], True
            for label, bits in (("nan", "7fc00001"), ("positive-inf", "7f800000"), ("negative-inf", "ff800000")):
                variant = copy.deepcopy(sample)
                self.field(variant, index, field_id)["raw32"] = bits
                output, result, compared = self.comparison(label, [sample], [variant], alignment, {**changed, "after": bits})
                evidence.append(output)
                finite_ok &= (result["status"] == "INCOMPARABLE" and self.unavailable(compared[0], path, field_id)
                              and not compared[0].get("differences"))
            self.suite.ledger.require("W09.nonfinite", finite_ok, "All nonfinite variants must be FIELD_UNAVAILABLE", evidence)
        except (OSError, ValueError, KeyError, TypeError) as exc:
            self.suite.ledger.record("W09.nonfinite", "FAIL", str(exc), [self.root])

        self.wide_integer(sample)
        self.conflicts(sample, index, alignment)
        self.unknowns(sample, index, field_id, path, alignment)
        self.earlier_gap(sample, right, alignment)
        self.decoder(sample, index, field_id)

    def wide_integer(self, sample):
        key = "W08.wide_integer"
        try:
            variant = copy.deepcopy(sample)
            variant["outer_token"] = "9007199254740993"
            source = self.input("wide-u64", [variant])
            code, output, result = self.cli("wide-u64", "world-replay", ["--capture", str(source)])
            decoded = list(rows(output / "captures.jsonl")) if code == 0 else []
            passed = code == 0 and result["status"] == "REENCODED" and len(decoded) == 1
            passed &= bool(decoded) and decoded[0][2]["outer_token"] == "9007199254740993" and canonical(variant) == decoded[0][2]
            save(source / "changes.json", {"source_world": str(self.world), "scope": "OWNED_VALUE_COPY_ONLY",
                                           "field": "outer_token", "before": sample["outer_token"], "after": variant["outer_token"]})
            self.suite.ledger.require(key, passed, "Production parser/reencoder retains exact decimal-string u64", [source, output])
        except (OSError, ValueError, KeyError, TypeError) as exc:
            self.suite.ledger.record(key, "FAIL", str(exc), [self.root])

    def conflicts(self, sample, index, alignment):
        key = "W10.conflicts"
        try:
            left, right = copy.deepcopy(sample), copy.deepcopy(sample)
            duplicate = copy.deepcopy(right["objects"][index])
            duplicate["local_id"] = str(max(number(obj["local_id"]) for obj in right["objects"]) + 1)
            right["objects"].append(duplicate)
            right["required_objects"] += 1
            right["required_memberships"] += len(duplicate["memberships"])
            referrer = None
            keys = collections.Counter((obj["class"], mapped_key(obj)) for obj in sample["objects"])
            for candidate, obj in enumerate(sample["objects"]):
                if (candidate == index or mapped_key(obj) is None
                        or keys[(obj["class"], mapped_key(obj))] != 1):
                    continue
                reference = next((field["field_id"] for field in obj["fields"] if field["encoding"] == "REFERENCE"), None)
                if reference:
                    referrer = candidate, reference
                    break
            if referrer:
                for record in (left, right):
                    field = self.field(record, *referrer)
                    for payload in ("raw32", "raw64", "bytes_le"):
                        field.pop(payload, None)
                    field["status"] = "VALUE"
                    field["reference"] = {"status": "KNOWN_CURRENT", "target": {
                        "local_id": sample["objects"][index]["local_id"],
                        "use_generation": sample["objects"][index]["use_generation"]}}
            output, result, compared = self.comparison("collision", [left], [right], alignment,
                {"operation": "duplicate object under a different local_id, retain the same mapped key",
                 "duplicate_local_id": duplicate["local_id"], "reference_from": referrer})
            groups = compared[0].get("collisions", [])
            valid = result["status"] == "INCOMPARABLE" and any(len(group.get("right", [])) == 2 for group in groups)
            if referrer:
                ref_path = self.target_path(sample, referrer[0], alignment, referrer[1])
                valid &= self.unavailable(compared[0], ref_path, referrer[1])
            if not valid:
                self.suite.ledger.record(key, "FAIL", "Collision group/reference was not retained as unknown", [output])
            elif referrer is None:
                self.suite.ledger.record(key, "NOT_COVERED", "Collision covered; no second mapped reference-bearing object for target-collision case", [output])
            else:
                self.suite.ledger.record(key, "PASS", "Full group retained and a reference to it remains unknown", [output])
        except (OSError, ValueError, KeyError, TypeError) as exc:
            self.suite.ledger.record(key, "FAIL", str(exc), [self.root])

    def unknowns(self, sample, index, field_id, path, alignment):
        key = "W10.unknowns"
        evidence, checks = [], []

        def compare(label, left, right, mapping, expected):
            output, result, compared = self.comparison(label, left, right, mapping, {"operation": label})
            evidence.append(output)
            checks.append(result["status"] == "INCOMPARABLE" and expected(compared[0]))

        try:
            mapping = {**alignment, "births": [], "players": []}
            compare("no-mapping", [sample], [sample], mapping,
                    lambda row: any(item["reason"] == "IDENTITY_UNRESOLVED" for item in row["unknown"]))
            for status in ("READ_FAILED", "NOT_READY"):
                variant = copy.deepcopy(sample)
                field = self.field(variant, index, field_id)
                field["status"] = status
                field.pop("raw32")
                compare(status.lower(), [sample], [variant], alignment,
                        lambda row: self.unavailable(row, path, field_id) and not row.get("differences"))
            variant = copy.deepcopy(sample)
            variant["objects"][index]["fields"] = [
                field for field in variant["objects"][index]["fields"] if field["field_id"] != field_id]
            compare("missing-field", [sample], [variant], alignment,
                    lambda row: self.unavailable(row, path, field_id) and not row.get("differences"))
            variant = copy.deepcopy(sample)
            variant["complete"], variant["reason"] = False, "READ_FAILED"
            variant["sources"][0]["status"] = "READ_FAILED"
            removed = variant["objects"].pop(index)
            variant["required_objects"] -= 1
            variant["required_memberships"] -= len(removed["memberships"])
            compare("missing-source-object", [sample], [variant], alignment,
                    lambda row: any(item["reason"] == "PRESENCE_UNKNOWN" for item in row["unknown"]))
            compare("missing-tick", [sample], [], alignment, lambda row: row.get("reason") == "SAMPLE_GAP")
            a, b = copy.deepcopy(sample), copy.deepcopy(sample)
            a["attempt_seq"], b["attempt_seq"] = "1", "2"
            a["capture_seq"], b["capture_seq"] = "1", "2"
            compare("duplicate-tick", [sample], [a, b], alignment,
                    lambda row: row.get("reason") == "ALIGNMENT_AMBIGUOUS")
            variant = copy.deepcopy(sample)
            variant["phase"] = "OWNED_WRONG_PHASE"
            compare("wrong-phase", [sample], [variant], alignment,
                    lambda row: row.get("reason") == "ALIGNMENT_UNAVAILABLE")
            reference = next((field for field in sample["objects"][index]["fields"] if field["encoding"] == "REFERENCE"), None)
            if reference is not None:
                reference_id = reference["field_id"]
                reference_path = self.target_path(sample, index, alignment, reference_id)
                for relation in ("NULL", "KNOWN_CURRENT", "UNRESOLVED", "RETIRED", "AMBIGUOUS_REUSE"):
                    left, right = copy.deepcopy(sample), copy.deepcopy(sample)
                    for record in (left, right):
                        field = self.field(record, index, reference_id)
                        for payload in ("raw32", "raw64", "bytes_le"):
                            field.pop(payload, None)
                        field["status"] = "VALUE"
                        field["reference"] = {"status": relation}
                        if relation == "KNOWN_CURRENT":
                            field["reference"]["target"] = {"local_id": sample["objects"][index]["local_id"],
                                                            "use_generation": sample["objects"][index]["use_generation"]}
                    output, _, compared = self.comparison("ref-" + relation.lower(), [left], [right], alignment,
                                                           {"field_id": reference_id, "reference": relation})
                    evidence.append(output)
                    unavailable = self.unavailable(compared[0], reference_path, reference_id)
                    checks.append(unavailable == (relation not in ("NULL", "KNOWN_CURRENT")) and not compared[0].get("differences"))
            passed = all(checks)
            if not passed:
                self.suite.ledger.record(key, "FAIL", "At least one unknown/presence/reference case misreported", evidence)
            elif reference is None:
                self.suite.ledger.record(key, "NOT_COVERED", "Other unknowns covered; source sample has no mapped reference field", evidence)
            else:
                self.suite.ledger.record(key, "PASS", f"{len(checks)} owned-copy incomplete/reference cases checked", evidence)
        except (OSError, ValueError, KeyError, TypeError) as exc:
            self.suite.ledger.record(key, "FAIL", str(exc), evidence)

    def earlier_gap(self, sample, different, alignment):
        key = "W10.earlier_gap"
        try:
            gap = copy.deepcopy(sample)
            gap.update(record="gap", complete=False, reason="LIFECYCLE", attempt_seq="1", capture_seq=None,
                       authority_tick=None, authority_kind=0, objects=[], required_objects=0, required_memberships=0)
            left, right = copy.deepcopy(sample), copy.deepcopy(different)
            for record in (left, right):
                record["attempt_seq"], record["capture_seq"] = "2", "1"
            output, result, compared = self.comparison("earlier-gap", [gap, left], [gap, right], alignment,
                                                       {"operation": "prepend a labelled owned-copy lifecycle gap before the known field difference"})
            passed = (result["status"] == "DIFFERENT" and compared[0]["earlier_coverage_unknown"] is True
                      and result["first_observed_difference"]["earlier_coverage_unknown"] is True)
            self.suite.ledger.require(key, passed, "Earlier unavailable window is retained with the first observed difference", [output])
        except (OSError, ValueError, KeyError, TypeError) as exc:
            self.suite.ledger.record(key, "FAIL", str(exc), [self.root])

    def decoder(self, sample, index, field_id):
        key = "W10.decoder"
        try:
            evidence, checks = [], []
            for label in ("manifest-domain", "manifest-version", "record-domain", "record-version",
                          "field-id", "field-width", "field-length", "decimal-string", "extra-payload", "unavailable-value"):
                variant, manifest = copy.deepcopy(sample), copy.deepcopy(self.manifest)
                field = self.field(variant, index, field_id)
                if label == "manifest-domain":
                    manifest["domain"] = "unsupported"
                elif label == "manifest-version":
                    manifest["version"] = 2
                elif label == "record-domain":
                    variant["domain"] = "unsupported"
                elif label == "record-version":
                    variant["version"] = 2
                elif label == "field-id":
                    field["field_id"] = "ffff"
                elif label == "field-width":
                    field["width"] = 8
                elif label == "field-length":
                    field["raw32"] = "0"
                elif label == "decimal-string":
                    variant["outer_token"] = "01"
                elif label == "extra-payload":
                    field["raw64"] = "0000000000000000"
                elif label == "unavailable-value":
                    field["status"] = "READ_FAILED"
                source = self.input("invalid-" + label, [variant], manifest)
                save(source / "changes.json", {"scope": "OWNED_VALUE_COPY_ONLY", "operation": label,
                                               "source_world": str(self.world)})
                code, output, _ = self.cli("reject-" + label, "world-replay", ["--capture", str(source)])
                failure = output / "failure.json"
                rejected = code != 0 and failure.is_file() and load(failure)["status"] == "FAIL"
                evidence.append(output)
                checks.append(rejected)
            self.suite.ledger.require(key, all(checks), f"{len(checks)} malformed inputs must be rejected by production CLI", evidence)
        except (OSError, ValueError, KeyError, TypeError) as exc:
            self.suite.ledger.record(key, "FAIL", str(exc), [self.root])
