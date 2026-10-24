#!/usr/bin/env python3
"""tx-01-98 operation 9: one-shot real-Authority B source-stop orchestration.

Precondition: the user has already started one isolated isaac-ng.exe process
and left it in a normal active run with A present. This script starts the
controller, waits for the preloaded B script to finish and for its last target
to be effective, then sends the controller's normal stop command exactly once.
It never launches, injects, retries, or unloads the game.
"""
from __future__ import annotations

import argparse
import json
import shutil
import struct
import subprocess
import sys
import time
from pathlib import Path

TOOL_ROOT = Path(__file__).resolve().parent
REPO_ROOT = TOOL_ROOT.parent

SOURCE_FILES = [
    "cs-t0/build-authority-source-stop.ps1",
    "cs-t0/run-authority-source-stop-e2e.py",
    "cs-t0/report-authority-source-stop.py",
    "cs-t0/accept-authority-source-stop.ps1",
    "cs-t0/testdata/tx-01-98-source-stop-plan.json",
    "cs-t0/testdata/tx-01-98-source-stop-script.json",
    "cs-t0/testdata/tx-01-98-input-config.json",
    "docs/refactor/cs-t0/t0-07/tx-01-98-source-stop-e2e.md",
    "cs-t0/native/include/isaac_cs_t0/input_core.hpp",
    "cs-t0/native/src/input_core.cpp",
    "cs-t0/native/include/isaac_cs_t0/coop_input.hpp",
    "cs-t0/native/src/coop_input.cpp",
    "cs-t0/native/include/isaac_cs_t0/authority_tick.hpp",
    "cs-t0/native/src/authority_tick.cpp",
    "cs-t0/native/include/isaac_cs_t0/authority_tick_pipeline.hpp",
    "cs-t0/native/src/authority_tick_pipeline.cpp",
    "cs-t0/native/include/isaac_cs_t0/coop_active_ipc.hpp",
    "cs-t0/native/src/coop_active_runtime.cpp",
    "cs-t0/native/include/isaac_cs_t0/world_record.hpp",
    "cs-t0/native/src/world_record.cpp",
    "cs-t0/src/coop_active.rs",
    "cs-t0/src/world_record.rs",
]


EXPECTED_TIMESTAMP = 0x69E6E3A7
EXPECTED_SECTIONS = [
    (b".text\0\0\0", 0x00716134, 0x00001000, 0x00716200, 0x00000400, 0x60000020),
    (b".rdata\0\0", 0x000DF948, 0x00718000, 0x000DFA00, 0x00716600, 0x40000040),
    (b".data\0\0\0", 0x000A4AA4, 0x007F8000, 0x00069E00, 0x007F6000, 0xC0000040),
    (b".rsrc\0\0\0", 0x00003B20, 0x0089D000, 0x00003C00, 0x0085FE00, 0x40000040),
    (b".reloc\0\0", 0x0005C93C, 0x008A1000, 0x0005CA00, 0x00863A00, 0x42000040),
    (b".bind\0\0\0", 0x0002D808, 0x008FE000, 0x0002D808, 0x008C0400, 0x60000000),
]


def validate_game_pe(path: Path):
    """Read the on-disk PE identity expected by target_image.hpp.

    This is structural identity only; no file or image hash is computed.
    """
    raw = path.read_bytes()
    if len(raw) < 0x200 or raw[:2] != b"MZ":
        raise SystemExit("GameExe is not a DOS/PE image")
    pe = struct.unpack_from("<I", raw, 0x3C)[0]
    if pe + 24 + 0xF0 > len(raw) or raw[pe:pe + 4] != b"PE\0\0":
        raise SystemExit("GameExe has no valid PE header")
    machine, sections = struct.unpack_from("<HH", raw, pe + 4)
    timestamp = struct.unpack_from("<I", raw, pe + 8)[0]
    characteristics = struct.unpack_from("<H", raw, pe + 22)[0]
    size_optional = struct.unpack_from("<H", raw, pe + 20)[0]
    opt = pe + 24
    optional_magic = struct.unpack_from("<H", raw, opt)[0]
    entrypoint = struct.unpack_from("<I", raw, opt + 16)[0]
    image_base = struct.unpack_from("<I", raw, opt + 28)[0]
    section_alignment, file_alignment = struct.unpack_from("<II", raw, opt + 32)
    size_of_image = struct.unpack_from("<I", raw, opt + 56)[0]
    size_of_headers = struct.unpack_from("<I", raw, opt + 60)[0]
    dll_characteristics = struct.unpack_from("<H", raw, opt + 70)[0]
    if (machine != 0x014C or timestamp != EXPECTED_TIMESTAMP or characteristics != 0x0122 or
            optional_magic != 0x010B or image_base != 0x00400000 or
            section_alignment != 0x1000 or file_alignment != 0x0200 or
            size_of_headers != 0x0400 or dll_characteristics != 0x8140):
        raise SystemExit("GameExe PE header does not match the approved 32-bit target identity")
    if sections == 6 and size_of_image == 0x0092C000 and entrypoint == 0x008FE310:
        profile = "canonical"
        expected = EXPECTED_SECTIONS
    elif sections == 5 and size_of_image == 0x008FE000 and entrypoint == 0x006EFC46:
        profile = "approved-unpacked"
        expected = EXPECTED_SECTIONS[:5]
    else:
        raise SystemExit("GameExe PE section image/entrypoint is not an approved target profile")
    section_table = opt + size_optional
    for index, wanted in enumerate(expected):
        offset = section_table + index * 40
        if offset + 40 > len(raw):
            raise SystemExit("GameExe section table is truncated")
        name = raw[offset:offset + 8]
        virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", raw, offset + 8)
        section_characteristics = struct.unpack_from("<I", raw, offset + 36)[0]
        actual = (name, virtual_size, virtual_address, raw_size, raw_offset, section_characteristics)
        if actual != wanted:
            raise SystemExit("GameExe section identity differs at index " + str(index))
    return {
        "profile": profile,
        "timestamp_hex": f"0x{timestamp:08X}",
        "sections": sections,
        "size_of_image_hex": f"0x{size_of_image:X}",
        "entrypoint_hex": f"0x{entrypoint:X}",
        "file_size": len(raw),
    }


def load_json(path: Path):
    with path.open("r", encoding="utf-8") as stream:
        return json.load(stream)


def write_json(path: Path, value) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write("\n")


def copy_tree_file(relative: str, destination_root: Path) -> str:
    source = REPO_ROOT / relative
    if not source.is_file():
        return relative + " (missing)"
    destination = destination_root / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    return relative


class EventTail:
    def __init__(self, path: Path):
        self.path = path
        self.offset = 0
        self.pending = b""

    def poll(self):
        if not self.path.is_file():
            return []
        with self.path.open("rb") as stream:
            stream.seek(self.offset)
            chunk = stream.read()
            self.offset = stream.tell()
        self.pending += chunk
        rows = []
        while b"\n" in self.pending:
            raw, self.pending = self.pending.split(b"\n", 1)
            text = raw.strip()
            if not text:
                continue
            try:
                rows.append(json.loads(text.decode("utf-8")))
            except (UnicodeDecodeError, json.JSONDecodeError) as exc:
                rows.append({"__orchestrator_parse_error__": str(exc),
                             "raw_prefix": raw[:160].decode("utf-8", "replace")})
        return rows


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--game-exe", required=True)
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--artifact-dir", required=True)
    parser.add_argument("--authorization", required=True)
    parser.add_argument("--start-delay-seconds", type=float, default=0.0)
    args = parser.parse_args()

    artifact = Path(args.artifact_dir).resolve()
    build = Path(args.build_dir)
    if not build.is_absolute():
        build = (REPO_ROOT / build).resolve()
    game_exe = Path(args.game_exe).resolve()
    if artifact.exists():
        raise SystemExit("artifact directory must be new; no automatic retry")
    if not game_exe.is_file():
        raise SystemExit("game executable does not exist")
    if not args.authorization.strip():
        raise SystemExit("the actual operation authorization must be recorded")
    if not 0.0 <= args.start_delay_seconds <= 60.0:
        raise SystemExit("start delay must be in 0..60 seconds")
    game_identity = validate_game_pe(game_exe)
    controller = build / "Rust" / "IsaacCsT0.exe"
    native = build / "Native" / "bin" / "IsaacCsT0.Native.dll"
    helper = build / "Helper" / "bin" / "IsaacCsT0.Helper.exe"
    for path in (controller, native, helper):
        if not path.is_file():
            raise SystemExit("required build artifact is missing: " + str(path))

    plan_path = TOOL_ROOT / "testdata" / "tx-01-98-source-stop-plan.json"
    script_path = TOOL_ROOT / "testdata" / "tx-01-98-source-stop-script.json"
    config_path = TOOL_ROOT / "testdata" / "tx-01-98-input-config.json"
    plan = load_json(plan_path)
    script = load_json(script_path)
    config = load_json(config_path)
    if plan.get("tx") != "tx-01-98" or plan.get("operation") != 9:
        raise SystemExit("unexpected source-stop plan identity")
    if len(script.get("steps", [])) > 256:
        raise SystemExit("script exceeds the accepted 256-step source capacity")

    artifact.mkdir(parents=True)
    capture = artifact / "capture"
    write_json(artifact / "plan.json", plan)
    write_json(artifact / "script.json", script)
    write_json(artifact / "input-config.json", config)
    source_copy_results = []
    for relative in SOURCE_FILES:
        source_copy_results.append(copy_tree_file(relative, artifact / "sources"))
    build_copy = artifact / "build"
    build_copy.mkdir()
    shutil.copy2(controller, build_copy / "IsaacCsT0.exe")
    shutil.copy2(native, build_copy / "IsaacCsT0.Native.dll")
    shutil.copy2(helper, build_copy / "IsaacCsT0.Helper.exe")
    for name in ("build.json", "Rust-build.log", "Native-build.log", "Helper-build.log"):
        source = build / name
        if source.is_file():
            shutil.copy2(source, build_copy / name)

    game_stat = game_exe.stat()
    invocation = {
        "tx": "tx-01-98",
        "operation": 9,
        "authorization": args.authorization,
        "game_exe_name": game_exe.name,
        "game_exe_length": game_stat.st_size,
        "game_exe_mtime_ns": game_stat.st_mtime_ns,
        "game_pe_identity": game_identity,
        "build_directory": str(build),
        "controller": "build/IsaacCsT0.exe",
        "native": "build/IsaacCsT0.Native.dll",
        "helper": "build/IsaacCsT0.Helper.exe",
        "script": "script.json",
        "input_config": "input-config.json",
        "world_record": "min-read-v1",
        "b_character": "native",
        "controller_start_delay_seconds": args.start_delay_seconds,
        "recorded_precondition": "One user-started isolated isaac-ng.exe already in a normal active run with A present; no Online/Isaac Sync.",
        "automatic_retry": False,
        "hashes": "NOT_USED",
        "source_files": source_copy_results,
    }
    write_json(artifact / "invocation.json", invocation)

    controller_command = [
        str(controller),
        "coop-active",
        "--game-exe", str(game_exe),
        "--native-dll", str(native),
        "--helper", str(helper),
        "--artifact-dir", str(capture),
        "--script", str(script_path),
        "--authorization", args.authorization,
        "--input-config", str(config_path),
        "--world-record", "min-read-v1",
        "--b-character", "native",
    ]
    if args.start_delay_seconds > 0:
        # This exists only to let the external physical-input tool enter its
        # continuous A movement window before injection/Join and the B script.
        time.sleep(args.start_delay_seconds)
    started_at = time.monotonic()
    stdout_path = artifact / "controller.stdout.log"
    phase = "launch"
    joined_tick = None
    script_complete_tick = None
    max_b_target = 0
    max_b_availability_tick = 0
    effective_b_availability_ticks = set()
    last_committed_tick = 0
    failure_event = None
    retired_event = None
    parse_errors = []
    stop_sent = False
    stop_reason = None
    exit_code = None
    timed_out = False
    extra_signal = False
    process = None
    with stdout_path.open("wb") as stdout_stream:
        try:
            process = subprocess.Popen(
                controller_command,
                cwd=str(REPO_ROOT),
                stdin=subprocess.PIPE,
                stdout=stdout_stream,
                stderr=subprocess.STDOUT,
            )
            tail = EventTail(capture / "events.jsonl")
            deadline = started_at + 180.0
            while True:
                elapsed = time.monotonic() - started_at
                for event in tail.poll():
                    if "__orchestrator_parse_error__" in event:
                        parse_errors.append(event)
                        continue
                    kind = int(event.get("kind", 0))
                    ordinal = int(event.get("ordinal", 0xFFFFFFFF))
                    tick = int(event.get("tick", 0))
                    if kind == 12 and ordinal == 1 and joined_tick is None:
                        joined_tick = tick
                    elif kind == 18 and ordinal == 1 and script_complete_tick is None:
                        script_complete_tick = tick
                    elif kind == 4 and ordinal == 1 and int(event.get("decision", 0)) == 1:
                        max_b_target = max(max_b_target, int(event.get("target", 0)))
                    elif kind == 35 and ordinal == 1:
                        max_b_availability_tick = max(max_b_availability_tick, tick)
                        if int(event.get("detail", 0)) & 1 and int(event.get("boolean", 0)) == 1:
                            effective_b_availability_ticks.add(tick)
                    elif kind == 32 and int(event.get("decision", 0)) == 2:
                        before = event.get("before") or []
                        if len(before) == 4:
                            completed = int(before[0]) | (int(before[1]) << 32)
                            last_committed_tick = max(last_committed_tick, completed)
                    elif kind == 16:
                        failure_event = event
                    elif kind == 14 and ordinal == 1 and int(event.get("boolean", 0)) == 1:
                        retired_event = event
                if process.poll() is not None:
                    exit_code = process.returncode
                    phase = "controller_exited_before_stop"
                    break
                if failure_event is not None or parse_errors:
                    phase = "controller_failure_event" if failure_event is not None else "event_parse_error"
                    # Send the normal stop once even on a controlled failure so the
                    # resident Native cleanup path can run. Never retry the run.
                    try:
                        process.stdin.write(b"stop\n")
                        process.stdin.flush()
                        stop_sent = True
                        stop_reason = phase
                    except (OSError, ValueError):
                        pass
                    break
                if joined_tick is not None and script_complete_tick is not None and max_b_target > 0:
                    # Do not stop on a neutral/future-only row: the exact final
                    # scripted target must be effective. Require one additional
                    # committed boundary so cooperative retirement is observable
                    # strictly after the planned analysis window.
                    if max_b_target in effective_b_availability_ticks and last_committed_tick > max_b_target:
                        phase = "script_complete_and_final_target_effective"
                        process.stdin.write(b"stop\n")
                        process.stdin.flush()
                        stop_sent = True
                        stop_reason = phase
                        break
                if elapsed > 180.0:
                    timed_out = True
                    phase = "wait_deadline_exceeded"
                    if not stop_sent:
                        try:
                            process.stdin.write(b"stop\n")
                            process.stdin.flush()
                            stop_sent = True
                            stop_reason = phase
                        except (OSError, ValueError):
                            pass
                    break
                time.sleep(0.15)
            if process.poll() is None:
                shutdown_grace = 30 if timed_out else 180
                try:
                    exit_code = process.wait(timeout=shutdown_grace)
                except subprocess.TimeoutExpired:
                    timed_out = True
                    extra_signal = True
                    process.kill()
                    exit_code = process.wait(timeout=30)
            else:
                exit_code = process.returncode
        finally:
            if process is not None and process.poll() is None:
                extra_signal = True
                process.kill()
                process.wait(timeout=30)

    # Final size/close evidence and a compact machine summary.
    event_size = (capture / "events.jsonl").stat().st_size if (capture / "events.jsonl").is_file() else 0
    clean_stop = (phase == "script_complete_and_final_target_effective" and stop_sent and
                  not timed_out and not extra_signal and exit_code == 0 and
                  failure_event is None and not parse_errors)
    orchestrator_result = {
        "tx": "tx-01-98",
        "operation": 9,
        "status": "STOPPED" if clean_stop else "FAIL",
        "phase": phase,
        "controller_exit": exit_code,
        "controller_stop_sent": stop_sent,
        "stop_reason": stop_reason,
        "extra_signal_sent": extra_signal,
        "wait_timed_out": timed_out,
        "joined_tick": joined_tick,
        "script_complete_tick": script_complete_tick,
        "max_b_accepted_target": max_b_target,
        "max_b_availability_tick": max_b_availability_tick,
        "final_target_was_effective": max_b_target in effective_b_availability_ticks,
        "effective_b_availability_count": len(effective_b_availability_ticks),
        "last_committed_tick": last_committed_tick,
        "failure_event": failure_event,
        "b_retired_event": retired_event,
        "parse_errors": parse_errors[:8],
        "events_bytes_at_stop": event_size,
        "elapsed_seconds": round(time.monotonic() - started_at, 3),
        "automatic_retry": False,
        "hashes": "NOT_USED",
    }
    write_json(artifact / "orchestrator.json", orchestrator_result)

    report_script = TOOL_ROOT / "report-authority-source-stop.py"
    analysis = subprocess.run(
        [sys.executable, str(report_script), "--artifact-dir", str(artifact)],
        cwd=str(REPO_ROOT),
        capture_output=True,
        text=True,
        timeout=180,
        check=False,
    )
    (artifact / "report").mkdir(exist_ok=True)
    (artifact / "report" / "analysis.stdout.log").write_text(analysis.stdout, encoding="utf-8")
    (artifact / "report" / "analysis.stderr.log").write_text(analysis.stderr, encoding="utf-8")
    if analysis.returncode != 0 or orchestrator_result["status"] != "STOPPED":
        print(json.dumps({"status": "FAIL", "report_exit": analysis.returncode,
                          "orchestrator_status": orchestrator_result["status"]}, sort_keys=True))
        return 1
    print(json.dumps({"status": "PASS", "report_exit": 0,
                      "orchestrator_status": orchestrator_result["status"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
