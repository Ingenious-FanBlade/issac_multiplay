# tx-04-08 录制状态归属修复

日期：2026-10-29，Asia/Singapore。继续同一 tx-04-08 的完整目标：修复 e2e-11 的异步录制等待错误，然后由统一入口执行一次整体 E2E，保留原失败和全部未覆盖项。

## 失败与修改

e2e-11 的新 start 已回复 request71 / generation2 / Preparing，但下一次 status 仍是旧 primary 的 request1 / generation1 / Closed。旧等待只检查状态，因而提前失败；新录制随后实际进入 Recording。见[原执行记录](../../../artifacts/repentogon/tx-04-08-e2e-11/execution-notes.md)。

`tools/harness/src/tx08.rs` 现在将 start 回复的 request、generation、recording 传入等待；旧代次（包括尚无身份的 Off）只能继续等待，不能成功或触发新录制终态失败。相同代次的身份冲突、被更新代次取代、当前录制终态与超时仍明确失败。所有 primary/comparison/fault/healthy/continue/exit-menu 入口共用一次 start，不重发。stop 用其回复的 generation/recording 等待 Closed，不把 stop 请求编号误当 start 编号。每次成功等待保存 admission、终态和旧快照次数，原始控制口记录继续完整保留。

静态读取发现工作区旧 `tx-04-08-defg-build` 目录已删除。源码冻结改为复制本次 `tx-04-08-recording-wait-build` 的真实构建日志，避免在游戏启动前因历史日志目录缺失中止；不恢复用户删除的历史文件。DLL 本轮未修改，仍冻结实际部署二进制。

## 上次 UNKNOWN_CLASS 的归属

直接读取 e2e-11 原始 captures：28 次均为同一 Entity 域 local_id119 / use1，Type6 / Variant19 / SubType0，vptr01675bdc；首次对应 attempt2668。出生描述同为 Type6 / Variant19。这是 Laser 类型，属于本轮明确不提供专用 schema 的类别。采集保留 UnknownEntity 和已确认公共头，不删除对象、不补造字段或完整资格；相关不完整样本仍保持不完整。此判断没有运行游戏或重新执行离线 CLI。

## 本次执行约定

用户本会话明确要求修复后直接测试、不再询问准备。保留逐启动说明和启动文件记录，由 Codex 按该指示写入当次 READY；每次启动前仍告知停止手动输入，结束后告知恢复。场景保持主250 / 对照32份完整普通采集、30秒采集上限；两次顺序启动、故障注入、Continue、直接退出和离线归约均属于同一统一入口。分析修改期间不运行测试，不新增单元测试；纯编译后才执行整体 E2E，失败不自动重跑。

实际编译、运行和清理结果另记新 artifact，本文不预记通过。

实际结果：[e2e-12](../../../artifacts/repentogon/tx-04-08-e2e-12/execution-notes.md)。最终Rust纯编译17.47秒成功；14次start都跳过1次旧快照并进入正确Recording，主稳定段253/250通过。整体因Continue后Player替换夹具超时而FAIL，thread故障归约另有首因不符；完整报告已生成，环境恢复无错误，未启动第二进程或自动重跑。
