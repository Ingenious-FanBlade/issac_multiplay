# RE-1 补遗：关闭插值后的 Manager／Render 节拍

日期：2026-11-12。返回：[RE-1 总览](README.md)。本文件回答新增问题：“本机关闭插值的奇数帧，原生是否仍调用 Render？”结论来自 J273 静态调用点和原始指令，没有实机运行。

**同日 RE-10 补充**：下表专述入口奇数快返；普通路径在 `0x92E816—0x92E826` 也检查本机 A／B，A 非零且 B 为零则直接返回、不递增 M、不运行相机／音频。因此下文普通路径的“通常 M+1”只适用于实际到达尾部的周期。原版 Online 更新还有独立早退，C/S 不调用它重演该分支。完整条件与音频顺序见 [RE-10](../RE-10/manager-call-sites.md#4-分支不能合并成一个-paused)，未实施／未测试。

**主循环仍调用 Manager::Render。通常该调用会因更新后的 M 为偶数而跳过整个绘制体；但存在保持 M 为奇数的快返例外，此时仍能进入绘制体。** 不能把“调用 Render 入口”“进入 Manager 绘制体”“最终调用 Game::Render”混成一件事。

## 1. 资料与范围

优先读本地 REPENTOGON SDK `F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\Manager.zhl`／`OptionsConfig.zhl`，确认 Manager 更新／绘制声明、`_state +0x8` 与 `_enableInterpolation`。SDK 未命名的快返控制字节再用当前 J273 IDB 的指令补足。原版／REPENTOGON Wiki 的暂停接口只提供语义，不说明该主循环绘制节拍。

下列均为 VA，image base `0x400000`：

| 入口／字段 | 地址或本地偏移 | 本文记号 |
| --- | --- | --- |
| Manager::Update | `0x92E3E0`，RVA `0x52E3E0` | 更新入口 |
| Manager::Render | `0x92EC40`，RVA `0x52EC40` | 绘制入口 |
| 本机 Manager 帧 | `Manager+0x4A8B8` | 更新前 `Mpre`、更新后 `Mpost` |
| 本机插值设置 | `Manager+0x2A1FB` | `I`，非零表示开启 |
| 两个未命名控制字节 | `Manager+0x4A8C0`／`+0x4A8C1` | `A`／`B`；不猜测具体菜单／单步语义 |

讨论常规主循环、有效非负 M 以及该路径没有其他代码改 M 的情况。启动、重置与本地帧 owner 见 [RE-3](../RE-3/native-clocks.md)；不能以有符号余数处理细节创造另一套时钟。

## 2. Update 提前返回不会取消随后对 Render 的调用

主循环 `_main 0x90AC60` 中，调用点紧邻：

```asm
90AF06  push 1
90AF08  call 92E3E0   ; Manager::Update
90AF0D  call 92EC40   ; Manager::Render
```

两条 call 之间没有按 Update 结果分支。Update 的快返是 `ret 4`，并非跳过 caller 后续的 Render。`0x90AF12` 之后还有其他处理，不改变这一对调用的先后。

静态交叉引用还发现 `0x8D9228` 的另一个 Manager::Render 调用（所属函数 `0x8D7ED0`，加载／资源路径）及 `0x6F42AD` 的另一个 Manager::Update 调用；因此本文真值表限定主循环的 Update→Render 配对，不声称穷尽所有渲染调用。

## 3. 关闭插值的奇数帧有两种快返

Manager::Update 前缀的原始判断：

```text
0x92E406  读取 Mpre
0x92E41A  判断奇数相位（原指令带有符号余数规范化）
0x92E41F  检查 I
           Mpre 非负奇数且 I==0 时，进入快返：
0x92E428    检查 A
0x92E431    检查 B
             A!=0 且 B==0 → 0x92EC0F，直接返回，不递增 M
0x92E43E    其他组合计算 Mpre+1
0x92E441    写入 Mpost=Mpre+1
0x92E45A    ret 4
```

这两条快返都跳过 Game 更新／Odd helper 和相机尾部。旧描述“奇数帧写 M+1 后返回”只涵盖一般组合；**A 非零且 B 为零时不写 M+1**。这两个字节的精确产品语义尚未命名，不影响此处控制流结论，也不据此运行调试／单步实验。

普通路径在 `0x92EC09` 有本地帧推进。Client 已跳过完整 Manager 原链，后续需由唯一显示 owner 保持这一增量或保持行为，不能快返处加一次、统一尾部再加一次。

## 4. Render 读取的是 Mpost

Manager::Render 的入口判断：

```asm
92EC51  test byte ptr [Manager+4A8B8h], 1
92EC58  jnz  92EC67          ; 奇数：进入绘制体
92EC5A  cmp byte ptr [Manager+2A1FBh], 0
92EC61  jz   92F5E8          ; 偶数且关闭插值：直接返回
92EC67  ...                  ; 绘制体
```

因此绘制体条件是 **`I !=0 || (Mpost & 1) !=0`**。不是 Mpre，也不是 Host GameFrame；普通快返已经将奇数 Mpre 改成偶数 Mpost。

| I | Mpre | A／B | Update 行为 | Mpost | 随后 Manager::Render 入口 | Manager 绘制体 |
| --- | --- | --- | --- | --- | --- | --- |
| 开 | 偶数 | 任意 | 普通路径，Game 更新；相机另受暂停条件控制 | 通常 Mpre+1，奇数 | 调用 | 进入 |
| 开 | 奇数 | 任意 | 普通半帧路径；相机另受暂停条件控制 | 通常 Mpre+1，偶数 | 调用 | 进入 |
| 关 | 偶数 | 任意 | 普通路径，Game 更新；相机另受暂停条件控制 | 通常 Mpre+1，奇数 | 调用 | 进入 |
| 关 | 奇数 | A==0 或 B!=0 | 快返；不运行 Game／相机，M 自增 | Mpre+1，偶数 | 调用 | 跳过 |
| 关 | 奇数 | A!=0 且 B==0 | 快返；不运行 Game／相机，M 保持 | Mpre，奇数 | 调用 | 进入 |

最后一行允许在保持同一 M 时反复绘制，但没有对应的 Game／相机更新。不能以“M 没变”去掉这次原生 Render；也不能把每次 Render 当作新的 HUD／FX 更新周期。

**进入 Manager 绘制体仍不保证 Game::Render 被调用。** 绘制体还有 `Manager+0x4A3D4` 的特殊 UI／资源分派；普通状态分派在 `0x92EDFF` 读 `Manager+0x8`，游戏状态 2 才在 `0x92EE15` 调 `Game::Render 0x6F7A80`。本表的 Game 绘制推论以当前为有效游戏分派为前提，不授权伪造 Manager.state 或越过菜单／资源 owner。

## 5. 相机与暂停的关系

能到达普通 Manager 尾部时：

1. `0x92E94A` 调 `Game::IsPaused 0x6F92A0`。
2. 若 paused 为真，只有 RoomTransition mode `Game+0x1B83C !=0` 且 phase `Game+0x1B858 ==19` 能通过 `0x92E953／0x92E95C`。
3. 本机 CameraStyle=2 时 `0x92E980 → UpdateDrag2 0x91E5E0`；其他值 `0x92E998 → UpdateUltrasmooth 0x91D840`，传当前本机半帧相位。
4. Ultrasmooth 后总走 `0x92E99D` 清 `Camera+0x8C`；Drag 只在奇数相位走该清除点。

> **2026-11-12 审阅更正**：`Game+0x1B858` 是 RoomTransition `+0x1C` 的 animation 参数（Start 在 `0x823B05` 写入），19 为 MINECART，不是“阶段”；本文“phase／阶段 19”均指 animation 19。按 Q12，Client 的 paused 取 Host 同步的 IsPaused 结果；按 Q11，mode 与 animation 取已安装的 Host 当前值。

关闭插值的快返在所有这些步骤之前。阶段 19 不能让已经快返的奇数周期重新运行相机。另一方面，Game::Update 某个 fade 分支提前返回也不等于 IsPaused 为真，详见[分支表](early-return-branches.md#4-相机的-paused-与上述分支并非同一个选择器)。

## 6. 实现交接与第 15 步覆盖

`M`、`I`、`A`、`B`、CameraStyle、Manager.state 均为本机 owner，不同步 Host 的字段替换它们。第 13 步在 Manager 边界按 **Mpre 判更新资格，推进／保持后由原生 Render 按 Mpost 判绘制资格**；原生 Manager::Render 入口继续负责自身绘制判断。`Game::Render` 包装只在实际被调用时工作，不另补一次 Game::Render。

第 15 步统一覆盖：插值开／关切换、奇偶跨越、A／B 的保持组合、Host 暂停与阶段 19、相同 F／M 的重复绘制、Client 安装与本地 M 推进顺序、菜单／资源分派和退出恢复。重点防止用 Mpre 反转绘制相位、漏掉保持例外、M 双增、把 Host 插值设置套到 Client，以及为维持画面补跑被拦截的 Odd／玩法更新。本次没有执行这些场景。
