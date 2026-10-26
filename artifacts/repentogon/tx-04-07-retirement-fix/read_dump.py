"""只读解析已发生的 e2e-02 minidump；不启动/附着游戏，不运行功能测试。"""
import json
import pathlib
import struct
import sys

data = pathlib.Path(sys.argv[1]).read_bytes()


def u32(offset):
    return struct.unpack_from('<I', data, offset)[0]


def u64(offset):
    return struct.unpack_from('<Q', data, offset)[0]


streams = {u32(o): (u32(o + 4), u32(o + 8))
           for o in range(u32(12), u32(12) + 12 * u32(8), 12)}
modules = []
directory = streams[4][1]
for offset in range(directory + 4, directory + 4 + 108 * u32(directory), 108):
    name = u32(offset + 20)
    modules.append((u64(offset), u32(offset + 8),
                    data[name + 4:name + 4 + u32(name)].decode('utf-16le')))

# MemoryList 和 Memory64List 是转储中的原有内存片段；缺失内容保持 null。
memory = []
if 5 in streams:
    directory = streams[5][1]
    for offset in range(directory + 4, directory + 4 + 16 * u32(directory), 16):
        memory.append((u64(offset), u32(offset + 8), u32(offset + 12)))
if 9 in streams:
    directory = streams[9][1]
    position = u64(directory + 8)
    for offset in range(directory + 16, directory + 16 + 16 * u64(directory), 16):
        address, size = u64(offset), u64(offset + 8)
        memory.append((address, size, position))
        position += size


def read(address, size):
    for start, length, position in memory:
        if start <= address and address + size <= start + length:
            return data[position + address - start:position + address - start + size]
    return None


def word(address):
    value = read(address, 4)
    return struct.unpack('<I', value)[0] if value is not None else None


def vector(address):
    value = read(address, 12)
    if value is None:
        return None
    begin, end, capacity = struct.unpack('<III', value)
    count = (end - begin) // 4 if begin <= end <= capacity and (end - begin) % 4 == 0 else None
    return {'address': hex(address), 'begin': hex(begin), 'end': hex(end),
            'capacity': hex(capacity), 'count': count,
            'players': [hex(word(begin + i * 4)) if word(begin + i * 4) is not None else None
                        for i in range(count)] if count is not None and count <= 16 else None}


base = next(base for base, _, name in modules
            if pathlib.PureWindowsPath(name).name.lower() == 'isaac-ng.exe')
exception = streams[6][1]
context = u32(exception + 164)
registers = {name: u32(context + offset) for name, offset in
             {'edi': 156, 'esi': 160, 'ebx': 164, 'edx': 168, 'ecx': 172,
              'eax': 176, 'ebp': 180, 'eip': 184, 'esp': 196}.items()}
game = word(base + 0x7E49F4)
manager = word(base + 0x7E4A1C)
result = {
    'source': sys.argv[1], 'game_base': hex(base),
    'exception_code': hex(u32(exception + 8)),
    'exception_address': hex(u64(exception + 24)),
    'exception_game_rva': hex(u64(exception + 24) - base),
    'exception_parameters': [hex(u64(exception + 40 + i * 8))
                             for i in range(min(u32(exception + 32), 15))],
    'registers': {name: hex(value) for name, value in registers.items()},
    'game': hex(game) if game is not None else None,
    'global_manager': hex(manager) if manager is not None else None,
    'wrong_this_equals_global_manager': registers['ebx'] == manager,
    'wrong_this_as_player_vector': vector(registers['ebx']),
    'actual_game_player_manager': vector(game + 0x1BAA8) if game is not None else None,
    'note': 'Read-only dump analysis; no live process, no hash, no inferred PASS.'
}
print(json.dumps(result, indent=2))
