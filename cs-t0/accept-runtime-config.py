"""One unified offline acceptance for T0-07 operation 7; no game access."""
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


def run_process(invocation, timeout=60):
    return subprocess.run(invocation, capture_output=True, text=True,
                          timeout=timeout, check=False)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--artifact-dir', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    target = args.artifact_dir.resolve()
    target.mkdir(parents=True, exist_ok=False)
    result = {
        'tx': 'tx-01-96',
        'scope': 'T0-07 operation 7 CLI/manifest runtime input configuration and Native IPC v5',
        'status': 'FAIL',
        'game_access': False,
        'native_runtime': 'NOT_RUN',
        'checks': [],
        'errors': [],
    }
    try:
        build = args.build_dir.resolve()
        plan = json.loads((root / 'cs-t0/testdata/tx-01-96-input-runtime-config.json')
                          .read_text(encoding='utf-8'))
        names = [
            'cs-t0/accept-runtime-config.py',
            'cs-t0/build-runtime-config.ps1',
            'cs-t0/testdata/tx-01-96-input-runtime-config.json',
            'cs-t0/src/main.rs',
            'cs-t0/src/coop_active.rs',
            'cs-t0/Cargo.toml',
            'cs-t0/Cargo.lock',
            'cs-t0/native/scenarios/runtime_config_main.cpp',
            'cs-t0/native/IsaacCsT0.RuntimeConfigScenario.vcxproj',
            'cs-t0/native/IsaacCsT0.Native.vcxproj',
            'cs-t0/native/include/isaac_cs_t0/input_core.hpp',
            'cs-t0/native/src/input_core.cpp',
            'cs-t0/native/include/isaac_cs_t0/coop_input.hpp',
            'cs-t0/native/src/coop_input.cpp',
            'cs-t0/native/include/isaac_cs_t0/authority_tick.hpp',
            'cs-t0/native/src/authority_tick.cpp',
            'cs-t0/native/include/isaac_cs_t0/authority_tick_pipeline.hpp',
            'cs-t0/native/src/authority_tick_pipeline.cpp',
            'cs-t0/native/include/isaac_cs_t0/coop_active_ipc.hpp',
            'cs-t0/native/src/coop_active_runtime.cpp',
            'docs/refactor/cs-t0/t0-07/tx-01-96-runtime-config.md',
            'docs/refactor/cs-t0/t0-07/tx-01-96-static-review.md',
        ]
        for name in names:
            destination = target / 'sources' / name
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(root / name, destination)

        binaries = target / 'build'
        binaries.mkdir()
        provenance = []
        for relative in [
            'Rust/bin/IsaacCsT0.exe',
            'Native/bin/IsaacCsT0.Native.dll',
            'RuntimeConfigScenario/bin/IsaacCsT0.RuntimeConfigScenario.exe',
            'Rust-build.log',
            'Native-build.log',
            'RuntimeConfigScenario-build.log',
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

        rust_exe = binaries / 'IsaacCsT0.exe'
        scenario_exe = binaries / 'IsaacCsT0.RuntimeConfigScenario.exe'
        configs_dir = target / 'configs'
        configs_dir.mkdir()
        resolved = {}
        for case in plan['rust_cases']:
            name = case['name']
            invocation = [str(rust_exe), 'coop-input-config']
            if 'manifest' in case:
                manifest = configs_dir / f'{name}.manifest.json'
                manifest.write_text(json.dumps(case['manifest'], indent=2) + '\n', encoding='utf-8')
                invocation += ['--manifest', str(manifest)]
            invocation += case.get('cli', [])
            output = configs_dir / f'{name}.output.json'
            invocation += ['--output', str(output)]
            started = time.monotonic()
            process = run_process(invocation, timeout=30)
            elapsed = time.monotonic() - started
            (configs_dir / f'{name}.stdout.log').write_text(process.stdout, encoding='utf-8')
            (configs_dir / f'{name}.stderr.log').write_text(process.stderr, encoding='utf-8')
            expect_exit = case.get('expect_exit', 0)
            failures = {}
            if process.returncode != expect_exit:
                failures['exit_code'] = {'expected': expect_exit, 'actual': process.returncode}
            for text in case.get('stderr_contains', []):
                if text not in process.stderr:
                    failures[f'stderr_contains:{text}'] = {'expected': True,
                                                           'actual': process.stderr}
            if expect_exit == 0:
                if not output.exists():
                    failures['output_exists'] = {'expected': True, 'actual': False}
                else:
                    data = json.loads(output.read_text(encoding='utf-8'))
                    resolved[name] = data
                    failures.update(differences(data, case.get('expect', {})))
            elif output.exists():
                failures['unexpected_output'] = {'expected': False, 'actual': True}
            result['checks'].append({
                'kind': 'rust-config',
                'case': name,
                'invocation': invocation,
                'elapsed_seconds': elapsed,
                'passed': not failures,
                'differences': failures,
            })

        transcript = []
        expected = []
        for scene in plan['scenarios']:
            source_name = scene['config_case']
            config = resolved[source_name]['effective']
            transcript.append('scenario {} {} {} {} {} {} {}'.format(
                scene['name'], scene.get('initial_tick', 0), config['input_lead_ticks'],
                config['future_window_ticks'], config['max_input_age_ticks'],
                config['script_drain_budget'], config['admission_attempt_budget']))
            for index, item in enumerate(scene['steps'], 1):
                transcript.append(item['command'])
                expected.append((scene['name'], index, item))
        commands = target / 'commands.txt'
        commands.write_text('\n'.join(transcript) + '\n', encoding='utf-8')
        invocation = [str(scenario_exe), str(commands), str(target / 'events.jsonl')]
        result['scenario_invocation'] = invocation
        started = time.monotonic()
        process = run_process(invocation, timeout=60)
        result['scenario_exit_code'] = process.returncode
        result['scenario_elapsed_seconds'] = time.monotonic() - started
        (target / 'scenario.stdout.log').write_text(process.stdout, encoding='utf-8')
        (target / 'scenario.stderr.log').write_text(process.stderr, encoding='utf-8')
        actual = [json.loads(line) for line in
                  (target / 'events.jsonl').read_text(encoding='utf-8').splitlines()]
        result['checks'].append({
            'kind': 'scenario-transcript',
            'passed': process.returncode == 0 and len(actual) == len(expected),
            'exit_code': process.returncode,
            'expected_rows': len(expected),
            'actual_rows': len(actual),
        })
        for index, (name, step_index, item) in enumerate(expected):
            row = actual[index] if index < len(actual) else {}
            wanted = dict(item['expect'], scenario=name, step=step_index)
            failures = differences(row, wanted)
            result['checks'].append({
                'kind': 'scenario-step',
                'scenario': name,
                'step': step_index,
                'command': item['command'],
                'passed': not failures,
                'differences': failures,
            })
        result['rust_cases'] = len(plan['rust_cases'])
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
