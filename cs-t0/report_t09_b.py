"""Static analyzer for one T0-09-B offline two-process F0 artifact.

This module never launches or modifies a game. It reads only the artifact created
by the single unified acceptance invocation and writes create-new analyzer files.
"""
from __future__ import annotations

import argparse
import json
import struct
from collections import Counter
from pathlib import Path
from typing import Any, Iterable

TX = "tx-03-02"
OFFLINE_SCOPE = "T0-09-B offline Wire IPC/UDP/control F0"
REAL_SCOPE = "host+VM two real isaac-ng.exe P1 NOT_RUN"
ACCEPTED = 1
ACK_PURPOSE_FIRST = 1
ACK_PURPOSE_PROGRESS = 2
STATE_STATUS_GAP = 2


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def jsonl(path: Path) -> Iterable[dict[str, Any]]:
    if not path.is_file():
        return
    with path.open("r", encoding="utf-8") as handle:
        for line_number, line in enumerate(handle, 1):
            text = line.strip()
            if not text:
                raise ValueError(f"{path}:{line_number}: empty JSONL line")
            value = json.loads(text)
            if not isinstance(value, dict):
                raise ValueError(f"{path}:{line_number}: JSONL row is not an object")
            yield value


def parse_packet(raw: bytes) -> dict[str, Any]:
    if not 64 <= len(raw) <= 1200:
        raise ValueError(f"packet length {len(raw)} outside 64..1200")
    if raw[:4] != b"ICT8":
        raise ValueError("packet magic is not ICT8")
    version, kind, header_size, role = struct.unpack_from("<HHHH", raw, 4)
    if version != 2 or header_size != 64:
        raise ValueError(f"wire version/header {version}/{header_size}")
    session = raw[16:32]
    sender, sequence = struct.unpack_from("<QQ", raw, 32)
    payload_size = struct.unpack_from("<I", raw, 48)[0]
    if payload_size != len(raw) - 64:
        raise ValueError("packet payload length mismatch")
    value: dict[str, Any] = {
        "kind": kind,
        "role": role,
        "session": session.hex(),
        "sender": sender,
        "sequence": sequence,
        "length": len(raw),
        "raw": raw,
    }
    if kind == 3:
        value["ack"] = {
            "server_tick": struct.unpack_from("<Q", raw, 64)[0],
            "client_id": struct.unpack_from("<Q", raw, 72)[0],
            "accepted": struct.unpack_from("<Q", raw, 80)[0],
            "processed": struct.unpack_from("<Q", raw, 88)[0],
            "processed_tick": struct.unpack_from("<Q", raw, 96)[0],
            "processed_source": struct.unpack_from("<Q", raw, 104)[0],
            "generation": struct.unpack_from("<Q", raw, 112)[0],
            "rejected_count": struct.unpack_from("<Q", raw, 120)[0],
            "purpose": struct.unpack_from("<H", raw, 134)[0],
        }
    elif kind == 4:
        value["state"] = {
            "tick": struct.unpack_from("<Q", raw, 96)[0],
            "run": struct.unpack_from("<Q", raw, 104)[0],
            "room": struct.unpack_from("<Q", raw, 112)[0],
            "attempt": struct.unpack_from("<Q", raw, 120)[0],
            "capture": struct.unpack_from("<Q", raw, 128)[0],
            "status": struct.unpack_from("<H", raw, 140)[0],
            "reason": struct.unpack_from("<H", raw, 142)[0],
            "count": struct.unpack_from("<H", raw, 144)[0],
        }
    elif kind == 2:
        count = struct.unpack_from("<H", raw, 64)[0]
        last_ack = struct.unpack_from("<Q", raw, 72)[0]
        records = []
        for index in range(count):
            record = 80 + index * 64
            command = {
                "client": struct.unpack_from("<Q", raw, record + 16)[0],
                "sequence": struct.unpack_from("<Q", raw, record + 24)[0],
                "target_tick": struct.unpack_from("<Q", raw, record + 32)[0],
                "edge_flags": struct.unpack_from("<I", raw, record + 48)[0],
            }
            records.append(command)
        value["input"] = {"count": count, "last_ack": last_ack, "records": records}
    return value


def wire_rows(role_dir: Path) -> list[dict[str, Any]]:
    rows = []
    for row in jsonl(role_dir / "wire.jsonl"):
        copied = dict(row)
        if "raw" in row:
            copied["packet"] = parse_packet(bytes.fromhex(row["raw"]))
        rows.append(copied)
    return rows


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
    except Exception as error:
        check("artifact.required-json", False, str(error))
        return finish(target, checks, fatal=str(error))

    check("manifest.magic-version", manifest.get("magic") == "T9M1" and manifest.get("version") == 1)
    check("manifest.case-F0", manifest.get("case") == "F0" and manifest.get("repeat") == 1)
    check(
        "manifest.offline-only",
        manifest.get("user_preconditions", {}).get("real_game_authorized") is False,
        manifest.get("user_preconditions"),
    )
    wire = manifest.get("wire", {})
    script = manifest.get("script", {})
    last_tick = int(script.get("last_tick", 0))
    lead_ticks = int(script.get("lead_ticks", 0))
    session = str(wire.get("session", "")).lower()
    client_b = int(wire.get("clients", [0, 0])[1])
    check("manifest.10000-tick-F0", last_tick == 10000, {"last_tick": last_tick})
    check("manifest.lead", 1 <= lead_ticks <= 256, {"lead_ticks": lead_ticks})

    check(
        "orchestrator.offline-pass",
        orchestrator.get("status") == "PASS"
        and orchestrator.get("runStartSent") is True
        and orchestrator.get("stopSent") is True
        and orchestrator.get("targetReached") is True
        and orchestrator.get("allRetired") is True
        and orchestrator.get("rolesRetired") is True
        and orchestrator.get("realGame") == "NOT_RUN"
        and orchestrator.get("automaticRetry") is False
        and orchestrator.get("error") is None,
        {
            "status": orchestrator.get("status"),
            "runStartSent": orchestrator.get("runStartSent"),
            "stopSent": orchestrator.get("stopSent"),
            "targetReached": orchestrator.get("targetReached"),
            "allRetired": orchestrator.get("allRetired"),
            "rolesRetired": orchestrator.get("rolesRetired"),
            "error": orchestrator.get("error"),
        },
    )

    for role, result, bridge in (("A", host_result, host_bridge), ("B", vm_result, vm_bridge)):
        check(
            f"{role}.endpoint-result",
            result.get("status") == "PASS"
            and result.get("readySent") is True
            and result.get("armedSent") is True
            and result.get("stopRequested") is True
            and result.get("driverExited") is True
            and result.get("endpointRetired") is True
            and result.get("ringOverwritten") == 0
            and result.get("error") is None,
            {
                "status": result.get("status"),
                "readySent": result.get("readySent"),
                "armedSent": result.get("armedSent"),
                "stopRequested": result.get("stopRequested"),
                "driverExited": result.get("driverExited"),
                "endpointRetired": result.get("endpointRetired"),
                "ringOverwritten": result.get("ringOverwritten"),
                "error": result.get("error"),
            },
        )
        overwritten = result.get("finalStatus", {}).get("overwritten", [])
        check(
            f"{role}.wire-no-overwrite",
            overwritten == [0, 0]
            and bridge.get("wireOverwritten") == [0, 0]
            and bridge.get("rejected") == 0
            and int(bridge.get("outbound", 0)) > 0
            and int(bridge.get("inbound", 0)) > 0,
            {"finalOverwritten": overwritten, "bridge": bridge},
        )

    check(
        "relay.ready",
        relay_ready.get("a", {}).get("bind") == control_ready.get("relay", {}).get("a")
        and relay_ready.get("b", {}).get("bind") == control_ready.get("relay", {}).get("b"),
        {"relay_ready": relay_ready, "control_ready": control_ready},
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

    # Parse the role-perspective wire logs. A writes ACK/STATE and reads INPUT;
    # B writes INPUT and reads ACK/STATE.
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

    check("wire.A-outbound-role-session", identity_ok(host_out, 1), {"rows": len(host_out)})
    check("wire.B-outbound-role-session", identity_ok(vm_out, 2), {"rows": len(vm_out)})
    check("wire.inbound-role-session", identity_ok(host_in, 2) and identity_ok(vm_in, 1))

    ack_packets = [row["packet"] for row in host_out if row["packet"]["kind"] == 3]
    state_packets = [row["packet"] for row in host_out if row["packet"]["kind"] == 4]
    unexpected_a_kinds = [row["packet"]["kind"] for row in host_out if row["packet"]["kind"] not in (1, 3, 4)]
    host_hellos = [row["packet"] for row in host_out if row["packet"]["kind"] == 1]
    vm_hellos = [row["packet"] for row in vm_out if row["packet"]["kind"] == 1]
    first_acks = [packet for packet in ack_packets if packet["ack"]["purpose"] == ACK_PURPOSE_FIRST]
    progress_acks = [packet for packet in ack_packets if packet["ack"]["purpose"] == ACK_PURPOSE_PROGRESS]
    progress_ticks = [packet["ack"]["server_tick"] for packet in progress_acks]
    state_ticks = [packet["state"]["tick"] for packet in state_packets]
    check("wire.first-ack-exactly-one", len(first_acks) == 1, {"count": len(first_acks)})
    check(
        "wire.authority-progress-ack-continuous",
        progress_ticks == list(range(1, last_tick + 1)),
        {"count": len(progress_ticks), "first": progress_ticks[:1], "last": progress_ticks[-1:]},
    )
    check(
        "wire.authority-state-continuous-gap",
        state_ticks == list(range(1, last_tick + 1))
        and all(packet["state"]["status"] == STATE_STATUS_GAP for packet in state_packets),
        {"count": len(state_ticks), "first": state_ticks[:1], "last": state_ticks[-1:]},
    )
    check(
        "wire.one-hello-each",
        len(host_hellos) == 1 and len(vm_hellos) == 1 and unexpected_a_kinds == [],
        {"hostHellos": len(host_hellos), "vmHellos": len(vm_hellos), "unexpectedA": unexpected_a_kinds[:20]},
    )
    check(
        "wire.authority-progress-monotonic",
        all(
            progress_acks[index]["ack"]["server_tick"] == index + 1
            and progress_acks[index]["ack"]["accepted"] >= (progress_acks[index - 1]["ack"]["accepted"] if index else 0)
            and progress_acks[index]["ack"]["processed"] >= (progress_acks[index - 1]["ack"]["processed"] if index else 0)
            for index in range(len(progress_acks))
        ),
    )

    vm_inputs = [row["packet"] for row in vm_out if row["packet"]["kind"] == 2]
    input_records: list[dict[str, Any]] = []
    input_shape_ok = True
    for packet in vm_inputs:
        input_value = packet.get("input", {})
        if input_value.get("count") != 1:
            input_shape_ok = False
            continue
        for record in input_value.get("records", []):
            input_records.append(record)
            if record["client"] != client_b or not record["sequence"] or not record["target_tick"]:
                input_shape_ok = False
    sequences = [record["sequence"] for record in input_records]
    targets = [record["target_tick"] for record in input_records]
    edge_inputs = [record for record in input_records if record["edge_flags"] & 0xF]
    check(
        "wire.B-input-shape",
        input_shape_ok
        and len(input_records) >= 9000
        and sequences == list(range(1, len(sequences) + 1))
        and targets == sorted(set(targets))
        and all(0 < target <= last_tick for target in targets),
        {
            "packets": len(vm_inputs),
            "records": len(input_records),
            "sequenceFirstLast": [sequences[:1], sequences[-1:]],
            "targetFirstLast": [targets[:1], targets[-1:]],
        },
    )
    check("wire.B-edge-exactly-one", len(edge_inputs) == 1, {"count": len(edge_inputs)})
    host_in_kinds = Counter(row["packet"]["kind"] for row in host_in if "packet" in row)
    vm_in_kinds = Counter(row["packet"]["kind"] for row in vm_in if "packet" in row)
    check(
        "wire.inbound-packet-counts",
        host_in_kinds == Counter({1: 1, 2: len(vm_inputs)})
        and vm_in_kinds == Counter({1: 1, 3: len(ack_packets), 4: len(state_packets)}),
        {"hostIn": dict(host_in_kinds), "vmIn": dict(vm_in_kinds), "BOutbound": len(vm_inputs), "AOutbound": len(host_out)},
    )

    if progress_acks:
        final_ack = progress_acks[-1]["ack"]
        check(
            "wire.final-authority-progress",
            final_ack["server_tick"] == last_tick
            and final_ack["accepted"] == (sequences[-1] if sequences else 0)
            and final_ack["processed"] == (sequences[-1] if sequences else 0)
            and final_ack["processed_tick"] <= last_tick,
            final_ack,
        )

    # Relay raw packet identity: F0 must preserve every byte and deliver each
    # endpoint outbound packet exactly once.
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
    host_out_raw = Counter(row.get("raw", "") for row in host_out)
    vm_out_raw = Counter(row.get("raw", "") for row in vm_out)
    check(
        "relay.byte-preserving-receive",
        relay_receive["A"] == host_out_raw and relay_receive["B"] == vm_out_raw,
        {
            "relayA": sum(relay_receive["A"].values()),
            "hostOut": sum(host_out_raw.values()),
            "relayB": sum(relay_receive["B"].values()),
            "vmOut": sum(vm_out_raw.values()),
        },
    )
    check(
        "relay.f0-no-drop-duplicate",
        relay_drop == 0 and relay_dup == 0,
        {"drop": relay_drop, "duplicate": relay_dup},
    )
    check(
        "relay.forward-counts",
        relay_forward["A_TO_B"] == sum(host_out_raw.values())
        and relay_forward["B_TO_A"] == sum(vm_out_raw.values()),
        relay_forward,
    )

    # Scenario event logs prove the production Endpoint/admission path actually
    # produced and consumed the records represented by the raw wire packets.
    try:
        host_events = list(jsonl(target / "host" / "driver-events.jsonl"))
        vm_events = list(jsonl(target / "vm" / "driver-events.jsonl"))
    except Exception as error:
        check("driver-events.parse", False, str(error))
        host_events, vm_events = [], []
    ack_sent = [row for row in host_events if row.get("name") == "ack-sent"]
    state_sent = [row for row in host_events if row.get("name") == "state-sent"]
    admissions = [row for row in host_events if row.get("name") == "input-admission"]
    target_reached = [row for row in host_events if row.get("name") == "target-reached"]
    input_produced = [row for row in vm_events if row.get("name") == "input-produced"]
    ack_received = [row for row in vm_events if row.get("name") == "ack-received"]
    check(
        "driver.A-authority-events",
        len(ack_sent) == last_tick
        and len(state_sent) == last_tick
        and [int(row["tick"]) for row in ack_sent] == list(range(1, last_tick + 1))
        and [int(row["tick"]) for row in state_sent] == list(range(1, last_tick + 1)),
        {"ackSent": len(ack_sent), "stateSent": len(state_sent)},
    )
    admitted_sequences = [int(row.get("sequence", 0)) for row in admissions]
    admitted_decisions = [int(row.get("decision", 0)) for row in admissions]
    admitted_edges = sum(int(row.get("edgeFlags", 0)) != 0 for row in admissions)
    check(
        "driver.A-all-input-admitted",
        len(admissions) == len(input_records)
        and admitted_sequences == sequences
        and all(decision == ACCEPTED for decision in admitted_decisions)
        and admitted_edges == 1,
        {
            "admissions": len(admissions),
            "inputRecords": len(input_records),
            "decisions": Counter(admitted_decisions),
            "admittedEdges": admitted_edges,
        },
    )
    check(
        "driver.A-edge-exactly-once",
        len(target_reached) == 1
        and int(target_reached[0].get("edgesProcessed", -1)) == 1
        and int(target_reached[0].get("accepted", -1)) == len(admissions),
        target_reached,
    )
    check(
        "driver.B-control-heartbeat-to-input",
        len(input_produced) == len(input_records)
        and all(
            int(row.get("targetTick", 0)) == int(row.get("tick", 0)) + lead_ticks
            and int(row.get("controllerHeartbeat", 0)) == int(row.get("tick", 0))
            for row in input_produced
        ),
        {"inputProduced": len(input_produced), "leadTicks": lead_ticks},
    )
    check(
        "driver.B-input-plus-ack-observation",
        len(input_produced) == len(input_records)
        and len(ack_received) == len(ack_packets)
        and [int(row.get("sequence", 0)) for row in input_produced] == sequences,
        {"inputProduced": len(input_produced), "ackReceived": len(ack_received), "ackPackets": len(ack_packets)},
    )
    host_retire = [row for row in host_events if row.get("name") == "endpoint-retired"]
    vm_retire = [row for row in vm_events if row.get("name") == "endpoint-retired"]
    check(
        "driver.A-normal-retire",
        len(host_retire) == 1
        and sum(row.get("name") == "driver-stopped" for row in host_events) == 1
        and int(host_retire[0].get("lastTick", -1)) == last_tick
        and int(host_retire[0].get("accepted", -1)) == len(admissions),
        host_retire,
    )
    check(
        "driver.B-normal-retire",
        len(vm_retire) == 1
        and sum(row.get("name") == "driver-stopped" for row in vm_events) == 1
        and int(vm_retire[0].get("acks", -1)) == last_tick
        and int(vm_retire[0].get("states", -1)) == last_tick
        and int(vm_retire[0].get("controllerHeartbeat", -1)) == last_tick,
        vm_retire,
    )

    # T9C1 ordering and per-direction sequence/ack validation.
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
    run_start_after_armed = False
    stop_after_target = False
    run_start_sent = False
    target_seen = False
    stop_sent = {"A": False, "B": False}
    stop_complete = {"A": False, "B": False}
    retire_complete = {"A": False, "B": False}
    heartbeat_ticks: list[int] = []
    forwarded_heartbeats: list[int] = []
    control_order_ok = True
    for row in control_rows:
        direction = row.get("direction")
        if direction == "closed":
            # Connection closure after RETIRE_COMPLETE is an audit row, not a
            # sequenced T9C1 envelope.
            continue
        role = str(row.get("role"))
        kind = str(row.get("type"))
        sequence = int(row.get("seq", 0))
        ack = int(row.get("ack", 0))
        if role not in ("A", "B"):
            control_order_ok = False
            continue
        if direction == "send":
            if sequence != next_send[role] or ack < last_server_ack[role] or ack > next_receive[role] - 1:
                control_order_ok = False
            next_send[role] += 1
            last_server_ack[role] = ack
            if kind == "RUN_START":
                if not (armed["A"] and armed["B"]):
                    run_start_after_armed = False
                else:
                    run_start_after_armed = True
                run_start_sent = True
            if kind == "STOP_REQUEST":
                if run_start_sent and target_seen:
                    stop_after_target = True
                stop_sent[role] = True
            if kind == "TICK_HEARTBEAT" and role == "B":
                forwarded_heartbeats.append(int(row.get("body", {}).get("authority_tick", 0)))
        elif direction == "receive":
            if sequence != next_receive[role] or ack < last_client_ack[role] or ack > next_send[role] - 1:
                control_order_ok = False
            next_receive[role] += 1
            last_client_ack[role] = ack
            if kind == "ARMED":
                armed[role] = True
            elif kind == "TICK_HEARTBEAT" and role == "A":
                body = row.get("body", {})
                tick = int(body.get("authority_tick", 0))
                heartbeat_ticks.append(tick)
                if body.get("target_reached") is True:
                    target_seen = True
            elif kind == "STOP_COMPLETE":
                stop_complete[role] = True
            elif kind == "RETIRE_COMPLETE":
                retire_complete[role] = True
        else:
            control_order_ok = False
    check("control.sequence-ack", control_order_ok)
    check("control.run-start-after-both-armed", run_start_after_armed and armed == {"A": True, "B": True}, armed)
    check("control.stop-after-target", stop_after_target and all(stop_sent.values()) and target_seen)
    check(
        "control.stop-retire-both",
        all(stop_complete.values()) and all(retire_complete.values()),
        {"stop": stop_complete, "retire": retire_complete},
    )
    check(
        "control.heartbeat-monotonic",
        bool(heartbeat_ticks)
        and heartbeat_ticks == sorted(set(heartbeat_ticks))
        and heartbeat_ticks[-1] == last_tick
        and target_seen,
        {"count": len(heartbeat_ticks), "first": heartbeat_ticks[:1], "last": heartbeat_ticks[-1:]},
    )
    check(
        "control.heartbeat-forwarded-to-B",
        bool(forwarded_heartbeats)
        and forwarded_heartbeats == sorted(set(forwarded_heartbeats))
        and forwarded_heartbeats[-1] == last_tick,
        {"count": len(forwarded_heartbeats), "first": forwarded_heartbeats[:1], "last": forwarded_heartbeats[-1:]},
    )
    check(
        "control.artifact-ready",
        control_ready.get("runId") == manifest.get("run_id")
        and control_ready.get("automaticRetry") is False,
    )

    return finish(target, checks)


def finish(target: Path, checks: list[dict[str, Any]], fatal: str | None = None) -> dict[str, Any]:
    passed = sum(1 for row in checks if row["passed"])
    total = len(checks)
    failures = [row for row in checks if not row["passed"]]
    status = "OFFLINE_PASS" if not failures and fatal is None else "FAIL"
    result = {
        "tx": TX,
        "identity": "IsaacCsT0-T09B-Analyzer-v1",
        "scope": OFFLINE_SCOPE,
        "status": status,
        "offlinePass": status == "OFFLINE_PASS",
        "realHostVmP1": "NOT_RUN",
        "realP1Note": "Offline transport/pass is not host+VM two-real-game P1 evidence.",
        "checksPassed": passed,
        "checksTotal": total,
        "failures": failures,
        "fatal": fatal,
        "automaticRetry": False,
        "hashes": "NOT_USED",
    }
    analyzer = target / "analyzer"
    if not analyzer.is_dir():
        analyzer.mkdir(parents=True, exist_ok=True)
    with (analyzer / "checks.jsonl").open("x", encoding="utf-8") as handle:
        for row in checks:
            handle.write(json.dumps(row, ensure_ascii=False) + "\n")
    first_failure = {
        "tx": TX,
        "status": "FAIL" if failures or fatal else "NONE",
        "firstFailure": failures[0] if failures else None,
        "fatal": fatal,
        "checksPassed": passed,
        "checksTotal": total,
    }
    (analyzer / "first-failure.json").write_text(
        json.dumps(first_failure, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    (analyzer / "result.json").write_text(
        json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("artifact_dir", type=Path)
    args = parser.parse_args()
    result = analyze(args.artifact_dir.resolve())
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result["status"] == "OFFLINE_PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
