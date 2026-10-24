"""Independent wire-v2 reference fixtures, authored before implementation.

Not a production decoder. Invoked only by the single final E2E executor.
"""
import json
import random
import struct
import zlib
from pathlib import Path

SESSION = bytes([99]) + bytes(15)
DOMAIN = b'isaac.cs.t0.observation.v1'
BUILD = b'isaac-t0-tx-02-00'


def hello(role):
    local, peer = (11, 22) if role == 1 else (22, 11)
    return struct.pack('<32s32s16s16sHHHHQIIQQ', BUILD, DOMAIN,
                       bytes([local]), bytes([peer]), 2, 1, 2, role, 15, 1200, 0, 101, 202)


def packet(kind, role, seq, payload):
    header = struct.pack('<4sHHHHI16sQQIIII', b'ICT8', 2, kind, 64, role, 0, SESSION,
                         101 if role == 1 else 202, seq, len(payload), 0, 0, 0)
    return repair(header + payload)


def repair(raw):
    raw = bytearray(raw)
    raw[56:60] = bytes(4)
    struct.pack_into('<I', raw, 56, zlib.crc32(raw))
    return bytes(raw)


def command(sequence, target):
    state = struct.pack('<HHIIhhhhI', 2, 24, 0, 8 if sequence == 1 else 0, 4096, 0, 0, 0, 0)
    return SESSION + struct.pack('<QQQ', 202, sequence, target) + state


def golden():
    first_ack = struct.pack('<8QIHHQ', 0, 202, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1) + hello(1)
    progress = struct.pack('<8QIHHQ', 10, 202, 4, 2, 10, 0, 1, 1, 128, 3, 2, 0) + hello(1)
    partial = bytearray(96)
    struct.pack_into('<32s5Q5HHIQ', partial, 0, DOMAIN, 10, 1, 1, 10, 2, 1, 1, 1, 15, 1, 0, 0, 10)
    value = struct.pack('<QQHBBHH16s', 1, 1, 11, 0, 1, 4, 0, struct.pack('<f', 1.0))
    gap = bytearray(96)
    struct.pack_into('<32s5Q5HHIQ', gap, 0, DOMAIN, 11, 1, 1, 11, 0, 1, 2, 2, 2, 0, 0, 0, 11)
    return {
        'hello-authority': packet(1, 1, 1, hello(1)),
        'hello-remote': packet(1, 2, 1, hello(2)),
        'first-ack': packet(3, 1, 2, first_ack),
        'input-one': packet(2, 2, 2, struct.pack('<HHIQ', 1, 0, 0, 2) + command(1, 9)),
        'input-four': packet(2, 2, 5, struct.pack('<HHIQ', 4, 0, 0, 2) + b''.join(command(i, 8+i) for i in range(1, 5))),
        'progress-neutral': packet(3, 1, 3, progress),
        'partial-values': packet(4, 1, 4, partial + value),
        'odd-gap': packet(4, 1, 5, gap),
    }


def write_corpus(directory: Path):
    directory.mkdir(parents=True, exist_ok=False)
    vectors = []
    good = golden()
    for name, raw in good.items():
        vectors.append((name, True, raw))
        for length in range(len(raw)):
            vectors.append((f'{name}.truncate.{length}', False, raw[:length]))
        for extra in (1, 2, 7, 64, 1201):
            vectors.append((f'{name}.extend.{extra}', False, raw + bytes(extra)))
        for offset in range(len(raw)):
            for bit in range(8):
                changed = bytearray(raw)
                changed[offset] ^= 1 << bit
                vectors.append((f'{name}.flip.{offset}.{bit}', False, bytes(changed)))
        for offset, width, value in [(4, 2, 1), (6, 2, 99), (8, 2, 63), (10, 2, 9),
                                      (12, 4, 1), (40, 8, 0), (40, 8, 2**64-1),
                                      (52, 4, 1), (60, 4, 1)]:
            changed = bytearray(raw)
            changed[offset:offset+width] = value.to_bytes(width, 'little')
            vectors.append((f'{name}.semantic.{offset}.{value}', False, repair(changed)))
    raw = good['input-four']
    for name, offset, width, value in [('count-zero',64,2,0), ('count-five',64,2,5),
            ('reserved-count',66,2,1), ('reserved-input',68,4,1),
            ('redundant-canonical',80+64+60,4,1), ('redundant-client',80+64+16,8,303),
            ('redundant-sequence',80+64+24,8,1)]:
        changed=bytearray(raw);changed[offset:offset+width]=value.to_bytes(width,'little')
        vectors.append(('input-four.semantic.'+name,False,repair(changed)))
    semantic={
        'hello-authority':[(164,2,1),(166,2,2),(180,4,1)],
        'input-one':[(104,8,0),(112,8,0),(120,2,1),(124,4,2**31)],
        'progress-neutral':[(88,8,5),(112,8,0),(120,8,0),(132,2,99),(134,2,9)],
        'first-ack':[(132,2,3)],
        'partial-values':[(140,2,0),(178,1,1),(179,1,2),(180,2,16),(182,2,1),(184,4,0x7fc00000),(188,1,1)],
    }
    for key,changes in semantic.items():
        for offset,width,value in changes:
            changed=bytearray(good[key]);changed[offset:offset+width]=value.to_bytes(width,'little')
            vectors.append((f'{key}.payload.{offset}.{value}',False,repair(changed)))
    changed=bytearray(good['hello-authority']);changed[64:96]=b'x'*32
    vectors.append(('hello.unterminated-build',False,repair(changed)))
    legacy=bytearray(good['hello-authority']);legacy[:4]=b'ISCS'
    vectors.append(('legacy-iscs',False,repair(legacy)))
    rng=random.Random(0x20008)
    for index in range(1024):
        raw=bytes(rng.randrange(256) for _ in range(rng.randrange(1401)))
        vectors.append((f'fuzz.{index}',False,raw))
    (directory/'golden.json').write_text(json.dumps({k:v.hex() for k,v in good.items()},indent=2)+'\n',encoding='utf-8')
    (directory/'vectors.txt').write_text(''.join(f'{name} {int(valid)} {raw.hex() or "-"}\n' for name,valid,raw in vectors),encoding='utf-8')
    return len(vectors)
