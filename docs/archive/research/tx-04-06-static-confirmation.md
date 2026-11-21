# tx-04-06 静态确认与主智能体审阅

日期：2026-10-25。范围：[实现方案](../design/tx-04-06-implementation-plan.md)第 12 节的 **ST1–ST9**。按用户要求，由 **GPT-6 Luna / max** 子智能体使用 IDA MCP 逆向，主智能体另行读取原始表、指令、调用方与官方源码审阅。本文保留先完成的 ST1 证据及当时边界；用户继续授权的 **[ST2–ST9 指定静态研究与主审已完成](tx-04-06-static-remaining.md)**。本次没有实现设备、安装 Hook、启动游戏、构建或执行测试，唯一整体 E2E 尚未执行。

**后续补充的实质变化：** 已确认设备通知触及控制器持久配置与原版联机回调；菜单物理确认能选中中性 K，暂停断连处理还会直接枚举读取派生尾部；角色工厂只有一个参数，CALL shim 拒绝清 8 仍正确；K controller 在工厂返回后才写入。ST1 下文的 `0x400` 是官方名称读取所需下限，不是完整 UI 安全布局。见补充报告和已同步的实现方案；兼容分派与副作用控制尚未实现。

## 1. 对象与证据口径

- 本轮 IDB 输入名为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`，imagebase 为 `0x400000`。本文地址均为 **RVA**；IDA VA = RVA + `0x400000`。输入路径和 IDB 路径保存在原始证据中；没有计算或校验哈希。
- `STATIC_CONFIRMED` 指本轮直接读取的表、指令及其明确控制流；`SOURCE` 指本地官方 `F:\projects_isaac\REPENTOGON-1.1.3` 源码；`STATIC_INFERENCE` 指限定路径下的推导；`DESIGN` 是 K 的实现选择。上述均不等于运行时观察。
- [证据目录](../../../artifacts/repentogon/tx-04-06-static/README.md)分别保存子智能体证据与主审[独立 IDA 导出](../../../artifacts/repentogon/tx-04-06-static/st1-primary-ida-evidence.txt)、[三张槽表](../../../artifacts/repentogon/tx-04-06-static/st1-primary-tables.json)和[官方源码片段](../../../artifacts/repentogon/tx-04-06-static/st1-primary-source-evidence.txt)。
- 这是已定位消费者的静态分析，不宣称完成全映像间接调用的穷尽证明。未知插件、运行时 Hook 顺序、实际线程与生命周期交错仍没有运行时观察。

## 2. ST1 结论

**基类 38 槽表已读取；四个重点槽的调用约定和三个删除入口已确认。原方案的六槽覆写集合需要修订。**

| 问题 | 结论 | 对 K 的影响 |
| --- | --- | --- |
| 槽 2 | 释放 action 映射行、映射指针表及 current/previous 两个缓冲；无栈参数、普通 `ret`，没有 bool 返回合同 | 按 NEVER 策略覆写为保留基类状态的退休通知；不能透传释放 |
| 槽 3 | 返回设备名称 `const char*`，EAX 返回，无栈参数；Keyboard 返回 `"Keyboard"`，Gamepad 返回 `[this+0x3FC]` | 返回常驻、有效、非空的自有名称字符串；不能返回整数类型码或用 0 代替名称 |
| 槽 35 | Poll：ECX=this，无栈参数，AL 为成功/仍连接；false 进入删除路径 | K 有效时 true，Retiring/Removed 时 false；不调用物理设备 Poll |
| 槽 0 | scalar deleting destructor：ECX=this，栈上一项 flags，占 4 字节，`retn 4`，EAX 返回 this；原实现先清理再按 flags 位 0 释放对象 | K 只标记删除通知并返回 this，不调用原析构，不释放对象或有效基类资源 |
| 非 action 查询 | raw 按钮槽 5/6/7、原始触发键槽 11、活动槽 26/27 均存在独立消费者 | 明确提供中性输出，不能用“8/9/10 已中性”代替这些入口的策略 |
| 官方 Initialize Hook | `SOURCE`：在进入原生 Initialize 前可能读取 `this+0x3FC` 的名称指针 | K 分配/布局必须覆盖 `+0x3FC..+0x3FF`，不能把任意状态放在此位置 |

`DESIGN`：K 从**基类**复制 38 槽，覆写 **0/2/3/5/6/7/8/9/10/11/26/27/35**。其中 0/2/3/8/9/10/35 负责 NEVER、纯虚与业务查询；5/6/7/11/26/27 是本次明确的非 gameplay 中性策略，防止原生 setter 改变自有缓存后，经 raw/菜单活动入口形成旁路。其余 25 槽保留基类实现；不复制 Keyboard/Gamepad 的物理实现。

槽 5/6 的中性值为 AL=false；槽 7 为 x87 `ST(0)=0.0f`；槽 11 为 EAX=`-1`（“无触发键”，**不是 0**）；槽 26/27 为 AL=false。槽 8/9 的中性值为 false，槽 10 为 x87 `0.0f`。查询覆写还应遵守已有 enable、会话、阻断与 consumer/Join scope 语义；本次不决定 ST3 的 Join 动作序列。

## 3. 38 槽清单

`STATIC_CONFIRMED`：基类 vtable 位于 `0x732F04`，槽偏移为 `4 × slot`，纯虚槽为 3/35；Keyboard 表 `0x73B490`、Gamepad 表 `0x73B3F0` 的对应槽另存 JSON。Gamepad Poll 会调用其额外第 38 槽，K 不复制该派生表，也不调用该 Poll。

表中的功能名按已读指令描述，不把 IDA 自动函数名或错误的 Hex-Rays 原型当作正式 ABI。

| 槽 | 基类目标 RVA | 已读行为 / K 选择 |
| --- | --- | --- |
| 0 | `0x5EC660` | deleting destructor；覆写，保留对象并返回 this |
| 1 | `0x5EC6A0` | Initialize，分配映射/状态、清零并虚调槽 30；保留，一次性调用 |
| 2 | `0x5EC7B0` | 释放映射与状态缓冲；覆写，不释放 |
| 3 | `0x68801D` | `_purecall`；覆写，返回常驻名称 |
| 4 | `0x5EC840` | 按原始按钮编号返回标签字符串；保留 |
| 5 | `0x5ECB80` | 原始按钮 Pressed；覆写 false |
| 6 | `0x5ECC40` | 原始按钮 Triggered；覆写 false |
| 7 | `0x5ECD00` | 原始按钮 Scalar；覆写 x87 零 |
| 8 | `0x5ECD80` | action Pressed，原实现会调用槽 5；覆写为受上下文约束的 K 查询 |
| 9 | `0x5ECDE0` | action Triggered，原实现会调用槽 6；覆写为受上下文约束的 K 查询 |
| 10 | `0x5ECE50` | action Scalar，原实现会调用槽 7；覆写为受上下文约束的 K 查询 |
| 11 | `0x5ECED0` | 找到一个原始触发按钮，未找到返回 -1；覆写 -1 |
| 12 | `0x5ECF50` | bool 转 1/0 后调用槽 13 写原始按钮状态；保留 |
| 13 | `0x5ECF90` | 写原始按钮/带标签方向的状态；保留，不作为远端输入入口 |
| 14 | `0x5ED070` | bool 转 1/0 后调用槽 15 写 action 状态；保留 |
| 15 | `0x5ED0B0` | 范围合法时写 current action float；保留，不作为远端输入入口 |
| 16 | `0x5ED0D0` | 将原始按钮加入 previous/mask 状态；保留 |
| 17 | `0x5ED130` | 对 action 的两个映射按钮调用槽 16，并清其 previous float；保留 |
| 18 | `0x5ED170` | 向调用方缓冲复制默认二维值，EAX 返回输出指针、`retn 8`；保留基类默认 |
| 19 | `0x5C4E70` | `xor eax,eax; ret`；保留，不为未命名能力猜测业务含义 |
| 20 | `0x5ED190` | 清零并按标记选择一条 `0x30` 字节记录；保留 |
| 21 | `0x5ED210` | 清零并按 id/标记选择一条 `0x30` 字节记录；保留 |
| 22 | `0x5ED2A0` | 清零并筛选记录集合，输出区大小 `0x304`；保留 |
| 23 | `0x028390` | 返回整数 0，`retn 4`；保留 |
| 24 | `0x028390` | 同槽 23；保留 |
| 25 | `0x5ED330` | 调槽 23/24，转为两个 float 写调用方缓冲；保留 |
| 26 | `0x5ED3A0` | 综合当前按钮、方向、action/记录的活动判断；覆写 false |
| 27 | `0x5ED4E0` | 综合原始触发/action 上升沿/记录的活动判断；覆写 false |
| 28 | `0x5ED600` | `device+0x0D=0`，禁用；保留 |
| 29 | `0x5ED610` | `device+0x0D=1`，启用；保留 |
| 30 | `0x5ED620` | 按模板重建每个 action 的两个映射；保留 |
| 31 | `0x5ED6A0` | 读取第一个映射按钮，越界返回 -1；保留 |
| 32 | `0x5ED6C0` | 读取两个映射按钮到输出区；保留原有边界语义 |
| 33 | `0x5ED700` | 在 action/映射索引合法时写一个映射；保留 |
| 34 | `0x5ED730` | 记录待重新绑定的 action/映射索引到 `+0x3DC/+0x3E0`；保留 |
| 35 | `0x68801D` | `_purecall`；覆写为自有连接状态 Poll |
| 36 | `0x027EE0` | `retn 8` 的 no-op；保留，不调用 Gamepad 的物理输出 |
| 37 | `0x5ED850` | 对输入的 `0x30` 字节记录集合推导/更新状态；保留，不作 K 输入来源 |

槽 18 从全局 `0x7EB900` 复制 8 字节；本轮 IDB 中这 8 字节全零，原始值另存[证据](../../../artifacts/repentogon/tx-04-06-static/st1-primary-vector-default.json)。`STATIC_INFERENCE`：在该默认向量不变、K 不调用原生 setter 注入事件的前提下，保留的基类方向/记录 helper 为中性。静态初值不证明运行时所有写入者均不存在；K 的 gameplay/Join 输出仍只经槽 8/9/10。

## 4. 已确认的消费者与路径

### 4.1 InputManager 更新与删除

`STATIC_CONFIRMED`：InputManager 更新 `0x5EDCC0` 在 `0x5EDD22` 对每个设备直接调用公共更新 `0x5EC900`。公共更新先复制 current→previous、清空 current，然后在 `0x5EC984` 读取设备 vtable `+0x8C`，`0x5EC98A` 执行 `call eax` 调槽 35，下一条 `0x5EC98C` 检查 AL。

- Poll=true 后，公共更新在 `0x5EC9A3/0x5ECA00` 调槽 18，两个栈参数包含输出缓冲和方向选择；待重绑定索引有效时在 `0x5ECA66` 调槽 11；按输出计时状态可在 `0x5ECB2F/0x5ECB67` 调槽 36。
- 公共更新成功后，InputManager 在 `0x5EDD96` 调槽 26；true 且设备 `+0x0C` 尚未标记时，进入原生活动设备数组/通知分支。这里仅确认该槽参与决策，连接提示及回调副作用的完整分析留给 ST2。
- Poll=false：InputManager 在 `0x5EDD64` 调槽 2，随后 `push 1; call [eax]`（`0x5EDD6F/0x5EDD71`）调用槽 0，再移除 vector 元素。
- 主动 RemoveDevice `0x5EEBB0`：`0x5EEC24` 调槽 2，`0x5EEC2B/0x5EEC2D` 以 flags=1 调槽 0。
- InputManager 整体关闭 `0x5EDAA0`：`0x5EDAC6` 调槽 2，`0x5EDAD1/0x5EDAD3` 以 flags=1 调槽 0。该调用存在不等于已确认每次退出 run 都执行它；run/menu 保留问题属于 ST2。

槽 0 原实现 `0x5EC666` 会把 vptr 改回基类表，`0x5EC66C` 直接调用清理函数，随后检查 flags 位 0，决定是否释放 `0x3F8` 大小对象，最后 EAX=this、`retn 4`。所以 K 的槽 0 不能先“调用原函数再保留内存”。槽 2 已经会独立释放/清空 `+0x1C/+0xC8/+0xCC`，因此只覆写槽 0 也不满足规范第 9 节。

### 4.2 InputManager 查询包装

`STATIC_CONFIRMED`：下表的 explicit id 路径先经 `0x5EEC40` 找到设备；支持 controller=-1 的路径逐设备查询。这些是设备查询，与 InputManager 内部 `manager+0x48` 锁对象的虚调用区分开来。

| 包装函数 RVA | 设备槽 | 返回 |
| --- | --- | --- |
| `0x5EDC10` | 3 | 名称指针 |
| `0x5EDC30`、`0x5EDC60` | 4 | 按钮/映射标签字符串 |
| `0x5EDEA0` | 5 | raw Pressed bool |
| `0x5EDF60` | 6 | raw Triggered bool |
| `0x5EE040` | 7 | raw Scalar；`0x5EE020` 另经该入口判断是否为零 |
| `0x5EE140` | 8 | action Pressed bool |
| `0x5EE200` | 9 | action Triggered bool |
| `0x5EE2E0` | 10 | action Scalar，x87 float |
| `0x5EE3E0` | 11 | 原始触发按钮或 -1 |
| `0x5EE6C0`、`0x5EE6F0` | 26、27 | 当前活动 / 新活动 bool |
| `0x5EE720`、`0x5EE790` | 28、29 | 禁用 / 启用 |
| `0x5EE800/820/850/880/8B0` | 30/31/32/33/34 | 重置、读写映射、申请重绑定 |

中间还有对槽 12–25 的设置/查询包装；完整指令保存在主审导出。不能因为函数名含 `GetDeviceType` 就把返回值解释为整数类型：本地 `.zhl` 的该特征实际匹配 `0x5EEC40`，其指令遍历设备并返回对象指针。槽 3 的合同由两种派生实现和 `0x5EDC10` 的调用方确定。

### 4.3 Join pass

`STATIC_CONFIRMED`：`0x591360` 对设备 id 调 `0x5EE200`（槽 9）及 `0x5EE140`（槽 8），并有取得设备后直接调用槽 17 的动作屏蔽路径。直接调用槽 17 的位置为 `0x591794`。这说明 K 仍需有效的映射表与 previous 缓冲，不能在删除通知前释放，或把无关基类槽全部换为通用 `return 0`。

本节仅回答槽的可达性；动作编号、选人分支、需要几个 pass 与最小报告序列留给 ST3，不把原型序列当作本轮确认。

### 4.4 菜单与 UI

`STATIC_CONFIRMED`：官方 `MenuManager::Update` 特征唯一匹配 `0x561CB0`。该函数先通过 `0x5EE6C0` 查询槽 26；其枚举设备的循环还在 `0x56207D` 直接查询槽 27，并据结果更新当前菜单控制器 id。K 的非 gameplay 动作中性不能代替这两个活动入口的中性。

按键设置更新 `0x4B9AD0`：等待按键时在 `0x4B9D1A` 查询槽 11；`0x4B9D1F` 比较 -1，返回 -1 则不提交新映射，其他值会流向 `0x4B9D58` 的槽 33 调用。原生还可在 `0x4B9D00` 对选中的设备调用槽 30 重置映射；导航走槽 9，并存在对 id=0 设备的单独 fallback。原生 UI 对名称和按钮标签的查询会经上表槽 3/4；mapping 读写 helper 保留基类合同。K 名称应为可读的常驻字符串；名称字符串本身不构成“所有代码都不把 K 当物理设备”的证明。

**`STATIC_CONFIRMED` 风险，完整可达性 `UNKNOWN`：** 另一菜单绘制函数实际起点为 `0x4B8C70`；在菜单控制器 id 非零且 lookup 成功的分支，`0x4B8D3D` 直接读 `device+0x48C`，`0x4B8D46` 直接读 `device+0x498`，超出 `0x3F8` 基类区，也超出本次为名称预留的 `0x400` 范围。第 26/27 槽中性会阻止已定位的活动选择路径选中 K，但不证明所有 UI 都不会从其他入口把 K 设为当前控制器；名称与 13 槽覆写同样不消除直接读尾部的风险。完整排除/适配留给 ST9，本次不为这两个未知字段扩张或伪造 Gamepad 布局。

`SOURCE`：REPENTOGON 的 `ModsMenuStuff.cpp:130–135` 会在搜索状态提前返回 false，跳过原生 InputManager action Triggered；因此未到达 K 查询不等于不存在请求。这里不分析该页面全部交互或副作用，ST9 仍需单独确认非 gameplay 场景。

## 5. 官方 Initialize Hook 对布局的补充

`SOURCE`：官方 `Input.zhl:29–34` 声明 Initialize 为 `bool(void*)`，`_deviceName` 位于 `+0x3FC`；`MiscFixes.cpp:268–273` 的入口 Hook 在 `blockUnknownDevices` 打开且该指针非空时构造字符串，与 `"Unknown Device"` 比较，可提前返回 false。

`STATIC_CONFIRMED`：原生基类构造函数本身只负责 `0x3F8` 基类布局；Initialize 的原生 body 最后虚调槽 30，AL=1、`retn 4`。它不负责为自有尾部提供一个合法的 `+0x3FC` 名称指针。

`DESIGN（ST1 当时的名称兼容下限）`：K 保留 `0x3F8` 原生基类区，并在自有扩展中显式预留 `+0x3F8` padding 和 `+0x3FC` 名称指针；对象存储至少覆盖 `0x400` 字节。调用可能经过 REPENTOGON Hook 的 Initialize **之前**就把名称字段初始化为常驻自有名称（或合法的空指针）；槽 3 返回常驻非空名称，不能依赖读取未初始化尾部。正常只初始化一次，检查返回值；本轮不绕开官方 Hook，也不修改其源码。此处是源码可见的兼容要求，不声称该选项已在用户机器开启。**后续 ST2/ST9 已确认 `+0x3F8`、`+0x48C/+0x498` 的额外消费者，因此不能再把 `0x400` 后任意存放私有状态视为完整布局方案；具体消费者过滤见补充报告。**

## 6. 主审与边界

主审独立导出基类/Keyboard/Gamepad 表，以及公共更新、查询包装、Join、菜单活动、按键设置与三条删除路径，关键 ABI 均回到汇编判定。Hex-Rays 对公共更新报 bad-SP 并伪造 Poll 参数，对槽 18 将全局名显示成 `a4`；这些内容原样保存在证据中，不据此声明多参数 Poll 或读取调用方未初始化变量。

主审已读取 Luna 的[最终结论](../../../artifacts/repentogon/tx-04-06-static/st1-subagent-findings.md)及其正确的[KeyConfig 补充证据](../../../artifacts/repentogon/tx-04-06-static/st1-subagent-evidence/ida-st1-keyconfig-confirmed.json)。采纳其经更正的 38 槽、0/2/3/35 ABI、查询分派与消费者结论；13 槽集合按 DESIGN 采纳，保留基类槽 17/30/33 的前提是 K 的远端输入不读写这些原生缓存。双方均不将通用 `+0x0C` 候选、空扫描或名称字符串作为完整设备调用集合/UI 安全的证明。子智能体漏读槽 11、候选地址错误、imagebase 错误以及主审草稿调用点更正均保存在[更正记录](../../../artifacts/repentogon/tx-04-06-static/st1-review-corrections.md)。

ST1 单独不证明运行时并发、OOM/EH、回调重入或额外插件兼容。原先留给 ST2 的注册/保留与通知分析、ST3 的最小 Join 序列、ST9 的非 gameplay 行为分析等，现已在[补充报告](tx-04-06-static-remaining.md)逐项研究并审阅，风险与剩余实现问题如实保留，不改成已运行时观察或 E2E 通过。没有新增 gate，没有计算/校验哈希，也没有执行静态阶段之外的测试。
