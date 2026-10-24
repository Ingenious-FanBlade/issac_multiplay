"""Analyzer for one live host+VM T0-09-B P1/F0 artifact.

Reads only the completed artifact. It does not start games, inject, or retry.
A LIVE_F0_PASS is P1 F0 evidence only, not the full T0-09 matrix.
"""
from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
from report_t09_b import jsonl, load_json, parse_packet, wire_rows

TX = "tx-03-02"
ACCEPTED = 1
ACK_FIRST = 1
ACK_PROGRESS = 2
STATE_GAP = 2


def analyze(target: Path) -> dict[str, Any]:
    checks: list[dict[str, Any]] = []

    def check(name: str, passed: bool, detail: Any = None) -> None:
        checks.append({"name": name, "passed": bool(passed), "detail": detail})

    try:
        manifest = load_json(target / "manifest.json")
        orchestrator = load_json(target / "orchestrator-result.json")
        host_result = load_json(target / "host" / "result.json")
        vm_result = load_json(target / "vm" / "result.json")
        host_bridge = load_json(target / "host" / "bridge.json")
        vm_bridge = load_json(target / "vm" / "bridge.json")
        relay_ready = load_json(target / "relay" / "ready.json")
        relay_stopped = load_json(target / "relay" / "stopped.json")
        control_ready = load_json(target / "control" / "ready.json")
        vm_launch = load_json(target / "control" / "vm-launch.json")
        user_environment = load_json(target / "control" / "user-environment.json")
    except Exception as error:
        check("artifact.required-json", False, str(error))
        return finish(target, checks, fatal=str(error))

    check("manifest.magic-version", manifest.get("magic") == "T9M1" and manifest.get("version") == 1)
    check(
        "manifest.live-f0",
        manifest.get("case") == "F0"
        and manifest.get("repeat") == 1
        and manifest.get("user_preconditions", {}).get("real_game_authorized") is True
        and manifest.get("endpoints", {}).get("a", {}).get("os") == "host"
        and manifest.get("endpoints", {}).get("b", {}).get("os") == "vm",
        manifest.get("user_preconditions"),
    )
    script = manifest.get("script", {})
    last_tick_count = int(script.get("last_tick", 0))
    lead_ticks = int(script.get("lead_ticks", 0))
    check("manifest.10000-tick-F0", last_tick_count == 10000, {"last_tick": last_tick_count})
    check(
        "environment.frozen-baseline",
        user_environment.get("seed") == "PY1JDB20"
        and user_environment.get("room") == "initial-room"
        and user_environment.get("hostACharacter") == "isaac"
        and user_environment.get("remoteBCharacter") == "isaac"
        and user_environment.get("rdpSessionMustRemainConnected") is True
        and user_environment.get("automaticRetry") is False,
        {
            "seed": user_environment.get("seed"),
            "room": user_environment.get("room"),
            "hostACharacter": user_environment.get("hostACharacter"),
            "remoteBCharacter": user_environment.get("remoteBCharacter"),
            "rdp": user_environment.get("rdpSessionMustRemainConnected"),
        },
    )
    check("manifest.lead", 1 <= lead_ticks <= 256, {"lead_ticks": lead_ticks})
    wire = manifest.get("wire", {})
    session = str(wire.get("session", "")).lower()
    client_b = int(wire.get("clients", [0, 0])[1])

    check(
        "orchestrator.live-pass",
        orchestrator.get("status") == "PASS"
        and orchestrator.get("liveHostVmP1") is True
        and orchestrator.get("realGame") == "ATTEMPTED"
        and orchestrator.get("runStartSent") is True
        and orchestrator.get("stopSent") is True
        and orchestrator.get("targetReached") is True
        and orchestrator.get("allRetired") is True
        and orchestrator.get("rolesRetired") is True
        and orchestrator.get("automaticRetry") is False
        and orchestrator.get("error") is None,
        {
            "status": orchestrator.get("status"),
            "liveHostVmP1": orchestrator.get("liveHostVmP1"),
            "realGame": orchestrator.get("realGame"),
            "error": orchestrator.get("error"),
        },
    )
    check(
        "orchestrator.vm-controller",
        isinstance(orchestrator.get("vmController"), dict)
        and orchestrator["vmController"].get("status") == "PASS"
        and orchestrator["vmController"].get("role") == "B",
        orchestrator.get("vmController"),
    )

    check(
        "controller.A-live-result",
        host_result.get("status") == "PASS"
        and host_result.get("role") == "A"
        and host_result.get("readySent") is True
        and host_result.get("armedSent") is True
        and host_result.get("stopRequested") is True
        and host_result.get("driverExited") is True
        and host_result.get("gameProcessAlive") is True
        and host_result.get("endpointRetired") is True
        and host_result.get("ringOverwritten") == 0
        and host_result.get("error") is None,
        {
            "status": host_result.get("status"),
            "driverExited": host_result.get("driverExited"),
            "gameProcessAlive": host_result.get("gameProcessAlive"),
            "endpointRetired": host_result.get("endpointRetired"),
            "ringOverwritten": host_result.get("ringOverwritten"),
            "error": host_result.get("error"),
        },
    )
    check(
        "controller.B-live-result",
        vm_result.get("status") == "PASS"
        and vm_result.get("role") == "B"
        and vm_result.get("readySent") is True
        and vm_result.get("armedSent") is True
        and vm_result.get("stopRequested") is True
        and vm_result.get("driverExited") is True
        and vm_result.get("gameProcessAlive") is True
        and vm_result.get("dllResident") is True
        and vm_result.get("endpointRetired") is True
        and vm_result.get("ringOverwritten") == 0
        and vm_result.get("error") is None,
        {
            "status": vm_result.get("status"),
            "driverExited": vm_result.get("driverExited"),
            "gameProcessAlive": vm_result.get("gameProcessAlive"),
            "dllResident": vm_result.get("dllResident"),
            "endpointRetired": vm_result.get("endpointRetired"),
            "ringOverwritten": vm_result.get("ringOverwritten"),
            "error": vm_result.get("error"),
        },
    )
    restoration = host_result.get("authorityRestoration") or {}
    check(
        "controller.A-native-restoration",
        restoration.get("same_process") is True
        and restoration.get("dll_resident") is True
        and restoration.get("native_restored") is True
        and restoration.get("native_error") == 0
        and restoration.get("event_overwritten") == 0
        and restoration.get("restored_catalog") is True
        and int(restoration.get("restored_catalog_count", 0)) > 0
        and int(restoration.get("restored_rows", -1))
        == int(restoration.get("restored_catalog_count", 0))
        and restoration.get("stop_success") is True,
        restoration,
    )
    for role, bridge in (("A", host_bridge), ("B", vm_bridge)):
        check(
            f"bridge.{role}-clean",
            bridge.get("wireOverwritten") == [0, 0]
            and bridge.get("rejected") == 0
            and int(bridge.get("lateDroppedAfterClose", 0)) <= 32
            and int(bridge.get("outbound", 0)) > 0
            and int(bridge.get("inbound", 0)) > 0,
            bridge,
        )

    check(
        "relay.ready",
        relay_ready.get("a", {}).get("bind") == control_ready.get("relay", {}).get("a")
        and relay_ready.get("b", {}).get("bind") == control_ready.get("relay", {}).get("b"),
        {"relay": relay_ready, "control": control_ready},
    )
    check(
        "relay.stopped-cleanly",
        relay_stopped.get("status") == "STOPPED"
        and relay_stopped.get("stopRequested") is True
        and relay_stopped.get("rejected") == 0
        and relay_stopped.get("sendErrors") == 0
        and relay_stopped.get("dropped") == {"aToB": 0, "bToA": 0}
        and relay_stopped.get("duplicates") == {"aToB": 0, "bToA": 0},
        relay_stopped,
    )
    check(
        "control.vm-launch-shared",
        isinstance(vm_launch.get("scriptHost"), str)
        and isinstance(vm_launch.get("scriptGuest"), str)
        and vm_launch.get("automaticRetry") is False,
        {key: vm_launch.get(key) for key in ("scriptHost", "scriptGuest", "artifactHost")},
    )

    try:
        host_wire = wire_rows(target / "host")
        vm_wire = wire_rows(target / "vm")
    except Exception as error:
        check("wire.parse", False, str(error))
        return finish(target, checks, fatal=str(error))
    host_out = [row for row in host_wire if row.get("direction") == "outbound"]
    host_in = [row for row in host_wire if row.get("direction") == "inbound"]
    vm_out = [row for row in vm_wire if row.get("direction") == "outbound"]
    vm_in = [row for row in vm_wire if row.get("direction") == "inbound"]

    def identity_ok(rows: list[dict[str, Any]], expected_role: int) -> bool:
        return all(
            "packet" in row
            and row["packet"]["session"] == session
            and row["packet"]["role"] == expected_role
            for row in rows
        )

    check("wire.A-role-session", identity_ok(host_out, 1), {"rows": len(host_out)})
    check("wire.B-role-session", identity_ok(vm_out, 2), {"rows": len(vm_out)})
    check("wire.inbound-role-session", identity_ok(host_in, 2) and identity_ok(vm_in, 1))
    host_hello = [row["packet"] for row in host_out if row["packet"]["kind"] == 1]
    vm_hello = [row["packet"] for row in vm_out if row["packet"]["kind"] == 1]
    ack_packets = [row["packet"] for row in host_out if row["packet"]["kind"] == 3]
    state_packets = [row["packet"] for row in host_out if row["packet"]["kind"] == 4]
    first_acks = [packet for packet in ack_packets if packet["ack"]["purpose"] == ACK_FIRST]
    progress_acks = [packet for packet in ack_packets if packet["ack"]["purpose"] == ACK_PROGRESS]
    unexpected_a = [
        row["packet"]["kind"] for row in host_out if row["packet"]["kind"] not in (1, 3, 4)
    ]
    check(
        "wire.hello-and-first-ack",
        len(host_hello) == 1
        and len(vm_hello) == 1
        and len(first_acks) == 1
        and unexpected_a == [],
        {"hostHello": len(host_hello), "vmHello": len(vm_hello), "firstAck": len(first_acks)},
    )
    progress_ticks = [packet["ack"]["server_tick"] for packet in progress_acks]
    state_ticks = [packet["state"]["tick"] for packet in state_packets]
    contiguous = (
        len(progress_ticks) == last_tick_count
        and all(progress_ticks[index] == progress_ticks[0] + index for index in range(len(progress_ticks)))
    )
    check(
        "wire.10000-authority-acks",
        contiguous,
        {"count": len(progress_ticks), "first": progress_ticks[:1], "last": progress_ticks[-1:]},
    )
    check(
        "wire.10000-gap-states",
        state_ticks == progress_ticks
        and all(packet["state"]["status"] == STATE_GAP for packet in state_packets),
        {"count": len(state_ticks), "first": state_ticks[:1], "last": state_ticks[-1:]},
    )
    check(
        "wire.progress-monotonic",
        all(
            progress_acks[index]["ack"]["accepted"]
            >= (progress_acks[index - 1]["ack"]["accepted"] if index else 0)
            and progress_acks[index]["ack"]["processed"]
            >= (progress_acks[index - 1]["ack"]["processed"] if index else 0)
            for index in range(len(progress_acks))
        ),
    )

    input_records: list[dict[str, Any]] = []
    input_shape_ok = True
    vm_inputs = [row["packet"] for row in vm_out if row["packet"]["kind"] == 2]
    for packet in vm_inputs:
        value = packet.get("input", {})
        if value.get("count") != 1:
            input_shape_ok = False
            continue
        for record in value.get("records", []):
            input_records.append(record)
            if record["client"] != client_b or not record["sequence"] or not record["target_tick"]:
                input_shape_ok = False
    sequences = [record["sequence"] for record in input_records]
    targets = [record["target_tick"] for record in input_records]
    edge_inputs = [record for record in input_records if record["edge_flags"] & 0xF]
    check(
        "wire.B-input-identity",
        input_shape_ok
        and len(input_records) >= 9000
        and sequences == list(range(1, len(sequences) + 1))
        and targets == sorted(set(targets)),
        {
            "packets": len(vm_inputs),
            "records": len(input_records),
            "firstSequence": sequences[:1],
            "lastSequence": sequences[-1:],
            "firstTarget": targets[:1],
            "lastTarget": targets[-1:],
        },
    )
    check("wire.B-edge-exactly-one", len(edge_inputs) == 1, {"count": len(edge_inputs)})
    host_in_kinds = Counter(row["packet"]["kind"] for row in host_in if "packet" in row)
    vm_in_kinds = Counter(row["packet"]["kind"] for row in vm_in if "packet" in row)
    check(
        "wire.inbound-counts",
        host_in_kinds == Counter({1: 1, 2: len(vm_inputs)})
        and vm_in_kinds == Counter({1: 1, 3: len(ack_packets), 4: len(state_packets)}),
        {"host": dict(host_in_kinds), "vm": dict(vm_in_kinds), "inputs": len(vm_inputs)},
    )

    try:
        host_events = list(jsonl(target / "host" / "events.jsonl"))
    except Exception as error:
        check("host-events.parse", False, str(error))
        host_events = []
    admissions = [
        row
        for row in host_events
        if row.get("kind") == 4 and row.get("ordinal") == 1 and row.get("decision") == ACCEPTED
    ]
    admission_sequences = [int(row.get("command", 0)) for row in admissions]
    admission_targets = [int(row.get("target", 0)) for row in admissions]
    admission_edges = sum(
        int((row.get("state") or {}).get("edge_flags", 0)) != 0 for row in admissions
    )
    final_status = host_result.get("finalStatus", {})
    end_tick = int(final_status.get("targetTick", 0))
    run_start_tick = int(final_status.get("runStartTick", 0))
    expected_admitted = [
        record for record in input_records if int(record["target_tick"]) <= end_tick
    ]
    expected_sequences = [int(record["sequence"]) for record in expected_admitted]
    check(
        "input.b-end-at-authority",
        run_start_tick > 0
        and end_tick == run_start_tick + last_tick_count
        and progress_ticks
        and run_start_tick == progress_ticks[0] - 1
        and progress_ticks[-1] == end_tick,
        {
            "runStartTick": run_start_tick,
            "endTick": end_tick,
            "firstPublished": progress_ticks[:1],
            "lastPublished": progress_ticks[-1:],
        },
    )
    check(
        "input.authority-byte-admission",
        bool(expected_sequences)
        and admission_sequences == expected_sequences
        and admission_targets == [int(record["target_tick"]) for record in expected_admitted]
        and admission_edges == 1,
        {
            "admissions": len(admission_sequences),
            "expected": len(expected_sequences),
            "edges": admission_edges,
            "sequenceMismatch": next(
                (
                    (index, actual, expected)
                    for index, (actual, expected) in enumerate(
                        zip(admission_sequences, expected_sequences)
                    )
                    if actual != expected
                ),
                None,
            ),
        },
    )
    edge_progress = [
        row
        for row in host_events
        if row.get("kind") == 33
        and row.get("ordinal") == 1
        and row.get("query") == 7
    ]
    edge_processed_count = sum(
        int((row.get("before") or [0, 0, 0, 0])[2])
        | (int((row.get("before") or [0, 0, 0, 0])[3]) << 32)
        for row in edge_progress
    )
    check(
        "input.edge-processed-once",
        len(edge_progress) == 4 and edge_processed_count == 1,
        {"edgeQueryRows": len(edge_progress), "processedCount": edge_processed_count},
    )
    if progress_acks:
        final_ack = progress_acks[-1]["ack"]
        check(
            "input.final-progress-complete",
            final_ack["server_tick"] == end_tick
            and final_ack["accepted"] == len(expected_sequences)
            and final_ack["processed"] == len(expected_sequences),
            final_ack,
        )

    relay_receive: dict[str, Counter[str]] = {"A": Counter(), "B": Counter()}
    relay_forward = {"A_TO_B": 0, "B_TO_A": 0}
    relay_drop = 0
    relay_dup = 0
    try:
        for row in jsonl(target / "relay" / "relay.jsonl"):
            event = row.get("event")
            if event == "receive":
                relay_receive[str(row.get("face"))][str(row.get("raw", ""))] += 1
            elif event == "forward":
                direction = str(row.get("direction"))
                if direction in relay_forward:
                    relay_forward[direction] += 1
            elif event == "decision":
                decision = str(row.get("decision"))
                if "drop" in decision:
                    relay_drop += 1
                if decision == "duplicate":
                    relay_dup += 1
    except Exception as error:
        check("relay.parse", False, str(error))
    host_raw = Counter(row.get("raw", "") for row in host_out)
    vm_raw = Counter(row.get("raw", "") for row in vm_out)
    check(
        "relay.byte-preserving",
        relay_receive["A"] == host_raw and relay_receive["B"] == vm_raw,
        {
            "relayA": sum(relay_receive["A"].values()),
            "hostOut": sum(host_raw.values()),
            "relayB": sum(relay_receive["B"].values()),
            "vmOut": sum(vm_raw.values()),
        },
    )
    check("relay.f0-zero-impairment", relay_drop == 0 and relay_dup == 0, {"drop": relay_drop, "duplicate": relay_dup})
    check(
        "relay.forward-complete",
        relay_forward["A_TO_B"] == sum(host_raw.values())
        and relay_forward["B_TO_A"] == sum(vm_raw.values()),
        relay_forward,
    )

    try:
        control_rows = list(jsonl(target / "control" / "control.jsonl"))
    except Exception as error:
        control_rows = []
        check("control.parse", False, str(error))
    next_send = {"A": 1, "B": 1}
    next_receive = {"A": 1, "B": 1}
    last_server_ack = {"A": 0, "B": 0}
    last_client_ack = {"A": 0, "B": 0}
    armed = {"A": False, "B": False}
    run_start_ok = False
    stop_after_target = False
    target_seen = False
    stop_sent = {"A": False, "B": False}
    stop_complete = {"A": False, "B": False}
    retire_complete = {"A": False, "B": False}
    heartbeat_a: list[tuple[int, bool]] = []
    heartbeat_b: list[tuple[int, bool]] = []
    order_ok = True
    for row in control_rows:
        direction = row.get("direction")
        if direction == "closed":
            continue
        role = str(row.get("role"))
        kind = str(row.get("type"))
        sequence = int(row.get("seq", 0))
        ack = int(row.get("ack", 0))
        if role not in ("A", "B"):
            order_ok = False
            continue
        if direction == "send":
            if sequence != next_send[role] or ack < last_server_ack[role] or ack > next_receive[role] - 1:
                order_ok = False
            next_send[role] += 1
            last_server_ack[role] = ack
            if kind == "RUN_START":
                run_start_ok = armed["A"] and armed["B"]
            elif kind == "STOP_REQUEST":
                if target_seen:
                    stop_after_target = True
                stop_sent[role] = True
            elif kind == "TICK_HEARTBEAT" and role == "B":
                body = row.get("body", {})
                heartbeat_b.append(
                    (int(body.get("authority_tick", 0)), body.get("target_reached") is True)
                )
        elif direction == "receive":
            if sequence != next_receive[role] or ack < last_client_ack[role] or ack > next_send[role] - 1:
                order_ok = False
            next_receive[role] += 1
            last_client_ack[role] = ack
            if kind == "ARMED":
                armed[role] = True
            elif kind == "TICK_HEARTBEAT" and role == "A":
                body = row.get("body", {})
                heartbeat_a.append(
                    (int(body.get("authority_tick", 0)), body.get("target_reached") is True)
                )
                if body.get("target_reached") is True:
                    target_seen = True
            elif kind == "STOP_COMPLETE":
                stop_complete[role] = True
            elif kind == "RETIRE_COMPLETE":
                retire_complete[role] = True
        else:
            order_ok = False
    check("control.sequence-ack", order_ok)
    check("control.run-start-after-armed", run_start_ok and armed == {"A": True, "B": True}, armed)
    check("control.stop-after-target", stop_after_target and all(stop_sent.values()) and target_seen)
    check(
        "control.stop-retire-both",
        all(stop_complete.values()) and all(retire_complete.values()),
        {"stop": stop_complete, "retire": retire_complete},
    )
    final_heartbeat_ok = (
        bool(heartbeat_a)
        and bool(heartbeat_b)
        and [tick for tick, _ in heartbeat_a] == sorted(set(tick for tick, _ in heartbeat_a))
        and [tick for tick, _ in heartbeat_b] == sorted(set(tick for tick, _ in heartbeat_b))
        and heartbeat_a[-1][0] == end_tick
        and heartbeat_a[-1][1] is True
        and heartbeat_b[-1][0] == end_tick
        and heartbeat_b[-1][1] is True
    )
    check(
        "control.heartbeat-final",
        final_heartbeat_ok,
        {
            "aCount": len(heartbeat_a),
            "aLast": heartbeat_a[-1:] if heartbeat_a else [],
            "bCount": len(heartbeat_b),
            "bLast": heartbeat_b[-1:] if heartbeat_b else [],
            "endTick": end_tick,
        },
    )
    check(
        "process.identities",
        int(host_result.get("finalStatus", {}).get("heartbeatTick", 0)) == end_tick
        and host_result.get("gameProcessAlive") is True
        and vm_result.get("gameProcessAlive") is True
        and vm_result.get("dllResident") is True,
    )

    return finish(target, checks)


def finish(target: Path, checks: list[dict[str, Any]], fatal: str | None = None) -> dict[str, Any]:
    failures = [row for row in checks if not row["passed"]]
    passed = len(checks) - len(failures)
    status = "LIVE_F0_PASS" if not failures and fatal is None else "FAIL"
    result = {
        "tx": TX,
        "identity": "IsaacCsT0-T09B-LiveAnalyzer-v1",
        "scope": "live host+VM two-game P1 F0",
        "status": status,
        "liveF0Pass": status == "LIVE_F0_PASS",
        "t0_09_b": "P1_F0_PASS" if status == "LIVE_F0_PASS" else "IN_PROGRESS",
        "t0_09": "IN_PROGRESS",
        "checksPassed": passed,
        "checksTotal": len(checks),
        "failures": failures,
        "fatal": fatal,
        "automaticRetry": False,
        "hashes": "NOT_USED",
    }
    analyzer = target / "analyzer"
    analyzer.mkdir(parents=True, exist_ok=True)
    with (analyzer / "checks.jsonl").open("x", encoding="utf-8") as handle:
        for row in checks:
            handle.write(json.dumps(row, ensure_ascii=False) + "\n")
    (analyzer / "first-failure.json").open("x", encoding="utf-8").write(
        json.dumps(
            {
                "tx": TX,
                "status": "FAIL" if failures or fatal else "NONE",
                "firstFailure": failures[0] if failures else None,
                "fatal": fatal,
                "checksPassed": passed,
                "checksTotal": len(checks),
            },
            ensure_ascii=False,
            indent=2,
        )
        + "\n"
    )
    (analyzer / "result.json").open("x", encoding="utf-8").write(
        json.dumps(result, ensure_ascii=False, indent=2) + "\n"
    )
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("artifact_dir", type=Path)
    args = parser.parse_args()
    result = analyze(args.artifact_dir.resolve())
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result["status"] == "LIVE_F0_PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
