# Effect 效果实体：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

Type=1000。effectEL由空间/碰撞处理产生，是派生子集；不能用可见性、列表名或“看起来像特效”排除gameplay影响。State/Timeout/LifeSpan的raw位置已登记，计时语义和所有variant行为未穷尽。

首版对未证明纯展示的Effect按权威实体处理。只有明确不参与碰撞、RNG或规则的音效/粒子才可走可丢弃表现事件；客户端不得因事件再生成带gameplay副作用的原生Effect。

## 源码声明的补丁、调用依赖与自有表

当前没有本模块专用的源码安装点；共享路径见[Entity](entity.md)、[游戏循环](game-loop.md)。下列逆向入口不因此升级为已安装Hook。

## 当前 RTTI 与虚表候选

来源：[tx-01-64 RTTI原始记录](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json)。槽0..3只按原始表登记；Grid槽语义与Entity不同，不统一命名。下列槽并非全部被patch。

| 类名 | vtable RVA | TypeDescriptor IDA VA | slot0 / slot1 / slot2 / slot3 原函数RVA |
| --- | --- | --- | --- |
| ??_7Entity_Effect@IsaacRepentancePlus@@6B@ | 0x763bdc | 0xc5d5ac | 0x001CD9F0 / 0x001CDB20 / 0x002AE7D0 / 0x001D4380 |

## 已登记字段

字段位置可读不等于可安全写回。RAW保留原位串；bool为byte，引用不发送裸指针；专用字段需满足初始化与当前generation读取依据。

| 字段 | 对象相对偏移 | 字节 | 证据范围 |
| --- | --- | --- | --- |
| State | 0x450 | 4 | CURRENT_SOURCE_MIN_READ |
| Timeout | 0x45C | 4 | CURRENT_SOURCE_MIN_READ |
| LifeSpan | 0x460 | 4 | CURRENT_SOURCE_MIN_READ |

主要依据：[实际采集Spec](../../../cs-t0/native/src/world_record.cpp)、[偏移修订](../refactor/cs-t0/t0-06/tx-01-69-offset-reconciliation.md)、[读取与引用规格](../refactor/cs-t0/t0-06/tx-01-71-sampling-design.md)。派生Entity共享字段另见[Entity](entity.md)。

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x001CDA90 | 0x005CDA90 | Effect pool element destructor | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x001CDB20 | 0x005CDB20 | Effect Init | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x004854A0 | 0x008854A0 | sub_8854A0 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x0049D4D0 | 0x0089D4D0 | Lua ToEffect RTTI conversion | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |

## 证据与限制

[最近固定点位与ABI复核](../refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../refactor/project-conventions.md)。全量历史研究保留于 `docs/archive/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。
