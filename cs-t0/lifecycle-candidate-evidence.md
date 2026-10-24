# T0-02 run lifecycle candidates

状态：`RUNTIME_OBSERVED_OFFLINE_C1`（信号级；单个静态 target 仍为 `UNKNOWN`）。本页登记 2026-08-19 对固定 x86 patched variant 的 x32dbg 只读静态调查结果；调查期间目标保持 running，断点和 patch 均为零，没有 pause、单步、trace、注入或写内存。`Isaac Callbacks.svg` 只用于选择搜索方向，不作为当前构建的地址证据。

## 静态候选

- `PostGameStart` dispatcher 为 RVA `0x464EE0`，callback id `0x0F`，callee-clean、一个 dword 参数。完整 `.text` 中共有六个直接 caller：RVA `0x2F580B`、`0x2F61FF`、`0x2F72C9`、`0x2F7721`、`0x2F7A10`、`0x559B82`；其中四条传 `0`、两条传 `1`。
- `PreGameExit` dispatcher 为 RVA `0x4650A0`，callback id `0x11`。它在 run teardown RVA `0x2FA0C0` 中只有一个直接 call，RVA `0x2FA0E2`。
- teardown 在调用 `PreGameExit` 前检查 `[g_Game+0x2658A] != 0`，正常返回前写 `word [g_Game+0x2658A]=0`。`g_Game` slot 为 RVA `0x871678`，不能与已否定的 manager slot RVA `0x87169C` 混淆。
- teardown 的完整直接 caller 集合为 RVA `0x559696` 和 `0x5AB84A`；没有发现绝对函数指针 owner。两处都传一个 dword，并收敛到同一个 `ret 4` callee，适合事务化改写为共享 passthrough wrapper。
- `g_Game+0x265C0` 会在多处清零、自增并与 `8` 比较，不是 generation counter。
- `PostGameEnd` 只表示死亡/胜利；死亡画面仍可能继续 game logic，不作为通用 retirement 信号。

每个目标的原始字节、masked feature、call target、ABI 和证据标签以 `ledger/isaac-cs-t0-mode-ledger.json` 为机器可读来源。所有 feature 已在固定 PE 的完整 `.text` 上验证唯一匹配一次。

## 待验证不变量

后续无断点 smoke 必须证明：New Run 恰好一次 start 且参数为 `0`；Continue 参数为 `1`；普通返回菜单恰好一次 teardown；teardown 前 game object 不变且 active 非零、原函数返回后同一对象 active word 为零；pause、切房、换层和死亡画面不产生 teardown；第二局产生更大的 generation；lifecycle 与 simulation callback 在线程和顺序上互证。任何目标漂移、重入、参数变化、非单次原调用、active 转换不成立或事件缺号均失败。

Offline C1 已直接验证两次 New Run start、一次普通返回菜单 teardown、active `1 -> 0`、第二 generation 递增、同 simulation 线程、事件连续和单次原调用。尚未覆盖 Continue、pause、切房、换层、死亡画面以及 OriginalOnline；共享 wrapper 也没有记录具体命中的六个 start caller，因此这些目标及未覆盖语义继续保持 `UNKNOWN`。
