# Slot 机器实体：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

本模块保留当前静态RTTI与最初虚槽入口，避免把“当前min-read-v1只专门处理七类Entity”误写成游戏只有这些类型。该类没有独立的专用宿主Hook，也没有完整的专用字段采集/写回规格；共享Entity观测可能看到其基类头，但不能据此声明该类可完整复制。状态同步遇到尚未支持的gameplay类型时，报告UnsupportedClass并停止相关Replica展示或请求完整恢复，不静默按NPC/Effect解释。

## 源码声明的补丁、调用依赖与自有表

当前没有本模块专用的源码安装点；共享路径见[Entity](entity.md)、[游戏循环](game-loop.md)。下列逆向入口不因此升级为已安装Hook。

## 当前 RTTI 与虚表候选

来源：[tx-01-64 RTTI原始记录](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json)。槽0..3只按原始表登记；Grid槽语义与Entity不同，不统一命名。下列槽并非全部被patch。

| 类名 | vtable RVA | TypeDescriptor IDA VA | slot0 / slot1 / slot2 / slot3 原函数RVA |
| --- | --- | --- | --- |
| ??_7Entity_Slot@IsaacRepentancePlus@@6B@ | 0x764c50 | 0xc5e5c8 | 0x0025D130 / 0x0025D200 / 0x002AE7D0 / 0x0025D950 |

## 已知静态函数入口索引

目前没有额外的独立函数导出；已有虚表/安装点已在上文列明。未知入口不填猜测地址。

## 证据与限制

[最近固定点位与ABI复核](../refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../refactor/project-conventions.md)。全量历史研究保留于 `docs/archive/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。
