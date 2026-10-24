"""Report the single real-game opposite-direction acceptance capture."""
import argparse
import collections
import json
import struct
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact-dir", type=Path, required=True)
    parser.add_argument("--controller-exit", type=int, required=True)
    args = parser.parse_args()
    root = args.artifact_dir.resolve()
    load = lambda path: json.loads(path.read_text(encoding="utf-8-sig"))
    raw = load(root / "capture/result.json")
    restoration = load(root / "capture/restoration.json")
    process = load(root / "process-after.json")
    plan = load(root / "operation-plan.json")
    expected_script = {}
    for segment in plan["scriptSegments"]:
        for index in range(segment["first"], segment["last"] + 1):
            expected_script[index] = segment
    transitions = {
        (1, 3, 2): "left-then-right", (2, 3, 1): "right-then-left",
        (4, 12, 8): "up-then-down", (8, 12, 4): "down-then-up",
    }
    latest, context, samples, accepted, resolved = {}, {}, {}, {}, {}
    physical_commands = {}
    evidence = collections.defaultdict(list)
    applied = collections.defaultdict(set)
    counts = collections.Counter()
    paths = set()
    violations = []
    script_seen, script_applied = set(), set()
    maximum_tick = 0

    def remember(name, e):
        if len(evidence[name]) < 16:
            evidence[name].append(e["published"])

    def fail(reason, e):
        if len(violations) < 64:
            violations.append({"reason": reason, "eventSequence": e["published"]})

    with (root / "capture/events.jsonl").open(encoding="utf-8-sig") as source:
        for line in source:
            e = json.loads(line)
            kind, who, gen = e["kind"], e["ordinal"], e["generation"]
            counts[kind] += 1
            maximum_tick = max(maximum_tick, e["tick"])
            if kind == 24:
                mask, selected = e["action"], e["query"]
                previous_mask, previous_selected, reset, index = e["after"]
                values = struct.unpack("<4f", struct.pack("<4I", *e["before"]))
                actual_mask = sum(1 << i for i, value in enumerate(values) if value > 0)
                state = e["state"]
                if mask != actual_mask or selected & ~mask or selected & 3 == 3 or selected & 12 == 12:
                    fail("Source selected an absent or opposing pair of directions", e)
                prior = latest.get(who)
                if reset:
                    context.pop(who, None)
                    if previous_mask or previous_selected:
                        fail("New source identity retained a prior direction history", e)
                elif prior and e["after"][:2] != [prior["action"], prior["query"]]:
                    fail("Source history does not follow its own previous sample", e)
                if mask == previous_mask and selected != previous_selected:
                    fail("Unchanged held directions changed priority", e)
                if e["detail"] in [1, 3]:
                    if bool(mask) != bool(state["held_flags"] & 1) or bool(mask) != bool(state["aim_x"] or state["aim_y"]):
                        fail("Digital held shooting became neutral or persisted after all keys released", e)
                    signs = (int(state["aim_x"] < 0) | int(state["aim_x"] > 0) << 1 |
                             int(state["aim_y"] < 0) << 2 | int(state["aim_y"] > 0) << 3)
                    if signs != selected:
                        fail("Canonical aim disagrees with the selected source directions", e)
                names = []
                transition = transitions.get((previous_mask, mask, selected))
                if transition:
                    context[who] = (mask, selected, [transition])
                elif who in context and context[who][:2] != (mask, selected):
                    context.pop(who, None)
                if who in context:
                    names.extend(context[who][2])
                if previous_mask in [3, 12] and mask in [1, 2, 4, 8] and selected == mask:
                    name = "release-winner-fallback" if previous_selected != mask else "release-nonwinner-stable"
                    context[who] = (mask, selected, [name])
                    names.append(name)
                if mask and mask == previous_mask and selected == previous_selected:
                    names.append("held-priority-stable")
                if not mask:
                    names.append("all-released-neutral")
                if mask in [5, 6, 9, 10] and selected == mask:
                    names.append("adjacent-overlap")
                if previous_mask == 0 and mask in [3, 12, 15]:
                    if selected != mask & 10:
                        fail("First same-sample opposite keys did not use the accepted right/down fallback", e)
                    names.append("first-tie-right-down")
                if e["detail"] == 3:
                    segment = expected_script.get(index)
                    if segment is None or mask != segment["mask"] or selected != segment["selected"]:
                        fail("B source did not follow the prewritten scenario direction", e)
                    else:
                        script_seen.add(segment["name"])
                        names.append("script-" + segment["name"])
                for name in set(names):
                    remember(f"{who}-{name}", e)
                samples[e["published"]] = (who, set(names), e)
                latest[who] = e
            if kind == 17:
                source_event = latest.get(0)
                if source_event is None or source_event["state"] != e["state"]:
                    # Edge publication may remove an already consumed edge; levels must match.
                    before = dict(source_event["state"]) if source_event else {}
                    after = dict(e["state"])
                    before["edge_flags"] = after["edge_flags"] = 0
                    if before != after:
                        fail("A command lacks a matching fresh source conversion", e)
                if source_event:
                    physical_commands[(gen, e["command"])] = source_event["published"]
            if kind == 4 and e["decision"] == 1:
                source_id = physical_commands.get((gen, e["command"])) if who == 0 else None
                if who == 1:
                    candidate = latest.get(1)
                    if candidate and candidate["state"] == e["state"] and e["target"] in [candidate["target"], candidate["target"] - 1]:
                        source_id = candidate["published"]
                accepted[(who, gen, e["command"])] = (e, source_id)
            if kind == 4 and e["decision"] in [2, 12]:
                remember(f"decision-{e['decision']}", e)
            if kind == 5:
                resolved[(who, gen, e["tick"])] = e
                record = accepted.get((who, gen, e["command"]))
                if e["command"] and record is None:
                    fail("Resolved command lacks same-generation acceptance", e)
                if record:
                    command, source_id = record
                    if command["state"] != e["state"]:
                        fail("Resolve changed the accepted input state", e)
                    if source_id in samples and e["boolean"]:
                        sample_who, names, _ = samples[source_id]
                        if sample_who != who:
                            fail("A/B source histories crossed participants", e)
                        for name in names:
                            applied[who].add(name)
                            remember(f"applied-{who}-{name}", e)
                            if name.startswith("script-"):
                                script_applied.add(name[7:])
            if kind == 6 and e["decision"] & 1 and e["action"] < 8 and e["query"] in [1, 3]:
                state = e["state"]
                component = [state["move_x"], state["move_y"], state["aim_x"], state["aim_y"]][e["action"] // 2]
                magnitude = max(component if e["action"] & 1 else -component, 0) / 32767
                expected = float(magnitude != 0) if e["query"] == 1 else magnitude
                if abs(e["scalar"] - expected) > 0.00001:
                    fail("Eligible native query disagrees with Resolve", e)
                resolve = resolved.get((who, gen, e["tick"]))
                if resolve is None or resolve["state"] != state:
                    fail("Query lacks a matching same-tick Resolve", e)
                elif e["scalar"] and 4 <= e["action"] < 8:
                    paths.add((who, resolve["detail"]))
                    remember(f"query-{who}-{resolve['detail']}", e)
            if kind == 13 and e["detail"] == 9:
                remember("teardown", e)
            if kind == 14 and e["boolean"] and e["detail"] >= 1:
                remember("detached", e)

    checks = []

    def check(name, seen, evidence_keys=(), failed=False):
        checks.append({"name": name, "status": "FAIL" if failed else "PASS" if seen else "NOT_OBSERVED",
                       "evidenceFile": "capture/events.jsonl",
                       "eventSequences": sorted({n for key in evidence_keys for n in evidence[key]})})

    for who in [0, 1]:
        for name in list(transitions.values()) + ["release-winner-fallback", "release-nonwinner-stable", "held-priority-stable", "all-released-neutral", "adjacent-overlap"]:
            check(f"participant-{who}-{name}-source-and-resolve", name in applied[who],
                  [f"{who}-{name}", f"applied-{who}-{name}"])
        for path in [1, 2]:
            check(f"participant-{who}-native-query-path-{path}", (who, path) in paths, [f"query-{who}-{path}"])
    segment_names = {segment["name"] for segment in plan["scriptSegments"]}
    check("B-complete-prewritten-script-segments", segment_names <= script_seen and segment_names <= script_applied)
    check("B-first-sample-tie-right-down", "first-tie-right-down" in applied[1], ["1-first-tie-right-down", "applied-1-first-tie-right-down"])
    check("duplicate-and-old-generation-rejected", bool(evidence["decision-2"] and evidence["decision-12"]), ["decision-2", "decision-12"])
    check("single-device-registration-and-owned-native-join", counts[2] == counts[11] == counts[12] == 1,
          failed=any(counts[k] > 1 for k in [2, 11, 12]))
    visual_path = root / "manual-shooting-observations.json"
    visual = load(visual_path) if visual_path.exists() else {}
    for field in plan["manualObservationFields"]:
        check("visual-" + field, visual.get(field) == "PASS", failed=visual.get(field) == "FAIL")
    targets = [t for t in restoration["targets"] if t["action"] != 4294967295]
    restored = (restoration["nativeRestored"] and restoration["pinned"] and len(targets) == 48 and
                all(t["boolean"] and t["before"] == t["after"] for t in targets) and process["running"])
    check("menu-pair-absent-48-hooks-restored-dll-resident", restored and bool(evidence["teardown"] and evidence["detached"]),
          ["teardown", "detached"], failed=not restored)
    invariants = ["duplicateResolve", "duplicateWorldCommit", "duplicateConsumer", "duplicateAccept", "inconsistentQuery"]
    native_failed = (raw.get("status") == "FAIL" or raw.get("nativeError") != 0 or raw.get("transportFailure") is not None or
                     counts[16] or any(raw.get(k) is not False for k in invariants) or args.controller_exit not in [0, 1])
    check("native-transport-and-runtime-invariants", not native_failed, failed=bool(native_failed))
    check("direction-source-queue-resolve-query-invariants", not violations, failed=bool(violations))
    status = "FAIL" if any(c["status"] == "FAIL" for c in checks) else "PASS" if all(c["status"] == "PASS" for c in checks) else "NOT_OBSERVED"
    result = {"tx": "tx-01-52", "status": status, "checks": checks, "violations": violations,
              "eventCount": sum(counts.values()), "maximumAuthorityTick": maximum_tick,
              "scriptSegmentsObserved": sorted(script_seen), "scriptSegmentsApplied": sorted(script_applied),
              "rawFullFeatureStatus": raw.get("status"), "nativeError": raw.get("nativeError"),
              "transportFailure": raw.get("transportFailure"), "restoredSlotCount": len(targets),
              "manualObservations": visual, "automaticRetry": False,
              "scope": "Opposite shooting priority; room auto-fire and unaccepted Join path remain unresolved"}
    with (root / "result.json").open("x", encoding="utf-8") as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
        output.write("\n")
    print(json.dumps({"tx": "tx-01-52", "status": status, "checks": checks, "violations": violations}, ensure_ascii=False))
    return 0 if status == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
