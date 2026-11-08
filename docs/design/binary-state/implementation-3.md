# 第 3 步实现说明

日期：2026-11-08（Asia/Singapore）。对应[第 3 步](3-entity-common.md)。本步只进行资料分析、静态反编译、布局生成和纯编译；没有测试、游戏启动、注入或部署。整个重构仍在第 15 步统一整体验收。

## 布局和生成表

`mod/binding/state/entity.json` 接管 Entity 公共部分 `[0,0x410)`，并声明独立的 EntityPlus 公共前缀。Entity 从原来的 `entity-effect.json` 生成链种子中移出，Effect 及其变体继续继承这一份公共事实，不复制第二套 Entity 字段。

Entity 的 E0—E9 固定为分类、运动、生命／碰撞、状态效果、外观、动画、图层、颜色参数、引用和 REPENTOGON 扩展。主 Sprite 只声明一次 `Entity+0x48` 的 ANM2，通过子块投影分别进入 E4／E5／E6。E1／E4／E5 携带普通实体使用 world、Player 使用 body 的明确元数据，供第 5／10／11 步接线。布局和逐段未知统计以[生成表](layouts/Entity.md)为准，未知不复制，也不表示类型完整。

最终布局版本为 **12**。版本 12 来自第 3 步提交后的审阅整理：Entity 的 66 条 `field` 回映射 `reason` 按实际字段重写，字节归属、未知计数和线上布局不变；同时生成器开始校验标量／`bytes[N]` 标签宽度，布局版本收紧为 u16（与第 9 步帧头一致），`FindLayout` 改为生成的直接索引。Entity 生成 77 个字段／投影和 23 个直接复制区间，最小状态 634B；原生范围仍有 335B 未知，其中 14B 来自内嵌 ANM2。EntityPlus 公共前缀另有 5B 未知；容器元素的未知按各自布局保留。字段表以数字布局号／索引查询，直接复制段按 native 和 wire 均相邻的条件合并，读取资格不同即断开。末轮主审将 +0x3D0 控制字保守改为未知，并由原始回写指令确认 +0x314/+0x318 的暂停保存位置状态；它与 +0x344/+0x348 插值历史位置分开。

生成表同时保留形态字段标记和读取资格。`removed_by_factory +0x177` 使用专门的 `current-entity-byte-0x177` 资格；生成器不会把它与资格不同的字段合并。第 10 步必须提供规范第 11 节所要求的真实见证，不能从布局声明推导读取授权。

新的数字 Client 表由同一源生成。当前运行的旧 JSON／字符串路径仍保留；第 13 步接入原生执行器，第 14 步统一退役旧路径。这些表不能被当成新同步已经运行的证明。

## 帧、引用和扩展

FrameCount 发送 Host 帧空间中的 `+0x328` 基准，不发送逐帧增长的 FrameCount。`SpawnGridIndex +0x32C` 是另一个独立状态，不能当成帧字段。插值暂存位置 `+0x344/+0x348` 保留，不能用当前 position 覆盖历史。`+0x175` 虽有写 0／1 的指令，读取者和完整用途未闭合，主审没有把它升级成可复制 bool，仍列未知。屏幕坐标缓存 `+0x1A4/+0x1A8` 由本地屏幕参数和已核原生 Render 重新生成。

状态倒计时使用显式的条件锚点合同：两个 u32 分别保存 mode 和 value；实际按 Host 全局帧递减的正倒计时保存到期帧，暂停／零／负哨兵保存原计数。`anchors.h` 提供纯值换算。字段 owner 必须提供已核的实际递减条件，未知条件报缺口，不能一律设为运行。具体计时域与条件见[静态证据](entity-common-evidence.md)，实际采集和展示写入在第 10／13 步接线。

另有两类不能套普通倒计时规则：毒／火伤害相位 `+0x2E0/+0x2E4` 在对应原生分支递增，活动时发送 `game_frame-counter` 基准；ColorParams 的 Lifespan `+0x34` 无条件有符号递减，负值也继续减，活动时发送 `game_frame+counter`。二者使用 `SignedCounterAnchor` 按模 2^32 还原，不把负值当停走哨兵，也不在零夹限。ColorParams Duration `+0x30` 只作淡出分母，保留直接状态。颜色元素删除只按 Host 容器变化安装，Client 不重演删除／伤害规则。原生路径停走时才编码冻结原值；无法证明活动时钟关系时必须报缺口。

原生索引只投影为 Host 绘制排序值，不写 Client 本地 index，也不作为网络身份。EntityPtr 经当前成员表转换为 u16，并由引用 owner 维护反向关系。反向观察者树的本地根地址不发送，邻接控制字 +0x3D0 尚未证明计数语义，继续列为未知。

EntityPlus 是伴随来源，不是 Entity 内嵌对象。正式读取顺序是两半地址 → 已存在的 EntityPlusHolder → data；缺失时报缺口，前后复读来源并检查 ReadWindow。`GetEntityPlus` 的缺失分支会分配、打印，不能用在只读采集。

`entity_extension.*` 只读取 engaged 字节，再按需读取有效 payload。飞行覆盖编码为 -1／0／1；水面覆盖编码为 0（Null）或 `(1<<32)|u32`，有效零与 Null 不混淆。解码拒绝其他编码，原生 optional 交第 13 步 owner 赋值，不 memcpy 容器或 optional 内存。当前 v143／Win32 的 optional 大小、公开成员位置和 engaged 偏移随普通 SDK 对象编译做 static_assert；未知对齐字节仍不复制。

## 主智能体审阅

必要逆向由用户指定的 GPT-6 Luna max 子智能体完成，主智能体独立读取官方源码和 IDA 原始指令，按结论逐项采用或更正。没有以子智能体身份代替事实核对。

本轮已直接核对的关键点：

- FrameCount getter：VA `0x6A5295` 读取 `Game+0x25A70`，VA `0x6A529B` 减去 `Entity+0x328`；Init 在 `0x6A4B2B/0x6A4B31` 读帧并写基准。
- SpawnGridIndex：VA `0x6EB980` 读取 `+0x32C`，`0x6EB986` 是 ret；Init `0x6A4B46` 写 -1。研究首报把 ret 位置写成读取指令，已纠正。
- Entity RTTI：vtable VA `0xB18224` 的前项指向 COL `0xB439E4`，COL +0xC 指向 TypeDescriptor `0xBD1244`，装饰名从 descriptor +8 开始，源中的 RVA `0x7D1244` 正确。
- Entity 构造在 `0x6A39BB` 调用 ANM2 构造，接收地址为 Entity +0x48。
- 当前 J273 插值槽为 `0x6AC2A0`：`0x6AC2E8/0x6AC2F1` 写暂存坐标，`0x6AC320` 写 +0x175；Render 槽为 `0x6AC4E0`，`0x6AC646/0x6AC64A` 写屏幕缓存，`0x6AC64F` 调主 Sprite 渲染。
- `HasFullHealth` 在 `0x6A52B0/0x6A52B8` 以 movss／comiss 比较 HP≥MaxHP；`HasMortalDamage` 在 `0x6A8810/0x6A8815` 比较 `pending_damage +0x14` 与 HP。后者是 Host 伤害缓冲，不由 HP 单独派生。
- `GetBossID` 在 `0x7DF5F0` 读取 Entity +0x330 的实际选中配置，非空时在 `0x7DF5FA` 取配置 +0x30，否则返回 0。因此 +0x330 是资源指针，需要资源序号；它与 REP 的 `GetEntityConfigEntity` 按 Type／Variant／SubType 重新查配置是两种语义。
- `GetLastParent 0x6A4010`／`GetLastChild 0x6A4030` 沿引用链返回末端，首边为空时返回 SELF。本地 Wiki 描述为 nil，与 J273 静态 getter 冲突，保留差异供第 15 步对照，不能按 Wiki 描述覆盖原始指令。
- ColorParams 更新的 `0x6AA122/0x6AA12A/0x6AA12E` 读取、无条件减一并写回 Lifespan +0x34，`0x6AA13B` 检查结果零后移除；没有正值门。毒／火相位分别在 `0x6AAE10/0x6AAE17`、`0x6AAEDA/0x6AAEDD` 加一写回，并按旧值 mod 20 判节奏。原先按字段名猜成倒计时的初稿已经更正。
- 原生 WaterClipInfo `0x6B2F70` 已主审反编译。它读取分类、排序层、位置偏移Y、HP、IsFlying，并依赖 Pickup +0x534、Player 道具62查询、Room helper 等；回映射已明确依赖第 4／5／7／8 步的数据，不将 optional override 冒充完整原生公式。pause timer 在 `0x6C0891/0x6C0892` 的正值递减及保存坐标回写也已复读。

历史 tx-03 文档的 `0x6B0EB0/0x6B10F0` 在当前 IDB 不是上述两个函数，不能作为 J273 点位沿用。本轮保留历史记录，采用当前虚表重新定位的结果。FrameCount 注册顺序的首报 getter `0x6A4280` 也已纠正为 `0x6A5290`。

## 编译和实际边界

已完成基础库及完整 `zhlIsaacCS` 的 Release／Win32／v143 编译、链接，`ISAACCS_BUILD_E2E_TOOLS=OFF`。生成器由普通 CMake 构建执行，没有另运行校验脚本。SDK 对象仍输出官方头既有的 `CMAKE_REPENTOGON_VERSION undefined` 编译提示；本次没有修改官方源码。

没有添加或执行单元测试、probe、smoke、E2E 或游戏实验。正常构建输出留在既有 build／out 目录，没有另行归档 artifact。SDK 声明、未知字节、跨域资源／房间依赖及尚未接线的 owner 继续明确保留，不能据纯编译通过宣称功能验收通过。
