"""Single offline production-chain acceptance for T0-07 operation 5; no game access."""
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
    result = {'tx': 'tx-01-93', 'scope': 'T0-07 operation 5 offline production Session/Clock/codec/Participant lifecycles',
              'status': 'FAIL', 'game_access': False, 'native_runtime': 'NOT_RUN', 'checks': [], 'errors': []}
    try:
        build = args.build_dir.resolve()
        plan = json.loads((root / 'cs-t0/testdata/tx-01-93-tick-pipeline.json').read_text(encoding='utf-8'))
        names = ['cs-t0/accept-tick-pipeline.py', 'cs-t0/build-tick-pipeline.ps1',
                 'cs-t0/testdata/tx-01-93-tick-pipeline.json', 'cs-t0/native/scenarios/tick_pipeline_main.cpp',
                 'cs-t0/native/IsaacCsT0.TickPipelineScenario.vcxproj', 'cs-t0/native/IsaacCsT0.Native.vcxproj',
                 'cs-t0/native/include/isaac_cs_t0/authority_tick_pipeline.hpp', 'cs-t0/native/src/authority_tick_pipeline.cpp',
                 'cs-t0/native/include/isaac_cs_t0/world_record.hpp', 'cs-t0/native/src/world_record.cpp',
                 'cs-t0/native/include/isaac_cs_t0/authority_tick.hpp', 'cs-t0/native/src/authority_tick.cpp',
                 'cs-t0/native/include/isaac_cs_t0/authority_session.hpp', 'cs-t0/native/src/authority_session.cpp',
                 'cs-t0/native/include/isaac_cs_t0/coop_input.hpp', 'cs-t0/native/src/coop_input.cpp',
                 'cs-t0/native/include/isaac_cs_t0/input_core.hpp', 'cs-t0/native/src/input_core.cpp',
                 'cs-t0/native/include/isaac_cs_t0/loop_runtime.hpp', 'cs-t0/native/src/loop_runtime.cpp',
                 'cs-t0/native/src/coop_active_runtime.cpp', 'cs-t0/native/include/isaac_cs_t0/coop_active_ipc.hpp',
                 'docs/refactor/cs-t0/t0-07/tx-01-93-tick-pipeline.md',
                 'docs/refactor/cs-t0/t0-07/tx-01-93-static-review.md']
        for name in names:
            destination = target / 'sources' / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(root / name, destination)
        binaries = target / 'build'
        binaries.mkdir()
        provenance = []
        for relative in ['Native/bin/IsaacCsT0.Native.dll', 'TickPipelineScenario/bin/IsaacCsT0.TickPipelineScenario.exe',
                         'Native-build.log', 'TickPipelineScenario-build.log', 'toolchain.json']:
            source = build / relative
            shutil.copy2(source, binaries / source.name)
            provenance.append({'path': str(source), 'bytes': source.stat().st_size, 'mtime_ns': source.stat().st_mtime_ns})
        (target / 'binary-provenance.json').write_text(json.dumps({'method': 'paths, lengths and timestamps; no digests',
                                                                  'files': provenance}, indent=2) + '\n', encoding='utf-8')
        transcript, expected = [], []
        for scenario in plan['scenarios']:
            transcript.append('scenario ' + scenario['name'] + ' ' + scenario.get('initial_tick', '0') + ' ' + str(scenario.get('budget', 258)))
            for index, step in enumerate(scenario['steps'], 1):
                transcript.append(step['command'])
                expected.append((scenario['name'], index, step))
        commands = target / 'commands.txt'
        commands.write_text('\n'.join(transcript) + '\n', encoding='utf-8')
        invocation = [str(binaries / 'IsaacCsT0.TickPipelineScenario.exe'), str(commands),
                      str(target / 'events.jsonl'), str(target / 'batch-events.jsonl')]
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
        result['checks'].append({'name': 'complete_cycles', 'passed': len(batches) == sum(scene.get('batch_count', 0) for scene in plan['scenarios']), 'actual': len(batches)})
        for scene in plan['scenarios']:
            if 'batch_count' not in scene:
                continue
            group = [row for row in batches if row['scenario'] == scene['name']]
            failures = []
            for index, row in enumerate(group, 1):
                failure = differences(row, {'index': index, 'phase': 'Published', 'completed': str(index), 'body_calls': str(index),
                                            'publications': str(index), 'a.edge_count': '1', 'a.processed': str(index),
                                            'b.submit': '0', 'receipt.published': True})
                if failure: failures.append({'index': index, 'differences': failure})
            result['checks'].append({'scenario': scene['name'], 'name': 'ordered_complete_lifecycles',
                                     'passed': len(group) == scene['batch_count'] and not failures, 'failures': failures})
        result['scenarios'] = len(plan['scenarios'])
        result['commands'] = len(expected)
        result['cycle_records'] = len(batches)
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
