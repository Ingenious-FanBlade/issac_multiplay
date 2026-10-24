# Laser 激光：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

当前已保留Laser RTTI与初始虚槽。Laser.Timeout +0x458有具名绑定依据，但未进入当前min-read-v1。它不属于Tear或Projectile，持续激光与蓄力武器的长度、角度、父子关系、动画和伤害字段尚未形成完整副本规格。当前无独立Laser宿主Hook；首版支持哪些Laser variant需在实现任务中明确，不能把此文档当成已支持全部武器。

## 源码声明的补丁、调用依赖与自有表

当前没有本模块专用的源码安装点；共享路径见[Entity](entity.md)、[游戏循环](game-loop.md)。下列逆向入口不因此升级为已安装Hook。

## 当前 RTTI 与虚表候选

来源：[tx-01-64 RTTI原始记录](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json)。槽0..3只按原始表登记；Grid槽语义与Entity不同，不统一命名。下列槽并非全部被patch。

| 类名 | vtable RVA | TypeDescriptor IDA VA | slot0 / slot1 / slot2 / slot3 原函数RVA |
| --- | --- | --- | --- |
| ??_7Entity_Laser@IsaacRepentancePlus@@6B@ | 0x77cd40 | 0xc5d740 | 0x0056CCF0 / 0x0056F590 / 0x002AE7D0 / 0x00577CC0 |

## 已登记字段

字段位置可读不等于可安全写回。RAW保留原位串；bool为byte，引用不发送裸指针；专用字段需满足初始化与当前generation读取依据。

| 字段 | 对象相对偏移 | 字节 | 证据范围 |
| --- | --- | --- | --- |
| Timeout（仅Lua绑定研究，未进入min-read-v1） | 0x458 | 4 | STATIC_CONFIRMED_BINDING_ONLY |

主要依据：[实际采集Spec](../../cs-t0/native/src/world_record.cpp)、[偏移修订](../../docs/refactor/cs-t0/t0-06/tx-01-69-offset-reconciliation.md)、[读取与引用规格](../../docs/refactor/cs-t0/t0-06/tx-01-71-sampling-design.md)。派生Entity共享字段另见[Entity](entity.md)。

## 已知静态函数入口索引

目前没有额外的独立函数导出；已有虚表/安装点已在上文列明。未知入口不填猜测地址。

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。
