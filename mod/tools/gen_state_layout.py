"""从唯一 JSON 布局源生成二进制复制、解码、回映射和单行布局表。

这是构建生成器：拒绝不成立的声明，不读取游戏，不执行测试，不计算哈希。
已明确的未知区可生成，但 complete 始终为 false；后续步骤不能把草稿当完整布局。
"""
import argparse
import copy
import json
import posixpath
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CATEGORIES = dict(zip(
    ['状态', '换算', '引用', '资源', '容器', '本地', '派生', '权威', '填充', '未知'],
    ['State', 'Convert', 'Reference', 'Resource', 'Container', 'Local', 'Derived', 'Authority', 'Padding', 'Unknown']))
OPERATIONS = {'copy': 'Copy', 'convert': 'Convert', 'reference': 'Reference',
              'resource': 'Resource', 'container': 'Container', 'nested': 'Nested', 'choice': 'Choice'}
SCALARS = {'u8': 1, 'bool': 1, 'i8': 1, 'u16': 2, 'i16': 2,
           'u32': 4, 'i32': 4, 'f32': 4, 'u64': 8, 'i64': 8, 'f64': 8, 'bytes': 1}
STATUS = {'已核', 'SDK声明', '推断', '未知'}
DYNAMIC = 0xFFFFFFFF

# 文档目录沿用总计划步骤号；布局编号与线格式不参与文档归类。
DOC_STEPS = {
    '02-base-types': ('2：基础值类型与嵌套规则', '2-base-types.md'),
    '03-entity-common': ('3：Entity 公共与主 Sprite', '3-entity-common.md'),
    '04-entity-classes': ('4：Entity 子类', '4-entity-classes.md'),
    '05-player': ('5：EntityPlayer', '5-player.md'),
    '06-grid': ('6：GridEntity', '6-grid.md'),
    '07-room-level': ('7：Room、Level、RoomSave 与 Exploration', '7-room-level.md'),
    '08-run-presentation': ('8：Run、Presentation 与 Random', '8-run-presentation.md'),
    '09-wire-format': ('9：公共报文结构', '9-wire-format.md'),
}
DOC_DOMAIN_STEPS = {
    'base.json': '02-base-types', 'entity.json': '03-entity-common',
    'entity-effect.json': '04-entity-classes', 'entity-classes.json': '04-entity-classes',
    'entity-text-dummy.json': '04-entity-classes', 'player.json': '05-player',
    'grid.json': '06-grid', 'grid-classes.json': '06-grid',
    'room.json': '07-room-level', 'level.json': '07-room-level',
    'run.json': '08-run-presentation', 'presentation.json': '08-run-presentation',
    'hud-local.json': '08-run-presentation',
    'random.json': '08-run-presentation',
    'wire.json': '09-wire-format',
}
# 正式布局源中的历史证据仍引用这两个旧路径，保留跳转页，不改写证据或布局版本。
DOC_LEGACY_PATHS = {'EntitySaveState': '05-player', 'GridEntityDesc': '06-grid'}
# 旧文档引用保留可读跳转；新版本不同时生成两份同编号有效布局。
DOC_RENAMED_TYPES = {'RoomTransitionResult': 'BossVersusResult'}


def doc_path(compiler, name):
    """同一类型只保留一份正式表，跨步骤引用使用其归属目录。"""
    return f'{compiler.doc_steps[name]}/{name}.md'


def read_json(path):
    def bad_constant(value):
        raise ValueError(f'{path}: JSON 不允许非有限常量 {value}')
    def unique_object(pairs):
        out = {}
        for key, value in pairs:
            require(key not in out, str(path), '重复 JSON 键: ' + key)
            out[key] = value
        return out
    return json.loads(path.read_text(encoding='utf-8'), parse_constant=bad_constant, object_pairs_hook=unique_object)


def require(condition, where, message):
    """构建错误带类型/字段路径，避免只报告泛化的生成失败。"""
    if not condition:
        raise ValueError(f'{where}: {message}')


def integer(value):
    if isinstance(value, str):
        return int(value, 0)
    if isinstance(value, int) and not isinstance(value, bool):
        return value
    raise ValueError(f'不是整数: {value!r}')


def scalar_size(name):
    match = re.fullmatch(r'(\w+)(?:\[(\d+)\])?', name)
    if not match or match[1] not in SCALARS:
        return None
    return SCALARS[match[1]] * int(match[2] or 1)


def nonempty(value):
    return isinstance(value, str) and bool(value.strip())


def quote(value):
    return json.dumps(str(value), ensure_ascii=False)


def write(path, value):
    """相同内容不重写，避免每次构建无故重编译所有消费单元。"""
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text(encoding='utf-8') != value:
        path.write_text(value, encoding='utf-8', newline='\n')


def schema_check(value, schema, definitions, where):
    """实现本仓库 schema 使用的 JSON Schema 子集，不引入构建机的额外 Python 包。"""
    if '$ref' in schema:
        return schema_check(value, definitions[schema['$ref'].rsplit('/', 1)[1]], definitions, where)
    if 'oneOf' in schema:
        matches = 0
        for candidate in schema['oneOf']:
            try:
                schema_check(value, candidate, definitions, where)
                matches += 1
            except ValueError:
                pass
        require(matches == 1, where, '不符合唯一结构类型')
        return
    types = schema.get('type', [])
    types = [types] if isinstance(types, str) else types
    predicates = {'object': lambda v: isinstance(v, dict), 'array': lambda v: isinstance(v, list),
                  'string': lambda v: isinstance(v, str), 'integer': lambda v: isinstance(v, int) and not isinstance(v, bool),
                  'number': lambda v: isinstance(v, (int, float)) and not isinstance(v, bool),
                  'boolean': lambda v: isinstance(v, bool), 'null': lambda v: v is None}
    require(not types or any(predicates[t](value) for t in types), where, 'JSON 类型错误')
    if 'enum' in schema:
        require(value in schema['enum'], where, '不在枚举集合中')
    if isinstance(value, dict):
        require(set(schema.get('required', [])).issubset(value), where, '缺少必需字段')
        for key, child in value.items():
            rule = schema.get('properties', {}).get(key, schema.get('additionalProperties', True))
            require(rule is not False, where + '.' + key, '未声明的属性')
            if isinstance(rule, dict):
                schema_check(child, rule, definitions, where + '.' + key)
    if isinstance(value, list):
        require(len(value) >= schema.get('minItems', 0) and len(value) <= schema.get('maxItems', DYNAMIC), where, '数组长度越界')
        if schema.get('uniqueItems'):
            rendered = [json.dumps(v, sort_keys=True) for v in value]
            require(len(rendered) == len(set(rendered)), where, '重复数组元素')
        if 'items' in schema:
            for i, child in enumerate(value):
                schema_check(child, schema['items'], definitions, f'{where}[{i}]')
    if isinstance(value, str):
        require(len(value) >= schema.get('minLength', 0), where, '字符串过短')
        if 'pattern' in schema:
            require(re.fullmatch(schema['pattern'], value), where, '字符串格式错误')
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        require(value >= schema.get('minimum', -float('inf')) and value <= schema.get('maximum', float('inf')), where, '数值越界')


class Compiler:
    """类型先展开继承和变体，再分块展开嵌套成员；任何本地地址都不能作为 copy 项。"""
    def __init__(self, source):
        self.source = source
        self.version = read_json(source / 'version.json')
        require(self.version.get('format') == 'isaac.cs.binary-state-layout', 'version', '布局源格式不匹配')
        # 帧头 layout_version 是 u16，布局源不能声明线上放不下的版本。
        require(0 < integer(self.version['version']) <= 0xFFFF, 'version', '版本必须是 u16 正整数')
        require(isinstance(self.version.get('domains'), list) and self.version['domains'] and len(self.version['domains']) == len(set(self.version['domains'])), 'version', '域清单为空或重复')
        # 资源空间只能在版本源中显式登记，拼写错误不能悄悄新建第二张同义资源表。
        resource_tables = self.version.get('resource_tables')
        require(isinstance(resource_tables, list) and resource_tables and
                all(isinstance(n, str) and re.fullmatch(r'[a-z][a-z0-9_-]*', n) for n in resource_tables) and
                len(resource_tables) == len(set(resource_tables)), 'version.resource_tables', '资源表清单为空、重名或非法')
        self.resource_tables = set(resource_tables)
        self.types = {}
        self.doc_steps = {}
        # 公共报文编号、枚举和预算与布局同源生成；这里只登记合同，不启用网络消息。
        self.protocol_constants = []
        self.resource_protocol = None
        self.budget_policy = None
        self.budget_workloads = []
        self.hud_local_policy = None
        constant_keys = set()
        schema = read_json(source / 'schema.json')
        for domain in self.version['domains']:
            require(re.fullmatch(r'[a-z][a-z0-9_-]*\.json', domain), domain, '非法域文件名')
            data = read_json(source / domain)
            schema_check(data, schema, schema['$defs'], domain)
            if 'budget_policy' in data:
                require(self.budget_policy is None, domain, '预算合同只能有一个唯一源')
                self.budget_policy = data['budget_policy']
            self.budget_workloads.extend(data.get('budget_workloads', []))
            if 'hud_local_policy' in data:
                require(self.hud_local_policy is None, domain, 'HUD本地映射只能有一个唯一源')
                self.hud_local_policy = data['hud_local_policy']
            if 'resource_protocol' in data:
                require(self.resource_protocol is None, domain, '资源合同只能有一个唯一源')
                self.resource_protocol = data['resource_protocol']
            for constant in data.get('protocol_constants', []):
                key = (constant['domain'], constant['name'])
                require(key not in constant_keys, domain, '公共协议常量重名')
                constant_keys.add(key)
                self.protocol_constants.append(constant)
            for entry in data['types']:
                name = entry['type']
                require(re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', name), name, '非法类型名')
                require(name not in self.types, name, '重复类型')
                self.types[name] = copy.deepcopy(entry)
                self.doc_steps[name] = DOC_DOMAIN_STEPS[domain]
        self.expanded = {}
        self.plans = {}
        self.native_cache = {}
        self.expand_stack = set()
        self.plan_stack = set()
        self.ids = set()

    def expand(self, name):
        if name in self.expanded:
            return self.expanded[name]
        require(name in self.types and name not in self.expand_stack, name, '未知父类型或继承循环')
        self.expand_stack.add(name)
        entry = copy.deepcopy(self.types[name])
        entry['native_size'] = integer(entry['native_size'])
        require((0 < entry['native_size'] <= 0x1000000) or
                (entry['native_size'] == 0 and entry.get('extent') == 'protocol' and not entry['ranges'] and not entry.get('parent')),
                name, '非法原生大小；纯协议记录必须明确无原生对象')
        entry['layout'] = integer(entry['layout'])
        require(0 < entry['layout'] < 65535 and entry['layout'] not in self.ids, name, '布局编号重复或越界')
        self.ids.add(entry['layout'])
        entry.setdefault('members', [])
        entry.setdefault('public_owners', [])
        entry.setdefault('validators', {})
        entry.setdefault('conversions', {})
        entry.setdefault('contexts', {})
        entry.setdefault('companions', [])
        entry.setdefault('count_groups', [])
        entry.setdefault('synthetic_fields', [])
        entry.setdefault('catalog_rows', [])
        entry.setdefault('supplementary_rows', [])
        require(isinstance(entry.get('rtti'), dict) and 'name' in entry['rtti'] and 'rva' in entry['rtti'], name, '缺少 RTTI 声明（值类型可为空）')
        require(0 <= integer(entry['rtti']['rva']) <= DYNAMIC, name, 'RTTI RVA 超过 u32')
        if entry.get('parent'):
            parent = self.expand(entry['parent'])
            # Dummy有独立RTTI但无尾部成员，允许等大继承；不能借此覆盖父类字节。
            require(parent['native_size'] <= entry['native_size'] and
                    (parent['native_size'] < entry['native_size'] or not entry['ranges']),
                    name, '子类不得小于父类，等大继承不得声明额外字节')
            entry['ranges'] = copy.deepcopy(parent['ranges']) + entry['ranges']
            parent_ids = {b['id'] for b in parent['blocks']}
            require(not parent_ids.intersection(b['id'] for b in entry['blocks']), name, '子类覆盖父类块号')
            require(not entry['blocks'] or min(b['id'] for b in entry['blocks']) > max(parent_ids, default=-1), name, '子类块必须排在父类之后')
            entry['blocks'] = copy.deepcopy(parent['blocks']) + entry['blocks']
            entry['validators'] = {**parent['validators'], **entry['validators']}
            entry['conversions'] = {**parent['conversions'], **entry['conversions']}
            entry['contexts'] = {**parent['contexts'], **entry['contexts']}
            entry['members'] = copy.deepcopy(parent['members']) + entry['members']
            entry['count_groups'] = copy.deepcopy(parent['count_groups']) + entry['count_groups']
            entry['synthetic_fields'] = copy.deepcopy(parent['synthetic_fields']) + entry['synthetic_fields']
            # 派生实体可把同名Plus伴随源收窄成派生Plus；公共块仍只发送一次。
            # 只有真实继承关系允许替换，不能借同名来源改解释成无关对象。
            companions = {c['name']: copy.deepcopy(c) for c in parent['companions']}
            for companion in entry['companions']:
                inherited = companions.get(companion['name'])
                if inherited:
                    ancestor = companion['type']
                    visited = set()
                    while ancestor != inherited['type'] and ancestor in self.types and ancestor not in visited:
                        visited.add(ancestor)
                        ancestor = self.types[ancestor].get('parent')
                    require(ancestor == inherited['type'], name + '.' + companion['name'], '伴随源只能收窄为原类型的派生类')
                companions[companion['name']] = copy.deepcopy(companion)
            require(len(entry['companions']) == len({c['name'] for c in entry['companions']}), name, '子类伴随来源重名')
            entry['companions'] = list(companions.values())
            # Player只调整自己继承块的序列，不改变普通Entity或父类字段布局。
            for block_id, sequence in entry.get('block_sequences', {}).items():
                require(int(block_id) in parent_ids, name, '只能调整真实父块的序列')
                next(b for b in entry['blocks'] if b['id'] == int(block_id))['sequence'] = sequence
        # 同一原生对象可有不同绘制视图；父类字节位置和块号保持不变，
        # 只替换明确声明的等大内嵌视图，避免为子类重复发送必被绘制覆写的值。
        for field, view in entry.get('nested_views', {}).items():
            require(entry.get('parent'), name, '内嵌视图替换只适用于真实派生类')
            item = next((r for r in entry['ranges'] if r['name'] == field), None)
            require(item and item in parent['ranges'] and item.get('wire', {}).get('kind') == 'nested'
                    and 'storage' not in item['wire'] and 'native_type' not in item,
                    name + '.' + field, '只能替换父类直接内嵌的状态视图')
            child = self.expand(view['type'])
            require(child.get('native_view_of') == item['wire']['type'] and
                    child['native_size'] == item['size'], name + '.' + field,
                    '替换须声明原视图且保持原生大小')
            item['type'] = item['wire']['type'] = view['type']
            item['semantics'], item['evidence'] = view['semantics'], view['evidence']
        self.validate_ranges(entry)
        if entry.get('native_view_of'):
            original = self.expand(entry['native_view_of'])
            require(not entry.get('parent') and entry['native_size'] == original['native_size'] and
                    [(r['name'], r['offset'], r['size']) for r in entry['ranges']] ==
                    [(r['name'], r['offset'], r['size']) for r in original['ranges']],
                    name, '原生视图必须保留原类型全部字节边界与名称')
            require(all(a['category'] != '未知' or b['category'] == '未知'
                        for a, b in zip(original['ranges'], entry['ranges'])),
                    name, '展示视图不得消除原生未知字节')
        self.expanded[name] = entry
        self.expand_stack.remove(name)
        return entry

    def validate_ranges(self, entry):
        """覆盖检查按偏移前进；绝不把空洞自动当填充或把未知段吞进复制区间。"""
        end = 0
        names = set()
        for item in entry['ranges']:
            where = f"{entry['type']}.{item.get('name', '?')}"
            item['offset'], item['size'] = integer(item['offset']), integer(item['size'])
            require(item['offset'] == end and item['size'] > 0, where, '字节重叠、留空或未排序')
            end += item['size']
            require(end <= entry['native_size'], where, '字节超出原生对象')
            require(item['category'] in CATEGORIES, where, '未知归属类别')
            require(item['status'] in STATUS, where, '非法证据状态')
            for key in ('name', 'type', 'semantics', 'evidence'):
                require(nonempty(item.get(key)), where, f'缺少 {key}')
            # 标量和 bytes[N] 标签会进入生成表与文档，声明宽度必须等于实际字节数。
            declared = scalar_size(item['type'])
            require(declared is None or declared == item['size'], where, '类型标签宽度与 size 不一致')
            require(item['name'] not in names, where, '重复字段名称')
            names.add(item['name'])
            if 'native_type' in item:
                child = self.types.get(item['native_type'])
                require(child and item['size'] == integer(child['native_size']) * item.get('native_count', 1),
                        where, '原生递归类型或数组范围不一致')
            transmitted = item['category'] in {'状态', '换算', '引用', '资源', '容器'}
            require(transmitted == ('wire' in item), where, '状态字段必须且仅能声明一份 wire 规则')
            if not transmitted:
                continue
            wire = item['wire']
            kind = wire.get('kind')
            # 条件或具名来源必须保留嵌套分派，不能把不存在的8B位置降为无条件memcpy。
            if (kind == 'nested' and self.types.get(wire.get('type'), {}).get('pure_value')
                    and 'when' not in wire and not wire.get('host') and not wire.get('client')):
                wire['kind'] = kind = 'copy'  # 纯值成员内联，允许与父块相邻状态合并。
            require(kind in OPERATIONS, where, '缺少合法传输操作')
            require('storage' not in wire or kind in {'container', 'choice'} or
                    (kind == 'nested' and wire['storage'] == 'shared-owner'), where, 'storage只适用于容器或间接共享owner')
            require('projections' not in wire or kind in {'nested', 'container'}, where, '只有嵌套类型或复杂容器元素可以按子块投影')
            require('max_bytes' not in wire or kind == 'container', where, '只有容器可以声明独立编码字节预算')
            require('count_type' not in wire or kind == 'container', where, '计数宽度只适用于容器')
            require('inline_count' not in wire or kind == 'container', where, '只有容器可以声明内联数组槽数')
            if kind == 'copy':
                require(item['category'] == '状态', where, '只能复制有语义的状态字节')
                require(item['type'] == wire.get('type') or (item['type'] == 'bool' and wire.get('type') == 'u8'), where, 'copy 不能把原生指针/容器/复杂类型改名为裸字节')
                width = scalar_size(wire.get('type', ''))
                if width is None:
                    nested = self.types.get(wire.get('type'))
                    require(nested and nested.get('pure_value'), where, '复杂嵌套类型不得整块复制')
                    width = integer(nested['native_size'])
                    # 纯值类型也必须通过展开后的逐字节归属，不能只相信一个标记。
                    pure = self.expand(nested['type'])
                    require(all(r['category'] == '状态' and r.get('wire', {}).get('kind') == 'copy' for r in pure['ranges']), where, '纯值类型含非值字节')
                require(width == item['size'], where, 'copy 宽度与原生值不一致')
            elif kind == 'nested':
                require(wire.get('type') in self.types and wire['type'] != entry['type'], where, '未知或自递归嵌套类型')
                if wire.get('storage') == 'shared-owner':
                    # holder两字只标记间接来源；具体payload逐字段投影，绝不按根对象内嵌读取。
                    require(item['size'] == 8 and item['category'] == '容器', where, '共享owner必须是8字节holder')
                    require(nonempty(wire.get('host')) and nonempty(wire.get('client')), where, '共享owner缺少具名读取/重建处理器')
                    require(not self.types[wire['type']].get('pure_value'), where, '共享owner必须保留具体payload布局')
                else:
                    require('storage' not in wire, where, '内嵌对象不能声明容器存储')
                    require(integer(self.types[wire['type']]['native_size']) == item['size'], where, '嵌套对象大小不一致')
            elif kind in {'reference', 'resource', 'convert'}:
                require(nonempty(wire.get('host')) and nonempty(wire.get('client')), where, '缺少具名双端处理器')
                require(scalar_size(wire.get('type', '')) is not None, where, '转换结果必须是定长值')
                if kind == 'reference':
                    # 固定关系数组也逐项保留身份；不是提前生成的Peer成员序号。
                    require(nonempty(wire.get('target')) and re.fullmatch(r'u16(?:\[[1-9][0-9]*\])?', wire['type']), where, '引用缺少目标空间或不是 u16固定值／数组')
                if kind == 'resource':
                    require(wire.get('target') in self.resource_tables, where,
                            '未登记资源表: ' + str(wire.get('target')) + '；请使用 version.resource_tables 的规范名称')
                if kind == 'convert':
                    require(wire['host'] in entry['conversions'] and wire['client'] in entry['conversions'], where, '换算处理器没有语义声明')
            elif kind == 'container':
                require(nonempty(wire.get('element')) and 0 < integer(wire.get('limit', 0)) < 65536, where, '容器没有元素类型或 u16 上限')
                # 聚合投影没有一对一原生元素：stride必须为0，禁止把map分组按结果pair步进。
                aggregate = wire.get('storage') == 'aggregate'
                require((integer(wire.get('stride', 0)) == 0 if aggregate else integer(wire.get('stride', 0)) > 0) and
                        nonempty(wire.get('host')) and nonempty(wire.get('client')), where, '容器缺少原生步长/具名聚合/重建处理器')
                element = wire['element']
                if 'max_bytes' in wire:
                    require(2 <= integer(wire['max_bytes']) < DYNAMIC, where, '容器编码预算必须容纳u16计数且小于u32边界')
                width = scalar_size(element)
                require(width is not None or element in self.types, where, '未知容器元素类型')
                require(aggregate or integer(wire['stride']) == (width if width is not None else integer(self.types[element]['native_size'])), where, '容器元素原生步长不一致')
                require(wire.get('storage') in {'vector', 'string', 'array', 'deque', 'c-string', 'ordered-map', 'set', 'aggregate'}, where, '容器存储未登记')
                if wire['storage'] == 'vector':
                    require(item['size'] == 12, where, 'J273 vector 头必须为 12 字节')
                elif wire['storage'] == 'string':
                    require(item['size'] == 24 and element == 'u8', where, 'J273 string 必须为 24 字节字节串')
                elif wire['storage'] == 'c-string':
                    # Text是内联终止字节串，没有string头或独立count；终止后的旧槽数据不上线。
                    require(element == 'u8' and item['type'] == f'bytes[{item["size"]}]' and
                            1 < item['size'] <= 256 and integer(wire['limit']) == item['size'] - 1 and
                            not any(key in wire for key in ('inline_count', 'count_field', 'count_from', 'projections')) and
                            nonempty(item.get('read_requires')), where,
                            '内联终止串须声明容量、容量减一上限及真实写入资格，不能伪造计数源')
                elif wire['storage'] == 'deque':
                    # NPC三组容器的真实ABI；不能把20B头当vector或连续数组复制。
                    require(item['size'] == 20 and integer(wire['stride']) in {4, 8, 12}, where,
                            'J273已核deque必须为20B头、4B引用/8B点/12B残影元素')
                else:
                    count = wire.get('count_field')
                    require('fixed_count' in wire or (nonempty(count) and count in {r['name'] for r in entry['ranges']}) or nonempty(wire.get('count_from')), where, '数组缺少计数字段或具名 owner 计数来源')
                # 内联数组的全部槽位都有原生字节归属；线上只发送当前有效前缀。
                # 普通 array 也可由本地指针解析，不能只按 size 或名称猜它是内联数组。
                if 'inline_count' in wire:
                    count = integer(wire['inline_count'])
                    require(wire['storage'] == 'array' and element in self.types and
                            item['size'] == count * integer(wire['stride']) and
                            integer(wire['limit']) <= count, where, '内联数组范围、步长或上限不一致')
                if 'fixed_count' in wire:
                    require(wire['storage'] == 'array' and wire['fixed_count'] == wire.get('inline_count') == wire['limit'],
                            where, '固定数组必须发送全部真实物理槽，且不编码count')
            elif kind == 'choice':
                require(wire.get('storage') == 'slots-mask' and 0 < wire.get('limit', 0) <= 8 and
                        item['size'] == wire['limit'] * 4 and nonempty(wire.get('host')) and nonempty(wire.get('client')),
                        where, '动态槽必须声明真实指针槽数及具名双端owner')
                choices = wire.get('choices', [])
                require(choices and len(choices) == len(set(choices)) and all(c in self.types for c in choices), where, '动态布局集合缺失或重复')
            if 'mask' in wire:
                require(kind == 'convert' and wire['type'] == 'u32' and 0 < integer(wire['mask']) <= 0xFFFFFFFF,
                        where, '位投影只能声明u32具名换算')
        require(end == entry['native_size'], entry['type'], '原生对象尾部未覆盖')

    def make_variants(self):
        """变体只替换明确覆盖的原生区间和具名块；其余父类与公共部分保持不变。"""
        for name in list(self.types):
            entry = self.expand(name)
            variants = entry.get('variants')
            if not variants:
                continue
            selector = next((r for r in entry['ranges'] if r['name'] == variants['selector']), None)
            require(selector and selector.get('shape'), name, '变体选择字段不是形态字段')
            used = set()
            for arm in variants['arms']:
                values = {integer(v) for v in arm['values']}
                require(len(values) == len(arm['values']) and not used.intersection(values) and values, name, '变体值重复或集合重叠')
                used |= values
                variant = copy.deepcopy(entry)
                variant['type'] = name + '_' + arm['name']
                variant['layout'] = integer(arm['layout'])
                require(re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', variant['type']), name, '非法变体名称')
                require(variant['type'] not in self.types and 0 < variant['layout'] < 65535 and variant['layout'] not in self.ids, name, '重复变体类型或编号')
                self.ids.add(variant['layout'])
                replacements = arm.get('ranges', [])
                affected = [r for r in variant['ranges'] if any(r['offset'] < integer(new['offset']) + integer(new['size']) and r['offset'] + r['size'] > integer(new['offset']) for new in replacements)]
                # 最后的全覆盖检查同时检查替换组首尾；允许把一段 union 拆成多段有语义字段。
                variant['ranges'] = [r for r in variant['ranges'] if r not in affected] + replacements
                variant['ranges'].sort(key=lambda r: integer(r['offset']))
                blocks = {b['id']: b for b in variant['blocks']}
                blocks.update({b['id']: b for b in arm.get('blocks', [])})
                variant['blocks'] = [blocks[k] for k in sorted(blocks)]
                variant['validators'] = {**variant['validators'], **arm.get('validators', {})}
                variant['conversions'] = {**variant['conversions'], **arm.get('conversions', {})}
                variant['members'] = [m for m in variant['members'] if m['id'] not in {n['id'] for n in arm.get('members', [])}] + arm.get('members', [])
                variant['variant_values'] = sorted(values)
                variant['variant_selector'] = variants['selector']
                variant['variant_base'] = entry['layout']
                variant['public_owners'] = []
                self.validate_ranges(variant)
                self.expanded[variant['type']] = variant
                self.doc_steps[variant['type']] = self.doc_steps[name]

    def plan(self, name):
        if name in self.plans:
            return self.plans[name]
        require(name not in self.plan_stack, name, '嵌套类型循环')
        self.plan_stack.add(name)
        entry = self.expanded[name]
        ranges = {r['name']: r for r in entry['ranges']}
        # C/S元数据没有游戏内偏移；生成FieldSource::Synthetic，绝不能落入memcpy计划。
        for item in entry['synthetic_fields']:
            key = item['name']
            require(key not in ranges, name + '.' + key, '合成字段重名')
            require(item['host'] in entry['conversions'] and item['client'] in entry['conversions'], name + '.' + key, '合成值缺少双端语义')
            wire = item.get('wire', dict(kind='convert', type=item['type'], host=item['host'], client=item['client']))
            if wire['kind'] == 'container':
                element = wire.get('element', '')
                # 显示结果列表由自有缓存提供，复杂元素按登记布局解码；零步长禁止把缓存误当原生数组。
                aggregate = wire.get('storage') == 'aggregate'
                require(((aggregate and integer(wire.get('stride', 0)) == 0 and element in self.expanded) or
                         (wire.get('storage') == 'array' and scalar_size(element) is not None and
                          scalar_size(element) == integer(wire.get('stride', 0)))) and
                        item['type'] == element and
                        0 < wire.get('limit', 0) <= 65535 and wire.get('host') == item['host'] and wire.get('client') == item['client'] and
                        not any(k in wire for k in ('fixed_count', 'inline_count', 'projections')), name + '.' + key, '合成容器必须是具名纯值数组或零步长复杂结果列表')
            elif wire['kind'] == 'nested':
                # 资源协议内嵌自有值没有原生对象；不借此放行任意原生类型／偏移。
                require(entry.get('extent') == 'protocol' and item['type'] in self.expanded and
                        self.expanded[item['type']].get('extent') == 'protocol' and
                        wire.get('type') == item['type'] and not any(k in wire for k in ('storage', 'block', 'projections')),
                        name + '.' + key, '合成嵌套只允许完整自有协议值')
            else:
                require(scalar_size(item['type']) is not None, name + '.' + key, '合成标量不是已核值类型')
                require(wire['kind'] in {'convert', 'reference', 'resource'} and wire.get('type') == item['type'] and
                        wire.get('host') == item['host'] and wire.get('client') == item['client'], name + '.' + key, '合成标量规则不一致')
                if wire['kind'] == 'reference':
                    require(item['type'] == 'u16' and nonempty(wire.get('target')), name + '.' + key, '合成引用缺少身份空间')
                if wire['kind'] == 'resource':
                    require(item['type'] == 'u16' and wire.get('target') in self.resource_tables,
                            name + '.' + key, '合成资源引用必须是u16并绑定已登记资源表')
                require('count_type' not in wire, name + '.' + key, '合成标量不能声明容器计数')
            ranges[key] = dict(item, offset=DYNAMIC, size=0, category='换算', synthetic=True,
                               wire=wire)
        # 伴随对象有自己的原生字节目录；这里只建立 owner 链，绝不伪造根对象偏移。
        for companion in entry['companions']:
            key, child = companion['name'], companion['type']
            require(key not in ranges and child in self.expanded and child != name, name + '.' + key, '伴随源名称冲突或类型不存在')
            ranges[key] = dict(companion, offset=DYNAMIC, size=0, category='状态',
                               companion=True, wire=dict(kind='nested', type=child,
                               host=companion['host'], client=companion['client']))
            if 'when' in companion:
                ranges[key]['wire']['when'] = companion['when']
        # ANM2 等嵌套对象的外观、动画和图层进入父类不同块；原生字节归属仍只声明一次。
        projected = {}
        for key, item in ranges.items():
            wire = item.get('wire', {})
            if 'projections' in wire:
                # Sprite数组/共享payload同样按元素子块投影，各块保留自己的count。
                # Client须先核各投影计数相同，再由同一owner安装，不能混用不同形态的元素。
                child_type = wire['type'] if wire['kind'] == 'nested' else wire['element']
                require(child_type in self.expanded, name + '.' + key,
                        '投影元素必须是已声明的复杂布局，收到: ' + str(child_type))
                nested = self.plan(child_type)
                # 实际发送子块必须投影，空预留子块允许省略；兼容既有空投影，原块号不变。
                expected = {b['id'] for b in nested['blocks'] if b['max'] != 0}
                allowed = {b['id'] for b in nested['blocks']}
                actual = set(wire['projections'])
                require(expected <= actual <= allowed, name + '.' + key,
                        f'子块投影不完整：元素 {child_type}，缺少 {sorted(expected - actual)}，非法 {sorted(actual - allowed)}')
                for block_id in wire['projections']:
                    virtual = copy.deepcopy(item)
                    virtual['name'] = f'{key}#{block_id}'
                    require(virtual['name'] not in ranges and virtual['name'] not in projected, name, '子块投影名称冲突')
                    virtual['wire']['block'] = block_id
                    projected[virtual['name']] = virtual
            elif 'wire' in item:
                projected[key] = item
        seen, ids, fields, blocks, copies = set(), set(), [], [], []
        previous_block = -1
        for block in entry['blocks']:
            number = integer(block['id'])
            require(0 <= number < 32 and number not in ids, name, '块号重复或超过 32 块')
            require(number > previous_block, name, '块必须按编号升序排列')
            previous_block = number
            ids.add(number)
            require(block.get('sequence') in {'world', 'body', 'world-or-player-body'} and nonempty(block.get('name')), name, '块缺少序列或语义名称')
            low, high, dynamic = 0, 0, False
            for key in block['fields']:
                where = name + '.' + key
                require(key in projected and key not in seen, where, '块字段不存在、重复或不产生状态')
                seen.add(key)
                item, wire = projected[key], projected[key]['wire']
                kind = wire['kind']
                nested = None
                if kind == 'copy':
                    minimum = maximum = item['size']
                    if wire['type'] in self.expanded:
                        nested = self.plan(wire['type'])
                        require(nested['min'] == nested['max'] == item['size'], where, '纯值嵌套类型的状态不等于原生大小')
                elif kind in {'reference', 'resource', 'convert'}:
                    minimum = maximum = scalar_size(wire['type'])
                elif kind == 'nested':
                    nested = self.plan(wire['type'])
                    if 'block' in wire:
                        child = next(b for b in nested['blocks'] if b['id'] == wire['block'])
                        minimum, maximum = child['min'], child['max']
                    else:
                        minimum, maximum = nested['min'], nested['max']
                    # 可选owner仅首个投影携带u8 present；缺失时各投影保留最短全零编码。
                    # 其它投影依赖同目标版本首块的present，不另加容器count或复制holder。
                    if wire.get('storage') == 'shared-owner' and wire.get('block', 0) == 0:
                        minimum, maximum = minimum + 1, maximum + 1
                elif kind == 'choice':
                    choices = [self.plan(child) for child in wire['choices']]
                    require(all(len(p['blocks']) == 1 for p in choices), where, '动态槽view必须只有一个值块')
                    minimum, maximum = 1, 1 + wire['limit'] * (2 + max(p['max'] for p in choices))
                else:
                    element = wire['element']
                    if element in self.expanded:
                        nested = self.plan(element)
                        width = next(b['max'] for b in nested['blocks'] if b['id'] == wire['block']) if 'block' in wire else nested['max']
                    else:
                        width = scalar_size(element)
                    count = wire.get('fixed_count')
                    child_min = (next(b['min'] for b in nested['blocks'] if b['id'] == wire['block']) if 'block' in wire else nested['min']) if nested else width
                    count_bytes = 1 if wire.get('count_type') == 'u8' else 2
                    require(count_bytes == 2 or (not count and wire['limit'] <= 255), where, 'u8计数不允许固定槽或超过255个元素')
                    minimum, maximum = (count * child_min, count * width) if count else (count_bytes, count_bytes + integer(wire['limit']) * width)
                    # 元素数上限与编码字节预算分别约束；复杂元素嵌套不能乘出无界理论分配。
                    # 实际编码/解码仍须检查真实长度，超预算报缺口，不能截断元素或降低count。
                    maximum = min(maximum, integer(wire.get('max_bytes', maximum)))
                if 'when' in wire:
                    require(kind != 'copy', where, '条件字段须经具名处理器，不能无条件合并复制')
                    minimum = 0
                require(minimum <= maximum, where, '字段编码预算小于格式下界')
                offset = DYNAMIC if dynamic else low
                field = dict(layout=entry['layout'], index=len(fields), block=number,
                             native_offset=item['offset'], native_size=item['size'], wire_offset=offset,
                             min_size=minimum, max_size=maximum, operation=OPERATIONS[kind],
                             name=key, type=wire.get('type', wire.get('element', 'layout-choice')), host=wire.get('host', ''),
                             client=wire.get('client', ''), target=wire.get('target', ''),
                             storage=wire.get('storage', ''), count_source=wire.get('count_field', wire.get('count_from', '')),
                             nested_layout=nested['entry']['layout'] if nested else 0,
                             nested_block=wire.get('block', 255),
                             limit=integer(wire.get('limit', 0)), stride=integer(wire.get('stride', 0)),
                             companion=item.get('companion', False),
                             synthetic=item.get('synthetic', False), fixed_count=wire.get('fixed_count', 0),
                             count_bytes=(0 if wire.get('fixed_count') else (1 if wire.get('count_type') == 'u8' else 2)) if kind == 'container' else 0,
                             mask=integer(wire.get('mask', 0)), condition=wire.get('when'),
                             choices=wire.get('choices', []),
                             shape=item.get('shape', False), read_requirement=item.get('read_requires', ''),
                             semantics=item['semantics'], source=item)
                require(len(fields) < 65535, where, '字段索引超过 u16')
                fields.append(field)
                if kind == 'copy':
                    # wire 紧邻和 native 紧邻都成立才合并；跨块、换算、指针和未知均断开。
                    if copies and copies[-1]['block'] == number and not dynamic and copies[-1]['offset'] + copies[-1]['size'] == item['offset'] and copies[-1]['wire'] + copies[-1]['size'] == offset and len(fields) >= 2 and fields[-2]['operation'] == 'Copy' and copies[-1]['read_requirement'] == field['read_requirement']:
                        copies[-1]['size'] += item['size']
                    else:
                        copies.append(dict(block=number, offset=item['offset'], wire=offset, size=item['size'], read_requirement=field['read_requirement']))
                low, high = low + minimum, high + maximum
                dynamic |= minimum != maximum
            require(low <= high < DYNAMIC, name, '块大小溢出')
            require(block['fields'] or block.get('reserved'), name, '空块必须明确 reserved')
            blocks.append(dict(id=number, name=block['name'], body=block['sequence'] == 'body',
                               player_body=block['sequence'] == 'world-or-player-body', min=low, max=high))
        require(ids == set(range(len(ids))), name, '块号必须从 0 连续编号，预留块需显式声明')
        require(seen == set(projected), name, '有状态字段或嵌套子块未归入块')
        if entry.get('pure_value'):
            require(len(blocks) == 1 and all(f['operation'] == 'Copy' and f['wire_offset'] == f['native_offset'] for f in fields)
                    and sum(f['native_size'] for f in fields) == entry['native_size'], name, '纯值类型必须逐成员按原生顺序内联且没有未复制字节')
        known = {f['name'] for f in fields}
        for f in fields:
            condition = f['condition']
            if condition:
                selector = next((v for v in fields if v['name'] == condition['field']), None)
                require(selector and selector['block'] == f['block'] and selector['index'] < f['index'] and
                        selector['min_size'] == selector['max_size'] and not selector['condition'], name + '.' + f['name'],
                        '条件选择器必须是同块先行无条件定长值')
                require(condition['predicate'] in entry['conversions'], name + '.' + f['name'], '条件谓词缺少具名语义')
        for key, context in entry['contexts'].items():
            require(re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', key), name, '非法上下文名称')
            require(all(nonempty(context.get(prop)) for prop in ('type', 'semantics', 'evidence')), name + '.' + key, '上下文缺少类型、语义或证据')
        context_names = {'@' + key for key in entry['contexts']}
        for member in entry['members']:
            require(nonempty(member.get('id')) and nonempty(member.get('reason')), name, '回映射缺少成员名或依据')
            require(member.get('disposition') in {'field', 'derived', 'authority', 'not_applicable'}, member['id'], '非法回映射处置')
            mapped = member.get('fields', [])
            require(all(f in ranges or f in known or f in context_names for f in mapped), member['id'], '回映射字段或父上下文不存在')
            if member['disposition'] in {'field', 'derived'}:
                require(mapped and all(f in known or (member['disposition'] == 'derived' and f in context_names) for f in mapped), member['id'], '值/派生成员没有对应状态字段或已核上下文')
        for key, validator in entry['validators'].items():
            require(key in known, name + '.' + key, '校验器没有对应传输字段')
            require(validator.get('kind') in {'finite', 'boolean', 'range', 'enum'}, name + '.' + key, '非法校验器')
            require(validator.get('scalar') in SCALARS, name + '.' + key, '校验器没有元素标量类型')
            field = next(f for f in fields if f['name'] == key)
            if not field['nested_layout'] and field['operation'] in {'Copy', 'Convert', 'Reference', 'Resource'}:
                scalar = field['type'].split('[', 1)[0]
                require(validator['scalar'] == scalar or {validator['scalar'], scalar} == {'bool', 'u8'}, name + '.' + key, '校验标量与线上元素类型不一致')
            if validator['kind'] == 'range':
                require(validator['min'] <= validator['max'], name + '.' + key, '校验范围颠倒')
            if validator['kind'] == 'enum':
                require(validator.get('values') and len(set(validator['values'])) == len(validator['values']), name + '.' + key, '空枚举或重复枚举值')
        for f in fields:
            if f['operation'] == 'Copy' and not f['nested_layout']:
                scalar = f['type'].split('[', 1)[0]
                validator = entry['validators'].get(f['name'], {})
                if scalar in {'f32', 'f64'}:
                    require(validator.get('kind') in {'finite', 'range'} and validator.get('scalar') == scalar, name + '.' + f['name'], '浮点状态缺少有限值校验')
                if f['source']['type'] == 'bool':
                    require(validator.get('kind') == 'boolean' and validator.get('scalar') in {'bool', 'u8'}, name + '.' + f['name'], '布尔状态缺少 0/1 校验')
        # 显式组限制多个原生容器的合计元素数；分块投影的重复count只算一次。
        groups = copy.deepcopy(entry['count_groups'])
        grouped, group_names = set(), set()
        for group in groups:
            require(group['name'] not in group_names, name, '计数组名称重复: ' + group['name'])
            group_names.add(group['name'])
            for key in group['fields']:
                require(key in ranges and ranges[key].get('wire', {}).get('kind') == 'container',
                        name + '.' + group['name'], '计数组成员不是容器: ' + key)
                require(key not in grouped, name + '.' + key, '容器属于多个计数组')
                grouped.add(key)
        for key, item in ranges.items():
            wire = item.get('wire', {})
            if wire.get('kind') == 'container' and 'projections' in wire and key not in grouped:
                groups.append(dict(name='投影一致性:' + key, fields=[key], limit=wire['limit'], evidence=item['evidence']))
        count_constraints = []
        require(len(groups) < 65535, name, '计数组数量超过u16')
        for group_id, group in enumerate(groups):
            require(len(group['fields']) < 65535, name, '计数组槽位超过u16')
            for slot, key in enumerate(group['fields']):
                matching = [f for f in fields if f['name'] == key or f['name'].startswith(key + '#')]
                require(matching, name + '.' + key, '计数组容器未生成状态字段')
                for f in matching:
                    count_constraints.append(dict(layout=entry['layout'], group=group_id, slot=slot,
                                                  field=f['index'], limit=group['limit']))
        native_ranges = self.native_ranges(name)
        unknown = sum(r['size'] for r in native_ranges if r['category'] == '未知')
        complete = entry.get('extent', 'object') in {'object', 'protocol'} and not unknown and all(r['status'] == '已核' for r in entry['ranges'] + entry['companions']) and all(self.plans[f['type']]['complete'] for f in fields if f['nested_layout'])
        minimum, maximum = sum(b['min'] for b in blocks), sum(b['max'] for b in blocks)
        if 'record_limit' in entry:
            require(minimum <= entry['record_limit'] < DYNAMIC, name, '记录预算小于布局下界或溢出')
            maximum = min(maximum, entry['record_limit'])
        result = dict(entry=entry, fields=fields, blocks=blocks, copies=copies, unknown=unknown,
                      native_ranges=native_ranges, count_constraints=count_constraints, count_groups=groups,
                      complete=complete, min=minimum, max=maximum)
        self.plans[name] = result
        self.plan_stack.remove(name)
        return result

    def native_ranges(self, name):
        """原生字节归属递归展开内嵌对象，供未知区探测；不能把整份 AnimationState 误标为状态。"""
        if name in self.native_cache:
            return self.native_cache[name]
        result = []
        confidence = {'已核': 0, 'SDK声明': 1, '推断': 2, '未知': 3}
        for item in self.expanded[name]['ranges']:
            wire = item.get('wire', {})
            nested = item.get('native_type', wire.get('type'))
            inline_count = item.get('native_count', wire.get('inline_count', 0))
            if inline_count:
                nested = item.get('native_type', wire.get('element', nested))
            if inline_count or ('native_type' in item) or (wire.get('kind') in {'copy', 'nested'} and wire.get('storage') != 'shared-owner' and nested in self.expanded):
                # 即使有效前缀为空，未发送槽位中的 UNKNOWN 也不能从原生目录消失。
                children = self.native_ranges(nested)
                for index in range(inline_count or 1):
                    for child in children:
                        value = copy.deepcopy(child)
                        stride = self.expanded[nested]['native_size']
                        value['offset'] += item['offset'] + (index * stride if inline_count else 0)
                        prefix = item['name'] + (f'[{index}]' if inline_count else '')
                        value['name'] = prefix + '.' + child['name']
                        value['semantics'] = item['semantics'] + '：' + child['semantics']
                        value['evidence'] = item['evidence'] + '；嵌套字段：' + child['evidence']
                        value['status'] = max((item['status'], child['status']), key=confidence.__getitem__)
                        # 不上线的保存体仍展开未知；内嵌状态不能反向恢复被裁的Host输入。
                        if item['category'] in {'权威', '本地'} and value['category'] not in {'未知', '填充', '本地'}:
                            value['category'] = item['category']
                        result.append(value)
            else:
                result.append(item)
        self.native_cache[name] = result
        return result

    def catalogs(self):
        """仅要求已声明接管的 owner；后续域未入源前明确不宣称全域目录覆盖。"""
        expected = {}
        sources = {}
        for rel in self.version['catalogs']:
            path = (ROOT / rel).resolve()
            require(path.is_relative_to(ROOT), rel, '目录输入越出仓库')
            data = read_json(path)
            # 旧stream目录的原返回类型叫type且没有owner；仅在内存中适配，原文件和签名保持。
            # 它按source_schema分域登记逐行处置，不扩大其他既有public_owners的覆盖声明。
            stream = data.get('format') == 'isaac.cs.stream.catalog'
            sources[rel] = [dict(member, owner=member['id'].split('.', 1)[0],
                                 returns=member['type'], domain=member['source_schema'].removeprefix('isaac.cs.'))
                            for member in data['members']] if stream else data['members']
            if stream:
                continue
            for member in data['members']:
                if member.get('ownership') == 'ReplicatedState':
                    expected.setdefault(member['owner'], set()).add(member['id'])
        owners = set()
        for plan in self.plans.values():
            entry = plan['entry']
            mapped = [m['id'] for m in entry['members']]
            require(len(mapped) == len(set(mapped)), entry['type'], '重复回映射')
            for owner in entry['public_owners']:
                require(owner not in owners and owner in expected, owner, '重复或不存在的公开 owner')
                owners.add(owner)
                require(expected[owner].issubset(mapped), owner, 'ReplicatedState 缺少回映射: ' + ', '.join(sorted(expected[owner] - set(mapped))))
        # 正式逐行回映射不能按ID或签名去重；保留目录原始null与空字符串。
        catalog_rows = {}
        for plan in self.plans.values():
            entry = plan['entry']
            rows = entry['catalog_rows']
            covered = {}
            for row in rows:
                catalog = row['catalog']
                require(catalog in self.version['catalogs'], entry['type'], '逐行来源未登记')
                source = sources[catalog]
                ordinal = row['ordinal']
                require(0 < ordinal <= len(source) and ordinal not in covered.setdefault(catalog, set()), catalog, '目录行越界或重复')
                covered[catalog].add(ordinal)
                actual = source[ordinal - 1]
                require(all(row[key] == actual.get(key) for key in ('id', 'owner', 'parameters', 'returns')), f'{catalog}:{ordinal}', '目录行身份或原签名不一致')
                self.validate_member(entry, plan, row)
                catalog_rows.setdefault(catalog, []).append((entry, row))
            for catalog in entry.get('catalog_complete', []):
                require(catalog in self.version['catalogs'], entry['type'], '全量目录未登记')
                require(covered.get(catalog) == set(range(1, len(sources[catalog]) + 1)), catalog, '逐行回映射未覆盖完整目录')
            supplementary_keys = set()
            for row in entry['supplementary_rows']:
                key = row['source'], row['ordinal']
                require(key not in supplementary_keys, entry['type'], '补充成员来源行重复')
                supplementary_keys.add(key)
                self.validate_member(entry, plan, row)
            alias_keys = set()
            for alias in entry.get('catalog_aliases', []):
                catalog, ordinal, target = alias['catalog'], alias['ordinal'], alias['target_ordinal']
                require(catalog in sources and 0 < ordinal <= len(sources[catalog]), entry['type'], '交叉目录来源行不存在')
                row = next((r for r in rows if r['ordinal'] == target and r['catalog'] == alias['target_catalog']), None)
                require(row and all(row[k] == sources[catalog][ordinal - 1].get(k) for k in ('id', 'owner', 'parameters', 'returns')),
                        entry['type'], '交叉目录目标行不存在或原签名不同')
                key = catalog, ordinal, alias['target_catalog'], target
                require(key not in alias_keys, entry['type'], '交叉目录链接重复')
                alias_keys.add(key)

        # 一个公开域可分布在多个真实布局中；完整声明按原目录行聚合，不能按ID去重。
        # 只检查显式声明的目录，不把尚未研究的Room等域变成当前工作的前置条件。
        complete = self.version.get('catalog_complete', [])
        require(isinstance(complete, list) and all(isinstance(c, str) for c in complete) and
                len(complete) == len(set(complete)), 'version.catalog_complete', '完整目录声明非法或重复')
        for catalog in complete:
            require(catalog in sources, catalog, '全量目录未登记')
            ordinals = [row['ordinal'] for _, row in catalog_rows.get(catalog, [])]
            require(len(ordinals) == len(set(ordinals)) and
                    set(ordinals) == set(range(1, len(sources[catalog]) + 1)), catalog, '跨布局逐行回映射缺失或重复')
        # 四域共用的目录可以只完成其中一个真实domain；按原始行号检查，
        # 不能强迫未研究的Presentation/Random/Mod入源，也不能漏掉新的Run行。
        scopes = self.version.get('catalog_complete_scopes', [])
        require(isinstance(scopes, list), 'version.catalog_complete_scopes', '分域完整目录声明必须是数组')
        outputs = [(catalog, None) for catalog in complete]
        for scope in scopes:
            require(isinstance(scope, dict) and set(scope) == {'catalog', 'domain'},
                    'version.catalog_complete_scopes', '分域声明只允许catalog与domain')
            catalog, domain = scope['catalog'], scope['domain']
            require(isinstance(catalog, str) and catalog in sources and isinstance(domain, str) and
                    re.fullmatch(r'[a-z][a-z0-9_-]*', domain), 'version.catalog_complete_scopes', '目录未登记或domain非法')
            require((catalog, domain) not in outputs and catalog not in complete, catalog, '分域完整声明重复')
            expected_rows = {i for i, member in enumerate(sources[catalog], 1) if member.get('domain') == domain}
            ordinals = [row['ordinal'] for _, row in catalog_rows.get(catalog, []) if row['ordinal'] in expected_rows]
            require(expected_rows and len(ordinals) == len(set(ordinals)) and set(ordinals) == expected_rows,
                    catalog + ':' + domain, '分域逐行回映射缺失或重复')
            outputs.append((catalog, domain))
        self.catalog_outputs = outputs
        self.catalog_rows = catalog_rows
        self.catalog_sources = sources

    def validate_member(self, entry, plan, member):
        """逐行处置引用正式字段或明确的跨域上下文；保留未来adapter尚未实施的边界。"""
        known = {f['name'] for f in plan['fields']} | {'@' + k for k in entry['contexts']}
        require(all(k in known for k in member.get('fields', [])), member['id'], '逐行回映射指向不存在的正式字段/上下文')
        require(member['disposition'] not in {'field', 'derived'} or member.get('fields'), member['id'], '逐行状态/派生缺少来源')

    def compile(self):
        for name in self.types:
            self.expand(name)
        self.make_variants()
        for name in self.expanded:
            self.plan(name)
        # 合同跟随结果布局生成；声明依赖必须能解析到真实类型，且不能跨序列偷换提交范围。
        for name, plan in self.plans.items():
            contract = plan['entry'].get('presentation_contract')
            if not contract:
                continue
            require(plan['fields'] and all(b['body'] == (contract['sequence'] == 'body')
                                          for b in plan['blocks'] if b['max']),
                    name, '显示合同与实际块序列不符或没有结果负载')
            require(contract['atomic_scope'] == contract['sequence'] + '-target',
                    name, '显示合同共同提交范围不符')
            for dependency in contract['dependency_types']:
                require(dependency in self.expanded and dependency != name,
                        name, '显示依赖类型不存在或自依赖: ' + dependency)
            require('record_limit' in plan['entry'], name, '显示结果必须声明总编码预算')
        self.catalogs()
        return sorted(self.plans.values(), key=lambda p: p['entry']['layout'])


def array(typ, name, count):
    """各类代码表共用的数组声明；Grid与普通布局生成保持同一作用域。"""
    return f'constexpr std::array<{typ}, {count}> {name} = {{{{\n'


def emit_grid_description(compiler, plans, out):
    """为第6步描述视图生成语义角色到数字字段的映射，不复制另一份原生偏移账本。"""
    roles = ['desc', 'grid_index', 'water_clip_flags',
             'sprite#0', 'sprite#1', 'sprite#2',
             'fire_sprite#0', 'fire_sprite#1', 'fire_sprite#2',
             'ember_sprite#0', 'ember_sprite#1', 'ember_sprite#2',
             'timer_plate#0', 'timer_plate#1', 'timer_plate#2',
             'extra_sprite#0', 'extra_sprite#1', 'extra_sprite#2',
             'embers', 'rubble_animation', 'rubble_offsets',
             'target_room_type', 'extra_visible', 'target_room_index', 'door_flags']
    selected = []
    for plan in plans:
        entry = plan['entry']
        ancestor = entry
        while ancestor.get('parent') and ancestor['type'] != 'GridEntity':
            ancestor = compiler.expanded[ancestor['parent']]
        if ancestor['type'] == 'GridEntity' and entry['type'] != 'GridEntity':
            selected.append(plan)
    # 尚未研究Grid的源也可生成空视图；一旦接入真实类，所有发送字段必须有明确角色。
    body = ['// 同源生成的Grid描述计划；只修改正式布局源和生成器。\n',
            f'static_assert(std::size_t(ClientGridFieldRole::Count) == {len(roles)});\n',
            array('GridDescriptionPlan', 'kGridDescriptionPlans', len(selected))]
    for plan in selected:
        entry = plan['entry']
        fields = {f['name']: f for f in plan['fields']}
        require(set(fields).issubset(roles), entry['type'], 'Grid发送字段缺少类型化描述角色')
        require(all(f['wire_offset'] != DYNAMIC for f in plan['fields']), entry['type'], 'Grid根视图需要明确的块内字段起点')
        desc = fields['desc']
        desc_fields = {f['name']: f for f in compiler.plans[desc['type']]['fields']}
        require(all(f['min_size'] == f['max_size'] == 4 for f in desc_fields.values()),
                entry['type'], 'Grid描述标量不再是当前32位值，须同步调整类型化读取')
        offsets = [desc_fields[n]['wire_offset'] if n in desc_fields else DYNAMIC
                   for n in ('type', 'variant', 'state', 'spawn_seed')]
        require(all(offset != DYNAMIC for offset in offsets[:3]), entry['type'], 'Grid描述缺少三个固定展示标量')
        mask = sum(1 << b['id'] for b in plan['blocks'] if b['max'])
        ids = [fields[n]['index'] if n in fields else 65535 for n in roles]
        for prefix in ('sprite', 'fire_sprite', 'ember_sprite', 'timer_plate', 'extra_sprite'):
            group = [fields.get(f'{prefix}#{i}') for i in range(3)]
            if any(group):
                require(all(group) and len({f['nested_layout'] for f in group}) == 1 and
                        [f['nested_block'] for f in group] == [0, 1, 2], entry['type'], 'Grid三份Sprite投影不一致')
        # 两个值容器有明确固定元素codec；源变更须同步维护读取器，不能默读旧偏移。
        for name, size, expected in (
                ('embers', 16, {'position': (0, 8), 'age': (8, 4), 'max_life': (12, 4)}),
                ('rubble_offsets', 8, {'x': (0, 4), 'y': (4, 4)})):
            if name in fields:
                element = compiler.plans[fields[name]['type']]
                actual = {f['name']: (f['wire_offset'], f['min_size']) for f in element['fields']}
                require(actual == expected and len(element['blocks']) == 1 and
                        element['blocks'][0]['min'] == element['blocks'][0]['max'] == size,
                        entry['type'], 'Grid固定值容器codec与正式元素布局不一致')
        body.append(f'  {{{entry["layout"]}, {mask}u, {{{",".join(map(str, ids))}}}, '
                    + '{' + ','.join(f'{v}u' for v in offsets) + '}},\n')
    body.append('}};\n')
    # 工厂type→RTTI事实复用已有world_layout，不再手写第二份27类switch。
    world = read_json(ROOT / 'mod/binding/world_layout.json')
    by_name = {p['entry']['type']: p['entry']['layout'] for p in selected}
    kinds = sorted((g for g in world['grid_types'] if g['rtti']), key=lambda g: g['value']) if selected else []
    require(all('GridEntity' + g['rtti'] in by_name for g in kinds), 'GridFactory', '工厂真实类没有描述视图')
    body.append(array('GridFactoryType', 'kGridFactoryTypes', len(kinds)))
    for kind in kinds:
        body.append(f'  {{{kind["value"]}, {by_name["GridEntity" + kind["rtti"]]}}},\n')
    body.append('}};\n')
    write(out / 'state_grid_view.inc', ''.join(body))


def emit_room_description(compiler, plans, out):
    """第7步只生成已登记字段的数字索引；不另写一份线偏移或保存图标帧界。"""
    groups = [
        ('Room', 'Room', ['descriptor', 'dimension', 'room_type', 'grid_width', 'grid_height', 'bounds',
          'frame_baseline', 'greed_wave_timer', 'floor_color', 'wall_color', 'water_amount', 'lava_intensity',
          'render_scroll', 'door_slots', 'door_grid_positions', 'backdrop', 'fx_layers', 'water_visual',
          'lighting', 'waves', 'forced_focus', 'rails', 'hell_backdrop', 'wave_sprite_present', 'shockwave_overlay_sprite']),
        ('LevelGamePrefix', 'Level', ['stage', 'stage_type', 'effective_curses', 'current_entry_index',
          'current_dimension', 'greed_wave', 'current_is_last_boss', 'room_offset', 'current_visual']),
        ('RoomDescriptor', 'Descriptor', ['grid_index', 'safe_grid_index', 'dimension', 'data', 'override_data',
          'allowed_doors', 'doors', 'display_flags', 'visited_count', 'flags', 'greed_treasure_icon', 'map_icons']),
        ('RoomConfig_Room', 'Configuration', ['StageId', 'Type', 'Subtype', 'Width', 'Height', 'Shape'])]
    by_name = {p['entry']['type']: p for p in plans}
    selected = [by_name[name] for name, _, _ in groups if name in by_name]
    require(not selected or len(selected) == len(groups), 'RoomDescription', 'Room／Level描述所需正式根必须一起登记')
    role_count = sum(len(fields) for _, _, fields in groups)
    body = ['// 与正式布局同源生成；不得在描述层手写布局号、线偏移或图标帧界。\n',
            f'constexpr std::size_t kWorldDescriptionRoles = {role_count};\n',
            f'constexpr std::size_t kWorldDescriptionMaxFields = {max((len(p["fields"]) for p in selected), default=1)};\n',
            array('WorldDescriptionPlan', 'kWorldDescriptionPlans', len(selected))]
    start = 0
    for name, kind, roles in groups:
        if name in by_name:
            plan = by_name[name]
            fields = {f['name']: f for f in plan['fields']}
            require(set(fields) == set(roles) and all(f['wire_offset'] != DYNAMIC for f in fields.values()),
                    name, '描述角色未覆盖全部字段或根字段不再有固定起点，须同步更新描述视图')
            ids = [65535] * role_count
            for i, role in enumerate(roles):
                ids[start + i] = fields[role]['index']
            mask = sum(1 << b['id'] for b in plan['blocks'] if b['max'])
            body.append(f'  {{ClientWorldDescriptionKind::{kind}, {plan["entry"]["layout"]}, {mask}u, '
                        + '{' + ','.join(map(str, ids)) + '}},\n')
        start += len(roles)
    body.append('}};\n')
    # L2只解析明确登记的九个按位条件帧，不从旧保存体／图标名字重新推断类别。
    frames = []
    offsets = [0, 0, 0]
    if selected:
        outer = {f['name']: f for f in by_name['FinalSavedMapIcons']['fields']}
        payload = {f['name']: f for f in by_name['SavedMapIconPayload']['fields']}
        offsets = [outer['present']['wire_offset'], outer['payload']['wire_offset'], payload['icon_mask']['wire_offset']]
        require(offsets == [0, 1, 0] and payload['icon_mask']['min_size'] == 2,
                'FinalSavedMapIcons', '图标presence／mask不再是当前有界编码')
        for key, f in payload.items():
            if key == 'icon_mask':
                continue
            match = re.fullmatch(r'frame_bit(\d+)', key)
            require(match and f['type'] == 'u8' and f['condition'] and f['condition']['field'] == 'icon_mask',
                    'SavedMapIconPayload.' + key, '图标动态帧不再是mask的具名u8条件字段')
            bit = int(match[1])
            predicate = by_name['SavedMapIconPayload']['entry']['conversions'][f['condition']['predicate']]
            validator = by_name['SavedMapIconPayload']['entry']['validators'][key]
            require(0 <= bit < 16 and predicate == f'(icon_mask & {1 << bit})!=0' and
                    validator['kind'] == 'range' and validator['min'] == 0 and 0 <= validator['max'] <= 255,
                    key, '图标谓词或帧界不符合正式按位解码合同')
            frames.append((bit, validator['max']))
        require([bit for bit, _ in frames] == sorted(set(bit for bit, _ in frames)),
                'SavedMapIconPayload', '图标帧必须按唯一bit升序排列')
    body += [f'constexpr std::array<std::uint32_t,3> kSavedIconOffsets = {{{",".join(map(str, offsets))}}};\n',
             array('SavedIconFramePlan', 'kSavedIconFrames', len(frames))]
    for bit, maximum in frames:
        body.append(f'  {{{bit}, {maximum}}},\n')
    body.append('}};\n')
    door_layout = by_name.get('GridEntityDoor', {}).get('entry', {}).get('layout', 0)
    body.append(f'constexpr std::uint16_t kWorldDoorLayout = {door_layout};\n')
    grids = sorted(p['entry']['layout'] for p in plans if p['entry'].get('parent') == 'GridEntity')
    body.append(f'constexpr std::array<std::uint16_t,{len(grids)}> kWorldGridLayouts = '
                + '{' + ','.join(map(str, grids)) + '};\n')
    write(out / 'state_room_view.inc', ''.join(body))


def emit_body_codec(compiler, plans, out, docs):
    """将标量、条件和计数组编译为数字表；正文热路径不解析名称或语义字符串。"""
    fields, validators, enum_values, rules = [], [('Validation::None', 'ScalarKind::U8', 0, 0, 0, 0)], [], []
    storage_names = {'': 'None', 'vector': 'Vector', 'string': 'String', 'array': 'Array',
                     'deque': 'Deque', 'c-string': 'CString', 'ordered-map': 'OrderedMap',
                     'set': 'Set', 'aggregate': 'Aggregate', 'shared-owner': 'SharedOwner', 'slots-mask': 'SlotsMask'}
    predicate_names = {'equal': 'Equal', 'not-equal': 'NotEqual', 'any-bits': 'AnyBits',
                       'one-of': 'OneOf', 'item-config': 'ItemConfig'}
    resource_ids = {c['name']: c['value'] for c in compiler.protocol_constants if c['domain'] == 'resource_table'}

    def scalar(name):
        match = re.fullmatch(r'(u8|i8|u16|i16|u32|i32|u64|i64|f32|f64|bool|bytes)(?:\[(\d+)\])?', name)
        if not match:
            return 'None', 0
        return {'bool': 'U8', 'bytes': 'U8'}.get(match[1], match[1].upper()), int(match[2] or 1)

    def bits(value, name):
        if name in {'f32', 'f64'}:
            return int.from_bytes(struct.pack('<f' if name == 'f32' else '<d', value), 'little')
        width = 8 * SCALARS[name]
        signed = name.startswith('i')
        low, high = (-(1 << (width-1)), (1 << (width-1))-1) if signed else (0, (1 << width)-1)
        require(isinstance(value, int) and low <= value <= high, 'body validator', '整数校验边界超出标量宽度')
        return int(value) & ((1 << (8 * SCALARS[name])) - 1)

    for p in plans:
        entry = p['entry']
        canonical = {}
        for f in p['fields']:
            canonical.setdefault(f['name'].split('#', 1)[0], f['index'])
        constraints = {c['field']: c for c in p['count_constraints']}
        for f in p['fields']:
            atom, components = scalar(f['type']) if not f['nested_layout'] else ('None', 0)
            require(atom != 'None' or f['nested_layout'] or f['operation'] == 'Choice', entry['type'], '正文标量类型未登记')
            require(components <= 65535, entry['type'], '单字段固定标量数组超过当前数字计划容量')
            require(f['operation'] != 'Resource' or f['target'] in resource_ids, entry['type'], '资源字段缺少公共空间编号')
            if f['storage'] == 'set':
                require(components == 1 and atom not in {'None', 'F32', 'F64'}, entry['type'], 'set必须是有序整数纯值')
            if f['storage'] == 'ordered-map':
                require(f['nested_layout'] and f['nested_block'] == 255, entry['type'], '有序map须包含完整key/value元素')
            validation = 0
            v = entry['validators'].get(f['name'])
            if v:
                require(atom != 'None' or v['kind'] == 'finite', entry['type'], '嵌套字段的非有限值校验须下沉到真实标量')
                validation = len(validators)
                first = len(enum_values)
                enum_values.extend(bits(value, v['scalar']) for value in v.get('values', []))
                validators.append(('Validation::' + v['kind'].title(), 'ScalarKind::' + scalar(v['scalar'])[0],
                                   bits(v.get('min', 0), v['scalar']), bits(v.get('max', 0), v['scalar']), first, len(enum_values) - first))
            selector, predicate, value, mask = 65535, 'None', 0, 0
            if f['condition']:
                c = f['condition']
                selector = next(n['index'] for n in p['fields'] if n['name'] == c['field'])
                selector_plan = p['fields'][selector]
                selector_scalar, selector_components = scalar(selector_plan['type'])
                require(0 < selector_plan['max_size'] <= 8 and not selector_plan['nested_layout'] and selector_components == 1 and
                        selector_scalar not in {'None', 'F32', 'F64'}, entry['type'], '条件选择器须为先行整数标量')
                test = c['test']
                predicate = predicate_names[test['kind']]
                value = test.get('value', 0)
                mask = sum(1 << n for n in test.get('values', []))
                require((predicate in {'OneOf', 'ItemConfig'}) == bool(test.get('values')), entry['type'], '条件集合与操作不匹配')
                require(predicate in {'OneOf'} or 'value' in test, entry['type'], '条件缺少操作数')
                if predicate == 'ItemConfig':
                    s = p['fields'][selector]
                    require(s['operation'] == 'Resource' and s['target'] == 'item-config', entry['type'], '配置条件须绑定真实item-config资源')
            constraint = constraints.get(f['index'])
            group = slot = 65535
            limit = copies = slots = 0
            if constraint:
                group, slot, limit = constraint['group'], constraint['slot'], constraint['limit']
                copies = sum(c['group'] == group and c['slot'] == slot for c in p['count_constraints'])
                slots = len({c['slot'] for c in p['count_constraints'] if c['group'] == group})
            rule = {'non-null': 'NonNull', 'nullable-water': 'NullableWater',
                    'entity-counter': 'EntityCounter', 'countdown': 'Countdown',
                    'display-utf8': 'DisplayUtf8'}.get(f['source']['wire'].get('value_rule'), 'None')
            require(rule == 'None' or (rule == 'DisplayUtf8' and atom == 'U8' and components == 1 and
                    f['operation'] == 'Container' and f['storage'] == 'array' and not f['fixed_count']) or
                    (rule != 'DisplayUtf8' and components == 1 and atom == ('U16' if rule == 'NonNull' else 'U64')),
                    entry['type'], '具名线上编码规则的标量宽度不匹配')
            fields.append((atom, components, storage_names[f['storage']], resource_ids.get(f['target'], 255),
                           canonical[f['name'].split('#', 1)[0]], selector, predicate, value, mask, rule, validation,
                           group, slot, limit, copies, slots))
        body_rule = entry.get('body_rule')
        if body_rule:
            first = 'item_id' if body_rule == 'empty-active-zero' else 'index'
            second = 'layer_id' if body_rule == 'null-costume-layer-zero' else None
            rules.append((entry['layout'], 'EmptyActiveZero' if not second else 'NullCostumeLayerZero',
                          next(f['index'] for f in p['fields'] if f['name'] == first),
                          next(f['index'] for f in p['fields'] if f['name'] == second) if second else 65535))
            selector_index = rules[-1][2]
            targets = [f for f in p['fields'] if f['index'] != selector_index] if not second else [p['fields'][rules[-1][3]]]
            require(all(f['index'] > selector_index and f['block'] == p['fields'][selector_index]['block'] and
                        f['min_size'] == f['max_size'] and scalar(f['type'])[0] != 'None' for f in targets),
                    entry['type'], '规范零负载须在同块选择器之后且为定长纯值')
    body = ['// 同源生成的正文计划；所有名称仅供诊断，不参与热分派。\n',
            f'constexpr std::size_t kBodyMaxFields = {max(len(p["fields"]) for p in plans)};\n',
            array('BodyFieldPlan', 'kBodyFieldPlans', len(fields))]
    for atom, components, storage, table_id, canonical, selector, predicate, value, mask, rule, validation, group, slot, limit, copies, slots in fields:
        body.append(f'  {{ScalarKind::{atom}, {components}, BodyStorage::{storage}, {table_id}, {canonical}, {selector}, PredicateKind::{predicate}, {value}ull, {mask}ull, ValueRule::{rule}, {validation}, {group}, {slot}, {limit}, {copies}, {slots}}},\n')
    body += ['}};\n', array('BodyValidator', 'kBodyValidators', len(validators))]
    for kind, atom, low, high, first, count in validators:
        body.append(f'  {{{kind}, {atom}, {low}ull, {high}ull, {first}, {count}}},\n')
    body += ['}};\n', array('std::uint64_t', 'kBodyEnumValues', len(enum_values)),
             ','.join(f'{n}ull' for n in enum_values) + '\n', '}};\n', array('BodyLayoutRule', 'kBodyLayoutRules', len(rules))]
    for layout, kind, selector, other in rules:
        body.append(f'  {{{layout}, LayoutRuleKind::{kind}, {selector}, {other}}},\n')
    body.append('}};\n')
    # 固定头仍留作历史字节目录，但当前正文入口不得编码／接受被禁用的媒体布局。
    disabled = [p for p in plans if not p['entry'].get('body_enabled', True)]
    body += [array('std::uint16_t', 'kBodyDisabledLayouts', len(disabled)),
             ','.join(str(p['entry']['layout']) for p in disabled) + '\n', '}};\n']
    text_limits = {f['limit'] for p in plans for f in p['fields']
                   if f['source']['wire'].get('value_rule') == 'display-utf8'}
    require(len(text_limits) == 1, 'display-utf8', '显示文字须共用唯一字节预算')
    body.append(f'constexpr std::size_t kBodyDisplayTextBytes = {text_limits.pop()};\n')
    write(out / 'state_body_codec.inc', ''.join(body))
    summary = f'# 各域正文数字编解码计划\n\n布局版本 {compiler.version["version"]}；由同一布局源生成，不是运行测试报告。\n\n'
    maximum_fields = max(len(p['fields']) for p in plans)
    summary += f'共{len(plans)}种布局、{len(fields)}个字段计划、{sum(f[6] != "None" for f in fields)}个条件字段；最大根字段数{maximum_fields}。无负载的Host权威类型只登记空值布局，不补造状态。\n\n'
    summary += '条件、资源空间、整数／浮点校验、计数组和规范零负载在构建时转成数字规则；正文热路径不按名称解释。字段／引用的图关系、资源ready和原生应用成功仍由完整目标处理。\n\n'
    summary += '当前禁用正文布局：' + '、'.join(f'{p["entry"]["layout"]} {p["entry"]["type"]}' for p in disabled) + '。保留固定字节表不表示允许媒体下行。\n\n'
    summary += '| 域源 | 布局数（含继承／变体） | 字段数 |\n|---|---|---|\n'
    for domain, step in DOC_DOMAIN_STEPS.items():
        # 同一步有多份域源；直接按原始类型与展开的variant_base定位，不按目录重复计数。
        names = {e['type'] for e in read_json(compiler.source / domain)['types']}
        selected = [p for p in plans if p['entry']['type'] in names or
                    (p['entry'].get('variant_base') and any(compiler.expanded[n]['layout'] == p['entry']['variant_base'] for n in names))]
        summary += f'| {domain} | {len(selected)} | {sum(len(p["fields"]) for p in selected)} |\n'
    summary += '\n## 条件数字规则\n\n集合mask的第n位表示允许值n；item-config的mask表示配置type集合，operand表示配置id。\n\n'
    summary += '| 布局／字段 | 先行selector | 操作 | operand | 集合mask |\n|---|---|---|---|---|\n'
    for p in plans:
        for f in p['fields']:
            if not f['condition']:
                continue
            test = f['condition']['test']
            selector = next(v['index'] for v in p['fields'] if v['name'] == f['condition']['field'])
            mask = sum(1 << n for n in test.get('values', []))
            summary += f'| {p["entry"]["layout"]}/{f["index"]} {f["name"]} | {selector} | {test["kind"]} | {test.get("value", 0)} | {mask} |\n'
    write(docs / '09-wire-format/body-codec-plan.md', summary)


def emit_state_budget(compiler, plans, out, docs):
    """只按布局声明计算整数大小；不编码样本、不运行校验或游戏实验。"""
    policy = compiler.budget_policy
    require(policy is not None, 'budget_policy', '缺少预算唯一源')
    require(policy['world_body_zstd'] == 1 and policy['zstd_level'] == 3 and policy['zstd_window_bytes'] == 8*1024*1024,
            'budget_policy', '独立world/body压缩沿用level3与8MiB窗口保护')
    require(policy['host_bytes_per_second']*100 == policy['link_bytes_per_second']*policy['host_utilization_percent'] and
            policy['host_utilization_percent'] == 90, 'budget_policy', '本轮用户确定模组占10Mbps的90%')
    require(policy['host_bytes_per_second'] == 3 * policy['peer_bytes_per_second'] and
            sum(policy[k + '_bytes_per_second'] for k in ('control', 'world', 'body', 'reliable')) == policy['peer_bytes_per_second'],
            'budget_policy', '逐通道／逐Peer／总持续额度不一致')
    require(3*policy['peer_value_bytes']+policy['shared_value_bytes'] <= policy['pool_value_bytes'] and
            3*policy['peer_wire_bytes']+policy['shared_wire_bytes'] <= policy['pool_wire_bytes'] and
            policy['workspace_job_bytes']*policy['workspace_jobs'] <= policy['pool_workspace_bytes'],
            'budget_policy', '子额度超过既有进程共享池')
    constants = {(c['domain'], c['name']): c['value'] for c in compiler.protocol_constants}
    for name, key in [('total_mod_bytes_per_second','host_bytes_per_second'), ('peer_bytes_per_second','peer_bytes_per_second'),
                      ('world_bytes_per_second','world_bytes_per_second'), ('body_bytes_per_second','body_bytes_per_second'),
                      ('reliable_bytes_per_second','reliable_bytes_per_second')]:
        require(constants[('budget', name)] == policy[key], name, '公共常量与预算合同不一致')
    body = ['// 同源预算与静态大小；不表示运行性能已经通过。\nconstexpr StateBudgetPolicy kStateBudgetPolicy{\n']
    body.extend(f'  .{k}={v}ull,\n' for k, v in policy.items())
    body.append('};\n')
    write(out / 'state_budget.inc', ''.join(body))
    text = f'# 完整消息静态预算\n\n布局版本 {compiler.version["version"]}；普通构建生成，仅整数公式，不运行编解码样例或测试。\n\n'
    text += '| 合同 | 数值 |\n|---|---|\n'
    text += ''.join(f'| {k} | {v} |\n' for k, v in policy.items())
    by_name = {p['entry']['type']: p for p in plans}
    size = lambda name: by_name[name]['min']
    def network(body):
        logical = body + policy['envelope_bytes']
        fragments = (logical + policy['fragment_payload_bytes'] - 1) // policy['fragment_payload_bytes']
        return logical + fragments * (policy['fragment_header_bytes'] + policy['ip_udp_bytes'])
    text += '\n## 设计负载静态下界\n\n不压缩、不计合包收益，IP／UDP按48B，三端各发一份。只计所列成员／资源；不含其余必要显示／资源／控制／重传，不是完整场景通过结论。数量是设计输入，不是原版硬上限或观察样本。\n\n| 负载 | 正文B | 一端发出B | Hz | 一端B/s | 三端B/s |\n|---|---|---|---|---|---|\n'
    details = '\n| 负载 | 组成 |\n|---|---|\n'
    for work in compiler.budget_workloads:
        kind = work['kind']
        total = size('StateCommonHeader') + (size('StateTableHeader') if kind == 'resources' else
                size('StateFrameHeader') + (2 if kind == 'baseline' else 1) * size('StateSectionHeader'))
        members = 0
        for term in work['terms']:
            p = by_name[term['type']]
            if not term.get('result_envelope'):
                members += term['count']
            extra = term.get('extra_bytes', 0) + sum(size(e['type']) * e['count'] for e in term.get('extra_layouts', []))
            if kind == 'resources':
                total += term['count'] * (size('ResourceEntryHeader') + p['min'] + extra)
            elif term.get('result_envelope'):
                require(p['entry'].get('presentation_contract', {}).get('envelope', {}).get('mode') == 'group',
                        work['name'], '显示预算必须使用正式具名结果组')
                # 结果不套12B成员头；实际依赖与贡献在同一项extra_layouts显式收费。
                total += term['count'] * (size('StateResultGroupHeader') + p['min'] + extra)
            else:
                selected = [b for b in p['blocks'] if b['max'] and ('blocks' not in term or b['id'] in term['blocks'])]
                streams = {False: 0, True: 0}
                for b in selected:
                    is_body = b['body'] or b['player_body'] and term['type'] == 'EntityPlayer'
                    if kind == 'baseline' or is_body == (kind == 'body'):
                        streams[is_body] += b['min']
                for is_body, value in streams.items():
                    if value:
                        total += term['count'] * (size('StateRecordHeader') + value)
                total += term['count'] * extra
        cost = network(total)
        text += f'| {work["name"]} | {total} | {cost} | {work["hz"]} | {cost*work["hz"]} | {3*cost*work["hz"]} |\n'
        details += f'| {work["name"]} | ' + '; '.join(f'{t["type"]}×{t["count"]}' +
                ('（具名结果组）' if t.get('result_envelope') else '') +
                (f' blocks={t["blocks"]}' if 'blocks' in t else '') +
                ''.join(f' + {e["type"]}×{e["count"]}/项' for e in t.get('extra_layouts', [])) for t in work['terms']) + ' |\n'
        if kind == 'baseline':
            table = size('StateCommonHeader') + size('StateTableHeader') + members * size('StateMemberEntry')
            text += f'| 上述成员表{members}项（另计） | {table} | {network(table)} | 0 | 0 | 0 |\n'
    text += details
    text += '\n## 普通发送峰值的静态必要条件\n\n只按不压缩、无其他排队竞争计算；是可行性的上限，不是每帧固定配额，也不是延迟保证。\n\n| 序列 | 平均每次发出B | 可容纳正文B | 截止ms | 空桶可容纳正文B | 满通道突发桶可容纳正文B |\n|---|---|---|---|---|---|\n'
    def body_for_cost(allowed):
        lo, hi = 0, policy['ordinary_message_bytes'] - policy['envelope_bytes']
        while lo < hi:
            middle = (lo+hi+1)//2
            if network(middle) <= allowed:
                lo = middle
            else:
                hi = middle-1
        return lo
    for stream in ('world', 'body'):
        rate, hz, deadline = policy[stream+'_bytes_per_second'], policy[stream+'_hz'], policy[stream+'_send_ms']
        average = rate//hz
        text += f'| {stream} | {average} | {body_for_cost(average)} | {deadline} | {body_for_cost(rate*deadline//1000)} | {body_for_cost(rate*deadline//1000+policy[stream+"_burst_bytes"])} |\n'
    text += '\n## 各类型整记录声明界限\n\n仍不含公共帧／分段；上界只描述该类型record_limit，真正整帧必须再扣所有头／其他记录，条件下界不能视为完整合法样本。\n\n| 类型 | 布局 | 正文下界B | 正文声明上界B |\n|---|---|---|---|\n'
    for p in plans:
        text += f'| {p["entry"]["type"]} | {p["entry"]["layout"]} | {p["min"]} | {p["max"]} |\n'
    text += '\n## 容量边界单价\n\n| 对象 | 正文B | 不压缩一端发出B |\n|---|---|---|\n'
    for name, value in [('ContextReady', size('StateCommonHeader')+size('StateContextReady')),
                        ('单项AppliedAck', size('StateCommonHeader')+size('StateAppliedAckHeader')+size('StateAppliedAckEntry')),
                        ('joint AppliedAck', size('StateCommonHeader')+size('StateAppliedAckHeader')+2*size('StateAppliedAckEntry')),
                        ('Resync', size('StateCommonHeader')+size('StateResyncHeader')),
                        ('最大成员表', size('StateCommonHeader')+size('StateTableHeader')+65535*size('StateMemberEntry')),
                        ('4096条最大19B音频事件区域', 4096*(size('StateAudioEventHeader')+19)),
                        ('普通完整消息保护', policy['ordinary_message_bytes']-policy['envelope_bytes']),
                        ('可靠完整消息保护', policy['reliable_message_bytes']-policy['envelope_bytes'])]:
        text += f'| {name} | {value} | {network(value)} |\n'
    text += '\n## 逐块声明上下界\n\n下界可能要求不同条件同时缺席，不能直接宣称可构成一个合法完整对象。上界受记录／整消息／字段访问预算共同约束，不代表允许按此大小持续发送。\n\n| 类型 | 布局／块 | 序列 | 下界B | 声明上界B |\n|---|---|---|---|---|\n'
    for p in plans:
        for b in p['blocks']:
            stream = 'body' if b['body'] or b['player_body'] and p['entry']['type'] == 'EntityPlayer' else 'world'
            text += f'| {p["entry"]["type"]} | {p["entry"]["layout"]}/{b["id"]} | {stream} | {b["min"]} | {b["max"]} |\n'
    write(docs / '09-wire-format/budget-plan.md', text)


def emit_state_protocol(compiler, plans, out, docs):
    """完整消息只使用同源固定头／字段索引，不再手写字节偏移。"""
    names = ['StateCommonHeader', 'StateFrameHeader', 'StateSectionHeader', 'StateRecordHeader',
             'StateTableHeader', 'StateMemberEntry', 'StateResultGroupHeader', 'StateDependencyEntry', 'StateContributionEntry', 'StateAudioEventHeader',
             'StateContextReady', 'StateAppliedAckHeader', 'StateAppliedAckEntry', 'StateResyncHeader']
    body = ['// 完整二进制消息的同源固定头与接受保护；不安装游戏对象。\n']
    summary = f'# 完整消息与接受数字合同\n\n布局版本 {compiler.version["version"]}；同源生成，不是测试报告。\n\n| 头 | 布局 | 字节 |\n|---|---|---|\n'
    for name in names:
        p = next(p for p in plans if p['entry']['type'] == name)
        require(p['min'] == p['max'] and all(f['min_size'] == f['max_size'] and
                f['max_size'] in (1, 2, 4, 8) for f in p['fields']), name, '完整消息头须为定长整数')
        body += [f'constexpr std::uint16_t kW_{name} = {p["entry"]["layout"]};\n',
                 f'constexpr std::size_t kW_{name}Bytes = {p["min"]}u;\n',
                 f'enum class {name}Field : std::uint16_t {{' +
                 ','.join(f'{f["name"]}={f["index"]}' for f in p['fields']) + f', count={len(p["fields"])}' + '};\n']
        summary += f'| {name} | {p["entry"]["layout"]} | {p["min"]} |\n'
    for c in compiler.protocol_constants:
        if c['domain'] in ('acceptance_policy', 'presentation_budget', 'message'):
            body.append(f'constexpr std::uint32_t kW_{c["name"]} = {c["value"]}u;\n')
    summary += '\n接受、ACK丢失、不可变目标与污染恢复见[第五项合同](../../9-acceptance-recovery.md)。字节保护不替代10Mbps／首次准备预算。\n'
    write(out / 'state_protocol.inc', ''.join(body))
    write(docs / '09-wire-format/state-protocol-plan.md', summary)


def emit_resource_protocol(compiler, plans, out, docs):
    """资源矩阵、固定字段、图形允许集与保护预算同源生成，不把媒体内容引入协议。"""
    source = compiler.resource_protocol
    require(source is not None, 'resource_protocol', '缺少资源合同')
    by_name = {p['entry']['type']: p for p in plans}
    selected = ['StateCommonHeader', 'StateTableHeader', 'ResourceEntryHeader', 'SurfaceRecipeHeader',
                'SurfaceRecipeClear', 'SurfaceRecipeQuad', 'PresentationImageContribution', 'AudioVoiceParameters']
    selected += [p['entry']['type'] for p in plans if 425 <= p['entry']['layout'] <= 433]
    body = ['// 由资源唯一源生成；不包含原生地址、文件内容或GPU命令。\n']
    body.append(f'constexpr std::size_t kResourceMaxRootFields = {max(len(by_name[n]["fields"]) for n in selected)};\n')
    for name in selected:
        p = by_name[name]
        body.append(f'constexpr std::uint16_t kR_{name} = {p["entry"]["layout"]};\n')
        size = p['min'] if p['min'] == p['max'] else 0
        body.append(f'constexpr std::size_t kR_{name}Bytes = {size}u;\n')
        body.append(f'enum class {name}Field : std::uint16_t {{' +
                    ','.join(f'{f["name"]}={f["index"]}' for f in p['fields']) + f', count={len(p["fields"])}' + '};\n')
    entries = source['entries']
    require(len({(e['table'], e['kind']) for e in entries}) == len(entries) and
            {e['kind'] for e in entries} == set(range(10)) - {5}, 'resource_protocol', '资源矩阵缺项／重复／启用像素')
    body.append(array('ResourceDispatch', 'kResourceDispatch', len(entries)))
    summary = f'# 资源协议数字合同\n\n布局版本 {compiler.version["version"]}；同源生成，不是测试报告。\n\n'
    summary += '| table | kind | typed布局 | 编码 |\n|---|---|---|---|\n'
    for e in entries:
        p = by_name[e['type']]
        body.append(f'  {{{e["table"]}, {e["kind"]}, {p["entry"]["layout"]}, {str(e["recipe"]).lower()}}},\n')
        summary += f'| {e["table"]} | {e["kind"]} | {p["entry"]["layout"]} {e["type"]} | {"具名有序recipe" if e["recipe"] else "完整值"} |\n'
    body.append('}};\n')
    for key, width, typ in [('blend_profiles', 5, 'BlendProfile'), ('sampler_profiles', 4, 'SamplerProfile')]:
        profiles = source[key]
        require([p['id'] for p in profiles] == list(range(len(profiles))) and all(len(p['values']) == width for p in profiles), key, 'profile须稠密且宽度固定')
        require(all(all(v in (0, 1) for v in p['values']) if key == 'sampler_profiles' else
                    (0 <= p['values'][0] <= 4 and all(0 <= v <= 9 for v in p['values'][1:])) for p in profiles),
                key, 'profile含未登记的sampler／blend值')
        body.append(array(typ, 'kResource_' + key, len(profiles)))
        for p in profiles:
            body.append('  {{' + ','.join(map(str, p['values'])) + '}},\n')
        body.append('}};\n')
        summary += f'\n{key}：\n\n| 编号 | 固定值 |\n|---|---|\n'
        for p in profiles:
            summary += f'| {p["id"]} | {p["values"]} |\n'
    uses = source['graphics_uses']
    require([u['id'] for u in uses] == list(range(len(uses))), 'graphics_uses', '用途编号须稠密')
    body.append(array('GraphicsRule', 'kResourceGraphicsRules', len(uses)))
    for u in uses:
        mask = sum(1 << v for v in u['shaders'])
        body.append(f'  {{{mask}u, {u["blend_mask"]}u, {u["sampler_mask"]}u}},\n')
        body.append('')
    body.append('}};\n')
    for u in uses:
        body.append(f'constexpr std::uint8_t kRUse_{u["name"]} = {u["id"]};\n')
    for name, value in source['limits'].items():
        body.append(f'constexpr std::uint32_t kRLimit_{name} = {value}u;\n')
    summary += '\n固定shader用途：\n\n| 用途号 | 名称 | 内置shader |\n|---|---|---|\n'
    for u in uses:
        summary += f'| {u["id"]} | {u["name"]} | {u["shaders"]} |\n'
    summary += '\nkind5、305／306及Kind24正文禁用；recipe首项完整Clear，只有Clear／Quad，不含像素或任意参数字典。表元数据、闭包、实际本地物化分开，未ready不能ACK。\n'
    write(out / 'state_resource_protocol.inc', ''.join(body))
    write(docs / '09-wire-format/resource-protocol-plan.md', summary)


def emit_presentation_envelope(compiler, plans, out, docs):
    """同源生成固定头字段索引、用途／pass允许集；接收端不按自然语言选择adapter。"""
    constants = {(c['domain'], c['name']): c['value'] for c in compiler.protocol_constants}
    body = ['// 同源生成：显示封装数字合同，不能由报文扩展用途或pass。\n']
    headers = {'Group': 'StateResultGroupHeader', 'Dependency': 'StateDependencyEntry',
               'Contribution': 'StateContributionEntry', 'Event': 'StateAudioEventHeader'}
    for name, typ in headers.items():
        p = next(p for p in plans if p['entry']['type'] == typ)
        fields = p['fields']
        require(all(f['min_size'] == f['max_size'] and f['max_size'] in (1, 2, 4, 8) and
                    not f['nested_layout'] for f in fields), typ, '封装头须为定长整数字段')
        body += [f'constexpr std::uint16_t k{name}Layout = {p["entry"]["layout"]};\n',
                 f'constexpr std::size_t k{name}Bytes = {sum(f["max_size"] for f in fields)};\n',
                 f'enum class {name}Field : std::uint16_t {{' +
                 ','.join(f'{f["name"]}={f["index"]}' for f in fields) + f', count={len(fields)}' + '};\n']
    for domain in ('contribution_pass', 'presentation_budget'):
        for c in compiler.protocol_constants:
            if c['domain'] == domain:
                body.append(f'constexpr std::uint32_t kEnvelope_{c["name"]} = {c["value"]}u;\n')
    policies = [p for p in plans if p['entry'].get('presentation_contract')]
    body.append(array('ResultPolicy', 'kResultPolicies', len(policies)))
    summary = f'# 显示封装数字合同\n\n布局版本 {compiler.version["version"]}；由唯一源生成，不是测试结果。\n\n'
    summary += '| 用途／布局 | 序列 | 模式 | subject布局（0为world） | 可引用缓存 | D0依赖 | 绘制拓扑 | 允许配对mask | pass mask |\n|---|---|---|---|---|---|---|---|---|\n'
    for p in policies:
        c = p['entry']['presentation_contract']
        e = c['envelope']
        lid = p['entry']['layout']
        subject = 0 if e['subject_type'] == 'world' else compiler.expanded[e['subject_type']]['layout']
        subject_field = next((f['index'] for f in p['fields'] if f['name'] == e['subject_field']), 65535)
        require(not e['subject_field'] or (subject_field != 65535 and p['fields'][subject_field]['operation'] == 'Reference'),
                p['entry']['type'], '主体字段必须是真实成员引用')
        mask = sum(1 << constants[('contribution_pass', n)] for n in e['passes'])
        require((e['mode'] == 'event') == (not mask), p['entry']['type'], '事件不可作为普通结果组')
        body.append(f'  {{{lid}, {subject}, {subject_field}, {str(c["sequence"] == "body").lower()}, '
                    f'{str(e["mode"] == "event").lower()}, {str(e["cacheable"]).lower()}, '
                    f'{str(e["clock"]).lower()}, {str(e["topology"]).lower()}, {e["paired_streams"]}, {mask}u}},\n')
        summary += f'| {lid} {p["entry"]["type"]} | {c["sequence"]} | {e["mode"]} | {subject} | {e["cacheable"]} | {e["clock"]} | {e["topology"]} | {e["paired_streams"]} | {mask} |\n'
    body.append('}};\n')
    # 按真实嵌套布局保留具名子owner依赖，不凭stats等字段名猜测其类型。
    by_id = {p['entry']['layout']: p for p in plans}
    policy_ids = {p['entry']['layout'] for p in policies}
    requirements = []
    for root in policies:
        pending, seen, required = [root['entry']['layout']], set(), set()
        while pending:
            lid = pending.pop()
            if lid in seen:
                continue
            seen.add(lid)
            p = by_id[lid]
            c = p['entry'].get('presentation_contract')
            if c:
                required.update(compiler.expanded[n]['layout'] for n in c['dependency_types']
                                if compiler.expanded[n]['layout'] in policy_ids)
            pending.extend(f['nested_layout'] for f in p['fields'] if f['nested_layout'])
        requirements.extend((root['entry']['layout'], dep) for dep in sorted(required))
    body.append(array('ResultRequirement', 'kResultRequirements', len(requirements)))
    for root, dependency in requirements:
        body.append(f'  {{{root}, {dependency}}},\n')
    body.append('}};\n')
    summary += '\n聚合owner的具名结果依赖（包括实际嵌套owner）：\n\n| 用途 | 必需结果用途 |\n|---|---|\n'
    for root, dependency in requirements:
        summary += f'| {root} | {dependency} |\n'
    for name, typ, field in [('AudioKind', 'AudioWorldEvent', 'kind')]:
        p = next(p for p in plans if p['entry']['type'] == typ)
        f = next(f for f in p['fields'] if f['name'] == field)
        body.append(f'constexpr std::uint16_t k{name}Field = {f["index"]};\n')
    write(out / 'state_presentation_envelope.inc', ''.join(body))
    summary += '\n贡献类别编号见protocol-constants.md。Context仅定位；多次独立贡献保持原数组顺序，不按位置排序重写payload。所有资源、成员、拓扑和缓存就绪由第12／13步正式owner提供，结构解码不提交或ACK。\n'
    write(docs / '09-wire-format/presentation-envelope-plan.md', summary)


def emit_hud_local(compiler, plans, out):
    """普通映射边界及435数字路径同源生成，不在C++维护第二份布局表。"""
    policy = compiler.hud_local_policy
    require(policy is not None, 'hud-local', '缺少本地HUD映射合同')
    plan = next(p for p in plans if p['entry']['type'] == 'PlayerHUDLocalClock')
    fields = {f['name']: f for f in plan['fields']}
    names = ['player', 'binding_revision', 'update_ordinal', 'sample_game_frame', 'critical_health',
             'mantle_source_revision', 'drop_anchor_update', 'drop_held', 'drop_anchor_phase']
    require(set(names) == set(fields), 'hud-local', '时钟值接口与正式字段不一致')
    require(plan['min'] == plan['max'] == 41, 'hud-local', '时钟必须是41B定长值')
    body = '// 由gen_state_layout.py生成；只修改mod/binding/state/hud-local.json。\n'
    body += f'constexpr std::uint16_t kHudClockLayout={plan["entry"]["layout"]};\n'
    body += f'constexpr std::uint32_t kHudClockBytes={plan["max"]};\n'
    for key, target in [('active_last', 'ActiveLast'), ('trinket_last', 'TrinketLast'), ('golden_mask', 'GoldenMask')]:
        body += f'constexpr std::uint32_t kHud{target}={policy[key]};\n'
    for key, target in [('active_special', 'ActiveSpecial'), ('trinket_special', 'TrinketSpecial')]:
        require(all(v <= policy['active_last' if key == 'active_special' else 'trinket_last'] for v in policy[key]),
                'hud-local', '特殊映射越过已登记原版范围')
        values = ','.join(str(v) for v in policy[key])
        body += f'constexpr std::array<std::uint32_t,{len(policy[key])}> kHud{target}={{{values}}};\n'
    for target, values in [('Fields', [fields[n]['index'] for n in names]), ('Widths', [fields[n]['min_size'] for n in names])]:
        body += 'constexpr std::array<std::uint16_t,9> kHudClock' + target + '={' + ','.join(map(str, values)) + '};\n'
    write(out / 'state_hud_local.inc', body)


def emit(compiler, plans, out, docs):
    """四份 .inc 共享相同字段索引；代码表、协议偏移与文档在一次构建内同时更新。"""
    header = '// 由 gen_state_layout.py 生成；只修改 mod/binding/state/*.json。\n'
    emit_grid_description(compiler, plans, out)
    emit_room_description(compiler, plans, out)
    emit_body_codec(compiler, plans, out, docs)
    emit_presentation_envelope(compiler, plans, out, docs)
    emit_resource_protocol(compiler, plans, out, docs)
    emit_state_protocol(compiler, plans, out, docs)
    emit_state_budget(compiler, plans, out, docs)
    emit_hud_local(compiler, plans, out)
    count_fields = sum(len(p['fields']) for p in plans)
    wire = [header, f"constexpr std::uint16_t kStateLayoutVersion ={integer(compiler.version['version'])};\n", array('Layout', 'kStateLayouts', len(plans))]
    capture = [header, array('CopyRange', 'kStateCopyRanges', sum(len(p['copies']) for p in plans))]
    client = [header, array('Validator', 'kStateValidators', sum(len(p['entry']['validators']) for p in plans))]
    members = [header, array('Member', 'kStateMembers', sum(len(p['entry']['members']) for p in plans))]
    first_field, first_block, first_copy = 0, 0, 0
    for p in plans:
        e, lid = p['entry'], p['entry']['layout']
        public_header = compiler.doc_steps[e['type']] == '09-wire-format'
        parent = compiler.expanded[e['parent']]['layout'] if e.get('parent') else 0
        prefix = e.get('extent', 'object') == 'base_prefix'
        protocol = e.get('extent') == 'protocol'
        wire.append(f'  {{{lid}, {parent}, {quote(e["type"])}, {e["native_size"]}, {p["unknown"]}, {len(p["blocks"])}, {str(p["complete"]).lower()}, {str(prefix).lower()}, {str(protocol).lower()}, {quote(e["rtti"]["name"])}, {integer(e["rtti"]["rva"])}, {first_field}, {len(p["fields"])}, {first_block}, {first_copy}, {len(p["copies"])}}},\n')
        first_field += len(p['fields'])
        first_block += len(p['blocks'])
        first_copy += len(p['copies'])
        for c in sorted(p['copies'], key=lambda c: (c['block'], c['offset'])):
            capture.append(f'  {{{lid}, {c["block"]}, {c["offset"]}, {c["wire"]}u, {c["size"]}, {quote(c["read_requirement"])}}},\n')
        for key, v in e['validators'].items():
            field = next(f for f in p['fields'] if f['name'] == key)
            values = ','.join(str(n) for n in v.get('values', []))
            # Validator上下界为double；大u64上界用浮点字面量，避免C++十进制整数常量溢出。
            bounds = [repr(float(v.get(k, 0))) if abs(v.get(k, 0)) > 2**53 else str(v.get(k, 0)) for k in ('min', 'max')]
            client.append(f'  {{{lid}, {field["index"]}, Validation::{v["kind"].title()}, {quote(v["scalar"])}, {bounds[0]}, {bounds[1]}, {quote(values)}}},\n')
        for m in e['members']:
            members.append(f'  {{{lid}, {quote(m["id"])}, {quote(m["disposition"])}, {quote(",".join(m.get("fields", [])))}, {quote(m["reason"])}}},\n')
        native_cells = [f'0x{r["offset"]:X} {r["type"]} {r["name"]}【{r["category"]}】{r["semantics"]}（{r["size"]}B；{r["status"]}）' for r in p['native_ranges']]
        body = f'# {e["type"]}\n\n由唯一布局源生成。布局编号 {lid}；布局版本 {integer(compiler.version["version"])}；原生大小 0x{e["native_size"]:X}；未知 {p["unknown"]}B；完整：{"是" if p["complete"] else "否"}。\n\n'
        if prefix:
            body += '**范围仅为已核前缀，不是动态对象分配大小；具体对象仍需完整布局。**\n\n'
        if e.get('notes'):
            body += e['notes'] + '\n\n'
        if not e.get('body_enabled', True):
            body += '**当前正文编解码禁用此布局；下面的固定字节表仅保留定义事实，不表示可接受媒体正文。**\n\n'
        contract = e.get('presentation_contract')
        if contract:
            body += '具名显示结果合同（接线声明，不表示运行实现）：\n\n'
            body += table([f'{key}：{", ".join(value) if isinstance(value, list) else value}'
                           for key, value in contract.items()]) + '\n'
        if e.get('record_limit'):
            body += f'记录编码预算≤{e["record_limit"]}B；body组≤{e.get("body_limit", 0)}B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。\n\n'
        if e.get('variant_values'):
            body += f'变体选择字段 `{e["variant_selector"]}`，取值：{e["variant_values"]}。\n\n'
        body += '原生字节归属：\n\n' + table(native_cells or ['纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移']) + '\n'
        if public_header:
            body += f'公共封装布局（总计 {p["min"]}—{p["max"]}B，不再附加12B状态记录头）：\n\n'
        else:
            body += f'状态二进制布局（总计 {p["min"]}—{p["max"]}B，不含第 9 步记录头）：\n\n'
        if not p['blocks']:
            body += table(['无发送字节：该类型没有可复制状态']) + '\n'
        for b in p['blocks']:
            sequence = 'world；Player 为 body' if b['player_body'] else 'body' if b['body'] else 'world'
            if public_header:
                body += f'固定封装（{b["min"]}—{b["max"]}B；块表中的world仅为生成占位，不限制承运通道或所属序列）：\n\n'
            else:
                body += f'块 {b["id"]}：{b["name"]}（{sequence}，{b["min"]}—{b["max"]}B）：\n\n'
            cells = []
            for f in p['fields']:
                if f['block'] != b['id']:
                    continue
                offset = '顺序变长偏移' if f['wire_offset'] == DYNAMIC else f'+0x{f["wire_offset"]:X}'
                typename = f['type']
                if f['operation'] == 'Container':
                    typename = f'{typename}[{f["fixed_count"]}]（固定槽，无count）' if f['fixed_count'] else f'u{8*f["count_bytes"]} count + count×{typename}（上限{f["limit"]}）'
                    if f['nested_block'] != 255:
                        typename += f' 子块{f["nested_block"]}'
                if f['operation'] == 'Nested':
                    typename += '（见该类型状态表）'
                    if f['nested_block'] != 255:
                        typename += f' 子块{f["nested_block"]}'
                    if f['storage'] == 'shared-owner':
                        typename = ('u8 present + ' if f['nested_block'] in {0, 255} else '') + typename + '（间接owner；缺失时最短全零）'
                if f['companion']:
                    typename += '（伴随源，无根对象偏移）'
                if f['synthetic']:
                    typename += '（C/S自有元数据，无原生偏移）'
                if f['operation'] == 'Choice':
                    typename = 'u8 present_mask + 按槽序(u16 view_layout + view值)；允许：' + '、'.join(f['choices'])
                if f['condition']:
                    typename += f'（仅{f["condition"]["predicate"]}({f["condition"]["field"]})成立时存在）'
                if f['mask']:
                    typename += f'（只保留0x{f["mask"]:X}位）'
                cells.append(f'{offset} {typename} {f["name"]}：{f["semantics"]}')
            body += table(cells or ['无发送字节：显式预留块']) + '\n'
        body += '证据：\n\n' + '\n'.join(f'- `{r["name"]}`：{r["evidence"]}' for r in p['native_ranges']) + '\n'
        if e['companions']:
            body += '\n伴随来源（不属于上述原生对象字节）：\n\n' + '\n'.join(f'- `{c["name"]}` → `{c["type"]}`：{c["semantics"]}；{c["evidence"]}（{c["status"]}）' for c in e['companions']) + '\n'
        if p['count_groups']:
            body += '\n容器计数约束（同一容器各投影count相同，按容器去重求和）：\n\n' + '\n'.join(
                f'- `{g["name"]}`：' + ' + '.join(g['fields']) + f' ≤ {g["limit"]}；{g["evidence"]}' for g in p['count_groups']) + '\n'
        if e['catalog_rows']:
            body += '\n逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：\n\n'
            body += '| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |\n|---|---|---|---|---|\n'
            for row in e['catalog_rows']:
                cells = [row['catalog'] + ':' + str(row['ordinal']), row['id'],
                         json.dumps(row['parameters'], ensure_ascii=False) + ' / ' + json.dumps(row['returns'], ensure_ascii=False),
                         row['disposition'] + ' → ' + ','.join(row.get('fields', [])) + '；' + row['route'], row['reason']]
                body += '| ' + ' | '.join(v.replace('|', '\\|').replace('\n', ' ') for v in cells) + ' |\n'
        if e['supplementary_rows']:
            body += '\n补充成员逐行回映射：\n\n| 来源行 | ID | 处置／来源 | 原始定位与依据 |\n|---|---|---|---|\n'
            for row in e['supplementary_rows']:
                cells = [row['source'] + ':' + str(row['ordinal']), row['id'], row['disposition'] + ' → ' + ','.join(row['fields']), row['detail'] + '；' + row['reason']]
                body += '| ' + ' | '.join(v.replace('|', '\\|').replace('\n', ' ') for v in cells) + ' |\n'
        if e.get('catalog_aliases'):
            body += f'\n另一公开目录的逐行交叉链接：{len({(a["catalog"], a["ordinal"]) for a in e["catalog_aliases"]})}个来源行，共{len(e["catalog_aliases"])}条目标行链接；见同源`CatalogAliases()`生成表，重复签名不去重。\n'
        write(docs / doc_path(compiler, e['type']), body)
    # 布局号到表下标的直接索引：布局号稀疏但上限很小，按线上布局号查询时不必线性扫描。
    slots = [65535] * (max(p['entry']['layout'] for p in plans) + 1)
    for position, p in enumerate(plans):
        slots[p['entry']['layout']] = position
    wire += ['}};\n', array('std::uint16_t', 'kStateLayoutSlots', len(slots)), ','.join(map(str, slots)) + '\n', '}};\n',
             array('Block', 'kStateBlocks', sum(len(p['blocks']) for p in plans))]
    for p in plans:
        for b in p['blocks']:
            wire.append(f'  {{{p["entry"]["layout"]}, {b["id"]}, {str(b["body"]).lower()}, {str(b["player_body"]).lower()}, {quote(b["name"])}, {b["min"]}, {b["max"]}}},\n')
    wire += ['}};\n', array('Field', 'kStateFields', count_fields)]
    for p in plans:
        for f in p['fields']:
            source = 'Companion' if f['companion'] else 'Synthetic' if f['synthetic'] else 'Object'
            wire.append(f'  {{{f["layout"]}, {f["index"]}, {f["block"]}, {f["native_offset"]}u, {f["native_size"]}, {f["wire_offset"]}u, {f["min_size"]}, {f["max_size"]}, Operation::{f["operation"]}, {quote(f["name"])}, {quote(f["type"])}, {quote(f["host"])}, {quote(f["target"])}, {quote(f["storage"])}, {quote(f["count_source"])}, {f["nested_layout"]}, {f["nested_block"]}, {f["limit"]}, {f["stride"]}, FieldSource::{source}, {str(f["shape"]).lower()}, {quote(f["read_requirement"])}, {f["fixed_count"]}, {f["mask"]}u, {f["count_bytes"]}}},\n')
    wire += ['}};\n', array('NativeRange', 'kStateNativeRanges', sum(len(p['native_ranges']) for p in plans))]
    for p in plans:
        for r in p['native_ranges']:
            wire.append(f'  {{{p["entry"]["layout"]}, {r["offset"]}, {r["size"]}, Category::{CATEGORIES[r["category"]]}, {quote(r["name"])}, {quote(r["type"])}, {quote(r["semantics"])}, {quote(r["evidence"])}, {quote(r["status"])}}},\n')
    wire += ['}};\n', array('Variant', 'kStateVariants', sum(len(p['entry'].get('variant_values', [])) for p in plans))]
    for p in plans:
        e = p['entry']
        if e.get('variant_values'):
            selector = next(r for r in e['ranges'] if r['name'] == e['variant_selector'])
            for value in e['variant_values']:
                wire.append(f'  {{{e["variant_base"]}, {e["layout"]}, {selector["offset"]}, {value}}},\n')
    wire += ['}};\n', array('ContextField', 'kStateContexts', sum(len(p['entry']['contexts']) for p in plans))]
    for p in plans:
        for name, context in p['entry']['contexts'].items():
            wire.append(f'  {{{p["entry"]["layout"]}, {quote(name)}, {quote(context["type"])}, {quote(context["semantics"])}, {quote(context["evidence"])}}},\n')
    wire += ['}};\n', array('FieldCondition', 'kStateConditions', sum(bool(f['condition']) for p in plans for f in p['fields']))]
    for p in plans:
        for f in p['fields']:
            if f['condition']:
                selector = next(v['index'] for v in p['fields'] if v['name'] == f['condition']['field'])
                wire.append(f'  {{{f["layout"]}, {f["index"]}, {selector}, {quote(f["condition"]["predicate"])}}},\n')
    wire += ['}};\n', array('LayoutChoice', 'kStateChoices', sum(len(f['choices']) for p in plans for f in p['fields']))]
    for p in plans:
        for f in p['fields']:
            for child in f['choices']:
                wire.append(f'  {{{f["layout"]}, {f["index"]}, {compiler.expanded[child]["layout"]}}},\n')
    wire.append('}};\n')
    wire.append(array('LayoutBudget', 'kStateBudgets', sum('record_limit' in p['entry'] for p in plans)))
    for p in plans:
        e = p['entry']
        if 'record_limit' in e:
            wire.append(f'  {{{e["layout"]}, {e["record_limit"]}u, {e.get("body_limit", 0)}u}},\n')
    wire.append('}};\n')
    contracts = [p for p in plans if p['entry'].get('presentation_contract')]
    wire.append(array('PresentationContract', 'kStatePresentationContracts', len(contracts)))
    for p in contracts:
        c = p['entry']['presentation_contract']
        values = [c[k] for k in ('owner', 'subject', 'producer', 'adapter', 'dependencies',
                                 'atomic_scope', 'validity', 'invalidation', 'contribution_identity', 'source_gap')]
        wire.append(f'  {{{p["entry"]["layout"]}, {str(c["sequence"] == "body").lower()}, ' +
                    ', '.join(quote(v) for v in values) + '},\n')
    wire.append('}};\n')
    wire.append(array('PresentationDependency', 'kStatePresentationDependencies',
                      sum(len(p['entry']['presentation_contract']['dependency_types']) for p in contracts)))
    for p in contracts:
        for dependency in p['entry']['presentation_contract']['dependency_types']:
            wire.append(f'  {{{p["entry"]["layout"]}, {compiler.expanded[dependency]["layout"]}}},\n')
    wire.append('}};\n')
    wire.append(array('ProtocolConstant', 'kStateProtocolConstants', len(compiler.protocol_constants)))
    for c in compiler.protocol_constants:
        wire.append(f'  {{{quote(c["domain"])}, {quote(c["name"])}, {c["value"]}ull, {quote(c["semantics"])}}},\n')
    wire.append('}};\n')
    capture.append('}};\n')
    client += ['}};\n// 处理器名只在注册时绑定，热路径不比较成员名。\n', array('ClientField', 'kStateClientFields', count_fields)]
    for p in plans:
        for f in p['fields']:
            client.append(f'  {{{f["layout"]}, {f["index"]}, Operation::{f["operation"]}, {quote(f["client"])}}},\n')
    client += ['}};\n', array('CountConstraint', 'kStateCountConstraints', sum(len(p['count_constraints']) for p in plans))]
    for p in plans:
        for c in p['count_constraints']:
            client.append(f'  {{{c["layout"]}, {c["group"]}, {c["slot"]}, {c["field"]}, {c["limit"]}}},\n')
    client.append('}};\n')
    members += ['}};\n', array('CatalogRow', 'kStateCatalogRows', sum(len(p['entry']['catalog_rows']) for p in plans))]
    for p in plans:
        for row in p['entry']['catalog_rows']:
            strings = [row['catalog'], row['id'], row['owner'], row['parameters'] or '', row['returns'] or '']
            tail = [row['disposition'], ','.join(row.get('fields', [])), row['route'], row['reason']]
            members.append(f'  {{{p["entry"]["layout"]}, {row["ordinal"]}, ' + ', '.join(quote(v) for v in strings) + ', ' +
                           ', '.join(str(row[k] is None).lower() for k in ('parameters', 'returns')) + ', ' + ', '.join(quote(v) for v in tail) + '},\n')
    members.append('}};\n')
    members.append(array('CatalogAlias', 'kStateCatalogAliases', sum(len(p['entry'].get('catalog_aliases', [])) for p in plans)))
    for p in plans:
        for a in p['entry'].get('catalog_aliases', []):
            members.append(f'  {{{p["entry"]["layout"]}, {a["ordinal"]}, {a["target_ordinal"]}, {quote(a["catalog"])}, {quote(a["target_catalog"])}}},\n')
    members.append('}};\n')
    members.append(array('SupplementaryMember', 'kStateSupplementaryMembers', sum(len(p['entry']['supplementary_rows']) for p in plans)))
    for p in plans:
        for row in p['entry']['supplementary_rows']:
            values = [row['source'], row['id'], row['disposition'], ','.join(row['fields']), row['route'], row['detail'], row['reason']]
            members.append(f'  {{{p["entry"]["layout"]}, {row["ordinal"]}, ' + ', '.join(quote(v) for v in values) + '},\n')
    members.append('}};\n')
    for filename, content in [('state_wire.inc', wire), ('state_capture.inc', capture), ('state_client.inc', client), ('state_members.inc', members)]:
        write(out / filename, ''.join(content))
    index = '# 按总计划步骤归类的生成布局索引\n\n'
    index += '上级：[二进制重构总计划](../README.md#7-步骤)。按总计划步骤号阅读，步骤号与布局编号分别管理；各步骤内按布局编号排列。\n\n'
    index += '布局内容只修改 `mod/binding/state/`；目录与索引由 `mod/tools/gen_state_layout.py` 同源生成。未知和未核声明照实保留，不能据本表宣称原生同步已可用。\n\n'
    index += '以布局源所属步骤归类，变体与附属类型随所属域；共享类型只保存一份。第7步已登记的显示／资源结构仍随第7步，后续协议与运行职责见对应计划，不以目录位置宣称已实现。\n\n'
    index += '第0—1步负责起点与生成机制，没有单独类型表；第9步登记公共报文固定头与常量，第10—15步的运行接线和验收仍见总计划。\n\n'
    index += '| 总计划步骤 | 布局表数 | 公开目录表数 |\n|---|---|---|\n'
    step_indexes = {}
    catalog_counts = {step: 0 for step in DOC_STEPS}
    for catalog, domain in compiler.catalog_outputs:
        step = catalog_doc_step(catalog, domain)
        catalog_counts[step] += 1
    for step, (title, plan_file) in DOC_STEPS.items():
        selected = [p for p in plans if compiler.doc_steps[p['entry']['type']] == step]
        index += f'| [{title}]({step}/README.md) | {len(selected)} | {catalog_counts[step]} |\n'
        body = f'# {title}：生成布局\n\n[返回布局总索引](../README.md) · [对应总计划步骤](../../{plan_file})\n\n'
        body += '按布局编号排列；共享类型通过引用复用，不重复生成。未知字节与完整标志只描述布局事实，不表示采集、应用或验收已完成。\n\n'
        body += '| 类型 | 布局编号 | 原生字节 | 未知字节 | 完整 |\n|---|---|---|---|---|\n'
        body += ''.join(f'| [{p["entry"]["type"]}]({p["entry"]["type"]}.md) | {p["entry"]["layout"]} | {p["entry"]["native_size"]} | {p["unknown"]} | {"是" if p["complete"] else "否"} |\n' for p in selected)
        step_indexes[step] = body
    # 汇总表和C++逐行表共用正式布局源；保留重复ID的公开名及原始签名以便人工阅读。
    for catalog, domain in compiler.catalog_outputs:
        filename = Path(catalog).stem + ('-' + domain if domain else '') + '-members.md'
        step = catalog_doc_step(catalog, domain)
        rows = sorted((item for item in compiler.catalog_rows[catalog]
                       if domain is None or compiler.catalog_sources[catalog][item[1]['ordinal'] - 1].get('domain') == domain),
                      key=lambda item: item[1]['ordinal'])
        body = f'# {Path(catalog).stem}：逐行公开成员回映射\n\n'
        body += f'布局版本 {compiler.version["version"]}；来源 `{catalog}`，共 {len(rows)} 行。由正式布局源生成；行号从1开始，重复ID不去重。\n\n'
        if domain:
            body += f'本表只完成原目录的 `{domain}` 域，其它域不在本次完整声明中。\n\n'
        body += '这里只声明目录处置与正式布局依赖，不代表运行reader／writer、绘制adapter或第15步验收已经完成。\n\n'
        counts = {kind: sum(row['disposition'] == kind for _, row in rows)
                  for kind in ('field', 'derived', 'authority', 'not_applicable')}
        body += '处置统计：' + '；'.join(f'{kind}={count}' for kind, count in counts.items()) + '。\n\n'
        body += '| 来源行 | ID／公开名 | parameters / returns（JSON原值） | 正式布局／字段 | 处置／职责 | 逐行依据 |\n|---|---|---|---|---|---|\n'
        for entry, row in rows:
            source = compiler.catalog_sources[catalog][row['ordinal'] - 1]
            cells = [str(row['ordinal']), row['id'] + ' / ' + source.get('member', row['id']),
                     json.dumps(row['parameters'], ensure_ascii=False) + ' / ' + json.dumps(row['returns'], ensure_ascii=False),
                     f'[{entry["type"]} ({entry["layout"]})]({posixpath.relpath(doc_path(compiler, entry["type"]), step)})：' + ','.join(row['fields']),
                     row['disposition'] + '；' + row['route'], row['reason']]
            body += '| ' + ' | '.join(v.replace('|', '\\|').replace('\n', ' ') for v in cells) + ' |\n'
        write(docs / step / filename, body)
        step_indexes[step] += f'\n公开目录：[{Path(catalog).stem} {domain + "域" if domain else ""}全部 {len(rows)} 行]({filename})。\n'
    if compiler.protocol_constants:
        body = f'# 公共协议常量\n\n布局版本 {compiler.version["version"]}；由唯一布局源生成。数值登记不表示运行编解码已经接通。\n\n'
        body += '| 分类 | 名称 | 数值 | 语义 |\n|---|---|---|---|\n'
        for c in compiler.protocol_constants:
            body += '| ' + ' | '.join(str(c[k]).replace('|', '\\|') for k in ('domain', 'name', 'value', 'semantics')) + ' |\n'
        write(docs / '09-wire-format/protocol-constants.md', body)
        step_indexes['09-wire-format'] += '\n[公共报文编号、枚举与预算](protocol-constants.md)。\n'
    step_indexes['09-wire-format'] += '\n[各域正文数字编解码计划](body-codec-plan.md)。\n'
    step_indexes['09-wire-format'] += '\n[显示封装数字合同](presentation-envelope-plan.md)。\n'
    step_indexes['09-wire-format'] += '\n[资源协议数字合同](resource-protocol-plan.md)。\n'
    step_indexes['09-wire-format'] += '\n[完整消息与接受数字合同](state-protocol-plan.md)。\n'
    step_indexes['09-wire-format'] += '\n[完整消息静态预算](budget-plan.md)。\n'
    for step, body in step_indexes.items():
        write(docs / step / 'README.md', body)
    index += '\n旧证据路径仅保留 [EntitySaveState](EntitySaveState.md) 和 [GridEntityDesc](GridEntityDesc.md) 两个跳转页，正式表见对应步骤目录。\n'
    for name, step in DOC_LEGACY_PATHS.items():
        write(docs / (name + '.md'), f'# {name}：旧证据路径\n\n正式布局已归入 [{name}]({step}/{name}.md)。\n\n本页仅保留布局源中既有证据路径；原生字节归属、状态布局及未知项以正式表为准。\n')
    for old_name, name in DOC_RENAMED_TYPES.items():
        if name in compiler.doc_steps:
            step = compiler.doc_steps[name]
            write(docs / step / (old_name + '.md'), f'# {old_name}：旧布局名\n\n版本61起布局374收窄为 [{name}]({name}.md)，仅承接独立Boss versus；普通切房surface结果已取消。原版本定义见提交历史，不保留两套当前必发表达。\n')
    write(docs / 'README.md', index)


def catalog_doc_step(catalog, domain):
    """公开目录随总计划所属步骤归档，不根据布局编号或生成先后排序。"""
    if Path(catalog).stem == 'grid-catalog':
        return '06-grid'
    if Path(catalog).stem == 'run-catalog':
        return '08-run-presentation'
    if Path(catalog).stem == 'stream-catalog' and domain in {'room', 'level', 'room_save', 'exploration'}:
        return '07-room-level'
    if Path(catalog).stem == 'stream-catalog' and domain in {'presentation', 'random'}:
        return '08-run-presentation'
    raise ValueError(f'{catalog}:{domain}: 尚未登记公开目录的总计划步骤')


def table(cells):
    cells = [s.replace('|', '\\|').replace('\n', ' ') for s in cells]
    return '| ' + ' | '.join(cells) + ' |\n|' + '|'.join('---' for _ in cells) + '|\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'mod/binding/state')
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--docs', type=Path, default=ROOT / 'docs/design/binary-state/layouts')
    args = parser.parse_args()
    compiler = Compiler(args.source)
    plans = compiler.compile()
    emit(compiler, plans, args.out, args.docs)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, TypeError) as error:
        print(f'布局生成失败: {error}', file=sys.stderr)
        sys.exit(1)
