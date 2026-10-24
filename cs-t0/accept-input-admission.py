"""Single offline production-chain acceptance for T0-07 operation 4; no game access."""
import argparse
import json
import shutil
import subprocess
import time
from pathlib import Path


def field(row, path):
    for part in path.split('.'):
        row = row[int(part)] if isinstance(row, list) else row[part]
    return row


def differences(row, expected):
    failures = {}
    for key, value in expected.items():
        try:
            actual = field(row, key)
        except (KeyError, IndexError, TypeError, ValueError):
            actual = None
        if actual != value:
            failures[key] = {'expected': value, 'actual': actual}
    return failures


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--artifact-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    target = args.artifact_dir.resolve()
    target.mkdir(parents=True, exist_ok=False)
    result = {'tx': 'tx-01-92', 'scope': 'T0-07 operation 4 offline production Session/Clock/codec/Participant lifecycles',
              'status': 'FAIL', 'game_access': False, 'native_runtime': 'NOT_RUN', 'checks': [], 'errors': []}
    try:
        build = args.build_dir.resolve()
        plan = json.loads((root / 'cs-t0/testdata/tx-01-92-input-admission.json').read_text(encoding='utf-8'))
        names = ['cs-t0/accept-input-admission.py', 'cs-t0/build-input-admission.ps1',
                 'cs-t0/testdata/tx-01-92-input-admission.json', 'cs-t0/native/scenarios/input_admission_main.cpp',
                 'cs-t0/native/IsaacCsT0.InputAdmissionScenario.vcxproj', 'cs-t0/native/IsaacCsT0.Native.vcxproj',
                 'cs-t0/native/include/isaac_cs_t0/authority_tick.hpp', 'cs-t0/native/src/authority_tick.cpp',
                 'cs-t0/native/include/isaac_cs_t0/authority_session.hpp', 'cs-t0/native/src/authority_session.cpp',
                 'cs-t0/native/include/isaac_cs_t0/coop_input.hpp', 'cs-t0/native/src/coop_input.cpp',
                 'cs-t0/native/include/isaac_cs_t0/input_core.hpp', 'cs-t0/native/src/input_core.cpp',
                 'cs-t0/native/include/isaac_cs_t0/loop_runtime.hpp', 'cs-t0/native/src/loop_runtime.cpp',
                 'cs-t0/native/src/coop_active_runtime.cpp', 'cs-t0/native/include/isaac_cs_t0/coop_active_ipc.hpp',
                 'docs/refactor/cs-t0/t0-07/tx-01-92-input-admission.md',
                 'docs/refactor/cs-t0/t0-07/tx-01-92-static-review.md']
        for name in names:
            destination = target / 'sources' / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(root / name, destination)
        binaries = target / 'build'
        binaries.mkdir()
        provenance = []
        for relative in ['Native/bin/IsaacCsT0.Native.dll', 'InputAdmissionScenario/bin/IsaacCsT0.InputAdmissionScenario.exe',
                         'Native-build.log', 'InputAdmissionScenario-build.log', 'toolchain.json']:
            source = build / relative
            shutil.copy2(source, binaries / source.name)
            provenance.append({'path': str(source), 'bytes': source.stat().st_size, 'mtime_ns': source.stat().st_mtime_ns})
        (target / 'binary-provenance.json').write_text(json.dumps({'method': 'paths, lengths and timestamps; no digests',
                                                                  'files': provenance}, indent=2) + '\n', encoding='utf-8')
        transcript, expected = [], []
        for scenario in plan['scenarios']:
            transcript.append('scenario ' + scenario['name'] + ' ' + scenario.get('initial_tick', '0'))
            for index, step in enumerate(scenario['steps'], 1):
                transcript.append(step['command'])
                expected.append((scenario['name'], index, step))
        commands = target / 'commands.txt'
        commands.write_text('\n'.join(transcript) + '\n', encoding='utf-8')
        invocation = [str(binaries / 'IsaacCsT0.InputAdmissionScenario.exe'), str(commands),
                      str(target / 'events.jsonl'), str(target / 'batch-events.jsonl'), str(target / 'admissions.jsonl')]
        result['invocation'] = invocation
        started = time.monotonic()
        with (target / 'stdout.log').open('wb') as stdout, (target / 'stderr.log').open('wb') as stderr:
            process = subprocess.run(invocation, stdout=stdout, stderr=stderr, timeout=60, check=False)
        result['exit_code'] = process.returncode
        result['elapsed_seconds'] = time.monotonic() - started
        actual = [json.loads(line) for line in (target / 'events.jsonl').read_text(encoding='utf-8').splitlines()]
        result['checks'].append({'name': 'complete_transcript', 'passed': process.returncode == 0 and len(actual) == len(expected),
                                 'expected': len(expected), 'actual': len(actual)})
        for index, (name, step_index, step) in enumerate(expected):
            row = actual[index] if index < len(actual) else {}
            failures = differences(row, dict(step['expect'], scenario=name, step=step_index))
            result['checks'].append({'scenario': name, 'step': step_index, 'command': step['command'],
                                     'passed': not failures, 'differences': failures})
        batches = [json.loads(line) for line in (target / 'batch-events.jsonl').read_text(encoding='utf-8').splitlines()]
        result['checks'].append({'name': 'complete_batches', 'passed': len(batches) == sum(s.get('batch', {}).get('count', 0) for s in plan['scenarios']),
                                 'actual': len(batches)})
        for scenario in plan['scenarios']:
            if 'batch' not in scenario:
                continue
            group = [row for row in batches if row['scenario'] == scenario['name']]
            batch = scenario['batch']
            failures = differences(group[-1] if group else {}, batch['last'])
            ordered = all(row['sequence'] == str(i) and row['target'] == str(i if scenario['name'] in ('history-eviction-highwater','local-continues-remote-outage') else 1)
                          and row['completed'] == str(i if scenario['name'] in ('history-eviction-highwater','local-continues-remote-outage') else 0)
                          and row['decision'] == ('Accepted' if i <= batch['accepted'] else 'Full')
                          for i, row in enumerate(group, 1))
            result['checks'].append({'scenario': scenario['name'], 'name': 'batch_lifecycles',
                                     'passed': len(group) == batch['count'] and ordered and not failures,
                                     'expected': batch['count'], 'actual': len(group), 'differences': failures})
        arrivals = [json.loads(line) for line in (target / 'admissions.jsonl').read_text(encoding='utf-8').splitlines()]
        result['checks'].append({'name': 'exact_admission_transcript', 'passed': len(arrivals) == plan['admissions'] and
                                 all(len(row['bytes_le']) == row['length'] * 2 for row in arrivals),
                                 'expected': plan['admissions'], 'actual': len(arrivals)})
        for scenario in plan['scenarios']:
            for check in scenario.get('admission_checks', []):
                group = [row for row in arrivals if row['scenario'] == scenario['name']]
                selected = group[check['index'] - 1] if len(group) >= check['index'] else {}
                failures = differences(selected, check['expect'])
                result['checks'].append({'scenario': scenario['name'], 'name': 'admission_' + str(check['index']),
                                         'passed': not failures, 'differences': failures})
        parity = [r for r in arrivals if r['scenario'] == 'transport-parity']
        result['checks'].append({'name': 'transport_parity_bytes_and_decisions', 'passed': len(parity)==2 and
                                 parity[0]['bytes_le']==parity[1]['bytes_le'] and
                                 [r['decision'] for r in parity]==['Accepted','Duplicate']})
        result['admissions'] = len(arrivals)
        result['scenarios'] = len(plan['scenarios'])
        result['commands'] = len(expected)
        result['batch_commands'] = len(batches)
        result['passed'] = sum(check['passed'] for check in result['checks'])
        result['total'] = len(result['checks'])
        result['status'] = 'PASS' if result['passed'] == result['total'] else 'FAIL'
    except Exception as error:
        result['errors'].append({'type': type(error).__name__, 'message': str(error)})
    finally:
        (target / 'result.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items() if key != 'checks'}, ensure_ascii=False))
    return 0 if result['status'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
