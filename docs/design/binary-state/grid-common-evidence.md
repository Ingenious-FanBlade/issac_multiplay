# 第 6 步：GridEntity 公共字节归属

> **版本50同步职责复核取代部分上线决定**：[重新核对](grid-common-synchronization-review.md)撤销CollisionClass原值传输，改发按实例最终WaterClipFlags；Grid主Sprite使用专用视图，排除默认动画名／待加载图层路径，补入Dataminer地址派生的最终两位纹理选择。冠军色载体只保留前四float，公共最小状态171→142B，根UNKNOWN仍17B。本文版本48的字节事实、当时决定、失败与编译记录保持历史事实；以新复核和生成表决定当前传输。

日期：2026-11-10（Asia/Singapore）。范围仅为公共前缀 `[0,0x154)` 及内嵌 `GridEntityDesc`，不推进子类字节归属。目标为 J273，IDA imagebase `0x400000`；下文地址为 VA，RVA 统一减去 `0x400000`。本轮只做文件阅读、静态反编译、原始指令核对、布局生成与产品纯编译，没有测试、游戏启动、注入或部署，没有修改 IDB。

正式源为 [`mod/binding/state/grid.json`](../../../mod/binding/state/grid.json)，布局版本 **48**：`GridEntityDesc=198`、`GridEntity=199`。两张单行表由普通构建生成，见 [Desc](layouts/06-grid/GridEntityDesc.md) 和 [Grid 公共部分](layouts/06-grid/GridEntity.md)。后者明确为 `base_prefix`，不能拿来识别或复制整个派生对象。

## 结论与证据边界

340B 公共前缀的每个字节均已登记归属。仍有 **17B UNKNOWN**：描述间隙 3B，加现行 ANM2 递归未知 14B；不能称“全部字节语义已知”或“完整类型”。Sprite 复用现行基础布局，SDK 声明等级不因嵌入 Grid 而升级。

公共 G0 仅发送 24B：描述的 type、variant、state、spawn_seed 四项共 16B，加 grid_index 与 collision_class 各 4B。G1/G2/G3 复用 ANM2 的外观、动画与图层投影，公共最小线状态 **171B**，不含记录头。SpawnSeed 有真实 Light 消费者；其余来源初始化／玩法元数据不因旧目录曾发送而继续同步。这取代第 6 步初稿“desc 八项都是状态、spawn_frame 进入 G0”的设计，旧运行路径的改换仍在第 10/13/14 步进行。

## 公共范围逐段目录

区间左闭右开，按对象绝对偏移排列。这里用于解释证据；规范要求的两张单行布局表以生成结果为准。

| 原生区间 | 类型／名称 | 归属及依据 |
| --- | --- | --- |
| `[0x000,0x004)` | vptr | **本地**。构造 `0x704FC7` 写 `GridEntity` 虚表 `0xB1975C`；Factory 装派生表。只用于本地分派／RTTI 识别，不传地址或覆写虚表。 |
| `[0x004,0x008)` | i32 desc.type | **状态**。构造 `0x704FD0` 清零，Factory `0x706636` 写请求 type；Light `0x801C24` 直接取类型。用于结构类型和 Room 绘制分支，type／RTTI 变化仍走结构事务。 |
| `[0x008,0x00C)` | i32 desc.variant | **状态**。构造 `0x704FD7` 清零。Decoration Render `0x973190` 用 variant==1000 隐藏；Poop `0x710130` 区分 7—10／9；Rock `0x7146E0` 用于碎块帧选择；PressurePlate `0x95E140` 的 variant==2 绘制计时文字。Light 也有变体分支。 |
| `[0x00C,0x010)` | i32 desc.state | **状态**。构造 `0x704FDE` 清零。Rock `0x7146E0` 区分 1/4 与碎块态 2；Fire `0x67B620` 以 state<4 决定 ember 绘制；Poop／Rock WaterClip `0x711620/0x718650` 依赖 state。Room 的下层 pass／Light 也消费。 |
| `[0x010,0x014)` | i32 desc.spawn_count | **权威**。构造 `0x704FE5` 清零，公共 Init `0x70508D` 加一；Fire Init `0x67A45F` 检查该次数<=1，再在随机分支选择最终 variant。Client 不重演 Init，也不靠来源次数建立自己的 owner。 |
| `[0x014,0x018)` | u32 desc.spawn_seed | **状态**。构造 `0x704FEC` 写默认 `0x16A9DE81`，Init `0x705079` 写 Seed 参数；Light 的 `0x801BFF` 读取它，`0x801C07` 读 `Game+0x25A70`，`0x801C0D` 相加生成栈上绘制 RNG。不能把全部种子一律判为 Host-only。 |
| `[0x018,0x01C)` | i32 desc.var_data | **权威**。构造 `0x704FF3` 清零；hurt_func `0x7055D6/0x7055EF` 在奖励／随机逻辑内读取并递增，TrapDoor Update `0x71A7B0` 等检查。Client 只接收最终姿态／state／成员，不运行这些玩法分支。 |
| `[0x01C,0x01D)` | bool desc.initialized | **权威**。构造 `0x704FFA` 写 0，公共 Init 在派生 InitSubclass 正常返回后 `0x705090` 写 1。可作为 Host 读取资格，不能把它当作 Client 已构造、已装资源或已发布的见证。 |
| `[0x01D,0x020)` | bytes[3] | **UNKNOWN**。SDK 的 bool 与下一个 u32 之间有间隙；构造仅写 +1C 一个字节，下一条从 +20 写四字节。没有填充证明，不读取／复制；保存体的整段复制也不赋予语义。 |
| `[0x020,0x024)` | u32 desc.variable_seed | **权威**。构造 `0x704FFE` 写默认种子。hurt_func 的 `0x7056F1` 及后续奖励臂把对象 RNG 的状态写到 +20；它与 Light 实际读取的 +14 不同。 |
| `[0x024,0x028)` | u32 grid_index | **状态**。公共构造不写；Factory `0x706639` 写 index。GetPosition `0x7050B0`、GetRenderPosition `0x705110`、Textured Pit `0x70DFC0` 用它算位置。范围 0—447；不是独立网络身份，必须与房间／成员关系吻合。 |
| `[0x028,0x02C)` | u32 spawn_frame | **权威**。Init `0x705094/0x70509A` 将当前 GameFrame 写入。TrapDoor Update `0x71AC95/0x71AC98` 读取它并比较 `Room+0x11F0`；已核绘制根不以它计算展示。Client 不执行 Update 或将其改成本地帧。 |
| `[0x02C,0x03C)` | RNG 四个 u32 | **权威**。构造 `0x705005` 写 +30/+34 为 5/9，`0x70500A` 写 +2C 为 `0xAA17414F`，`0x705011` 写 +38 为 7。hurt_func `0x7055CE` 将对象+2C 传给 RNG Next；玩法状态不上线。 |
| `[0x03C,0x040)` | i32 collision_class | **状态**。Init `0x70507E` 清零后调用派生 InitSubclass，不能把初值当最终值。Lock WaterClip `0x70CCE0` 读取该字段是否为 0，返回 38／57，实际影响水面裁剪展示。 |
| `[0x040,0x154)` | ANM2 sprite | **递归归属**。构造 `0x704FCD` 取对象+40，`0x705014` 调 ANM2 ctor `0x402B00`；Free `0x705020` 析构同一对象。外观／动画／图层分别投影，资源地址、loaded、事件位、UNKNOWN 沿用现行 ANM2 合同。 |

描述本身的大小为 32B：实际状态 16B、权威 13B、未知 3B；根对象不重复发送描述别名。Grid 的原生 Position／RenderPosition 没有独立存储区，不能为 getter 再造两个原生 Vector 字段。

## 实际消费者与职责核对

逐项回读 18 张 Grid 虚表（根＋17 个派生）的 slot 3／6／8／9，并检查以下唯一原生绘制函数；这只是公共字段的消费者分析，没有给子类尾部建立正式布局。

| 用途 | VA | 对公共部分的意义 |
| --- | --- | --- |
| Decoration Render | `0x973190` | variant、grid_index、主 Sprite；Room floor color 临时覆盖主 Sprite 颜色。 |
| Rock Render／Pillar row | `0x7146E0`／`0x7149E0` | type、variant、state、grid_index、主 Sprite；碎块还依赖子类容器／动画，后续另核。 |
| Pit／Textured Pit | `0x70DE30`／`0x70DFC0` | grid_index、主 Sprite 及资源层；本地换 Image／visible 的临时绘制必须恢复。 |
| Lock／Teleporter Render | `0x70CB80` | grid_index 和主 Sprite，消费 overlay 顺序／加载准备结果。 |
| Fire Render | `0x67B620` | grid_index、主 Sprite、state；其他两份 Sprite／ember 留待子类。 |
| Poop Render | `0x710130` | variant、grid_index、主 Sprite。 |
| Wall Render | `0x71B4E0` | grid_index、主 Sprite；特定房间 type==16 时取 Room floor color。 |
| Spikes／Web／TNT／TrapDoor／Stairs／Gravity Render | `0x719270` | grid_index、主 Sprite，并从 Room floor color 覆盖颜色。 |
| PressurePlate Render | `0x95E140` | variant、grid_index、主 Sprite；variant==2 的文字还读 Room+0x723C 计时及 Host GameFrame 闪烁，属于第 7/8 步展示上下文，不能声称仅主 Sprite 已覆盖。 |
| Statue Render | `0x4291D0` | 原生空函数。不能据此宣称 Statue 全部展示已完成，其他通路仍属子类／Room 工作。 |
| Begin／End batch | `0x7060A0`／`0x7060B0` | 操作对象+40 的 ANM2，不访问对象 RNG 或来源 Init 元数据。 |
| WaterClip | `0x705250/0x70F120/0x70C360/0x95E940` | 原生固定值分别 57／38／0／25；相应动态类选择留给实际 RTTI。 |
| WaterClip | `0x70CCE0/0x711620/0x718650` | Lock 依赖 collision，Poop／Rock 依赖 state。 |
| Room::RenderGridLight | `0x801BE0` | type／variant／state／SpawnSeed／grid_index 与主 Sprite 层、子类及 Room／Run 上下文；不推进对象+2C 的 RNG。 |

Light 的 TrapDoor 特定房间分支用栈上 `SpawnSeed+GameFrame` RNG 选择闪动位移。它和第 8 步已有两份共享绘制 RNG、Grid 对象内 RNG 分别管理；Client 展示时钟必须来自最近已应用 world，不能用本地墙钟／Authority tick／Client GameFrame 替换。没有这些上下文时报告依赖缺口，不能恢复 Client Update 补算。

GetPosition `0x7050B0` 与 Room GetGridPosition `0x8087A0` 都是 `x=40*(index%width)+40`、`y=40*(index/width)+120`。GetRenderPosition `0x705110` 在此基础上使用屏幕宽高、0.65 比例及 pixel scale 做四舍五入投影；它使用 Client 本地视口，不安装 Host 屏幕坐标。IsBreakableRock `0x7060C0` 只读 type，接受 2/4/5/6/22/25/26/27，排除 3/24；它不是 Client 破坏格子的授权。

这些 Host-only 决定依据当前 Client 不运行玩法 Init／Update／伤害／奖励的设计及上述具体消费者；不是全程序别名形式化证明，也不扩展到任意第三方 Mod。后续子类研究发现新正面展示消费者时应带点位修订归属，不能据旧目录恢复整段发送。

## 未知区与公共区外依赖

17B 未知的对象绝对位置：`[1D,20)`、`[85,88)`、`[A5,A8)`、`[B1,B4)`、`[142,144)`、`[148,149)`、`[14A,14C)`。后六段来自 ANM2／两个 AnimationState，大小合计 14B。未知不复制，不因本轮没有测试而假称稳定或填充；第 15 步 E27 在实际覆盖场景观察。

官方 `repentogon/Patches/GridEntityPlus.cpp/.h` 把可空 `waterClipInfoFlagsOverride` 保存在 **448 项外部旁表**，按 grid_index 取项；Factory／Init 重置旁表。它没有占用 Grid 公共区的任何字节。`LuaGridEntity.cpp` 的设置／重置 API 操作此旁表，官方 WaterClip hooks 在原生结果后覆盖。因而单靠公共 state／collision 不能重建完整 REP GetWaterClipFlags；正式旁表布局／owner 仍归第 6 步后续，不能把空覆盖当成已知 0，或声称本轮已实现查询执行器。

索引上限来自 `Room+0x24` 的 448 槽。8 个门槽、同格门／装饰的 door_aliases、房间宽度与颜色等不嵌入公共 0x154；分别由现有身份／第 7 步上下文承接。Grid 类型总数依据 Factory 与 RTTI 是 **根＋17 个派生，共 18 类**，原第 6 步“18 个子类”的措辞有误，本轮更正计数，不新增／遗漏类型。

## 本轮落地与后续接线

- `grid.json` 建立两份正式源，公共块只占 G0—G3；G4/G5 留给真实子类声明，不在根上空占后再覆盖父块。
- 原生表／复制表／线上元数据／数字分派与成员说明由既有生成器生成，没有另写测试或校验脚本。旧 room-grid 应用目录的 119 个成员逐行回映射仍在第 6 步后续，`public_owners=[]` 明确不宣称全域覆盖；本轮只登记已有公共成员的具体处置。
- `grid-current-window` 是第 10 步待接线的读取资格：主线程普通完整窗口，来源房间／实际 RTTI／membership 与 index 关系稳定，沿用来源 Init 资格规则；布局字符串本身不创造见证。后台只接收自有纯值缓冲，不交付原生对象地址。
- 第 13/14 步按新源替换旧 `Common/CommonMatches` 的八项描述／spawn_frame 写入和旧相等要求，使用本地构造／资源 owner；不能为匹配旧 JSON 回读把已排除字段重新送回。
- 第 15 步在原 Grid 场景、E27、E28 及 malformed／恢复覆盖中验收实际状态与画面、UNKNOWN、索引边界、无玩法重演与借用恢复；本轮不提前运行。

普通 Release／Win32／v143 产品构建已完成：`ISAACCS_BUILD_E2E_TOOLS=OFF`，生成器由 CMake 普通构建执行，`isaac_cs_state` 及完整 `zhlIsaacCS.dll` 编译／链接成功。首次 `/parallel 4` 构建停在 CMake 重新配置后无后续输出，已中止；改用 MSBuild `/m:1 /nr:false /p:BuildInParallel=false` 后完成。没有因这次构建停滞启动程序验证或另开测试。官方 SDK 的 `CMAKE_REPENTOGON_VERSION undefined` 提示以及 RapidJSON 枚举／SDK 名称遮蔽编译警告仍存在，没有改动 SDK。

纯编译／链接不等于运行验收。17B UNKNOWN、所有子类、Plus 正式 owner、Room／Run 展示来源及实际接线仍明确保留；没有新增单元测试、运行游戏／E2E／校验脚本或留存单独 artifact，也没有提交。
