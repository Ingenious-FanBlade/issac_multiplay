"""Analyze the sole moved-room native Baby-mode to owned full-Player Join E2E."""
import argparse
import collections
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifact-dir', required=True, type=Path)
    parser.add_argument('--controller-exit', required=True, type=int)
    args = parser.parse_args()
    root = args.artifact_dir.resolve()
    load = lambda path: json.loads(path.read_text(encoding='utf-8-sig'))
    raw = load(root / 'capture/result.json')
    restoration = load(root / 'capture/restoration.json')
    process = load(root / 'process-after.json')
    counts = collections.Counter()
    evidence, records = collections.defaultdict(list), collections.defaultdict(list)
    violations, accepted, bindings = [], {}, {}
    maximum_tick = 0

    def remember(key, event):
        if len(evidence[key]) < 32:
            evidence[key].append(event['published'])

    def fail(reason, event):
        if len(violations) < 64:
            violations.append({'reason': reason, 'eventSequence': event['published']})

    with (root / 'capture/events.jsonl').open(encoding='utf-8-sig') as stream:
        for line in stream:
            e = json.loads(line)
            kind, who = e['kind'], e['ordinal']
            counts[kind] += 1
            maximum_tick = max(maximum_tick, e['tick'])
            if kind in [2, 9, 10, 11, 12, 28, 29]:
                records[kind].append(e)
                remember(f'kind{kind}', e)
            if kind == 3:
                bindings[(who, e['generation'])] = e
                remember(f'binding{who}', e)
            if kind == 4 and e['decision'] == 1:
                accepted[(who, e['generation'], e['command'])] = e
            if kind == 4 and e['decision'] in [2, 12]:
                remember(f'reject{e["decision"]}', e)
            if kind == 5 and e['command']:
                command = accepted.get((who, e['generation'], e['command']))
                if command is None or command['target'] != e['tick'] or command['state'] != e['state'] or (who, e['generation']) not in bindings:
                    fail('Resolve lacked its accepted command or exact generation binding', e)
                elif any(e['state'][k] for k in ['move_x', 'move_y', 'aim_x', 'aim_y']):
                    remember(f'input{who}', e)
            if kind == 7 and e['detail'] in [1, 5] and e['decision'] == 1 and e['boolean'] and e['before'] != e['after']:
                if any(e['state'][k] for k in ['move_x', 'move_y', 'aim_x', 'aim_y']):
                    remember(f'world{who}-{e["detail"]}', e)
            if kind == 19 and who == 1 and e['action'] == 2 and e['detail'] in [25, 28]:
                fail('Owned player was rejected by creation identity or native Player kind', e)
            if kind == 13 and e['detail'] == 9:
                remember('teardown', e)
            if kind == 14 and e['boolean'] and e['detail'] >= 1:
                remember('detached', e)

    devices = records[2]
    creates, joined = records[11], records[12]
    routes, outcomes = records[28], records[29]
    passes = {e['command']: e['detail'] for e in records[9] if e['detail']}
    own = [e for e in routes if e['ordinal'] == 1]
    foreign = [e for e in routes if e['ordinal'] != 1]
    for e in routes:
        if not e['boolean'] or e['before'][2] != e['after'][0] or e['query'] != bool(e['before'][2] & 0x10) or e['after'][3] != 1:
            fail('Native predicate, flags stability or original-once observation disagreed', e)
        if e['command'] not in passes or e['action'] not in [0x5BAE7E, 0x5BB081, 0x5BB288]:
            fail('Route lacked its native callsite or bounded Join pass', e)
        if e['ordinal'] == 1:
            if e['decision'] != 0 or e['detail'] != 3:
                fail('Owned device did not select the full-Player route', e)
        elif e['query'] != e['decision'] or e['detail']:
            fail('Foreign device route changed', e)
    for e in outcomes:
        if e['query'] != 1 or e['decision'] not in [1, 2] or not e['boolean'] or e['before'] != e['after']:
            fail('Join changed its host identity/health or had unexpected topology', e)
        if e['decision'] == 2 and e['detail'] != 0:
            fail('Join published a Baby/possessor Variant', e)
    for e in records[10]:
        if e['query'] == 1 and (e['boolean'] or e['decision'] != 1):
            fail('Possessor gameplay Pressed input was nonneutral', e)
        if e['query'] == 2 and e['decision']:
            expected = (passes.get(e['command']) == 1 and e['action'] == 19) or (passes.get(e['command']) == 2 and e['action'] == 14)
            if bool(e['boolean']) != expected:
                fail('Join command escaped the two-pass control protocol', e)

    main_threads = set()
    with (root / 'capture/loop-events.jsonl').open(encoding='utf-8-sig') as stream:
        for line in stream:
            e = json.loads(line)
            if e['flags'] & 2 and e['threadId']:
                main_threads.add(e['threadId'])
    route_threads = {e['target'] for e in routes}
    checks = []

    def check(name, observed, keys=(), failed=False, file='capture/events.jsonl'):
        checks.append({'name': name, 'status': 'FAIL' if failed else 'PASS' if observed else 'NOT_OBSERVED',
                       'evidenceFile': file, 'eventSequences': sorted({s for key in keys for s in evidence[key]})})

    for site in [0x5BAE7E, 0x5BB081, 0x5BB288]:
        covered = any(e['action'] == site and e['query'] == 1 and e['decision'] == 0 and e['before'][2] & 0x10 for e in own)
        check(f'actual-Baby-true-to-owned-full-Player-route-{site:06X}', covered, ['kind28'])
    check('foreign-native-device-route-preserved', bool(foreign), ['kind28'])
    check('all-native-predicates-main-thread-original-once-flags-unchanged', bool(routes) and len(route_threads) == 1 and route_threads <= main_threads, ['kind28'])
    check('single-registration-single-ordinary-create-single-publication', len(devices) == len(creates) == len(joined) == 1 and creates[0]['detail'] == 0x5BB415,
          ['kind2', 'kind11', 'kind12'], failed=any(len(items) > 1 for items in [devices, creates, joined]))
    identity = False
    if len(devices) == len(creates) == len(joined) == 1:
        device, create, player = devices[0], creates[0], joined[0]
        b = next((e for (who, _), e in bindings.items() if who == 1), None)
        identity = b is not None and b['before'][0] == player['before'][1] and b['action'] == device['before'][0] == create['before'][0] == player['before'][3]
        identity = identity and b['before'][1:3] == device['before'][1:3] == create['after'][:2] == player['after'][:2]
        identity = identity and player['after'][2:] == [1, 0] and player['before'][2] == 2 and create['command'] == player['command']
        identity = identity and all(e['before'][0] == b['action'] and e['before'][1] == create['before'][1] and e['before'][3] == player['before'][0] and e['after'][1:3] == b['before'][1:3] for e in own)
    check('B-Type1-Variant0-exact-owned-pair-and-factory-identity', identity, ['kind2', 'kind11', 'kind12', 'binding1'])
    check('native-Join-closes-within-two-passes', 1 <= len(passes) <= 2 and len(joined) == 1 and joined[0]['command'] in passes,
          ['kind9', 'kind12'], failed=len(passes) > 2)
    check('A-health-unchanged-through-every-Join-pass', len(outcomes) == len(passes) and bool(outcomes) and all(e['boolean'] and e['before'] == e['after'] for e in outcomes), ['kind29'])
    check('possessor-Pressed8-observed-neutral', any(e['action'] == 8 and e['query'] == 1 and e['decision'] == 1 and not e['boolean'] for e in records[10]), ['kind10'])
    for who in [0, 1]:
        check(f'participant-{who}-fresh-scheduler-input', bool(evidence[f'input{who}']), [f'binding{who}', f'input{who}'])
        for consumer in [1, 5]:
            check(f'participant-{who}-native-world-consumer-{consumer}', bool(evidence[f'world{who}-{consumer}']), [f'world{who}-{consumer}'])
    check('duplicate-and-old-generation-input-rejected', bool(evidence['reject2'] and evidence['reject12']), ['reject2', 'reject12'])
    targets = [e for e in restoration['targets'] if e['action'] != 4294967295]
    target_map = {e['action']: e for e in targets}
    restored = restoration['nativeRestored'] and restoration['pinned'] and process['running'] and len(targets) == len(target_map) == 51
    restored = restored and all(e['boolean'] and e['before'] == e['after'] for e in targets)
    for site, data in [(0x5BAE7E, bytes.fromhex('E8 FD FA FF FF')), (0x5BB081, bytes.fromhex('E8 FA F8 FF FF')), (0x5BB288, bytes.fromhex('E8 F3 F6 FF FF'))]:
        target = target_map.get(site)
        restored = restored and target is not None and target['query'] == 5 and target['before'][:2] == [int.from_bytes(data[:4], 'little'), data[4]]
    check('menu-teardown-root-DLL-resident-and-all-51-targets-restored', bool(evidence['teardown'] and evidence['detached']) and restored,
          ['teardown', 'detached'], failed=not restored, file='capture/restoration.json')
    invariants = ['duplicateResolve', 'duplicateWorldCommit', 'duplicateConsumer', 'duplicateAccept', 'inconsistentQuery']
    native_failure = raw.get('status') == 'FAIL' or raw.get('nativeError') != 0 or raw.get('transportFailure') is not None or counts[16] or any(raw.get(k) is not False for k in invariants) or args.controller_exit not in [0, 1]
    check('native-transport-scheduler-invariants', not native_failure, failed=bool(native_failure))
    check('route-identity-health-and-input-invariants', not violations, failed=bool(violations))
    manual_path = root / 'manual-join-observations.json'
    manual = load(manual_path) if manual_path.exists() else {}
    check('user-observed-full-B-unchanged-A-health-and-normal-input', manual.get('status') == 'PASS',
          failed=manual.get('status') == 'FAIL', file=manual_path.name)
    status = 'FAIL' if any(c['status'] == 'FAIL' for c in checks) else 'PASS' if all(c['status'] == 'PASS' for c in checks) else 'NOT_OBSERVED'
    result = {'tx': 'tx-01-56', 'status': status, 'checks': checks, 'violations': violations,
              'nativeJoinRoutes': routes, 'nativeJoinOutcomes': outcomes, 'eventCount': sum(counts.values()),
              'maximumAuthorityTick': maximum_tick, 'restoredSlotCount': len(targets),
              'nativeError': raw.get('nativeError'), 'transportFailure': raw.get('transportFailure'),
              'automaticRetry': False,
              'boundary': 'Actual moved-room Baby-mode predicate routed to one owned full Player; historical tx-01-51 cause remains inferred. Not complete T0-05 or arbitrary native Join branches.'}
    with (root / 'result.json').open('x', encoding='utf-8') as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
        output.write('\n')
    print(json.dumps({'tx': 'tx-01-56', 'status': status, 'checks': [{'name': c['name'], 'status': c['status']} for c in checks], 'violations': violations}, ensure_ascii=False))
    return 0 if status == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
