"""Analyze only the capture produced by the single startup-input acceptance."""
import argparse
import collections
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
    accepted, resolved, bindings = {}, {}, {}
    startups, physical, violations = [], [], []
    evidence = collections.defaultdict(list)
    counts = collections.Counter()
    last_binding = {}
    active_binding = {}
    room_generations = {}
    startup_sources = {}
    latest_physical = None
    path_inputs = set()
    explicit = collections.defaultdict(set)
    maximum_tick = 0
    script_complete_tick = None

    def remember(key, e):
        if len(evidence[key]) < 24:
            evidence[key].append(e["published"])

    def fail(reason, e):
        if len(violations) < 64:
            violations.append({"reason": reason, "eventSequence": e["published"]})

    with (root / "capture/events.jsonl").open(encoding="utf-8-sig") as source:
        for line in source:
            e = json.loads(line)
            kind, ordinal, generation = e["kind"], e["ordinal"], e["generation"]
            counts[kind] += 1
            maximum_tick = max(maximum_tick, e["tick"])
            identity = (ordinal, generation)
            if kind == 18:
                script_complete_tick = e["tick"]
                remember("scriptComplete", e)
            if kind == 4 and e["decision"] in [2, 12]:
                remember(f"decision{e['decision']}", e)
            if kind == 3:
                bindings[identity] = e
                last_binding[ordinal] = e
                active_binding[ordinal] = generation
            if kind == 13:
                active_binding.pop(ordinal, None)
                if e["detail"] == 3:
                    room_generations[(ordinal, generation)] = e["tick"]
                    remember("room", e)
                if e["detail"] in [1, 2]:
                    remember("pauseResume", e)
                if e["detail"] == 9:
                    remember("teardown", e)
            if kind == 4 and e["decision"] == 1:
                accepted[(ordinal, generation, e["command"])] = e
            if kind == 5:
                resolved[(ordinal, generation, e["tick"])] = e
                if ordinal == 1 and script_complete_tick is not None and e["tick"] > script_complete_tick + 2 and e["command"] == 0 and not any(e["state"][k] for k in ["held_flags", "edge_flags", "move_x", "move_y", "aim_x", "aim_y"]):
                    remember("missingNeutral", e)
                if ordinal == 1 and e["state"]["edge_flags"]:
                    explicit[identity].add(e["tick"])
                    remember("explicitEdge", e)
            if kind == 6 and e["decision"] & 1:
                if e["scalar"] and e["action"] < 8:
                    path = resolved.get((ordinal, generation, e["tick"]), {}).get("detail")
                    path_inputs.add((ordinal, path, "move" if e["action"] < 4 else "shoot"))
                    remember(f"input-{ordinal}-{path}", e)
                if e["action"] >= 8 and e["query"] == 2 and e["boolean"]:
                    remember("edgeQuery", e)
                if e["action"] < 8 and e["query"] in [1, 3]:
                    state = e["state"]
                    component = [state["move_x"], state["move_y"], state["aim_x"], state["aim_y"]][e["action"] // 2]
                    value = max(component if e["action"] & 1 else -component, 0) / 32767
                    expected = float(value != 0) if e["query"] == 1 else value
                    if abs(e["scalar"] - expected) > 0.00001:
                        fail("Eligible native query disagrees with the resolved input", e)
            if kind == 7 and ordinal < 2 and e["action"] == 0 and e["query"] in [0x1895C, 0x2FD60C] and active_binding.get(ordinal) == generation:
                fail("Ordinary native Player simulation is outside Authority", e)
            if kind == 22:
                startups.append(e)
                if ordinal == 0:
                    startup_sources[e["published"]] = latest_physical
                remember(f"startup{ordinal}", e)
            if kind == 23:
                physical.append(e)
                latest_physical = e
                shooting = (e["action"] >> 4) & 15
                if shooting in [5, 9, 6, 10]:
                    remember("overlap", e)
                    state = e["state"]
                    if not (state["held_flags"] & 1 and state["aim_x"] and state["aim_y"]):
                        fail("Adjacent physical shooting overlap became canonical neutral", e)
            if kind == 14 and e["boolean"] and e["detail"] >= 1:
                remember("detached", e)

    complete = collections.Counter()
    startup_sequences = set()
    room_startups = 0
    started_generations = set()
    for e in startups:
        who, gen, tick = e["ordinal"], e["generation"], e["target"]
        if (who, gen) in started_generations:
            fail("More than one startup in the same participant generation", e)
        started_generations.add((who, gen))
        first = accepted.get((who, gen, e["command"]))
        sequence = e["before"][0] | e["before"][1] << 32
        nominal_tick = e["before"][2] | e["before"][3] << 32
        nominal = accepted.get((who, gen, sequence))
        startup_sequences.add((who, gen, e["command"]))
        binding = bindings.get((who, gen))
        if who == 0:
            observed = startup_sources.get(e["published"])
            if observed is None or observed["target"] != e["query"] or observed["before"] != e["after"]:
                fail("Physical startup has no exact-pair poll from the same native loop", e)
            else:
                source_levels = dict(observed["state"])
                startup_levels = dict(e["state"])
                source_levels["edge_flags"] = startup_levels["edge_flags"] = 0
                allowed_edges = observed["after"][1] & ~observed["after"][0]
                if source_levels != startup_levels or e["state"]["edge_flags"] & ~allowed_edges:
                    fail("Physical startup changed fresh levels or replayed a consumed edge", e)
        if first is None or nominal is None or binding is None:
            fail("Startup lacks accepted commands or unique generation binding", e)
            continue
        if tick != e["tick"] + 1 or nominal_tick != e["tick"] + 2 or first["target"] != tick or nominal["target"] != nominal_tick:
            fail("Startup or nominal continuation has an invalid target", e)
        levels = dict(first["state"])
        levels["edge_flags"] = 0
        if first["state"] != e["state"] or nominal["state"] != levels:
            fail("Startup nominal continuation altered levels or repeated an edge", e)
        if e["after"] != binding["before"][:3] + [binding["action"]]:
            fail("Startup exact pair differs from frozen participant binding", e)
        start = resolved.get((who, gen, tick))
        follow = resolved.get((who, gen, nominal_tick))
        if follow and not follow["command"]:
            fail("Startup moved the missing-input gap to the next tick", follow)
        if start and start["command"] == e["command"] and start["boolean"]:
            remember(f"firstResolve{who}", start)
            if follow and follow["command"] == sequence and follow["boolean"] and not follow["state"]["edge_flags"]:
                complete[who] += 1
                remember(f"nominalResolve{who}", follow)
                if who == 0 and any(o == 0 and g < gen and room_tick == e["tick"] for (o, g), room_tick in room_generations.items()):
                    room_startups += 1
        elif start:
            fail("Accepted startup was not consumed at its declared tick", e)
    for key, e in accepted.items():
        if key not in startup_sequences and e["target"] != e["tick"] + 2:
            fail("Ordinary input changed its lead=2 target", e)
    for (who, gen, _), e in resolved.items():
        if e["command"] and (who, gen, e["command"]) not in accepted:
            fail("Resolved sequence lacks an acceptance in the same generation", e)

    checks = []

    def check(name, observed, keys=(), failed=False):
        checks.append({"name": name, "status": "FAIL" if failed else "PASS" if observed else "NOT_OBSERVED",
                       "evidenceFile": "capture/events.jsonl", "eventSequences": sorted({s for key in keys for s in evidence[key]})})

    check("single-registration-and-native-join", counts[2] == 1 and counts[11] == 1 and counts[12] == 1,
          failed=counts[2] > 1 or counts[11] > 1 or counts[12] > 1)
    for who in [0, 1]:
        check(f"participant-{who}-startup-and-nominal-resolve-without-gap", complete[who] > 0,
              [f"startup{who}", f"firstResolve{who}", f"nominalResolve{who}"])
        for path in [1, 2]:
            check(f"participant-{who}-path-{path}-move-and-shoot", all((who, path, family) in path_inputs for family in ["move", "shoot"]), [f"input-{who}-{path}"])
    check("A-room-rebind-starts-without-two-neutral-steps", room_startups > 0, ["room", "startup0", "firstResolve0", "nominalResolve0"])
    check("adjacent-physical-overlap-remains-nonzero", bool(evidence["overlap"]), ["overlap"])
    check("new-generation-explicit-edge-and-query", bool(evidence["explicitEdge"] and evidence["edgeQuery"]), ["explicitEdge", "edgeQuery"])
    check("missing-input-after-script-completion-neutral", bool(evidence["missingNeutral"]), ["scriptComplete", "missingNeutral"])
    check("duplicate-and-old-generation-rejected", bool(evidence["decision2"] and evidence["decision12"]), ["decision2", "decision12"])
    check("B-startup-after-rebind", len({gen for who, gen in started_generations if who == 1}) > 1, ["startup1", "firstResolve1", "nominalResolve1"])
    check("pause-resume-observed", bool(evidence["pauseResume"]), ["pauseResume"])
    visual_path = root / "manual-charge-observations.json"
    visual = load(visual_path) if visual_path.exists() else {}
    for field in ["chargeAcrossRoom", "chargeWhileAdjacentOverlap"]:
        check(field, visual.get(field) == "PASS", failed=visual.get(field) == "FAIL")
    targets = [t for t in restoration["targets"] if t["action"] != 4294967295]
    restored = restoration["nativeRestored"] and restoration["pinned"] and len(targets) == 48 and all(t["boolean"] and t["before"] == t["after"] for t in targets) and process["running"]
    check("menu-detached-resident-root-and-48-hooks-restored", bool(evidence["teardown"] and evidence["detached"]) and restored, ["teardown", "detached"], not restored)
    invariants = ["duplicateResolve", "duplicateWorldCommit", "duplicateConsumer", "duplicateAccept", "inconsistentQuery"]
    native_failure = raw.get("status") == "FAIL" or raw.get("nativeError") != 0 or raw.get("transportFailure") is not None or counts[16] or any(raw.get(k) is not False for k in invariants) or args.controller_exit not in [0, 1]
    check("native-transport-and-runtime-invariants", not native_failure, failed=bool(native_failure))
    check("startup-target-freshness-edge-and-query-invariants", not violations, failed=bool(violations))
    status = "FAIL" if violations or any(c["status"] == "FAIL" for c in checks) else "PASS" if all(c["status"] == "PASS" for c in checks) else "NOT_OBSERVED"
    result = {"tx": "tx-01-51", "status": status, "checks": checks, "violations": violations,
              "startupCount": len(startups), "completedStartupByOrdinal": dict(complete), "aRoomStartupCount": room_startups,
              "eventCount": sum(counts.values()), "maximumAuthorityTick": maximum_tick,
              "restoredSlotCount": len(targets), "nativeError": raw.get("nativeError"), "transportFailure": raw.get("transportFailure"),
              "rawFullFeatureStatus": raw.get("status"), "automaticRetry": False,
              "manualChargeObservations": visual,
              "chargeObservation": "Raw input/queries do not prove charge completion; retain separately labeled user visual observations",
              "fullFeature": "No claim for unobserved full T0-05 scenarios"}
    with (root / "result.json").open("x", encoding="utf-8") as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
        output.write("\n")
    print(json.dumps({"tx": "tx-01-51", "status": status, "checks": [{"name": c["name"], "status": c["status"]} for c in checks], "violations": violations}, ensure_ascii=False))
    return 0 if status == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
