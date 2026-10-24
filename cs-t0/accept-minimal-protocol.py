"""One complete offline two-endpoint acceptance invocation for tx-01-99."""
import argparse
import json
import shutil
import subprocess
import time
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--artifact-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    target = args.artifact_dir.resolve()
    target.mkdir(parents=True, exist_ok=False)
    result = dict(tx='tx-01-99', scope='T0-08 operations 1-4; owned in-memory endpoint lifecycle',
                  status='FAIL', game_access=False, sockets='NOT_RUN', native_runtime='NOT_RUN',
                  automatic_retry=False, hashes='NOT_USED', checks=[], errors=[])
    try:
        names = ['cs-t0/accept-minimal-protocol.py', 'cs-t0/build-minimal-protocol.ps1',
                 'cs-t0/native/scenarios/minimal_protocol_main.cpp',
                 'cs-t0/native/IsaacCsT0.MinimalProtocolScenario.vcxproj',
                 'cs-t0/native/include/isaac_cs_t0/minimal_protocol.hpp',
                 'cs-t0/native/src/minimal_protocol.cpp',
                 'cs-t0/native/include/isaac_cs_t0/coop_input.hpp', 'cs-t0/native/src/coop_input.cpp',
                 'cs-t0/native/include/isaac_cs_t0/authority_session.hpp', 'cs-t0/native/src/authority_session.cpp',
                 'cs-t0/native/include/isaac_cs_t0/input_core.hpp', 'cs-t0/native/src/input_core.cpp',
                 'docs/refactor/cs-t0/t0-08/tx-01-99-failure-modes.md',
                 'docs/refactor/cs-t0/t0-08/tx-01-99-wire.md',
                 'docs/refactor/cs-t0/t0-08/tx-01-99-static-review.md']
        for name in names:
            destination = target / 'sources' / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(root / name, destination)
        build = args.build_dir.resolve()
        evidence = target / 'build'
        evidence.mkdir()
        provenance = []
        for name in ['bin/IsaacCsT0.MinimalProtocolScenario.exe', 'build.log', 'toolchain.json']:
            source = build / name
            shutil.copy2(source, evidence / source.name)
            provenance.append(dict(path=str(source), bytes=source.stat().st_size, mtime_ns=source.stat().st_mtime_ns))
        (target / 'provenance.json').write_text(json.dumps(provenance, indent=2) + '\n', encoding='utf-8')
        invocation = [str(evidence / 'IsaacCsT0.MinimalProtocolScenario.exe'),
                      str(target / 'checks.jsonl'), str(target / 'packets.jsonl')]
        result['invocation'] = invocation
        started = time.monotonic()
        with (target / 'stdout.log').open('wb') as stdout, (target / 'stderr.log').open('wb') as stderr:
            process = subprocess.run(invocation, stdout=stdout, stderr=stderr, timeout=60, check=False)
        result['elapsed_seconds'] = time.monotonic() - started
        result['exit_code'] = process.returncode
        result['checks'] = [json.loads(row) for row in (target / 'checks.jsonl').read_text(encoding='utf-8').splitlines()]
        packets = [json.loads(row) for row in (target / 'packets.jsonl').read_text(encoding='utf-8').splitlines()]
        result['packet_records'] = len(packets)
        result['passed'] = sum(row['passed'] for row in result['checks'])
        result['total'] = len(result['checks'])
        result['status'] = 'PASS' if process.returncode == 0 and result['total'] > 200 and result['passed'] == result['total'] and packets else 'FAIL'
    except Exception as error:
        result['errors'].append(dict(type=type(error).__name__, message=str(error)))
    finally:
        (target / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items() if key != 'checks'}, ensure_ascii=False))
    return 0 if result['status'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
