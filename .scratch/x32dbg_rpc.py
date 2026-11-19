"""第15步临时 MCP 客户端：只调用服务器公开的工具，供续测诊断使用。"""
import json
import sys
import urllib.request

def call(name, arguments=None):
    body = json.dumps({'jsonrpc': '2.0', 'id': 1, 'method': 'tools/call',
                       'params': {'name': name, 'arguments': arguments or {}}}).encode()
    req = urllib.request.Request('http://127.0.0.1:3000/mcp', data=body,
                                 headers={'Content-Type': 'application/json'})
    with urllib.request.urlopen(req, timeout=35) as reply:
        result = json.load(reply)
    if 'error' in result:
        raise RuntimeError(result['error'])
    blocks = result['result'].get('content', [])
    values = []
    for block in blocks:
        if block.get('type') == 'text':
            try:
                values.append(json.loads(block['text']))
            except ValueError:
                values.append(block['text'])
    return values[0] if len(values) == 1 else values

if __name__ == '__main__':
    print(json.dumps(call(sys.argv[1], json.loads(sys.argv[2]) if len(sys.argv)>2 else {}), ensure_ascii=False, indent=2))
