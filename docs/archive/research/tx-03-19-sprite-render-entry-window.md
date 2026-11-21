# tx-03-19：独立 Sprite 的实际绘制接入窗口

日期：2026-10-23（Asia/Shanghai）。承接 [tx-03-18 的最小适配合同](tx-03-18-single-sprite-adapter-contract.md)，本轮研究实际调用位置、线程／WGL current 的检查依据、Sprite 参数 ABI、绘制批次的引用和正常恢复顺序。GPT-6 Luna / max 研究 Hook／context guard，主智能体独立取证复核，并研究最终 pass、批次提交和图像寿命，见 [Luna 报告](tx-03-19-luna-render-window-guard.md)和 [主审记录](tx-03-19-primary-review.md)。

本轮仅静态逆向与文档；未安装新 Hook、实现 adapter 或执行任何 native／OS helper。研究和编辑期间零测试，全部完成后只执行一次整体静态文档审阅；没有游戏、调试、构建、资源加载或像素实验。

## 结论

**最小屏幕空间 Sprite 的具体候选，是 `9555C0` 最终调用 `A19180` 之前的窗口：callsite VA `9560D0`、RVA `5560D0`。** 原生最后一次屏幕合成 draw 在它之前，`A19180` 随后提交批次，再根据当前 render target 清离屏目标或交换窗口缓冲。这个窗口适合进一步实现一张已准备好的独立 Sprite 的屏幕叠加；尚不承担房间内遮挡排序、光照／后处理一致性或完整 Replica 的绘制。

现有外层 RenderHook 只包裹普通主循环 callsite。其返回后已越过最终 flush／可能的 SwapBuffers，不能直接把 post-render 事件当成本帧新增绘制的接入点；其进入前则尚未建立本帧目标和 pass。新增内部 callsite wrapper 是设计候选，未加入安装事务，也未运行。

正常批次**持有自己的 image/control 强引用**，不是只借用 Sprite 的裸图像指针。batch 绑定会从图形 cache 找到同一个 image，再复制 pair；flush 后 batch 仍可能留在复用结构中，因此 Sprite 析构、批次提交、最后图像释放、GPU 完成是不同时间点。最小 owner 方案仍保守保持 Sprite 到最终原生 helper 正常返回，再在有效 context 下退休，不用一次函数返回推导 GPU idle。

本文 `STATIC_CONFIRMED` 为直接指令／源码事实，`STATIC_DERIVED` 为正常顺序推导，具体窗口、guard 和回调流程为 `DESIGN_PROPOSAL`。线程 ID、OS current HGLRC／HDC、真实 pass、像素和 GPU 结果均未运行观察。

## 证据与地址

活动 IDB 为 `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe.i64`，记录 input path 为同目录 EXE，imagebase=`0x400000`。`RVA = VA - 0x400000`。字段／虚槽 offset 不是 RVA；运行实现应使用实际模块基址加 RVA。未比较磁盘文件身份，未计算或校验任何哈希。

| 本轮主审原始证据 | 范围 |
| --- | --- |
| [primary-render-pass.json](../../../artifacts/cs-t0/tx-03-19-static/primary-render-pass.json) | render owner、最终 flush/present、目标切换／clear、Sprite Render 与 image draw 包装 |
| [primary-submit-flush.json](../../../artifacts/cs-t0/tx-03-19-static/primary-submit-flush.json) | Sprite 逐 track draw、uniform 设置、image→batch 提交、active batch flush、目标栈 |
| [primary-batch-ownership.json](../../../artifacts/cs-t0/tx-03-19-static/primary-batch-ownership.json) | batch 获取／几何复制／GL draw、shader／uniform 版本与特殊 shader 选择 |
| [primary-batch-bind-state.json](../../../artifacts/cs-t0/tx-03-19-static/primary-batch-bind-state.json) | batch 持有图像 pair、状态比较／快照、cache 查找与相关全局 xrefs |
| [primary-context-guard-review.json](../../../artifacts/cs-t0/tx-03-19-static/primary-context-guard-review.json) | 主审独立重取启动、加载旁路、TLS/current、WGL getter 解析／使用和窗口安装 |
| [primary-guard-detail.json](../../../artifacts/cs-t0/tx-03-19-static/primary-guard-detail.json) | 主审独立补取启动到 loading helper、TLS 初始化与临时 current 检查；不把初始化 helper 当 runtime guard |
| [primary-loop-source.json](../../../artifacts/cs-t0/tx-03-19-static/primary-loop-source.json) | 当前源码关键段及一基行号；不是已安装或已执行的证明 |

只读采集源：[export_static.py](../../../artifacts/cs-t0/tx-03-19-static/export_static.py)。`TX0319_OUTPUT` 指定新文件，保留 create-new；源码摘录由读取当前 `loop_runtime.cpp` 的指定行生成。子任务另有独立导出和失败采集分类，见子报告／主审记录。完整导出不表示每个无关分支均已闭合。

| 位置／函数／数据 | VA | RVA | 正常角色／ABI |
| --- | --- | --- | --- |
| 现有外层 render callsite | `0x9312F1` | `0x5312F1` | `E8 CA 42 02 00`，源码 loop 槽 7；只包裹此 caller |
| render owner | `0x9555C0` | `0x5555C0` | 外层源码按无栈参 VoidFn 调用；EBP 等伪参数不作为新增参数 |
| 加载旁路 callsite | `0x8FC32C` | `0x4FC32C` | `E8 8F 92 05 00`，直接调用同一 owner |
| 最终 pre-present 候选 callsite | `0x9560D0` | `0x5560D0` | `E8 AB 30 0C 00`，前置 stack DWORD=1、ECX=renderer |
| 最终候选的 return address | `0x9560D5` | `0x5560D5` | caller 续点，不是独立函数或第二个 Hook 点 |
| 原生 flush/present helper | `0xA19180` | `0x619180` | ECX=renderer，栈上一词，读取其 byte；ret 4，保留原生 EAX 返回 |
| Sprite Render | `0x40A0D0` | `0xA0D0` | ECX=payload，三个栈上 Vec2 指针，ret 12；无可靠整体成功 bool |
| active batch flush | `0xA139F0` | `0x6139F0` | ECX=renderer；提交 active batches，并维护复用结构 |
| batch 绑定 | `0xA66E30` | `0x666E30` | ECX=batch、栈传 image；内部复制图像 pair 与 shader 状态 |
| WGL current DC 函数指针字段 | `0xC75D50` | `0x875D50` | 正常 WGL loader 解析得到的动态指针，不是静态函数入口 |
| WGL current context 函数指针字段 | `0xC75D54` | `0x875D54` | 同上，返回调用线程实际 HGLRC |
| 本线程 current-window TLS 配置 | `0xC74B80` | `0x874B80` | +0 allocated、+4 index；TLS 内容是 GLFW window，而非 HGLRC |

## 1. 为什么选择最终 helper 的 pre 窗口

`9555C0` 首先可能依据 Game 状态直接跳到尾部，不绘制也不到最终 helper；因此它不是每次被调用就一定 present。正常绘制路径从 `A18300(renderer,0,0)` 开始设置目标，按状态／离屏需求压入目标并切换，调用世界／菜单等分派，再按条件做中间 flush、pop target、clear 和屏幕合成。

tail 中 `9560C6: FF 50 14` 是有最终缩放合成时的 image draw；随后所有到达正常绘制尾部的路径汇到 `9560C9: 6A 01`、`9560CB: B9 E0 98 C7 00`、`9560D0`。不能把 `9560C6` 描述成每帧必执行的函数调用，最终 helper 的前置条件仍需每次检查。

`A19180` 在 `A191A0` 首先调用 `A139F0`。之后清本轮 shader／临时记录、复位部分绘制状态并选择默认 shader；当栈 byte 非零时检查 renderer+68：非零走离屏 clear，零才经 renderer+BC window 的 +230 callback 请求 swap。所选 callsite 虽传 1，也不自动证明一定 swap，因此最小 profile 明确要求最终 target=0。

```text
普通主循环已学得主线程与 loop iteration
  -> 现有 RenderHook：进入 Phase::Render，调用原生 owner 一次
     -> 原生各 pass／离屏合成
     -> 9560D0 的未来 wrapper
        -> 检查 owner、context、pass、代次与自有 draw 重入
        -> 对已准备 Sprite 调 Render(position, zeroCrop, zeroCrop)
        -> 原生 A19180 exactly-once：批次提交，正常 target=0 时请求 swap
        -> 正常返回后标记 CPU 使用结束，按 owner 顺序处理退休
  -> 原生 owner 返回，现有 RenderHook 退出
```

拒绝自有绘制时仍调用原生 `A19180` exactly-once；不得为了隔离加载旁路而跳过原有 present 或改写原生 Game 状态。若未来安装该 callsite 槽，必须纳入现有统一安装／回滚／恢复事务，更新其实际容量与健康语义；本轮没有改变源码或当前槽数。

该窗口位于最后合成之后：独立 Sprite 可能叠在 HUD／菜单之上，并绕过此前房间后处理。故它是受限屏幕叠加的最小实现位置，不自动成为 Entity 世界绘制位置。房间内 z-order、阴影／光照和 Player 组合外观仍另行研究。

## 2. guard 的可读输入与不能推导的保证

源码 [InvokeVoid](../../../cs-t0/native/src/loop_runtime.cpp#L484) 先建立 PhaseScope，再决定是否 observe，始终调用 original；[RenderHook](../../../cs-t0/native/src/loop_runtime.cpp#L1099) 只是包裹主循环 callsite。`phase=Render` 也可能出现在尚未 armed 或线程错误的调用里，不能独自作为许可。

[LoopClockHook](../../../cs-t0/native/src/loop_runtime.cpp#L996) 通过时钟 anchor 学习主线程；[EstablishOrCheckMainThread](../../../cs-t0/native/src/loop_runtime.cpp#L385) 在主线程仍为零时让普通 Hook 直通，不由 RenderHook 自己学习。`CurrentObservationContext()` 的 thread_id、main_thread_id、phase、phase_depth、loop_iteration 是本地观察信息；`CompanionHealthy()` 反映现有 observer 的 armed／unloading／unsafe／mask／事件健康，不含图形 context 检查。源码具备这些读取入口，不等于本轮读到了它们的运行值。

| 检查组 | 最小候选需要的实际信息 | 不满足／未知时 |
| --- | --- | --- |
| C/S 与调用来源 | C/S 展示 owner 有效，observer 健康，非零 learned main_thread_id 与当前线程相等，当前普通 outer-render token、phase/depth 匹配 | 不执行自有 draw；原生调用照常 |
| 代次与重入 | session/run/room/context generation 和本地实例仍有效，自有 draw depth=0，当前不是 loading／teardown／未知 pass | 停止对应发布；不靠旧 pointer／句柄复活 |
| renderer／后端 | initialized、当前 window、已解释 WGL callbacks 和有效 owner stamp；不是仅 renderer 全局地址非零 | 不主动切换 context 或初始化后端 |
| TLS current window | TLS allocated/index 有效，以本线程 TlsGetValue 读到的 window 等于 renderer+BC | 返回未知／不匹配，不调用 make-current setter 修复 |
| OS current | 正常已解析 WGL getter 当前返回的 HDC／HGLRC，与同一 live window+264／268 非空句柄一致 | 不执行自有绘制；没有当前值就保留 UNKNOWN |
| 最终 pass | target=0、目标栈和 shader／颜色栈符合所声明 pass，镜像／Game offset／坐标环境受支持 | 该 profile 不支持，不能清引擎全局伪造符合 |

`A7FCE0` 在 `A7FD99/A7FDAB` 分别写入 `C75D50/C75D54`，并在 `A7FE68/A7FE71` 调用它们保存旧 current。它们返回调用线程的 OS current，是未来只读 guard 的候选入口；本轮没有执行 getter。静态 FF 字节和相同字段名不是有效运行函数指针，必须先有正常 WGL 初始化与明确函数解析 owner。

初始化状态也应分开：`A19102: 83 4E 04 01` 设置 renderer+4 bit0（该对象为 C798E0 时，flags 位于 C798E4）；此前 `A190F9` 对 C37984 的 bit0 写入属于图形资源/cache 相关状态，不能单凭后者命名 renderer initialized。两项正常写入均不能代替 current 检查。

`A6BBB0` 与 `A19DF0` 的 TlsGetValue 之后会解绑／绑定窗口，**二者不是纯 getter，不允许为检查而调用**。`A7FA50` 正常 make-current 成功后写 TLS；失败／外部直接 WGL 调用可能令 TLS 与 OS 状态不一致，所以二者需要分别读取。仅字段相等也不证明句柄从未被删除／复用，context generation 不能只由 `(window,HDC,HGLRC)` 相等推出。

普通启动的 `8FB120 → 8FC32C → 9555C0` 不经现有 RenderHook；未来内部槽若不检查来源便会一起拦到它。phase/depth 对已见普通路径有区分价值，但不证明所有动态／递归调用均被排除：最小 owner 还需自己的 outer-render token、draw depth 和 loading／generation 状态。这些 guard 均为待实现设计，没有把调用次数或 render iteration 当 Authority tick。

renderer target 镜像为零也不证明任意 Mod 的裸 GL framebuffer／viewport 状态一致；本候选限定正常引擎受控 pass，任意外部 GL 改写不在支持范围。它未成为全场景 context／pass 安全检查器。

## 3. Sprite 参数与坐标语义

`40A0D0` raw 从 `[ebp+8/+C/+10]` 取三个指针，转给 main／overlay state；末尾 `40A133: C2 0C 00`。receiver 是独立 Sprite payload，参数依次是 draw position、两组 crop 的本地 Vec2 地址；不是直接传两个 f32，不传 Lua wrapper，不把 EBP 伪参数当实参。三个向量必须在同步调用期间有效，值受支持且有限；零 crop 应使用明确的本地 `{0,0}`，不把未知全局向量当前值当作零。

该 position 沿当前 pass 进入 Sprite 几何和 renderer 的坐标处理，并非一个无条件的“窗口物理像素”API。`A671B0` 会根据 render target／renderer flags 做纹理坐标处理、像素对齐和额外偏移，还递减 renderer 的逐 quad 深度字段 `C798FC`；tx-03-16 中的颜色栈、镜像及 Game offset 依赖仍在。首个实现应声明屏幕逻辑坐标和视口转换，不把 Authority 世界坐标直接塞入这里，也不盲目清全局偏移。

`40A0D0` 只用 +109 允许分派，不做 tx-03-18 的最终资源／姿态完整验证；native void 正常返回更不证明绘制了非空像素。候选需是此前已准备、未过期的固定姿态对象，本窗口只消费它，不调用 Update／SetFrame／游戏事件或 Entity 工厂。

## 4. 批次持有图像与延迟提交

已读 PNG slot14 路线为 `A128F0 → image slot10(A129A0) → A13750(batch) → A671B0(复制几何)`；indexed 图像另有 shader push/pop，本轮最小样本仍须选择明确支持的 PNG 资源。Sprite 栈上临时位置、quad、颜色数据由提交链复制到 native buffer，当前正常路线不把这些栈地址直接延迟到帧后使用；异常分配／所有 backend 不由此证明安全。

`A66E30` 为 batch 绑定 image 时，先按 image+48 在全局 graphics cache 中查找，并确认节点 image 是同一地址。命中才经 `40C550` 复制缓存 pair；未命中产生空 pair，不盲目 retain 任意 raw image。随后 `A66F74: 8D 4E 08`、`A66F77: E8 34 54 9A FF` 经 `40C3B0` 将 pair 关联到 **batch+8 image／+C control**。因此该正常成功路线有独立强引用；同时也说明 Sprite final-pair loaded 不单独证明 graphics cache/batch 绑定条件成立。

batch+10..20 复制当前 5-word 绘制状态，+24 在 `A6701F/A67024` 从全局当前 shader `C379B8` 保存指针，+28..30 保存 uniform 版本索引向量；`A670C0` 以这些信息判断已有 batch 能否复用。Shader／uniform 数据并非读取时就直接产生像素，已排队批次会使用其对应状态版本。

`A139F0` 遍历 renderer+80..84 active batch vector，在 `A13A32` 调 `A67BD0`。后者按 batch 图像调用 bind、设置 blend/shader/buffer，在 `A67E41` 调 `epoxy_glDrawElements`，正常尾部推进 buffer 使用标记、清 batch 活动位。`A13A8B` 重置 active vector 的 end，后续维护复用结构；已读正常路径没有把每个 batch+8/+C 都清成空 pair。图像可能随 batch／cache 保持更久，不能从 Sprite.Reset 或一次 flush 断言纹理立即销毁。

这补充了 tx-03-18 的“无在途使用”：既要解释自有 callback／registry 借用，也要解释已排队 native batch 与它自己的引用。最小方案保持 Sprite 到原生最终 helper 正常返回，随后结束该次 CPU 使用；不根据此刻 GPU 是否完成裁决退休，也不手工释放 batch、删图形缓存或 glFinish。图像最终销毁仍走原生强引用规则和有效 context 内的正常 owner 顺序。

## 5. 状态恢复与操作范围

Sprite draw 不是完整 save/restore。`409120` 正常尾部恢复其保存的 `C79900..C79910` 五个 DWORD，但随后 `409873` 调 `A14050` 选择命名的默认 `KAGE_ColorTextureShader`；它不恢复任意进入时 shader。`A140A8` 将找到的默认 shader 写到 `C379B8`。shader uniform 版本、batch 队列、逐 quad 深度和部分 GL 绑定也会变化。

因此首个候选只接纳**当前 shader 已符合默认尾部 pass、shader／颜色／目标栈符合声明且特殊 flags=0**的场景，正常尾部继续由原生 `A19180` 提交、清本轮记录并恢复其定义的默认状态。不把“正常返回恢复五个字”扩大为任意 renderer 状态均被恢复。

不要 memcpy 整个 renderer／shader 对象或回退 active vector end、uniform 历史／版本索引、顶点游标和 `C798FC` 来伪造无副作用；这些数据包含本次真正提交的绘制结果。当前 shader／栈不符合受限 profile 时跳过自有绘制，不在未知条件下强行覆盖全局。外部任意 shader/pass 的透明保存恢复不属于本轮已解决能力。

在这个最终 pre-present 窗口只调用已准备对象的同步 Render，**不同时做 Load／Reload／换图／Reset／析构**。这些操作可改共享 sampler、GL 绑定和 cache，并可能触发最后释放；不能仅因当前 context 符合就混入仍有旧批次的窗口。资源准备和退休安排在独立的有序 owner 位置，检查队列／引用及 context 后再进行；实际接入代码和 E2E 仍待实现。

正常使用结束后，先撤销新的自有 draw／发布，等待自有使用结束，再按 tx-03-18 的稳定 cache/context 条件退休。退出时仍须早于正常图形缓存／window context teardown；不能把 StopObserver 的通用 Hook drain 当作图像资源退休调度，也不能在 context 丢失时盲目析构。EH／OOM、GL 错误、worker 并发及重入异常保持 UNKNOWN。

## 完成范围与下一步

本轮已把最小屏幕 Sprite 对应到一个精确 callsite、两类 ABI、guard 的真实数据来源、batch 的强引用／状态版本和正常 final flush 顺序。这个候选仍未安装或运行，不能据静态位置宣称像素、GPU、房间遮挡或完整逻辑／展示隔离已完成。

下一轮建议完成**具体资源与姿态闭环**：确定首个可信本地 ANM／PNG 清单，解释 Authority 同窗口采集的资源名／main-overlay key 向量，再收敛已有实例连续姿态更新及资源更换的写入／失败顺序。随后进入最小 adapter 实现；剩余正常使用／退休缺口在该具体样本中补齐，不泛化为全部 Player／特殊视觉或完整异常闭包。

唯一一次静态文档验收见 [主审记录](tx-03-19-primary-review.md)和 [final-static-review.json](../../../artifacts/cs-t0/tx-03-19-static/final-static-review.json)。既有生产代码、机器 Hook 索引、IDB、历史失败及 C/S 与原版 Online／Isaac Sync 的隔离保持原状。
