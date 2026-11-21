# Hook 安装与恢复：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

所有宿主patch由唯一owner协调，不能在同一位置重复安装不同family。RelativeCallPatch保存5-byte E8及目标，PointerPatch保存原指针和明确owner，EntryPatch保存完整指令跨度并用trampoline；原生字节/ABI保护仍是功能行为，不计算任何哈希。

观察wrapper original exactly-once；C/S主动Shooting抑制是已明确的功能例外，受阻断时原consumer调用0次，不把它报告为观察wrapper。恢复关闭publication/input，退休关联，处理在途callback，逆序恢复；未知ABI、错误线程和不确定恢复停止相关修改，不盲目重注入。设备/DLL按NEVER策略常驻，不通过强卸载完成“清理”。

主要实现：cs-t0/native/src/hooks.cpp、loop_runtime.cpp及world_e2e_bridges.inc。函数地址依赖当前目标构建，静态源码常量不能证明当前进程正在安装，也不能证明异常/worker路径安全。

## 源码声明的补丁、调用依赖与自有表

当前没有本模块专用的源码安装点；共享路径见[Entity](entity.md)、[游戏循环](game-loop.md)。下列逆向入口不因此升级为已安装Hook。

## 已知静态函数入口索引

目前没有额外的独立函数导出；已有虚表/安装点已在上文列明。未知入口不填猜测地址。

## 证据与限制

[最近固定点位与ABI复核](../refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../refactor/project-conventions.md)。全量历史研究保留于 `docs/archive/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。
