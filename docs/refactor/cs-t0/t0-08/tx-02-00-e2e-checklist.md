# tx-02-00：T0-08 操作5–9 E2E清单

当前验收结果：**tx-02-04 唯一整体 E2E 33/33 PASS**。初始4步和Authority激活后的连续64步W/D/S/A均真实发送并全程前台；A有646/6269个非零PhysicalSample和真实位置变化，trace56,070/lost0，6,268 commit逐条ACK/STATE/身份配对，B outage/恢复、3/3 Drop边沿、70槽恢复和正常stop全部通过。见[tx-02-04结果](tx-02-04-results.md)和[owned输入实现](tx-02-04-owned-physical-input.md)。tx-02-03外部输入接力超时（live=NOT_RUN）、tx-02-02的29/31、tx-02-01的23/30及tx-02-00失败均保留，本清单不放宽失败项。

历史tx-02-00第三次实际执行（2026-10-20）：用户确认继续并手动聚焦游戏后，已进入真实Authority，但随后因`protocol trace overflow`失败，事件保存至commit20、协议312条，未到达outage。正常停止helper返回成功，逐槽恢复产物未落盘。前两次无进程/输入等待超时原样保留，见[历史执行记录](tx-02-00-results.md)。下文初次结果及设计步骤仅保留历史语境。

首次执行历史状态：实施前建立，第一次统一执行离线19,022/19,022 PASS，枚举isaac-ng.exe为空，整体FAIL、实机NOT_RUN。2026-10-20，Asia/Shanghai。后续用户授权的执行见上文及[实际结果](tx-02-00-results.md)。下列步骤保留执行设计，不表示完整实机清单已经通过。

一次统一入口覆盖以下项目，保留逐项PASS/FAIL/NOT_COVERED及原始包、实际事件、源码、二进制、构建日志、工具链、配置、复现说明。用户准备全新隔离游戏后才执行；不提前probe、不自动重跑。无哈希，包CRC保留。

| 项 | 场景/判据 | 必须保留的依据 |
| --- | --- | --- |
| P01 | 新wire版本/常量、4类独立golden字节与生产codec一致 | 独立Python参考构造、原始golden、C++解码/重编码 |
| P02 | 每种golden逐长度截断、扩展、每位翻转、旧版本、未知kind/flag/reserved、wrap和固定seed fuzz均严格拒绝或分类 | corpus输入/预期类别/生产返回/输出不变 |
| P03 | 一包最多4条INPUT，整包先验证，旧到新共用生产Participant准入；重复不重复边沿 | source/packet/input identity、实际Decision/Resolve/End |
| P04 | 丢包冗余恢复、重复、乱序旧包不覆盖；非法冗余不提交合法前缀 | FaultTransport发送/入队/丢弃/到达和准入事件 |
| P05 | ACK/STATE独立lane最新消息与完整identity；同lane进度回退拒绝，中性source0保留 | 双端状态、逐lane最新包和拒绝原因 |
| P06 | 固定seed延迟/抖动/loss/duplicate/reorder/outage可复现；容量满不消耗待发送序号 | 精确transport轨迹、容量边界、原始包配对 |
| P07 | 退休/失败无复活，generation变化不接纳旧包，输入预算不足不部分提交 | 协议/Participant生命周期与未覆盖边界 |
| L01 | 唯一批准PE、新鲜进程、安全新局/只有A，无冲突模块；用户准备确认 | 当前用户确认、唯一入口的真实进程/PE信息 |
| L02 | 显式IPC扩展冻结协议配置，双HELLO/首ACK成功；不代替exact pair | Native配置、原始握手、真实Joined/Binding |
| L03 | B脚本意图经wire/内存FaultTransport/完整decoder/生产AcceptInput后才Resolve；A/B同首个+1，后续lead8，迟到不重定向 | Send→Enqueue→Deliver→Receive/Rejected，逐条64字节input→Admission→Submit及完整身份/状态/Decision；每generation一次startup意图 |
| L04 | loss/duplicate/reorder与指定outage真实作用于协议链，B缺输入中性/恢复，A/tick继续 | transport轨迹、B availability/Resolve、A Resolve与位置 |
| L05 | B边沿最多一次，冗余/dup不重复；原始来源/accepted/processed可配对 | source sequence、edge进度及wire ACK |
| L06 | 每个真实commit各发ACK/STATE，远端两lane实际接受有效包；state为明确gap | 全部receipt进度、拒绝计数/掩码/会话状态与ACK对应；每条STATE与真实capture/gap身份对应 |
| L07 | 正常stop、输入/publication关闭、日志drain、70 Hook恢复、DLL常驻且进程健康 | restoration、controller状态、trace终止 |
| L08 | 清单逐项总结，失败/未覆盖透明，不外推Steam/双游戏/Replica | 统一result.json与逐项证据路径 |

真实输入仅使用短、有界移动和无资源消耗的Drop边沿；不使用Bomb/拾取/退出run/解锁等额外持久动作。状态观察以真实采集的缺口元数据为准；没有完整字段来源时不构造原始值。

执行步骤：

1. 用户告知已准备全新隔离`isaac-ng.exe`，位于安全新局，只有键盘A，无原版Online/Isaac Sync、调试器或旧T0 DLL。
2. 启动[统一入口](../../../../cs-t0/accept-protocol-e2e.py)，命令见[实现说明](tx-02-00-implementation.md)。只执行这一次；离线部分失败则不接触游戏，不自动重跑。
3. 出现`PROTOCOL_E2E_PHYSICAL_INPUT_WINDOW`后，用isaac_test聚焦游戏并执行一次短闭合W/D/S/A前台检查，将实际工具结果写入artifact内`physical-input-ready.json`。controller随后启动。
4. 收到新的`PROTOCOL_E2E_CONTINUOUS_INPUT_WINDOW`（表示真实Authority commit已出现）后，立即按`physical-input-continuous-request.json`的64步执行连续W/D/S/A，并把实际工具结果写入`physical-input-continuous-result.json`。该连续窗口必须覆盖tick160..179的outage；不能提前运行，也不能在stop后继续。
5. 执行器等待连续输入结果有效、B脚本结束且最后source target后16个真实tick，再发送一次stop。超时/失败同样请求正常stop，不杀游戏、不强制卸载DLL。
6. 同次执行产出`result.json`、`offline-checks.jsonl`、`offline-trace.jsonl`、`live-checks.json`、`packet-pairing.json`、`capture/`原始事件/包/world/恢复记录，以及源码/二进制/构建日志/配置。人工清单和结果说明按实际事实补充；未运行、失败及未知不写PASS。

本次真实拓扑为一个游戏Authority加同进程自有逻辑Remote；不声称socket/Steam、两个游戏、可玩Replica或跨进程世界相等。
