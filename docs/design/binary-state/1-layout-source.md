# 1：布局源、字节归属模型与生成器

状态：已实施，普通 Release 纯编译／链接通过（2026-11-08），没有测试。上级：[总计划](README.md)。实现入口：[第 0—2 步实现说明](implementation-0-2.md)。

## 目标

建立唯一的布局源，描述每个原生类型的字节归属和状态布局，并用一个生成器同时产出以下内容。这样代码、协议和文档出自同一份数据，不会互相脱节：

- Host 复制计划；
- 线格式偏移表；
- Client 解码、校验和写入分派表；
- 公开成员回映射表；
- 文档中的单行表格。

## 输入事实

- **已有原生字段清单**：`mod/binding/world_layout.json` 有 78 个字段，每个字段带 `offset`、`width`、`encoding`、`status`、`evidence`。`classes` 记录了对象大小和 RTTI，`sources` 记录实体表、roster、Grid 数组等枚举来源。由 `mod/tools/gen_binding.py` 生成 `world_layout.inc`（`mod/CMakeLists.txt:110-113`）。
- **已有 X-macro 表**：`mod/src/game/client_entity_fields.inc` 有 57 个公共字段加 2 个特殊字段，目前只驱动 Client 写入，按字符串比较成员名（`client_entity_common_native.cpp:43-50`）。
- **已核偏移**：`docs/design/tx-04-19/entity-application.json` 中，`common_native` 有 56 项，`class_native` 有 174 项，都带已核偏移和原生类型。
- **待取代的生成器**：`mod/tools/gen_stream_catalog.py` 生成逐成员目录、`field_ids` 和字段分层，将由本步的生成器取代（第 14 步退役）。
- **公开成员数量**：Entity 域 1016 个 `ReplicatedState` 成员中，只有约 230 个已核定原生位置。Room／Grid 有 472 个，Player 另见 `player-application.json`。

## 设计

### 1.1 布局源

**位置与版本**：`mod/binding/state/<域>.json`，每个域一个文件，另设 `mod/binding/state/version.json` 记录整数布局版本。只要布局源有任何改动，就必须手工递增版本号。版本是 u16 正整数，与第 9 步帧头的 `layout_version` 同宽，生成器拒绝超出范围的值。生成器把版本写入协议声明，握手时比较，不计算哈希。

**每个类型条目包含**：

- `type`：原生类型名。
- `native_size`：对象大小。以 J273 实际大小为准（池步长、工厂申请大小等），不以 SDK 声明为准；两者不同时分列保留，例如 Effect 为 0x4C0／0x490（第 4 步 4.1），Door 为 0x2F0／0x2EC（第 6 步）。
- `parent`：父类型名。
- `rtti`：用于 Host 按虚表分类的 RTTI 名，以及 J273 的 RVA。
- `layout`：布局编号。
- `ranges[]`：按偏移排序、互不重叠、覆盖 `[0,native_size)` 的字节段。每段包含：
  - `offset`、`size`；
  - `category`：第 3 节的十种类别之一；
  - `type`：值类型，例如 `f32`、`i32`、`u8`、`bool`、`Vector`、`std::vector<T>`；
  - `name`、`semantics`（中文语义，必填）；
  - `evidence`：来源种类和定位，例如 `ida:J273 0x6A4B1D`、`sdk:Entity.zhl:194`、`lua:LuaEntity.cpp`；
  - `status`：`已核`、`SDK声明`、`推断` 或 `未知`。
- `blocks[]`：状态块，每块有块号、名称、字段顺序，以及所属版本序列（`world` 或 `body`）。
  - **块号规则**：父类型的块号固定在前，子类型的块接着编号。Entity 布局的 E0—E9 固定为块 0—9，子类块从 10 开始。
  - **块数上限**：每个布局最多 32 块（第 9 步 9.4）。
- `conversions`：换算类字段的 Host 读取规则和 Client 写入规则，以具名函数引用表示，例如 Effect Timeout 的到期帧。随帧变化的计数按帧锚点规则处理（总计划第 4 节）。
- `validators`：取值范围、枚举集合、有限浮点、容器上限。
- `count_groups`：多个原生容器共用的元素数总上限，包含名称、字段名、limit 和证据；例如蝗虫两组 Sprite 合计最多 16384。生成到 `CountConstraint`，同容器各子块投影必须同 count，求和时只计一次。`ValidContainerCountGroups` 检查完整同版本计数，Host 复制元素前及 Client 写入前分别调用。
- `members`：公开成员到布局字段的回映射。
- 共享公开目录可在`version.json.catalog_complete_scopes`声明`{"catalog":"原目录路径","domain":"run"}`，按原始一基行号完整覆盖该domain并生成专用汇总表；不要求尚未研究的同文件其它域提前入源，也不按重复ID去重。这是普通构建的源一致性处理，不是额外测试。
- `catalog_rows`：保留来源目录＋一基行号、ID、owner、原parameters／returns、处置、正式字段／上下文及理由；重复ID不合并。类型级`catalog_complete`仍要求该类型覆盖完整目录；版本55增加`version.json.catalog_complete`，允许一个目录的行分布在多个真实布局，按原行号聚合且恰好一次。普通构建同源生成完整目录汇总文档；Grid119行见[映射说明](grid-member-map.md)。
- **回映射不强制发送**：已有公开目录行改归 Host 权威时保留该行，使用 `disposition=authority` 并移除旧状态依赖；同步删除原生范围的 `wire`、对应 validator 和块内字段。空块显式 `reserved=true`，不前移后续块号。不能用旧 E2E 原值相等要求反向恢复不必要的同步，见[版本 28 职责复核](synchronization-field-review.md)。
- `contexts`：嵌套类型的已核父上下文，例如所属资源号和容器元素序号。派生成员用 `@名称` 回映射；上下文不冒充原生字节，也不重复发送。
- `extent`：默认 `object`。只有已核前缀、尚不代表具体动态对象大小时写 `base_prefix`；此类布局不能宣称完整，例如 Weapon 基类。Trail节点自版本38起已有606190的0x40分配与完整复制证据，不再是base_prefix。
- 第 3 步补充：`companions` 表示另有原生字节表的伴随对象，以具名 Host/Client owner 解析；生成字段明确为 `FieldSource::Companion`，没有根对象偏移，不把 EntityPlus 伪造成 Entity 内嵌字段。父类的伴随来源随继承保留。
- 第 4 步补充：子类可用同名 `companions` 把来源收窄到派生类型，例如 Familiar／Knife 的 EntityPlus；生成器校验真实继承关系，不重复发送公共前缀，也不允许替换成无关类型。
- 第 3 步补充：`read_requires` 保留特殊读取资格，生成到字段和合并区间；资格不同的区间不合并。`shape` 生成到字段元数据。`world-or-player-body` 表示公共块在普通 Entity 使用 world，在 Player 使用 body，供第 5/10/11 步接线。
- `variants`：变体臂，可选。用于同一个原生类按某个字段的取值采用不同字节归属的情况，例如 Effect 的 0x420 复用区和 ExtraData（第 4 步 4.1.2），以及 NPC 中只对 Type 412 有意义的 Delirium 段（第 4 步 4.2）。
  - **内容**：选择字段，以及每个臂的名称、变体集合、布局编号，和该臂覆盖的 `ranges`、`blocks`。
  - **默认臂**：未列出的取值使用基础臂。
  - **稳定性**：选择字段本身的变化必须属于形态变化，这样一个成员在生命期内布局编号不变。
  - **生成**：生成器为每个臂各输出完整的复制计划和表格。

### 1.2 生成产物

| 产物 | 用途 |
| --- | --- |
| `state_capture.inc` | Host 复制计划：每个布局的合并区间、换算项、引用项、容器项 |
| `state_wire.inc` | 每个布局、每个块在线上的偏移、大小、元素类型和上限 |
| `state_client.inc` | Client 解码校验表与写入分派表，按字段索引分派，不比较字符串 |
| `state_body_codec.inc` | 第9步同源正文计划：标量／条件／精确范围／计数组／规范规则，运行按数字索引分派 |
| `state_members.inc` | 公开成员回映射，供 E2E Lua 对照（第 15 步）和覆盖报告使用 |
| `docs/design/binary-state/layouts/<步骤目录>/<类型>.md` | 两张单行表格，格式见[总计划第 5 节](README.md#5-布局表格要求) |

**版本60正文机器规则**：`wire.when.test`为条件提供明确的整数／bit／集合／item-config操作，原predicate语义和证据保留；`wire.value_rule`登记非Null、可空水面及计数模式，`body_rule`登记空主动槽／Null costume规范。它们同源生成到`state_body_codec.inc`，不在运行时解释自然语言，也不改变公共头或字段顺序。实现边界见[第9步正文](9-domain-body-codec.md)。

### 1.3 区间合并规则

- **合并条件**：只合并同一状态块内、类别都是“状态”、在原生内存中首尾相接的字段。
- **断开条件**：遇到填充、未知、本地、引用、资源、容器、换算字节都要断开。证实为填充的字节也断开，保证 memcmp 只比较有语义的字节。
- **复制顺序**：区间按原生偏移升序复制，在状态块中按块内字段顺序排列。
- **嵌套拆块**：复杂嵌套的 `wire.projections` 可以把子块分别放入父块（用 `字段名#子块号` 引用），要求所有实际产生发送字节的子块恰好各一次；空预留子块可省略，原编号保持；既存空投影合法且不产生负载。这样 Entity 的 E4/E5/E6 可复用 ANM2 一份字节事实。
- **间接共享 owner**（第 4 步）：`kind=nested, storage=shared-owner` 的原生段必须为 8B holder 且归属“容器”，必须给出具名 Host／Client 处理器；payload 大小来自独立布局，不能按 holder 内嵌读取。投影 0 带 u8 present，其余投影依赖同目标版本存在性；缺失 owner 时各投影使用最短全零编码。根对象的原生表只计 holder，payload 的未知字节单独统计。编码与有界解码由 `state/owned_payload.*` 提供。
- **容器元素拆块**：复杂容器的 `wire.projections` 同样投影元素子块。每个投影带自己的 u16 count；同一目标版本的投影计数必须一致，Client 在全部依赖就绪后由同一 owner 安装。Familiar 的两组蝗虫 Sprite 和共享 payload 据此拆分外观／动画／图层。
- **容器字节预算**：`wire.max_bytes` 可另限每个容器字段／投影的完整编码大小（含 u16 count），不改变 `limit` 的元素数边界。生成的 `Field.max_size` 取二者约束的较小值，采集／解码还须检查实际编码长度和全局池预算，超限报缺口，不截断。这样复杂 ANM2 容器不会因逐级理论最大值相乘而溢出 u32。
- **J273 deque（版本41）**：`wire.storage=deque`显式声明20B原生头，只允许已核4B EntityPtr／8B Vector步长，归并后按同一u16 count＋投影元素格式编码。生成器不把头作为普通复制区间；`ReadDequeInto`处理16B块、环形逻辑顺序与合法窗口，Client具名owner保留本地proxy并重建map／blocks。详见[NPC证据](npc-evidence.md)。

### 1.4 生成器拒绝生成的情况

以下情况在构建时直接报错，属于构建行为，不是测试：

- **字节覆盖**：字节段重叠、留空，或超出 `native_size`。
- **缺少内容**：任一段没有语义文字或证据。
- **定义不全**：引用字段没有目标空间，容器没有元素类型或上限。
- **资源表拼错**：资源空间须在 `version.json.resource_tables` 中声明，字段 `wire.target` 必须使用规范名称；Familiar 与 TemporaryEffect 共用 `item-config`，不隐式产生同义表。
- **计数组无效**：组名重复、字段不是容器、同容器重复分组或未生成状态字段；容器分块计数按同一 slot 关联，不能各算一组独立上限。
- **布局不一致**：块中字段与字节段对不上，或同一字段出现在两个块中。
- **映射缺失**：公开 `ReplicatedState` 成员没有回映射，也没有声明为派生、权威或不适用。
- **变体臂不成立**：两个臂的变体集合重叠；选择字段不是形态字段；或者某个臂合并共用部分后，没有完整覆盖 `[0,native_size)`。

## 改动范围

- **新增**：`mod/binding/state/*.json`、`mod/tools/gen_state_layout.py`，以及 `mod/CMakeLists.txt` 中对应的生成命令。
- **保留**：`world_layout.json`，供既有 Collector 和 Hook 使用；其中已核字段可被布局源引用作为证据，但不复制成第二份事实。

## 失败方式

- **布局与 J273 不符**：布局源与 J273 实际布局不一致，例如 SDK 名与公开语义错位的 DepthOffset。
- **区间吞入无意义字节**：合并区间吞入填充或指针，导致 memcmp 误判变化，或发出本地地址。
- **版本未递增**：布局改了版本号没变，两端按不同布局解码。
- **输出不稳定**：生成顺序不确定，导致布局编号或偏移每次构建都不同。
- **文档未更新**：文档表格没有重新生成，与代码不一致。
- **映射不全**：回映射只覆盖了一部分成员，却被当作完整核对。

## 完成内容

- **生成器**：布局源 schema、结构与语义检查、四份代码表和文档生成已实现；基础库及完整 DLL 的普通 Release 编译／链接通过。
- **实现位置**：`mod/binding/state/schema.json`、`mod/tools/gen_state_layout.py`、`mod/src/state/layout.*` 和 CMake 的 `isaac_cs_state`。只在普通构建中生成，不另运行校验脚本或测试。
- **首批产物**：以第 2 步的基础类型和 EntityEffect 为首批输入。Effect 沿用修订后的 FX0/FX1/FX2 和变体模型，不采用总计划 5.1 已被取代的 69B 单块示例；各类型的完整内容由第 2—8 步填入。
- **布局与表格**：首批 14 基础类型加 Entity/Effect 种子，共 18 个展开布局，入口为[生成布局索引](layouts/README.md)。未知、SDK 声明和基类前缀的未闭合状态照实显示，不充当完整支持的证明。

## 未决问题

- **格式已定**：沿用 JSON，与现有生成链一致；生成器内置 schema 所用结构检查，不要求额外 Python 包。
- 回映射中“派生”的判定，需要第 3—8 步逐类给出依据。

版本38支持显式 `wire.inline_count`：固定内联复杂数组按槽数×原生步长覆盖范围，线上上限不得超过物理容量；原生字节表展开全部物理槽位，即使有效前缀为空也保留各槽UNKNOWN。普通 `storage=array` 也可能是指针／具名owner归并，只有显式inline_count才按内联数组展开。Effect191的四槽ANM2采用这一声明，线状态只发有效前缀；专用视图134只把该场景绘制前完全覆写的Offset／Scale归派生，通用ANM2不变。见[Effect复核](effect-synchronization-review.md)。
