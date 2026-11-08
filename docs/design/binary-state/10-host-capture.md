# 10：Host 采集器

状态：设计稿，未实施。上级：[总计划](README.md)。

## 目标

在主线程的合法窗口内，按生成的复制计划把全部成员的状态复制到预分配的自有缓冲区，然后非阻塞地交给后台。主线程每帧 p95 ≤2 ms、p99 ≤4 ms。

## 输入事实

### 现有 Collector（`mod/src/world/collector.h`）

- **定位**：tx-04-08 建立的主线程只读采集器。所有入口零等待，运行期不分配内存。
- **读取方式**：通过 `MemoryReader.copy` 进行有界原始内存复制，捕获不可读地址，不调用游戏接口。
- **其他已有能力**：
  - 维护 `IdentityTable`；
  - 按 RTTI 分类；
  - 枚举实体表（`Room+0x1218` 起的 8 个列表）、roster、Grid 槽和门槽；
  - 记录逐身体证言 `PlayerPostWitness`。

### 现有 CaptureWorker 交接（`stream_capture_worker.h`、`stream-catalog.json` capture_profiles）

- **交接槽位**：一个构建中、一个最新的 ordinary 槽；冷源单独固定；8 个最新身体槽；256 个有序控制。
- **撤销机制**：用代次和水位撤销失效的交接。
- **后台约束**：后台线程不借用 Lua 或原生对象。

### 现有成本口径

stage11—18（[tx-04-21 采集性能方案](../tx-04-21/tx-04-21-capture-performance-plan.md)），其中 stage12 是主线程采集。

### 合法窗口与读取规则

ordinary 窗口、PlayerPost 窗口，以及规范第 11 节的条件读取规则（例如 `removedByFactory`）。

第 3 步的生成表把读取资格写在 `Field.read_requirement` 和 `CopyRange.read_requirement` 上（`entity-current-window`、`current-entity-byte-0x177`、`entity-current-window-and-confirmed-timer-clock`、`entity-current-window-and-confirmed-color-clock`）。本步必须为每种资格提供真实见证，不能从布局声明推导授权。条件计时由字段 owner 给出已核的实际递减条件；活动但时钟关系未知时报缺口，不能用 mode=0 冒充停走。EntityPlus 只经 `ReadEntityExtension` 读取已存在的来源（[第 3 步实现](implementation-3.md)）。Delirium 变身内的 Init 之后，`current-entity-byte-0x177` 的资格仍由 `world_delirium_owner` 见证恢复（第 4 步 4.2.6）。

## 设计

### 10.1 位置与时机

- **采集位置**：新采集器是 Collector 的一个采集阶段，在已核的 ordinary 窗口和 PlayerPost 窗口内执行，不新增 Hook，也不新增窗口。
- **按会话启停**：会话外逻辑停用，Hook 保持常驻。

### 10.2 布局识别

- **建立映射**：会话开始时，在后台按 `rtti` 名找到 J273 的类型描述符，建立“虚表地址 → 布局编号”映射表。
- **主线程查表**：主线程读取对象的虚表指针后查表。查不到时报具名缺口（`unknown_vtable`），该对象不采集，不能按 `_type` 猜测布局。
- **选臂**：如果类型声明了变体臂（第 1 步 `variants`），查到虚表之后还要读选择字段，按变体集合选定布局编号，例如 Effect 读 `Entity._variant`（第 4 步 4.1.2），NPC 读 `Entity._type`（第 4 步 4.2）。
  - 未列出的取值使用基础臂。
  - 选择字段读取失败时报具名缺口，该对象不采集，不能退回基础臂。
  - Delirium 的 AI 会临时把 `_type/_variant` 换成 Boss，返回前恢复。采集窗口不能落在 `0x49F3D0` 内，沿用 owner 未退尽不提交采集的规则。

### 10.3 复制计划的执行

第 4 步子类计数的 `entity-current-window-and-confirmed-subclass-counter-clock` 需要字段 owner 给出实际增／减／停走分支；未知方向或活动时钟关系报 Gap。Familiar／Knife 的 E9 使用收窄后的派生 Plus，Knife 可复用 `ReadKnifeExtension`。容器投影既检查元素数，也检查生成的 `Field.max_size` 与全局池预算；所有投影来自同一个窗口和同一组元素，不截断。

HeadFrameDelay 不属于已核线性计时，直接复制原始 i32。`ReadKnifeExtension` 接收会话 owner 核定的 Knife／KnifePlus 具体虚表绑定，只遍历前后两次 holder 链：先判目标类型，非 Knife 返回 Null；Knife 交 `ReferenceCapture::identify_or_reserve` 得到同代 Identity／Pending 纯值句柄，新成员不因尚无序号而失败。回调负责有界预约，不得向后台交原生地址；最终序号在后台分配。计数组按生成的 `CountConstraint`／`ValidContainerCountGroups` 检查，蝗虫两组合计最多 16384，投影重复 count 不重复计数。

生成器为每个布局产出一份操作序列（`state_capture.inc`）。主线程按顺序执行：

| 操作 | 动作 |
| --- | --- |
| `Copy(src, len, dst)` | 合并区间，一次 memcpy 进缓冲区 |
| `Convert(fn, dst)` | 调用具名换算函数，例如 Effect Timeout 的到期帧 = 全局帧 + Timeout |
| `Ref(src, dst)` | 读指针，查 `IdentityTable` 得到本地身份句柄，写入缓冲区；后台再转成成员序号 |
| `Resource(src, kind, dst)` | 读资源指针或路径，查资源表；遇到新资源先登记待发送 |
| `Container(src, elem_layout, cap, dst)` | 读 begin／end 或 count，校验后按元素布局逐项执行（嵌套计划） |
| `Sub(offset, layout)` | 对内嵌对象（ANM2、LayerState 数组等）递归执行其计划 |

- **一个对象一层保护**：每个对象的计划在一个 SEH 保护块内执行（一个对象一次，不是一个字段一次）。发生访问异常时，该对象的整条记录作废并报具名缺口，不写入半条记录。
- **地址来源**：对象地址只来自已核的枚举来源，以及 Collector 已校验的成员关系，不跟随任意指针。
- **条件读取**：带条件的字段（规范第 11 节）由计划中的条件操作控制，条件不满足时写入“不可读”标记。

### 10.4 缓冲区

- **预分配**：缓冲区和身份句柄表在会话开始时一次分配，分别归入现有的 512／128／128 MiB 三池。
- **双缓冲**：主线程写一份，后台读另一份。
- **超出容量**：报具名缺口，并把本次采集标为“不完整”交给后台。后台不发布不完整的 world 帧，改发下一次完整采集；容量按高水位在会话之间调整。
- **主线程不分配内存**：采集过程中主线程不分配内存，不加锁，不等待。

### 10.5 频率分层

- **body**：每个合格身体在 PlayerPost 窗口内执行 Player 的 body 块计划。
  - 只复制 P0 以及 E1／E4／E5 中属于 Player 的部分，写入对应的身体槽。
  - 没有 PlayerPost 资格的身体，由 ordinary 窗口补采（沿用现有的 ordinary body-joint 语义）。
- **普通**：ordinary 窗口按约 30 Hz 采集全部普通成员和房间、楼层的动态块。
- **低频轮转**：描述表 L1／L2、Mod 域、Run／Presentation 中的大块内容分片轮转读取。
  - 每个窗口只读一片，一整轮的周期由第 11 步按预算定稿。
  - 房间切换、楼层切换等已知事件触发立即全量重读。
  - 不依赖“通知完整”来跳过读取。

### 10.6 成本估计与计量

- **估计**：
  - 一个典型房间约 300 个实体。每个实体有效复制约 0.5—2 KB，图层和容器在外。
  - 合计约 0.6 MB memcpy 加约 1—3 万次指针跳转。
  - 估计主线程成本在 1 ms 量级。这是按 isaac-lan 实测（约 1.3—2.6 ms／视图，含 Lua）推算的目标，不是实测值。
- **计量**：沿用 stage12 口径，并按域细分计时，方便第 15 步定位。

## 改动范围

- Collector 新增采集阶段，以及复制计划解释器（读取生成的计划表，不手写逐字段代码）。
- 虚表映射表和资源表登记。
- CaptureWorker 交接的载荷改为二进制缓冲区，槽位和撤销语义不变。
- 现有 `CopyLuaSource`、`lua_entity.cpp` 的正式采集入口在第 14 步移除。

## 失败方式

- **跟随野指针**：跟随未经校验的指针读取，触发访问异常，或读到已释放的对象。
- **选错变体臂**：选择字段读错，成员套上错误布局；或者选择字段读取失败时没有报缺口，而是退回了基础臂。
- **复制半条记录**：异常发生在对象中途，前面的块已写入缓冲区却没有作废。
- **窗口时序错误**：在窗口外执行，或者 body 采集早于该身体的 PlayerPost 更新。
- **身份时序错误**：引用指向本帧刚加入、还未登记身份的成员，句柄查找失败。句柄必须按“待登记”处理，不能写 Null。
- **轮转漏变化**：低频轮转周期太长，变化长时间不被发现；或者已知事件没有触发重读。
- **超额不报**：缓冲区超额后静默截断。
- **主线程阻塞**：主线程等待后台释放缓冲区。缓冲区不足时，必须丢弃本次采集并报缺口。

## 完成内容

- **实现与编译**：采集器与计划解释器实现完成，纯编译通过。
- **代码约束**：主线程路径没有 Lua 调用、没有 JSON、没有内存分配，所有核心函数和数据结构带中文注释。
- **验证安排**：性能和正确性的验证全部在第 15 步完成。

## 未决问题

- 低频轮转的分片大小和一整轮的周期。
- 身体槽数量 8 是否继续足够；Esau、Tainted Lazarus 等多身体情况下要核对上限。
