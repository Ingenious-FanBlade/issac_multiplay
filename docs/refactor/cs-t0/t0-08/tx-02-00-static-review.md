# tx-02-00：静态分析与主智能体审阅

2026-10-20，Asia/Shanghai。按用户指定使用同一个GPT-6 Luna / max子智能体只读分析与审阅，主智能体复读、判断和修正。没有运行测试、游戏或输入；纯编译另记。

| 发现 | 处理 |
| --- | --- |
| B最窄接点是Draining，ACK来自TopExit receipt | 保留原scheduler/原生owner，不增加Hook；B Pump在BeginSimulation前，wire publication在真实commit后。 |
| 不应重用ScriptStep故障位/SharedV5 | 新增显式IPC v6、冻结FaultConfig、独立trace ring，旧入口默认不启用。 |
| 迟到脚本项可能绕过wire直接Submit | 迟到项按原target进入SendInput及完整wire，由共同validator判过期，不滑动edge。 |
| lead8不会清B startup_pending | 用户明确选择A/B同首个+1，B迟到正常过期，后续lead8。改为源端每generation一次startup标记，两条意图均走wire；不由接收端pending补发。 |
| 旧代包需在Endpoint.Receive前拒绝 | Pump带当前generation预筛，换代清队列/history，不重置session/sequence。 |
| ACK缺generation单调/可能退回Warmup | 增加generation非降，Progress只允许Active及后续；mask累计、计数和字段约束明确，source0合法。 |
| raw memcpy不能称共同Encode | 合法A/B在共同Submit调用V2 Encode，历史非法故障才保留raw字节供原validator拒绝记账。 |
| receipt缺run/room/token，token在outer后清零 | Capture保存真实post身份，accessor核对expected tick；不填常数或读取已清零token。 |
| Complete不能误称NotReady | 新增明确reason19 ValuesNotExported；真实gap及odd原因保留，无原始值/完整世界声明。 |
| 最后丢失edge不能凭无后续包恢复 | 文档限定unreliable；场景有后续输入和末段中性，区分交付与最多一次处理。 |
| transport没有损坏动作 | bitflip/truncate/extend仅由同次corpus验证，实机只覆盖实际fault profile。 |
| Submit仅按序号/target配对会漏掉move/edge或身份变化 | 协议Submit保存session/client/提交generation；报告有序比较完整64字节、generation、tick、Decision，并与实际Receive中逐条payload一致。 |
| 缺ACK或只有一条STATE仍可能通过旧报告 | 按每个commit核对ACK/STATE Send覆盖；ACK拒绝计数/掩码/会话状态独立重建，STATE逐一对应真实capture/gap，另要求远端两lane实际接受有效包。 |
| Deliver无Receive、Enqueue身份/时刻未核对 | 核对Send→Enqueue→Deliver→Receive/Rejected完整对偶，INPUT全部记录恰有Admission；关闭恰一次，队列/已投递结果全部收尾。 |
| 物理输入提示先于controller，断网窗口可能没有A动作 | 实机阶段以artifact记录同步工具动作开始，默认2秒后启动controller；明确持续覆盖tick74..83，保存工具请求/结果。 |
| 离线production标签可能误指完整pipeline | 场景标签改participant；说明离线typed Participant接口，生产pipeline bytes路径由同次实机阶段覆盖。 |
| 按tick转dict会掩盖重复commit/receipt，startup仅保存raw不能核对代际 | 保留原始行计数并单独检查tick唯一；startup记录首次Send的generation/tick并逐条匹配，Submit当前generation也与提交参数一致。 |

主智能体另复读wire偏移/长度/CRC/输出原子性，trace published→consume顺序与Rust布局、预算、正常stop/session退休/失败及常驻恢复。最终`out/cs-t0/tx-02-00-build-ready-02/`的Rust/Native/Helper/Scenario纯编译成功、退出码0；未执行产物。子智能体只读审阅已收束；静态结论不代替运行证据，尚未运行本次唯一验收。

world逐commit覆盖包含真实capture和gap；若backpressure、attempt耗尽等使元数据存在而world行缺失，覆盖检查必须失败，不能放宽成“有若干条匹配”即通过。实机输入拟用64步W/D/S/A、每步150ms、无额外间隔（共9.6秒），与ready记录同步开始；若初始化耗时导致未覆盖目标窗口，按实际失败记录，不自动重跑。
