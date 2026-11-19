"""第15步诊断：用公开 MCP 跳过启动 TLS 停点，在第一次 HUD 选择检查处保留现场。"""
import json
import sys
import time
from x32dbg_rpc import call

print(call('debug_attach_pid', {'pid': int(sys.argv[1]), 'detach_first': False, 'attach_break': True}), flush=True)
module = call('module_get_main')
base = int(module['base'], 16)
target = base + 0x57dc30
print('module', module, flush=True)
for bp in call('breakpoint_list')['breakpoints']:
    if bp['enabled']:
        try:
            call('breakpoint_disable', {'address': bp['address']})
        except RuntimeError as e:
            print('old breakpoint', bp['address'], str(e), flush=True)
call('breakpoint_set', {'address': hex(target)})
for iteration in range(100):
    state = call('debug_get_state')
    if state['state'] == 'stopped':
        print(state, flush=True)
        break
    if state['state'] == 'running':
        time.sleep(0.2)
        continue
    ip = int(state['eip'], 16)
    if ip == target:
        raw = call('memory_read', {'address': hex(base+0x7e49f4), 'size': 4})['data']
        game = int.from_bytes(bytes.fromhex(raw), 'little')
        raw = call('memory_read', {'address': hex(game+0x1bab4), 'size': 92})
        print('HUD_GAME', hex(game), raw, flush=True)
        break
    bps = call('breakpoint_list')['breakpoints']
    hits = [bp for bp in bps if int(bp['address'], 16) == ip]
    if iteration and not any(bp.get('name', '').startswith('TLS') for bp in hits):
        print('UNKNOWN_STOP', state, flush=True)
        print(call('context_get_snapshot', {'include_stack': True}), flush=True)
        break
    for bp in bps:
        if bp['enabled'] and bp.get('name', '').startswith('TLS'):
            call('breakpoint_disable', {'address': bp['address']})
    print(call('debug_run'), flush=True)
