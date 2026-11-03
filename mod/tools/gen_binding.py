"""由唯一清单生成 binding、透传 wrapper、world 观察 wrapper 与布局清单。

数据来源（本脚本是唯一生成入口；生成期做结构化校验，任一项失败即非零退出）：

  mod/binding/hooks.json             唯一 Hook 清单：
                                     - `hooks`：41 条既有位置式行（栈参数 ABI）
                                     - `world_observations`：19 项 world 观察入口
  mod/binding/world_layout.json      world 只读布局清单（字段/来源/类别/RTTI/常量）
  mod/binding/v1_9_7_12-static.json  目标版本特征字节（自有定义使用）
  docs/hooks/repentogon-binding.json 旧点位索引（只用于填充 prototype 字段）

生成产物（写入 --out 目录）：

  hooks_generated.cpp                全部 Hook wrapper（既有 + world）
  binding_v1_9_7_12.inc              设备依赖特征
  permanent_patches.inc              Odd 普通点补丁
  host_patches.inc                   Host/K 补丁与原生依赖
  hook_capacity.h                    待注册表容量与行数（注册期显式报告）
  world_layout.inc                   字段/来源/类别/观察/常量表
  hook-manifest.json                 机器可读 Hook + 布局清单
  hook-manifest.md                   同一清单的阅读版本
  world-layout.json                  布局清单的规范化导出（E2E / 离线工具）

本脚本只生成构建输入；不运行游戏、不跑测试、不校验哈希。指令特征来自
v1.9.7.12 IDB 的静态导出；world 布局证据见 docs/research/tx-04-08-*。
"""

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HOOKS_PATH = ROOT / 'mod/binding/hooks.json'
LAYOUT_PATH = ROOT / 'mod/binding/world_layout.json'
PATTERNS_PATH = ROOT / 'mod/binding/v1_9_7_12-static.json'
BINDING_PATH = ROOT / 'docs/hooks/repentogon-binding.json'

# --- 与 C++ 侧一致的表 -----------------------------------------------------
# world/observation.h 的枚举顺序。
OBSERVATION_ORDER = [
    'Factory', 'Init', 'Publish', 'Remove', 'ClearReferences', 'Destroy',
    'PlayerCtor', 'PlayerCreate', 'ManagerRestore', 'Replace', 'PlayerRestore',
    'NpcMorph', 'PickupMorph', 'GridFactory', 'GridInit', 'GridSpawn', 'Door',
    'GridFree', 'Spawn', 'GridCtor',
]
# world/model.cpp 的 kClassNames（含 UnknownEntity）。
CLASS_ORDER = ['Player', 'NPC', 'Tear', 'Projectile', 'Pickup', 'Familiar',
               'Effect', 'Grid', 'UnknownEntity']
# world/model.cpp 的 kSourceNames。
SOURCE_ORDER = ['main', 'persistent', 'update', 'render', 'effect', 'temp',
                'bufferTemplate', 'wisp', 'roster', 'esau', 'player_relations',
                'grid', 'door', 'reserved_13', 'reserved_14']
# world_layout.h 的 RttiClass 顺序。
RTTI_ORDER = ['Entity', 'Player', 'NPC', 'Tear', 'Projectile', 'Pickup',
              'Familiar', 'Effect', 'Grid', 'Door', 'Decoration', 'Rock', 'Pit',
              'Spikes', 'Web', 'Lock', 'TNT', 'Fire', 'Poop', 'Wall', 'TrapDoor',
              'Stairs', 'Gravity', 'PressurePlate', 'Statue', 'Teleporter']
ENCODINGS = {'Raw': 0, 'F32': 1, 'Reference': 2, 'Derived': 3}
FIELD_STATUS = {'InitConfirmed': 0, 'InitDefault': 1, 'InitUnknown': 2,
                'InitAssumed': 3, 'LuaBinding': 4, 'Derived': 5}
FIELD_BASES = {'entity': 0, 'grid': 1, 'game': 2, 'derived': 3}
SOURCE_KINDS = {'entity_list': 0, 'roster': 1, 'fixed_array': 2, 'relation': 3,
                'reserved': 4}
GP_REGISTERS = {'eax': 0, 'ecx': 1, 'edx': 2, 'ebx': 3, 'esp': 4, 'ebp': 5,
                'esi': 6, 'edi': 7}
XMM_REGISTERS = {'xmm%d' % i: i for i in range(9)}
# 与 world_layout.h 的 Constants 成员顺序一致；生成代码用指定初始化器，
# 名称或顺序不一致会直接编译失败。
CONSTANT_ORDER = [
    ('sprite_null_names_offset', 'u32'),
    ('sprite_event_names_offset', 'u32'),

    ('entity_text_td_rva', 'u32'),
    ('entity_dummy_td_rva', 'u32'),
    ('entity_text_counter_offset', 'u32'),
    ('entity_text_duration_offset', 'u32'),
    ('entity_text_buffer_offset', 'u32'),
    ('entity_text_capacity', 'u32'),
    ('entity_text_color_offset', 'u32'),
    ('entity_text_variant_offset', 'u32'),
    ('entity_text_writer_return_rva', 'u32'),
    ('entity_laser_td_rva', 'u32'),
    ('entity_laser_sample_mode_offset', 'u32'),
    ('entity_laser_start_offset_mode_offset', 'u32'),

    ('game_root_rva', 'u32'),
    ('game_frame_offset', 'u32'),
    ('game_room_offset', 'u32'),
    ('game_room_index_offset', 'u32'),
    ('game_dimension_offset', 'u32'),
    ('game_object_size', 'u32'),
    ('room_object_size', 'u32'),
    ('player_manager_object_size', 'u32'),
    ('game_player_manager_offset', 'u32'),
    ('room_grid_width_offset', 'u32'),
    ('room_grid_height_offset', 'u32'),
    ('room_entity_list_offset', 'u32'),
    ('room_entity_list_first_el_offset', 'u32'),
    ('room_entity_list_stride', 'u32'),
    ('room_entity_list_count', 'u32'),
    ('room_grid_slot_offset', 'u32'),
    ('room_grid_slot_count', 'u32'),
    ('room_grid_max_index', 'u32'),
    ('room_door_slot_offset', 'u32'),
    ('room_door_slot_count', 'u32'),
    ('room_door_grid_map_offset', 'u32'),
    ('room_constructor_rva', 'u32'),
    ('room_constructor_clear_offset', 'u32'),
    ('room_constructor_clear_bytes', 'u32'),
    ('player_manager_roster_offset', 'u32'),
    ('player_manager_esau_offset', 'u32'),
    ('esau_slot_count', 'u32'),
    ('relation_node_budget', 'u32'),
    ('entity_flags_offset', 'u32'),
    ('entity_hp_offset', 'u32'),
    ('entity_max_hp_offset', 'u32'),
    ('entity_frame_stamp_offset', 'u32'),
    ('grid_position_step', 'f32'),
    ('grid_position_origin_x', 'f32'),
    ('grid_position_origin_y', 'f32'),
    ('grid_position_getter_rva', 'u32'),
    ('grid_index_by_tile_rva', 'u32'),
    ('rtti_complete_object_locator_size', 'u32'),
    ('rtti_class_hierarchy_descriptor_size', 'u32'),
    ('rtti_base_class_descriptor_size', 'u32'),
    ('rtti_expected_signature', 'u32'),
    ('rtti_expected_hierarchy_attributes', 'u32'),
    ('rtti_expected_base_attributes', 'u32'),
    ('entity_list_descriptor_size', 'u32'),
    ('entity_list_descriptor_data_offset', 'u32'),
    ('entity_list_descriptor_capacity_offset', 'u32'),
    ('entity_list_descriptor_size_offset', 'u32'),
    ('roster_vector_size', 'u32'),
    ('roster_vector_begin_offset', 'u32'),
    ('roster_vector_end_offset', 'u32'),
    ('roster_vector_capacity_offset', 'u32'),
]
# 同址入口只准具名的 Client 主动外层与既有观察层配对；Player 另有内层见证。
ALLOWED_SAME_RVA = {
    0x2F6D30: {'client_game_update','game_update'},
    0x446C90: {'client_post_update','post_update'},
    0x3F7F60: {'client_room_update','room_update'},
    0x13F00: {'client_entity_list','entity_list'},
    0x37BD40: {'client_player_update','player_update','player_native'},
    0x2F9340: {'client_odd_helper','odd_helper'},
    0x509100: {'client_room_lifecycle','room_lifecycle'},
    0x426430: {'client_door_update','door_update'},
    0x2FA0F0: {'client_door_nested','door_nested'},
    0x44B930: {'client_new_room','new_room'},
}


class Problems:
    """收集生成期问题；任意一项都让生成失败，绝不静默跳过。"""

    def __init__(self):
        self.items = []

    def add(self, where, message):
        self.items.append('%s: %s' % (where, message))

    def finish(self):
        if not self.items:
            return
        sys.stderr.write('gen_binding: 清单校验失败（%d 项）\n' % len(self.items))
        for item in self.items:
            sys.stderr.write('  - %s\n' % item)
        sys.exit(2)


def load_json(path, problems):
    """读 JSON；返回 None 表示已有错误记录，调用方不得继续使用。"""
    try:
        return json.loads(path.read_text(encoding='utf-8'))
    except FileNotFoundError:
        problems.add(str(path), '文件不存在')
    except json.JSONDecodeError as error:
        problems.add(str(path), 'JSON 语法错误: %s' % error)
    return None


def parse_int(value, where, problems):
    """接受 JSON 整数或 "0x.."/十进制字符串；其他形态记为错误。"""
    if isinstance(value, bool):
        problems.add(where, '布尔值不能作为数量/偏移')
        return None
    if isinstance(value, int):
        return value
    if isinstance(value, str):
        try:
            return int(value.strip(), 0)
        except ValueError:
            problems.add(where, '无法解析的整数: %r' % value)
            return None
    problems.add(where, '期望整数，得到 %r' % (value,))
    return None


def c_string(text):
    """把任意文本转成安全的 C++ 字符串字面量。"""
    escaped = str(text).replace('\\', '\\\\').replace('"', '\\"')
    escaped = escaped.replace('\r', ' ').replace('\n', '\\n')
    return '"%s"' % escaped


def hex_text(value):
    """清单导出的统一十六进制文本（大写、0x 前缀）。"""
    return '0x%X' % (value or 0)


def split_params(text):
    """按顶层逗号切分参数列表，跳过 <> 与 () 内的逗号。"""
    text = (text or '').strip()
    if not text:
        return []
    parts, depth, current = [], 0, []
    for char in text:
        if char in '<(':
            depth += 1
        elif char in '>)':
            depth = max(0, depth - 1)
        if char == ',' and depth == 0:
            parts.append(''.join(current).strip())
            current = []
        else:
            current.append(char)
    if current:
        parts.append(''.join(current).strip())
    return [part for part in parts if part]


# ===========================================================================
# 既有 hooks 行
# ===========================================================================

LEGACY_GROUPS = {'loop', 'input', 'player', 'join', 'control', 'device', 'observe', 'world', 'client'}


def parse_legacy_rows(rows, problems):
    parsed = []
    if not isinstance(rows, list):
        problems.add('hooks.json.hooks', '必须是数组')
        return parsed
    for index, row in enumerate(rows):
        where = 'hooks[%d]' % index
        if not isinstance(row, list) or len(row) != 14:
            problems.add(where, '位置式行必须是 14 列')
            continue
        (ident, group, cls, method, ret, params, call, rv, owner, static,
         conv, own, priority, required) = row
        if not isinstance(ident, str) or not ident:
            problems.add(where, 'id 必须是非空字符串')
            continue
        where = 'hooks[%d](%s)' % (index, ident)
        if group not in LEGACY_GROUPS:
            problems.add(where, 'group 非法: %r' % (group,))
        if conv not in ('', 'cdecl', 'stdcall'):
            problems.add(where, 'conv 非法: %r' % (conv,))
        if not isinstance(static, bool) or not isinstance(own, bool):
            problems.add(where, 'static/own 必须是布尔值')
            continue
        if not isinstance(required, bool):
            problems.add(where, 'required 必须是布尔值')
        if conv == 'cdecl' and not static:
            problems.add(where, '既有行 schema 不能表达成员 cdecl；请用 world_observations')
        rva = parse_int(rv, where + '.rva', problems)
        priority_value = parse_int(priority, where + '.priority', problems)
        if rva is None or priority_value is None:
            continue
        if not 0 < rva < 0x800000:
            problems.add(where, 'RVA 超出映像范围: %#x' % rva)
        parsed.append(dict(
            kind='legacy', id=ident, group=group, cls=cls, method=method,
            ret=ret, params=params, call=call, rva=rva, owner=owner,
            static=static, conv=conv, own=own, priority=priority_value,
            required=bool(required), nargs=len(split_params(params)),
            name=('IsaacCS::' + method) if own else cls + '::' + method))
    return parsed


# ===========================================================================
# world 观察入口
# ===========================================================================

def parse_world_rows(doc, problems):
    rows = doc.get('world_observations')
    if not isinstance(rows, list):
        problems.add('hooks.json', '缺少 world_observations 数组')
        return []
    parsed, seen_observations = [], []
    for index, row in enumerate(rows):
        where = 'world_observations[%d]' % index
        if not isinstance(row, dict):
            problems.add(where, '必须是对象')
            continue
        ident = row.get('id')
        if not isinstance(ident, str) or not ident:
            problems.add(where, 'id 必须是非空字符串')
            continue
        where = 'world_observations[%d](%s)' % (index, ident)
        observation = row.get('observation')
        if observation not in OBSERVATION_ORDER:
            problems.add(where, 'observation 必须是 20 项观察语义之一: %r'
                         % (observation,))
            continue
        seen_observations.append(observation)
        if row.get('group') != 'world':
            problems.add(where, 'group 必须是 world')
        if not isinstance(row.get('shared_control', False), bool):
            problems.add(where, 'shared_control 必须是布尔值')
        cls, method = row.get('class'), row.get('method')
        definition = row.get('definition')
        if not all(isinstance(value, str) and value
                   for value in (cls, method, definition)):
            problems.add(where, 'class/method/definition 必须是非空字符串')
            continue
        own, static = row.get('own'), row.get('static')
        if not isinstance(own, bool) or not isinstance(static, bool):
            problems.add(where, 'own/static 必须是布尔值')
            continue
        if not isinstance(row.get('required'), bool):
            problems.add(where, 'required 必须是布尔值')
        conv = row.get('conv')
        if conv not in ('thiscall', 'cdecl'):
            problems.add(where, 'conv 必须是 thiscall 或 cdecl')
            continue
        if static != (conv == 'cdecl'):
            problems.add(where, 'static 与 conv 必须一致：静态入口是 cdecl，成员入口是 thiscall')
        expected_name = ('IsaacCS::' + method) if own else cls + '::' + method
        if definition != expected_name:
            problems.add(where, 'definition 必须是 %r（own=%s）'
                         % (expected_name, own))
        rva = parse_int(row.get('rva'), where + '.rva', problems)
        priority_value = parse_int(row.get('priority'), where + '.priority', problems)
        if rva is None or priority_value is None:
            continue
        pattern = row.get('pattern', '')
        if not isinstance(pattern, str):
            problems.add(where, 'pattern 必须是字符串')
            continue
        if own and not pattern:
            problems.add(where, '自有定义必须给出已核对的入口特征字节')
        if not own and pattern:
            problems.add(where, '官方定义不使用 pattern，请留空')
        entry_bytes = row.get('entry_bytes', '')
        if not isinstance(entry_bytes, str) or len(entry_bytes.split()) < 5:
            problems.add(where, 'entry_bytes 至少要能表达一条完整指令')
        args = row.get('args')
        if not isinstance(args, list):
            problems.add(where, 'args 必须是数组')
            continue
        arg_specs, arg_ok = [], True
        for ai, arg in enumerate(args):
            awhere = '%s.args[%d]' % (where, ai)
            if not isinstance(arg, dict):
                problems.add(awhere, '必须是对象')
                arg_ok = False
                continue
            arg_type, arg_name, abi = arg.get('type'), arg.get('name'), arg.get('abi')
            if not isinstance(arg_type, str) or not arg_type:
                problems.add(awhere, 'type 必须是非空字符串')
                arg_ok = False
            if not isinstance(arg_name, str) or not re.fullmatch(
                    r'[A-Za-z_]\w*', arg_name or ''):
                problems.add(awhere, 'name 必须是合法标识符')
                arg_ok = False
            if not isinstance(abi, str):
                problems.add(awhere, 'abi 必须是字符串')
                arg_ok = False
                continue
            if abi.startswith('stack:'):
                size = parse_int(abi.split(':', 1)[1], awhere + '.abi', problems)
                if size is None or size < 1:
                    problems.add(awhere, 'stack:N 的 N 必须 >= 1')
                    arg_ok = False
                else:
                    arg_specs.append(dict(kind='stack', size=size, type=arg_type,
                                          name=arg_name, abi=abi))
            elif abi.lower() in GP_REGISTERS:
                arg_specs.append(dict(kind='gp', reg=GP_REGISTERS[abi.lower()],
                                      type=arg_type, name=arg_name,
                                      abi=abi.lower()))
            elif abi.lower() in XMM_REGISTERS:
                arg_specs.append(dict(kind='xmm', reg=XMM_REGISTERS[abi.lower()],
                                      type=arg_type, name=arg_name,
                                      abi=abi.lower()))
            else:
                problems.add(awhere, 'abi 既不是 stack:N 也不是已知寄存器: %r' % abi)
                arg_ok = False
            if re.search(r'\b(float|double)\b', arg_type or ''):
                problems.add(awhere, '本轮 world 观察不登记浮点参数'
                             '（XMM 传参需要真实 ArgData 与语义确认）')
                arg_ok = False
        ret = row.get('return')
        if not isinstance(ret, str) or not ret:
            problems.add(where, 'return 必须是非空字符串（void 也要写）')
            continue
        if re.search(r'long long|int64|uint64|\bfloat\b|\bdouble\b|&\s*$', ret):
            problems.add(where, 'world 观察只登记 void/bool/指针/32 位整数返回值；'
                         '64 位与浮点返回需要 LONGLONG/XMM 约定，本轮不登记')
        return_abi = row.get('return_abi', '')
        if ret == 'void' and return_abi:
            problems.add(where, 'void 入口不应声明 return_abi')
        if ret != 'void' and not return_abi:
            problems.add(where, '非 void 入口必须声明 return_abi')
        derived_flags = ((0 if static else 1) | (2 if conv == 'cdecl' else 0) |
                         (4 if ret == 'void' else 0))
        sdk = row.get('sdk')
        if own and sdk is not None:
            problems.add(where, '自有定义不应带 sdk 元数据')
        if not own:
            if not isinstance(sdk, dict):
                problems.add(where, '官方定义必须登记 sdk.flags/argdata 供交叉校验')
            else:
                sdk_flags = parse_int(sdk.get('flags'), where + '.sdk.flags', problems)
                if sdk_flags is not None and sdk_flags != derived_flags:
                    problems.add(where, 'ABI 推导 flags=%d 与 SDK 元数据 %d 不一致'
                                 % (derived_flags, sdk_flags))
                sdk_args = sdk.get('argdata')
                if not isinstance(sdk_args, list):
                    problems.add(where, 'sdk.argdata 必须是数组')
                else:
                    derived = [('stack:%d' % spec['size']) if spec['kind'] == 'stack'
                               else ('%s:0' % spec['abi']) for spec in arg_specs]
                    if derived != sdk_args:
                        problems.add(where, '参数 ABI 与 SDK ArgData 不一致: 清单 %r / SDK %r'
                                     % (derived, sdk_args))
        if not arg_ok:
            continue
        parsed.append(dict(
            kind='world', id=ident, observation=observation, group='world',
            cls=cls, method=method, definition=definition, own=own,
            static=static, conv=conv, rva=rva, pattern=pattern,
            priority=priority_value, args=arg_specs, ret=ret,
            return_abi=return_abi, entry_bytes=entry_bytes,
            completion=row.get('completion', ''), reach=row.get('reach', ''),
            required=bool(row.get('required')), evidence=row.get('evidence', ''),
            nargs=len(arg_specs), flags=derived_flags, name=definition,
            shared_control=bool(row.get('shared_control', False))))
    if sorted(seen_observations) != sorted(OBSERVATION_ORDER):
        problems.add('world_observations', '必须恰好覆盖 20 项观察语义，当前 %r'
                     % (seen_observations,))
    return parsed


def check_unique(legacy, world, problems):
    seen_ids = set()
    for row in legacy + world:
        if row['id'] in seen_ids:
            problems.add(row['id'], 'id 重复')
        seen_ids.add(row['id'])
    by_rva = {}
    for row in legacy + world:
        by_rva.setdefault(row['rva'], []).append(row)
    for rva, rows in by_rva.items():
        if len(rows) == 1:
            continue
        names = {row['id'] for row in rows}
        if all(row['kind'] == 'legacy' for row in rows) and \
                ALLOWED_SAME_RVA.get(rva) == names:
            continue
        problems.add('rva %#x' % rva,
                     '同址多入口只允许 hooks 中已登记的成对层，当前 %r' % sorted(names))


# ===========================================================================
# 生成：hooks_generated.cpp
# ===========================================================================

GENERATED_HEADER = """// 由 gen_binding.py 生成，请修改 binding/hooks.json。
#include "runtime.h"
#include "IsaacRepentance.h"
#include "hooks/cs_hook.h"
#include "binding/offsets.h"
#include "game/bridge.h"
#include "game/client_objects_native.h"
#include "game/client_grid_native.h"
#include "game/client_presentation_native.h"
#include "game/client_entity_draw_native.h"
#include "game/world_sources/loot.h"
#include "game/world_sources/lua_entity.h"
#include "game/world_sources/global_random.h"
#include "game/world_sources/random_sources.h"
#include <intrin.h>
// tx-04-08 world 观察入口的专用作用域与 20 项观察编号（C++17 兼容头）。
#include "world/observe.h"
// 自有定义仅借用类型表达 ABI，不构造这些对象。
struct CsStatic {};
struct CsMtSeed {};
struct CsEntityList {};
struct CsRoom {};
struct CsLifecycle {};
struct CsDevice {};
struct CsButtonMaps {};
// tx-04-07 只读观察组的 ABI 替身：只用于成员指针类型，不构造对象。
struct CsEntity {};
struct CsSoundEffects {};
struct CsGame {};
struct CsManager {};
struct CsGameState {};
struct CsPersistentGameData {};
"""


def legacy_flags(row):
    return ((0 if row['static'] else 1) | (2 if row['conv'] == 'cdecl' else 0) |
            (4 if row['ret'] == 'void' else 0))


def legacy_body(row):
    """既有入口的 wrapper 体（保持 tx-04-07 已验收的适配语义不变）。"""
    ident, ret, call = row['id'], row['ret'], row['call']
    if ident == 'client_entity_draw_paused':
        return ['    bool value=false;', '    if(cs::game::ClientEntityDrawPaused(value)) return value;',
                '    cs::CallScope scope(record);', '    return super();']
    if ident == 'client_entity_owned_final':
        return ['    if(cs::game::ClientEntityDrawOwnedFinal(this,sprite,offset)) return;',
                '    cs::CallScope scope(record);', '    super(sprite,offset);']
    if ident.startswith('client_entity_draw_') or ident.startswith('client_entity_shadow_') or ident.startswith('client_entity_post_'):
        kind=2 if ident.startswith('client_entity_post_') else 1 if ident.startswith('client_entity_shadow_') else 0
        return ['    cs::game::ClientEntityDrawScope draw(this,%s,%d);' % ('nullptr' if kind==2 else 'offset',kind),
                '    if(!draw.CallOriginal()) return%s;' % (' false' if ret=='bool' else ''),
                '    cs::CallScope scope(record);', '    %ssuper(%s);' % ('return ' if ret!='void' else '',call)]
    grid_light_queries={'client_grid_light_seed':0,'client_grid_light_stage':1,'client_grid_light_alt':2,
                        'client_grid_light_boss':3,'client_grid_light_suffix':4}
    if ident in grid_light_queries:
        query=grid_light_queries[ident]
        return ['    // 仅当前Client Grid灯光用途读取Authority条件；Host/Off与其它调用原样透传。',
                '    std::int64_t result=0;',
                '    if (cs::game::ClientGridLightQuery(%d, %s, %s, result)) return static_cast<%s>(result);'
                % (query,'nullptr' if row['static'] else 'this','seedEffect' if query==0 else '0',ret),
                '    cs::CallScope scope(record);',
                '    return super(%s);' % call]
    if ident.startswith('client_grid_water_'):
        return ['    cs::CallScope scope(record);',
                '    auto* result=super(out);',
                '    cs::game::ClientGridWaterClip(this,result);',
                '    return result;']
    if ident in ('client_player_render_body_diagnostic', 'client_player_render_head_diagnostic'):
        body = 'true' if ident == 'client_player_render_body_diagnostic' else 'false'
        return ['    // 只读诊断由当前Client实例限制；原链每次仍恰好执行一次。',
                '    cs::CallScope scope(record);',
                '    cs::game::ClientObjectPlayerDrawDiagnostic(this, position, %s, false);' % body,
                '    super(position);',
                '    cs::game::ClientObjectPlayerDrawDiagnostic(this, position, %s, true);' % body]
    if ident == 'client_minimap_render_diagnostic':
        return ['    cs::CallScope scope(record);',
                '    cs::game::ClientPresentationMinimapDiagnostic(this, false);',
                '    super();',
                '    cs::game::ClientPresentationMinimapDiagnostic(this, true);']
    if ident == 'client_backdrop_effects':
        return ['    // Source只读记录真实独立RNG前后态，Client按完整来源重放并省去本地实体生成。',
                '    cs::game::ClientBackdropEffectScope trace(this, %s);' % call,
                '    if (trace.Handled()) return;',
                '    cs::CallScope scope(record);',
                '    super(%s);' % call,
                '    trace.Returned();']
    if ident == 'client_backdrop_wall':
        return ['    cs::game::ClientBackdropWallScope trace(this);',
                '    if (!trace.Ready()) return;',
                '    cs::CallScope scope(record);',
                '    super();',
                '    trace.Returned();']
    if ident == 'client_backdrop_graphics':
        return ['    cs::game::ClientBackdropBuildScope trace(this);',
                '    if (!trace.Ready()) return 0;',
                '    cs::CallScope scope(record);',
                '    auto result=super();',
                '    trace.Returned();',
                '    return result;']
    if ident == 'client_hud_achievement_query':
        return ['    // 仅当前Client HUD绘制读取Authority的最终徽章条件；进度能力不受此查询影响。',
                '    bool result=false;',
                '    // J273 HUD传未使用的栈word（push ecx），不能以其bool值决定是否应用显示来源。',
                '    if (cs::game::ClientPresentationHudAchievementQuery(result)) return result;',
                '    cs::CallScope scope(record);',
                '    return super(unk);']
    if ident == 'client_menu_render':
        return ['    if (cs::game::ClientMenuRenderBoundary()) return;',
                '    // 替代分派先决策；仅原菜单真正进入时才记录原调用。',
                '    record.calls.fetch_add(1, std::memory_order_relaxed);',
                '    super();',
                '    record.completed.fetch_add(1, std::memory_order_relaxed);']
    # Client 外层只负责决策；CallOriginal 才进入既有观察 Hook 与官方原链。
    outer_dispatch={
        'client_game_update':'GameUpdate','client_post_update':'PostUpdate',
        'client_room_update':'RoomUpdate','client_entity_list':'EntityListUpdate',
        'client_player_update':'PlayerUpdate','client_odd_helper':'OddHelper',
        'client_room_lifecycle':'LifecycleFSM','client_door_update':'DoorUpdate',
        'client_door_nested':'DoorNested','client_new_room':'NewRoom',
        'room_init':'RoomInit','game_render':'GameRender',
        'room_prerender':'RoomPreRender','room_render':'RoomRender',
        'entitylist_render_sort':'EntityListRenderSort','client_game_start':'GameStart'
    }
    if ident in outer_dispatch:
        entry=outer_dispatch[ident]
        out=['    if (cs::game::ClientDispatchBefore(cs::game::ClientEntry::%s) != cs::game::ClientEntryDecision::CallOriginal) return%s;'
             % (entry,' 0' if ret!='void' else ''),
             '    // 主动跳过单独由Client计数，观察调用只对应真正进入的原链。',
             '    record.calls.fetch_add(1, std::memory_order_relaxed);']
        out.append('    %ssuper(%s);' % ('auto result = ' if ret!='void' else '',call))
        out += ['    cs::game::ClientDispatchReturned(cs::game::ClientEntry::%s);' % entry,
                '    record.completed.fetch_add(1, std::memory_order_relaxed);']
        if ret!='void':out.append('    return result;')
        return out
    if ident in ('native_level_random','native_generator_random'):
        out=['    cs::game::NativeRandomCall state(%s, cs::game::RandomAlgorithm::XorShift32, "%s", 0, %s);'
             % ('random' if ident=='native_level_random' else 'this','Level.generate_dungeon.argument' if ident=='native_level_random' else 'LevelGenerator.Generate', 'false' if ident=='native_level_random' else 'true')]
        if ident=='native_level_random':
            out+=['    cs::game::NativeRandomCall generation(g_Game ? &g_Game->_generationRNG : nullptr, cs::game::RandomAlgorithm::XorShift32, "Level.generate_dungeon.GameGenerationRNG");']
        out+=['    record.calls.fetch_add(1, std::memory_order_relaxed);','    super(%s);' % call,'    state.Returned(0);']
        if ident=='native_level_random':out+=['    generation.Returned(0);']
        return out+['    record.completed.fetch_add(1, std::memory_order_relaxed);']
    if ident.startswith('native_rng_') or ident.startswith('native_mt_'):
        algorithm='MTRNG' if ident.startswith('native_mt_') else 'XorShift32'
        initialize=ident in ('native_rng_ctor','native_rng_seed','native_mt_seed')
        argument='seed' if initialize else 'max' if ident=='native_rng_int' else '0'
        member={'native_rng_ctor':'RNG.constructor','native_rng_seed':'RNG.SetSeed','native_rng_next':'RNG.Next',
                'native_rng_int':'RNG.RandomInt','native_rng_float':'RNG.RandomFloat',
                'native_mt_seed':'MTRNG.Init','native_mt_next':'MTRNG.Next'}[ident]
        out=['    cs::game::NativeRandomCall observation(this, cs::game::RandomAlgorithm::%s, "%s", %s, %s, %s);'
             % (algorithm,member,argument,str(initialize).lower(),'shift' if ident in ('native_rng_ctor','native_rng_seed') else '0'),
             '    record.calls.fetch_add(1, std::memory_order_relaxed);']
        if ret=='void':
            out+=['    super(%s);' % call,'    observation.Returned(0);']
        elif ret=='float':
            out+=['    auto result=super();','    std::uint32_t bits=0; std::memcpy(&bits,&result,4);','    observation.Returned(bits,true);']
        else:
            out+=['    auto result=super(%s);' % call,'    observation.Returned(result);']
        return out+['    record.completed.fetch_add(1, std::memory_order_relaxed);']+([] if ret=='void' else ['    return result;'])
    if ident in ('native_global_random','native_global_seed'):
        return (['    unsigned initialization_value=0;',
                 '    // Client类别构造只安装Source结果；此窄分支主动不调用原随机源，不计入观察完成。',
                 '    if(cs::game::ClientEntityInitializationRandom(initialization_value)) return initialization_value;'] if ident=='native_global_random' else [])+['    cs::game::GlobalRandomCall observation(%s, _AddressOfReturnAddress());' % ('true, std::uint32_t(reinterpret_cast<uintptr_t>(this))' if ident=='native_global_seed' else 'false, 0'),
                '    record.calls.fetch_add(1, std::memory_order_relaxed);',
                '    auto result=super();',
                '    observation.Returned(result);',
                '    record.completed.fetch_add(1, std::memory_order_relaxed);',
                '    return result;']
    if ident == 'client_entity_local_sound':
        return ['    // 访客准备至安全退休期间不执行本地SFX/声音RNG；Authority瞬时音画由27具名消费。',
                '    if(cs::game::ClientBusy()) return;',
                '    record.calls.fetch_add(1, std::memory_order_relaxed);',
                '    super(id, volume, delay, loop, pitch, pan);',
                '    record.completed.fetch_add(1, std::memory_order_relaxed);']
    if ident == 'client_entity_retire_sound':
        return ['    // 仅具名Entity退休内的原生Stop不重演全局音效状态；其余用途原样调用。',
                '    if(cs::game::ClientEntitySuppressRetireSound()) return;',
                '    record.calls.fetch_add(1, std::memory_order_relaxed);',
                '    super(sound);',
                '    record.completed.fetch_add(1, std::memory_order_relaxed);']
    if ident == 'native_text_render':
        return ['    cs::game::ClientEntityDrawScope draw(this,offset,0);',
                '    if(!draw.CallOriginal()) return;',
                '    cs::game::ActualValueScope observation;',
                '    cs::game::EntityTextRenderWitness witness;',
                '    if(observation.main && cs::game::ObservationLuaNeeded("Render") && g_Manager && g_Game && offset)',
                '      witness=cs::game::BeginEntityTextRender(this,g_Manager->_framecount,g_Game->IsPaused(),offset->x,offset->y);',
                '    record.calls.fetch_add(1, std::memory_order_relaxed);',
                '    if(!observation.main) record.foreign.fetch_add(1, std::memory_order_relaxed);',
                '    super(offset);',
                '    cs::game::FinishEntityTextRender(witness);',
                '    record.completed.fetch_add(1, std::memory_order_relaxed);']
    if ident == 'native_text_update':
        return ['    cs::game::ActualValueScope observation;',
                '    const auto witness=cs::game::BeginEntityTextUpdate(this, _AddressOfReturnAddress());',
                '    record.calls.fetch_add(1, std::memory_order_relaxed);',
                '    if(!observation.main) record.foreign.fetch_add(1, std::memory_order_relaxed);',
                '    const auto result=super();',
                '    cs::game::FinishEntityTextUpdate(witness);',
                '    record.completed.fetch_add(1, std::memory_order_relaxed);',
                '    return result;']
    if ident.startswith('actual_') and ident.endswith('_loot'):
        member={'actual_pickup_loot':'EntityPickup.GetLootList',
                'actual_fireplace_loot':'EntityNPC.GetFireplaceLoot',
                'actual_shopkeeper_loot':'EntityNPC.GetShopkeeperLoot'}[ident]
        return ['    cs::game::ActualValueScope observation;',
                '    record.calls.fetch_add(1, std::memory_order_relaxed);',
                '    if(!observation.main) record.foreign.fetch_add(1, std::memory_order_relaxed);',
                '    auto result=super(%s);' % call,
                '    cs::game::ObserveActualLoot(this, "%s", advance, result);' % member,
                '    record.completed.fetch_add(1, std::memory_order_relaxed);',
                '    return result;']
    lines = []
    if ident == 'client_player_pre_stats':
        return ['    cs::CallScope scope(record);', '    if (cs::game::ClientObjectSuppressPlayerPreStats(this)) return;', '    super();']
    # Client 主动持久分派位于观察作用域之外；被隔离的调用绝不进入官方外层 Hook。
    if ident in ('start_game', 'restore', 'game_exit', 'post_start', 'lua_post_render',
                 'manager_set_save_slot', 'manager_save_game_state', 'manager_record_completion', 'game_save_state',
                 'pgd_add_challenge', 'pgd_add_collection', 'pgd_add_boss', 'pgd_unlock_steam',
                 'gamestate_clear',
                 'pgd_counter', 'pgd_local', 'pgd_cloud',
                 'gamestate_delete', 'gamestate_delete_rerun', 'load_game_state'):
        lines.append('    if (cs::game::ClientPersistenceBlocked()) { cs::game::ObserveClientPersistenceDenied("%s"); return; }' % ident)
    elif ident in ('pgd_unlock', 'gamestate_write', 'gamestate_read',
                   'pgd_add_bestiary_kill', 'pgd_encounter', 'gamestate_write_rerun', 'gamestate_read_rerun'):
        lines.append('    if (cs::game::ClientPersistenceBlocked()) { cs::game::ObserveClientPersistenceDenied("%s"); return false; }' % ident)
    # 拒绝计数与original计数分别记录；拒绝路径不能先创建CallScope伪装原链已执行。
    lines.append('    cs::CallScope scope(record);')
    if ident == 'lua_register_classes':
        lines += ['    if (scope.main) cs::game::ClientLuaGuardClosed();',
                  '    super();',
                  '    if (scope.main) cs::game::ClientGuardLuaBindings(true);']
        return lines
    if ident == 'lua_run_bundled_script':
        lines += ['    if (scope.main) cs::game::ClientGuardLuaBindings();',
                  '    cs::game::ClientCallbackScriptScope callback_script(scope.main, path);',
                  '    if (!callback_script.AllowOriginal()) return false;',
                  '    const auto result=super(path);',
                  '    callback_script.Returned(result);',
                  '    return result;']
        return lines
    if ident == 'isaac_shutdown':
        lines += ['    if (scope.main) cs::game::ClientShutdownBeginning();',
                  '    super();',
                  '    // main 的平台图形清理晚于 Shutdown；在该截止点前撤销本方 context 资格。',
                  '    if (scope.main) cs::game::ClientContextClosing();']
        return lines
    if ident == 'lua_destroy':
        lines += ['    if (scope.main) cs::game::ClientLuaClosing();',
                  '    if (scope.main) cs::game::ClientLuaGuardClosed();',
                  '    super();']
        return lines
    if ident == 'manager_render':
        lines.append('    cs::game::ClientRenderScope client_render(scope.main);')
    if ident == 'manager_update':
        lines += ['    cs::FrameScope frame(scope.main);',
                  '    if (cs::game::ClientManagerBoundary()) return;',
                  '    super(%s);' % call]
    elif ident == 'console_command':
        lines += [
            '    if (scope.main && cs::OwnCommand(in)) {',
            '        try {',
            '            auto* game = static_cast<unsigned char*>(cs::GamePointer());',
            '            const bool lua = game && *reinterpret_cast<void**>(game+cs::offset::CommandCapture);',
            '            auto response = cs::Command(in, lua ? "lua" : "console");',
            '            // 与 REPENTOGON 一致：返回缓冲和控制台输出二选一。',
            '            if (out) out->append(response);',
            '            // Print 捕获分支直接追加字符串并返回，不写控制台历史。',
            '            else Print(response, 0xFFFFFFFFu, 150);',
            '        } catch (...) { cs::Trace("command_error", "exception"); }',
            '        return;',
            '    }',
            '    // Client 原生命令可直接改 achievements/challenges；仅保留本方 cs 命令。',
            '    if (cs::game::ClientPersistenceBlocked()) { cs::game::ObserveClientPersistenceDenied("console_command"); return; }',
            '    super(%s);' % call,
        ]
    else:
        # 此处只桥接主线程适配；原生异常由各层 RAII 观察后继续传播。
        world_stage = {'game_update': 'Owner', 'room_update': 'Room',
                       'post_update': 'Tail'}.get(ident)
        if world_stage:
            obj = 'nullptr' if row['static'] else 'this'
            lines.append('    cs::game::WorldBoundaryScope world_stage(cs::game::WorldBoundary::%s, %s);'
                         % (world_stage, obj))
        if ident in ('restore', 'room_lifecycle', 'door_update', 'door_nested',
                     'new_room', 'game_exit'):
            boundary = 'RunEnd' if ident == 'game_exit' else 'Lifecycle'
            lines.append('    cs::game::WorldBoundaryScope world_lifecycle(cs::game::WorldBoundary::%s);' % boundary)
        if ident.startswith('pgd_') or ident == 'button_maps':
            persistence_args = (', achievementID' if ident == 'pgd_unlock'
                                else ', eEventCounters, num' if ident == 'pgd_counter'
                                else '')
            lines.append('    cs::game::PersistenceScope persistence("%s", this%s);'
                         % (ident, persistence_args))
        if ident == 'room_update':
            lines.append('    cs::game::SimulationScope simulation(cs::game::SimulationKind::Room, this);')
        elif ident == 'odd_helper':
            lines.append('    cs::game::SimulationScope simulation(cs::game::SimulationKind::Odd);')
        elif ident == 'player_update':
            lines.append('    cs::game::PlayerScope player_scope(this);')
        elif ident == 'player_native':
            lines.append('    cs::game::PlayerNativeReached(this);')
        elif ident == 'join_pass':
            lines.append('    cs::game::JoinScope join_scope(this);')
        elif ident == 'input_update':
            lines += ['    cs::game::InputLease input_lease(cs::game::HostBusy() || cs::game::ClientBusy());',
                      '    if (!input_lease.Ready()) {',
                      '        cs::game::InputLeaseUnavailable();',
                      '        if (cs::game::ClientBusy()) { cs::game::ClientInputUnavailable(); return 0; }',
                      '    }']
        elif ident == 'game_exit':
            # 真实退出即使发生在 Host 已停止后，也需要独立的原生调用与窗口关联。
            lines += ['    cs::game::PersistenceCallScope call("game_exit", this);',
                      '    if (scope.main) cs::game::RunTeardownBeginning(true);']
        elif ident == 'device_update':
            lines.append('    cs::game::DeviceScope device_scope(this);')
        elif ident == 'restore':
            lines.append('    cs::game::RestoreScope restore_scope(this, state, start);')
        elif ident == 'start_game':
            # ST4：原函数每帧调用，starting=0 时只有早退；不为该空调用开启
            # 持久窗口或更新 defer_through。原 super 仍每次 exactly-once。
            lines += ['    const bool requested = cs::game::StartGameRequested(this);',
                      '    cs::game::WorldBoundaryScope world_start(cs::game::WorldBoundary::RunStart, this, requested);',
                      '    cs::game::PersistenceCallScope call("start_game", this, requested);',
                      '    cs::game::LoadingOwnerScope loading_owner(true, requested);']
        elif ident == 'room_lifecycle':
            lines.append('    cs::game::LoadingOwnerScope loading_owner(false);')
        elif ident == 'entity_kill':
            lines += ['    cs::game::PersistenceCallScope call("entity_kill", this);',
                      '    cs::game::ObserveEntityKill(this, source);']
        elif ident == 'revive_ghost':
            lines += ['    cs::game::PersistenceCallScope call("revive_ghost", this);',
                      '    cs::game::ObserveReviveGhost(this);']
        elif ident == 'stage_transition':
            lines += ['    cs::game::PersistenceCallScope call("stage_transition", this);',
                      '    cs::game::ObserveStageTransition(this, sameStage, param2, player);']
        elif ident == 'load_game_state':
            lines += ['    cs::game::PersistenceCallScope call("load_game_state", this);',
                      '    cs::game::ObserveLoadGameState(this, saveslot);']
        elif ident in ('gamestate_write', 'gamestate_read', 'gamestate_delete',
                       'gamestate_delete_rerun'):
            lines.append('    cs::game::PersistenceCallScope call("%s", this);' % ident)
        consumers = {'move': 'Movement', 'bomb': 'Bomb', 'active_item': 'Item',
                     'card_pill': 'Pill', 'shoot': 'Shooting', 'drop': 'Drop'}
        if ident in consumers:
            lines.append('    cs::game::ConsumerScope consumer(this, cs::game::ConsumerKind::%s);'
                         % consumers[ident])
            if ident == 'shoot':
                lines.append('    if (!consumer.CallOriginal()) return;')
        lines.append('    %ssuper(%s);' % ('auto result = ' if ret != 'void' else '', call))
        if ident == 'pgd_unlock':
            lines.append('    persistence.Returned(result);')
        if ident in ('entity_kill', 'revive_ghost', 'stage_transition',
                     'load_game_state', 'gamestate_delete',
                     'gamestate_delete_rerun', 'start_game'):
            lines.append('    call.Returned();')
        if ident in ('gamestate_write', 'gamestate_read'):
            lines.append('    call.Returned(result);')
        if ident == 'device_update':
            lines.append('    device_scope.Returned(result);')
        if ident == 'input_update':
            lines.append('    if (scope.main) cs::game::InputFrameFinished();')
        if ident == 'manager_render':
            lines.append('    if (scope.main) cs::game::ClientMenuRendered();')
        if ident == 'console_check':
            lines.append('    if (scope.main) cs::game::ObserveConsole(result);')
        if ident == 'new_room':
            lines.append('    if (scope.main) cs::game::NewRoomReturned();')
        if ident == 'game_exit':
            lines += ['    call.Returned();',
                      '    if (scope.main) cs::RunExit();']
        if ret != 'void':
            lines.append('    return result;')
    return lines


def world_arg_table(row):
    """自有定义的 cs::HookArg 表；返回 (声明行列表, 表名或 nullptr)。"""
    specs = []
    for arg in row['args']:
        if arg['kind'] == 'stack':
            specs.append('    {cs::HookArg::Kind::Stack, 0, %d},' % arg['size'])
        elif arg['kind'] == 'gp':
            specs.append('    {cs::HookArg::Kind::GpRegister, %d, 0},' % arg['reg'])
        else:
            specs.append('    {cs::HookArg::Kind::XmmRegister, %d, 0},' % arg['reg'])
    if not row['own'] or not specs:
        return [], 'nullptr'
    name = 'k' + ''.join(part.capitalize() for part in row['id'].split('_')) + 'Args'
    lines = ['// %s：自有定义的显式参数 ABI（%s，%d 项）。'
             % (row['id'], row['conv'], len(specs)),
             'static constexpr cs::HookArg %s[] = {' % name]
    lines += specs
    lines.append('};')
    return lines, name


def world_body(row):
    """world 观察入口的 wrapper 体：专用作用域 + exactly-once 原调用。"""
    obj = 'nullptr' if row['static'] else 'this'
    words = []
    for arg in row['args']:
        if arg['type'].strip().endswith('&'):
            words.append('cs::world::PackWord(&%s)' % arg['name'])
        else:
            words.append('cs::world::PackWord(%s)' % arg['name'])
    call = ', '.join(arg['name'] for arg in row['args'])
    lines = [
        '    // %s：%s 的 world 观察。原调用 exactly-once，异常只撤销本次观察资格。'
        % (row['id'], row['observation']),
        '    cs::world::ObservationScope observation(',
        '        cs::world::Observation::%s, cs::IsMain(), %s,' % (row['observation'], obj),
        '        {&record.calls, &record.foreign, &record.completed},',
        '        {%s}, %s, %s);' % (', '.join(words), str(bool(row.get('native_witness'))).lower(), str(row['shared_control']).lower()),
    ]
    if row['ret'] == 'void':
        lines.append('    super(%s);' % call if call else '    super();')
        lines.append('    observation.Returned();')
    else:
        lines.append('    auto result = super(%s);' % call)
        lines.append('    observation.Returned(cs::world::PackWord(result));')
        lines.append('    return result;')
    return lines


def emit_hooks(legacy, world, patterns):
    lines = [GENERATED_HEADER]
    for row in legacy:
        macro = 'CS_HOOK_STATIC' if row['static'] else 'CS_HOOK_METHOD'
        conv = '__' + row['conv'] if row['conv'] else ''
        pattern = patterns.get(row['rva'], '') if row['own'] else ''
        lines.append('%s(%s, %s, "%s", %s, %s, %d, %s, "%s", %s, %d, %d, %s, %s, (%s), %s) {'
                     % (macro, row['id'], row['group'].title(), row['name'],
                        row['cls'], row['method'], row['priority'], hex(row['rva']),
                        pattern, str(row['own']).lower(), row['nargs'],
                        legacy_flags(row), row['owner'], conv, row['params'],
                        row['ret']))
        lines += legacy_body(row)
        lines.append('}')
    if world:
        lines += ['',
                  '// ===================================================================',
                  '// tx-04-08 world 观察入口：清单见 binding/hooks.json 的',
                  '// world_observations；完成含义与原生 body 到达证据见各行。',
                  '// ===================================================================']
    # 同一清单派生内层 witness，复用相同 ZHL 定义和 ABI，不建立第二套补丁 owner。
    native_rows = [dict(row, id=row['id'] + '_native', priority=row['native_priority'],
                        native_witness=True) for row in world]
    for row in world + native_rows:
        table_lines, table_name = world_arg_table(row)
        if table_lines:
            lines += table_lines
        macro = 'CS_WORLD_HOOK_STATIC' if row['static'] else 'CS_WORLD_HOOK_METHOD'
        conv = '__' + row['conv'] if row['static'] else ''
        params = ', '.join('%s %s' % (arg['type'], arg['name'])
                           for arg in row['args'])
        lines.append('%s(%s, "%s", %s, %s, %d, %s, "%s", %s, %d, %s, %d, %s, %s, %s, (%s), %s) {'
                     % (macro, row['id'], row['definition'], row['cls'],
                        row['method'], row['priority'], hex(row['rva']),
                        row['pattern'], str(row['own']).lower(), row['nargs'],
                        table_name, row['flags'], row['observation'], str(row['shared_control']).lower(), conv,
                        params, row['ret']))
        lines += world_body(row)
        lines.append('}')
    lines.append('')
    return '\n'.join(lines)


# ===========================================================================
# 布局清单
# ===========================================================================

def parse_layout(doc, problems):
    where = 'world_layout.json'
    if doc.get('schema') != 1:
        problems.add(where, 'schema 必须是 1')
    if doc.get('domain') != 'isaac.cs.world-record':
        problems.add(where, 'domain 必须是 isaac.cs.world-record')
    if doc.get('profile') != 'min-read-v1':
        problems.add(where, 'profile 必须是 min-read-v1')

    constants = doc.get('constants')
    if not isinstance(constants, dict):
        problems.add(where, '缺少 constants 对象')
        constants = {}
    expected_keys = [name for name, _ in CONSTANT_ORDER]
    for key in expected_keys:
        if key not in constants:
            problems.add(where + '.constants', '缺少 %s' % key)
    for key in constants:
        if key not in expected_keys:
            problems.add(where + '.constants', '未登记的常量 %s' % key)
    for key, kind in CONSTANT_ORDER:
        value = constants.get(key)
        if kind == 'f32':
            if not isinstance(value, str) or not re.fullmatch(r'-?\d+(\.\d+)?', value):
                problems.add(where + '.constants.' + key,
                             '浮点常量必须写成十进制字符串')
        elif parse_int(value, where + '.constants.' + key, problems) is None:
            pass

    classes = doc.get('classes')
    if not isinstance(classes, list):
        problems.add(where, '缺少 classes 数组')
        classes = []
    parsed_classes, class_sizes = [], {}
    for index, entry in enumerate(classes):
        cwhere = '%s.classes[%d]' % (where, index)
        if not isinstance(entry, dict):
            problems.add(cwhere, '必须是对象')
            continue
        rtti = entry.get('class')
        if rtti not in RTTI_ORDER:
            problems.add(cwhere, 'class 必须是已登记的 RTTI 类: %r' % (rtti,))
            continue
        if any(row['rtti'] == rtti for row in parsed_classes):
            problems.add(cwhere, 'class 重复: %s' % rtti)
            continue
        schema = entry.get('schema')
        if not isinstance(schema, bool):
            problems.add(cwhere, 'schema 必须是布尔值')
            continue
        if schema != (rtti in ('Player', 'NPC', 'Tear', 'Projectile',
                               'Pickup', 'Familiar', 'Effect', 'Grid')):
            problems.add(cwhere, 'schema 标记与七类专用 schema + Grid 不一致')
        # 新 Grid 派生类只承诺已核定的公共前缀；分配请求不等于完整对象尺寸。
        # 非 schema 类的 null 表示未知，生成的 object_size=0 也只作该诊断含义。
        size = (0 if entry.get('object_size') is None and not schema
                else parse_int(entry.get('object_size'), cwhere + '.object_size', problems))
        if size is None:
            continue
        prefix = (parse_int(entry['readable_prefix_size'], cwhere + '.readable_prefix_size', problems)
                  if 'readable_prefix_size' in entry else size)
        allocation = (parse_int(entry['factory_allocation_size'], cwhere + '.factory_allocation_size', problems)
                      if entry.get('factory_allocation_size') is not None else 0)
        if prefix is None or allocation is None:
            continue
        if prefix <= 0 or (size and prefix > size) or (allocation and prefix > allocation):
            problems.add(cwhere, '已核定读取前缀必须为正且不大于已知对象/分配范围')
        class_sizes[rtti] = prefix
        td = parse_int(entry.get('td_rva'), cwhere + '.td_rva', problems)
        if td is None:
            continue
        bases = entry.get('bases')
        if not isinstance(bases, list) or not bases:
            problems.add(cwhere, 'bases 必须是非空数组')
            continue
        parsed_bases, has_self = [], False
        for bi, base in enumerate(bases):
            bwhere = '%s.bases[%d]' % (cwhere, bi)
            if not isinstance(base, dict):
                problems.add(bwhere, '必须是对象')
                continue
            base_td = parse_int(base.get('td_rva'), bwhere + '.td_rva', problems)
            attrs = parse_int(base.get('attributes'), bwhere + '.attributes', problems)
            pmd = (parse_int(base.get('mdisp'), bwhere + '.mdisp', problems),
                   parse_int(base.get('pdisp'), bwhere + '.pdisp', problems),
                   parse_int(base.get('vdisp'), bwhere + '.vdisp', problems))
            if base_td is None or attrs is None or None in pmd:
                continue
            if base_td == td:
                has_self = True
            expected_attrs = parse_int(constants.get('rtti_expected_base_attributes'),
                                       'constants.rtti_expected_base_attributes',
                                       problems)
            if expected_attrs is not None and attrs != expected_attrs:
                problems.add(bwhere, 'BCD attributes 必须等于常量期望值')
            if (pmd[0], pmd[1], pmd[2]) != (0, -1, 0):
                problems.add(bwhere, '本轮只支持 offset-0 非虚直接基类（PMD 0/-1/0）')
            if not isinstance(base.get('td_name'), str) or not base['td_name']:
                problems.add(bwhere, 'td_name 必须是非空字符串')
            parsed_bases.append(dict(td_rva=base_td, td_name=base.get('td_name', ''),
                                     mdisp=pmd[0], pdisp=pmd[1], vdisp=pmd[2],
                                     attributes=attrs,
                                     supported=bool(base.get('supported'))))
        if not has_self:
            problems.add(cwhere, 'CHD 基类表必须包含自身 TypeDescriptor')
        for key in ('vtable_rva', 'col_rva', 'free_slot_rva'):
            if parse_int(entry.get(key), '%s.%s' % (cwhere, key), problems) is None:
                continue
        parsed_classes.append(dict(
            rtti=rtti, schema=schema,
            # 非 schema 类（Entity 根 / Door）没有文件类别；Class::Unknown 只表示
            # “不映射到某个专用 schema”，编号在 world/model.h 的 Class 枚举里。
            schema_class=rtti if schema else 'Unknown',
            vtable_rva=parse_int(entry.get('vtable_rva'), cwhere, problems),
            col_rva=parse_int(entry.get('col_rva'), cwhere, problems),
            td_rva=td, object_size=size, readable_prefix_size=prefix,
            factory_allocation_size=allocation, td_name=entry.get('td_name', ''),
            free_slot_rva=parse_int(entry.get('free_slot_rva'), cwhere, problems),
            init_rva=(parse_int(entry.get('init_rva'), cwhere + '.init_rva', problems)
                      if entry.get('init_rva') is not None else 0),
            init_source=entry.get('init_source', ''),
            evidence=entry.get('evidence', ''), bases=parsed_bases))
    if sorted(row['rtti'] for row in parsed_classes) != sorted(RTTI_ORDER):
        problems.add(where + '.classes', '必须恰好覆盖 %d 个已登记 RTTI 类' % len(RTTI_ORDER))
    if sorted(row['rtti'] for row in parsed_classes if row['schema']) != sorted(
            ['Player', 'NPC', 'Tear', 'Projectile', 'Pickup', 'Familiar', 'Effect',
             'Grid']):
        problems.add(where + '.classes', '七类专用 schema + Grid 必须严格登记')

    game_size = parse_int(constants.get('game_object_size'),
                          'constants.game_object_size', problems)
    room_size = parse_int(constants.get('room_object_size'),
                          'constants.room_object_size', problems)
    manager_size = parse_int(constants.get('player_manager_object_size'),
                             'constants.player_manager_object_size', problems)

    fields = doc.get('fields')
    if not isinstance(fields, list):
        problems.add(where, '缺少 fields 数组')
        fields = []
    if len(fields) != 78:
        problems.add(where + '.fields', 'min-read-v1 必须登记 78 项，当前 %d'
                     % len(fields))
    parsed_fields, seen_ids, previous = [], set(), 0
    for index, entry in enumerate(fields):
        fwhere = '%s.fields[%d]' % (where, index)
        if not isinstance(entry, dict):
            problems.add(fwhere, '必须是对象')
            continue
        fid = parse_int(entry.get('id'), fwhere + '.id', problems)
        if fid is None:
            continue
        if fid in seen_ids:
            problems.add(fwhere, '字段 ID 重复: %#06x' % fid)
        seen_ids.add(fid)
        if fid <= previous:
            problems.add(fwhere, '字段必须按 ID 升序登记')
        previous = fid
        width = parse_int(entry.get('width'), fwhere + '.width', problems)
        if width not in (1, 4, 8, 16):
            problems.add(fwhere, '宽度必须是 1/4/8/16，当前 %r' % (width,))
            continue
        encoding = entry.get('encoding')
        if encoding not in ENCODINGS:
            problems.add(fwhere, 'encoding 非法: %r' % (encoding,))
            continue
        base = entry.get('base')
        if base not in FIELD_BASES:
            problems.add(fwhere, 'base 非法: %r' % (base,))
            continue
        status = entry.get('status')
        if status not in FIELD_STATUS:
            problems.add(fwhere, 'status 非法: %r' % (status,))
            continue
        owner = entry.get('owner')
        if owner not in CLASS_ORDER + ['Entity', 'Game', 'Derived']:
            problems.add(fwhere, 'owner 非法: %r' % (owner,))
            continue
        offset = entry.get('offset')
        if base == 'derived':
            if offset is not None:
                problems.add(fwhere, '派生字段不应有对象偏移')
                continue
            if owner != 'Derived' and owner != 'Grid':
                problems.add(fwhere, '派生字段的 owner 只能是 Derived 或 Grid')
            if status != 'Derived':
                problems.add(fwhere, '派生字段的 status 必须是 Derived')
            offset = 0
        else:
            offset = parse_int(offset, fwhere + '.offset', problems)
            if offset is None:
                continue
            if status == 'Derived':
                problems.add(fwhere, '非派生字段不能使用 Derived 状态')
        limit = class_sizes.get(owner)
        if owner == 'Game':
            limit = game_size
        if limit is None and owner != 'Derived':
            problems.add(fwhere, 'owner %r 没有登记对象尺寸，无法校验偏移' % owner)
        if limit is not None and offset + width > limit:
            problems.add(fwhere, '字段越界: %s+%#x+%d > %#x'
                         % (owner, offset, width, limit))
        if not isinstance(entry.get('name'), str) or not entry['name']:
            problems.add(fwhere, 'name 必须是非空字符串')
            continue
        if not isinstance(entry.get('sdk_member'), str):
            problems.add(fwhere, 'sdk_member 必须是字符串（未命名写空串）')
            continue
        if not isinstance(entry.get('evidence'), str) or not entry['evidence']:
            problems.add(fwhere, 'evidence 必须是非空字符串')
            continue
        parsed_fields.append(dict(
            id=fid, name=entry['name'], width=width, encoding=encoding,
            base=base, owner=owner, offset=offset, status=status,
            sdk_member=entry['sdk_member'], evidence=entry['evidence']))

    sources = doc.get('sources')
    if not isinstance(sources, list):
        problems.add(where, '缺少 sources 数组')
        sources = []
    parsed_sources = []
    for index, entry in enumerate(sources):
        swhere = '%s.sources[%d]' % (where, index)
        if not isinstance(entry, dict):
            problems.add(swhere, '必须是对象')
            continue
        sid = parse_int(entry.get('id'), swhere + '.id', problems)
        if sid is None:
            continue
        if not 0 <= sid < len(SOURCE_ORDER):
            problems.add(swhere, 'source ID 越界: %d' % sid)
            continue
        if entry.get('name') != SOURCE_ORDER[sid]:
            problems.add(swhere, 'source 名称必须与文件模型一致: 期望 %r 实际 %r'
                         % (SOURCE_ORDER[sid], entry.get('name')))
        kind = entry.get('kind')
        if kind not in SOURCE_KINDS:
            problems.add(swhere, 'kind 非法: %r' % (kind,))
            continue
        base = entry.get('base')
        if base not in ('room', 'game', 'player', 'none'):
            problems.add(swhere, 'base 非法: %r' % (base,))
            continue
        offset = (parse_int(entry.get('offset'), swhere + '.offset', problems)
                  if entry.get('offset') is not None else 0)
        count = parse_int(entry.get('count'), swhere + '.count', problems)
        max_index = parse_int(entry.get('max_index'), swhere + '.max_index', problems)
        if offset is None or count is None or max_index is None:
            continue
        if count:
            if max_index != count - 1:
                problems.add(swhere, 'max_index 必须等于 count-1（包含式最大索引）')
            if kind != 'fixed_array':
                problems.add(swhere, '只有固定数组来源才登记固定 count')
        if kind == 'reserved':
            if base != 'none' or offset != 0:
                problems.add(swhere, 'reserved 来源不能带对象位置')
        elif base == 'none':
            problems.add(swhere, '非 reserved 来源必须声明基址')
        if kind == 'fixed_array' and count == 0:
            problems.add(swhere, '固定数组来源必须给出 count')
        map_offset = (parse_int(entry.get('index_map_offset'),
                                swhere + '.index_map_offset', problems)
                      if entry.get('index_map_offset') is not None else 0)
        if map_offset is None:
            continue
        # 位置必须落在已登记对象尺寸内；不按房间形状缩减固定来源。
        if base == 'room' and room_size is not None:
            span = (count * 4) if kind == 'fixed_array' else 16
            if kind != 'reserved' and offset + span > room_size:
                problems.add(swhere, '来源越界: room+%#x+%d > %#x'
                             % (offset, span, room_size))
            if map_offset and map_offset + 8 * 4 > room_size:
                problems.add(swhere, 'door→grid 映射表越界')
        if base == 'game':
            if kind == 'roster' and offset + 12 > (game_size or 0):
                problems.add(swhere, 'roster vector 越界')
            if kind == 'fixed_array':
                position = offset - (parse_int(constants.get(
                    'game_player_manager_offset'), 'constants', problems) or 0)
                if count * 4 + position > (manager_size or 0):
                    problems.add(swhere, 'Esau 数组越界（按 PlayerManager 尺寸）')
        if not isinstance(entry.get('evidence'), str) or not entry['evidence']:
            problems.add(swhere, 'evidence 必须是非空字符串')
        parsed_sources.append(dict(source=sid, kind=kind, base=base, offset=offset,
                                   count=count, max_index=max_index,
                                   index_map_offset=map_offset,
                                   evidence=entry.get('evidence', ''),
                                   name=SOURCE_ORDER[sid]))
    if sorted(row['source'] for row in parsed_sources) != list(range(len(SOURCE_ORDER))):
        problems.add(where + '.sources', '必须恰好覆盖 15 个来源 ID')

    grid_count = parse_int(constants.get('room_grid_slot_count'), 'constants', problems)
    grid_max = parse_int(constants.get('room_grid_max_index'), 'constants', problems)
    grid_source = next((row for row in parsed_sources if row['source'] == 11), None)
    if grid_source and grid_count is not None:
        if grid_source['count'] != grid_count:
            problems.add(where, 'grid 来源容量必须等于 room_grid_slot_count')
        if grid_max is not None and grid_source['max_index'] != grid_max:
            problems.add(where, 'grid 来源 max_index 必须等于 room_grid_max_index')
        if grid_count * 4 + grid_source['offset'] != 0x724:
            problems.add(where, '448 个 Grid 指针之后必须紧接 door 槽（+0x724）')
    door_count = parse_int(constants.get('room_door_slot_count'), 'constants', problems)
    door_source = next((row for row in parsed_sources if row['source'] == 12), None)
    if door_source and door_count is not None and door_source['count'] != door_count:
        problems.add(where, 'door 来源容量必须等于 room_door_slot_count')

    assumptions, unknowns = doc.get('assumptions'), doc.get('unknowns')
    if not isinstance(assumptions, list) or not assumptions:
        problems.add(where, '缺少 assumptions 数组')
        assumptions = []
    if not isinstance(unknowns, list) or not unknowns:
        problems.add(where, '缺少 unknowns 数组')
        unknowns = []
    descriptors = doc.get('descriptors')
    if not isinstance(descriptors, dict) or not descriptors:
        problems.add(where, '缺少 descriptors 对象')
        descriptors = {}

    return dict(constants=constants, classes=parsed_classes, fields=parsed_fields,
                sources=parsed_sources, assumptions=assumptions,
                unknowns=unknowns, class_sizes=class_sizes,
                descriptors=descriptors)


def constant_literal(key, kind, value, problems):
    if kind == 'f32':
        if not isinstance(value, str) or not re.fullmatch(r'-?\d+(\.\d+)?', value):
            problems.add('constants.' + key, '浮点常量格式非法: %r' % (value,))
            return '0.0f'
        return value + 'f'
    number = parse_int(value, 'constants.' + key, problems)
    if number is None:
        return '0u'
    if number < 0:
        problems.add('constants.' + key, '无符号常量不能为负')
        return '0u'
    return '0x%Xu' % number


SOURCE_KIND_NAMES = {0: 'EntityList', 1: 'Roster', 2: 'FixedArray', 3: 'Relation',
                     4: 'Reserved'}
BASE_NAMES = {'entity': 'Entity', 'grid': 'Grid', 'game': 'Game',
              'derived': 'Derived'}


def emit_layout(layout, world_rows, capacity, problems):
    lines = ['// 由 gen_binding.py 生成，请修改 binding/world_layout.json 与',
             '// binding/hooks.json。包含字段/来源/类别/观察入口表与目标版本常量。',
             'namespace {']
    lines.append('constexpr FieldSpec kLayoutFields[] = {')
    for row in layout['fields']:
        lines.append(
            '    {.id = 0x%04X, .base = FieldBase::%s, .offset = 0x%Xu, '
            '.width = %d, .encoding = Encoding::%s, .status = FieldStatus::%s, '
            '.name = %s, .owner = %s, .sdk_member = %s, .evidence = %s},'
            % (row['id'], BASE_NAMES[row['base']], row['offset'], row['width'],
               row['encoding'], row['status'], c_string(row['name']),
               c_string(row['owner']), c_string(row['sdk_member']),
               c_string(row['evidence'])))
    lines.append('};')
    lines.append('constexpr SourceSpec kLayoutSources[] = {')
    for row in layout['sources']:
        lines.append(
            '    {.source = %d, .kind = SourceKind::%s, .base = %s, '
            '.offset = 0x%Xu, .count = %d, .max_index = %d, '
            '.index_map_offset = 0x%Xu, .evidence = %s},'
            % (row['source'], SOURCE_KIND_NAMES[SOURCE_KINDS[row['kind']]],
               c_string(row['base']), row['offset'], row['count'],
               row['max_index'], row['index_map_offset'],
               c_string(row['evidence'])))
    lines.append('};')
    # 每个类别的基类表单独定义，ClassSpec 只保存指针。
    for cls in layout['classes']:
        lines.append('constexpr BaseClassSpec kBases%s[] = {' % cls['rtti'])
        for base in cls['bases']:
            lines.append(
                '    {.td_rva = 0x%Xu, .td_name = %s, .mdisp = %d, .pdisp = %d, '
                '.vdisp = %d, .attributes = 0x%Xu, .supported = %s},'
                % (base['td_rva'], c_string(base['td_name']), base['mdisp'],
                   base['pdisp'], base['vdisp'], base['attributes'],
                   'true' if base['supported'] else 'false'))
        lines.append('};')
    lines.append('constexpr ClassSpec kLayoutClasses[] = {')
    for cls in layout['classes']:
        lines.append(
            '    {.rtti = RttiClass::%s, .schema = %s, .schema_class = Class::%s, '
            '.vtable_rva = 0x%Xu, .col_rva = 0x%Xu, .td_rva = 0x%Xu, '
            '.object_size = 0x%Xu, .td_name = %s, .free_slot_rva = 0x%Xu, '
            '.init_rva = 0x%Xu, .init_source = %s, .bases = kBases%s, '
            '.base_count = %d, .evidence = %s},'
            % (cls['rtti'], 'true' if cls['schema'] else 'false',
               cls['schema_class'], cls['vtable_rva'] or 0, cls['col_rva'] or 0,
               cls['td_rva'] or 0, cls['object_size'], c_string(cls['td_name']),
               cls['free_slot_rva'] or 0, cls['init_rva'] or 0,
               c_string(cls['init_source']), cls['rtti'], len(cls['bases']),
               c_string(cls['evidence'])))
    lines.append('};')
    lines.append('constexpr ObservationSpec kLayoutObservations[] = {')
    for row in sorted(world_rows, key=lambda r:
                      OBSERVATION_ORDER.index(r['observation'])):
        lines.append(
            '    {.observation = Observation::%s, .hook_id = %s, '
            '.definition = %s, .completion = %s, .reach = %s, '
            '.rva = 0x%Xu, .priority = %d, .own_definition = %s, '
            '.required = %s},'
            % (row['observation'], c_string(row['id']),
               c_string(row['definition']), c_string(row['completion']),
               c_string(row['reach']), row['rva'], row['priority'],
               'true' if row['own'] else 'false',
               'true' if row['required'] else 'false'))
    lines.append('};')
    lines.append('constexpr const char *kLayoutAssumptions[] = {')
    for text in layout['assumptions']:
        lines.append('    %s,' % c_string(text))
    lines.append('};')
    lines.append('constexpr const char *kLayoutUnknowns[] = {')
    for text in layout['unknowns']:
        lines.append('    %s,' % c_string(text))
    lines.append('};')
    # manifest 形状：writer 只序列化，不再二次解释。
    lines.append('constexpr LayoutField kManifestFields[] = {')
    for row in layout['fields']:
        lines.append('    {.id = 0x%04X, .offset = 0x%Xu, .evidence = %s},'
                     % (row['id'], row['offset'], c_string(row['evidence'])))
    lines.append('};')
    lines.append('constexpr LayoutSource kManifestSources[] = {')
    for row in layout['sources']:
        if row['kind'] == 'reserved':
            location = 'NOT_APPLICABLE'
        elif row['count']:
            location = '%s+0x%X count=%d max_index=%d' % (
                row['base'], row['offset'], row['count'], row['max_index'])
        else:
            location = '%s+0x%X dynamic' % (row['base'], row['offset'])
        lines.append('    {.source = %d, .location = %s, .capacity = %d, '
                     '.evidence = %s},'
                     % (row['source'], c_string(location), row['count'],
                        c_string(row['evidence'])))
    lines.append('};')
    lines.append('constexpr LayoutClass kManifestClasses[] = {')
    for cls in layout['classes']:
        if not cls['schema']:
            continue
        lines.append('    {.kind = Class::%s, .descriptor_rva = "0x%X", '
                     '.evidence = %s},'
                     % (cls['schema_class'], cls['td_rva'] or 0,
                        c_string(cls['evidence'])))
    lines.append('};')
    lines.append('constexpr Constants kLayoutConstants = {')
    for key, kind in CONSTANT_ORDER:
        lines.append('    .%s = %s,' % (key, constant_literal(
            key, kind, layout['constants'].get(key), problems)))
    lines.append('};')
    lines.append('} // namespace')
    lines.append('')
    lines.append('// 清单引用摘要：写入 manifest.json 的 hook_manifest 字段。')
    lines.append('constexpr const char kHookManifestReference[] =')
    lines.append('    "tx-04-08 rows=%d capacity=%d legacy=%d world=%d '
                 'fields=%d sources=%d classes=%d";'
                 % (capacity['rows'], capacity['pending'], capacity['legacy'],
                    capacity['world'], len(layout['fields']),
                    len(layout['sources']), len(layout['classes'])))
    lines.append('static_assert(kHookManifestReference[0] != 0,'
                 ' "清单引用不能为空");')
    lines.append('')
    return '\n'.join(lines)


def emit_capacity(capacity):
    return '\n'.join([
        '#pragma once',
        '// 由 gen_binding.py 生成：待注册表容量与实际行数。',
        '// 注册期必须显式报告超限（旧实现静默丢弃），生成期超限直接失败。',
        '#include <cstddef>',
        'namespace cs {',
        'inline constexpr std::size_t kHookPendingCapacity = %d;'
        % capacity['pending'],
        'inline constexpr std::size_t kHookManifestRows = %d;' % capacity['rows'],
        'inline constexpr std::size_t kHookLegacyRows = %d;' % capacity['legacy'],
        'inline constexpr std::size_t kWorldObservationRows = %d;' % capacity['world'],
        'inline constexpr std::size_t kHookDistinctRva = %d;' % capacity['distinct_rva'],
        'static_assert(kHookManifestRows <= kHookPendingCapacity,',
        '              "Hook 清单行数超过待注册表容量");',
        'static_assert(kHookManifestRows == kHookLegacyRows + kWorldObservationRows,',
        '              "行数必须等于既有入口与 world 观察之和");',
        '} // namespace cs',
        '',
    ])


# ===========================================================================
# 主流程
# ===========================================================================

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    problems = Problems()
    document = load_json(HOOKS_PATH, problems)
    layout_doc = load_json(LAYOUT_PATH, problems)
    patterns_doc = load_json(PATTERNS_PATH, problems)
    binding = load_json(BINDING_PATH, problems)
    if document is None or layout_doc is None or patterns_doc is None or binding is None:
        problems.finish()

    schema = document.get('schema')
    if not isinstance(schema, dict):
        problems.add('hooks.json', '缺少 schema 对象')
        schema = {}
    if schema.get('version') != 2:
        problems.add('hooks.json', 'schema.version 必须是 2')
    pending_capacity = parse_int(schema.get('pending_capacity'),
                                 'hooks.json.schema.pending_capacity', problems)
    if schema.get('layout_manifest') != 'binding/world_layout.json':
        problems.add('hooks.json', 'schema.layout_manifest 必须指向 '
                     'binding/world_layout.json')

    legacy = parse_legacy_rows(document.get('hooks'), problems)
    world = parse_world_rows(document, problems)
    check_unique(legacy, world, problems)
    native_priority = parse_int(schema.get('world_native_witness_priority'),
                                'schema.world_native_witness_priority', problems)
    for row in world:
        row['native_priority'] = native_priority
        if native_priority is None or native_priority <= row['priority']:
            problems.add(row['id'], 'native witness priority 必须严格位于外层观察之内')

    capacity = {
        'pending': pending_capacity if pending_capacity is not None else 0,
        'rows': len(legacy) + 2 * len(world),
        'legacy': len(legacy),
        'world': 2 * len(world),
        'distinct_rva': len({row['rva'] for row in legacy + world}),
    }
    capacity['headroom'] = capacity['pending'] - capacity['rows']
    if pending_capacity is not None and capacity['rows'] > pending_capacity:
        problems.add('hooks.json',
                     '待注册行数 %d 超过容量 %d：%s'
                     % (capacity['rows'], pending_capacity,
                        '、'.join(row['id'] for row in legacy + world)))
    if capacity['headroom'] < 0:
        problems.add('hooks.json', '容量不足：headroom=%d' % capacity['headroom'])

    layout = parse_layout(layout_doc, problems)
    patterns = {int(entry['rva'], 16): entry['pattern']
                for entry in patterns_doc.get('functions', [])}
    # 自有定义靠入口特征字节自校验；缺特征会让 PatternMatches 恒真，必须报错。
    for row in legacy:
        if row['own'] and not patterns.get(row['rva']):
            problems.add('hooks.%s' % row['id'],
                         '自有定义在该 RVA 没有登记特征字节'
                         '（binding/v1_9_7_12-static.json）')
    problems.finish()

    # --- hooks_generated.cpp ---------------------------------------------
    runtime=document.get('runtime_random_hooks',[])
    if {r.get('source') for r in runtime} != set(range(5)):
        raise ValueError('runtime_random_hooks必须明确列出五个来源，不从代码外猜测Hook')
    (args.out / 'random_runtime.inc').write_text(''.join(
        '  {%s,%s,%s,%du},\n' % (c_string(r['id']),c_string(r.get('module','')),c_string(r.get('symbol','')),r['source'])
        for r in runtime),encoding='utf-8')
    (args.out / 'hooks_generated.cpp').write_text(
        emit_hooks(legacy, world, patterns) + '\n', encoding='utf-8')

    # --- 设备特征与字节补丁 ----------------------------------------------
    device = [(rva, patterns[rva]) for rva in
              (0x5EC570, 0x5EC6A0, 0x5EEB40, 0x5EEBB0)]
    (args.out / 'binding_v1_9_7_12.inc').write_text(
        '\n'.join('{0x%X, "%s"},' % (rva, pattern) for rva, pattern in device)
        + '\n', encoding='utf-8')
    patch = next(row for row in document['byte_patches'] if row['id'] == 'odd_normal')
    expected = [int(value, 16) for value in patch['expected'].split()]
    (args.out / 'permanent_patches.inc').write_text(
        '// 由唯一 Hook 清单生成，勿手改。\n'
        'inline constexpr uintptr_t OddCallRva = %s;\n'
        'inline constexpr uintptr_t OddResumeRva = %s;\n'
        'inline constexpr const char* OddPattern = "%s";\n'
        'inline constexpr unsigned char OddExpected[] = {%s};\n'
        % (patch['rva'], patch['resume'], patch['expected'],
           ','.join(hex(value) for value in expected)), encoding='utf-8')
    host_patches = [row for row in document['byte_patches'] if row['group'] == 'join']
    host_lines = ['// 由唯一 Hook 清单生成，勿手改。',
                  'struct HostPatchSpec { const char* id; uintptr_t rva, resume, '
                  'alternate; const char* expected; unsigned size; bool call; };',
                  'inline constexpr HostPatchSpec HostPatches[] = {']
    for row in host_patches:
        host_lines.append('  {"%s", %s, %s, %s, "%s", %s, %s},'
                          % (row['id'], row['rva'], row['resume'],
                             row['alternate'], row['expected'], row['size'],
                             'true' if row['kind'] == 'call' else 'false'))
    host_lines.append('};')
    host_lines += ['struct HostDependencySpec { const char* id; uintptr_t rva; '
                   'const char* expected; };',
                   'inline constexpr HostDependencySpec HostDependencies[] = {']
    for row in document.get('native_dependencies', []):
        host_lines.append('  {"%s", %s, "%s"},'
                          % (row['id'], row['rva'], row['expected']))
    host_lines.append('};')
    (args.out / 'host_patches.inc').write_text('\n'.join(host_lines) + '\n',
                                               encoding='utf-8')
    # 退出判断与 Host patch 分开编译，仍全部来自同一清单、同一安装 owner。
    exit_patch = next(row for row in document['byte_patches'] if row['id'] == 'world_process_exit')
    (args.out / 'exit_patch.inc').write_text(
        '// 唯一清单生成的 main 关闭判断；完整 cmp+jnz 共十字节。\n'
        'inline constexpr uintptr_t ExitCheckRva = %s, ExitResumeRva = %s, ExitLoopRva = %s;\n'
        'inline constexpr const char* ExitCheckPattern = "%s";\n'
        'inline constexpr unsigned ExitCheckBytes = %s;\n'
        % (exit_patch['rva'], exit_patch['resume'], exit_patch['alternate'],
           exit_patch['expected'], exit_patch['size']), encoding='utf-8')
    # 背景实体生成两点同样来自唯一清单；Source/Off重放被覆盖的真实GlobalRandom调用。
    backdrop_rows = [next(row for row in document['byte_patches'] if row['id'] == ident)
                     for ident in ('client_backdrop_effect_1', 'client_backdrop_effect_2')]
    backdrop_random = next(row['rva'] for row in legacy if row['id'] == 'native_global_random')
    (args.out / 'backdrop_patch.inc').write_text(
        'inline constexpr uintptr_t BackdropEffectRva[2] = {%s};\n'
        'inline constexpr uintptr_t BackdropEffectResume[2] = {%s};\n'
        'inline constexpr uintptr_t BackdropEffectSkip[2] = {%s};\n'
        'inline constexpr const char *BackdropEffectPattern[2] = {%s};\n'
        'inline constexpr uintptr_t BackdropRandomRva = 0x%Xu;\n'
        % (','.join(row['rva'] for row in backdrop_rows),
           ','.join(row['resume'] for row in backdrop_rows),
           ','.join(row['alternate'] for row in backdrop_rows),
           ','.join(c_string(row['expected']) for row in backdrop_rows), backdrop_random), encoding='utf-8')
    # Shutdown 两段原生直接写点与关闭判断同属常驻字节补丁，唯一来源仍是 hooks.json。
    shutdown_rows = [next(row for row in document['byte_patches'] if row['id'] == ident)
                     for ident in ('client_shutdown_counter', 'client_shutdown_save')]
    (args.out / 'shutdown_patch.inc').write_text(
        '// J273 Shutdown progress 分支；Client 跳过后仍继续 Options 与正常析构。\n'
        + ''.join('inline constexpr uintptr_t Shutdown%sRva = %s, Shutdown%sResumeRva = %s, Shutdown%sSkipRva = %s;\n'
                  'inline constexpr const char *Shutdown%sPattern = "%s";\n'
                  'inline constexpr unsigned Shutdown%sBytes = %s;\n'
                  % (name, row['rva'], name, row['resume'], name, row['alternate'],
                     name, row['expected'], name, row['size'])
                  for name, row in zip(('Counter', 'Save'), shutdown_rows)),
        encoding='utf-8')

    # --- 容量与布局 -------------------------------------------------------
    (args.out / 'hook_capacity.h').write_text(emit_capacity(capacity),
                                              encoding='utf-8')
    (args.out / 'world_layout.inc').write_text(
        emit_layout(layout, world, capacity, problems) + '\n', encoding='utf-8')
    problems.finish()

    # --- hook-manifest.json / .md ----------------------------------------
    legacy_manifest = []
    for row in legacy:
        prototypes = [item['id'] for item in binding['items']
                      if int(item.get('v12', {}).get('target') or '0', 16) == row['rva']]
        legacy_manifest.append(dict(
            id=row['id'], group=row['group'], definition=row['name'],
            rva=hex(row['rva']), priority=row['priority'],
            mechanism='ZHL_OWN' if row['own'] else 'ZHL_REPENTOGON',
            required_nonzero=row['required'], owner=row['owner'], armed='session',
            inactive=('handle cs; other commands super exactly once'
                      if row['id'] == 'console_command' else 'super exactly once'),
            prototype=prototypes, evidence='STATIC_CONFIRMED',
            abi=dict(model='stack-words', nargs=row['nargs'],
                     thiscall=not row['static'], conv=row['conv'] or 'thiscall',
                     flags=legacy_flags(row))))
    world_manifest = []
    for row in world:
        world_manifest.append({
            'id': row['id'], 'observation': row['observation'],
            'group': 'observe' if row['shared_control'] else 'world', 'shared_control': row['shared_control'],
            'definition': row['definition'], 'class': row['cls'],
            'method': row['method'], 'rva': hex(row['rva']),
            'priority': row['priority'],
            'mechanism': 'ZHL_OWN' if row['own'] else 'ZHL_REPENTOGON',
            'own_definition': row['own'], 'required': row['required'],
            'args': [dict(type=arg['type'], name=arg['name'], abi=arg['abi'])
                     for arg in row['args']],
            'return': row['ret'], 'return_abi': row['return_abi'],
            'entry_bytes': row['entry_bytes'], 'completion': row['completion'],
            'reach': row['reach'], 'evidence': row['evidence'],
            'flags': row['flags'],
            'native_body_reach': 'OWN_ENTRY' if row['own'] else 'UNKNOWN'})
        world_manifest[-1]['native_witness'] = dict(
            id=row['id'] + '_native', priority=row['native_priority'],
            definition=row['definition'], rva=hex(row['rva']),
            completion='内层进入且 super 正常返回，再与外层同 call 配对；运行证据仍待整体 E2E')
    total = dict(
        tx='tx-04-09', schema=2,
        generated_from=['mod/binding/hooks.json',
                        'mod/binding/world_layout.json'],
        capacity=capacity, hooks=legacy_manifest,
        world_observations=world_manifest,
        world_native_witnesses=[dict(row, id=row['native_witness']['id'],
                                    priority=row['native_witness']['priority'], role='native_witness')
                                for row in world_manifest],
        layout=dict(manifest='binding/world_layout.json',
                    fields=len(layout['fields']), sources=len(layout['sources']),
                    classes=len(layout['classes']),
                    schema_classes=sorted(cls['rtti'] for cls in layout['classes']
                                          if cls['schema'])),
        pointer_hooks=document['pointer_hooks'],
        native_dependencies=document.get('native_dependencies', []),
        runtime_random_hooks=document.get('runtime_random_hooks', []),
        byte_patches=document['byte_patches'])
    (args.out / 'hook-manifest.json').write_text(
        json.dumps(total, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')

    md = ['# tx-04-09 Hook manifest', '',
          'Generated from `mod/binding/hooks.json` and '
          '`mod/binding/world_layout.json`. 安装后常驻；session 只控制逻辑启停。',
          '',
          '待注册容量：%d / %d（既有 %d + world %d，distinct RVA %d）。'
          % (capacity['rows'], capacity['pending'], capacity['legacy'],
             capacity['world'], capacity['distinct_rva']),
          '', '## 既有入口', '',
          '| id | group | definition | RVA | priority | required nonzero |',
          '|---|---|---|---|---|---|']
    md += ['| %s | %s | %s | %s | %s | %s |'
           % (row['id'], row['group'], row['definition'], row['rva'],
              row['priority'], row['required_nonzero'])
           for row in legacy_manifest]
    md += ['', '## world 观察入口', '',
           '| observation | id | definition | RVA | priority | ABI | 完成含义 |',
           '|---|---|---|---|---|---|---|']
    for row in world_manifest:
        abi = ', '.join('%s:%s' % (arg['name'], arg['abi']) for arg in row['args'])
        md.append('| %s | %s | %s | %s | %s | %s | %s |'
                  % (row['observation'], row['id'], row['definition'],
                     row['rva'], row['priority'], abi or '(none)',
                     row['completion']))
    md += ['', '每项 world 观察另由同清单派生 `_native` 内层（priority=%s），'
           '相同定义/ABI、原调用一次；原生到达与正常返回均需同 call 配对。' % native_priority]
    md += ['', '## 布局', '',
           '字段 %d 项、来源 %d 项、类别 %d 项（schema: %s）。'
           % (len(layout['fields']), len(layout['sources']),
              len(layout['classes']),
              ', '.join(sorted(cls['rtti'] for cls in layout['classes']
                               if cls['schema']))),
           '', 'Pointer hooks: Keyboard/Gamepad slots 8, 9, 10, 35. Permanent.',
           'OddNormal: %s, expected %s, resume %s. Permanent.'
           % (patch['rva'], patch['expected'], patch['resume'])]
    for row in host_patches:
        md.append('Host/K: %s @ %s, expected %s, resume %s. Permanent.'
                  % (row['id'], row['rva'], row['expected'], row['resume']))
    md.append('World exit: %s @ %s, expected %s, shutdown %s, loop %s. Permanent.'
              % (exit_patch['id'], exit_patch['rva'], exit_patch['expected'], exit_patch['resume'], exit_patch['alternate']))
    (args.out / 'hook-manifest.md').write_text('\n'.join(md) + '\n',
                                               encoding='utf-8')

    # --- world-layout.json（E2E / 离线工具的规范化导出） -------------------
    export = dict(
        tx='tx-04-09', schema=1, domain=layout_doc.get('domain'),
        profile=layout_doc.get('profile'), target=layout_doc.get('target'),
        evidence_reports=layout_doc.get('evidence_reports'),
        capacity=capacity, constants=layout['constants'],
        descriptors=layout['descriptors'],
        classes=[dict(rtti=cls['rtti'], schema=cls['schema'],
                      schema_class=cls['schema_class'],
                      vtable_rva=hex_text(cls['vtable_rva']),
                      col_rva=hex_text(cls['col_rva']),
                      td_rva=hex_text(cls['td_rva']),
                      object_size=(hex_text(cls['object_size']) if cls['object_size'] else None),
                      readable_prefix_size=hex_text(cls['readable_prefix_size']),
                      factory_allocation_size=(hex_text(cls['factory_allocation_size'])
                                               if cls['factory_allocation_size'] else None),
                      td_name=cls['td_name'],
                      free_slot_rva=hex_text(cls['free_slot_rva']),
                      init_rva=hex_text(cls['init_rva']),
                      init_source=cls['init_source'],
                      bases=[dict(td_rva=hex_text(base['td_rva']),
                                  td_name=base['td_name'], mdisp=base['mdisp'],
                                  pdisp=base['pdisp'], vdisp=base['vdisp'],
                                  attributes=hex_text(base['attributes']),
                                  supported=base['supported'])
                             for base in cls['bases']],
                      evidence=cls['evidence'])
                 for cls in layout['classes']],
        grid_types=layout_doc.get('grid_types', []),
        fields=[dict(id='0x%04X' % row['id'], name=row['name'],
                     width=row['width'], encoding=row['encoding'],
                     base=row['base'], owner=row['owner'],
                     offset=(None if row['base'] == 'derived'
                             else '0x%X' % row['offset']),
                     status=row['status'], sdk_member=row['sdk_member'],
                     evidence=row['evidence'])
                for row in layout['fields']],
        sources=[dict(id=row['source'], name=row['name'], kind=row['kind'],
                      base=row['base'], offset='0x%X' % row['offset'],
                      count=row['count'], max_index=row['max_index'],
                      index_map_offset=('0x%X' % row['index_map_offset']
                                        if row['index_map_offset'] else None),
                      evidence=row['evidence'])
                 for row in layout['sources']],
        observations=[dict(observation=row['observation'], hook_id=row['id'],
                           definition=row['definition'],
                           rva='0x%X' % row['rva'],
                           priority=row['priority'],
                           own_definition=row['own'], required=row['required'],
                           completion=row['completion'], reach=row['reach'])
                      for row in sorted(world, key=lambda r:
                                        OBSERVATION_ORDER.index(r['observation']))],
        assumptions=layout['assumptions'], unknowns=layout['unknowns'])
    (args.out / 'world-layout.json').write_text(
        json.dumps(export, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')

    print('gen_binding: rows=%d/%d (legacy=%d world=%d) fields=%d sources=%d '
          'classes=%d' % (capacity['rows'], capacity['pending'],
                          capacity['legacy'], capacity['world'],
                          len(layout['fields']), len(layout['sources']),
                          len(layout['classes'])))


if __name__ == '__main__':
    main()
