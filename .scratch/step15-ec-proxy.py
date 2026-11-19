"""仅转发隔离双端的原包。在外层 World 通道按固定随机种子丢包或延迟。"""
import heapq
import json
import random
import select
import socket
import struct
import time

host = ("127.0.0.1", 27090)
front = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
front.bind(("127.0.0.1", 27110))
back = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
back.bind(("127.0.0.1", 0))
control = socket.socket()
control.bind(("127.0.0.1", 27111))
control.listen(2)
for endpoint in (front, back, control):
    endpoint.setblocking(False)

rng = random.Random(84)
mode = "clear"
client = None
counts = dict(world=0, dropped=0, delayed=0, down_bytes=0, up_bytes=0)
delayed = []
running = True
print("proxy ready", flush=True)
while running:
    for endpoint in select.select((front, back, control), (), (), .002)[0]:
        if endpoint is control:
            peer, _ = control.accept()
            with peer:
                peer.settimeout(1)
                request = peer.makefile('rb').readline(32).decode().strip()
                if request in ("clear", "loss", "delay"):
                    mode = request
                elif request == "reconnect":
                    client = None
                    delayed.clear()
                elif request == "stop":
                    running = False
                peer.sendall((json.dumps(dict(mode=mode, client=client, **counts))+"\n").encode())
            continue
        for _ in range(64):
            try:
                data, address = endpoint.recvfrom(65536)
            except BlockingIOError:
                break
            except ConnectionResetError:
                # Windows 在隔离实例关闭时可能报告上一次 UDP 发送没有接收端。
                continue
            if endpoint is front:
                if client is None:
                    client = address
                if address == client:
                    counts["up_bytes"] += len(data)+28
                    back.sendto(data, host)
            elif address == host and client is not None:
                counts["down_bytes"] += len(data)+28
                # 状态正文的 ICRF 外层中 Frame/WorldDynamic 为 21/4。
                world = len(data)>=64 and data[:4]==b"ICRF" and struct.unpack_from("<HB", data, 48)==(21,4)
                if world:
                    counts["world"] += 1
                    if mode=="loss" and rng.random()<.10:
                        counts["dropped"] += 1
                        continue
                    if mode=="delay":
                        counts["delayed"] += 1
                        heapq.heappush(delayed,(time.monotonic()+.060,data,client))
                        continue
                front.sendto(data,client)
    now=time.monotonic()
    while delayed and delayed[0][0]<=now:
        _,data,address=heapq.heappop(delayed)
        if address==client:
            front.sendto(data,address)
