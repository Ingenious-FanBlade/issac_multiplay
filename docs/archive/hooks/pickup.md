# Pickup 拾取物：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

Type=5不只包括道具底座。Morph RVA 0x2E30A0 在同一this重新Init，KeepSeed=false可能变更seed，商店分支还有特例。Morph不能当作新物理对象，当前分类和出生描述分开。

Timeout/Wait/State为最小raw字段，不保证已恢复完整商店、库存和拾取规则。Flip/Moving Box等保存记录与空闲池槽不是房间活体。首版客户端不调用原生拾取以重放事件；库存、资源和拾取物存在性由服务器状态确定。

## 源码声明的补丁、调用依赖与自有表

| 安装家族 / 序号 | 类型 | site RVA → 原目标 RVA | 用途 | 字节 / 跨度 | 源码 |
| --- | --- | --- | --- | --- | --- |
| world_record.cpp [12] | ENTRY_PATCH | 0x002E30A0 → 0x002E30A0 | Pickup::Morph | 55 8B EC 6A FF / 5 | [world_record.cpp:1839](../../../cs-t0/native/src/world_record.cpp) |

入口桥最小ABI（转发约定，不是完整高层参数语义）：

| RVA | 约定 |
| --- | --- |
| 0x002E30A0 | ECX=this；6个raw32栈参数；ret 24 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |

## 当前 RTTI 与虚表候选

来源：[tx-01-64 RTTI原始记录](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json)。槽0..3只按原始表登记；Grid槽语义与Entity不同，不统一命名。下列槽并非全部被patch。

| 类名 | vtable RVA | TypeDescriptor IDA VA | slot0 / slot1 / slot2 / slot3 原函数RVA |
| --- | --- | --- | --- |
| ??_7Entity_Pickup@IsaacRepentancePlus@@6B@ | 0x767f24 | 0xc5d7d4 | 0x002E0130 / 0x002E02F0 / 0x002AE7D0 / 0x002E3940 |

## 已登记字段

字段位置可读不等于可安全写回。RAW保留原位串；bool为byte，引用不发送裸指针；专用字段需满足初始化与当前generation读取依据。

| 字段 | 对象相对偏移 | 字节 | 证据范围 |
| --- | --- | --- | --- |
| Timeout | 0x540 | 4 | CURRENT_SOURCE_MIN_READ |
| Wait | 0x544 | 4 | CURRENT_SOURCE_MIN_READ |
| State | 0x584 | 4 | CURRENT_SOURCE_MIN_READ |

主要依据：[实际采集Spec](../../../cs-t0/native/src/world_record.cpp)、[偏移修订](../refactor/cs-t0/t0-06/tx-01-69-offset-reconciliation.md)、[读取与引用规格](../refactor/cs-t0/t0-06/tx-01-71-sampling-design.md)。派生Entity共享字段另见[Entity](entity.md)。

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x00036210 | 0x00436210 | sub_436210 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x001B1940 | 0x005B1940 | sub_5B1940 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x0020D1F0 | 0x0060D1F0 | sub_60D1F0 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x002E0160 | 0x006E0160 | Pickup pool element destructor | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002E02F0 | 0x006E02F0 | Pickup Init | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x002E30A0 | 0x006E30A0 | sub_6E30A0 | [agent-native-evidence.json](../../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x0045E440 | 0x0085E440 | sub_85E440 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x00484EA0 | 0x00884EA0 | sub_884EA0 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x0049D250 | 0x0089D250 | Lua ToPickup RTTI conversion | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |

## 证据与限制

[最近固定点位与ABI复核](../refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../refactor/project-conventions.md)。全量历史研究保留于 `docs/archive/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。
