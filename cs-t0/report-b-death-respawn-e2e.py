"""Report one completed death/respawn capture. Never attaches to the game."""
import argparse
import collections
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact-dir", type=Path, required=True)
    parser.add_argument("--controller-exit", type=int, required=True)
    parser.add_argument("--tx", default="tx-01-49")
    parser.add_argument("--expected-hook-count", type=int, default=47)
    parser.add_argument("--require-dual-path-input", action="store_true")
    args = parser.parse_args()
    root = args.artifact_dir.resolve()
    capture = root / "capture"
    load = lambda path: json.loads(path.read_text(encoding="utf-8-sig"))
    raw = load(capture / "result.json")
    recovery = load(capture / "restoration.json")
    process_after = load(root / "process-after.json")
    counts = collections.Counter()
    evidence = collections.defaultdict(list)
    violations = []
    baseline = death = respawn = rebound = registration = None
    dead = False
    old_sequence_high = last_accepted = 0
    fresh_accepted = {}
    dead_ticks = set()
    dead_commits = set()
    dead_a_ticks = set()
    post_a_ticks = set()
    max_tick = 0
    attributed_dead_queries = 0
    resolve_paths = {}
    bound_generations = {}
    input_paths = collections.Counter()
    held_world_paths = collections.Counter()
    commit_paths = collections.Counter()
    held_run = longest_held_run = 0
    last_held = None

    def remember(name, event):
        if len(evidence[name]) < 16:
            evidence[name].append(event["published"])

    def fail(reason, event):
        if len(violations) < 32:
            violations.append({"reason": reason, "eventSequence": event["published"]})

    def neutral(state):
        return all(state[key] == 0 for key in ["held_flags", "edge_flags", "move_x", "move_y", "aim_x", "aim_y"])

    with (capture / "events.jsonl").open(encoding="utf-8-sig") as source:
        for line in source:
            e = json.loads(line)
            kind, ordinal = e["kind"], e["ordinal"]
            counts[kind] += 1
            max_tick = max(max_tick, e["tick"])
            if args.require_dual_path_input:
                if kind == 3:
                    bound_generations[ordinal] = e["generation"]
                if kind == 13:
                    bound_generations.pop(ordinal, None)
                if kind == 5:
                    resolve_paths[(e["tick"], ordinal)] = e["detail"]
                    if ordinal == 0:
                        state = e["state"]
                        held = bool(e["boolean"] and e["command"] and state["held_flags"] & 1 and (state["aim_x"] or state["aim_y"]))
                        identity = (e["generation"], state["aim_x"], state["aim_y"])
                        held_run = held_run + 1 if held and last_held == (e["tick"] - 1, identity) else int(held)
                        last_held = (e["tick"], identity) if held else None
                        if held_run > longest_held_run:
                            longest_held_run = held_run
                            remember("continuousAInput", e)
                if kind == 8:
                    commit_paths[e["detail"]] += 1
                    remember(f"commitPath{e['detail']}", e)
                path = resolve_paths.get((e["tick"], ordinal), 0)
                if kind == 6 and e["decision"] & 1 and e["scalar"] != 0:
                    family = "move" if e["action"] < 4 else "shoot" if e["action"] < 8 else "other"
                    input_paths[(ordinal, path, family)] += 1
                    remember(f"input-{ordinal}-{path}-{family}", e)
                if kind == 7 and e["detail"] in [1, 5] and ordinal < 2:
                    if e["decision"] == 1 and e["action"] != path:
                        fail("Native consumer path does not match its Authority Resolve", e)
                    if e["detail"] == 5 and e["state"]["held_flags"] & 1:
                        cleared = e["after"][0] == 4294967295 or not any(e["after"][1:3])
                        if e["boolean"] and e["decision"] == 1:
                            if cleared:
                                fail("Eligible held aim was cleared by native Shooting", e)
                            else:
                                held_world_paths[(ordinal, path)] += 1
                                remember(f"held-world-{ordinal}-{path}", e)
                        special_fsm = e["query"] in [0x2FD4AC, 0x2FD54D]
                        if e["action"] == 0 and not special_fsm and bound_generations.get(ordinal) == e["generation"] and e["before"][0] != 4294967295 and cleared:
                            fail("An unscoped native Shooting cleared held aim after an eligible update", e)
                if kind == 20 and e["action"] == 0x2FD60C:
                    if e["detail"] & 3 == 3:
                        remember(f"oddNativeSource{ordinal}", e)
                    elif not e["detail"] & 28 and bound_generations.get(ordinal) == e["generation"] and not e["detail"] & 1:
                        fail("Ordinary native odd-frame Player Update still has no Authority window", e)
            if kind == 2 and e["detail"] == 5:
                registration = registration or e
                remember("registration", e)
            if kind == 11:
                remember("nativeCreate", e)
            if kind == 12:
                remember("joined", e)
            if kind == 4 and ordinal == 1 and e["decision"] == 1:
                last_accepted = max(last_accepted, e["command"])
            if kind == 3 and ordinal == 1 and death is None:
                baseline = e
                remember("baselineBinding", e)
            if kind == 13 and ordinal == 1 and e["detail"] == 6:
                if death is not None:
                    fail("More than one B death in the requested single-cycle scenario", e)
                death = death or e
                old_sequence_high = last_accepted
                dead = True
                remember("death", e)
                if baseline is not None and e["generation"] <= baseline["generation"]:
                    fail("Death did not invalidate B generation", e)
            if kind == 13 and ordinal == 1 and e["detail"] == 7:
                if not dead or death is None:
                    fail("B Respawn without an observed preceding B Death", e)
                respawn = respawn or e
                dead = False
                remember("respawn", e)
                if death is not None and e["generation"] <= death["generation"]:
                    fail("Respawn did not invalidate B generation", e)
            if dead:
                if kind == 5 and ordinal == 1:
                    dead_ticks.add(e["tick"])
                    remember("deadNeutralResolve", e)
                    if e["boolean"] or e["command"] or not neutral(e["state"]):
                        fail("B Resolve remained visible or retained input while dead", e)
                if kind == 4 and ordinal == 1 and e["decision"] == 1:
                    fail("B accepted input while dead", e)
                if kind == 3 and ordinal == 1:
                    fail("B rebound before observed Respawn", e)
                if kind == 6 and ordinal == 1:
                    attributed_dead_queries += 1
                    if e["boolean"] or e["scalar"] != 0:
                        fail("B query produced output while dead", e)
                if kind == 7 and ordinal == 1 and e["decision"] == 1:
                    fail("B consumer remained eligible while dead", e)
                if kind == 5 and ordinal == 0 and e["boolean"]:
                    dead_a_ticks.add(e["tick"])
                    remember("aContinuesDuringDeath", e)
                if kind == 8:
                    dead_commits.add(e["tick"])
            if respawn is not None:
                if kind == 3 and ordinal == 1:
                    rebound = rebound or e
                    remember("respawnBinding", e)
                    pair = registration["before"] if registration else None
                    if pair is not None and (e["action"] != pair[0] or e["before"][1:3] != pair[1:3]):
                        fail("Respawn binding does not match the registered owned K/device/control", e)
                    if e["before"][3] != 1 or e["after"][:3] != [2, 0, 1] or e["generation"] < respawn["generation"]:
                        fail("Respawn identity or generation is invalid", e)
                if kind == 4 and ordinal == 1:
                    if e["decision"] == 1:
                        fresh_accepted[e["command"]] = e
                        remember("freshAccepted", e)
                    elif e["decision"] == 12:
                        remember("wrongGenerationRejectedAfterRespawn", e)
                if ordinal == 1 and kind in [5, 6, 7]:
                    eligible = (kind == 5 and e["boolean"]) or (kind == 6 and e["decision"] & 1) or (kind == 7 and e["decision"] == 1)
                    if eligible and e["command"]:
                        if e["command"] <= old_sequence_high:
                            fail("A pre-death sequence was applied after Respawn", e)
                        fresh = fresh_accepted.get(e["command"])
                        if fresh is None or fresh["generation"] != e["generation"]:
                            fail("Eligible post-respawn input has no matching fresh-generation acceptance", e)
                        else:
                            if kind == 5:
                                remember("freshResolve", e)
                            if kind == 6 and (e["boolean"] or e["scalar"] != 0):
                                remember("freshQuery", e)
                            if kind == 7 and e["detail"] in [1, 5] and e["boolean"] and e["before"] != e["after"] and not neutral(e["state"]):
                                remember("freshWorldChange", e)
                if kind == 5 and ordinal == 0 and e["boolean"]:
                    post_a_ticks.add(e["tick"])
                    remember("aContinuesAfterRespawn", e)
            if kind == 13 and e["detail"] == 9:
                remember("teardown", e)
            if kind == 14 and ordinal == 1 and e["boolean"] and e["detail"] >= 1:
                remember("detachedWithResidentRoot", e)
            if kind == 15:
                remember("restoration", e)

    checks = []

    def check(name, observed, keys, failed=False):
        checks.append({"name": name, "status": "FAIL" if failed else "PASS" if observed else "NOT_OBSERVED",
                       "evidenceFile": "capture/events.jsonl", "eventSequences": sorted({sequence for key in keys for sequence in evidence[key]})})

    registrations = sum(1 for _ in evidence["registration"])
    single_create = counts[11] == 1 and counts[12] == 1 and registrations == 1
    check("single-device-native-create-and-binding", single_create and baseline is not None, ["registration", "nativeCreate", "joined", "baselineBinding"], counts[11] > 1 or counts[12] > 1 or registrations > 1)
    check("B-death-invalidates-generation", death is not None and baseline is not None and death["generation"] > baseline["generation"], ["baselineBinding", "death"])
    check("B-dead-resolve-neutral-at-least-three-ticks", len(dead_ticks) >= 3, ["death", "deadNeutralResolve"])
    check("A-and-authority-continue-during-B-death", len(dead_commits) >= 3 and bool(dead_a_ticks), ["aContinuesDuringDeath", "deadNeutralResolve"])
    check("B-respawn-invalidates-generation", death is not None and respawn is not None and respawn["generation"] > death["generation"], ["death", "respawn"])
    check("B-respawn-rebinds-frozen-identity-and-owned-pair", rebound is not None and registration is not None, ["registration", "respawn", "respawnBinding"])
    check("B-fresh-input-query-and-world-change-after-respawn", bool(evidence["freshResolve"] and evidence["freshQuery"] and evidence["freshWorldChange"]), ["freshAccepted", "freshResolve", "freshQuery", "freshWorldChange"])
    check("old-sequence-not-reapplied-and-old-generation-rejected", old_sequence_high > 0 and bool(evidence["freshResolve"] and evidence["wrongGenerationRejectedAfterRespawn"]), ["death", "freshResolve", "wrongGenerationRejectedAfterRespawn"])
    check("A-continues-after-B-respawn", bool(post_a_ticks), ["aContinuesAfterRespawn"])
    restored_slots = [e for e in recovery["targets"] if e["action"] != 4294967295]
    restored = recovery["nativeRestored"] and recovery["pinned"] and len(restored_slots) == args.expected_hook_count and all(e["boolean"] and e["before"] == e["after"] for e in restored_slots)
    check(f"menu-teardown-resident-root-and-all-{args.expected_hook_count}-hooks-restored", bool(evidence["teardown"] and evidence["detachedWithResidentRoot"]) and restored and process_after["running"], ["teardown", "detachedWithResidentRoot", "restoration"])
    if args.require_dual_path_input:
        check("both-native-simulation-paths-commit", commit_paths[1] > 0 and commit_paths[2] > 0, ["commitPath1", "commitPath2"])
        for who in [0, 1]:
            for path in [1, 2]:
                check(f"participant-{who}-path-{path}-movement-and-held-shooting", input_paths[(who, path, "move")] > 0 and input_paths[(who, path, "shoot")] > 0 and held_world_paths[(who, path)] > 0,
                      [f"input-{who}-{path}-move", f"input-{who}-{path}-shoot", f"held-world-{who}-{path}"])
            check(f"participant-{who}-odd-native-source-current-eligible", bool(evidence[f"oddNativeSource{who}"]), [f"oddNativeSource{who}"])
        check("A-held-aim-continuous-at-least-60-simulation-steps", longest_held_run >= 60, ["continuousAInput"])
        check("no-held-aim-neutral-interruption", not violations, [], bool(violations))
    invariant_names = ["duplicateResolve", "duplicateWorldCommit", "duplicateConsumer", "duplicateAccept", "inconsistentQuery"]
    raw_failure = raw.get("status") == "FAIL" or raw.get("nativeError") != 0 or raw.get("transportFailure") is not None or counts[16] > 0 or any(raw.get(key) is not False for key in invariant_names) or not restored or not process_after["running"] or args.controller_exit not in [0, 1]
    check("native-transport-and-runtime-invariants", not raw_failure, [], raw_failure)
    status = "FAIL" if violations or any(c["status"] == "FAIL" for c in checks) else "PASS" if all(c["status"] == "PASS" for c in checks) else "NOT_OBSERVED"
    result = {
        "tx": args.tx, "scope": "dual native input paths and B death/respawn" if args.require_dual_path_input else "B death/respawn focused E2E only", "status": status,
        "fullFeatureStatus": "NOT_OBSERVED; no claim for other missing full-feature scenarios",
        "checks": checks, "violations": violations, "eventCount": sum(counts.values()), "maximumAuthorityTick": max_tick,
        "deathTickCount": len(dead_ticks), "authorityCommitsDuringDeath": len(dead_commits), "aValidTicksDuringDeath": len(dead_a_ticks),
        "deathEvent": death, "respawnEvent": respawn, "respawnBindingEvent": rebound,
        "attributedBQueriesDuringDeath": attributed_dead_queries,
        "neutralEvidenceLimit": "Death neutrality is observed via ordinal=1 Resolve; unassigned TLS queries (ordinal UINT32_MAX) are not relabeled as B queries",
        "oldSequenceHighAtDeath": old_sequence_high, "rawControllerReport": "capture/result.json", "rawControllerStatus": raw.get("status"),
        "controllerExit": args.controller_exit, "processAfter": process_after, "restoredSlotCount": len(restored_slots),
        "nativeError": raw.get("nativeError"), "transportFailure": raw.get("transportFailure"),
        "invariants": {key: raw.get(key) for key in invariant_names}, "automaticRetry": False,
        "inputPathCounts": {str(key): value for key, value in input_paths.items()},
        "heldWorldPathCounts": {str(key): value for key, value in held_world_paths.items()},
        "commitPathCounts": dict(commit_paths), "longestAHeldAimRun": longest_held_run,
        "chargeEvidenceLimit": "Native aim/held consumers are observed; charge completion and beam emission require separately labeled visual observation",
    }
    with (root / "result.json").open("x", encoding="utf-8") as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
        output.write("\n")
    print(json.dumps({"tx": args.tx, "status": status, "checks": [{"name": c["name"], "status": c["status"]} for c in checks], "violations": violations}, ensure_ascii=False))
    return 0 if status == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
