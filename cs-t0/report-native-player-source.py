"""Capture-only source attribution; no process access or runtime operations."""
import argparse
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--artifact-dir', required=True, type=Path)
args = parser.parse_args()
root = args.artifact_dir.resolve()
capture = root / 'capture'
load = lambda name: json.loads((capture / name).read_text(encoding='utf-8-sig'))
feature = load('result.json')
restoration = load('restoration.json')
calls = {}
with (capture / 'events.jsonl').open(encoding='utf-8-sig') as source:
    for line in source:
        event = json.loads(line)
        if event['kind'] == 20:
            calls[event['after'][1]] = {'call': event, 'stackEvents': [], 'stackAddresses': []}
        elif event['kind'] == 21 and event['action'] in calls:
            sample = calls[event['action']]
            sample['stackEvents'].append(event['published'])
            sample['stackAddresses'].extend(event['before'][:min(4, event['detail'] - event['query'])])
samples = list(calls.values())
unscoped = [sample for sample in samples if sample['call']['detail'] == 0]
scoped = [sample for sample in samples if sample['call']['detail'] & 3 == 3]
slots = [event for event in restoration['targets'] if event['action'] != 4294967295]
restored = restoration['nativeRestored'] and restoration['pinned'] and len(slots) == 47 and all(event['boolean'] and event['before'] == event['after'] for event in slots)
failure = feature.get('nativeError') != 0 or feature.get('transportFailure') is not None or feature.get('status') == 'FAIL' or not restored
status = 'FAIL' if failure else 'SOURCE_CAPTURED' if scoped and unscoped else 'NOT_OBSERVED'
report = {'tx': 'tx-01-50', 'phase': 'source-attribution-only', 'status': status,
          'featureAcceptanceStatus': feature.get('status'), 'featureAcceptancePassed': False,
          'samples': samples, 'ordinaryUnscopedSourceIndices': [sample['call']['after'][1] for sample in unscoped],
          'nativeError': feature.get('nativeError'), 'transportFailure': feature.get('transportFailure'),
          'restoredSlots': len(slots), 'restoredAndPinned': restored, 'automaticRetry': False}
with (root / 'source-result.json').open('x', encoding='utf-8') as output:
    json.dump(report, output, ensure_ascii=False, indent=2)
    output.write('\n')
print(json.dumps({'status': status, 'sourceCount': len(samples), 'unscopedSourceCount': len(unscoped), 'restoredAndPinned': restored}))
raise SystemExit(0 if status == 'SOURCE_CAPTURED' else 1)
