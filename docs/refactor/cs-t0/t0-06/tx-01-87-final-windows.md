# tx-01-87：修正剩余汇总并补真实加载与 run 退出窗口

2026-10-19。目标仍为当前授权的六项剩余 E2E；修复 tx-01-86 的 schema/事件关联缺陷，在同一统一入口读取已保存原始材料，只新采真实 Continue 加载和主菜单 run 退出。Native/controller/Helper/Mod 均不修改或构建，复用 tx-01-86 E2E Native 和 fixture v1.3；不重复三条已实际成功消费的故障、不覆盖任何旧 FAIL，不恢复用户排除的 replay/Online-Sync。

## 修改前登记的失败方式

- 未知引用的 target 是可省略字段，直接索引会使完整报告丢失；只读 get 保留未知语义，不修改原记录。
- reason 必须使用生产编码的 BUDGET_EXCEEDED/UNKNOWN_CONTINUITY 等实际名称；不因汇总拼写错误漏掉真实失败分支。
- 派生 Init 传给 Base Init 的参数不必包含原请求 seed。短命对象必须按实际 Spawn 参数 words[6] 的 seed、该次正常 EAX 返回、同 outer 的真实出生事件关联；参数的 subject=0/1 副本不能双计。
- 普通门换房没有 AlternateStateApply，不可标为加载。主菜单注入后由用户手动 Continue，实际 state apply/owner 返回才产生 Loading/LoadingExit；参照已观察的 tx-01-55 主菜单 Continue 方法。
- 不能沿用“启动时 A 已存在”的说明来跳过真实加载；新段初始明确没有 A/B，真实 Continue 后才等待唯一绑定和单次原生 Join。
- 旧 primary 已有暂停、odd 与换房配对；新段补实际加载和 run 退休。两个进程的 epoch/身份不强配，分别分析完整窗口再按事实合并覆盖，不重跑旧暂停场景。
- 主菜单退出必须在控制器自动 30 分钟停止前完成并立即正常 stop，保存实际 run enter/exit、退休后不再采集和 70 槽恢复；不能再用旧进程补记录或改控制器时间来隐藏旧缺口。
- 所有分析修正和场景编排先完成，准备过程零测试。只执行一个新统一入口；失败继续保留，不自动重跑。

实现已完成并静态阅读；没有运行 Python、Lua、试跑或其它测试，没有重新构建 Native。新入口为 `cs-t0/accept-world-final-windows.py`，保存旧 tx-01-86 FAIL 副本，分别输出旧 primary 的修正只读分析和新 Continue 窗口证明；只有两组真实窗口和历史/新事实合并满足要求，才给新账本判定。

准备时唯一整体验收未开始。静态依据：tx-01-55 真实 Continue 已记录 caller RVA559B26/owner return RVA554D63；当前代码仍只在实际 AlternateStateApply 后打开加载窗口。普通 owner 空调用不能充当加载证据。

运行更新：同日唯一统一入口已执行，授权范围 6/6 PASS，累计 31 PASS / 2 项用户排除 NOT_COVERED；真实加载、run 退出和 70 槽恢复已记录，见[最终结果](tx-01-87-e2e-results.md)。本段不改变上述准备阶段的零测试事实。
