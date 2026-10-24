"""ONE unified T0-09-B acceptance invocation.

This entry performs the only T0-09-B build and execution for this transaction:
pure compilation, one offline two-process F0 run, one artifact analysis. It
never starts a game, injects a DLL, touches Original Online/Isaac Sync, retries
a run, or computes hashes. A successful offline result is explicitly not
host+VM two-real-game P1 evidence.
"""
from __future__ import annotations

import argparse
import json
import os
import secrets
import shutil
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parent.parent
TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))

import report_t09_b
ARTIFACT_PARENT = ROOT / "artifacts" / "cs-t0"
WORK_PARENT = ROOT / "out" / "cs-t0"
MIN_FREE_BYTES = 512 * 1024 * 1024
CREATE_NO_WINDOW = 0x08000000

SOURCE_PATHS = [
    "AGENTS.md",
    "docs/refactor/project-conventions.md",
    "docs/refactor/cs-t0/t0-09",
    "cs-t0/Cargo.toml",
    "cs-t0/Cargo.lock",
    "cs-t0/src/lib.rs",
    "cs-t0/src/t09_main.rs",
    "cs-t0/src/t09",
    "cs-t0/native/IsaacCsT0.T09EndpointScenario.vcxproj",
    "cs-t0/native/include/isaac_cs_t0/minimal_protocol.hpp",
    "cs-t0/native/include/isaac_cs_t0/authority_session.hpp",
    "cs-t0/native/include/isaac_cs_t0/coop_input.hpp",
    "cs-t0/native/include/isaac_cs_t0/t09_wire_ipc.hpp",
    "cs-t0/native/src/minimal_protocol.cpp",
    "cs-t0/native/src/authority_session.cpp",
    "cs-t0/native/src/coop_input.cpp",
    "cs-t0/native/src/input_core.cpp",
    "cs-t0/native/src/t09_wire_ipc.cpp",
    "cs-t0/native/scenarios/t09_endpoint_main.cpp",
    "cs-t0/build-t09-b.ps1",
    "cs-t0/accept-t09-b.py",
    "cs-t0/report_t09_b.py",
]


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="milliseconds").replace("+00:00", "Z")


def write_new_json(path: Path, value: Any) -> None:
    with path.open("x", encoding="utf-8") as handle:
        json.dump(value, handle, ensure_ascii=False, indent=2)
        handle.write("\n")


def write_new_bytes(path: Path, value: bytes) -> None:
    with path.open("xb") as handle:
        handle.write(value)


def safe_remove_work(work: Path, target: Path) -> None:
    resolved = work.resolve()
    if resolved.parent != WORK_PARENT.resolve() or resolved.name != target.name + "-work":
        raise RuntimeError(f"refusing to remove unexpected work path: {resolved}")
    if resolved.exists():
        shutil.rmtree(resolved)


def copy_source(target: Path) -> None:
    source = target / "source"
    source.mkdir(parents=True, exist_ok=False)
    for relative in SOURCE_PATHS:
        origin = ROOT / relative
        if not origin.exists():
            raise RuntimeError(f"required source path is missing: {relative}")
        destination = source / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        if origin.is_dir():
            shutil.copytree(origin, destination)
        else:
            shutil.copy2(origin, destination)


def provenance(target: Path) -> dict[str, Any]:
    files = []
    for path in sorted(target.rglob("*")):
        if not path.is_file() or path.name in {"provenance.json", "acceptance-result.json"}:
            continue
        stat = path.stat()
        files.append(
            {
                "path": path.relative_to(target).as_posix(),
                "bytes": stat.st_size,
                "mtime_ns": stat.st_mtime_ns,
            }
        )
    return {
        "tx": "tx-03-02",
        "hashes": "NOT_USED",
        "identity": "IsaacCsT0-T09B-Acceptance-v1",
        "generatedUtc": utc_now(),
        "files": files,
    }


def make_nonces() -> dict[str, str]:
    values: list[str] = []
    while len(values) < 4:
        value = secrets.token_hex(16)
        if value != "0" * 32 and value not in values:
            values.append(value)
    return {"session": values[0], "run_nonce": values[1], "authority_nonce": values[2], "remote_nonce": values[3]}


def build_manifest(run_id: str, artifact_root: str) -> dict[str, Any]:
    nonces = make_nonces()
    return {
        "magic": "T9M1",
        "version": 1,
        "run_id": run_id,
        "case": "F0",
        "repeat": 1,
        "build_label": "t0-09-b-offline-build",
        "wire": {
            "version": 2,
            "session": nonces["session"],
            "run_nonce": nonces["run_nonce"],
            "authority_nonce": nonces["authority_nonce"],
            "remote_nonce": nonces["remote_nonce"],
            "clients": [101, 202],
        },
        "endpoints": {
            "a": {"os": "host", "role": "authority", "artifact_subdir": "host"},
            "b": {"os": "host", "role": "remote", "artifact_subdir": "vm"},
        },
        "script": {
            "id": "t09b-f0-minimal-v1",
            "first_tick": 0,
            "last_tick": 10000,
            "lead_ticks": 16,
            "tick_interval_ms": 4,
        },
        "relay": {
            "a": {"bind": "127.0.0.1:0"},
            "b": {"bind": "127.0.0.1:0"},
            "faults": {
                "version": 1,
                "seed": 131080,
                "base_delay_ms": 0,
                "jitter_ms": 0,
                "loss_ppm": 0,
                "duplicate_ppm": 0,
                "reorder_ppm": 0,
                "reorder_extra_ms": 20,
                "outage_after_ms": 0,
                "outage_ms": 0,
            },
        },
        "control": {"bind": "127.0.0.1:0"},
        "artifact_root": artifact_root,
        "user_preconditions": {
            "safe_run": True,
            "only_a": True,
            "no_original_online": True,
            "real_game_authorized": False,
        },
    }


def command_display(command: list[str]) -> dict[str, Any]:
    return {
        "executable": Path(command[0]).name,
        "argumentCount": len(command) - 1,
        "automaticRetry": False,
    }


def run_owned(command: list[str], timeout: float) -> tuple[int, bytes, bytes]:
    process = subprocess.Popen(
        command,
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        creationflags=CREATE_NO_WINDOW,
    )
    try:
        stdout, stderr = process.communicate(timeout=timeout)
        return process.returncode, stdout, stderr
    except subprocess.TimeoutExpired:
        # Every child in this tree is an owned offline relay/controller/scenario;
        # no game or resident DLL is part of this invocation.
        subprocess.run(
            ["taskkill", "/PID", str(process.pid), "/T", "/F"],
            capture_output=True,
            check=False,
            creationflags=CREATE_NO_WINDOW,
        )
        try:
            stdout, stderr = process.communicate(timeout=30)
        except subprocess.TimeoutExpired:
            process.kill()
            stdout, stderr = process.communicate()
        raise subprocess.TimeoutExpired(command, timeout, output=stdout, stderr=stderr)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--artifact-dir", type=Path, required=True)
    parser.add_argument("--authorization", required=True)
    args = parser.parse_args()

    target = args.artifact_dir.resolve()
    if target.parent != ARTIFACT_PARENT.resolve():
        raise SystemExit(f"artifact-dir must be a new direct child of {ARTIFACT_PARENT}")
    if not target.name or target.exists():
        raise SystemExit("artifact-dir must be new and nonempty")
    if not args.authorization.strip():
        raise SystemExit("--authorization must record the offline operation authorization")
    ARTIFACT_PARENT.mkdir(parents=True, exist_ok=True)
    WORK_PARENT.mkdir(parents=True, exist_ok=True)
    work = WORK_PARENT / (target.name + "-work")
    if work.exists():
        raise SystemExit(f"work directory already exists; no automatic retry: {work}")
    free = shutil.disk_usage(target.parent if target.parent.exists() else target.parent.parent).free
    if free < MIN_FREE_BYTES:
        raise SystemExit(f"insufficient free space: {free} < {MIN_FREE_BYTES}")

    work.mkdir(parents=False, exist_ok=False)
    run_id = f"tx-03-02-F0-offline-{datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')}"
    manifest_input = work / "manifest-input.json"
    manifest = build_manifest(run_id, target.relative_to(ROOT).as_posix())
    write_new_json(manifest_input, manifest)
    build_dir = work / "build"
    build_relative = build_dir.relative_to(ROOT).as_posix()
    build_command = [
        "pwsh",
        "-NoProfile",
        "-File",
        str(TOOLS / "build-t09-b.ps1"),
        "-OutputDirectory",
        build_relative,
        "-Transaction",
        "tx-03-02",
    ]
    build_stdout = b""
    build_stderr = b""
    build_exit: int | None = None
    f0_command: list[str] | None = None
    f0_stdout = b""
    f0_stderr = b""
    f0_exit: int | None = None
    errors: list[dict[str, Any]] = []
    try:
        build_exit, build_stdout, build_stderr = run_owned(build_command, 300)
        if build_exit != 0:
            raise RuntimeError(f"pure compilation failed with exit code {build_exit}")
        t09_exe = build_dir / "Rust" / "IsaacCsT0T09.exe"
        scenario = build_dir / "Scenario" / "bin" / "IsaacCsT0.T09EndpointScenario.exe"
        build_record = json.loads((build_dir / "build.json").read_text(encoding="utf-8"))
        if not t09_exe.is_file() or not scenario.is_file():
            raise RuntimeError("compiled T09 runner/scenario executable is missing")
        if build_record.get("executedPrograms") is not False or build_record.get("automaticRetry") is not False:
            raise RuntimeError("pure compilation record is inconsistent")
        f0_command = [
            str(t09_exe),
            "f0",
            "--manifest",
            str(manifest_input),
            "--artifact-dir",
            str(target),
            "--scenario",
            str(scenario),
        ]
        f0_exit, f0_stdout, f0_stderr = run_owned(f0_command, 240)
        if f0_exit != 0:
            raise RuntimeError(f"offline F0 runner failed with exit code {f0_exit}")
    except subprocess.TimeoutExpired as error:
        errors.append({"type": "TimeoutExpired", "message": str(error)})
        if error.output:
            if f0_command is None:
                build_stdout += error.output
            else:
                f0_stdout += error.output
        if error.stderr:
            if f0_command is None:
                build_stderr += error.stderr
            else:
                f0_stderr += error.stderr
    except BaseException as error:
        errors.append({"type": type(error).__name__, "message": str(error)})

    # Create-new final artifact root even when the run failed before it existed.
    if not target.exists():
        target.mkdir(parents=False, exist_ok=False)
    control_dir = target / "control"
    if not control_dir.exists():
        control_dir.mkdir(parents=True, exist_ok=False)
    try:
        copy_source(target)
    except BaseException as error:
        errors.append({"type": "SourceSnapshot", "message": str(error)})
    if build_dir.exists():
        try:
            shutil.copytree(build_dir, target / "build")
        except BaseException as error:
            errors.append({"type": "BuildSnapshot", "message": str(error)})
    try:
        write_new_bytes(control_dir / "build.stdout.log", build_stdout)
        write_new_bytes(control_dir / "build.stderr.log", build_stderr)
        write_new_bytes(control_dir / "accept.stdout.log", b"")
        write_new_bytes(control_dir / "accept.stderr.log", b"")
        if f0_command is not None:
            write_new_bytes(control_dir / "orchestrator.stdout.log", f0_stdout)
            write_new_bytes(control_dir / "orchestrator.stderr.log", f0_stderr)
        write_new_json(
            control_dir / "invocation-private.json",
            {
                "tx": "tx-03-02",
                "authorizationRecorded": True,
                "python": sys.version,
                "runId": run_id,
                "build": command_display(build_command),
                "f0": command_display(f0_command) if f0_command else None,
                "automaticRetry": False,
                "hashes": "NOT_USED",
            },
        )
        shutil.copy2(manifest_input, control_dir / "input-manifest.json")
    except BaseException as error:
        errors.append({"type": "InvocationArtifact", "message": str(error)})

    analyzer_result: dict[str, Any] | None = None
    try:
        analyzer_result = report_t09_b.analyze(target)
    except BaseException as error:
        errors.append({"type": "Analyzer", "message": str(error)})
    if analyzer_result and analyzer_result.get("status") != "OFFLINE_PASS":
        errors.append(
            {
                "type": "AnalyzerChecks",
                "message": f"{analyzer_result.get('checksPassed')}/{analyzer_result.get('checksTotal')} checks passed",
            }
        )
    try:
        provenance_record = provenance(target)
        write_new_json(target / "provenance.json", provenance_record)
    except BaseException as error:
        errors.append({"type": "Provenance", "message": str(error)})
    status = (
        "OFFLINE_PASS"
        if build_exit == 0
        and f0_exit == 0
        and not errors
        and analyzer_result is not None
        and analyzer_result.get("status") == "OFFLINE_PASS"
        else "FAIL"
    )
    if status == "OFFLINE_PASS" and not errors:
        try:
            safe_remove_work(work, target)
        except BaseException as error:
            errors.append({"type": "WorkCleanup", "message": str(error)})
            status = "FAIL"
    if work.exists():
        try:
            write_new_json(
                target / "work-retained.json",
                {
                    "reason": "acceptance did not complete cleanly; staging retained for diagnosis",
                    "workBasename": work.name,
                    "automaticRetry": False,
                },
            )
        except BaseException as error:
            errors.append({"type": "WorkRetentionRecord", "message": str(error)})
    result = {
        "tx": "tx-03-02",
        "identity": "IsaacCsT0-T09B-Acceptance-v1",
        "status": status,
        "offlineTransportFoundation": "PASS" if status == "OFFLINE_PASS" else "FAIL",
        "t0_09_b": "IN_PROGRESS",
        "realHostVmP1": "NOT_RUN",
        "realP1Note": "No real game/VM was started or injected. This offline runner does not satisfy the host+VM two-real-game P1 topology.",
        "runId": run_id,
        "buildExit": build_exit,
        "f0Exit": f0_exit,
        "errors": errors,
        "analyzer": analyzer_result,
        "automaticRetry": False,
        "hashes": "NOT_USED",
        "generatedUtc": utc_now(),
    }
    write_new_json(target / "acceptance-result.json", result)
    print(
        json.dumps(
            {
                "status": result["status"],
                "t0_09_b": result["t0_09_b"],
                "realHostVmP1": result["realHostVmP1"],
                "buildExit": build_exit,
                "f0Exit": f0_exit,
                "analyzer": {
                    "status": analyzer_result.get("status") if analyzer_result else None,
                    "passed": analyzer_result.get("checksPassed") if analyzer_result else None,
                    "total": analyzer_result.get("checksTotal") if analyzer_result else None,
                },
                "errors": errors,
            },
            ensure_ascii=False,
        )
    )
    return 0 if status == "OFFLINE_PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
