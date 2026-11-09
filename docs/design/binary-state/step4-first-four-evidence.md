# 第 4 步前四项未决的必要静态证据

日期：2026-11-09。目标为 J273 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，IDA imagebase `0x400000`。使用当前 IDA MCP 连接的只读数据库和本地 REPENTOGON v1.1.3 SDK／既有研究记录。下文地址均为 IDA VA，RVA=`VA-0x400000`。没有运行游戏、构建、测试或校验脚本；没有读取哈希，也没有修改或保存 IDA 数据库。

证据分为 IDA 直接指令、官方 SDK 声明、项目既有 J273 记录和静态推导。`UNKNOWN` 表示本次资料不足，不以构造零写或搜索无命中自动推成填充。第1—5节保留初轮版本25的研究范围与当时结论；续研全映像候选及纠错见第6节以后。初轮负向搜索只覆盖所列代码段，不排除别名、间接调用或范围外消费者。

## 1. NPC／Familiar 大段未知的核对工作量

估算沿用已核定的池步长、公开字段和既有 owner 证据；这次不声称已经逐字节完成 NPC／Familiar。总步长减去已知字段字节会重复计算 Entity 公共前缀、嵌套对象和原生容器头，不能作为未知工作量。

| 类型 | 已有结构依据 | 尚需处理的实际范围与相对工作量 |
| --- | --- | --- |
| Familiar | 子类共 2848B；`+0x5FC` 的寻路对象 1872B 有 SDK 声明；`+0xDB8` 内嵌 ANM2 276B 可复用通用布局 | 子类自身未知 557B、9 段；其中 `+0x418—0x5F7` 连续 480B，其余 8 段合计 77B。根表的 906B 未知还含 Entity 公共 335B 与内嵌 ANM2 14B，不能重复算作 Familiar 专有范围。寻路对象证据等级、条件计时和 payload 完整性另需核对。 |
| NPC | 子类共 2896B；寻路对象 `+0x414—0xB63` 1872B；已有双 ANM2 `+0xCD8/+0xDEC` 共 552B；三组 deque 头 `+0xBF0/+0xC04/+0xC18` 共 60B | 上述已识别结构之外还有 412B 混合区域，含公开值、Delirium 变体臂、ColorMod、引用／容器及未知；412B 不等于未知字节总数。NPC 尚未进入生成源，需先迁入已识别结构，再按消费者细分剩余区域。deque 外部元素和 owner 不计入 60B 头。既有复合绘制证据见 [`entity-i20-i25-native.md`](../tx-04-19/entity-i20-i25-native.md)。 |

建议拆分次序如下，工作量为相对估算，不承诺固定工时：

1. 一起核 NPC `+0x414` 与 Familiar `+0x5FC` 寻路对象的构造、重置和调用链，先区分 Host AI 数据与确有展示依赖的部分；不把名称或空白区直接归权威。
2. 迁入 NPC 双 ANM2、三组 deque、已核公开字段和 Delirium 变体臂，再按实际消费者补 owner、引用和资源合同。
3. 核 Familiar `+0x418—0x5F7` 的元素、索引／长度规则、写入者和消费者。已有 60 组 8B 清零只证明构造动作。
4. 核 Familiar 零散段，优先 `+0xD90—0xDAB`、`+0xED8`、`+0xF28/+0xF2C`；其他小段保留其当前未知等级。
5. 按 `[0xB64,0xBF0)`、`[0xC2C,0xCD8)`、`[0xF00,0xF60)` 核 NPC 混合区，分开共用值、形态专属状态、引用、容器、权威和未知。
6. 最后复核零散尾部、SDK 差异、padding 候选及跨函数活动条件；证据不足的字节继续列 UNKNOWN。

这是一份核对次序建议，不表示任何未决 NPC／Familiar 字节已完成归属。

## 2. Knife、Slot、Projectile、Effect 的对象大小与尾部

### 2.1 编译器数组元素大小

`EntityFactory` 构造函数 `VA 0x6EEE60 / RVA 0x2EEE60` 调用 MSVC `eh vector constructor iterator`，为每池传入数量、单元素大小以及该类 ctor／dtor 回调。相关指令如下：

| 类 | ctor／dtor 参数与元素大小指令 | 数量 | 采用的 J273 元素大小 |
| --- | --- | ---: | ---: |
| Projectile | dtor `0x6EEF0F`、ctor `0x6EEF14`、size `0x6EEF28 push 0x478`、iterator call `0x6EEF31` | `0x200` | `0x478` |
| Slot | dtor `0x6EEFD3`、ctor `0x6EEFD8`、size `0x6EEFEF push 0x550`、iterator call `0x6EEFF5` | `0x80` | `0x550` |
| Knife | dtor `0x6EF097`、ctor `0x6EF09C`、size `0x6EF0AD push 0x520`、iterator call `0x6EF0B6` | `0x40` | `0x520` |
| Effect | dtor `0x6EEE8C`、ctor `0x6EEE91`、size `0x6EEE9B push 0x4C0`、iterator call `0x6EEEA1` | `0x400` | `0x4C0` |

上述参数是 J273 中以对应类构造／析构函数迭代数组时使用的编译器元素大小，是实际对象 `sizeof` 的强静态依据，本步按它们作为布局上界／元素范围。SDK 声明与之不一致时，以 J273 数值为准：Knife 的 ZHL `0xE78` 不是本池的实际类元素大小；Slot `0x548` 与 Projectile `0x470` 的声明少 8B；Effect 的 ZHL `0x490` 少 `0x30`。该参数本身不解释尾部字节的语义。

### 2.2 尾段直接证据与建议布局

| 类型／范围 | 直接静态证据 | 建议归属与仍未知内容 |
| --- | --- | --- |
| Knife `+0x504—0x51B` | ctor `0x939AD0 / RVA 0x539AD0`；Init `0x939C70 / RVA 0x539C70`；Update `0x93AAF0 / RVA 0x53AAF0`；分支辅助 `0x93A5F0/0x93A870`；draw helper `0x93F680 / RVA 0x53F680`。Init 在 `0x93A06E/0x93A0A1/0x93A0CF/0x93A0FB/0x93A109/0x93A0EF` 初始化或写入六个槽；Update 在 `0x93BB3A/0x93BB72/0x93BCC7/0x93BD69/0x93BDC7/0x93BDCF/0x93BE49/0x93C92A/0x93C948` 用 `movss` 读写；`0x93CBBE` 写 `+0x518`，draw helper `0x93F75D` 以 `movss` 读取 `+0x518`。 | 将 `+0x504、+0x508、+0x50C、+0x510、+0x514、+0x518` 各保留为一个 f32 原值；语义、有效变体和更新条件仍未知。`+0x518` 有绘制读取，不归 Host-only。保留 `+0x51C—0x51F` 为 UNKNOWN，不把 28B 尾段整体标成 padding。 |
| Slot `+0x548—0x54F` | Init `0x659AA0 / RVA 0x259AA0` 在 `0x659AE8` 将 `n4+0x14` 写到 `this+0x548`；官方 `Manager.zhl:105` 声明 `Manager._persistentGameData +0x14`。Slot Update `0x65A210 / RVA 0x25A210` 与 helper `0x65F340` 多次读取／传递 `+0x548`；构造函数 `0x6599C0`、Init 和 Update 也访问 `+0x544`。 | `+0x548` 是 `PersistentGameData*` 本地借用指针，不发送；variant 11 的原生路径可能将其替换为 Spawner 相关对象，Client 不运行该玩法路径。`+0x54C—0x54F` 在本次 Slot 代码范围内无直接位移命中，保留 UNKNOWN，不能仅因 `+0x548` 已识别而将后 4B判成 padding。 |
| Projectile `+0x470—0x477` | Init `0x64F4B0 / RVA 0x24F4B0` 在 `0x64F5F7` 清 `+0x470`。Update `0x64F990 / RVA 0x24F990` 在 `0x654AFB` 取该 dword，测试后经寄存器调用（`0x654B08 call eax`，随后 `0x654B0A add esp,8`）；Interpolate `0x658E10 / RVA 0x258E10` 在 `0x658EBB` 同样取出并调用（`0x658EC8 call eax`，随后 `0x658ECA add esp,8`）。 | `+0x470` 是本进程回调函数指针；Update 与 Interpolate 分别传入模式 `0`、`1`。它是本地归属，不得跨进程发送或把远端地址调用。`+0x474—0x477` 在 `0x64F400—0x659A00` 的本类代码扫描中无直接位移命中，保留 UNKNOWN。 |
| Effect `+0x4BC—0x4BF` | Pool iterator element size 为 `0x4C0`。Effect ctor `0x5CCEA0 / RVA 0x1CCEA0` 写入到 `+0x4B8`；析构 `0x5CCFD0 / RVA 0x1CCFD0` 销毁 `+0x4A8` vector 并经 `+0x4B4` 清 owner。本次对 Effect 代码段的直接位移扫描未找到 `+0x4BC` 访问。 | 对象布局按 `0x4C0`。`+0x4BC—0x4BF` 暂保留 UNKNOWN（尾部对齐是候选推断）；无访问不等于 padding 证据。 |

Effect 负向扫描范围为 IDA listing `VA [0x5CC000,0x604500)`，包括该地址段中可见的 ctor、Init、Update、Render、ClearReferences 与 helper 代码；按 `[reg+disp]` 直接位移检索 `+0x41C`、`+0x44C`、`+0x4BC` 均未命中。其余限定范围为Projectile `[0x64F400,0x659A00)`、Slot `[0x6599C0,0x663000)`、Knife `[0x939AD0,0x940000)`。Effect范围只覆盖ClearReferences在该界限内的部分，不宣称完整覆盖范围外的函数尾部或callee。由此只保留 UNKNOWN，不宣称全映像不存在通过别名或间接路径的消费者。

## 3. Projectile `DepthOffset +0x358` 别名复核

原生公开 binding 在 Entity 与 Projectile 两处都绑定到同一对 getter／setter：

| Binding 登记 | 指向函数 | 结果 |
| --- | --- | --- |
| Entity `0x84F33F/0x84F344/0x84F349` | `0x431380/0x4313A0` | `0x431388` 写 `float [ecx+0x358]`；getter `0x4313A0` 读同址 |
| Projectile `0x8514CF/0x8514D4/0x8514D9` | `0x431380/0x4313A0` | 同一 setter/getter，仍指 `+0x358` |

因此 Projectile 的目录成员 `DepthOffset` 是 Entity 公共 E4 `depth_offset` 的同值别名，只映射／发送一次。REPENTOGON SDK 中 `_depthOffset +0x2B0` 不构成此公开 binding 的证据。

## 4. Effect 指定区间

### 4.1 `sizeof` 与 `+0x484—0x493`

Effect 池由 `0x6EEE60` 的 `0x4C0` 元素大小、`0x400` 个元素、ctor `0x5CCEA0` 和 dtor `0x5CCFD0` 共同建立。该数组元素大小强力支持 J273 `Entity_Effect sizeof = 0x4C0`，不是 ZHL 声明的 `0x490`，也不是仅按最后一次 ctor store 推断的 `0x4BC`。可安全用于范围的结论是对象范围 `[0,0x4C0)`；尾端 `+0x4BC—+0x4BF` 单独仍未知。

`+0x484—0x493` 不是容器：唯一专用生成 helper `sub_5D3480 / RVA 0x1D3480` 将 `&Effect+0x484` 和 `&Effect+0x48C` 作为 `const Vector& position/velocity` 传给 `Game::Spawn 0x6FA210 / RVA 0x2FA210`。官方 `libzhl/functions/Game.zhl:29` 的 Spawn 签名确认这两个参数分别是位置与速度向量，各占 8B。ctor `0x5CCEA0` 在 `0x5CCEF3/0x5CCF07` 等指令清零其四个 dword，只能作为初始化佐证，不能再解释成 16B容器头。

### 4.2 `+0x440—0x44F` 与 `+0x47C—0x4A7`

`sub_5D3280 / RVA 0x1D3280` 被 Effect Update `sub_5D35D0` 在 `0x60079E / RVA 0x20079E` 调用；局部前置按 Variant `44、46、45、32、204` 分流（`0x60075F—0x60078F`）。函数直接使用：

- `+0x440` 在 `0x5D32AD` 读作模式值，与 `4、2、1` 比较；分支还读写 +0x430/+0x434 等状态。
- `+0x448` 在 `0x5D3336/0x5D3402` 作为浮点角度参与角度换算与速度旋转。
- `+0x444` 在 `0x5D3422` 作为引用参数传给 `0x633330 / RVA 0x233330`。后者在 `0x633460` 将关系写进新 Effect `+0x444`，若非空在 `0x633469` 调 `0x4C79D0` 登记反向引用。它不是可裸复制的远端地址；引用的完整生命周期与全部变体消费者仍须遵循 EntityPtr owner 规则。
- `+0x44C—0x44F` 在本节所述 Effect 代码范围内未找到直接位移，继续 UNKNOWN。

`sub_5D3480` 由 Effect Update 在 `0x6006CD / RVA 0x2006CD` 调用，用于 Variant 203 的生成路径。其 `Game::Spawn` 参数按官方 ABI 对应为：

| Effect 偏移 | Spawn 参数／用途 | 证据边界 |
| --- | --- | --- |
| `+0x47C` | Type | `0x5D3550` 压入；仅该专用生成路径 |
| `+0x480` | Variant | `0x5D354A` 压入 |
| `+0x484—0x48B` | Position `Vector` | `0x5D3543/0x5D3549` 传地址 |
| `+0x48C—0x493` | Velocity `Vector` | `0x5D352A/0x5D3542` 传地址 |
| `+0x494` | Spawner 指针 | `0x5D353C` 压入；本地玩法输入 |
| `+0x498` | SubType | `0x5D3536` 压入 |
| `+0x49C` | Seed | `0x5D3530` 压入 |
| `+0x4A0` | Spawn 第八参数 `unk` | `0x5D351E` 压入；业务语义仍 UNKNOWN |
| `+0x4A4` | `spawn_grid_index` | Spawn 返回后由 `0x5D355B/0x5D3563` 写到新 Entity `+0x32C`；当前项目 Entity 应用目录已命名该槽 |

这些字段解释的是 Host 玩法生成请求，不表示 Client 要再次调用 Spawn。建议按具名字段拆分，归权威／本地来源；该路径的专用用途已证实，证据等级仍为推断，因为未穷尽所有变体和间接消费者。`+0x440/+0x444/+0x448` 亦只按上述专用更新路径保留语义；其它使用和生命周期没有在本次穷尽。

### 4.3 保留的未知与布局建议

1. 布局对象范围采用 `Entity_Effect sizeof 0x4C0`；SDK `0x490` 不能做上界。
2. 删除“`+0x484—0x493` 内联容器”的旧描述，拆成两个 8B `Vector`。
3. 将 `+0x47C—0x4A7` 依上述生成参数拆字段，并按 Host 权威／本地指针分类；不发送 spawner 指针或本地容器头，不在 Client 重放玩法生成。
4. `+0x41C—0x41F`、`+0x44C—0x44F`、`+0x4BC—0x4BF` 保留 UNKNOWN。`+0x4BC` 是 0x4C0 元素范围内的尾部；对齐是候选解释，尚不提升为已核 padding。
5. `+0x440/+0x444/+0x448` 保留模式、Entity 引用关系、角度这些已有用途；若生成布局将其归权威，标为推断并保留全变体／间接消费者未闭合状态。

该结论不覆盖 Effect 基础臂 `+0x420—0x43F`、其它 variant 的私有状态、ExtraData 动态类型，也不等于第 15 步验收完成。


## 5. 主智能体审阅与最终采用

2026-11-09，主智能体独立复读上述工厂的ctor／dtor／数量／元素大小push与iterator调用；复读两组DepthOffset注册、`431380/4313A0`；复读Knife构造、Init尾段及`93BCC7/93BD69/93BDC7/93BDCF/93C92A/93C948/93CBBE/93F75D`的实际浮点指令；复读Slot构造／Init及Manager.zhl；复读Projectile两个`call eax`窗口；复读Effect构造、`5D3280`、`5D3480`、`633330`、`6044A0`及传播／生成分派窗口，并核对Game.zhl八参Spawn签名。

最终采用见[落地说明](implementation-4-first-four.md)，布局版本25：

- **采纳**：数组元素范围、DepthOffset单份映射、Knife六个尾部f32及独立K4、Slot进度对象指针和Projectile回调指针的本地归属、Effect两个Vector及专用传播／生成参数的具名边界。
- **保留证据等级**：Knife六个值用实际计算／偏移命名，完整业务语义未穷尽，保持“推断”。Effect56B参数按已见Host玩法用途归权威，保持“推断”；未将限定代码的阴性搜索升级为全程序无消费者证明。无新增Client玩法调用或引用写入。
- **纠错记录**：子智能体中间消息把`5D331B`的常数`0x3FE00000`称为0.5，实际是1.75；把`+444`初称裸借用，主审根据`63345A lea eax,[esi+444] → 633460 mov[eax],edi → 633466 push eax → 633467 ecx=edi → 633469 call4C79D0`纠正为有反向登记的关系槽。Knife步距中间消息误写“512”，按原始`push 0x520`纠正。Projectile间接调用准确地址采用`654B08/658EC8`，替代初稿“附近”地址。
- **不增加生命周期结论**：`6044A0`仅见基类ClearReferences和ExtraData清除，没有据此补造`+444`的完整撤边流程；新布局不写该Host关系。`+4A4`按已核新Entity `+32C`为i32格子序号，拒绝反编译器的float推型。
- **剩余未知**：Knife `+51C—51F`、Slot `+54C—54F`、Projectile `+474—477`、Effect `+41C—41F/+44C—44F/+4BC—4BF`没有被证明为填充。其他原有未知区、未核变体和来源生命周期继续保留。

本报告开头“没有构建”指子智能体的静态研究范围。主侧随后完成普通产品构建并生成版本25布局；构建不是运行时验证，未执行任何测试，统一验收仍在第15步。


## 6. 续研主审：大小、字段语义与四段空白的全映像候选

本节是版本26的主审记录，修正第1—5节中被新证据替代的结论；不删除初轮失败和错误判断。逆向子智能体仍为用户指定的同一个GPT-6 Luna（max）。主侧复读原始指令和伪码，并独立复现了三个偏移的全映像扫描。

### 6.1 编译器对象大小的第二组独立证据

四类vtable slot0删除析构器在释放分支显式压入大小，再压入对象地址，调用释放函数后回收8B参数：

| 类型 | vtable／删除析构 | 原始size指令 | 与工厂一致的sizeof |
| --- | --- | --- | --- |
| Knife | B2D7A0／939BB0 | `939C00 push 520h`；`939C05 push esi`；`939C06 call free_` | 0x520 |
| Slot | B15BDC／659A30 | `659A54 push 550h`；`659A59 push esi`；`659A5A call free_` | 0x550 |
| Projectile | B15B1C／64F470 | `64F487 push 478h`；`64F48C push esi`；`64F48D call free_` | 0x478 |
| Effect | B14D00／5CCF30 | `5CCFA2 push 4C0h`；`5CCFA7 push esi`；`5CCFA8 call free_` | 0x4C0 |

这些包装器先设置对应类vtable并执行成员／基类析构。Hex-Rays把释放伪码显示成单参数，不能因此忽略原始指令的size。工厂数组迭代和删除析构两条编译器路径共同核定大小；大小本身仍不能给尾字命名。

### 6.2 Knife六个f32的用途与重要纠错

`93A5F0`与`93A870`分别对应多维／棱镜分裂路径，官方EntityKnife.zhl命名`+4E6 _multidimensionalApplied`、`+4E5 _prismApplied`。主审复读：

- `93A638 → 93A645`和`93A8B8 → 93A8C5`把自身`+444 knifeDistance`原值写入`+514`，标记本刀分裂触发时的距离。
- 新子刀Type8在`93A814`／`93AA5F`接收父刀距离到`+504`。棱镜子刀的`+508`为`i*20-30`度；多维子刀`+50C=0`、`+510=+10/-10`。
- Update沿父刀链以`+504`判断分裂关系，`+508`旋转相对位置并叠加方向，`+50C/+510`作为二维局部偏移旋转后加世界位置。`93C92A`／`93C948`检查`+504/+514`是否非负，返回分支在`93C953`／`93C976`重置为-1；不是倒计时。
- **纠正初轮绘制说法**：vtable slot5为`93ECD0`（Render），slot23为`93F680`（HandleCollision），与官方EntityKnife.zhl虚槽声明相符。`93F732`检查碰撞目标Type4，`93F75D`读取`this+518`，角度转弧度后在`93F810/93F818`写目标速度。`+518`是Update保存的碰撞方向角，初轮“draw helper”及“绘制必需”判断错误。版本26将字段改名`knife_tail_collision_angle`，K4改名“分裂与碰撞参数”，仍保留六个f32／24B原值，不在Client执行碰撞。

以上六值的原生类型、具体计算用途及状态布局可以闭合，不要求恢复原作者C++变量名。证据升级为“已核”。Effect传播／生成56B参数同理：具体消费者及Host归属已核；不复制这些地址，也不在Client调用相关玩法，故不把穷尽原版关系生命周期列为本项同步依赖。

### 6.3 全映像候选方法与类型归属

子智能体扫描`.text [401000,AAA000)`的1,805,294条IDA code-head，decode操作数，以内存位移区间与目标4B范围相交筛MEM，附近LEA另列。主侧独立复现MEM范围相交扫描，Effect41C／44C／Projectile474候选函数数分别为108／33／91，与子智能体一致。这里的函数数是**同偏移候选数，不是该类型的消费者数**。栈访问、其他实体、保存结构和第三方库均会命中。

| 目标段 | MEM命中／函数数 | 附近LEA命中／函数数 | 主审结论 |
| --- | ---: | ---: | --- |
| Knife 51C—51F | 25／17 | 19／16 | 未找到真实Knife消费者；同偏移命中按下列来源排除 |
| Slot 54C—54F | 25／18 | 12／8 | 未找到真实Slot消费者；已覆盖本类尾部指针之外的候选 |
| Effect 4BC—4BF | 30／21 | 43／24 | 本类邻接别名仅为4B4的8B共享holder，不覆盖4BC |
| Effect 44C—44F | 45／33 | 72／29 | 本类邻接别名63345A只处理444的4B关系槽；其余命中均非Effect |

主要剔除链（地址均VA）：

- **Knife尾字**：`5CC260/5CC340/5CC580`属于Text（vtable B1410C，COL B434E0，TD BD2030），其518颜色copy覆盖51C。`9480E0`是Laser Init（vtable B2D81C）；`6450F0 → 7BF0D0`及`7A2080/9C7CF0 → 947E60`写的是Spawn Type7返回的Laser；`7C1510`有Laser RTTI；`9C5270 → 9C83C0 → 947E60`同样为Laser。`94C730`经`94C940/954310`接到Laser Update950B90／Interpolate956B50。`90335D`是栈cookie。`967E40`是XML颜色配置，`9A4F00/9A6420/9A9FE0/7A7D30/993780`处理GameStatePlayer，`9AF990/9B0A50`为UI Sprite，A4/A5/A7区相关候选是Theora/Vorbis编解码器对象／数组。
- **Slot尾字**：`5CCDF0`唯一caller `5F0452`的对象来自`5F03C0`的Pickup RTTI转换；`610CB0`写入前明确目标Type5；`6DD7C0/6E0650/6E5690/6EA0A0`是Pickup虚方法；`76A8E0/79C930/7CEF60`写Spawn Type5返回对象。`674190`是Tear ResetSpriteScale（内嵌ANM2）。`945A90`直接设置Laser vtable；`949840/94A360/94C490`经同一Laser Update／Interpolate链使用本类几何。其余为配置、保存结构或栈，不是Slot。
- **Effect末字**：`606B80/606BA0`在`851B1E/851B23/851B28`注册Laser Radius，邻接IsCircleLaser／SetMaxDistance。`798B90/7A5380/940110`的4BC写在Spawn Type7及Laser RTTI之后；`9C8730`直接创建Type7。`939C70/93A330/93AAF0`命中Knife本身或其生成的Laser。`9418D0`中的`943C45 [esi+515]`／`943C56 [esi+4BC]`之前，`943BC0`已用RTDynamicCast返回值重赋ESI，不能归入口Knife，更不能归Effect。`74E9A0`收到Player+20C0或栈结构；`8BC8C0`收到大型owner+E650；`8FA880/8FAC20/8FBE00`收到大型owner+21498；`80E2AC`是栈movups。
- **Effect末字的LEA**：`5CCF30/5CCFD0/5CD070/6044A0`取得`this+4B4`。`606310`只访问holder的`this[0]/this[1]`，删除析构只读`[holder+4]`再走释放回调；holder止于4BB。`662*/672*/675*`是Tear的ANM2+4AC；`6A0F90`经vtable B18118确认Bomb Render；`8C92C0/8C9650`是Lobby/Seed UI Sprite；其余保存、字符串、UI或编解码器别名也不属于Effect。未调用或复用这些原版在线相关候选。
- **Effect内部44C**：`431500/606AC0`为Bomb RadiusMultiplier访问器；`4326E0/670200`写Spawn Type4或Bomb RTTI结果；`674A90/9D4C50/9DA100`经`798B90/9C7B10`创建Bomb。真实Room save `7E51D0`读取44C位于Type4分支，restore `7E5730`写44C位于case4；Effect分支没有该访问。`9B8B10/9B8FB0`为ISAACNGSAVE02R存档结构。Knife／Laser方法访问各自字段；A5/A7候选为编解码器数组。
- **Effect内部44C的LEA**：`63345A`只取444关系槽登记4B；`606A40`返回43C但唯一caller `61A86D`仅读两字，止于443。Player的41C ANM2中，动画态恰位于44C，涉及`5B3780/773990/783680/784A60/7A33B0/7B66F0`；`6DF890`是Pickup价格Sprite；其余Tear、GameStatePlayer、字符串与编解码器别名均非Effect。`8DE400/8DE6F0`直接设置NetInputDevice虚表，其后续同偏移方法据此排除，不复用其代码或状态。

上述是有界静态消费者分析，不宣称二进制能恢复源级成员声明，也不宣称任意间接调用／任意指针算术都已形式化穷尽。四类末成员结束位置51C／54C／474／4BC均在下一个8B边界前4B，且sizeof分别到520／550／478／4C0；结合真实消费者排除，支持尾部填充的静态推断，不能仅靠“向8取整”升级已核。Effect内部44C缺少直接源级聚合体声明，必须单独保留其证据边界。

### 6.4 Projectile末字：91个候选函数的主审归组

`+474—477`的235条MEM候选涉及91个函数，按实际被解引用对象归组，而非按所在函数名归组：

- **Effect访问器及本类方法**：`431540/5775B0`为公开Rotation setter/getter，`851118/85111D/851122`将两者和字符串Rotation一起注册；`5CD070/5D35D0/600E60`由Effect vtable B14D00核定。`604E40`由Effect的`5D1960/5D35D0`调用。
- **生成Effect后设置Rotation**：`43C7B0/43F4D0/440340/442DE0/4476B0/44A450/44A8A0/456FB0/45BC50/468C70/470E80/481710/48EC60/4931B0/4964B0/49A470/4A9790/4AABE0/4AD4B0/4C8580/4CAFB0/4E13A0/4EF200/4F18B0/4F3D10/5011B0/50BC00/50E050/50EFA0/510360/515AF0/52A600/530AE0/54FE20/551E60/55FD10/56C110/56F1E0/58A640/595C90/599250/59FD00/5A9C60/5B16E0/5C9640/5CA560/63D890/677940/69DB30/6AE630/6AFA80/6D6420/6FC780/70A580/70CD70/773990/7AFE00/7CAEC0/7EDA40/7EE350/9D2B70/9DFF60`。逐处目标来自`424340/6FA210(Type=1000)`或明确Effect RTTI。变体包括1、2、15、30、44/46、47、59、72、83、142、143、148、150、178、188等；不把它们视为Projectile尾部。`470E80`需看原始指令：`473DD2`创建Type1000，`473DD8`转换Effect，`473DE5`重赋ESI，最后`473E86`才写ESI+474。
- **内联Spawn路径**：`5A85A0/64BC20/6C6110`先把Type参数设为1000，经过`84AE90`原生pre-spawn callback，再由`6EC540`从对应池取对象，写其+474。目标不是入口NPC／Familiar／Projectile。支持范围仍不含任意第三方gameplay mod改写Type；这里不声称回调可把任意类型改为Effect后仍安全。
- **回调上下文不是Projectile**：`577910`确实先生成Type9，但随后`6047F0`生成Type1000／variant119计时Effect，并把其返回值存入a4；+474/+478写入此Effect，`577690`回调读同一Effect，再经Parent影响Type9。不能因出现Projectile创建就把所有后续字段归它。
- **明确不同类型／存储**：`662F50/663F90/670200`为Tear自身的标志字段；`93A330`为Knife；`94A360/94C940/954310`为Laser；`7A5660/7A7D30/9A4F00`为GameStatePlayer；`9B8B10/9B9840`为存档结构；`903730 → 900D20`为进度保存对象；`682240`访问管理器集合中的文本／UI容器；`80C980`为栈访问；`8DE400/8DE6F0`为NetInputDevice。仅静态排除原版网络对象，不调用或复用它们。
- **按动态类型分支**：`6D69C0`先检查关系目标`[target+28]==1000`，再读取+474；不是Projectile。

邻近LEA操作数扫描为77条／38函数；另以渲染汇编检索非栈`+464..478`得到60条，二者是不同筛选口径。与真实Projectile相关的两处是`6552C7/655467`取得this+468：`9E5DD0`只读两个f32，`9E5F20`只复制两个dword，范围止于46F，既不覆盖470的回调指针，也不覆盖474。其他LEA分别落在Tear+478容器、Bomb+464 ANM2、Knife+468成员、Laser+464 ANM2、UI文字／Sprite（`42C0E0/8B25D0/9AE290/9AEF70/9AFFB0/9B0A50/9B0FD0`）、存档结构、NetInputDevice、编解码器（`A52860/A56370`）及栈上。

另复读原Projectile Update中的间接setter：`651069`创建Type1000，`65107D mov edi,eax`已改变EDI，`6510F7`调用的`431540`写Effect Rotation。此处撤回子智能体中间消息“Projectile+474被setter写入”的误判。结论：在已核目标程序的上述直接访问、成员别名及本类虚方法链中，没有发现Projectile末四字节的语义消费者；结合470为4B回调和478对象上界，支持尾部填充推断，不把这一推断写成源级声明或运行时观察。

### 6.5 Effect 41C附近取址的主审

主侧独立decode全映像LEA，复现437条／192函数。实际位移仅五种：410（56条）、414（292条）、418（24条）、41C（37条）、420（28条）。其中栈寻址亦保留；渲染汇编搜索的418条是另一口径，不能混算成更少的操作数候选。主要来源及边界：

- 414大量来自NPC寻路：`6B455D`明确安装NPC vtable，`6B4556/6B4563`构造414的Pathfinder；例如`43AB99/43ABA6`传给`7D99F0`。这些NPC AI函数的寻路子对象不能套到Effect的FallingSpeed。
- 418包括Familiar历史数组：`606F38`取址后，`606F3E`明确安装Familiar vtable，60次8B循环写到5F7；`62A8E4`按8B元素写历史位置。`51BDB0`中的418也是寻路对象内部数据。Text方法`5CC340/5CC580`以及`6FF270`的Type9001生成＋Text RTTI后，将418作为字符缓冲。
- 410包括Knife的BitSet128、Pickup辅助ANM2以及Projectile／Bomb标量。`654A13/657209/65734E`为Projectile Height；`69E132`为Bomb计时标量。`7BA3E0`取418做i32最大值，只访问四字节，止于41B。
- 41C主要是Player辅助ANM2：`74DC70/74F260/773990/77BD40/783680/78BBF0/78D000/79BDD0/79CBE0/79E3E0/7A3340/7A33B0/7A71C0/7C3C40/7CFC60/7CFFA0`；`825B79`亦经玩家集合得到Player对象后调用ANM2方法。另有UI字符串`42C0E0`、保存子对象和栈，不能归Effect。
- 实际Effect传播helper `5D33AC`取得420，`5D33B2`读取`[eax+4]`，其后沿420/424等非负成员偏移计算向量，不回退到41C。取420本身不说明会触及前四字节。

**反证限制**：SDK `BitSet128.zhl`实际只声明四个u32（ll/lh/hl/hh）；主审撤回“由SDK BitSet128直接证明8B对齐”的候选推理。420的起始位置及相邻f32只能支持对齐猜测，不能单独证明41C为padding。上述取址分析没有找到Effect41C的真实消费者，仍不是源级聚合声明或所有任意指针算术的形式化证明。

补充核对：官方`Entity.zhl:196`声明基类`unsigned long long _flags :0x168`，确有64位成员，可进一步支持尾部8B对齐的推断；它不是BitSet128。SDK未提供目标原程序的完整成员／packing声明，且此前大小声明本身与J273不一致，故这条资料仍不能单独决定六段字节的归属，尤其不能解释Effect两个内部空白。

### 6.6 Effect 41C的访问器与Weapon持有实体：主审补充

主审额外复读了全局候选中的访问器、生成helper及Weapon方法。Weapon vtable只能识别调用者，不能据此跳过其解引用的实体；具体排除依据为：

- `8514C0/8514C5/8514CA`把`844960/844980`绑定Projectile Damage；`851B91/851B96/851B9B`把`945DC0/844980`绑定Laser Angle。同偏移getter可以由不同类型复用，不能套入Effect。
- `851DBB/851DC0/851DC5`把16B访问器`5CCB80/844490`绑定Knife TearFlags。`634DC0/7A51E0`先Spawn Type8并转Knife RTTI再写410宽16B；`6448A0`同样先创建Type8；`9C8A00`还显式检查返回实体Type8。
- `4B5F70`从NPC持有集合取实体，写41C前明确检查Type4；`4B6950`使用其返回值。`49C170`三处直接创建Type4。`7C87B0`在`v20[10]==4`分支写41C，入口Manager不等于目标类型。
- `9D4C50/9DA100`经`9C7B10`取得Bomb；后者走`798B90`或直接Spawn Type4。`9D28A0`先以Type7筛集合再读取41C。
- `9D0F10`的Weapon+48h持有对象来自`9C8320 → 9C7CF0 → 947E60`的Laser创建链；`9DFF60`的Weapon+50h亦来自`947E60`。其Angle访问不属于同函数另持有的Effect。
- `9D5A20`的Weapon+44h经`9C8810 → 7A5080`取得Knife；`9DB350`同槽经`9C8A00`创建Knife。`9D6AE0`写入这些Knife的410—41F flags。
- `9CC070`的Weapon+44h起四个槽在使用前检查实体Type8，不符则经`9C88A0 → 7A51E0`重建；宽写410—41F作用于Knife。该函数另一个41C写入分支来自`9C7B10`的Bomb，不能用整函数一个类型覆盖两条路径。

上述补充直接支持“这些候选不是Effect41C”的结论，不据此把尚未证明的字节升级为填充。

### 6.7 补查ADD／SUB形成地址

除内存位移与LEA外，主侧检索六个目标4B范围常量的寄存器ADD／SUB，共11处。`421DB3`为配置／UI结构中的字符串成员（由`9319D0`取配置文本，再以`408660`复制）；`5690D5`只是NPC阈值公式`1620*n+1140`中的数值加法，不是指针。`5FE384`取Effect自身474 Rotation，不能归Projectile。`78376D/783873`为Player ANM2；`9A870C/9A88F0/9A9364/9AB694/9AB890/9ABECB`为GameStatePlayer复制／恢复中的成员地址。此次补查亦未确认目标六段的语义消费者，但不扩大成任意多步地址运算的完备性声明。

## 7. 最后七个helper的结果与主审边界

本节由主侧整理同一Luna max子智能体的续研消息，并复读下列关键伪码和原始调用参数。不把调用者类型、反编译形参名或相同位移代替实际目标对象的来源证明。

| helper | 已核对象来源／访问 | 结论与限制 |
| --- | --- | --- |
| 4D22B0／4D2980 | 函数内部将Type参数设为853，经84AE90→6EC540内联Spawn取得新NPC，末尾写该新对象的[262]/[263]归一化速度分量 | 排除为Effect41C；中间消息“Type1000／999”错误，按853纠正。仍适用不支持第三方gameplay回调任意改Type的边界 |
| 6069A0／673650 | 唯一调用位于6170F0。源v706来自Type2分支的Entity→Entity_Tear RTTI；目标v375来自424340(Type2)后再次转换为Tear。61B8E4调用getter，61B8EE调用setter | 两端均为Tear。中间消息“Familiar侧+41C”把caller类型混作源类型，撤回 |
| 93A4E0 | 将源+410—41F的16B复制到目标。三个caller先Spawn Type8；93A6F5/6F6与93A96A/96B压入源ESI、令ECX=新刀EDI；944B2C/2D压入源EDI、令ECX=新刀ESI | 已核三个写入目标为Knife，不根据伪码中的Familiar等形参标签认定源类型，也不把目标证明外推为所有源读的完整类型证明 |
| 8EC3B0 | 唯一caller8E75A0在8E7DD0传入自身；同一对象还向Lobby操作传入本体+6416，邻接current_stage／current_stage_type处理 | 来自大型Lobby相关对象链，未发现Effect对象来源；不把它命名为已核NetInputDevice，不调用或复用原版在线代码 |
| 4B26A0 | 六个调用均在4B28A0；两处参数直接来自Type4 Spawn（variant0／15），其余四处来自NPC持有deque，其中两处经4472A0索引，另两处直接取元素 | 两处目标Bomb已核；四处deque元素的全部producer仍未追清。4472A0只证明deque寻址，不能证明元素类型。这部分保持未核，不能宣称Effect41C全部间接路径已排除 |

### 7.1 随后的deque来源补证

上表4B26A0的四路未核项随后已补证，保留上表作为当时进度。子智能体定位生产链，主侧复读关键指令：`4472A0`的this为`NPC+BF0`，其base／capacity／index位于BF4／BF8／BFC；`4B328D/4B32BA`通过索引0／1取元素，`4B42DC/4B4408`直接展开同一寻址。case0先`4B2A2C call447390(2)`，再在`4B2A48/4B2A6F`清零两个槽。

生产helper `4B2260`由`4B28A0`的`4B2D8B`调用；三个槽赋值在`4B24C8/4B2522/4B25AC`调用`4B14E0`，随后撤销旧关系、写槽并反向登记新关系。`4B14E0`被IDA误命名为`___std_stacktrace_to_string@16_1`，实际是Bomb创建helper：`4B16BA push4`、`4B16BC call424340`、`4B16C1 mov esi,eax`，最终返回新实体。主审纠正子智能体把4B16C1称为Spawn指令的地址偏差。

因此，这四处在已核的原版队列初始化和生产／消费链内也属于Bomb，4B26A0的六个直接caller均未发现Effect目标。这个具体候选缺口已补上；不将它扩大为任意内存损坏、任意第三方修改或全程序间接指针分析的完备证明。

子智能体另报告：Effect的26个虚槽中未发现41C访问；从这些虚槽沿直接调用最多三层得到1767个唯一函数，其中命中41C的9个helper为5CCB80、677940、772300、947E60、6A38F0、7A5080、7C87B0、93A330、7A7D30。这个深度受限的调用图只用于排序候选，不是全程序调用闭包；主侧采用第6节已经逐目标复读的排除链，不将统计本身作为无消费者证明。

## 8. 版本27的最终采用与未完成部分

主侧采用已核的四类sizeof、DepthOffset单份映射、Knife六字段用途、Slot／Projectile本地指针，以及Effect传播／生成字段归属。六段共24B仍为UNKNOWN：Knife51C、Slot54C、Projectile474、Effect41C／44C／4BC各4B。四个尾字支持填充假设，内部空白依据更弱；没有将阴性搜索改写为填充证明。4B26A0队列元素经7.1实际生产链补证为Bomb，不再保留为这个helper的待查项。

本轮没有实现“原前四项完全闭合”。第1项工作量／研究次序与第3项DepthOffset已闭合；第2、4项的已核部分已落地，剩余字节与研究限制见[落地说明](implementation-4-first-four.md)。完整性仍为false；这既不是新增gate，也不禁止继续其他已授权工作。普通版本27编译／链接成功，未运行测试、游戏或注入；第15步保持唯一整体验收。
