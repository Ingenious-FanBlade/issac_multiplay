# 第 4 步：Familiar、Knife、Bomb 实现说明

日期：2026-11-08（Asia/Singapore）。范围是[第 4 步](4-entity-classes.md)的三个子类与已有专用表现，不代表第 4 步全部完成。逆向由用户指定的 GPT-6 Luna max 子智能体执行，主智能体复读当前 IDB 原始指令和官方源码后采用。只做静态分析、源码编辑、布局生成与纯编译；0—14 步不测试，第 15 步统一整体验收。

## 本轮交付与边界

唯一源为 `mod/binding/state/entity-classes.json`，布局版本 **16**。继承现有 Entity 的 E0—E9，子类块从 10 开始，全部属于 world 序列。新布局编号：Familiar 120、Knife 121、Bomb 122，Familiar 的 IsaacsBody／AbyssLocust 臂为 123／124。附属类型编号 16—19。两类派生 Plus 也有原生字节表，E9 收窄到派生类型，公共覆盖只发送一次。

版本 13 是本轮生成初稿，14 纳入主审后的未知区修正，15 补齐两个原生 payload 的真实 RTTI。版本 16 修复用户审阅指出的关系遗漏、计时语义、引用阶段、资源命名、证据等级和集合合计边界；最终生成表和 DLL 均为版本 16。

本轮按 J273 池步长覆盖 `[0,0xF30)`、`[0,0x520)`、`[0,0x9D8)`，没有沿用 Knife SDK 的 0xE78，也没有把构造清零或池尾部当成填充。每段未解释字节都有显式未知条目。生成表中的“完整：否”是实际状态，公开成员回映射齐全不等于字节完整或可玩同步已完成。

运行中的 Host／Client 仍使用旧同步链。第 10／13 步负责把本轮的数字表、换算和具名 owner 接入采集与应用，第 14 步退役旧 Lua／JSON 热路径；本轮不安装 Hook、不部署 DLL、不启动游戏。

## 三类实体

- **Familiar**：Player 是裸借用，独立映射为成员序号；不把 Player setter 的队形／玩法副作用带到 Client。轨道、资源、方向、状态、颜色与内嵌 `+0xDB8` ANM2 分别入块。`+0x5FC` Pathfinder 和 `+0xDB0` Weapon 属于 Host 玩法职责，不发送指针。`+0xED4` 配置转为资源序号。变体 48 的五份 Sprite／四个有符号 selector、变体 231 的两组 Sprite 经 typed shared owner 传送；其他私有 ExtraData 不被冒充为这两种表现。
- **Knife**：只使用 0x520 范围；投掷状态、旋转、路径参数、128 位 TearFlags 分块。REP `EntityKnifePlus::hitboxSource` 进入 E9，Client 必须经 EntityPtr owner 恢复关系。Parent 与 SpawnerEntity 分开，DeadEye Sprite 是进程级全局资源，不能误作每个 Knife 的内嵌字段。
- **Bomb**：Flags 位于 `+0x438`，`+0x420` 是下落加速度。五份 costume ANM2 位于 `+0x464`，步长 0x114，外观／动画／图层分块。`+0x462` 是绘制开关；`+0x463` 是 Host 懒重建请求，Client 安装最终资源，不重演 `0x6A2420`。命中集合保存 Host 原始 u32 索引，不冒充跨进程实体身份，也不启用 Client 碰撞。

## 帧计数和只读来源

`state/entity_counters.h` 明确定义 u64 编码：低 32 位是 mode，高 32 位是 value。mode 0 保存真实停走／稳定参数，mode 1 保存 `game_frame-counter`，mode 2 保存 `game_frame+counter`；算术复用 `anchors.h` 的 SignedCounterAnchor，不维护第二套加减实现。解码使用最近已应用 world 的展示时钟，按模 2^32 恢复，不因负数而擅自夹零。HeadFrameDelay 直接复制原始 i32，不属于此锚点协议。

三类实体不能按字段名统一倒计时。Familiar FireCooldown 已见无条件减一、正值减一和递增分支；Bomb `+0x410` 已见加一、递减、重置与帧差比较。布局因此明确要求每个具名转换 owner 提供对应变体和真实分支条件。条件尚未闭合的条目标为“推断”，必须报告 Gap，不能把未知活动状态编码成停走。本轮提供纯值编码，不宣称全部分支时钟已闭合。Bomb `+0x414` 的完整用途仍未知，不复制。

`ReadKnifeExtension` 经 ReadWindow、既存 holder 与会话 owner 提供的具体 Knife／KnifePlus 虚表绑定读取扩展，不调用会创建 holder 的 GetEntityPlus。非 Knife 目标先返回 Null，不查询成员表；Knife 目标经 `ReferenceCapture::identify_or_reserve` 取得 Identity 或 Pending 纯值句柄，后台再转换为成员序号。原生地址不进入返回状态。公共 optional 复用同次 data 读取，holder 链只在前后各遍历一次。Familiar 的倍率缓存属于 Host 回调求值，正式采集不为补缓存调用 GetMultiplier。

## 主审与未决

证据和主审采用／更正记录见[三类实体静态证据](familiar-knife-bomb-evidence.md)。主审已直接读取三份 RTTI 链与 vtable、Familiar／Knife／Bomb 构造、Knife owner 查找，以及计数器关键原始指令。官方 EntityPlus 声明及 Knife hitbox 回调也已复读；新增派生 Plus 的 Win32 ABI 断言在普通 SDK 编译对象中执行。

剩余缺口按生成表逐段保留。重点包括 Familiar 私有 AI 区、Knife 路径容器和私有标量、Bomb 双计时语义及尾部字节、AbyssLocust payload 未解释字段，以及 FireCooldown／Bomb 计数的完整活动条件。蝗虫两组 Sprite **合计**最多 16384 个元素，`CountConstraint` 及 `ValidContainerCountGroups` 同时检查合计值和同容器各投影 count 一致。每个投影另受编码上限和全局池预算约束，超限报 Gap，不截断；该边界不表示已满足带宽／性能预算。实际未知字节变化、场景覆盖和性能只在第 15 步验收。

| 根对象 | 原生范围 | 原生未知字节 | 生成表 |
| --- | --- | ---: | --- |
| Familiar（含两臂的根对象） | 0xF30 | 906B | [Familiar](layouts/EntityFamiliar.md) |
| Knife | 0x520 | 521B | [Knife](layouts/EntityKnife.md) |
| Bomb | 0x9D8 | 441B | [Bomb](layouts/EntityBomb.md) |

以上包含公共 Entity 和内嵌 ANM2 的未知。伴随 Plus 的 5B、IsaacsBodyData 的 70B、AbyssLocustData 的 36B 及其变长 Sprite 元素内部未知另在各自表中列出，不把它们混成根对象中的字节。

## 用户审阅修复（2026-11-08）

本轮以用户指出的七项问题逐项对照旧路径和生成器。它们均成立，不以原有“已主审”文字代替修复；旧实现与报告的错误发生事实保留如下：

1. 原先遗漏五个已同步关系槽。现恢复 Knife `+4F4/+4F8/+4FC/+500` 到 K3、Familiar `+DAC` 到 F2，新增五条具名回映射，均经 EntityPtr owner 安装和撤边。
2. 原先将 HeadFrameDelay 误作必须取得活动方向的计数。现改为原始 i32 状态，删除两个转换声明和专门的时钟读取要求，保持未证明的计时语义为未知。
3. 原先 `ReadKnifeExtension` 提前查成员序号，既拒绝新成员，也在非 Knife 上错误报缺口。现先判目标类型，再采集身份／待登记预约句柄；提供句柄的 owner 仍须由第 10 步接线，后台完成序号转换。
4. `item-configs` 是误拼，现统一为已有的 `item-config`；版本源新增资源表登记，生成器拒绝未登记的资源空间。
5. `share_parent_hit_list` 原先证据等级过高，还误加了 bool 约束。现降为“推断”的原始 u8，0／非零控制语义按已见指令描述，完整业务含义不升级为已核。
6. 原先只限每组 16384，未表达旧 owner 的合计限制。现用 `count_groups` 生成数字约束，并提供不分配内存的检查函数；第 10／12／13 步在完整同版本状态的采集／解码／安装前使用，不能重复计算投影 count 或用0补缺失数据。
7. 过期的 `260-261` 行号改为 `Compiler.make_variants()` 与 selector 查找逻辑的函数定位。

同时补充 Knife／KnifePlus 动态类型绑定，减少重复 holder 遍历，改善非法投影的报错，复用已有锚点算术，移除 layout.cpp 的无关 include。以上复用现有已核源码与历史证据，没有新增静态逆向需求。

按用户允许延后的范围，`max_bytes` 的父子容器累计预算仍须在第 10／12 步落入真实采集／解码调用，不能只看理论 `Field.max_size` 就宣称满足内存约束。Bomb `+463` 的 Client 清除与读回检查已明确写入第 13 步，退役旧路径时必须保留。

## 编译记录

普通 Release／Win32／v143 构建使用 `ISAACCS_BUILD_E2E_TOOLS=OFF`，布局只随普通 CMake 构建生成。首次并行 MSBuild 没有继续推进，已中断并改为单节点。生成器随后报告蝗虫 Sprite 容器的理论最大块大小溢出；现按独立编码字节预算生成上界，保留原有元素数边界。基础库和完整 `zhlIsaacCS` 的编译／链接已通过，派生 Plus ABI 断言也已通过。SDK 仍输出既有的 `CMAKE_REPENTOGON_VERSION undefined` 提示。

没有执行游戏、单元测试、probe、smoke、校验脚本或 E2E，没有另建 artifact 包。

版本16收尾时发现本地旧构建缓存设置了 `CMAKE_SUPPRESS_REGENERATION=ON`，新增的 `entity-classes.json` 尚未进入旧工程依赖，单独编辑该文件不会触发生成。已重新配置现有 build，开启 CMake 自动再生、保持 E2E 工具关闭，生成依赖现已包含新域；最终重新生成四份表并完成完整 DLL Release 编译／链接。此项是构建依赖修复，没有执行功能测试。
