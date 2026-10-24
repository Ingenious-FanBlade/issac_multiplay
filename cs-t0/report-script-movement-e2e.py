"""Check canonical movement preservation in the single B raw-shooting capture."""
import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact-dir", required=True, type=Path)
    parser.add_argument("--controller-exit", required=True, type=int)
    args = parser.parse_args()
    root = args.artifact_dir.resolve()
    load = lambda path: json.loads(path.read_text(encoding="utf-8-sig"))
    raw = load(root / "capture/result.json")
    restoration = load(root / "capture/restoration.json")
    process = load(root / "process-after.json")
    plan = load(root / "operation-plan.json")
    fixture = load(root / "scenario-input.json")["steps"]
    expected = {row["index"]: row for row in plan["expectedRawSteps"]}
    seen, applied, legacy_applied, paths, decisions = set(), set(), set(), set(), set()
    accepted, resolved = {}, {}
    latest, origin = None, None
    evidence, violations, counts = [], [], {}
    tick = 0
    teardown = detached = False

    def fail(reason, event):
        if len(violations) < 64:
            violations.append({"reason": reason, "eventSequence": event["published"]})

    with (root / "capture/events.jsonl").open(encoding="utf-8-sig") as source:
        for line in source:
            e = json.loads(line)
            kind, who, generation = e["kind"], e["ordinal"], e["generation"]
            counts[kind] = counts.get(kind, 0) + 1
            tick = max(tick, e["tick"])
            if kind == 13 and e["detail"] == 9:
                teardown = True
            if kind == 14 and e["boolean"] and e["detail"] >= 1:
                detached = True
            if kind == 24 and who == 1:
                index = e["after"][3]
                row = expected.get(index)
                if row is None:
                    fail("Unexpected raw shooting script step", e)
                    continue
                step, state = fixture[index], e["state"]
                pair = [state["move_x"], state["move_y"]]
                original = [step.get("move_x", 0), step.get("move_y", 0)]
                if pair != original or pair != [row["move_x"], row["move_y"]]:
                    fail("Raw shooting changed the pre-encoded movement pair", e)
                if e["action"] != step["shoot_directions"] or e["query"] != row["selected"]:
                    fail("Raw shooting priority differs from the prewritten scenario", e)
                signs = (int(state["aim_x"] < 0) | int(state["aim_x"] > 0) << 1 |
                         int(state["aim_y"] < 0) << 2 | int(state["aim_y"] > 0) << 3)
                if signs != row["selected"] or bool(state["held_flags"] & 1) != bool(row["selected"]):
                    fail("Canonical shooting does not match selected directions", e)
                seen.add(index)
                latest = (index, e)
                if origin is None:
                    origin = e["target"] - step["tick"]
                if len(evidence) < 16:
                    evidence.append(e["published"])
            if kind == 4 and who == 1:
                decisions.add(e["decision"])
                if e["decision"] == 1:
                    matched = None
                    if latest and e["target"] in [latest[1]["target"], latest[1]["target"] - 1] and e["state"] == latest[1]["state"]:
                        matched = latest[0]
                    accepted[(generation, e["command"])] = (e, matched)
            if kind == 5 and who == 1:
                resolved[(generation, e["tick"])] = e
                if e["command"]:
                    record = accepted.get((generation, e["command"]))
                    if record is None or record[0]["state"] != e["state"]:
                        fail("Resolve does not match its accepted canonical input", e)
                    elif e["boolean"]:
                        if record[1] is not None:
                            applied.add(record[1])
                        if origin is not None:
                            index = e["tick"] - origin
                            if index in plan["canonicalSteps"]:
                                step = fixture[index]
                                for field in ["held_flags", "edge_flags", "move_x", "move_y", "aim_x", "aim_y"]:
                                    if e["state"][field] != step.get(field, 0):
                                        fail("Legacy canonical script changed while switching source form", e)
                                legacy_applied.add(index)
            if kind == 6 and who == 1 and e["decision"] & 1 and e["action"] < 8 and e["query"] in [1, 3]:
                state = e["state"]
                component = [state["move_x"], state["move_y"], state["aim_x"], state["aim_y"]][e["action"] // 2]
                magnitude = max(component if e["action"] & 1 else -component, 0) / 32767
                value = float(magnitude != 0) if e["query"] == 1 else magnitude
                resolve = resolved.get((generation, e["tick"]))
                if abs(e["scalar"] - value) > 0.00001 or resolve is None or resolve["state"] != state:
                    fail("Native query differs from same-tick Resolve", e)
                elif value:
                    paths.add((resolve["detail"], "move" if e["action"] < 4 else "shoot"))

    checks = []

    def check(name, observed, failed=False):
        checks.append({"name": name, "status": "FAIL" if failed else "PASS" if observed else "NOT_OBSERVED"})

    check("all-raw-scenario-movement-pairs-preserved-through-resolve", set(expected) <= seen and set(expected) <= applied)
    check("canonical-and-raw-script-switch-without-movement-change", set(plan["canonicalSteps"]) <= legacy_applied)
    check("both-native-movement-and-shooting-query-paths", all((path, family) in paths for path in [1, 2] for family in ["move", "shoot"]))
    check("duplicate-and-old-generation-rejected", {2, 12} <= decisions)
    check("single-device-registration-and-owned-native-join", all(counts.get(k) == 1 for k in [2, 11, 12]),
          any(counts.get(k, 0) > 1 for k in [2, 11, 12]))
    targets = [t for t in restoration["targets"] if t["action"] != 4294967295]
    restored = (restoration["nativeRestored"] and restoration["pinned"] and len(targets) == 48 and
                all(t["boolean"] and t["before"] == t["after"] for t in targets) and process["running"])
    check("menu-pair-absent-restored-48-hooks-resident-dll", restored and detached and teardown, not restored)
    invariants = ["duplicateResolve", "duplicateWorldCommit", "duplicateConsumer", "duplicateAccept", "inconsistentQuery"]
    native_failed = (raw.get("status") == "FAIL" or raw.get("nativeError") != 0 or raw.get("transportFailure") is not None or
                     counts.get(16, 0) or any(raw.get(k) is not False for k in invariants) or args.controller_exit not in [0, 1])
    check("native-transport-and-runtime-invariants", not native_failed, bool(native_failed))
    check("movement-and-direction-field-invariants", not violations, bool(violations))
    status = "FAIL" if any(c["status"] == "FAIL" for c in checks) else "PASS" if all(c["status"] == "PASS" for c in checks) else "NOT_OBSERVED"
    result = {"tx": "tx-01-53", "status": status, "checks": checks, "violations": violations,
              "rawSourceStepCount": len(seen), "rawResolvedStepCount": len(applied),
              "canonicalResolvedIndices": sorted(legacy_applied), "sourceEventSequences": evidence,
              "eventCount": sum(counts.values()), "maximumAuthorityTick": tick,
              "nativeError": raw.get("nativeError"), "transportFailure": raw.get("transportFailure"),
              "restoredSlotCount": len(targets), "automaticRetry": False,
              "directionChargeEvidence": "Retain tx-01-52's same-code direction selector and human charge observations; no repeated visual charge claim"}
    with (root / "result.json").open("x", encoding="utf-8") as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
        output.write("\n")
    print(json.dumps(result, ensure_ascii=False))
    return 0 if status == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
