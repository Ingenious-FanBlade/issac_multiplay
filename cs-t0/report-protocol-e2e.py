"""Read-only packet/native pairing; called inside the single E2E invocation."""
import collections
import importlib.util
import json
import struct
import zlib
from pathlib import Path


def jsonl(path):
    with Path(path).open(encoding='utf-8') as stream:
        for line in stream:
            if line.strip():
                yield json.loads(line)


def load_module(name, path):
    spec=importlib.util.spec_from_file_location(name,path)
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def packet(raw):
    raw=bytes.fromhex(raw) if isinstance(raw,str) else raw
    if len(raw)<64 or len(raw)>1200 or raw[:4]!=b'ICT8':
        raise ValueError('packet bounds/magic')
    version,kind,header,role,flags=struct.unpack_from('<HHHHI',raw,4)
    sender,seq,size,r0,crc,r1=struct.unpack_from('<QQIIII',raw,32)
    copy=bytearray(raw);copy[56:60]=bytes(4)
    if version!=2 or header!=64 or flags or r0 or r1 or size!=len(raw)-64 or zlib.crc32(copy)!=crc:
        raise ValueError('packet version/length/reserved/CRC')
    return dict(raw=raw,kind=kind,role=role,sender=sender,sequence=seq,session=raw[16:32],payload=raw[64:])


def transport_checks(rows, config):
    errors=[];sent={};pending=[];delivered=[];active=None;closed=0;rng=config['seed'];mask=2**64-1
    def random64():
        nonlocal rng
        rng^=(rng<<13)&mask;rng^=rng>>7;rng^=(rng<<17)&mask;rng&=mask
        return rng
    for index,row in enumerate(rows):
        action=int(row['action']);token=int(row['token']);now=int(row['tick']);direction=int(row['direction'])
        if action!=8 and active is not None:
            if active['records']:errors.append(['missing-admissions',index,len(active['records'])])
            active=None
        if closed:errors.append(['trace-after-close',index])
        if action==1:
            p=packet(row['bytes'])
            if token in sent:errors.append(['duplicate-send-token',index])
            reliable=row['decision']==1
            outage=not reliable and config['outage_ticks'] and config['outage_start']<=now<config['outage_start']+config['outage_ticks']
            lost=bool(outage) or (not reliable and random64()%1000<config['loss_per_mille'])
            if lost:
                expected=[];drop=2 if outage else 1
            else:
                duplicate=not reliable and random64()%1000<config['duplicate_per_mille']
                jitter=0 if reliable else random64()%(1+config['jitter_ticks'])
                reorder=not reliable and random64()%1000<config['reorder_per_mille']
                delay=0 if reliable else config['delay_ticks']+jitter+(4 if reorder else 0)
                expected=[delay,delay+1] if duplicate else [delay];drop=0
            sent[token]=dict(row=row,packet=p,expected=expected,drop=drop,enqueues=[],drops=[],outcomes=[])
        elif action==3:
            item=sent.get(token)
            if not item or any(item['row'][key]!=row[key] for key in ('bytes','direction','generation','tick')):
                errors.append(['unknown-enqueue-or-identity',index]);continue
            copy=len(item['enqueues']);item['enqueues'].append(row['decision'])
            entry=dict(token=token,direction=direction,due=now+row['decision'],copy=copy,bytes=row['bytes'],generation=row['generation'])
            pending.append(entry)
        elif action==2:
            item=sent.get(token)
            if not item or any(item['row'][key]!=row[key] for key in ('bytes','direction','generation')):
                errors.append(['unknown-drop-or-identity',index]);continue
            if row['decision'] in (1,2):
                item['drops'].append(row['decision'])
                if now!=item['row']['tick']:errors.append(['loss-tick-differs-from-send',index])
            elif row['decision']==3:
                matches=[p for p in pending if p['token']==token]
                if matches:pending.remove(min(matches,key=lambda p:(p['due'],p['copy'])))
                else:errors.append(['clear-without-pending',index])
            else:errors.append(['unknown-drop-reason',index])
        elif action==4:
            ready=[p for p in pending if p['direction']==direction and p['due']<=now]
            expected=min(ready,key=lambda p:(p['due'],p['token'],p['copy'])) if ready else None
            if not expected or expected['token']!=token or expected['bytes']!=row['bytes'] or expected['generation']!=row['generation']:
                errors.append(['delivery-order-or-identity',index])
            if expected:
                pending.remove(expected);delivered.append(dict(row=row,copy=expected['copy']))
        elif action in (5,6):
            match=next((d for d in delivered if all(d['row'][key]==row[key] for key in ('token','tick','direction','generation','bytes'))),None)
            if match is None:errors.append(['receive-without-delivery',index]);continue
            delivered.remove(match);sent[token]['outcomes'].append(row)
            if (action==5)!=(row['decision']==0):errors.append(['receive-result',index])
            p=sent[token]['packet']
            if action==5 and direction==0 and p['kind']==2:
                count=struct.unpack_from('<H',p['payload'])[0]
                if not 1<=count<=4 or len(p['payload'])!=16+64*count:errors.append(['input-record-bounds',index])
                active=dict(token=token,tick=now,generation=row['generation'],records=[p['payload'][16+64*n:80+64*n] for n in range(count)])
        elif action==8:
            raw=bytes.fromhex(row['bytes'])
            if active is None or not active['records'] or direction!=0 or any(active[key]!=row[key] for key in ('token','tick','generation')):
                errors.append(['admission-without-input-receive',index])
            elif active['records'].pop(0)!=raw:errors.append(['admission-bytes-differ-from-wire',index])
        elif action==7:errors.append(['transport-full',index])
        elif action==9:closed+=1
        else:errors.append(['unknown-trace-action',index])
    if active is not None and active['records']:errors.append(['missing-final-admissions',len(active['records'])])
    for token,item in sent.items():
        if item['expected']!=item['enqueues'] or item['drops']!=([item['drop']] if item['drop'] else []):errors.append(['fault-model',token])
    if closed!=1 or not rows or rows[-1]['action']!=9 or pending or delivered:
        errors.append(['not-closed-or-unpaired-delivery',len(pending),len(delivered),closed])
    return errors,sent


def state_bytes(state):
    return struct.pack('<HHIIhhhhI',*(state[key] for key in ('version','encoded_length','held_flags','edge_flags','move_x','move_y','aim_x','aim_y','reserved')))


def submitted_bytes(row):
    return struct.pack('<4I',*row['before'])+struct.pack('<QQQ',row['after'][0]|row['after'][1]<<32,row['command'],row['target'])+state_bytes(row['state'])


def analyze(root, config, controller_exit, normal_stop):
    root=Path(root);checks=[]
    def check(name,passed,detail=None):checks.append(dict(name=name,passed=bool(passed),detail=detail))
    capture=root/'capture'
    required=['events.jsonl','protocol.jsonl','result.json','restoration.json','observation-metadata.json','world/captures.jsonl']
    missing=[name for name in required if not (capture/name).is_file()]
    if missing:
        native_path=capture/'result.json'
        native=json.loads(native_path.read_text(encoding='utf-8')) if native_path.is_file() else {}
        check('L07.required-capture-artifacts',False,dict(missing=missing,controller_exit=controller_exit,native=native))
        (root/'live-checks.json').write_text(json.dumps(checks,indent=2)+'\n',encoding='utf-8')
        return checks
    check('L07.required-capture-artifacts',True)
    events=list(jsonl(capture/'events.jsonl'));rows=list(jsonl(capture/'protocol.jsonl'))
    native=json.loads((capture/'result.json').read_text(encoding='utf-8'))
    restore=json.loads((capture/'restoration.json').read_text(encoding='utf-8'))
    metadata=json.loads((capture/'observation-metadata.json').read_text(encoding='utf-8'))
    check('L01.process-and-config',metadata.get('coop_version')==6 and native.get('nativeError')==0)
    check('L07.controller',controller_exit==0 and normal_stop and native.get('status')=='CAPTURE_CLOSED',native)
    check('L07.capture-error-preserved-and-cleanup-complete',not native.get('controllerError') and not native.get('cleanupErrors'),
          dict(controller_error=native.get('controllerError'),cleanup_errors=native.get('cleanupErrors')))
    check('L02.protocol-ring',bool(rows) and [r['sequence'] for r in rows]==list(range(1,len(rows)+1)))
    counters=native.get('protocol',{}).get('counters',{})
    check('L02.protocol-ring-drained-without-loss',counters.get('lost')==0 and
          counters.get('produced')==counters.get('consumed')==len(rows),counters)
    errors,sent=transport_checks(rows,config)
    check('L03.packet-pairing-and-deterministic-fault-model',not errors,errors[:20])
    counts=collections.Counter((r['action'],r['decision']) for r in rows)
    check('L04.loss-and-outage',counts[(2,1)]>0 and counts[(2,2)]>0,dict(random_loss=counts[(2,1)],outage_loss=counts[(2,2)]))
    check('L04.duplicate-and-delay',any(len(v['enqueues'])==2 for v in sent.values()) and any(any(d>2 for d in v['enqueues']) for v in sent.values()))
    received=[packet(r['bytes']) for r in rows if r['action']==5]
    check('L02.handshake',sum(p['kind']==1 for p in received)==2 and any(p['kind']==3 and struct.unpack_from('<H',p['payload'],70)[0]==1 for p in received))
    submits=[r for r in events if r['kind']==4 and r['ordinal']==1]
    admissions=[r for r in rows if r['action']==8]
    submitted=[(submitted_bytes(r),r['after'][2]|r['after'][3]<<32,r['tick'],r['decision']) for r in submits]
    admitted=[(bytes.fromhex(r['bytes']),r['generation'],r['tick'],r['decision']) for r in admissions]
    check('L03.every-B-submit-came-from-wire',submitted==admitted and bool(submits) and\
          all(r['generation']==(r['after'][2]|r['after'][3]<<32) for r in submits),dict(submits=len(submits),wire_admissions=len(admissions)))
    accepted=[r for r in submits if r['decision']==1]
    check('L03.accepted-and-deduplicated',len(accepted)>80 and any(r['decision']==2 for r in submits),dict(accepted=len(accepted),duplicates=sum(r['decision']==2 for r in submits)))
    commit_rows=[r for r in events if r['kind']==8]
    receipt_rows=[r for r in events if r['kind']==34 and r['ordinal']==1]
    commits={r['tick']:r for r in commit_rows}
    receipts={r['tick']:r for r in receipt_rows}
    check('L06.unique-commit-and-receipt-per-tick',len(commits)==len(commit_rows) and len(receipts)==len(receipt_rows))
    a_resolve={r['tick']:r for r in events if r['kind']==5 and r['ordinal']==0}
    b_resolve={r['tick']:r for r in events if r['kind']==5 and r['ordinal']==1}
    availability={r['tick']:r for r in events if r['kind']==35 and r['ordinal']==1}
    session=bytes.fromhex(metadata['session']);session_errors=[];rejected=0;rejection_mask=0;session_state=0;commit_progress={}
    for row in events:
        if row['kind']==31:session_state=row['decision']
        if row['kind']==4 and row['ordinal']==1 and row['decision']>=3:
            rejected+=1;rejection_mask|=1<<row['decision']
        if row['kind']==8:commit_progress[row['tick']]=(rejected,rejection_mask,session_state)
    for item in sent.values():
        p=item['packet'];direction=item['row']['direction']
        if p['session']!=session or p['role']!=(2 if direction==0 else 1) or p['sender']!=(2 if direction==0 else 1):
            session_errors.append(item['row']['token'])
    check('L02.frozen-session-and-sender',not session_errors,session_errors[:20])
    ack_errors=[];state_errors=[];state_identity={};ack_ticks=[];state_ticks=[]
    for item in sent.values():
        p=item['packet']
        if p['kind']==3 and struct.unpack_from('<H',p['payload'],70)[0]==2:
            tick,client,acc,proc,proc_tick,source,gen,rejected=struct.unpack_from('<8Q',p['payload'])
            mask,session_state=struct.unpack_from('<IH',p['payload'],64);ack_ticks.append(tick)
            r=receipts.get(tick)
            if not r or tick not in commits or r['command']!=client or r['target']!=proc or r['generation']!=gen or\
               (r['before'][0]|r['before'][1]<<32)!=acc or (r['before'][2]|r['before'][3]<<32)!=proc_tick or\
               (r['after'][0]|r['after'][1]<<32)!=source or (rejected,mask,session_state)!=commit_progress.get(tick) or\
               item['row']['tick']!=tick:ack_errors.append(tick)
        if p['kind']==4:
            tick,run,room,attempt,captured=struct.unpack_from('<5Q',p['payload'],32)
            schema,phase,status,reason,count=struct.unpack_from('<5H',p['payload'],72)
            r=commits.get(tick);state_ticks.append(tick)
            actual_attempt=(r['before'][2]|r['before'][3]<<32) if r else None
            actual_capture=(r['after'][2]|r['after'][3]<<32) if r else None
            expected_reason=(2 if r['detail']==2 else r['after'][1] or 19) if r else None
            token=struct.unpack_from('<Q',p['payload'],88)[0]
            state_identity[tick]=(run,room,attempt,captured,token)
            if not r or not run or not room or attempt!=actual_attempt or schema!=1 or status!=2 or count!=0 or\
               captured!=actual_capture or phase not in (1,2) or (phase==2)!=(reason==2) or reason!=expected_reason or\
               item['row']['tick']!=tick:state_errors.append(tick)
    expected_ticks=collections.Counter(r['tick'] for r in commit_rows)
    check('L06.wire-ack-matches-every-real-receipt',not ack_errors and bool(receipts) and set(receipts)==set(commits) and collections.Counter(ack_ticks)==expected_ticks,ack_errors[:20])
    check('L06.explicit-state-gaps-match-every-real-commit',not state_errors and bool(commits) and collections.Counter(state_ticks)==expected_ticks,state_errors[:20])
    accepted_ack=[p for p in received if p['kind']==3 and struct.unpack_from('<H',p['payload'],70)[0]==2]
    accepted_state=[p for p in received if p['kind']==4]
    check('L06.remote-accepts-both-publication-lanes',len(accepted_ack)>10 and len(accepted_state)>10,dict(ack=len(accepted_ack),state=len(accepted_state)))
    startup_errors=[];startups=[r for r in events if r['kind']==22]
    source_inputs={};source_origin={}
    for item in sent.values():
        p=item['packet']
        if p['kind']==2:
            count=struct.unpack_from('<H',p['payload'])[0]
            for n in range(count):
                raw=p['payload'][16+64*n:80+64*n];seq=struct.unpack_from('<Q',raw,24)[0]
                if seq in source_inputs and source_inputs[seq]!=raw:startup_errors.append(['source-conflict',seq])
                source_inputs[seq]=raw
                source_origin.setdefault(seq,(item['row']['generation'],item['row']['tick']))
    for row in startups:
        nominal_seq=row['before'][0]|row['before'][1]<<32;nominal_target=row['before'][2]|row['before'][3]<<32
        if row['target']!=row['tick']+1 or nominal_target!=row['tick']+8:startup_errors.append(['startup-target',row['ordinal']])
        if row['ordinal']==1:
            first=source_inputs.get(row['command']);nominal=source_inputs.get(nominal_seq);levels=dict(row['state']);levels['edge_flags']=0
            if first is None or nominal is None or first[40:]!=state_bytes(row['state']) or nominal[40:]!=state_bytes(levels) or\
               struct.unpack_from('<Q',first,32)[0]!=row['target'] or struct.unpack_from('<Q',nominal,32)[0]!=nominal_target or\
               source_origin.get(row['command'])!=(row['generation'],row['tick']) or source_origin.get(nominal_seq)!=(row['generation'],row['tick']):
                startup_errors.append(['startup-wire',row['generation']])
    check('L03.symmetric-startup-intent-without-late-retarget',not startup_errors and {r['ordinal'] for r in startups}=={0,1} and\
          len({(r['ordinal'],r['generation']) for r in startups})==len(startups),startup_errors)
    outage=range(config['outage_start']+10,config['outage_start']+config['outage_ticks'])
    neutral=[t for t in outage if t in availability and not(availability[t]['detail']&1) and availability[t]['detail']&2]
    zero=lambda s:all(s.get(k,0)==0 for k in ['held_flags','edge_flags','move_x','move_y','aim_x','aim_y'])
    check('L04.B-neutral-during-outage',len(neutral)>=3 and all(t in b_resolve and zero(b_resolve[t]['state']) for t in neutral),neutral)
    ready_path=root/'physical-input-ready.json'
    try:
        ready=json.loads(ready_path.read_text(encoding='utf-8'))
        initial=ready.get('initial_success',{}).get('result',{})
        initial_samples=initial.get('foreground_samples') or []
        initial_traces=initial.get('step_traces') or []
        initial_hwnd=initial.get('hwnd')
        initial_ok=(ready.get('initial_success',{}).get('isError') is False and
                    initial.get('is_error') is False and initial.get('steps_executed')==4 and
                    bool(initial_samples) and
                    all(s.get('foreground_pid')==metadata.get('game_pid') and
                        s.get('foreground_hwnd')==initial_hwnd for s in initial_samples) and
                    len(initial_traces)==4 and
                    all(t.get('foreground_ok') is True for t in initial_traces) and
                    not initial.get('released_on_failure'))
        initial_detail=dict(steps_executed=initial.get('steps_executed'),hwnd=initial_hwnd,
                            foreground_samples=len(initial_samples),foreground_ok=initial_ok,
                            error=initial.get('error'))
    except Exception as error:
        initial_ok=False;initial_detail=dict(error=f'missing/invalid initial physical input: {error}')
    check('L04.physical-input-initial-focus',initial_ok,initial_detail)
    validation_path=root/'physical-input-continuous-validation.json'
    try:
        continuous_validation=json.loads(validation_path.read_text(encoding='utf-8'))
    except Exception as error:
        continuous_validation=dict(valid=False,errors=[f'missing/invalid validation: {error}'])
    timing_ok=isinstance(continuous_validation.get('start_latency_seconds'),(int,float)) and\
        0<=continuous_validation['start_latency_seconds']<=5 and\
        isinstance(continuous_validation.get('tool_duration_seconds'),(int,float)) and\
        continuous_validation['tool_duration_seconds']>0
    check('L04.continuous-input-window',
          continuous_validation.get('valid') is True and timing_ok and
          continuous_validation.get('steps_executed')==continuous_validation.get('steps_expected')==64 and
          isinstance(continuous_validation.get('request_tick'),int) and
          0<continuous_validation['request_tick']<config['outage_start'] and
          continuous_validation.get('result_observed_tick',0)>config['outage_start']+config['outage_ticks'],
          continuous_validation)
    check('L04.A-and-clock-continue',all(t in commits and t in a_resolve for t in outage) and any(not zero(a_resolve[t]['state']) for t in outage if t in a_resolve))
    a_samples=[r for r in events if r['kind']==17 and r['ordinal']==0 and r.get('query')==2]
    foreground=[r for r in a_samples if r['boolean']==1 and r['action']==metadata['game_pid']]
    check('L04.A-native-source-retains-game-foreground',bool(a_samples) and len(foreground)==len(a_samples),
          dict(samples=len(a_samples),foreground_samples=len(foreground),
               nonzero_movement=sum(bool(r['state']['move_x'] or r['state']['move_y']) for r in a_samples)))
    recovery=[t for t,r in availability.items() if t>config['outage_start']+config['outage_ticks']+8 and r['detail']&1]
    check('L04.B-recovers-with-fresh-input',len(recovery)>15)
    edge_inputs=[r for r in accepted if r['state']['edge_flags']&8]
    edge_resolve=[r for r in b_resolve.values() if r['state']['edge_flags']&8]
    expected=collections.Counter(r['target'] for r in edge_inputs)
    actual=collections.Counter(r['tick'] for r in edge_resolve)
    check('L05.edge-accepted-to-resolve-once',bool(expected) and expected==actual and all(v==1 for v in actual.values()),dict(accepted=len(edge_inputs),resolved=len(edge_resolve)))
    progress=[r for r in events if r['kind']==33 and r['ordinal']==1 and r['query']==7]
    processed=max(((r['before'][2]|r['before'][3]<<32) for r in progress),default=0)
    check('L05.native-processed-edge-count',processed==len(edge_inputs),processed)
    helper=load_module('old_position_reader',Path(__file__).with_name('report-authority-source-stop.py'))
    positions={};world_counts=collections.Counter();identity_errors=[];identity_ticks=[]
    for row in jsonl(capture/'world'/'captures.jsonl'):
        if row.get('record') not in ('capture','gap'):continue
        world_counts['gap' if row['record']=='gap' else 'complete' if row.get('complete') else 'partial']+=1
        if row.get('authority_tick') is None:continue
        tick=int(row['authority_tick']);value=helper.player_position(row,0)
        if tick in state_identity:
            actual=(int(row['run_epoch']),int(row['room_epoch']),int(row['attempt_seq']),int(row['capture_seq'] or 0),int(row['outer_token']))
            if actual!=state_identity[tick]:identity_errors.append(tick)
            identity_ticks.append(tick)
        if value is not None:positions[tick]=value
    observed=[positions[t] for t in outage if t in positions]
    check('L04.A-real-partial-position-change',len(set(observed))>1,dict(samples=len(observed),world_records=dict(world_counts)))
    check('L06.world-observation-identity',bool(state_identity) and not identity_errors and collections.Counter(identity_ticks)==collections.Counter(state_identity.keys()),dict(matches=len(identity_ticks),expected=len(state_identity),errors=identity_errors[:20]))
    check('L07.no-failure-and-stable-lifecycle',not any(r['kind']==16 for r in events) and not any(r['kind']==13 and 8<r['tick']<max(commits,default=0)-4 for r in events))
    restored=restore.get('targets',[])
    slots=[r for r in restored if r['action']!=2**32-1 and r['boolean']==1 and r['before']==r['after']]
    check('L07.70-hooks-restored-and-resident',restore.get('nativeRestored') and restore.get('pinned') and len(slots)==70 and\
          len({r['action'] for r in slots})==70)
    (root/'live-checks.json').write_text(json.dumps(checks,indent=2)+'\n',encoding='utf-8')
    (root/'packet-pairing.json').write_text(json.dumps(dict(trace_records=len(rows),sent_packets=len(sent),errors=errors,world_records=dict(world_counts)),indent=2)+'\n',encoding='utf-8')
    return checks
