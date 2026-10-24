"""tx-02-00: ONE invocation for corpus + owned fault lifecycles + real Authority.

No probing before this entry; no automatic retries. The user prepares the game.
"""
import argparse
import ctypes
from ctypes import wintypes as w
import importlib.util
import json
import shutil
import struct
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

import e2e_physical_input as physical_input

TOOLS=Path(__file__).resolve().parent
ROOT=TOOLS.parent


def load(name,path):
    spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module);return module


def save(path,value):
    Path(path).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def utc_now():
    return datetime.now(timezone.utc).isoformat(timespec='milliseconds').replace('+00:00','Z')


def parse_utc(value):
    if not isinstance(value,str) or not value:
        raise ValueError('missing UTC timestamp')
    parsed=datetime.fromisoformat(value.replace('Z','+00:00'))
    if parsed.tzinfo is None:
        raise ValueError('UTC timestamp has no timezone')
    return parsed.astimezone(timezone.utc)


def continuous_input_steps():
    # 64 bounded one-key steps: sixteen closed W/D/S/A cycles, 200ms each.
    return [dict(keys=[key],hold_ms=200,delay_after_ms=0)
            for _ in range(16) for key in ('W','D','S','A')]


def validate_continuous_input(request,result,expected_pid,expected_created,observed_tick,config):
    errors=[]
    if not isinstance(request,dict):errors.append('missing continuous input request')
    if not isinstance(result,dict):errors.append('missing continuous input result')
    if errors:
        return dict(valid=False,errors=errors,request_tick=None,result_observed_tick=observed_tick),errors
    expected_steps=request.get('steps') or []
    if result.get('request_id')!=request.get('request_id'):
        errors.append('request id mismatch')
    if result.get('is_error') is not False:
        errors.append('physical input tool reported an error')
    if result.get('steps_executed')!=len(expected_steps):
        errors.append(f"steps_executed {result.get('steps_executed')} != {len(expected_steps)}")
    if result.get('pid')!=expected_pid:
        errors.append('game pid mismatch')
    if str(result.get('creation_time_100ns'))!=str(expected_created):
        errors.append('game creation time mismatch')
    expected_hwnd=result.get('hwnd')
    foreground_samples=result.get('foreground_samples')
    if not isinstance(foreground_samples,list) or not foreground_samples:
        errors.append('missing per-step foreground samples')
    elif any(sample.get('foreground_pid')!=expected_pid or
             sample.get('foreground_hwnd')!=expected_hwnd for sample in foreground_samples):
        errors.append('a physical-input step ran while the game was not foreground')
    step_traces=result.get('step_traces')
    if (not isinstance(step_traces,list) or len(step_traces)!=len(expected_steps) or
            any(trace.get('foreground_ok') is not True for trace in step_traces)):
        errors.append('missing or failed per-step foreground traces')
    if result.get('released_on_failure'):
        errors.append('physical input had to release keys through the failure path')
    try:
        requested=parse_utc(request.get('requested_utc'))
        started=parse_utc(result.get('started_utc'))
        finished=parse_utc(result.get('finished_utc'))
        if started<requested:errors.append('tool started before request')
        if finished<started:errors.append('tool finished before it started')
        start_latency=(started-requested).total_seconds()
        duration=(finished-started).total_seconds()
    except Exception as error:
        errors.append(f'timestamp validation: {error}')
        start_latency=None;duration=None
    if len(expected_steps)!=64:
        errors.append(f'expected 64 continuous steps, observed {len(expected_steps)}')
    request_tick=request.get('authority_active_tick')
    if not isinstance(request_tick,int) or request_tick<=0:
        errors.append('request was not issued after a real Authority commit')
    elif request_tick>=config['outage_start']:
        errors.append('continuous input requested after outage start')
    if observed_tick<=config['outage_start']+config['outage_ticks']:
        errors.append('continuous input completed before outage ended')
    validation=dict(valid=not errors,errors=errors,request_tick=request_tick,
        result_observed_tick=observed_tick,outage_start=config['outage_start'],
        outage_end=config['outage_start']+config['outage_ticks'],
        steps_expected=len(expected_steps),steps_executed=result.get('steps_executed'),
        started_utc=result.get('started_utc'),finished_utc=result.get('finished_utc'),
        start_latency_seconds=start_latency,tool_duration_seconds=duration)
    return validation,errors


def current_game_path():
    """Unique live path, inside the final acceptance only; no process mutation."""
    class Entry(ctypes.Structure):
        _fields_=[('size',w.DWORD),('usage',w.DWORD),('pid',w.DWORD),('heap',ctypes.c_size_t),
                  ('module',w.DWORD),('threads',w.DWORD),('parent',w.DWORD),('priority',w.LONG),('flags',w.DWORD),('exe',w.WCHAR*260)]
    k=ctypes.WinDLL('kernel32',use_last_error=True)
    k.CreateToolhelp32Snapshot.argtypes=[w.DWORD,w.DWORD];k.CreateToolhelp32Snapshot.restype=w.HANDLE
    k.Process32FirstW.argtypes=[w.HANDLE,ctypes.POINTER(Entry)];k.Process32FirstW.restype=w.BOOL
    k.Process32NextW.argtypes=[w.HANDLE,ctypes.POINTER(Entry)];k.Process32NextW.restype=w.BOOL
    k.CloseHandle.argtypes=[w.HANDLE];k.CloseHandle.restype=w.BOOL
    k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
    k.QueryFullProcessImageNameW.argtypes=[w.HANDLE,w.DWORD,w.LPWSTR,ctypes.POINTER(w.DWORD)];k.QueryFullProcessImageNameW.restype=w.BOOL
    k.GetProcessTimes.argtypes=[w.HANDLE,ctypes.POINTER(w.FILETIME),ctypes.POINTER(w.FILETIME),ctypes.POINTER(w.FILETIME),ctypes.POINTER(w.FILETIME)];k.GetProcessTimes.restype=w.BOOL
    snapshot=k.CreateToolhelp32Snapshot(2,0)
    if snapshot==ctypes.c_void_p(-1).value:raise OSError(ctypes.get_last_error(),'snapshot')
    pids=[]
    try:
        entry=Entry();entry.size=ctypes.sizeof(entry);ok=k.Process32FirstW(snapshot,ctypes.byref(entry))
        while ok:
            if entry.exe.casefold()=='isaac-ng.exe':pids.append(entry.pid)
            ok=k.Process32NextW(snapshot,ctypes.byref(entry))
    finally:k.CloseHandle(snapshot)
    if len(pids)!=1:raise RuntimeError(f'exactly one prepared game required; observed {pids}')
    process=k.OpenProcess(0x1000,False,pids[0])
    if not process:raise OSError(ctypes.get_last_error(),'OpenProcess query')
    try:
        buffer=ctypes.create_unicode_buffer(32768);length=w.DWORD(len(buffer))
        if not k.QueryFullProcessImageNameW(process,0,buffer,ctypes.byref(length)):raise OSError(ctypes.get_last_error(),'image path')
        created,exited,kernel,user=w.FILETIME(),w.FILETIME(),w.FILETIME(),w.FILETIME()
        if not k.GetProcessTimes(process,ctypes.byref(created),ctypes.byref(exited),ctypes.byref(kernel),ctypes.byref(user)):raise OSError(ctypes.get_last_error(),'process identity')
        return Path(buffer.value),pids[0],(created.dwHighDateTime<<32)|created.dwLowDateTime
    finally:k.CloseHandle(process)


def source_snapshot(target):
    for relative in ['cs-t0/native/include','cs-t0/native/src','cs-t0/native/scenarios','cs-t0/src','docs/refactor/cs-t0/t0-08']:
        shutil.copytree(ROOT/relative,target/relative)
    for path in list((TOOLS/'native').glob('*.vcxproj'))+[TOOLS/name for name in [
        'Cargo.toml','Cargo.lock','accept-protocol-e2e.py','e2e_physical_input.py','report-protocol-e2e.py','protocol-corpus.py',
        'build-protocol-e2e.ps1','run-authority-source-stop-e2e.py','report-authority-source-stop.py','README.md']]+[
        ROOT/'docs/refactor/project-conventions.md',ROOT/'docs/refactor/cs-t0/isaac-cs-t0-task-plan.md']:
        destination=target/path.relative_to(ROOT);destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,destination)


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--build-dir',type=Path,required=True)
    parser.add_argument('--artifact-dir',type=Path,required=True)
    parser.add_argument('--authorization',required=True)
    parser.add_argument('--transaction',default='tx-02-00')
    parser.add_argument('--prior-failure-artifact',type=Path)
    parser.add_argument('--game-exe',type=Path)
    parser.add_argument('--start-delay-seconds',type=float,default=2)
    args=parser.parse_args()
    target=args.artifact_dir.resolve();target.mkdir(parents=True,exist_ok=False)
    result=dict(tx=args.transaction,status='FAIL',automatic_retry=False,hashes='NOT_USED',checks=[],errors=[],live='NOT_RUN')
    process=None;stop_sent=False;controller_exit=None
    continuous_request=None;continuous_validation=None
    try:
        old=load('existing_live_helpers',TOOLS/'run-authority-source-stop-e2e.py')
        analysis=load('protocol_report',TOOLS/'report-protocol-e2e.py')
        if not args.authorization.strip() or not 0<=args.start_delay_seconds<=60:raise ValueError('authorization/start delay')
        source_snapshot(target/'sources')
        build=args.build_dir.resolve();binary_copy=target/'build';binary_copy.mkdir()
        paths=['Rust/IsaacCsT0.exe','Native/bin/IsaacCsT0.Native.dll','Helper/bin/IsaacCsT0.Helper.exe',
               'ProtocolFullScenario/bin/IsaacCsT0.ProtocolFullScenario.exe','build.json',
               'Rust-build.log','Native-build.log','Helper-build.log','ProtocolFullScenario-build.log']
        provenance=[]
        for relative in paths:
            path=build/relative;destination=binary_copy/path.name;shutil.copy2(path,destination)
            provenance.append(dict(path=str(path),bytes=path.stat().st_size,mtime_ns=path.stat().st_mtime_ns))
        save(target/'provenance.json',provenance)
        corpus=load('protocol_fixture_reference',TOOLS/'protocol-corpus.py')
        result['corpus_vectors']=corpus.write_corpus(target/'corpus')
        command=[str(binary_copy/'IsaacCsT0.ProtocolFullScenario.exe'),str(target/'corpus/vectors.txt'),
                 str(target/'offline-checks.jsonl'),str(target/'offline-trace.jsonl'),str(target/'offline-summary.json')]
        save(target/'offline-invocation.json',command)
        with (target/'offline.stdout.log').open('wb') as stdout,(target/'offline.stderr.log').open('wb') as stderr:
            completed=subprocess.run(command,stdout=stdout,stderr=stderr,timeout=120,check=False,creationflags=subprocess.CREATE_NO_WINDOW)
        offline=list(analysis.jsonl(target/'offline-checks.jsonl'))
        result['checks'].append(dict(name='P01-P07.offline-corpus-and-participant-lifecycles',passed=completed.returncode==0 and bool(offline) and all(r['passed'] for r in offline),
                                    detail=dict(exit_code=completed.returncode,passed=sum(r['passed'] for r in offline),total=len(offline))))
        if not result['checks'][-1]['passed']:raise RuntimeError('offline portion failed; no game access or retry')
        if args.prior_failure_artifact:
            # A read-only copy of the real historical failure, within this one
            # acceptance invocation. Never execute/reinject its old controller.
            old_capture=args.prior_failure_artifact.resolve()/'capture'
            fixture=target/'failure-report-fixture';fixture_capture=fixture/'capture';fixture_capture.mkdir(parents=True)
            for name in ['events.jsonl','protocol.jsonl','result.json','observation-metadata.json']:
                shutil.copy2(old_capture/name,fixture_capture/name)
            expected=json.loads((fixture_capture/'result.json').read_text(encoding='utf-8'))
            fixture_checks=analysis.analyze(fixture,{},1,False)
            detail=fixture_checks[0].get('detail',{}) if fixture_checks else {}
            passed=bool(fixture_checks) and not fixture_checks[0]['passed'] and \
                'restoration.json' in detail.get('missing',[]) and detail.get('native',{}).get('error')=='protocol trace overflow' and \
                expected.get('error')=='protocol trace overflow'
            result['checks'].append(dict(name='F01.historical-overflow-missing-restoration-report',passed=passed,detail=fixture_checks))
            if not passed:raise RuntimeError('historical failure reporting failed; no game access or retry')
        game,pid,created=current_game_path()
        if args.game_exe and args.game_exe.resolve()!=game.resolve():raise RuntimeError('prepared game differs from requested executable')
        identity=old.validate_game_pe(game)
        config=dict(version=1,size=64,seed=0x20008,delay_ticks=1,jitter_ticks=1,loss_per_mille=50,
                    duplicate_per_mille=150,reorder_per_mille=150,reserved=0,outage_start=160,outage_ticks=20,reserved_tail=0)
        input_config=dict(schema_version=1,input_lead_ticks=8,future_window_ticks=64,max_input_age_ticks=0,script_drain_budget=16,admission_attempt_budget=258)
        steps=[]
        for tick in range(240):
            x=0 if tick>=176 else (4096 if tick//20%2==0 else -4096)
            steps.append(dict(tick=tick,move_x=x,edge_flags=8 if tick in (20,100,180) else 0))
        save(target/'protocol-config.json',config);save(target/'input-config.json',input_config);save(target/'script.json',dict(steps=steps))
        save(target/'invocation.json',dict(tx=args.transaction,authorization=args.authorization,game_exe=str(game),pid=pid,creation_time_100ns=str(created),python=sys.version,
             game_pe_identity=identity,protocol=config,input_config=input_config,automatic_retry=False,
             controller_creation='CREATE_NO_WINDOW',helper_creation='CREATE_NO_WINDOW',
             real_topology='one real Authority; owned in-memory logical Remote; no Steam/socket/Replica',hashes='NOT_USED'))
        command=[str(binary_copy/'IsaacCsT0.exe'),'coop-active','--game-exe',str(game),'--native-dll',str(binary_copy/'IsaacCsT0.Native.dll'),
                 '--helper',str(binary_copy/'IsaacCsT0.Helper.exe'),'--artifact-dir',str(target/'capture'),'--script',str(target/'script.json'),
                 '--authorization',args.authorization,'--input-config',str(target/'input-config.json'),'--world-record','min-read-v1',
                 '--protocol','t0-v2','--protocol-config',str(target/'protocol-config.json'),'--b-character','native']
        save(target/'controller-command.json',command)
        initial_steps=[dict(keys=[key],hold_ms=150,delay_after_ms=0) for key in ('W','D','S','A')]
        initial_result=physical_input.run_sequence(pid,created,initial_steps)
        result['physical_input_initial']=dict(is_error=initial_result.get('is_error'),
            error=initial_result.get('error'),steps_executed=initial_result.get('steps_executed'),
            hwnd=initial_result.get('hwnd'),started_utc=initial_result.get('started_utc'),
            finished_utc=initial_result.get('finished_utc'))
        ready=dict(initial_request=dict(steps=initial_steps),initial_success=dict(
                       isError=initial_result.get('is_error') is not False,result=initial_result),
                   finished_utc=initial_result.get('finished_utc'),
                   writer='owned e2e_physical_input; one bounded focus check before controller')
        save(target/'physical-input-ready.json',ready)
        save(target/'stage.json',dict(phase='physical-input-ready',hwnd=initial_result.get('hwnd'),
             focused=initial_result.get('is_error') is False,
             instruction='Owned physical input completed the initial focus check; starting controller.'))
        print('PROTOCOL_E2E_OWNED_PHYSICAL_INPUT_READY',flush=True)
        if initial_result.get('is_error') is not False:
            raise RuntimeError(f"initial owned physical input failed: {initial_result.get('error')}")
        time.sleep(args.start_delay_seconds)
        with (target/'controller.stdout.log').open('wb') as stdout:
            process=subprocess.Popen(command,cwd=str(ROOT),stdin=subprocess.PIPE,stdout=stdout,stderr=subprocess.STDOUT,
                                     creationflags=subprocess.CREATE_NO_WINDOW)
            save(target/'stage.json',dict(phase='controller-started',pid=process.pid,
                 physical_input='owned input will run immediately after the first Authority commit'))
            print('PROTOCOL_E2E_CONTROLLER_STARTED',flush=True)
            result['live']='STARTED';started=time.monotonic();event_tail=old.EventTail(target/'capture/events.jsonl');wire_tail=old.EventTail(target/'capture/protocol.jsonl')
            script_complete=False;max_target=0;committed=0;failure=False;reported_active=False
            continuous_request=None;continuous_result=None;continuous_validation=None
            continuous_result_present=False
            while process.poll() is None:
                for row in event_tail.poll():
                    kind=row.get('kind');tick=int(row.get('tick',0))
                    if '__orchestrator_parse_error__' in row or kind==16:failure=True
                    if kind==18 and row.get('ordinal')==1:script_complete=True
                    if kind==8:committed=max(committed,tick)
                if committed and not reported_active:
                    reported_active=True
                    request=dict(request_id=f'authority-{committed}-{time.time_ns()}',
                        requested_utc=utc_now(),authority_active_tick=committed,
                        steps=continuous_input_steps(),
                        instruction='Owned physical input runs immediately after this Authority commit; no external handoff.')
                    continuous_request=request
                    save(target/'physical-input-continuous-request.json',request)
                    save(target/'stage.json',dict(phase='continuous-input-started',completed_tick=committed,
                         request_file=str(target/'physical-input-continuous-request.json'),
                         instruction='Owned 64-step physical input is running now.'))
                    print('PROTOCOL_E2E_CONTINUOUS_INPUT_WINDOW',flush=True)
                    continuous_result=physical_input.run_sequence(pid,created,request['steps'],
                                                                 hwnd=initial_result.get('hwnd'))
                    continuous_result['request_id']=request['request_id']
                    save(target/'physical-input-continuous-result.json',continuous_result)
                    continuous_result_present=True
                    print('PROTOCOL_E2E_CONTINUOUS_INPUT_COMPLETED',flush=True)
                    if continuous_result.get('is_error') is not False:
                        failure=True
                        result['errors'].append(dict(type='ContinuousInputFailure',
                            message=continuous_result.get('error') or 'owned physical input failed',
                            result=continuous_result))
                if continuous_result_present and continuous_validation is None and committed>config['outage_start']+config['outage_ticks']:
                    continuous_validation,errors=validate_continuous_input(
                        continuous_request,continuous_result,pid,created,committed,config)
                    save(target/'physical-input-continuous-validation.json',continuous_validation)
                    result['continuous_input']=continuous_validation
                    if errors:
                        failure=True
                        result['errors'].append(dict(type='ContinuousInputFailure',message='; '.join(errors),
                                                     validation=continuous_validation))
                    else:
                        save(target/'stage.json',dict(phase='continuous-input-completed',completed_tick=committed,
                             request_file=str(target/'physical-input-continuous-request.json'),
                             validation_file=str(target/'physical-input-continuous-validation.json')))
                for row in wire_tail.poll():
                    if row.get('action')==1 and row.get('direction')==0:
                        p=analysis.packet(row['bytes'])
                        if p['kind']==2:
                            count=struct.unpack_from('<H',p['payload'])[0]
                            max_target=max(max_target,struct.unpack_from('<Q',p['payload'],16+64*(count-1)+32)[0])
                done=script_complete and max_target>0 and committed>=max_target+16 and continuous_validation is not None and continuous_validation.get('valid') is True
                if failure or done or time.monotonic()-started>180:
                    process.stdin.write(b'stop\n');process.stdin.flush();stop_sent=True
                    result['normal_stop']=done and not failure;break
                time.sleep(.1)
            try:controller_exit=process.wait(timeout=90)
            except subprocess.TimeoutExpired:
                # Kill only the owned controller; never the game or resident DLL.
                process.kill();process.wait(timeout=30);raise RuntimeError('controller cleanup deadline; game state requires user attention')
        result['controller_exit']=controller_exit
        native_path=target/'capture/result.json'
        native=json.loads(native_path.read_text(encoding='utf-8')) if native_path.is_file() else {}
        if controller_exit!=0:
            result['errors'].append(dict(type='ControllerFailure',message=native.get('controllerError') or native.get('error') or
                native.get('transportFailure') or (f"native error {native['nativeError']}" if native.get('nativeError') else
                'controller exited without a successful capture'),exit_code=controller_exit,
                cleanup_errors=native.get('cleanupErrors',[])))
        after_game,after_pid,after_created=current_game_path()
        result['checks'].append(dict(name='L07.same-game-still-alive',passed=after_game==game and after_pid==pid and after_created==created))
        live=analysis.analyze(target,config,controller_exit,result.get('normal_stop',False))
        result['checks'].extend(live);result['live']='COMPLETED' if controller_exit==0 else 'FAILED'
        result['passed']=sum(r['passed'] for r in result['checks']);result['total']=len(result['checks'])
        result['status']='PASS' if result['passed']==result['total'] and not result['errors'] else 'FAIL'
    except BaseException as error:
        result['errors'].append(dict(type=type(error).__name__,message=str(error)))
        if process is not None and process.poll() is None and not stop_sent:
            try:process.stdin.write(b'stop\n');process.stdin.flush();stop_sent=True;process.wait(timeout=90)
            except (OSError,ValueError,subprocess.TimeoutExpired) as cleanup:result['errors'].append(dict(cleanup=str(cleanup)))
        if process is not None and process.poll() is None:
            process.kill()
            try:process.wait(timeout=30)
            except subprocess.TimeoutExpired:result['errors'].append(dict(cleanup='owned controller did not exit after kill; no game kill or forced DLL unload'))
    finally:
        result['controller_stop_sent']=stop_sent
        result['passed']=sum(r['passed'] for r in result['checks']);result['total']=len(result['checks'])
        save(target/'result.json',result)
    print(json.dumps({k:v for k,v in result.items() if k!='checks'},ensure_ascii=False),flush=True)
    return 0 if result['status']=='PASS' else 1


if __name__=='__main__':raise SystemExit(main())
