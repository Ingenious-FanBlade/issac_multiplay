"""Report the single tx-01-60 real-game capture; never accesses the game."""
import argparse
import collections
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifact-dir', required=True, type=Path)
    parser.add_argument('--controller-exit', required=True, type=int)
    parser.add_argument('--tx', default='tx-01-60')
    args = parser.parse_args()
    root = args.artifact_dir.resolve()
    load = lambda name: json.loads((root / name).read_text(encoding='utf-8-sig'))
    raw = load('capture/result.json')
    restoration = load('capture/restoration.json')
    process = load('process-after.json')
    counts = collections.Counter()
    evidence = collections.defaultdict(list)
    characters, outcomes, pill_changes, held_sources = [], [], [], collections.Counter()
    accepted, violations, bindings = {}, [], {}
    max_tick = 0

    def remember(key, event):
        if len(evidence[key]) < 32:
            evidence[key].append(event['published'])

    with (root / 'capture/events.jsonl').open(encoding='utf-8-sig') as stream:
        for line in stream:
            e = json.loads(line)
            who, kind = e['ordinal'], e['kind']
            counts[kind] += 1
            max_tick = max(max_tick, e['tick'])
            if kind == 30:
                characters.append(e)
                remember('character', e)
            if kind == 29:
                outcomes.append(e)
                remember('join-health', e)
            if kind == 3:
                bindings[(who, e['generation'])] = e
            if kind == 4 and e['decision'] == 1:
                accepted[(who, e['generation'], e['command'])] = e
            if kind == 5 and e['command']:
                command = accepted.get((who, e['generation'], e['command']))
                if command is None or command['target'] != e['tick'] or command['state'] != e['state'] or (who, e['generation']) not in bindings:
                    if len(violations) < 32:
                        violations.append({'eventSequence': e['published'], 'reason': 'Resolve lacks exact accepted command/generation binding'})
            if kind == 6 and who == 1 and e['action'] == 10 and e['query'] == 1 and e['boolean'] and e['decision'] & 1 and e['state']['held_flags'] & 4:
                remember('held10', e)
                source = 'controlled-companion' if e['before'][2] == 2 else 'engine-or-native-query-repeat'
                held_sources[source] += 1
            if kind == 6 and who == 1 and e['action'] == 10 and e['query'] == 2 and e['boolean'] and e['decision'] & 1:
                remember('triggered10', e)
            if kind == 7 and who == 1 and e['detail'] == 4 and e['decision'] == 1 and e['boolean'] and e['before'][1] and e['before'][1:] != e['after'][1:]:
                remember('pill-resource-change', e)
                if len(pill_changes) < 16:
                    pill_changes.append(e)
            if kind == 26 and who == 1 and e['action'] == 10 and e['query'] == 1 and e['boolean'] and e['decision'] == 3 and e['scalar'] > 0:
                remember('held10-pure', e)
            if kind == 13 and e['detail'] == 9:
                remember('teardown', e)

    checks = []
    required = {
        'single-device-registration', 'single-native-create-and-publish', 'B-character-selection',
        'A-B-native-identities-match', 'participant-0-binding', 'participant-1-binding',
        'participant-0-world-1', 'participant-0-world-5', 'participant-1-world-1', 'participant-1-world-5',
        'input-decision-2', 'input-decision-5', 'input-decision-7', 'input-decision-12', 'input-decision-3', 'input-decision-11',
        'A-physical-loopback', 'resident-dll-and-restoration', 'missing-input-neutral',
        'B-native-consumer-state-4', 'held-10', 'repeated-query-pure', 'B-retired-with-root',
        'A-continues-after-B', 'all-51-hooks-restored',
    }
    for check in raw.get('checks', []):
        if check['name'] in required:
            checks.append({**check, 'evidenceFile': 'capture/events.jsonl'})

    def check(name, observed, key=None, failed=False):
        checks.append({'name': name, 'status': 'FAIL' if failed else 'PASS' if observed else 'NOT_OBSERVED',
                       'evidenceFile': 'capture/events.jsonl', 'eventSequences': evidence[key or name]})

    selected = [e for e in characters if e['action'] == 2]
    created = [e for e in characters if e['action'] == 3]
    chosen = len(selected) == len(created) == 1 and selected[0]['decision'] == created[0]['decision'] == created[0]['query'] == 1
    check('B-request-factory-and-actual-role-1', chosen and raw.get('bCharacter', {}).get('requested', {}).get('id') == 1, 'character', failed=bool(characters and any(e['action'] >= 2 and (not e['boolean'] or e['decision'] != 1) for e in characters)))
    check('A-identity-and-health-preserved-at-Join', bool(outcomes) and all(e['boolean'] and e['before'] == e['after'] for e in outcomes), 'join-health', failed=any(not e['boolean'] or e['before'] != e['after'] for e in outcomes))
    check('B-PillCard-resource-slot-actually-consumed', bool(pill_changes), 'pill-resource-change')
    check('B-PillCard-Triggered10-read', bool(evidence['triggered10']), 'triggered10')
    check('B-held10-real-consumer-pure-read', bool(evidence['held10'] and evidence['held10-pure']), 'held10-pure')
    targets = [e for e in restoration.get('targets', []) if e['action'] != 4294967295]
    restored = restoration.get('nativeRestored') and restoration.get('pinned') and process.get('running') and process.get('nativeResident') and len(targets) == 51
    restored = restored and all(e['boolean'] and e['before'] == e['after'] for e in targets)
    check('menu-teardown-51-Hooks-and-resident-DLL', bool(evidence['teardown']) and restored, 'teardown', failed=not restored)
    invariants = ['duplicateResolve', 'duplicateWorldCommit', 'duplicateConsumer', 'duplicateAccept', 'inconsistentQuery']
    bad = raw.get('status') == 'FAIL' or raw.get('nativeError') != 0 or raw.get('transportFailure') is not None or counts[16] or violations or any(raw.get(k) is not False for k in invariants) or args.controller_exit not in [0, 1]
    check('native-transport-input-invariants', not bad, failed=bool(bad))
    status = 'FAIL' if any(c['status'] == 'FAIL' for c in checks) else 'PASS' if all(c['status'] == 'PASS' for c in checks) else 'NOT_OBSERVED'
    result = {'tx': args.tx, 'status': status, 'bCharacter': raw.get('bCharacter'), 'checks': checks,
              'checkCounts': dict(collections.Counter(c['status'] for c in checks)), 'held10ReadSources': dict(held_sources),
              'pillResourceChanges': pill_changes, 'nativeJoinOutcomes': outcomes, 'violations': violations,
              'eventCount': sum(counts.values()), 'maximumAuthorityTick': max_tick, 'restoredSlotCount': len(targets),
              'nativeError': raw.get('nativeError'), 'transportFailure': raw.get('transportFailure'),
              'notCoveredInThisWindow': [c['name'] for c in raw.get('checks', []) if c['name'] not in required and c['status'] != 'PASS'],
              'controllerExit': args.controller_exit, 'automaticRetry': False,
              'boundary': 'B=1 plus remaining Pill/Card resource use and held10. Companion reads identified; earlier Loading/death/abnormal-Join scenarios not rerun. Not complete OOM/EH/worker closure.'}
    with (root / 'result.json').open('x', encoding='utf-8') as output:
        json.dump(result, output, ensure_ascii=False, indent=2)
        output.write('\n')
    print(json.dumps({'tx': args.tx, 'status': status, 'checkCounts': result['checkCounts'], 'held10ReadSources': dict(held_sources), 'violations': violations}, ensure_ascii=False))
    return 0 if status == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
