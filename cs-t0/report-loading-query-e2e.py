"""Report the sole real Continue/loading and controlled repeated-query capture."""
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
    evidence, violations = collections.defaultdict(list), []
    lifecycle = collections.defaultdict(list)
    windows, recoveries, accepted, bindings, resolves = [], [], {}, {}, {}
    repeats, reads = [], collections.defaultdict(list)
    repeat_categories = set()
    blocked = False
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
            kind, who, gen, tick = e['kind'], e['ordinal'], e['generation'], e['tick']
            counts[kind] += 1
            maximum_tick = max(maximum_tick, tick)
            if kind == 13:
                lifecycle[(who, e['detail'])].append(e)
                if e['detail'] == 4:
                    blocked = True
                    remember('loading', e)
                if e['detail'] == 5:
                    remember('loadingExit', e)
                if e['detail'] == 9:
                    remember('teardown', e)
            if kind == 27 and e['action'] == 2:
                windows.append(e)
                if not e['boolean']:
                    fail('Unhealthy lifecycle envelope', e)
                if e['query'] == 1:
                    if e['before'][0] != 0 or e['after'][0] != 1 or e['after'][2] != 0 or not e['detail'] & 4:
                        fail('Loading entry did not invalidate bindings and block input', e)
                elif e['query'] == 0:
                    if e['after'][0] != 0 or e['after'][2] != 0 or e['after'][3] != 1 or not e['detail'] & 2:
                        fail('LoadingExit did not enter neutral authority recovery', e)
                elif e['query'] == 2:
                    blocked = False
                    recoveries.append(e)
                    remember('recovery', e)
                    if e['after'][3] or e['detail'] & 3:
                        fail('Authority recovery kept a stale loading or pause block', e)
            if blocked and (kind in [5, 8] or kind == 4 and e['decision'] == 1 or kind == 6 and (e['scalar'] or e['boolean'])):
                fail('Input submit, simulation or nonneutral query occurred during loading/recovery', e)
            if kind == 3:
                bindings[(who, gen)] = e
            if kind == 4 and e['decision'] == 1:
                accepted[(who, gen, e['command'])] = e
            if kind == 4 and e['decision'] in [2, 12]:
                remember(f'reject{e["decision"]}', e)
            if kind == 5:
                resolves[(who, gen, tick)] = e
                if e['command']:
                    command = accepted.get((who, gen, e['command']))
                    if command is None or command['target'] != tick or command['state'] != e['state'] or (who, gen) not in bindings:
                        fail('Resolve lacked a fresh accepted command and unique binding', e)
                    if recoveries and any(e['state'][k] for k in ['move_x', 'move_y', 'aim_x', 'aim_y']):
                        remember(f'resumed{who}', e)
            if kind == 6 and e['before'][2] in [1, 2]:
                reads[e['before'][3]].append(e)
            if kind == 26:
                repeats.append(e)
                group = reads.pop(e['command'], [])
                expected_key = (who, gen, tick, e['action'], e['query'], e['detail'])
                if e['decision'] != 3 or not e['boolean'] or e['after'][3] != 3 or e['before'][2] != e['after'][1]:
                    fail('Repeated read changed its value or Participant state', e)
                if e['before'][0] != e['before'][1] or e['before'][0] != e['after'][0]:
                    fail('Repeated read floating-point bits differ', e)
                if len(group) != 3 or [r['before'][1] for r in group] != [0, 1, 2]:
                    fail('Repeated-read group lacked three actual reads', e)
                for r in group:
                    key = (r['ordinal'], r['generation'], r['tick'], r['action'], r['query'], r['detail'])
                    snapshot = resolves.get((who, gen, tick))
                    if key != expected_key or not r['decision'] & 1 or r['scalar'] != e['scalar'] or snapshot is None or snapshot['state'] != r['state']:
                        fail('Repeated query changed context or lacked its resolved snapshot', r)
                repeat_categories.add((who, e['query'], e['scalar'] != 0))
                remember(f'repeat{who}-{e["query"]}', e)
            if kind == 14 and e['boolean']:
                remember('detached', e)

    for group in reads.values():
        for event in group:
            fail('Repeated-read records lacked their group conclusion', event)
    if blocked and windows:
        fail('Loading never reached authority recovery', windows[-1])

    wanted_loops = {e['target'] for e in windows if e['query'] in [0, 1]}
    native = collections.defaultdict(list)
    with (root / 'capture/loop-events.jsonl').open(encoding='utf-8-sig') as stream:
        for line in stream:
            e = json.loads(line)
            if e['loopIteration'] in wanted_loops and e['kind'] in [39, 40, 55, 56]:
                native[e['loopIteration']].append(e)
    provenance = 0
    entries = [e for e in windows if e['query'] == 1]
    exits = [e for e in windows if e['query'] == 0]
    for entry in entries:
        exit_event = next((e for e in exits if e['published'] > entry['published']), None)
        if exit_event is None:
            fail('Loading lacked its matching exit', entry)
            continue
        records = native[entry['target']]
        kinds = {e['kind'] for e in records}
        valid = all(k in kinds for k in [39, 40, 55, 56]) and entry['target'] == exit_event['target']
        valid = valid and entry['decision'] == 0x559B26 and exit_event['decision'] == 0x554D63
        threads = {e['threadId'] for e in records}
        valid = valid and len(threads) == 1 and 0 not in threads
        for record in records:
            valid = valid and not record['errorCode'] and bool(record['flags'] & 2) and not record['flags'] & 4
            if record['kind'] in [40, 56]:
                valid = valid and record['originalCalls'] == 1 and bool(record['flags'] & 8)
        if not valid:
            fail('Loading labels lacked native main-thread state-apply/owner provenance', entry)
        else:
            provenance += 1
            remember('nativeLoading', entry)

    checks = []

    def check(name, observed, keys=(), failed=False, file='capture/events.jsonl'):
        checks.append({'name': name, 'status': 'FAIL' if failed else 'PASS' if observed else 'NOT_OBSERVED',
                       'evidenceFile': file, 'eventSequences': sorted({s for key in keys for s in evidence[key]})})

    check('actual-native-continue-loading-transaction', provenance > 0, ['nativeLoading'])
    for who in [0, 1]:
        enters, leaves = lifecycle[(who, 4)], lifecycle[(who, 5)]
        paired = any(b['published'] > a['published'] and b['generation'] > a['generation'] for a in enters for b in leaves)
        check(f'participant-{who}-Loading-and-LoadingExit-generation-invalidated', paired, ['loading', 'loadingExit'])
        check(f'participant-{who}-fresh-input-after-loading', bool(evidence[f'resumed{who}']), [f'resumed{who}'])
        for query, name in [(1, 'Pressed'), (2, 'Triggered'), (3, 'Scalar')]:
            check(f'participant-{who}-same-{name}-read-three-times', any(w == who and q == query for w, q, _ in repeat_categories), [f'repeat{who}-{query}'])
        check(f'participant-{who}-nonzero-level-repeated-reads', (who, 1, True) in repeat_categories and (who, 3, True) in repeat_categories, [f'repeat{who}-1', f'repeat{who}-3'])
    check('loading-recovery-explicitly-released-before-input', bool(recoveries), ['loading', 'loadingExit', 'recovery'])
    check('duplicate-and-old-generation-input-rejected', bool(evidence['reject2'] and evidence['reject12']), ['reject2', 'reject12'])
    check('single-device-registration-and-native-join', counts[2] == 1 and counts[11] == 1 and counts[12] == 1,
          failed=counts[2] > 1 or counts[11] > 1 or counts[12] > 1)
    query_check = next((c for c in raw.get('checks', []) if c['name'] == 'repeated-query-pure'), {})
    check('existing-controller-observes-identical-same-key-reads', query_check.get('status') == 'PASS')
    targets = [e for e in restoration['targets'] if e['action'] != 4294967295]
    restored = restoration['nativeRestored'] and restoration['pinned'] and len(targets) == 48 and all(e['boolean'] and e['before'] == e['after'] for e in targets) and process['running']
    check('menu-teardown-resident-root-and-48-hooks-restored', bool(evidence['teardown'] and evidence['detached']) and restored,
          ['teardown', 'detached'], failed=not restored, file='capture/restoration.json')
    invariants = ['duplicateResolve', 'duplicateWorldCommit', 'duplicateConsumer', 'duplicateAccept', 'inconsistentQuery']
    native_failure = raw.get('status') == 'FAIL' or raw.get('nativeError') != 0 or raw.get('transportFailure') is not None or counts[16] or any(raw.get(k) is not False for k in invariants) or args.controller_exit not in [0, 1]
    check('native-transport-and-runtime-invariants', not native_failure, failed=bool(native_failure))
    check('loading-neutrality-and-read-state-purity-invariants', not violations, failed=bool(violations))
    status = 'FAIL' if any(c['status'] == 'FAIL' for c in checks) else 'PASS' if all(c['status'] == 'PASS' for c in checks) else 'NOT_OBSERVED'
    result = {'tx': 'tx-01-55', 'status': status, 'checks': checks, 'violations': violations,
              'nativeLoadingTransactions': provenance, 'repeatedReadGroups': len(repeats),
              'repeatedQueryCategories': sorted(repeat_categories), 'eventCount': sum(counts.values()),
              'maximumAuthorityTick': maximum_tick, 'restoredSlotCount': len(targets),
              'nativeError': raw.get('nativeError'), 'transportFailure': raw.get('transportFailure'),
              'automaticRetry': False,
              'boundary': 'Initial Continue loading before B Join; controlled Read repetitions in actual native query contexts, not a claim of naturally repeated engine calls or complete T0-05 E2E'}
    with (root / 'result.json').open('x', encoding='utf-8') as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
        output.write('\n')
    print(json.dumps({'tx': 'tx-01-55', 'status': status, 'checks': [{'name': c['name'], 'status': c['status']} for c in checks], 'violations': violations}, ensure_ascii=False))
    return 0 if status == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
