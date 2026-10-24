"""ONE unified live T0-09-B P1/F0 acceptance invocation.

The user must have both real games running at the agreed offline baseline and
keep the same RDP session connected. This script builds once, starts the host
orchestrator once, waits for the generated VM start script to become available,
and then waits for the user to run that script on the VM. It never retries,
never starts/terminates a game, and never injects directly.
"""
from __future__ import annotations

import argparse
import json
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
import report_t09_b_live

ARTIFACT_PARENT = ROOT / "artifacts" / "cs-t0"
WORK_PARENT = ROOT / "out" / "cs-t0"
MIN_FREE_BYTES = 1024 * 1024 * 1024
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
    "cs-t0/src/coop_active.rs",
    "cs-t0/src/win32.rs",
    "cs-t0/src/loop_ipc.rs",
    "cs-t0/native/IsaacCsT0.Native.def",
    "cs-t0/native/IsaacCsT0.Native.vcxproj",
    "cs-t0/native/IsaacCsT0.Helper.vcxproj",
    "cs-t0/native/IsaacCsT0.T09EndpointScenario.vcxproj",
    "cs-t0/native/include/isaac_cs_t0",
    "cs-t0/native/src",
    "cs-t0/native/scenarios",
    "cs-t0/build-t09-b.ps1",
    "cs-t0/accept-t09-b-live.py",
    "cs-t0/report_t09_b_live.py",
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
        "build_label": "t0-09-b-live-build",
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
            "b": {"os": "vm", "role": "remote", "artifact_subdir": "vm"},
        },
        "script": {
            "id": "t09b-live-f0-v1",
            "first_tick": 0,
            "last_tick": 10000,
            "lead_ticks": 8,
            "tick_interval_ms": 16,
        },
        "relay": {
            "a": {"bind": "127.0.0.1:0"},
            "b": {"bind": "192.168.137.1:0"},
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
        "control": {"bind": "192.168.137.1:0"},
        "artifact_root": artifact_root,
        "user_preconditions": {
            "safe_run": True,
            "only_a": True,
            "no_original_online": True,
            "real_game_authorized": True,
        },
    }


def run_build(command: list[str], timeout: float, log_prefix: Path) -> tuple[int, bytes, bytes]:
    process = subprocess.Popen(
        command,
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        creationflags=CREATE_NO_WINDOW,
    )
    try:
        stdout, stderr = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        process.kill()
        stdout, stderr = process.communicate()
        raise RuntimeError("pure compilation timeout")
    write_new_bytes(log_prefix.with_suffix(".stdout.log"), stdout)
    write_new_bytes(log_prefix.with_suffix(".stderr.log"), stderr)
    return process.returncode, stdout, stderr


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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--artifact-dir", type=Path, required=True)
    parser.add_argument("--authorization", required=True)
    parser.add_argument("--vm-share", type=Path, default=ROOT / "cs-t0" / "vm_share")
    parser.add_argument(
        "--vm-guest-share",
        default=r"\\tsclient\F\projects_isaac\issac_multiplay\cs-t0\vm_share",
    )
    parser.add_argument("--await-b-seconds", type=int, default=600)
    parser.add_argument("--run-timeout-seconds", type=int, default=3300)
    args = parser.parse_args()

    target = args.artifact_dir.resolve()
    if target.parent != ARTIFACT_PARENT.resolve() or target.exists() or not target.name:
        raise SystemExit(f"artifact-dir must be a new direct child of {ARTIFACT_PARENT}")
    if not args.authorization.strip():
        raise SystemExit("--authorization must record the live operation authorization")
    if not args.vm_share.is_dir():
        raise SystemExit(f"VM shared host directory does not exist: {args.vm_share}")
    ARTIFACT_PARENT.mkdir(parents=True, exist_ok=True)
    WORK_PARENT.mkdir(parents=True, exist_ok=True)
    work = WORK_PARENT / (target.name + "-work")
    if work.exists():
        raise SystemExit(f"work directory already exists; no automatic retry: {work}")
    free = shutil.disk_usage(ARTIFACT_PARENT).free
    if free < MIN_FREE_BYTES:
        raise SystemExit(f"insufficient free space: {free} < {MIN_FREE_BYTES}")
    work.mkdir(parents=False, exist_ok=False)
    run_id = f"tx-03-02-F0-live-{datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')}"
    manifest = build_manifest(run_id, target.relative_to(ROOT).as_posix())
    manifest_path = work / "manifest-input.json"
    write_new_json(manifest_path, manifest)
    write_new_json(
        work / "user-environment.json",
        {
            "tx": "tx-03-02",
            "vmName": "15468",
            "vmIp": "192.168.137.148",
            "hostIp": "192.168.137.1",
            "vmShareHost": str(args.vm_share.resolve()),
            "vmShareGuest": args.vm_guest_share,
            "gameBuild": "same-approved-build",
            "seed": "PY1JDB20",
            "room": "initial-room",
            "hostACharacter": "isaac",
            "remoteBCharacter": "isaac",
            "originalOnline": "NOT_USED",
            "rdpSessionMustRemainConnected": True,
            "authorization": "user confirmed Native/Helper injection, owned virtual B, UDP relay/TCP control and normal stop/restore",
            "automaticRetry": False,
        },
    )
    build_dir = work / "build"
    build_command = [
        "pwsh",
        "-NoProfile",
        "-File",
        str(TOOLS / "build-t09-b.ps1"),
        "-OutputDirectory",
        build_dir.relative_to(ROOT).as_posix(),
        "-Transaction",
        "tx-03-02",
    ]
    errors: list[dict[str, Any]] = []
    orchestrator_exit: int | None = None
    orchestrator_stdout = b""
    orchestrator_stderr = b""
    ready_record: dict[str, Any] | None = None
    try:
        build_exit, _, _ = run_build(build_command, 600, work / "build")
        if build_exit != 0:
            raise RuntimeError(f"pure compilation failed: {build_exit}")
        t09_exe = build_dir / "Rust" / "IsaacCsT0T09.exe"
        native_dll = build_dir / "Native" / "bin" / "IsaacCsT0.Native.dll"
        helper = build_dir / "Helper" / "bin" / "IsaacCsT0.Helper.exe"
        for path in (t09_exe, native_dll, helper):
            if not path.is_file():
                raise RuntimeError(f"compiled live executable missing: {path}")
        command = [
            str(t09_exe),
            "live-f0",
            "--manifest",
            str(manifest_path),
            "--artifact-dir",
            str(target),
            "--native-dll",
            str(native_dll),
            "--helper",
            str(helper),
            "--vm-share",
            str(args.vm_share.resolve()),
            "--vm-guest-share",
            args.vm_guest_share,
            "--await-b-seconds",
            str(args.await_b_seconds),
        ]
        write_new_json(
            work / "invocation-private.json",
            {
                "tx": "tx-03-02",
                "authorizationRecorded": True,
                "runId": run_id,
                "vmGuestShare": args.vm_guest_share,
                "awaitBSeconds": args.await_b_seconds,
                "automaticRetry": False,
                "hashes": "NOT_USED",
            },
        )
        process = subprocess.Popen(
            command,
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            creationflags=CREATE_NO_WINDOW,
        )
        started = time.monotonic()
        announced = False
        while process.poll() is None:
            launch_path = target / "control" / "vm-launch.json"
            if not announced and launch_path.is_file():
                ready_record = json.loads(launch_path.read_text(encoding="utf-8"))
                write_new_json(target / "control" / "live-ready-for-vm.json", ready_record)
                print(
                    "LIVE_READY_FOR_VM "
                    + json.dumps(
                        {
                            "runId": run_id,
                            "vmScript": ready_record.get("scriptGuest"),
                            "hostScript": ready_record.get("scriptHost"),
                            "instruction": "Keep RDP connected; run the VM script exactly once now.",
                        },
                        ensure_ascii=False,
                    ),
                    flush=True,
                )
                announced = True
            if time.monotonic() - started > args.run_timeout_seconds:
                errors.append(
                    {
                        "type": "LiveRunTimeout",
                        "message": "live orchestrator did not finish within the run timeout; no automatic retry",
                    }
                )
                process.kill()
                break
            time.sleep(0.5)
        orchestrator_stdout, orchestrator_stderr = process.communicate()
        orchestrator_exit = process.returncode
        if not announced:
            errors.append(
                {
                    "type": "VMReadyPromptMissing",
                    "message": "orchestrator exited before writing control/vm-launch.json",
                }
            )
        if orchestrator_exit != 0:
            errors.append(
                {
                    "type": "LiveOrchestratorFailure",
                    "message": f"live orchestrator exit {orchestrator_exit}",
                }
            )
    except BaseException as error:
        errors.append({"type": type(error).__name__, "message": str(error)})

    if not target.exists():
        target.mkdir(parents=False, exist_ok=False)
    control_dir = target / "control"
    control_dir.mkdir(parents=True, exist_ok=True)
    try:
        write_new_bytes(control_dir / "orchestrator.stdout.log", orchestrator_stdout)
        write_new_bytes(control_dir / "orchestrator.stderr.log", orchestrator_stderr)
        shutil.copy2(manifest_path, control_dir / "input-manifest.json")
        if (work / "invocation-private.json").is_file():
            shutil.copy2(work / "invocation-private.json", control_dir / "invocation-private.json")
        shutil.copy2(work / "user-environment.json", control_dir / "user-environment.json")
        if build_dir.is_dir():
            shutil.copytree(build_dir, target / "build")
        copy_source(target)
    except BaseException as error:
        errors.append({"type": "ArtifactSnapshot", "message": str(error)})
    try:
        analyzer = report_t09_b_live.analyze(target)
    except BaseException as error:
        analyzer = None
        errors.append({"type": "LiveAnalyzer", "message": str(error)})
    if analyzer is not None and analyzer.get("status") != "LIVE_F0_PASS":
        errors.append(
            {
                "type": "LiveAnalyzerChecks",
                "message": f"{analyzer.get('checksPassed')}/{analyzer.get('checksTotal')} checks passed",
            }
        )
    try:
        provenance = []
        for path in sorted(target.rglob("*")):
            if path.is_file() and path.name not in {"provenance.json", "acceptance-result.json"}:
                stat = path.stat()
                provenance.append(
                    {
                        "path": path.relative_to(target).as_posix(),
                        "bytes": stat.st_size,
                        "mtime_ns": stat.st_mtime_ns,
                    }
                )
        write_new_json(
            target / "provenance.json",
            {
                "tx": "tx-03-02",
                "hashes": "NOT_USED",
                "identity": "IsaacCsT0-T09B-LiveAcceptance-v1",
                "generatedUtc": utc_now(),
                "files": provenance,
            },
        )
    except BaseException as error:
        errors.append({"type": "Provenance", "message": str(error)})
    status = (
        "LIVE_F0_PASS"
        if orchestrator_exit == 0
        and not errors
        and analyzer is not None
        and analyzer.get("status") == "LIVE_F0_PASS"
        else "FAIL"
    )
    if status == "LIVE_F0_PASS":
        resolved = work.resolve()
        if resolved.parent == WORK_PARENT.resolve() and resolved.name == target.name + "-work":
            try:
                shutil.rmtree(resolved)
            except BaseException as error:
                errors.append({"type": "WorkCleanup", "message": str(error)})
                status = "FAIL"
    result = {
        "tx": "tx-03-02",
        "identity": "IsaacCsT0-T09B-LiveAcceptance-v1",
        "status": status,
        "t0_09_b": "P1_F0_PASS" if status == "LIVE_F0_PASS" else "IN_PROGRESS",
        "t0_09": "IN_PROGRESS",
        "scope": "live host+VM two-game P1 F0",
        "runId": run_id,
        "orchestratorExit": orchestrator_exit,
        "vmReadyPrompted": ready_record is not None,
        "errors": errors,
        "analyzer": analyzer,
        "automaticRetry": False,
        "hashes": "NOT_USED",
        "generatedUtc": utc_now(),
    }
    write_new_json(target / "acceptance-result.json", result)
    print(
        json.dumps(
            {
                "status": status,
                "t0_09_b": result["t0_09_b"],
                "t0_09": result["t0_09"],
                "orchestratorExit": orchestrator_exit,
                "vmReadyPrompted": ready_record is not None,
                "analyzer": {
                    "status": analyzer.get("status") if analyzer else None,
                    "passed": analyzer.get("checksPassed") if analyzer else None,
                    "total": analyzer.get("checksTotal") if analyzer else None,
                },
                "errors": errors,
            },
            ensure_ascii=False,
        )
    )
    return 0 if status == "LIVE_F0_PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
