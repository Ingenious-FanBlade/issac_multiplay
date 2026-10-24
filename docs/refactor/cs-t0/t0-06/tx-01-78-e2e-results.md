# tx-01-78 运行结果：外部观察器错误排除了 USER32 IAT 槽

日期：2026-10-19。用户明确同意修正模块识别后执行一次新整体验收。本次 PSAPI 路线已识别主模块，保存了实际完整模块清单：isaac-ng.exe base=7602176、size=9428992。接着在注入前的原字节采集阶段报错 **Pre-injection pointer is outside the game image**，统一结果 FAIL。

这是外部观察器的错误条件：70 槽不仅包括游戏内部调用/入口/虚表，还包括 USER32 的 PeekMessageW、TranslateMessage、DispatchMessageW IAT 槽。源码 loop_runtime.cpp 的 RVA 为 0x718714、0x718650、0x7185B8，其原生 owner 条件明确为 USER32.dll；观察器却要求所有 4 字节值落在游戏主映像。原有恢复目录也含三个系统模块地址，此前静态审阅遗漏了这一点。

本次仍在 Popen 控制器子进程之前失败，没有注入、安装 Hook、执行拾取/暂停/失联，没有新的应用用例 PASS。19 项 PASS 均保持历史 CARRIED_FORWARD。原始 [FAIL 报告](../../../../artifacts/cs-t0/tx-01-78-live-remaining-01/result.json)、[实际模块清单](../../../../artifacts/cs-t0/tx-01-78-live-remaining-01/captures/primary/external/module-inventory-before.json)与收尾记录保持不变，没有自动重跑。

后续修正按已存在的原生每槽 owner 规则识别这三处 IAT，保留游戏内指针条件，增加逐槽 owner 诊断；每个进程恢复均和其本次注入前原值比较，不用旧进程系统 DLL 绝对地址作当前 baseline。见 [tx-01-79 修正说明](tx-01-79-iat-observer-fix.md)。用户同意后再执行新的唯一一次整体验收。
