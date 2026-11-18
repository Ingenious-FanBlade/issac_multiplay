"""把需求筛选后的字段降成平铺主区、原生读写段和具名补丁。"""

from collections import defaultdict, deque
from dataclasses import dataclass, field
from pathlib import Path
import json
import re


NO_OFFSET = 0xFFFFFFFF
NO_INDEX = 0xFFFF
SCALAR_SIZE = {
    'u8': 1, 'i8': 1, 'bool': 1, 'bytes': 1, 'u16': 2, 'i16': 2,
    'u32': 4, 'i32': 4, 'f32': 4, 'u64': 8, 'i64': 8, 'f64': 8,
}
OPERATION = {'copy': 'Copy', 'reference': 'Reference', 'resource': 'Resource', 'convert': 'Convert'}
PREDICATE = {'equal': 'Equal', 'not-equal': 'NotEqual', 'any-bits': 'AnyBits',
             'one-of': 'OneOf', 'item-config': 'ItemConfig'}
VALUE_RULE = {'non-null': 'NonNull', 'nullable-water': 'NullableWater', 'entity-counter': 'EntityCounter',
              'countdown': 'Countdown', 'display-utf8': 'DisplayUtf8'}
CONTAINER = {
    'vector': ('Vector', 12), 'deque': ('Deque', 20), 'string': ('String', 24),
    'c-string': ('CString', 8), 'ordered-map': ('Tree', 8), 'set': ('Tree', 8),
    'unordered': ('Unordered', 32), 'aggregate': ('Aggregate', 8), 'array': ('Array', 8),
    'owner': ('Owner', 4),
}
# 用原生表示播种镜像的头都由具名补丁覆盖整个槽。间接数组／特殊投影直接由 handler 产出。
SEED_HEADERS = {'Vector', 'Deque', 'String', 'Tree', 'Unordered', 'LayerArray'}


def require(condition, where, message):
    if not condition:
        raise ValueError(f'{where}: {message}')


def number(value):
    return int(value, 0) if isinstance(value, str) else value


def scalar_width(scalar):
    base, _, count = scalar.partition('[')
    return SCALAR_SIZE[base] * (int(count[:-1]) if count else 1)


@dataclass
class Leaf:
    """一次展开得到一个线上字段；序号与紧凑镜像偏移在展开完成后分配。"""

    path: str
    scalar: str
    stream: str
    kind: str
    header: str
    width: int
    native: int
    native_size: int
    owner: int
    owner_layout: int
    source: str
    scope: int
    read_requirement: str
    host: str
    client: str
    target: str
    count: str
    count_offset: int
    stride: int
    limit: int
    max_bytes: int
    mask: int
    child: int
    choices: list
    shape: bool
    validator: dict
    value_rule: str
    frame_relative: bool
    conditions: list = field(default_factory=list)
    offset: int = 0
    ordinal: int = 0
    group: int = 0
    seeded: bool = False


class FlatCompiler:
    """同一个递归展开器处理根、内嵌数组和间接元素；分组按具体 owner 实例绑定。"""

    def __init__(self, compiler, name):
        self.compiler = compiler
        self.root = compiler.expanded[name]
        self.name = name
        self.player = name == 'EntityPlayer'
        self.leaves = []
        self.scopes = []
        self.dependencies = []
        self.count_groups = []
        self.zero_constraints = []
        self.children = set()

    def items(self, entry):
        native = [r for r in entry['ranges'] if 'wire' in r]
        synthetic = [
            dict(item, offset=NO_OFFSET, size=0, category='换算', source='Synthetic',
                 wire=item.get('wire', dict(kind='convert', type=item['type'],
                                            host=item['host'], client=item['client'])))
            for item in entry['synthetic_fields']
        ]
        companions = [
            dict(item, offset=NO_OFFSET, size=0, category='容器', source='Companion',
                 wire=dict(kind='nested', type=item['type'], storage='shared-owner',
                           host=item['host'], client=item['client'], when=item.get('when')))
            for item in entry['companions']
        ]
        return native + synthetic + companions

    def visit(self, name, base=0, prefix='', stream=None, scope=NO_INDEX, inherited_read='', inherited_shape=False):
        entry = self.compiler.expanded[name]
        paths = {}
        handlers = defaultdict(list)
        native = {r['name']: r for r in entry['ranges']}
        items = self.items(entry)
        first = len(self.leaves)
        for item in items:
            key = item['name']
            path = prefix + key
            wire = item['wire']
            where = self.name + '.' + path
            rule = item.get('sequence')
            require(rule in {'world', 'body', 'world-or-player-body'}, where, '字段缺少唯一流声明')
            flow = stream or ('body' if rule == 'world-or-player-body' and self.player else
                              'world' if rule == 'world-or-player-body' else rule)
            offset = NO_OFFSET if item['offset'] == NO_OFFSET else base + item['offset']
            read_requirement = item.get('read_requires', inherited_read)
            shape = inherited_shape or item.get('shape', False)
            kind, storage = wire['kind'], wire.get('storage')
            child = wire.get('type') if kind in ('nested', 'copy') else wire.get('element')
            count = wire.get('fixed_count')
            by_value = (kind == 'nested' and storage != 'shared-owner' or
                        kind == 'copy' and child in self.compiler.expanded or bool(count))
            start = len(self.leaves)
            if by_value:
                require(offset != NO_OFFSET and child in self.compiler.expanded, where, '内嵌字段缺少原生范围或布局')
                stride = self.compiler.expanded[child]['native_size']
                require(stride * (count or 1) == item['size'], where, '内嵌字段范围与元素布局不符')
                inline_scope = scope
                # 固定数组由原生偏移展开。具名 nested 才有不同于名义地址的解析入口。
                if kind == 'nested' and wire.get('host'):
                    inline_scope = len(self.scopes)
                    require(inline_scope < NO_INDEX, where, '具名内嵌来源过多')
                    self.scopes.append(dict(parent=scope, native=offset, size=item['size'],
                                            layout=self.compiler.expanded[child]['layout'],
                                            fields=[], host=wire['host'], client=wire.get('client', ''),
                                            read=read_requirement))
                for index in range(count or 1):
                    location = path if not count else f'{path}[{index}]'
                    self.visit(child, offset + index * stride, location + '.', flow, inline_scope, read_requirement, shape)
                if inline_scope != scope:
                    self.scopes[inline_scope]['fields'] = list(range(start, len(self.leaves)))
                validator = {k: v for k, v in entry['validators'].get(key, {}).items() if k != 'fields'}
                if validator:
                    for leaf in self.leaves[start:]:
                        require(leaf.scalar.split('[', 1)[0] == validator['scalar'] and
                                (not leaf.validator or leaf.validator == validator), where,
                                '内嵌值校验必须与展开后的标量校验等价')
                        leaf.validator = validator
            else:
                self.leaves.append(self.make_leaf(entry, item, wire, path, flow, offset, base,
                                                  scope, read_requirement, shape, native))
            produced = list(range(start, len(self.leaves)))
            require(produced, where, '需求字段没有可编码叶或间接槽')
            paths[key] = produced
            if not prefix:
                for i in produced:
                    self.declarations[i] = item['_declared_in']
            for handler in {wire.get('host'), wire.get('client')} - {None, ''}:
                handlers[handler].extend(produced)

        def inputs(keys, where):
            fields = []
            for key in keys:
                if key in paths:
                    fields.extend(paths[key])
                else:
                    # 允许声明内嵌叶路径，同时拒绝旧 # 投影名和缺失的依赖。
                    match = [i for i in range(first, len(self.leaves)) if self.leaves[i].path == prefix + key]
                    require(match, where, '写入依赖缺少字段 ' + key)
                    fields.extend(match)
            return fields

        for item in items:
            key, wire = item['name'], item['wire']
            where = self.name + '.' + prefix + key
            condition = wire.get('when')
            if condition:
                selector = inputs([condition['field']], where)
                require(len(selector) == 1 and self.leaves[selector[0]].kind in
                        {'Copy', 'Convert', 'Reference', 'Resource'}, where, '条件需要单个定长选择器')
                for index in paths[key]:
                    self.leaves[index].conditions.insert(0, (selector[0], condition['test']))
                self.dependencies.append(selector + paths[key])
            count_input = wire.get('count_field') or wire.get('count_from', '').partition('.')[0]
            if count_input in paths:
                self.dependencies.append(paths[key] + paths[count_input])
            validation_inputs = entry['validators'].get(key, {}).get('fields')
            if validation_inputs:
                self.dependencies.append(paths[key] + inputs(validation_inputs, where))

        where = self.name + '.' + prefix + name
        for group in entry['count_groups']:
            fields = inputs(group['fields'], where)
            require(all(self.leaves[i].kind == 'Container' for i in fields), where, '计数组必须使用容器槽')
            self.count_groups.append((fields, group['limit']))
            self.dependencies.append(fields)
        body_rule = entry.get('body_rule')
        if body_rule:
            if body_rule == 'empty-active-zero':
                selector = 'item_id'
                empty = 0
                targets = [key for key in paths if key != selector]
            else:
                require(body_rule == 'null-costume-layer-zero', where, '未登记的规范零值规则')
                selector, empty, targets = 'index', NO_INDEX, ['layer_id']
            source = inputs([selector], where)
            fields = inputs(targets, where)
            require(len(source) == 1 and fields and all(self.leaves[i].kind in {'Copy', 'Convert'} for i in fields),
                    where, '规范零值规则须指向定长标量')
            self.zero_constraints.append((source[0], empty, fields))
            self.dependencies.append(source + fields)
        for handler, conversion in entry['conversions'].items():
            if isinstance(conversion, dict) and handler in handlers:
                handlers[handler] += inputs(conversion['fields'], where + '.' + handler)
        for group in entry['write_groups']:
            fields = inputs(group['fields'], where)
            if 'handler' in group:
                handler = group['handler']
                require(handler in handlers, where, '写入组引用未使用的处理器 ' + handler)
                handlers[handler] += fields
            else:
                self.dependencies.append(fields)
        self.dependencies.extend(handlers.values())

    def make_leaf(self, entry, item, wire, path, flow, offset, owner, scope,
                  read_requirement, shape, native):
        where = self.name + '.' + path
        kind, size = wire['kind'], item['size']
        operation, header = OPERATION.get(kind), 'None'
        storage = wire.get('flat_storage', wire.get('storage'))
        child = wire.get('type') if kind == 'nested' else wire.get('element')
        scalar = wire.get('type', wire.get('element', ''))
        choices = wire.get('choices', [])
        count_offset = NO_OFFSET
        count_name = wire.get('count_field', wire.get('count_from', ''))
        if wire.get('count_field'):
            count = native.get(count_name)
            require(count and count['category'] not in {'未知', '填充'}, where, '缺少已核容器计数槽')
            count_offset = count['offset']
        if kind == 'choice':
            # 每个 Weapon 槽保存一个 u32，四周只保留本地原生 owner。
            operation, header, width = 'Owner', 'Weapon', wire['limit'] * 4
        elif kind == 'nested' or storage == 'owner':
            require(storage in {'shared-owner', 'owner'}, where, '间接子对象缺少 owner 类型')
            operation, header, width = 'Owner', 'Owner', 4
        elif kind == 'container':
            require(storage in CONTAINER, where, '未实现的平铺容器头：' + str(storage))
            operation = 'Container'
            header, width = CONTAINER[storage]
            if wire.get('inline_count'):
                header = 'InlinePrefix'
            elif count_name == 'layer_count' and size == 4:
                require(count_offset == item['offset'] + size and native[count_name]['size'] == 4 and
                        'wire' not in native[count_name], where, '图层数组必须拥有邻接的派生计数字')
                header = 'LayerArray'
                size += 4
            if header in SEED_HEADERS:
                require(size == width, where, '原生头与平铺模板宽度不符')
        else:
            require(operation is not None, where, '未实现的平铺字段操作')
            width = scalar_width(scalar)
            if kind in {'reference', 'resource'}:
                require(re.fullmatch(r'u16(?:\[[1-9][0-9]*\])?', scalar), where, '索引须来自 u16 资源或引用声明')
                width = max(size, width * 2)
                scalar = scalar.replace('u16', 'u32', 1)
        if child in self.compiler.expanded:
            self.children.add(child)
        self.children.update(choices)
        validator = entry['validators'].get(item['name'], {})
        return Leaf(path, scalar, flow, operation, header, width, offset, size, owner,
                    entry['layout'], item.get('source', 'Object'), scope, read_requirement,
                    wire.get('host', ''), wire.get('client', ''), wire.get('target', ''), count_name,
                    count_offset, number(wire.get('stride', 0)), wire.get('limit', 0),
                    number(wire.get('max_bytes', 0)), number(wire.get('mask', 0)),
                    self.compiler.expanded[child]['layout'] if child in self.compiler.expanded else 0,
                    [self.compiler.expanded[c]['layout'] for c in choices], shape, validator,
                    VALUE_RULE.get(wire.get('value_rule'), 'None'),
                    wire.get('client') == 'RemainingFrames' or wire.get('value_rule') == 'countdown')

    def assign_groups(self, ordered):
        """先合并显式和推导依赖，再以首个字段序号分配组号。"""
        root = list(range(len(self.leaves)))

        def find(index):
            while root[index] != index:
                root[index] = root[root[index]]
                index = root[index]
            return index

        for fields in self.dependencies:
            for index in fields[1:]:
                root[find(index)] = find(fields[0])
        groups = {}
        for index in ordered:
            groups.setdefault(find(index), []).append(index)
        result = list(groups.values())
        for group, fields in enumerate(result):
            for index in fields:
                self.leaves[index].group = group
        for fields in self.dependencies:
            require(len({self.leaves[index].group for index in fields}) == 1, self.name,
                    '同一个具名 owner、条件或计数规则跨越多个写入组')
        return result

    def place(self):
        """先排可读原生位置和合成槽，再分配父类优先的字段序号。"""
        plan = self.compiler.plans[self.name]
        ledger = plan['native_ranges']
        native_category = [None] * self.root['native_size']
        for row in ledger:
            native_category[row['offset']:row['offset'] + row['size']] = [row['category']] * row['size']
        layout_order = sorted(range(len(self.leaves)), key=lambda i: (self.leaves[i].native, i))
        copies, zeros = [], []
        end, native_end = 0, 0
        for index in layout_order:
            leaf = self.leaves[index]
            where = self.name + '.' + leaf.path
            raw = leaf.kind == 'Copy'
            normalize_seed = leaf.kind in {'Reference', 'Resource'} or leaf.header in SEED_HEADERS
            if leaf.native != NO_OFFSET:
                require(native_end <= leaf.native <= leaf.native + leaf.native_size <= len(native_category),
                        where, '平铺原生槽重叠或越界')
                native_end = leaf.native + leaf.native_size
            if raw:
                require(leaf.native != NO_OFFSET and leaf.native_size == leaf.width and
                        all(category == '状态' for category in native_category[leaf.native:native_end]),
                        where, '原样写段包含非状态字节')
            # 声明不完整的复杂头与条件槽由具名路径单独读取，不参加根 memcpy。
            if (raw or normalize_seed) and leaf.native != NO_OFFSET and leaf.scope == NO_INDEX and not leaf.conditions:
                categories = set(native_category[leaf.native:native_end])
                expected = {'Copy': {'状态'}, 'Reference': {'引用'}, 'Resource': {'资源'},
                            'Container': {'容器', '派生'}}[leaf.kind]
                require(categories <= expected and leaf.native_size <= leaf.width, where,
                        '复制范围包含未覆盖的原生类别或未知字节')
                leaf.seeded = True
                if copies:
                    copy = copies[-1]
                    copy_end = copy[0] + copy[2]
                    gap = leaf.native - copy_end
                    if (copy[1] + copy[2] == end and copy[3] == leaf.read_requirement and gap >= 0 and
                            all(category == '填充' for category in native_category[copy_end:leaf.native])):
                        if gap:
                            zeros.append((end, gap))
                        end += gap
                        copies[-1] = (copy[0], copy[1], copy[2] + gap + leaf.native_size, copy[3])
                    else:
                        copies.append((leaf.native, end, leaf.native_size, leaf.read_requirement))
                else:
                    copies.append((leaf.native, end, leaf.native_size, leaf.read_requirement))
                leaf.offset = end
                end += leaf.width
                continue
            leaf.offset = end
            end += leaf.width
        padded = (end + 31) & ~31
        if padded != end:
            zeros.append((end, padded - end))
        require(0 < padded < NO_OFFSET, self.name, '主区大小超界')
        return padded, copies, zeros

    def build(self):
        self.declarations = {}
        self.visit(self.name)
        require(0 < len(self.leaves) < NO_INDEX and
                len({f.path for f in self.leaves}) == len(self.leaves), self.name, '字段数越界或路径重复')
        mirror_size, copies, zeros = self.place()
        # 继承自父类型的合成槽亦必须排在子类型字段之前。
        ancestry = []
        name = self.root['_declaration_name']
        while name:
            ancestry.append(name)
            name = self.compiler.expanded[name].get('parent')
        order = {name: level for level, name in enumerate(reversed(ancestry))}
        ordered = sorted(range(len(self.leaves)), key=lambda i: (order[self.declarations[i]], self.leaves[i].offset))
        byte_fields = [NO_INDEX] * mirror_size
        for ordinal, index in enumerate(ordered):
            leaf = self.leaves[index]
            leaf.ordinal = ordinal
            last = leaf.offset + leaf.width
            require(all(b == NO_INDEX for b in byte_fields[leaf.offset:last]), self.name + '.' + leaf.path, '字段镜像范围重叠')
            byte_fields[leaf.offset:last] = [ordinal] * leaf.width
        groups = self.assign_groups(ordered)
        writes = []
        for indices in groups:
            segments = []
            for leaf in sorted((self.leaves[i] for i in indices), key=lambda leaf: leaf.offset):
                if leaf.kind != 'Copy' or leaf.conditions:
                    continue
                # 具名来源的 raw 叶由 scope writer 处理，不能猜测目的地址。
                if leaf.scope != NO_INDEX:
                    continue
                if segments and (segments[-1][0] + segments[-1][2], segments[-1][1] + segments[-1][2]) == (leaf.offset, leaf.native):
                    previous = segments.pop()
                    segments.append((previous[0], previous[1], previous[2] + leaf.width))
                else:
                    segments.append((leaf.offset, leaf.native, leaf.width))
            writes.append(segments)
        return dict(mirror_size=mirror_size, copies=copies, zeros=zeros, ordered=ordered, groups=groups,
                    writes=writes, byte_fields=byte_fields)


def emit_flat_layouts(compiler, plans, out, codec_scalar, scalar_bits):
    """同次构建生成唯一的数字索引，两端通过相同 layout id 访问。"""
    pending = deque(p['entry']['type'] for p in plans if p['entry'].get('flat') and p['fields'])
    complete = set()
    layouts = []
    output = ['// 由 gen_flat_layout.py 生成，勿编辑。\n']
    while pending:
        name = pending.popleft()
        if name in complete:
            continue
        complete.add(name)
        flat = FlatCompiler(compiler, name)
        data = flat.build()
        lid = flat.root['layout']
        layouts.append((lid, 'kFlat' + str(lid)))
        pending.extend(sorted(flat.children))
        emit_type(output, flat, data, 'kFlat' + str(lid), codec_scalar, scalar_bits)
    slots = dict(layouts)
    emit_array(output, 'const FlatLayout *', 'kFlatSlots',
               ('&' + slots[i] if i in slots else 'nullptr' for i in range(max(slots, default=-1) + 1)), compact=True)
    path = Path(out) / 'state_flat.inc'
    content = ''.join(output)
    if not path.exists() or path.read_text(encoding='utf-8') != content:
        path.write_text(content, encoding='utf-8', newline='\n')


def emit_array(output, cpp_type, name, rows, compact=False):
    rows = list(rows)
    if not rows:
        return '{}'
    output.append(f'constexpr std::array<{cpp_type}, {len(rows)}> {name} = {{{{\n')
    if compact:
        for index in range(0, len(rows), 32):
            output.append('  ' + ', '.join(rows[index:index + 32]) + ',\n')
    else:
        output.extend('  ' + row + ',\n' for row in rows)
    output.append('}};\n')
    return name


def braces(*values):
    return '{' + ', '.join(map(str, values)) + '}'


def quote(value):
    return json.dumps(value, ensure_ascii=False)


def emit_type(output, flat, data, symbol, codec_scalar, scalar_bits):
    """每个布局的数组存于只读区；按相同序号引用字段及写入组。"""
    leaves = flat.leaves

    def indexes(suffix, indices):
        return emit_array(output, 'std::uint16_t', symbol + suffix,
                          (str(leaves[i].ordinal) for i in indices), compact=True)

    fields, patches = [], []
    for index in data['ordered']:
        leaf = leaves[index]
        ordinal = leaf.ordinal
        v = leaf.validator
        scalar, components = codec_scalar(leaf.scalar)
        require(components <= NO_INDEX, flat.name + '.' + leaf.path, '标量分量数超出 u16')
        validator_scalar = v.get('scalar', leaf.scalar.split('[', 1)[0])
        if leaf.kind in {'Reference', 'Resource'} and validator_scalar == 'u16':
            validator_scalar = 'u32'
        values = emit_array(output, 'std::uint64_t', f'{symbol}Enum{ordinal}',
                            (f'{scalar_bits(value, validator_scalar)}ull' for value in v.get('values', [])), compact=True)
        bounds = [f'{scalar_bits(v[key], validator_scalar)}ull' if key in v else '0'
                  for key in ('min', 'max')]
        validation = braces('Validation::' + v.get('kind', 'none').title(),
                            'ScalarKind::' + (codec_scalar(validator_scalar)[0] if v else 'None'),
                            *bounds, values)
        operation = 'FlatPatchKind::' + leaf.kind
        fields.append(braces(quote(leaf.path), leaf.offset, leaf.width, leaf.group, components,
                             'ScalarKind::' + scalar, 'FlatStream::' + leaf.stream.title(), operation,
                             str(leaf.shape).lower(), 'FlatValueRule::' + leaf.value_rule, validation))
        if leaf.kind == 'Copy' and leaf.seeded:
            continue
        condition_rows = []
        for selector, test in leaf.conditions:
            require(not leaves[selector].conditions, flat.name + '.' + leaf.path, '条件选择器依赖另一条件')
            mask = sum(1 << n for n in test.get('values', []))
            condition_rows.append(braces(leaves[selector].ordinal, 'FlatPredicate::' + PREDICATE[test['kind']],
                                         str(test.get('value', 0)) + 'ull', str(mask) + 'ull'))
        conditions = emit_array(output, 'FlatCondition', f'{symbol}Conditions{ordinal}', condition_rows)
        choices = emit_array(output, 'std::uint16_t', f'{symbol}Choices{ordinal}', map(str, leaf.choices), compact=True)
        patches.append((leaf.offset, braces(leaf.offset, leaf.width, leaf.native, leaf.native_size, leaf.owner,
                                           leaf.count_offset, leaf.stride, leaf.limit, leaf.max_bytes, leaf.mask,
                                           ordinal, leaf.scope, leaf.child, leaf.owner_layout, operation,
                                           'FlatHeader::' + leaf.header, 'FieldSource::' + leaf.source,
                                           str(leaf.frame_relative).lower(),
                                           quote(leaf.host), quote(leaf.client), quote(leaf.target), quote(leaf.count),
                                           quote(leaf.read_requirement), conditions, choices)))
    for offset, width in data['zeros']:
        patches.append((offset, braces(offset, width, NO_OFFSET, 0, 0, NO_OFFSET, 0, 0, 0, 0, NO_INDEX,
                                       NO_INDEX, 0, 0, 'FlatPatchKind::Zero', 'FlatHeader::None',
                                       'FieldSource::Synthetic', 'false', '""', '""', '""', '""', '""', '{}', '{}')))
    field_table = emit_array(output, 'FlatField', symbol + 'Fields', fields)
    byte_table = emit_array(output, 'std::uint16_t', symbol + 'ByteFields',
                            ('kNullIndex' if value == NO_INDEX else str(value) for value in data['byte_fields']), compact=True)
    copy_table = emit_array(output, 'FlatCopySegment', symbol + 'Copies',
                            (braces(native, mirror, size, quote(read)) for native, mirror, size, read in data['copies']))
    patch_table = emit_array(output, 'FlatPatch', symbol + 'Patches', (row for _, row in sorted(patches)))
    scopes = []
    for i, scope in enumerate(flat.scopes):
        scopes.append(braces(scope['native'], scope['size'], scope['parent'], scope['layout'],
                             quote(scope['host']), quote(scope['client']), quote(scope['read']),
                             indexes(f'ScopeFields{i}', scope['fields'])))
    scope_table = emit_array(output, 'FlatScope', symbol + 'Scopes', scopes)
    groups = []
    for i, (group, writes) in enumerate(zip(data['groups'], data['writes'])):
        segments = emit_array(output, 'FlatWriteSegment', f'{symbol}Writes{i}', (braces(*row) for row in writes))
        groups.append(braces(indexes(f'GroupFields{i}', group), segments))
    group_table = emit_array(output, 'FlatWriteGroup', symbol + 'Groups', groups)
    count_table = emit_array(output, 'FlatCountGroup', symbol + 'CountGroups',
                             [braces(indexes(f'CountFields{i}', group), limit)
                              for i, (group, limit) in enumerate(flat.count_groups)])
    zero_table = emit_array(output, 'FlatZeroConstraint', symbol + 'ZeroConstraints',
                            [braces(leaves[selector].ordinal, empty, indexes(f'ZeroFields{i}', fields))
                             for i, (selector, empty, fields) in enumerate(flat.zero_constraints)])
    output.append('constexpr FlatLayout ' + symbol + ' = ' +
                  braces(flat.root['layout'], data['mirror_size'], field_table, byte_table, copy_table, patch_table,
                         scope_table, group_table, count_table, zero_table) + ';\n')
