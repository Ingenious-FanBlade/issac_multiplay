"""One unified offline acceptance for T0-07 operation 8; no game access."""
import argparse
import json
import shutil
import subprocess
import time
from pathlib import Path


def field(row, path):
    value = row
    for part in path.split('.'):
        value = value[int(part)] if isinstance(value, list) else value[part]
    return value


def differences(row, expected):
    failures = {}
    for key, wanted in expected.items():
        try:
            actual = field(row, key)
        except (KeyError, IndexError, TypeError, ValueError):
            actual = None
        if actual != wanted:
            failures[key] = {'expected': wanted, 'actual': actual}
    return failures


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--artifact-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    target = args.artifact_dir.resolve()
    target.mkdir(parents=True, exist_ok=False)
    result = {
        'tx': 'tx-01-97',
        'scope': 'T0-07 operation 8 production fault and boundary matrix',
        'status': 'FAIL',
        'game_access': False,
        'native_runtime': 'NOT_RUN',
        'checks': [],
        'errors': [],
    }
    try:
        build = args.build_dir.resolve()
        plan = json.loads((root / 'cs-t0/testdata/tx-01-97-fault-matrix.json')
                          .read_text(encoding='utf-8'))
        names = [
            'cs-t0/accept-fault-matrix.py',
            'cs-t0/build-fault-matrix.ps1',
            'cs-t0/testdata/tx-01-97-fault-matrix.json',
            'cs-t0/native/scenarios/fault_matrix_main.cpp',
            'cs-t0/native/IsaacCsT0.FaultMatrixScenario.vcxproj',
            'cs-t0/native/IsaacCsT0.Native.vcxproj',
            'cs-t0/native/include/isaac_cs_t0/authority_tick.hpp',
            'cs-t0/native/src/authority_tick.cpp',
            'cs-t0/native/include/isaac_cs_t0/authority_tick_pipeline.hpp',
            'cs-t0/native/src/authority_tick_pipeline.cpp',
            'cs-t0/native/include/isaac_cs_t0/authority_session.hpp',
            'cs-t0/native/src/authority_session.cpp',
            'cs-t0/native/include/isaac_cs_t0/coop_input.hpp',
            'cs-t0/native/src/coop_input.cpp',
            'cs-t0/native/include/isaac_cs_t0/input_core.hpp',
            'cs-t0/native/src/input_core.cpp',
            'cs-t0/native/include/isaac_cs_t0/coop_active_ipc.hpp',
            'cs-t0/native/src/coop_active_runtime.cpp',
            'docs/refactor/cs-t0/t0-07/tx-01-97-fault-matrix.md',
            'docs/refactor/cs-t0/t0-07/tx-01-97-static-review.md',
        ]
        for name in names:
            destination = target / 'sources' / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(root / name, destination)

        binaries = target / 'build'
        binaries.mkdir()
        provenance = []
        for relative in [
            'Native/bin/IsaacCsT0.Native.dll',
            'FaultMatrixScenario/bin/IsaacCsT0.FaultMatrixScenario.exe',
            'Native-build.log',
            'FaultMatrixScenario-build.log',
            'toolchain.json',
        ]:
            source = build / relative
            copy = binaries / source.name
            shutil.copy2(source, copy)
            provenance.append({'path': str(source), 'bytes': source.stat().st_size,
                               'mtime_ns': source.stat().st_mtime_ns})
        (target / 'binary-provenance.json').write_text(
            json.dumps({'method': 'paths, lengths and timestamps; no digests',
                        'files': provenance}, indent=2) + '\n', encoding='utf-8')

        transcript = []
        expected = []
        for scene in plan['scenarios']:
            transcript.append('scenario {} {} {}'.format(
                scene['name'], scene['initial_tick'], scene['sequence_start']))
            for index, item in enumerate(scene['steps'], 1):
                transcript.append(item['command'])
                expected.append((scene['name'], index, item))
        commands = target / 'commands.txt'
        commands.write_text('\n'.join(transcript) + '\n', encoding='utf-8')
        invocation = [str(binaries / 'IsaacCsT0.FaultMatrixScenario.exe'),
                      str(commands), str(target / 'events.jsonl')]
        result['invocation'] = invocation
        started = time.monotonic()
        process = subprocess.run(invocation, capture_output=True, text=True,
                                 timeout=120, check=False)
        result['exit_code'] = process.returncode
        result['elapsed_seconds'] = time.monotonic() - started
        (target / 'stdout.log').write_text(process.stdout, encoding='utf-8')
        (target / 'stderr.log').write_text(process.stderr, encoding='utf-8')
        actual = [json.loads(line) for line in
                  (target / 'events.jsonl').read_text(encoding='utf-8').splitlines()]
        result['checks'].append({
            'name': 'complete_transcript',
            'passed': process.returncode == 0 and len(actual) == len(expected),
            'exit_code': process.returncode,
            'expected': len(expected),
            'actual': len(actual),
        })
        for index, (name, step_index, item) in enumerate(expected):
            row = actual[index] if index < len(actual) else {}
            wanted = dict(item['expect'], scenario=name, step=step_index)
            failures = differences(row, wanted)
            result['checks'].append({
                'scenario': name,
                'step': step_index,
                'command': item['command'],
                'passed': not failures,
                'differences': failures,
            })
        result['scenarios'] = len(plan['scenarios'])
        result['commands'] = len(expected)
        result['passed'] = sum(check['passed'] for check in result['checks'])
        result['total'] = len(result['checks'])
        result['status'] = 'PASS' if result['passed'] == result['total'] else 'FAIL'
    except Exception as error:
        result['errors'].append({'type': type(error).__name__, 'message': str(error)})
    finally:
        (target / 'result.json').write_text(
            json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items() if key != 'checks'},
                     ensure_ascii=False))
    return 0 if result['status'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
