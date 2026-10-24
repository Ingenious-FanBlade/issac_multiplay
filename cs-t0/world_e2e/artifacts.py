"""Bounded artifact I/O and narrowly scoped acceptance observations."""
from __future__ import annotations

import collections
import json
from datetime import datetime, timezone
from pathlib import Path

MAX_RECORD = 128 * 1024 * 1024
CLASSES = ("Player", "NPC", "Tear", "Projectile", "Pickup", "Familiar", "Effect", "Grid", "UnknownEntity")
PHASE = "POST_STANDARD_UPDATE_PRE_TOP_EXIT"
DOMAIN = "isaac.cs.t0.world-record"
PROFILE = "min-read-v1"
CONTINUITY_RULE = "ORDINARY_FULL_PROFILE_ALLOW_EXPLICIT_ODD_GAPS"

# Automatic assertions describe only facts present in saved files. Manual
# obligations cannot be upgraded by a related automatic assertion.
OBLIGATIONS = {
    "W01": {
        "default_51": "Default v3, no world mapping, 51 original targets restored",
        "optional_70": "Optional v4, fixed world manifest, 70 original targets restored",
        "native_results": "Original ABI, return values and call counts preserved",  # manual
    },
    "W02": {
        "record_invariants": "Saved sequence, phase, thread and Authority-kind invariants",
        "phase_order": "Observed owner/Room/tail order and exact Authority commit",  # manual
        "blocked_windows": "Odd/pause/loading/lifecycle markers agree with actual gaps",  # manual
    },
    "W03": {
        "profile": "Saved records carry the fixed manifest's full field sets",
        "eight_classes": "All eight classes observed with VALUE fields",
        "sources_alias": "All 13 implemented sources and a Grid/door alias observed",
        "retained_unknown": "Retained Players, relation ambiguity and unknown-class handling",  # manual
    },
    "W04": {
        "creation_events": "Factory/Init/Publish/Spawn and confirmed birth events observed",
        "identity_history": "Nested/short-lived/reused objects and membership histories",  # manual
    },
    "W05": {"transitions": "Morph/Restore/Replace/room/run identity and retirement histories"},
    "W06": {
        "bootstrap_witness": "FIRST_OBSERVED A has evidence 3 without fabricated birth",
        "revocation": "Exact-pair/generation invalidation revokes and reacquires readiness",  # manual
    },
    "W07": {
        "continuous_10000": "10,000 contiguous full-profile ordinary captures; explicit odd-only gaps retained and excluded",
        "consumer_pipeline": "Mapping v2 consumer copied/written counts, bounded queue, final flush and complete drain",
        "readonly_calls": "Runtime read-only, RNG, thread, native calls and timing evidence",  # manual
    },
    "W08": {
        "replay": "Two production replays preserve canonical records and exact output bytes",
        "wide_integer": "Owned-copy u64 > 2^53 survives production parsing/reencoding",
    },
    "W09": {
        "single_field": "One owned-copy field change is located at the paired tick/entity/field",
        "signed_zero": "Signed-zero field equality retains both original bit patterns",
        "nonfinite": "NaN/Inf are unavailable for comparison, never silently equal",
    },
    "W10": {
        "conflicts": "Full collision groups and references to them remain incomparable",
        "unknowns": "Missing mapping/references/fields/sources/ticks and wrong phase stay unknown",
        "decoder": "Malformed schema/field/width/decimal/payload is rejected with failure artifact",
        "earlier_gap": "A saved earlier gap prevents a global-first divergence claim",
    },
    "W11": {
        "production_compare": "Explicit alignment drives production comparison of two process captures",
        "scenario_basis": "Actual seed/input/scenario and creation/tick alignment basis",  # manual
    },
    "W12": {
        "fault_isolation": "Controlled consumer/resource fault and actual scheduler continuity evidence",
    },
    "W13": {
        "normal_restore": "70 unique targets, closed producer, balanced entry calls and drained events",
        "controller_loss": "Actual controller loss, publication stop and raw recovery evidence",  # manual
    },
}
MANUAL = {
    "W01.native_results", "W02.phase_order", "W02.blocked_windows",
    "W03.retained_unknown", "W04.identity_history", "W05.transitions", "W06.revocation",
    "W07.readonly_calls", "W11.scenario_basis", "W12.fault_isolation", "W13.controller_loss",
}


def utc():
    return datetime.now(timezone.utc).isoformat()


def _unique(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def decode(raw):
    return json.loads(raw, object_pairs_hook=_unique,
                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError(f"non-JSON number: {value}")))


def load(path):
    return decode(Path(path).read_text(encoding="utf-8-sig"))


def save(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2, allow_nan=False)
        stream.write("\n")


def append(path, value):
    with Path(path).open("a", encoding="utf-8", newline="\n") as stream:
        stream.write(json.dumps(value, ensure_ascii=False, separators=(",", ":"), allow_nan=False) + "\n")


def same_file_bytes(left, right):
    """Exact streaming byte comparison, without any digest."""
    with Path(left).open("rb") as a, Path(right).open("rb") as b:
        while True:
            ac, bc = a.read(1024 * 1024), b.read(1024 * 1024)
            if ac != bc:
                return False
            if not ac:
                return True


def rows(path):
    with Path(path).open("rb") as stream:
        offset = 0
        while True:
            raw = stream.readline(MAX_RECORD + 1)
            if not raw:
                return
            if len(raw) > MAX_RECORD:
                raise ValueError(f"JSONL record exceeds 128 MiB: {path}, offset {offset}")
            yield offset, raw, decode(raw)
            offset += len(raw)


def number(value):
    if not isinstance(value, str) or not value.isascii() or not value.isdecimal():
        raise ValueError(f"expected u64 decimal string: {value!r}")
    parsed = int(value)
    if str(parsed) != value or parsed > 2**64 - 1:
        raise ValueError(f"noncanonical/out-of-range u64: {value!r}")
    return parsed


def canonical(record):
    # Mirrors the local replay order, without inventing a comparison identity.
    result = dict(record)
    result["sources"] = sorted(record["sources"], key=lambda source: source["source"])
    result["objects"] = []
    for obj in record["objects"]:
        obj = dict(obj)
        obj["fields"] = sorted(obj["fields"], key=lambda field: field["field_id"])
        obj["memberships"] = sorted(obj["memberships"], key=lambda member: (member["source"], member["index"]))
        result["objects"].append(obj)
    result["objects"].sort(key=lambda obj: (CLASSES.index(obj["class"]), number(obj["local_id"]), number(obj["use_generation"])))
    return result


def first_value_difference(left, right, path=()):
    """A readable location, including raw bits, when whole-record equality fails."""
    if type(left) is not type(right):
        return {"path": list(path), "left_type": type(left).__name__, "right_type": type(right).__name__}
    if isinstance(left, dict):
        for key in sorted(left.keys() | right.keys()):
            if key not in left or key not in right:
                return {"path": [*path, key], "left_present": key in left, "right_present": key in right}
            different = first_value_difference(left[key], right[key], (*path, key))
            if different is not None:
                return different
    elif isinstance(left, list):
        if len(left) != len(right):
            return {"path": list(path), "left_count": len(left), "right_count": len(right)}
        for index, (a, b) in enumerate(zip(left, right)):
            different = first_value_difference(a, b, (*path, index))
            if different is not None:
                return different
    elif left != right:
        return {"path": list(path), "left": left, "right": right}
    return None


def full(record):
    return (
        record["record"] == "capture" and record["complete"] and record["reason"] == "NONE"
        and record["phase"] == PHASE and record["authority_kind"] == 1
        and record["authority_tick"] is not None
        and any(obj["class"] == "Player" and obj["ordinal"] == 0 for obj in record["objects"])
        and len(record["sources"]) == 15
        and all(any(source["source"] == index and source["status"] == "NONE"
                    for source in record["sources"]) for index in range(13))
        and all(obj["class"] != "UnknownEntity" and obj["fields"] and all(field["status"] == "VALUE" for field in obj["fields"])
                for obj in record["objects"])
    )


def record_sequence(record, previous_attempt, previous_capture, session):
    attempt = number(record["attempt_seq"])
    capture = number(record["capture_seq"]) if record["capture_seq"] is not None else previous_capture
    valid = (attempt > previous_attempt and capture >= previous_capture and record["phase"] == PHASE
             and record["session"] == session)
    if record["capture_seq"] is not None:
        valid &= capture == previous_capture + 1
    if record["record"] == "capture":
        # A complete phase without this token's Authority commit may retain
        # an unavailable tick; it cannot join an ordinary comparison window.
        valid &= (record["authority_kind"] == 1 if record["authority_tick"] is not None
                  else record["authority_kind"] == 0)
    if record["authority_kind"] == 2:
        valid &= record["record"] == "gap" and not record["complete"]
    return bool(valid), attempt, capture


def explicit_odd_gap(record):
    return (
        record["record"] == "gap" and record["reason"] == "ODD_PLAYER_ONLY"
        and record["authority_kind"] == 2 and record["authority_tick"] is not None
        and record["complete"] is False and record["capture_seq"] is None
        and not record["objects"] and record["required_objects"] == 0 and record["required_memberships"] == 0
        and record["phase"] == PHASE
        and len(record["sources"]) == 15
        and {source["source"] for source in record["sources"]} == set(range(15))
        and all(source["status"] == "NONE" and source["count"] == 0 for source in record["sources"])
    )


class OrdinaryWindow:
    """Shared live/final count; odd gaps preserve, but never increase, the count."""
    def __init__(self):
        self.current = self.longest = self.odd_records = self.current_odd = self.longest_odd = 0
        self.previous = None
        self.start = self.best_start = self.best_end = None

    def advance(self, record, full_profile, valid):
        tick = number(record["authority_tick"]) if record["authority_tick"] is not None else None
        state = (
            (record["run_epoch"], record["room_epoch"], record["mutation_epoch"], record["diagnostics"]["thread"]),
            number(record["attempt_seq"]), tick, number(record["outer_token"]),
        )
        consecutive = (
            valid and self.previous is not None and state[0] == self.previous[0]
            and state[1] == self.previous[1] + 1 and tick is not None and tick > self.previous[2]
            and state[3] > self.previous[3]
        )
        odd = valid and explicit_odd_gap(record)
        if odd:
            self.odd_records += 1
        if full_profile and valid:
            if not consecutive:
                self.current = self.current_odd = 0
                self.start = record["attempt_seq"]
            self.current += 1
            self.previous = state
            if self.current > self.longest:
                self.longest, self.longest_odd = self.current, self.current_odd
                self.best_start, self.best_end = self.start, record["attempt_seq"]
            return "COUNT_ORDINARY"
        if odd and consecutive:
            self.current_odd += 1
            self.previous = state
            return "RETAIN_ODD_NOT_COUNTED"
        self.current = self.current_odd = 0
        self.previous = self.start = None
        return "ODD_OUTSIDE_INTERVAL" if odd else "BREAK_INTERVAL"


def mapped_key(obj):
    if obj["class"] == "Player" and obj["ordinal"] in (0, 1):
        return ("player", obj["ordinal"])
    if obj["birth_event"] is not None:
        birth = obj["birth"]
        return ("birth", obj["birth_event"], obj["object_domain"],
                *(birth[name] for name in ("kind", "entity_type", "variant", "subtype", "seed")))
    return None


def select_target(record):
    if record["record"] != "capture" or record["authority_kind"] != 1 or record["authority_tick"] is None:
        return None
    keys = collections.Counter((obj["class"], mapped_key(obj)) for obj in record["objects"] if mapped_key(obj) is not None)
    for index, obj in enumerate(record["objects"]):
        key = mapped_key(obj)
        if key is None or keys[(obj["class"], key)] != 1:
            continue
        for field in obj["fields"]:
            if (field["field_id"] == "000b" and field["status"] == "VALUE"
                    and int(field["raw32"], 16) & 0x7f800000 != 0x7f800000):
                return index, field["field_id"]
    return None


class Ledger:
    def __init__(self, root):
        self.root = Path(root)
        self.transaction = "tx-01-74"
        self.scope = "Only evidence saved by this invocation; manual reviews are identified"
        self.items = {}
        for group, obligations in OBLIGATIONS.items():
            for leaf, title in obligations.items():
                key = f"{group}.{leaf}"
                self.items[key] = {"id": key, "title": title, "status": "NOT_COVERED",
                                   "reason": "No sufficient evidence collected", "evidence": [],
                                   "method": "MANUAL_REVIEW" if key in MANUAL else "AUTOMATED"}

    def record(self, key, status, reason, evidence=()):
        if key not in self.items or status not in ("PASS", "FAIL", "NOT_COVERED", "NOT_RUN"):
            raise ValueError(f"invalid obligation/status: {key}, {status}")
        # A later observation must not erase a failure from this invocation.
        if self.items[key]["status"] == "FAIL" and status != "FAIL":
            raise ValueError(f"cannot replace a failed obligation: {key}")
        row = {**self.items[key], "status": status, "reason": reason,
               "evidence": [str(item) for item in evidence], "recorded_at_utc": utc()}
        self.items[key] = row
        append(self.root / "checks.jsonl", row)

    def require(self, key, condition, reason, evidence=()):
        self.record(key, "PASS" if condition else "FAIL", reason, evidence)
        return bool(condition)

    def export(self, failure=None, interrupted=False):
        groups = []
        for group in OBLIGATIONS:
            children = [row for key, row in self.items.items() if key.startswith(group + ".")]
            statuses = {row["status"] for row in children}
            status = "FAIL" if "FAIL" in statuses else "PASS" if statuses == {"PASS"} else "NOT_COVERED"
            groups.append({"id": group, "status": status, "checks": children})
        status = "FAIL" if failure or any(group["status"] == "FAIL" for group in groups) else (
            "PASS" if all(group["status"] == "PASS" for group in groups) else "INCOMPLETE")
        if interrupted:
            status = "INTERRUPTED"
        for group in groups:
            append(self.root / "cases.jsonl", group)
        result = {"transaction": self.transaction, "status": status, "ended_at_utc": utc(),
                  "error": failure, "cases": groups, "automatic_retry": False,
                  "required_completion_record": "finalization.json",
                  "hashes": "NOT_USED", "scope": self.scope}
        save(self.root / "result.json", result)
        return result


def restoration(directory, count):
    evidence = load(Path(directory) / "restoration.json")
    targets = [row for row in evidence["targets"] if row["action"] != 2**32 - 1]
    catalogs = [row for row in evidence["targets"] if row["action"] == 2**32 - 1]
    valid = (
        evidence["nativeRestored"] is True and evidence["pinned"] is True
        and len(targets) == count and len({row["action"] for row in targets}) == count
        and all(row["boolean"] == 1 and row["before"] == row["after"] and 0 < row["query"] <= 16 for row in targets)
        and any(row["boolean"] == 1 and row["detail"] == count for row in catalogs)
    )
    return valid


def scan_world(directory, report_path):
    """Stream once; keep coverage and one real frozen sample, never 10k worlds."""
    directory = Path(directory)
    manifest = load(directory / "manifest.json")
    if manifest["domain"] != DOMAIN or manifest["version"] != 1 or manifest["profile"] != PROFILE:
        raise ValueError("world manifest domain/version/profile mismatch")
    counts = collections.Counter()
    classes, readable_classes, sources = set(), set(), set()
    class_witnesses = {}
    scopes, births = {}, {}
    previous_attempt = previous_capture = 0
    window = OrdinaryWindow()
    threads = set()
    alias = False
    witness_ids, witness_events = set(), set()
    sample = None
    invariants, profile = [], []
    with Path(report_path).open("x", encoding="utf-8") as details:
        for offset, _, record in rows(directory / "captures.jsonl"):
            counts["records"] += 1
            valid, attempt, capture = record_sequence(record, previous_attempt, previous_capture, manifest["session"])
            if not valid and len(invariants) < 20:
                invariants.append({"offset": offset, "attempt": record["attempt_seq"]})
            counts[record["record"]] += 1
            counts["reason:" + record["reason"]] += 1
            threads.add(record["diagnostics"]["thread"])
            field_complete = True
            for obj in record["objects"]:
                classes.add(obj["class"])
                expected = set(manifest["required"][obj["class"]])
                actual = {field["field_id"] for field in obj["fields"]}
                if obj["fields"] and expected == actual and all(field["status"] == "VALUE" for field in obj["fields"]):
                    readable_classes.add(obj["class"])
                    class_witnesses.setdefault(obj["class"], {"source_offset": offset, "attempt_seq": record["attempt_seq"],
                        "authority_tick": record["authority_tick"], "run_epoch": record["run_epoch"],
                        "room_epoch": record["room_epoch"], "object": obj})
                if obj["class"] != "UnknownEntity" and expected != actual:
                    field_complete = False
                members = {member["source"] for member in obj["memberships"]}
                alias |= obj["class"] == "Grid" and {11, 12}.issubset(members)
                if (obj["class"] == "Player" and obj["ordinal"] == 0 and obj["origin"] == "FIRST_OBSERVED"
                        and obj["init_evidence"] == 3 and obj["birth_event"] is None):
                    witness_ids.add((record["run_epoch"], obj["local_id"], obj["use_generation"]))
                if obj["birth_event"] is not None:
                    key = (record["run_epoch"], obj["birth_event"])
                    births.setdefault(key, {"run_epoch": key[0], "birth_event": key[1], "birth": obj["birth"]})
            if not field_complete and len(profile) < 20:
                profile.append({"offset": offset, "attempt": record["attempt_seq"]})
            sources.update(source["source"] for source in record["sources"] if source["status"] == "NONE")
            scope_key = (record["run_epoch"], record["room_epoch"])
            scope = scopes.setdefault(scope_key, {"run_epoch": scope_key[0], "room_epoch": scope_key[1],
                                                   "first_tick": None, "last_tick": None, "records": 0})
            scope["records"] += 1
            if record["authority_tick"] is not None:
                scope["first_tick"] = scope["first_tick"] or record["authority_tick"]
                scope["last_tick"] = record["authority_tick"]
            disposition = window.advance(record, full(record) and field_complete, valid)
            details.write(json.dumps({"attempt_seq": record["attempt_seq"], "capture_seq": record["capture_seq"],
                                      "authority_tick": record["authority_tick"], "full_profile": full(record) and field_complete,
                                      "record": record["record"], "reason": record["reason"], "offset": offset,
                                      "continuity_action": disposition, "ordinary_count": window.current,
                                      "odd_gaps_in_current_interval": window.current_odd}) + "\n")
            if sample is None and select_target(record) is not None:
                sample = record
            previous_attempt, previous_capture = attempt, capture
    hook_enters, hook_returns = collections.Counter(), collections.Counter()
    event_kinds, event_reasons = collections.Counter(), collections.Counter()
    event_sequence, event_gaps = 0, []
    for offset, _, event in rows(directory / "lifecycle-events.jsonl"):
        sequence = number(event["sequence"])
        if sequence != event_sequence + 1 and len(event_gaps) < 20:
            event_gaps.append({"offset": offset, "previous": str(event_sequence), "current": event["sequence"]})
        event_sequence = sequence
        if event["event_kind"] == 1:
            for witness in class_witnesses.values():
                obj = witness["object"]
                if (witness["run_epoch"] == event["run_epoch"] and obj["local_id"] == event["local_id"]
                        and obj["use_generation"] == event["use_generation"] and obj["birth_event"] == event["birth_event"]):
                    witness["birth_event_evidence"] = event
        event_kinds[str(event["event_kind"])] += 1
        event_reasons[event["reason"]] += 1
        if event["event_kind"] == 6:
            witness_events.add((event["run_epoch"], event["local_id"], event["use_generation"]))
        if event["event_kind"] == 0:
            (hook_enters if event["entering"] else hook_returns)[event["hook"]] += 1
    result = load(directory / "result.json")
    summary = {
        "counts": dict(counts), "classes": sorted(classes), "readable_classes": sorted(readable_classes),
        "class_witnesses": class_witnesses,
        "sources": sorted(sources), "grid_door_alias": alias, "bootstrap_witness": bool(witness_ids & witness_events),
        "threads": sorted(threads), "invariant_failures": invariants, "profile_failures": profile,
        "continuity_rule": CONTINUITY_RULE, "longest_full_profile": window.longest,
        "explicit_odd_gap_records": window.odd_records,
        "interval": {"first_attempt": window.best_start, "last_attempt": window.best_end,
                     "ordinary_captures": window.longest, "odd_gaps_not_counted": window.longest_odd},
        "event_gaps": event_gaps, "event_kinds": dict(event_kinds), "event_reasons": dict(event_reasons),
        "hook_enters": dict(hook_enters), "hook_returns": dict(hook_returns),
        "last_event": str(event_sequence), "last_attempt": str(previous_attempt), "last_capture": str(previous_capture),
        "scopes": list(scopes.values()), "births": list(births.values()), "producer_result": result,
        "evidence_limit": "Saved-value checks alone do not prove native ABI/read-only behavior or scene causality",
    }
    return summary, sample


class LiveProgress:
    """Advisory progress from completed JSONL lines; final offline scan decides."""
    def __init__(self, world):
        self.world = Path(world)
        self.offset = self.records = self.ordinary = self.previous_attempt = self.previous_capture = 0
        self.window = OrdinaryWindow()

    def read(self):
        path = self.world / "captures.jsonl"
        manifest_path = self.world / "manifest.json"
        if not path.is_file() or not manifest_path.is_file():
            return {"status": "WAITING_FOR_WORLD_FILES"}
        manifest = load(manifest_path)
        with path.open("rb") as stream:
            stream.seek(self.offset)
            while True:
                raw = stream.readline(MAX_RECORD + 1)
                if len(raw) > MAX_RECORD:
                    raise ValueError("live JSONL record exceeds 128 MiB")
                if not raw or not raw.endswith(b"\n"):
                    break  # The producer may still be writing the last record.
                record = decode(raw)
                self.offset += len(raw)
                self.records += 1
                self.ordinary += record["record"] == "capture" and record["authority_kind"] == 1
                fields = all({field["field_id"] for field in obj["fields"]} == set(manifest["required"][obj["class"]])
                             for obj in record["objects"] if obj["class"] != "UnknownEntity")
                valid, attempt, capture = record_sequence(record, self.previous_attempt, self.previous_capture, manifest["session"])
                self.window.advance(record, full(record) and fields, valid)
                self.previous_attempt, self.previous_capture = attempt, capture
        return {"records": self.records, "ordinary_captures": self.ordinary,
                "continuity_rule": CONTINUITY_RULE,
                "current_full_profile": self.window.current, "longest_full_profile": self.window.longest,
                "explicit_odd_gap_records": self.window.odd_records,
                "odd_gaps_in_current_interval": self.window.current_odd,
                "odd_gaps_in_longest_interval": self.window.longest_odd,
                "acceptance": "ADVISORY_ONLY_FINAL_SCAN_REQUIRED"}
