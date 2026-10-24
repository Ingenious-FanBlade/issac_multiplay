"""One unified offline acceptance for T0-07 operation 6; no game access."""
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
        'tx': 'tx-01-95',
        'scope': 'T0-07 operation 6 missing input, source silence timeout and recovery',
        'status': 'FAIL',
        'game_access': False,
        'native_runtime': 'NOT_RUN',
        'checks': [],
        'errors': [],
    }
    try:
        build = args.build_dir.resolve()
        plan_path = root / 'cs-t0/testdata/tx-01-95-missing-input.json'
        plan = json.loads(plan_path.read_text(encoding='utf-8'))
        names = [
            'cs-t0/accept-missing-input.py',
            'cs-t0/build-missing-input.ps1',
            'cs-t0/testdata/tx-01-95-missing-input.json',
            'cs-t0/native/scenarios/missing_input_main.cpp',
            'cs-t0/native/IsaacCsT0.MissingInputScenario.vcxproj',
            'cs-t0/native/IsaacCsT0.Native.vcxproj',
            'cs-t0/native/include/isaac_cs_t0/authority_tick_pipeline.hpp',
            'cs-t0/native/src/authority_tick_pipeline.cpp',
            'cs-t0/native/include/isaac_cs_t0/authority_tick.hpp',
            'cs-t0/native/src/authority_tick.cpp',
            'cs-t0/native/include/isaac_cs_t0/authority_session.hpp',
            'cs-t0/native/src/authority_session.cpp',
            'cs-t0/native/include/isaac_cs_t0/coop_input.hpp',
            'cs-t0/native/src/coop_input.cpp',
            'cs-t0/native/include/isaac_cs_t0/input_core.hpp',
            'cs-t0/native/src/input_core.cpp',
            'cs-t0/native/include/isaac_cs_t0/coop_active_ipc.hpp',
            'cs-t0/native/src/coop_active_runtime.cpp',
            'docs/refactor/cs-t0/t0-07/tx-01-95-missing-input-timeout-recovery.md',
            'docs/refactor/cs-t0/t0-07/tx-01-95-static-review.md',
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
            'MissingInputScenario/bin/IsaacCsT0.MissingInputScenario.exe',
            'Native-build.log',
            'MissingInputScenario-build.log',
            'toolchain.json',
        ]:
            source = build / relative
            copy = binaries / source.name
            shutil.copy2(source, copy)
            provenance.append({
                'path': str(source),
                'bytes': source.stat().st_size,
                'mtime_ns': source.stat().st_mtime_ns,
            })
        (target / 'binary-provenance.json').write_text(
            json.dumps({'method': 'paths, lengths and timestamps; no digests',
                        'files': provenance}, indent=2) + '\n', encoding='utf-8')

        transcript = []
        expected = []
        for scenario in plan['scenarios']:
            transcript.append(
                'scenario ' + scenario['name'] + ' ' +
                str(scenario.get('initial_tick', 0)) + ' ' +
                str(scenario.get('budget', 258))
            )
            for index, item in enumerate(scenario['steps'], 1):
                transcript.append(item['command'])
                expected.append((scenario['name'], index, item))
        commands = target / 'commands.txt'
        commands.write_text('\n'.join(transcript) + '\n', encoding='utf-8')

        invocation = [
            str(binaries / 'IsaacCsT0.MissingInputScenario.exe'),
            str(commands),
            str(target / 'events.jsonl'),
            str(target / 'batch-events.jsonl'),
        ]
        result['invocation'] = invocation
        started = time.monotonic()
        with (target / 'stdout.log').open('wb') as stdout, (target / 'stderr.log').open('wb') as stderr:
            process = subprocess.run(invocation, stdout=stdout, stderr=stderr, timeout=60, check=False)
        result['exit_code'] = process.returncode
        result['elapsed_seconds'] = time.monotonic() - started

        actual = [json.loads(line) for line in
                  (target / 'events.jsonl').read_text(encoding='utf-8').splitlines()]
        result['checks'].append({
            'name': 'complete_transcript',
            'passed': process.returncode == 0 and len(actual) == len(expected),
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

        batches = [json.loads(line) for line in
                   (target / 'batch-events.jsonl').read_text(encoding='utf-8').splitlines()]
        expected_batch_count = sum(scene.get('batch_count', 0) for scene in plan['scenarios'])
        result['checks'].append({
            'name': 'complete_cycles',
            'passed': len(batches) == expected_batch_count,
            'expected': expected_batch_count,
            'actual': len(batches),
        })
        for expected_batch in [entry for scene in plan['scenarios']
                               for entry in scene.get('batch_expect', [])]:
            matching = [row for row in batches
                        if row.get('scenario') == expected_batch['scenario'] and
                        row.get('sequence') == expected_batch['sequence']]
            row = matching[0] if matching else {}
            failures = differences(row, expected_batch['expect'])
            result['checks'].append({
                'name': 'batch_observation',
                'scenario': expected_batch['scenario'],
                'sequence': expected_batch['sequence'],
                'passed': len(matching) == 1 and not failures,
                'differences': failures,
            })

        policy = plan['policy']
        policy_passed = bool(actual) and all(
            str(actual[0].get('policy', {}).get(key)) == str(value)
            for key, value in policy.items()
            if key in ('max_input_age_ticks', 'client_silence_timeout_ticks')
        )
        result['checks'].append({'name': 'policy_constants', 'passed': policy_passed})

        result['scenarios'] = len(plan['scenarios'])
        result['commands'] = len(expected)
        result['cycle_records'] = len(batches)
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
