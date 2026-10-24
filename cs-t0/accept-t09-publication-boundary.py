"""ONE tx-03-06 production publication-link integration acceptance.

Pure compilation, one scenario process, independent inspection of captured wire
bytes, and a repeatable artifact. Never touches a game, injects, retries, or
computes file/state hashes. This is not a two-machine live F0 result.
"""
from __future__ import annotations

import argparse
import json
import shutil
import struct
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCES = [
    "AGENTS.md", "docs/refactor/project-conventions.md",
    "docs/refactor/cs-t0/t0-09/tx-03-06-publication-boundary.md",
    "cs-t0/native/include/isaac_cs_t0", "cs-t0/native/src",
    "cs-t0/native/scenarios",
    "cs-t0/native/IsaacCsT0.T09PublicationScenario.vcxproj",
    "cs-t0/native/IsaacCsT0.T09EndpointScenario.vcxproj",
    "cs-t0/native/IsaacCsT0.Native.vcxproj", "cs-t0/native/IsaacCsT0.Native.def",
    "cs-t0/native/IsaacCsT0.Helper.vcxproj",
    "cs-t0/build-t09-b.ps1", "cs-t0/build-t09-publication.ps1",
    "cs-t0/accept-t09-publication-boundary.py", "cs-t0/Cargo.toml", "cs-t0/Cargo.lock",
    "cs-t0/src",
]


def write_json(path: Path, value: object) -> None:
    with path.open("x", encoding="utf-8") as handle:
        json.dump(value, handle, ensure_ascii=False, indent=2)
        handle.write("\n")


def inspect_capture(target: Path) -> list[dict]:
    checks: list[dict] = []
    expected = {"ten-thousand": (4795, 10000), "single-tick": (0, 1), "offset-window": (900, 7)}
    ticks = {name: {3: [], 4: []} for name in expected}
    sequences: dict[tuple[str, str], list[int]] = {}
    envelope_valid = True
    with (target / "evidence/packets.jsonl").open(encoding="utf-8") as handle:
        for line in handle:
            row = json.loads(line)
            raw = bytes.fromhex(row["hex"])
            kind = struct.unpack_from("<H", raw, 6)[0]
            sequence = struct.unpack_from("<Q", raw, 40)[0]
            envelope_valid &= raw[:4] == b"ICT8" and struct.unpack_from("<H", raw, 4)[0] == 2
            envelope_valid &= len(raw) == 64 + struct.unpack_from("<I", raw, 48)[0]
            envelope_valid &= kind == row["kind"] and sequence == row["sequence"]
            sequences.setdefault((row["case"], row["direction"]), []).append(sequence)
            if row["case"] in expected and row["direction"] == "authority-to-remote":
                if kind == 3 and struct.unpack_from("<H", raw, 134)[0] == 2:
                    tick = struct.unpack_from("<Q", raw, 64)[0]
                    ticks[row["case"]][kind].append(tick)
                    envelope_valid &= tick == row["slotTick"] == row["serverTick"]
                elif kind == 4:
                    tick = struct.unpack_from("<Q", raw, 96)[0]
                    ticks[row["case"]][kind].append(tick)
                    envelope_valid &= tick == row["slotTick"] == row["stateTick"]
    checks.append({"name": "capture.actual-byte-envelopes", "pass": bool(envelope_valid and sequences)})
    checks.append({"name": "capture.contiguous-packet-sequences", "pass": all(
        seq == list(range(1, len(seq) + 1)) for seq in sequences.values())})
    for name, (start, count) in expected.items():
        wanted = list(range(start + 1, start + count + 1))
        checks.append({"name": f"capture.{name}.exact-ack-state-ticks",
                       "pass": ticks[name][3] == wanted and ticks[name][4] == wanted})
    return checks


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact-dir", required=True)
    args = parser.parse_args()
    target = (ROOT / args.artifact_dir).resolve()
    if target.parent != (ROOT / "artifacts/cs-t0").resolve() or target.exists():
        parser.error("artifact-dir must be a new direct child of artifacts/cs-t0")
    target.mkdir(parents=True)
    result = {"tx": "tx-03-06", "startedUtc": datetime.now(timezone.utc).isoformat(),
              "scope": "production AuthorityWireLink + Windows mapping + Remote Endpoint integration",
              "liveGameExecuted": False, "automaticRetry": False, "status": "FAIL"}
    try:
        for relative in SOURCES:
            origin, destination = ROOT / relative, target / "source" / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            if origin.is_dir():
                shutil.copytree(origin, destination)
            else:
                shutil.copy2(origin, destination)
        build = ["pwsh", "-NoProfile", "-File",
                 str(ROOT / "cs-t0/build-t09-publication.ps1"),
                 "-OutputDirectory", (target / "build").relative_to(ROOT).as_posix()]
        write_json(target / "commands.json", {"build": build, "scenarioInvocations": 1,
                    "repeat": "Use the same entry with a new artifact-dir; never reuse an existing directory."})
        with (target / "build.log").open("x", encoding="utf-8") as handle:
            compiled = subprocess.run(build, cwd=ROOT, stdout=handle, stderr=subprocess.STDOUT,
                                      timeout=1200, creationflags=subprocess.CREATE_NO_WINDOW)
        result["buildExitCode"] = compiled.returncode
        if compiled.returncode:
            raise RuntimeError("pure compilation failed; integration scenario was not executed")
        evidence = target / "evidence"
        evidence.mkdir()
        scenario = [str(target / "build/Publication/bin/IsaacCsT0.T09PublicationScenario.exe"), str(evidence)]
        write_json(target / "scenario-command.json", scenario)
        with (target / "scenario.log").open("x", encoding="utf-8") as handle:
            run = subprocess.run(scenario, cwd=ROOT, stdout=handle, stderr=subprocess.STDOUT,
                                 timeout=180, creationflags=subprocess.CREATE_NO_WINDOW)
        result["scenarioExitCode"] = run.returncode
        scenario_result = json.loads((evidence / "scenario-result.json").read_text(encoding="utf-8"))
        checks = [json.loads(line) for line in (evidence / "checks.jsonl").read_text(encoding="utf-8").splitlines()]
        scenario_check_count = len(checks)
        checks += inspect_capture(target)
        checks.append({"name": "scenario.completed-once", "pass": run.returncode == 0 and
                       scenario_result["pass"] and
                       scenario_result["passed"] + scenario_result["failed"] == scenario_check_count})
        write_json(target / "checks.json", checks)
        result.update(passed=sum(row["pass"] for row in checks), total=len(checks),
                      firstFailure=next((row for row in checks if not row["pass"]), None))
        if all(row["pass"] for row in checks):
            result["status"] = "PASS"
    except Exception as error:
        result["error"] = f"{type(error).__name__}: {error}"
    result["finishedUtc"] = datetime.now(timezone.utc).isoformat()
    write_json(target / "acceptance-result.json", result)
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    sys.exit(main())
