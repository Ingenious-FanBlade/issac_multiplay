"""Analyze only the capture from the single tx-01-54 real-game acceptance."""
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
    accepted, resolved = {}, {}
    witnesses = collections.defaultdict(list)
    counts, dispatch_counts = collections.Counter(), collections.Counter()
    evidence, violations = collections.defaultdict(list), []
    physical, latest_source, startups = {}, None, {}
    room_seen, paths, missing, neutral = set(), set(), set(), set()
    maximum_tick = 0

    def remember(key, event):
        if len(evidence[key]) < 32:
            evidence[key].append(event['published'])

    def fail(reason, event):
        if len(violations) < 64:
            violations.append({'reason': reason, 'eventSequence': event['published']})

    def levels(state):
        return tuple(state[k] for k in ['held_flags', 'edge_flags', 'move_x', 'move_y', 'aim_x', 'aim_y'])

    with (root / 'capture/events.jsonl').open(encoding='utf-8-sig') as stream:
        for line in stream:
            e = json.loads(line)
            kind, who, gen, tick = e['kind'], e['ordinal'], e['generation'], e['tick']
            counts[kind] += 1
            maximum_tick = max(maximum_tick, tick)
            identity = (who, gen)
            if kind == 13:
                if e['detail'] == 3:
                    room_seen.add(who)
                    remember('room', e)
                if e['detail'] in [1, 2]:
                    remember('pauseResume', e)
                if e['detail'] == 9:
                    remember('teardown', e)
            if kind == 14 and e['boolean'] and e['detail'] >= 1:
                remember('detached', e)
            if kind == 4:
                if e['decision'] == 1:
                    accepted[(who, gen, e['command'])] = e
                if e['decision'] in [2, 12]:
                    remember(f'reject{e["decision"]}', e)
            if kind == 5:
                resolved[(who, gen, tick)] = e
                if e['command']:
                    command = accepted.get((who, gen, e['command']))
                    if command is None or command['target'] != tick or command['state'] != e['state']:
                        fail('Resolve lacks the same-generation accepted command', e)
                elif any(levels(e['state'])):
                    fail('Missing input retained a previous level or edge', e)
            if kind == 23:
                physical[e['target']] = e
            if kind == 24 and who == 0:
                latest_source = e
            if kind == 22:
                if identity in startups:
                    fail('Repeated startup in one generation', e)
                startups[identity] = e
                first = accepted.get((who, gen, e['command']))
                continuation = e['before'][0] | e['before'][1] << 32
                nominal = accepted.get((who, gen, continuation))
                if first is None or nominal is None or e['target'] != tick + 1 or nominal['target'] != tick + 2:
                    fail('Startup lacks its completed+1 and completed+2 pair', e)
                else:
                    expected = dict(first['state'])
                    expected['edge_flags'] = 0
                    if nominal['state'] != expected:
                        fail('Startup continuation changed levels or replayed an edge', e)
                if who == 0:
                    source = physical.get(e['query'])
                    if source is None or source['before'] != e['after']:
                        fail('A startup lacks a same-loop exact-pair source', e)
                    elif levels(source['state'])[2:] != levels(e['state'])[2:] or source['state']['held_flags'] != e['state']['held_flags']:
                        fail('A startup changed the fresh physical levels', e)
                    if who in room_seen and e['state']['held_flags'] & 1:
                        remember('heldRoomStartup', e)
            if kind == 6:
                snapshot = resolved.get((who, gen, tick))
                if e['decision'] & 1:
                    if snapshot is None or snapshot['state'] != e['state']:
                        fail('Eligible query lacks its resolved snapshot', e)
                    if e['action'] < 8 and e['query'] in [1, 3]:
                        axis = ['move_x', 'move_y', 'aim_x', 'aim_y'][e['action'] // 2]
                        value = max(e['state'][axis] * (1 if e['action'] & 1 else -1), 0) / 32767
                        expected = float(value != 0) if e['query'] == 1 else value
                        if abs(e['scalar'] - expected) > 0.00001:
                            fail('Native query differs from resolved input', e)
                    if e['detail'] == 5 and e['scalar'] and 4 <= e['action'] < 8:
                        paths.add((who, snapshot['detail'] if snapshot else 0))
                        remember(f'input{who}', e)
                elif e['scalar'] or e['boolean']:
                    fail('Ineligible query returned nonneutral input', e)
            if kind == 7 and e['detail'] == 5:
                witnesses[(who, gen, tick, e['query'])].append(e)
            if kind != 25:
                continue
            dispatch_counts[(who, e['action'])] += 1
            if e['action'] == 0:
                if not e['detail'] or e['detail'] & ~31:
                    fail('Shooting suspended without a recognized reason', e)
                if not e['boolean'] or e['before'] != e['after']:
                    fail('Suspended Shooting witness changed or was unreadable', e)
                transition_callers = [0x2FD4AC, 0x2FD54D, 0x4319CC, 0x431A14, 0x431A34]
                if e['query'] in transition_callers:
                    remember(f'roomSuspend{who}', e)
                    if who == 0 and latest_source and latest_source['target'] == e['target'] and latest_source['state']['held_flags'] & 1:
                        remember('heldRoomSuspension', e)
                if not (e['detail'] & 30) and e['query'] in [0x1895C, 0x2FD60C]:
                    fail('Ordinary Player gameplay update was missing Authority', e)
            elif e['action'] == 1:
                if e['detail']:
                    fail('Native Shooting ran in a blocked window', e)
                pending = witnesses[(who, gen, tick, e['query'])]
                if not pending:
                    fail('Executed Shooting lacks its consumer witness', e)
                else:
                    witness = pending.pop(0)
                    if witness['before'] != e['before'] or witness['after'] != e['after']:
                        fail('Dispatch differs from the original-call witness', e)
                snapshot = resolved.get((who, gen, tick))
                if snapshot and not any(levels(snapshot['state'])):
                    (neutral if snapshot['command'] else missing).add(who)
                    remember(f'neutral{who}' if snapshot['command'] else f'missing{who}', e)
            else:
                fail('Unknown Shooting dispatch action', e)

    for pending in witnesses.values():
        for event in pending:
            fail('Native Shooting witness lacks its executed dispatch', event)
    for e in accepted.values():
        startup = startups.get((e['ordinal'], e['generation']))
        if e['target'] != e['tick'] + 2 and not (startup and e['command'] == startup['command'] and e['target'] == e['tick'] + 1):
            fail('Ordinary input changed lead=2 or startup changed lead=1', e)

    checks = []

    def check(name, observed, keys=(), failed=False, file='capture/events.jsonl'):
        checks.append({'name': name, 'status': 'FAIL' if failed else 'PASS' if observed else 'NOT_OBSERVED',
                       'evidenceFile': file, 'eventSequences': sorted({s for key in keys for s in evidence[key]})})

    check('single-registration-and-native-join', counts[2] == 1 and counts[11] == 1 and counts[12] == 1,
          failed=counts[2] > 1 or counts[11] > 1 or counts[12] > 1)
    for who in [0, 1]:
        check(f'participant-{who}-actual-room-update-suspended', bool(evidence[f'roomSuspend{who}']), [f'roomSuspend{who}'])
        check(f'participant-{who}-both-native-paths-execute-fresh-shooting', all((who, path) in paths for path in [1, 2]), [f'input{who}'])
    check('A-held-source-at-room-suspension-and-fresh-restart', bool(evidence['heldRoomSuspension'] and evidence['heldRoomStartup']), ['heldRoomSuspension', 'heldRoomStartup'])
    check('B-explicit-release-still-executes-native-shooting', 1 in neutral, ['neutral1'])
    check('B-missing-input-still-executes-neutral-native-shooting', 1 in missing, ['missing1'])
    check('duplicate-and-old-generation-rejected', bool(evidence['reject2'] and evidence['reject12']), ['reject2', 'reject12'])
    check('pause-resume-observed', bool(evidence['pauseResume']), ['pauseResume'])
    visual_path = root / 'manual-room-shooting-observations.json'
    visual = load(visual_path) if visual_path.exists() else {}
    for field in ['heldAcrossRoomOutbound', 'heldAcrossRoomReturn', 'actualReleaseAfterRoom', 'chargeAfterRepress', 'holdAcrossPause']:
        check(field, visual.get(field) == 'PASS', failed=visual.get(field) == 'FAIL', file=visual_path.name)
    targets = [t for t in restoration['targets'] if t['action'] != 4294967295]
    restored = restoration['nativeRestored'] and restoration['pinned'] and len(targets) == 48 and all(t['boolean'] and t['before'] == t['after'] for t in targets) and process['running']
    check('menu-teardown-pair-absent-root-resident-48-hooks-restored', bool(evidence['teardown'] and evidence['detached']) and restored,
          ['teardown', 'detached'], failed=not restored, file='capture/restoration.json')
    invariants = ['duplicateResolve', 'duplicateWorldCommit', 'duplicateConsumer', 'duplicateAccept', 'inconsistentQuery']
    native_failure = raw.get('status') == 'FAIL' or raw.get('nativeError') != 0 or raw.get('transportFailure') is not None or counts[16] or any(raw.get(k) is not False for k in invariants) or args.controller_exit not in [0, 1]
    check('native-transport-and-runtime-invariants', not native_failure, failed=bool(native_failure))
    check('dispatch-neutrality-freshness-and-query-invariants', not violations, failed=bool(violations))
    status = 'FAIL' if any(c['status'] == 'FAIL' for c in checks) else 'PASS' if all(c['status'] == 'PASS' for c in checks) else 'NOT_OBSERVED'
    result = {'tx': 'tx-01-54', 'status': status, 'checks': checks, 'violations': violations,
              'eventCount': sum(counts.values()), 'maximumAuthorityTick': maximum_tick,
              'shootingDispatchCounts': {f'{who}:{action}': count for (who, action), count in dispatch_counts.items()},
              'restoredSlotCount': len(targets), 'nativeError': raw.get('nativeError'), 'transportFailure': raw.get('transportFailure'),
              'manualChargeObservations': visual, 'automaticRetry': False,
              'boundary': 'Native dispatch/input evidence and separately attributed human charge/laser observations; no claim for complete T0-05 E2E'}
    with (root / 'result.json').open('x', encoding='utf-8') as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
        output.write('\n')
    print(json.dumps({'tx': 'tx-01-54', 'status': status, 'checks': [{'name': c['name'], 'status': c['status']} for c in checks], 'violations': violations}, ensure_ascii=False))
    return 0 if status == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
