# Projectile 追踪、曲线、加速度、下落与摆动参数同步复核

日期：2026-11-09（Asia/Singapore）。第 4 步，布局版本 **36**。本轮只做本地资料阅读、J273 静态分析、布局源／计划修订及表格生成；没有构建、测试、游戏、注入、部署或 IDB 修改。版本28已经排除的Damage与Change三参数保持原职责，历史事实和UNKNOWN保留。

## 1. 采用结论

**六项运动输入留在Host：HomingStrength、CurvingStrength、Acceleration、FallingSpeed、FallingAccel、WiggleFrameOffset。** 同时核对相邻的Scale与ProjectileFlags：它们也是Host更新／动画选择／运动与碰撞规则输入，撤销传输。**Height有直接Render消费者，继续保留。**

| 原生范围 | 字段 | 当前决定 |
| --- | --- | --- |
| `+410`，f32 | **Height** | **保留**：原生Render在私有开关下按它临时调整PositionOffset.y，不能随下落速度／加速度一起删去 |
| `+414/+418`，各f32 | FallingSpeed／FallingAccel | Host下落积分与高度分支；Client接收Height及最终位置／绘制偏移，不再积分 |
| `+424/+428/+42C`，各f32 | HomingStrength／CurvingStrength／Acceleration | Host追踪、曲线、速度倍率求值；Client接收最终Position／Velocity，不求目标、不重算轨迹 |
| `+448`，i32 | WiggleFrameOffset | Host摆动运动相位输入；不建立Client展示计时锚点，不放行Interpolate重演摆动 |
| `+420`，f32 | Scale | Host尺寸／Small、Big动画选择输入；最终公共Size／SizeMulti及Sprite／图层缩放独立同步 |
| `+438—43F`，u64 | ProjectileFlags | Host运动、碰撞及效果生成规则；不是公共EntityFlags。当前展示读取后者和独立私有输入，不传本组规则位 |

排除 **8个原生范围／线上字段，共36B**，Projectile最小状态 **431→395B**。PR0只剩4B Height；PR1改为显式预留空块，后续块号／布局号129保持。父类Entity公共状态、DepthOffset别名、资源／姿态等仍按当前合同发送；空块不要求发送零长负载。

撤销八项 `wire`、专用读取资格、线上validator和公开成员状态回映射，改为 `disposition=authority`。Height继续f32及有限值检查。原生偏移、大小、类型和已核等级不变；375B未知仍保持，不能由“已知子类线上字段只剩Height”推定类型完整。上述最小字节数是布局算术，不是带宽实测。

## 2. Height与实际绘制输入

目标为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，ImageBase `0x400000`，地址均为VA。语义先核对本地 `IsaacDocs/docs/EntityProjectile.md`；存储依据[J273公开成员与访问器](../tx-04-19/entity-i10-i14-native.md)、[本类原始静态证据](slot-projectile-effect-evidence.md)。官方EntityProjectile.zhl只有类边界／虚函数声明，不伪称它声明了这些全部私有标量。

J273 vtable `B15B1C`：Update `64F990`、Interpolate `658E10`、Render `659460`、Shadow `6AD450`、PostRender `6AC660`、GetRenderZ `6AC3F0`、GetForwardVector `6B1FD0`。槽位与消费者分别核对，不把半帧更新当成绘制维护。

**Height不能只由最终PositionOffset替代。** Render `659460` 的实际指令：

- `65946B` 比较本体 `+440` 的byte；为真时在 `659474—659484` 执行 `PositionOffset.y(+350) -= Height(+410)`。
- 随后按公共EntityFlags与私有位置选择普通／特效绘制；普通路径在 `6597B3` 调公共Render。
- `6597B8` 再查同一开关，`6597C1—6597D1` 将Height加回，之后进入Lua render桥。

因此Host的公共最终偏移与Height是两份独立绘制输入。不能在Client用FallingSpeed／FallingAccel推进高度，也不能把Height烘焙到公共偏移后宣称原生Shadow／WaterClip／排序等pass仍然等价。第13步draw scope须精确保留／恢复实际临时改写的 `+350`；原生浮点减后加不保证逐位恢复，不能依赖这两条运算替代完整借用恢复。

**ProjectileFlags与公共EntityFlags分开。** Render在 `6594C7—6594D3` 使用公共 `Entity+168`，并读取 `+460/+464` 的私有位置；它没有读取本轮排除的 `Projectile+438/+43C`。公共Shadow／PostRender、GetRenderZ、WaterClip与Room Light也未发现这些运动输入／本组规则位的Projectile展示消费；其他类同偏移的flags或资源不是本类依赖。

**私有展示缺口继续保留。** `+440` 的byte开关、`+460/+464` 的f32位置在当前原生表仍分别属于 `unknown_440_447`、`unknown_460_46f`。Render直接消费不等于其生产者、逐字段布局、初始化／生命周期或采集与安装已经闭合。本轮在源中补写这个使用事实，保持原UNKNOWN范围，不把它们改归Host、填充或零值。第4步后续仍需补齐这些展示来源，不能只传Height就宣称Projectile画面完整；未接线／缺失场景不能记为通过。

Render在 `6594A4` 读取 `Game+25A70` 的游戏帧生成局部随机采样，并非读取WiggleFrameOffset；第8／13步既有D0／world展示时钟借用合同保持，不因摆动参数排除而省略公共帧／绘制上下文。

## 3. Host用途与间接路径核对

复用当前26个官方展示根（公共／各类Render、Shadow、PostRender、Player复合绘制、Room Light、排序、WaterClip）及明确停止展开的Lua render桥，直接可达集合为532函数。扫描执行段中目标位移内存、LEA及ADD／SUB取址候选，涉及778函数；与展示集合交叉为18函数。另核对跨字段宽访问：Room Light `7FD5EE` 的16B读取是**Type2 Tear flags**，不是Projectile的CurvingStrength／Acceleration；Slot的 `+42C` 是奖品ANM2、Knife的 `+448` 是拖影速度，均不能混作Projectile消费者。

| 原生路径 | 正面用途与Client边界 |
| --- | --- |
| Update `64F990`，追踪／曲线 | `6509D7/650A03` 读HomingStrength，`654DFC/656152` 取字段地址交更新求值；`654AB6—654AF1` 读CurvingStrength，应用到Velocity并复位参数，`655671` 另有取址输入。这些在Host运动更新中，不在展示路径调用 |
| Update，加速度 | `6561B4` 读Acceleration与当前速度倍率求值，`65627A` 另有除法分支，`6562F7` 传字段地址；不能把该字段从名字推成单一单位下的恒定向量加速度。Client直接安装最终Velocity／Position，不再乘一次 |
| Update，下落 | `65051C/650557`、`650BF7/650D4F` 等读FallingAccel／FallingSpeed；Height也被Update按分支更新。Host最终Height与公共绘制偏移分别采集，Client不积分、触地碰撞或生成效果 |
| Update，摆动 | `655D3D—655D68` 从i32 WiggleFrameOffset进入相位／三角求值，`655DF3/655E45` 还有分支读取；它是运动公式参数，不是需要Client随world时钟恢复的剩余帧数 |
| Update，Scale | `64FCAF—64FCDF` 读Scale并选择`Small%02d`／`Big%02d`等动画结果；其他分支还改变尺寸／变换。最终动画、selector、主／图层scale及公共碰撞几何单独同步；Client不按Scale重选图形 |
| Update／碰撞，ProjectileFlags | Update `64F9ED/64FB08` 起大量按64位规则分支；`657C60` HandleCollision也消费这组位。Client接收最终公共flags、Sprite、独立效果与目标状态，不运行碰撞／生成规则 |
| Interpolate `658E10` | 先推进Position并在 `658EBB—658EC8` 调本地 `+470` 回调；`658ECD/658ED5` 读ProjectileFlags，`658FF5/659106` 读WiggleFrameOffset求半帧位置，`6593D1/6593EC` 读FallingSpeed／FallingAccel并写 `+350`。它是真实运动模拟，不得为“平滑展示”在Client放行 |

`+470` 仍是本进程更新回调，Client不接收地址、不调用它，沿用已有本地归属。原生半帧还包含碰撞／位置维护调用；网络插值如实施只使用本地已应用快照历史，不能因需要曲线／摆动视觉就恢复原生模拟输入。

官方 `CustomCallbacks.cpp` 的PRE_PROJECTILE_RENDER wrapper传variant、对象及offset后调super，没有内建读取本轮八项字段；任意第三方Lua回调的私有读取不属于当前支持声明。这是当前官方消费者和Client隔离合同下的静态采用结论，不是任意虚分派／地址别名的全程序形式化证明；实际对象基址、宽访问和取址路径均不能用一次字符串无命中替代。

## 4. 后续接线与唯一验收

第10步按版本36生成计划不再复制八项Host输入；保留Height及公共最终位置、Velocity、绘制偏移、尺寸和Sprite。第12／13步不安装、校验或漂移比较已排除项；Height、公共DepthOffset和真实资源／姿态继续各自合同。私有展示缺口尚未补齐，运行时来源／安装不能被此次生成表冒充完成。

第14步退役相应旧Lua Source、字段写入和原值相等要求；Damage及Change三项继续版本28的排除。PR1预留不作为构造必收负载。保持本地构造／回调空状态、资源owner与draw借用，不调用派生Update／Interpolate／碰撞／Deflect来重建最终运动。运行中的旧Lua／JSON链本轮未修改。

第15步沿用已有条目，不提前运行或新增编号：

- E09／E25覆盖追踪、左右曲线、加速／减速、不同下落速度／加速度、摆动相位、Scale选图和ProjectileFlags组合造成的最终轨迹、大小、动画、成员及效果；比较真正上线结果，不比较八个Host输入。Height覆盖私有 `+440` 开关两分支及公共偏移非零场景，兼顾Shadow／水面／排序和特殊绘制路径；私有来源未齐时不得跳过后宣称通过。
- E13／E17继续拒绝实际线上Height的非有限值、真实资源／选择器／引用等非法状态；不再构造八项已撤销参数的线上非法负载。PR1空块不阻止完整构造／ACK。
- E28／E29确认Client没有本地追踪、曲线／加速、下落／摆动、碰撞或更新回调；检查Height临时偏移及Sprite借用精确恢复，避免漂移累计，正常资源退休保持。
- E34不新增WiggleFrameOffset或其他运动参数的帧锚点；公共FrameCount／D0与其他实际展示计时保持。E27继续全部375B未知及本类私有展示输入的原有范围，不因已知字段减少而删除观察。

本轮完成源／生成表、相关计划和整体静态审阅。没有构建或运行测试；第10／13步接线和第15步功能验收仍未完成。
