"""由唯一清单生成 binding、透传 wrapper 和 E2E 清单。

本脚本只生成构建输入；指令特征来自 v1.9.7.12 IDB 的静态导出。
"""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--out', type=Path, required=True)
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
catalog_document = json.loads((ROOT/'mod/binding/hooks.json').read_text(encoding='utf-8'))
catalog = catalog_document['hooks']
patterns = {int(f['rva'], 16): f['pattern'] for f in json.loads(
    (ROOT/'mod/binding/v1_9_7_12-static.json').read_text(encoding='utf-8'))['functions']}
binding = json.loads((ROOT/'docs/hooks/repentogon-binding.json').read_text(encoding='utf-8'))
source = ['// 由 gen_binding.py 生成，请修改 binding/hooks.json。',
          '#include "runtime.h"', '#include "IsaacRepentance.h"', '#include "hooks/cs_hook.h"',
          '#include "binding/offsets.h"', '#include "game/bridge.h"',
          '// 自有定义仅借用类型表达 ABI，不构造这些对象。',
          'struct CsStatic {};', 'struct CsEntityList {};',
          'struct CsLifecycle {};', 'struct CsDevice {};', 'struct CsButtonMaps {};']
manifest = []
for row in catalog:
    ident, group, cls, method, ret, params, call, rv, owner, static, conv, own, priority, required = row
    name = ('IsaacCS::'+method) if own else cls+'::'+method
    rva = int(rv,16)
    pattern = patterns.get(rva, '') if own else ''
    # thiscall 的 this 隐含在 ECX 中，ArgData 只描述显式参数。
    nargs = 0 if not params else params.count(',')+1
    flags = (0 if static else 1) | (2 if conv=='cdecl' else 0) | (4 if ret=='void' else 0)
    macro = 'CS_HOOK_STATIC' if static else 'CS_HOOK_METHOD'
    source.append(f'{macro}({ident}, {group.title()}, "{name}", {cls}, {method}, {priority}, {hex(rva)}, "{pattern}", {str(own).lower()}, {nargs}, {flags}, {owner}, {"__"+conv if conv else ""}, ({params}), {ret}) {{')
    source.append(f'    cs::CallScope scope(record);')
    if ident=='manager_update':
        source += ['    cs::FrameScope frame(scope.main);', f'    super({call});']
    elif ident=='console_command':
        source += ['    if (scope.main && cs::OwnCommand(in)) {',
                   '        try {',
                   '            auto* game = static_cast<unsigned char*>(cs::GamePointer());',
                   '            const bool lua = game && *reinterpret_cast<void**>(game+cs::offset::CommandCapture);',
                   '            auto response = cs::Command(in, lua ? "lua" : "console");',
                   '            // 与 REPENTOGON 一致：返回缓冲和控制台输出二选一。',
                   '            if (out) out->append(response);',
                   '            // Print 捕获分支直接追加字符串并返回，不写控制台历史。',
                   '            else Print(response, 0xFFFFFFFFu, 150);',
                   '        } catch (...) { cs::Trace("command_error", "exception"); }',
                   '        return;', '    }',f'    super({call});']
    else:
        # 此处只桥接主线程适配；原生异常由各层 RAII 观察后继续传播。
        if ident.startswith('pgd_') or ident == 'button_maps':
            persistence_args = ', achievementID' if ident == 'pgd_unlock' else ', eEventCounters, num' if ident == 'pgd_counter' else ''
            source.append(f'    cs::game::PersistenceScope persistence("{ident}", this{persistence_args});')
        if ident=='room_update':
            source.append('    cs::game::SimulationScope simulation(cs::game::SimulationKind::Room, this);')
        elif ident=='odd_helper':
            source.append('    cs::game::SimulationScope simulation(cs::game::SimulationKind::Odd);')
        elif ident=='player_update':
            source.append('    cs::game::PlayerScope player_scope(this);')
        elif ident=='player_native':
            source.append('    cs::game::PlayerNativeReached(this);')
        elif ident=='join_pass':
            source.append('    cs::game::JoinScope join_scope(this);')
        elif ident=='input_update':
            source += ['    cs::game::InputLease input_lease(cs::game::HostBusy());',
                       '    if (!input_lease.Ready()) cs::game::InputLeaseUnavailable();']
        elif ident=='game_exit':
            source.append('    if (scope.main) cs::game::RunTeardownBeginning();')
        elif ident=='device_update':
            source.append('    cs::game::DeviceScope device_scope(this);')
        elif ident=='restore':
            source += ['    if (scope.main) { cs::game::RestoreBeginning(); cs::RunExit(); }']
        consumers={'move':'Movement','bomb':'Bomb','active_item':'Item',
                   'card_pill':'Pill','shoot':'Shooting','drop':'Drop'}
        if ident in consumers:
            source.append(f'    cs::game::ConsumerScope consumer(this, cs::game::ConsumerKind::{consumers[ident]});')
            if ident=='shoot': source.append('    if (!consumer.CallOriginal()) return;')
        source.append(f'    {"auto result = " if ret!="void" else ""}super({call});')
        if ident=='device_update': source.append('    device_scope.Returned(result);')
        if ident=='input_update': source.append('    if (scope.main) cs::game::InputFrameFinished();')
        if ident=='console_check': source.append('    if (scope.main) cs::game::ObserveConsole(result);')
        if ident=='new_room': source.append('    if (scope.main) cs::game::NewRoomReturned();')
        if ident=='game_exit': source.append('    if (scope.main) cs::RunExit();')
        if ret!='void': source.append('    return result;')
    source.append('}')
    originals = [i['id'] for i in binding['items'] if int(i.get('v12',{}).get('target') or '0',16)==rva]
    manifest.append(dict(id=ident, group=group, definition=name, rva=hex(rva), priority=priority,
                         mechanism='ZHL_OWN' if own else 'ZHL_REPENTOGON', required_nonzero=required,
                         owner=owner, armed='session', inactive='handle cs; other commands super exactly once' if ident=='console_command' else 'super exactly once', prototype=originals,
                         evidence='STATIC_CONFIRMED'))
(args.out/'hooks_generated.cpp').write_text('\n'.join(source)+'\n', encoding='utf-8')
device = [(r,patterns[r]) for r in [0x5EC570,0x5EC6A0,0x5EEB40,0x5EEBB0]]
(args.out/'binding_v1_9_7_12.inc').write_text('\n'.join(
    f'{{0x{r:X}, "{p}"}},' for r,p in device)+'\n', encoding='utf-8')
# 字节补丁的地址与完整指令预算同样只来自这一张清单。
patch = catalog_document['byte_patches'][0]
expected = [int(value,16) for value in patch['expected'].split()]
(args.out/'permanent_patches.inc').write_text(
    '// 由唯一 Hook 清单生成，勿手改。\n' +
    f'inline constexpr uintptr_t OddCallRva = {patch["rva"]};\n' +
    f'inline constexpr uintptr_t OddResumeRva = {patch["resume"]};\n' +
    f'inline constexpr const char* OddPattern = "{patch["expected"]}";\n' +
    'inline constexpr unsigned char OddExpected[] = {' + ','.join(hex(value) for value in expected) + '};\n', encoding='utf-8')
# Host 补丁同样只从 hooks.json 生成，shim 顺序与此表一一对应。
host_patches = catalog_document['byte_patches'][1:]
host_lines = ['// 由唯一 Hook 清单生成，勿手改。',
    'struct HostPatchSpec { const char* id; uintptr_t rva, resume, alternate; const char* expected; unsigned size; bool call; };',
    'inline constexpr HostPatchSpec HostPatches[] = {']
for row in host_patches:
    host_lines.append('  {"%s", %s, %s, %s, "%s", %s, %s},' % (
        row['id'], row['rva'], row['resume'], row['alternate'], row['expected'],
        row['size'], 'true' if row['kind']=='call' else 'false'))
host_lines.append('};')
host_lines += ['struct HostDependencySpec { const char* id; uintptr_t rva; const char* expected; };',
               'inline constexpr HostDependencySpec HostDependencies[] = {']
for row in catalog_document.get('native_dependencies', []):
    host_lines.append('  {"%s", %s, "%s"},' % (row['id'], row['rva'], row['expected']))
host_lines.append('};')
(args.out/'host_patches.inc').write_text('\n'.join(host_lines)+'\n', encoding='utf-8')
(args.out/'hook-manifest.json').write_text(json.dumps({'tx':'tx-04-06','hooks':manifest,
    'pointer_hooks':catalog_document['pointer_hooks'],
    'native_dependencies':catalog_document.get('native_dependencies', []),
    'byte_patches':catalog_document['byte_patches']},indent=2)+'\n',encoding='utf-8')
md = ['# tx-04-06 Hook manifest', '', 'Generated from mod/binding/hooks.json. Session controls gameplay activation.', '',
      '| id | group | definition | RVA | priority | required nonzero |', '|---|---|---|---|---|---|']
md += [f"| {h['id']} | {h['group']} | {h['definition']} | {h['rva']} | {h['priority']} | {h['required_nonzero']} |" for h in manifest]
md += ['', 'Pointer hooks: Keyboard/Gamepad slots 8, 9, 10, 35. Permanent.',
       f'OddNormal: {patch["rva"]}, expected {patch["expected"]}, resume {patch["resume"]}. Permanent.']
(args.out/'hook-manifest.md').write_text('\n'.join(md)+'\n',encoding='utf-8')

with (args.out/'hook-manifest.md').open('a', encoding='utf-8') as handle:
    for row in host_patches:
        handle.write('\nHost/K: %s @ %s, expected %s, resume %s. Permanent.\n' % (row['id'], row['rva'], row['expected'], row['resume']))
