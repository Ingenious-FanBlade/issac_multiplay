"""从唯一 JSON 布局源生成二进制复制、解码、回映射和单行布局表。

这是构建生成器：拒绝不成立的声明，不读取游戏，不执行测试，不计算哈希。
已明确的未知区可生成，但 complete 始终为 false；后续步骤不能把草稿当完整布局。
"""
import argparse
import copy
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CATEGORIES = dict(zip(
    ['状态', '换算', '引用', '资源', '容器', '本地', '派生', '权威', '填充', '未知'],
    ['State', 'Convert', 'Reference', 'Resource', 'Container', 'Local', 'Derived', 'Authority', 'Padding', 'Unknown']))
OPERATIONS = {'copy': 'Copy', 'convert': 'Convert', 'reference': 'Reference',
              'resource': 'Resource', 'container': 'Container', 'nested': 'Nested'}
SCALARS = {'u8': 1, 'bool': 1, 'i8': 1, 'u16': 2, 'i16': 2,
           'u32': 4, 'i32': 4, 'f32': 4, 'u64': 8, 'i64': 8, 'f64': 8, 'bytes': 1}
STATUS = {'已核', 'SDK声明', '推断', '未知'}
DYNAMIC = 0xFFFFFFFF


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
        require(0 < integer(self.version['version']) <= DYNAMIC, 'version', '版本必须是 u32 正整数')
        require(isinstance(self.version.get('domains'), list) and self.version['domains'] and len(self.version['domains']) == len(set(self.version['domains'])), 'version', '域清单为空或重复')
        self.types = {}
        schema = read_json(source / 'schema.json')
        for domain in self.version['domains']:
            require(re.fullmatch(r'[a-z][a-z0-9_-]*\.json', domain), domain, '非法域文件名')
            data = read_json(source / domain)
            schema_check(data, schema, schema['$defs'], domain)
            for entry in data['types']:
                name = entry['type']
                require(re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', name), name, '非法类型名')
                require(name not in self.types, name, '重复类型')
                self.types[name] = copy.deepcopy(entry)
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
        require(0 < entry['native_size'] <= 0x1000000, name, '非法原生大小')
        entry['layout'] = integer(entry['layout'])
        require(0 < entry['layout'] < 65535 and entry['layout'] not in self.ids, name, '布局编号重复或越界')
        self.ids.add(entry['layout'])
        entry.setdefault('members', [])
        entry.setdefault('public_owners', [])
        entry.setdefault('validators', {})
        entry.setdefault('conversions', {})
        entry.setdefault('contexts', {})
        entry.setdefault('companions', [])
        require(isinstance(entry.get('rtti'), dict) and 'name' in entry['rtti'] and 'rva' in entry['rtti'], name, '缺少 RTTI 声明（值类型可为空）')
        require(0 <= integer(entry['rtti']['rva']) <= DYNAMIC, name, 'RTTI RVA 超过 u32')
        if entry.get('parent'):
            parent = self.expand(entry['parent'])
            require(parent['native_size'] < entry['native_size'], name, '父类大小必须小于子类')
            entry['ranges'] = copy.deepcopy(parent['ranges']) + entry['ranges']
            parent_ids = {b['id'] for b in parent['blocks']}
            require(not parent_ids.intersection(b['id'] for b in entry['blocks']), name, '子类覆盖父类块号')
            require(not entry['blocks'] or min(b['id'] for b in entry['blocks']) > max(parent_ids, default=-1), name, '子类块必须排在父类之后')
            entry['blocks'] = copy.deepcopy(parent['blocks']) + entry['blocks']
            entry['validators'] = {**parent['validators'], **entry['validators']}
            entry['conversions'] = {**parent['conversions'], **entry['conversions']}
            entry['contexts'] = {**parent['contexts'], **entry['contexts']}
            entry['members'] = copy.deepcopy(parent['members']) + entry['members']
            entry['companions'] = copy.deepcopy(parent['companions']) + entry['companions']
        self.validate_ranges(entry)
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
            require(item['name'] not in names, where, '重复字段名称')
            names.add(item['name'])
            transmitted = item['category'] in {'状态', '换算', '引用', '资源', '容器'}
            require(transmitted == ('wire' in item), where, '状态字段必须且仅能声明一份 wire 规则')
            if not transmitted:
                continue
            wire = item['wire']
            kind = wire.get('kind')
            if kind == 'nested' and self.types.get(wire.get('type'), {}).get('pure_value'):
                wire['kind'] = kind = 'copy'  # 纯值成员内联，允许与父块相邻状态合并。
            require(kind in OPERATIONS, where, '缺少合法传输操作')
            require('projections' not in wire or kind == 'nested', where, '只有复杂嵌套类型可以按子块投影')
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
                require(integer(self.types[wire['type']]['native_size']) == item['size'], where, '嵌套对象大小不一致')
            elif kind in {'reference', 'resource', 'convert'}:
                require(nonempty(wire.get('host')) and nonempty(wire.get('client')), where, '缺少具名双端处理器')
                require(scalar_size(wire.get('type', '')) is not None, where, '转换结果必须是定长值')
                if kind == 'reference':
                    require(nonempty(wire.get('target')) and wire['type'] == 'u16', where, '引用缺少目标空间或不是 u16')
                if kind == 'resource':
                    require(nonempty(wire.get('target')), where, '资源缺少目标表')
                if kind == 'convert':
                    require(wire['host'] in entry['conversions'] and wire['client'] in entry['conversions'], where, '换算处理器没有语义声明')
            elif kind == 'container':
                require(nonempty(wire.get('element')) and 0 < integer(wire.get('limit', 0)) < 65536, where, '容器没有元素类型或 u16 上限')
                require(integer(wire.get('stride', 0)) > 0 and nonempty(wire.get('host')) and nonempty(wire.get('client')), where, '容器缺少原生步长/读取/重建处理器')
                element = wire['element']
                width = scalar_size(element)
                require(width is not None or element in self.types, where, '未知容器元素类型')
                require(integer(wire['stride']) == (width if width is not None else integer(self.types[element]['native_size'])), where, '容器元素原生步长不一致')
                require(wire.get('storage') in {'vector', 'string', 'array'}, where, '容器必须指定 vector/string/array')
                if wire['storage'] == 'vector':
                    require(item['size'] == 12, where, 'J273 vector 头必须为 12 字节')
                elif wire['storage'] == 'string':
                    require(item['size'] == 24 and element == 'u8', where, 'J273 string 必须为 24 字节字节串')
                else:
                    count = wire.get('count_field')
                    require((nonempty(count) and count in {r['name'] for r in entry['ranges']}) or nonempty(wire.get('count_from')), where, '数组缺少计数字段或具名 owner 计数来源')
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

    def plan(self, name):
        if name in self.plans:
            return self.plans[name]
        require(name not in self.plan_stack, name, '嵌套类型循环')
        self.plan_stack.add(name)
        entry = self.expanded[name]
        ranges = {r['name']: r for r in entry['ranges']}
        # 伴随对象有自己的原生字节目录；这里只建立 owner 链，绝不伪造根对象偏移。
        for companion in entry['companions']:
            key, child = companion['name'], companion['type']
            require(key not in ranges and child in self.expanded and child != name, name + '.' + key, '伴随源名称冲突或类型不存在')
            ranges[key] = dict(companion, offset=DYNAMIC, size=0, category='状态',
                               companion=True, wire=dict(kind='nested', type=child,
                               host=companion['host'], client=companion['client']))
        # ANM2 等嵌套对象的外观、动画和图层进入父类不同块；原生字节归属仍只声明一次。
        projected = {}
        for key, item in ranges.items():
            wire = item.get('wire', {})
            if 'projections' in wire:
                nested = self.plan(wire['type'])
                require(set(wire['projections']) == {b['id'] for b in nested['blocks']}, name + '.' + key, '子块投影必须恰好覆盖全部嵌套块')
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
                else:
                    element = wire['element']
                    if element in self.expanded:
                        nested = self.plan(element)
                        width = nested['max']
                    else:
                        width = scalar_size(element)
                    minimum, maximum = 2, 2 + integer(wire['limit']) * width
                offset = DYNAMIC if dynamic else low
                field = dict(layout=entry['layout'], index=len(fields), block=number,
                             native_offset=item['offset'], native_size=item['size'], wire_offset=offset,
                             min_size=minimum, max_size=maximum, operation=OPERATIONS[kind],
                             name=key, type=wire.get('type', wire.get('element')), host=wire.get('host', ''),
                             client=wire.get('client', ''), target=wire.get('target', ''),
                             storage=wire.get('storage', ''), count_source=wire.get('count_field', wire.get('count_from', '')),
                             nested_layout=nested['entry']['layout'] if nested else 0,
                             nested_block=wire.get('block', 255),
                             limit=integer(wire.get('limit', 0)), stride=integer(wire.get('stride', 0)),
                             companion=item.get('companion', False),
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
        native_ranges = self.native_ranges(name)
        unknown = sum(r['size'] for r in native_ranges if r['category'] == '未知')
        complete = entry.get('extent', 'object') == 'object' and not unknown and all(r['status'] == '已核' for r in entry['ranges'] + entry['companions']) and all(self.plans[f['type']]['complete'] for f in fields if f['nested_layout'])
        result = dict(entry=entry, fields=fields, blocks=blocks, copies=copies, unknown=unknown,
                      native_ranges=native_ranges, complete=complete, min=sum(b['min'] for b in blocks), max=sum(b['max'] for b in blocks))
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
            nested = wire.get('type')
            if wire.get('kind') in {'copy', 'nested'} and nested in self.expanded:
                for child in self.native_ranges(nested):
                    value = copy.deepcopy(child)
                    value['offset'] += item['offset']
                    value['name'] = item['name'] + '.' + child['name']
                    value['semantics'] = item['semantics'] + '：' + child['semantics']
                    value['evidence'] = item['evidence'] + '；嵌套字段：' + child['evidence']
                    value['status'] = max((item['status'], child['status']), key=confidence.__getitem__)
                    result.append(value)
            else:
                result.append(item)
        self.native_cache[name] = result
        return result

    def catalogs(self):
        """仅要求已声明接管的 owner；后续域未入源前明确不宣称全域目录覆盖。"""
        expected = {}
        for rel in self.version['catalogs']:
            path = (ROOT / rel).resolve()
            require(path.is_relative_to(ROOT), rel, '目录输入越出仓库')
            data = read_json(path)
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

    def compile(self):
        for name in self.types:
            self.expand(name)
        self.make_variants()
        for name in self.expanded:
            self.plan(name)
        self.catalogs()
        return sorted(self.plans.values(), key=lambda p: p['entry']['layout'])


def emit(compiler, plans, out, docs):
    """四份 .inc 共享相同字段索引；代码表、协议偏移与文档在一次构建内同时更新。"""
    header = '// 由 gen_state_layout.py 生成；只修改 mod/binding/state/*.json。\n'
    def array(typ, name, count):
        return f'constexpr std::array<{typ}, {count}> {name} = {{{{\n'
    count_fields = sum(len(p['fields']) for p in plans)
    wire = [header, f"constexpr std::uint32_t kStateLayoutVersion = {integer(compiler.version['version'])};\n", array('Layout', 'kStateLayouts', len(plans))]
    capture = [header, array('CopyRange', 'kStateCopyRanges', sum(len(p['copies']) for p in plans))]
    client = [header, array('Validator', 'kStateValidators', sum(len(p['entry']['validators']) for p in plans))]
    members = [header, array('Member', 'kStateMembers', sum(len(p['entry']['members']) for p in plans))]
    first_field, first_block, first_copy = 0, 0, 0
    for p in plans:
        e, lid = p['entry'], p['entry']['layout']
        parent = compiler.expanded[e['parent']]['layout'] if e.get('parent') else 0
        prefix = e.get('extent', 'object') == 'base_prefix'
        wire.append(f'  {{{lid}, {parent}, {quote(e["type"])}, {e["native_size"]}, {p["unknown"]}, {len(p["blocks"])}, {str(p["complete"]).lower()}, {str(prefix).lower()}, {quote(e["rtti"]["name"])}, {integer(e["rtti"]["rva"])}, {first_field}, {len(p["fields"])}, {first_block}, {first_copy}, {len(p["copies"])}}},\n')
        first_field += len(p['fields'])
        first_block += len(p['blocks'])
        first_copy += len(p['copies'])
        for c in sorted(p['copies'], key=lambda c: (c['block'], c['offset'])):
            capture.append(f'  {{{lid}, {c["block"]}, {c["offset"]}, {c["wire"]}u, {c["size"]}, {quote(c["read_requirement"])}}},\n')
        for key, v in e['validators'].items():
            field = next(f for f in p['fields'] if f['name'] == key)
            values = ','.join(str(n) for n in v.get('values', []))
            client.append(f'  {{{lid}, {field["index"]}, Validation::{v["kind"].title()}, {quote(v["scalar"])}, {v.get("min", 0)}, {v.get("max", 0)}, {quote(values)}}},\n')
        for m in e['members']:
            members.append(f'  {{{lid}, {quote(m["id"])}, {quote(m["disposition"])}, {quote(",".join(m.get("fields", [])))}, {quote(m["reason"])}}},\n')
        native_cells = [f'0x{r["offset"]:X} {r["type"]} {r["name"]}【{r["category"]}】{r["semantics"]}（{r["size"]}B；{r["status"]}）' for r in p['native_ranges']]
        body = f'# {e["type"]}\n\n由唯一布局源生成。布局编号 {lid}；布局版本 {integer(compiler.version["version"])}；原生大小 0x{e["native_size"]:X}；未知 {p["unknown"]}B；完整：{"是" if p["complete"] else "否"}。\n\n'
        if prefix:
            body += '**范围仅为已核基类前缀，不是动态对象分配大小；具体子类仍需完整布局。**\n\n'
        if e.get('notes'):
            body += e['notes'] + '\n\n'
        if e.get('variant_values'):
            body += f'变体选择字段 `{e["variant_selector"]}`，取值：{e["variant_values"]}。\n\n'
        body += '原生字节归属：\n\n' + table(native_cells) + '\n'
        body += f'状态二进制布局（总计 {p["min"]}—{p["max"]}B，不含第 9 步记录头）：\n\n'
        if not p['blocks']:
            body += table(['无发送字节：该类型没有可复制状态']) + '\n'
        for b in p['blocks']:
            sequence = 'world；Player 为 body' if b['player_body'] else 'body' if b['body'] else 'world'
            body += f'块 {b["id"]}：{b["name"]}（{sequence}，{b["min"]}—{b["max"]}B）：\n\n'
            cells = []
            for f in p['fields']:
                if f['block'] != b['id']:
                    continue
                offset = '顺序变长偏移' if f['wire_offset'] == DYNAMIC else f'+0x{f["wire_offset"]:X}'
                typename = f['type']
                if f['operation'] == 'Container':
                    typename = f'u16 count + count×{typename}（上限{f["limit"]}）'
                if f['operation'] == 'Nested':
                    typename += '（见该类型状态表）'
                    if f['nested_block'] != 255:
                        typename += f' 子块{f["nested_block"]}'
                if f['companion']:
                    typename += '（伴随源，无根对象偏移）'
                cells.append(f'{offset} {typename} {f["name"]}：{f["semantics"]}')
            body += table(cells or ['无发送字节：显式预留块']) + '\n'
        body += '证据：\n\n' + '\n'.join(f'- `{r["name"]}`：{r["evidence"]}' for r in p['native_ranges']) + '\n'
        if e['companions']:
            body += '\n伴随来源（不属于上述原生对象字节）：\n\n' + '\n'.join(f'- `{c["name"]}` → `{c["type"]}`：{c["semantics"]}；{c["evidence"]}（{c["status"]}）' for c in e['companions']) + '\n'
        write(docs / (e['type'] + '.md'), body)
    wire += ['}};\n', array('Block', 'kStateBlocks', sum(len(p['blocks']) for p in plans))]
    for p in plans:
        for b in p['blocks']:
            wire.append(f'  {{{p["entry"]["layout"]}, {b["id"]}, {str(b["body"]).lower()}, {str(b["player_body"]).lower()}, {quote(b["name"])}, {b["min"]}, {b["max"]}}},\n')
    wire += ['}};\n', array('Field', 'kStateFields', count_fields)]
    for p in plans:
        for f in p['fields']:
            source = 'Companion' if f['companion'] else 'Object'
            wire.append(f'  {{{f["layout"]}, {f["index"]}, {f["block"]}, {f["native_offset"]}u, {f["native_size"]}, {f["wire_offset"]}u, {f["min_size"]}, {f["max_size"]}, Operation::{f["operation"]}, {quote(f["name"])}, {quote(f["type"])}, {quote(f["host"])}, {quote(f["target"])}, {quote(f["storage"])}, {quote(f["count_source"])}, {f["nested_layout"]}, {f["nested_block"]}, {f["limit"]}, {f["stride"]}, FieldSource::{source}, {str(f["shape"]).lower()}, {quote(f["read_requirement"])}}},\n')
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
    wire.append('}};\n')
    capture.append('}};\n')
    client += ['}};\n// 处理器名只在注册时绑定，热路径不比较成员名。\n', array('ClientField', 'kStateClientFields', count_fields)]
    for p in plans:
        for f in p['fields']:
            client.append(f'  {{{f["layout"]}, {f["index"]}, Operation::{f["operation"]}, {quote(f["client"])}}},\n')
    client.append('}};\n')
    members.append('}};\n')
    for filename, content in [('state_wire.inc', wire), ('state_capture.inc', capture), ('state_client.inc', client), ('state_members.inc', members)]:
        write(out / filename, ''.join(content))
    index = '# 生成布局索引\n\n只修改 `mod/binding/state/`。未知和未核声明照实保留，不能据本表宣称原生同步已可用。\n\n'
    index += '| 类型 | 布局编号 | 原生字节 | 未知字节 | 完整 |\n|---|---|---|---|---|\n'
    index += ''.join(f'| [{p["entry"]["type"]}]({p["entry"]["type"]}.md) | {p["entry"]["layout"]} | {p["entry"]["native_size"]} | {p["unknown"]} | {"是" if p["complete"] else "否"} |\n' for p in plans)
    write(docs / 'README.md', index)


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
