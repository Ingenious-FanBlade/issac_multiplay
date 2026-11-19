"""第15步临时控制口读数：输出指定命令的键值，不保存逐帧材料。"""
import json
import socket
import sys

port = int(sys.argv[1])
command = sys.argv[2]
keys = sys.argv[3:]
with socket.create_connection(('127.0.0.1', port), timeout=5) as connection:
    connection.settimeout(60)
    connection.sendall((command + '\n').encode())
    values = {}
    with connection.makefile('r', encoding='utf-8') as response:
        for line in response:
            line = line.rstrip('\r\n')
            if line == '.':
                break
            if '=' in line:
                key, value = line.split('=', 1)
                if not keys or any(part in key for part in keys):
                    values[key] = value
    print(json.dumps(values, ensure_ascii=False, indent=2))
