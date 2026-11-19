"""第15步临时实机采样：只读取正式控制口，按墙钟计算实际发布与接受频率。"""
import json
import socket
import sys
import time

def query(port, command):
    with socket.create_connection(('127.0.0.1', port), timeout=5) as connection:
        connection.settimeout(10)
        connection.sendall((command + '\n').encode())
        values = {}
        with connection.makefile('r', encoding='utf-8') as response:
            for line in response:
                line = line.rstrip('\r\n')
                if line == '.':
                    return values
                if '=' in line:
                    key, value = line.split('=', 1)
                    values[key] = value
        raise RuntimeError('control closed before response terminator')

def snapshot():
    result = {}
    for page in (0, 1):
        result.update(query(27092, f'cs stream client {page}'))
    return time.perf_counter(), result

if __name__ == '__main__':
    client_before = query(27092, 'cs status')
    start, before = snapshot()
    time.sleep(float(sys.argv[1]) if len(sys.argv) > 1 else 15)
    end, after = snapshot()
    seconds = end - start
    summary = {'seconds': seconds, 'streams': {}}
    for stream in ('world', 'body'):
        prefix = f'stream.binary.{stream}.'
        values = {}
        for key, value in after.items():
            if not key.startswith(prefix):
                continue
            short = key[len(prefix):]
            if short.endswith('.n'):
                delta = int(value) - int(before.get(key, 0))
                values[short] = {'delta': delta, 'hz': round(delta / seconds, 2)}
            elif short.endswith('.sum_us'):
                values[short] = int(value) - int(before.get(key, 0))
            elif short.startswith('wait.') or '.' not in short:
                values[short] = int(value) - int(before.get(key, 0))
            elif any(part in short for part in ('first.', 'install.', 'manager.', 'end_to_end.')):
                values[short] = value
        summary['streams'][stream] = values
    summary['holds_and_memory'] = {k: v for k, v in after.items() if k.startswith(('stream.binary.holds.', 'stream.binary.peer.', 'stream.binary.display.', 'stream.binary.shadow_blocks.'))}
    summary['client'] = {k: v for k, v in query(27092, 'cs status').items() if k.startswith('client.') or k in ('client_state', 'client_world_applied', 'last_error', 'loop')}
    summary['input_delta'] = {k: int(summary['client'][k]) - int(client_before[k]) for k in ('client.sent', 'client.accepted', 'client.processed')}
    print(json.dumps(summary, ensure_ascii=False, indent=2))
