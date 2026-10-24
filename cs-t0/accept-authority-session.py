"""One offline acceptance invocation for T0-07 operation 1; no game access."""
import argparse
import json
import shutil
import subprocess
import time
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--artifact-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    target = args.artifact_dir.resolve()
    target.mkdir(parents=True, exist_ok=False)
    result = {'tx': 'tx-01-89', 'scope': 'T0-07 operation 1 offline production session/input lifecycles',
              'status': 'FAIL', 'game_access': False, 'checks': [], 'errors': [],
              'runtime_hook_join_acceptance': 'NOT_RUN'}
    try:
        source = root / 'cs-t0/testdata/tx-01-89-authority-session.json'
        plan = json.loads(source.read_text(encoding='utf-8'))
        frozen = target / 'sources'
        paths = ['cs-t0/accept-authority-session.py', 'cs-t0/testdata/tx-01-89-authority-session.json',
                 'cs-t0/native/scenarios/authority_session_main.cpp',
                 'cs-t0/native/IsaacCsT0.AuthoritySessionScenario.vcxproj',
                 'cs-t0/native/include/isaac_cs_t0/authority_session.hpp',
                 'cs-t0/native/src/authority_session.cpp',
                 'cs-t0/native/include/isaac_cs_t0/coop_input.hpp', 'cs-t0/native/src/coop_input.cpp',
                 'cs-t0/native/include/isaac_cs_t0/input_core.hpp', 'cs-t0/native/src/input_core.cpp',
                 'cs-t0/native/src/coop_active_runtime.cpp', 'cs-t0/native/include/isaac_cs_t0/coop_active_ipc.hpp',
                 'docs/refactor/cs-t0/t0-07/tx-01-89-authority-session.md']
        for name in paths:
            destination = frozen / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(root / name, destination)
        binary = target / args.binary.name
        shutil.copy2(args.binary, binary)
        transcript = []
        expected = []
        for scenario in plan['scenarios']:
            transcript.append('scenario ' + scenario['name'])
            for index, step in enumerate(scenario['steps'], 1):
                transcript.append(step['command'])
                expected.append((scenario['name'], index, step))
        commands = target / 'commands.txt'
        commands.write_text('\n'.join(transcript) + '\n', encoding='utf-8')
        invocation = [str(binary), str(commands), str(target / 'events.jsonl')]
        start = time.monotonic()
        with (target / 'stdout.log').open('wb') as stdout, (target / 'stderr.log').open('wb') as stderr:
            process = subprocess.run(invocation, stdout=stdout, stderr=stderr, timeout=60, check=False)
        result['invocation'] = invocation
        result['exit_code'] = process.returncode
        result['elapsed_seconds'] = time.monotonic() - start
        actual = [json.loads(line) for line in (target / 'events.jsonl').read_text(encoding='utf-8').splitlines()]
        result['checks'].append({'name': 'complete_transcript', 'passed': process.returncode == 0 and len(actual) == len(expected),
                                 'expected': len(expected), 'actual': len(actual)})
        for index, (name, step_index, step) in enumerate(expected):
            row = actual[index] if index < len(actual) else {}
            failures = {key: {'expected': value, 'actual': row.get(key)} for key, value in
                        dict(step['expect'], scenario=name, step=step_index).items() if row.get(key) != value}
            result['checks'].append({'scenario': name, 'step': step_index, 'command': step['command'],
                                     'passed': not failures, 'differences': failures})
        result['scenarios'] = len(plan['scenarios'])
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
