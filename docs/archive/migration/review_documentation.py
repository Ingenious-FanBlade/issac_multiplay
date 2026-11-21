"""One complete static migration/document review; no build, game, digest or code tests.

Run this script once at the end of a substantive documentation transaction.
It reads source text/JSON and compares copied file bytes directly, never hashes.
Historical archives intentionally retain links to uncopied large live artifacts.
"""
from pathlib import Path
from collections import Counter
from datetime import datetime, timezone
from urllib.parse import unquote
import json
import re
import struct
import sys

ROOT=Path(__file__).resolve().parents[2]
OUTPUT=ROOT/'docs/archive/migration/static-review.json'
if OUTPUT.exists():
    raise FileExistsError('Review output already exists; do not automatically repeat acceptance.')
checks=[]
def record(name,passed,**facts):
    checks.append({'name':name,'passed':bool(passed),**facts})

manifest=json.loads((ROOT/'docs/archive/migration/source-copy-manifest.json').read_text(encoding='utf-8'))
SOURCE=Path(manifest['source_root'])
changed={'AGENTS.md','cs-t0/accept-t09-b-live.py'}
missing=[];different=[];copied_bytes=0
for row in manifest['files']:
    relative=row['path'];target=ROOT/relative;origin=SOURCE/relative
    if not target.is_file() or not origin.is_file():
        missing.append(relative);continue
    copied_bytes+=target.stat().st_size
    if relative not in changed and target.read_bytes()!=origin.read_bytes():different.append(relative)
record('source_copy_and_original_files_preserved',not missing and not different,
       manifest_files=len(manifest['files']),copied_bytes=copied_bytes,
       intentionally_adapted=sorted(changed),missing=missing,different=different)
record('destination_existing_git_directory_preserved',(ROOT/'.git').is_dir(),
       action='copier excludes .git and refuses overwriting existing files; no Git mutation commands')
record('excluded_cache_and_live_artifacts',not (ROOT/'cs-t0/target').exists() and not list((ROOT/'cs-t0/vm_share').iterdir()),
       target_cache_copied=False,vm_live_contents_copied=False,
       copied_artifact_directories=[p.name for p in sorted((ROOT/'artifacts/cs-t0').iterdir())])

docs=[ROOT/'README.md',ROOT/'AGENTS.md',ROOT/'docs/archive/README.md',ROOT/'docs/archive/migration/README.md']
docs+=sorted((ROOT/'docs/archive/hooks').glob('*.md'))
docs+=sorted((ROOT/'docs/archive/design').glob('*.md'))
bad_links=[];link_count=0
for document in docs:
    text=document.read_text(encoding='utf-8-sig')
    for target in re.findall(r'(?<!!)\[[^\]]+\]\(([^)]+)\)',text):
        if re.match(r'^[a-zA-Z][a-zA-Z0-9+.-]*:',target) or target.startswith('#'):continue
        target=unquote(target.split('#',1)[0]).strip('<>')
        if not target:continue
        link_count+=1
        candidate=(document.parent/target).resolve()
        if candidate==OUTPUT:continue
        if not candidate.exists():bad_links.append({'document':str(document.relative_to(ROOT)),'target':target})
record('new_document_relative_links',not bad_links,documents=len(docs),local_links=link_count,missing=bad_links,
       historical_archive_links='preserved; excluded from new-document link completeness')

inventory=json.loads((ROOT/'docs/archive/hooks/installed-hook-inventory.json').read_text(encoding='utf-8'))['records']
reverse=json.loads((ROOT/'docs/archive/hooks/reverse-point-inventory.json').read_text(encoding='utf-8'))
constants=json.loads((ROOT/'docs/archive/hooks/source-address-constants.json').read_text(encoding='utf-8'))['records']
expected={'loop_runtime.cpp':32,'active_targets.cpp':13,'active_runtime.cpp':13,
          'coop_active_runtime.cpp':19,'coop_passive_runtime.cpp':24,'input_runtime.cpp':11,
          'raw_apply_runtime.cpp':18,'runtime.cpp':18,'boundary_runtime.cpp':16,
          'offline_attestation_runtime.cpp':7,'world_record.cpp':19}
actual=dict(Counter(r['family'] for r in inventory if r['installation']=='SOURCE_INSTALLABLE'))
record('all_source_installation_families',actual==expected,expected=expected,actual=actual,
       records=len(inventory),count_semantics='family occurrences; overlapping sites and dependencies are not simultaneous unique hooks')
bad_rows=[]
for row in inventory:
    document=ROOT/'docs/archive/hooks'/(row['module']+'.md')
    content=document.read_text(encoding='utf-8') if document.exists() else ''
    if row['family'] not in content or (row['site_rva'] and row['site_rva'] not in content):
        bad_rows.append({'family':row['family'],'index':row['index'],'reason':'missing in module document'})
    for key,va_key in [('site_rva','ida_va'),('target_rva','target_ida_va')]:
        if row[key] and int(row[va_key],16)!=int(row[key],16)+0x400000:
            bad_rows.append({'family':row['family'],'index':row['index'],'reason':'RVA/VA mapping'})
    if row['kind'] in ('REL32_CALL','CALL_DEPENDENCY'):
        raw=bytes.fromhex(row['expected_bytes'])
        if len(raw)!=5 or raw[0]!=0xE8 or int(row['site_rva'],16)+5+struct.unpack('<i',raw[1:])[0]!=int(row['target_rva'],16):
            bad_rows.append({'family':row['family'],'index':row['index'],'reason':'source rel32 encoding'})
    if row['kind']=='ENTRY_PATCH' and len(bytes.fromhex(row['expected_bytes']))!=row['length']:
        bad_rows.append({'family':row['family'],'index':row['index'],'reason':'entry byte span'})
record('module_site_tables_and_source_encodings',not bad_rows,problems=bad_rows,
       note='Source declarations only; no PE opened or runtime instruction/ABI claim')

specs={'entity':'kCommon','player':'kPlayer','npc':'kNpc','familiar':'kFamiliar','effect':'kEffect',
       'pickup':'kPickup','tear':'kTear','projectile':'kProjectile','grid-room':'kGrid'}
source_text=(ROOT/'cs-t0/native/src/world_record.cpp').read_text(encoding='utf-8-sig')
field_errors=[]
for module,symbol in specs.items():
    match=re.search(r'const Spec '+symbol+r'\[\]\s*\{(.*?)\};',source_text,re.S)
    parsed=[]
    if match:
        parsed=[(int(a,16),int(b)) for a,b in re.findall(r'\{\s*0x[0-9A-Fa-f]+\s*,\s*(0x[0-9A-Fa-f]+)\s*,\s*(\d+)\s*,\s*Encoding::\w+\s*\}',match[1])]
    documented=[(int(r['offset'],16),r['width_bytes']) for r in reverse['fields'] if r['module']==module]
    if not match or parsed!=documented:field_errors.append({'module':module,'source':parsed,'documented':documented})
record('raw_field_tables_match_current_collector',not field_errors,fields=len(reverse['fields']),problems=field_errors,
       laser_exception='Laser Timeout is static binding only, explicitly outside collector min-read-v1')

index_errors=[]
for row in reverse['functions']:
    path=ROOT/'docs/archive/hooks'/(row['module']+'.md')
    if not path.exists() or row['rva'] not in path.read_text(encoding='utf-8'):
        index_errors.append({'rva':row['rva'],'reason':'module index absent'})
    for source in row['sources']:
        if not (ROOT/source).is_file():index_errors.append({'rva':row['rva'],'reason':'evidence absent','source':source})
for row in reverse['rtti']:
    path=ROOT/'docs/archive/hooks'/(row['module']+'.md')
    if row['rva'].lower() not in path.read_text(encoding='utf-8').lower():index_errors.append({'rtti':row['name']})
for row in constants:
    content=(ROOT/'docs/archive/hooks'/(row['module']+'.md')).read_text(encoding='utf-8')
    if row['symbol'] not in content or row['value'] not in content:index_errors.append({'constant':row['symbol']})
record('reverse_evidence_fields_vtables_and_constants_indexed',not index_errors,
       static_functions=len(reverse['functions']),rtti_tables=len(reverse['rtti']),
       named_location_constants=len(constants),problems=index_errors)

design=(ROOT/'docs/archive/design/state-synchronization.md').read_text(encoding='utf-8')
required=['单房间','无预测','完整基线','增量状态','表现事件','ImmutableBaseline','AppliedSnapshotAck',
          'room_epoch','主线程','gameplay RNG','半套世界','不计算或校验任何哈希','一次统一E2E','尚未实现']
missing_design=[term for term in required if term not in design]
record('adopted_design_and_evidence_boundaries',not missing_design,missing_topics=missing_design,
       reviewed_semantics=['immutable baseline reconstruction, not patching arbitrary current state',
       'persistent results are state; events do not replay damage',
       'native partial apply is not claimed atomic',
       'unknown replica apply and presentation boundaries retained',
       'no build/game/test results invented'])

vm_text=(ROOT/'cs-t0/accept-t09-b-live.py').read_text(encoding='utf-8-sig')
record('migration_runtime_default_path',r'\\tsclient\F\projects_isaac\issac_multiplay\cs-t0\vm_share' in vm_text,
       explicit_guest_override='--vm-guest-share')
record('root_build_and_embedded_source_inputs_present',all((ROOT/p).is_file() for p in (
    'Directory.Build.props','VERSION','cs-t0/Cargo.toml','cs-t0/Cargo.lock','cs-t0/rust-toolchain.toml',
    'cs-t0/native/IsaacCsT0.Native.vcxproj','cs-t0/native/IsaacCsT0.Helper.vcxproj')),
    claim='static file availability, not compilation')
failures=[row['name'] for row in checks if not row['passed']]
result={'transaction':'tx-03-09','time_utc':datetime.now(timezone.utc).isoformat(),
    'status':'STATIC_REVIEW_PASS' if not failures else 'STATIC_REVIEW_FAIL',
    'root':str(ROOT),'review_entry':str(Path(__file__).relative_to(ROOT)),
    'checks':checks,'failures':failures,'hashes_computed':False,'builds_run':False,
    'game_operations':False,'code_tests_run':False,'automatic_retry':False,
    'scope':'one final static documentation and copy review; not E2E/runtime/ABI acceptance'}
OUTPUT.write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'status':result['status'],'checks':len(checks),'failures':failures,'report':str(OUTPUT)},ensure_ascii=False))
sys.exit(0 if not failures else 1)
