# Tear 泪弹：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

Tear实体Type=2，与Projectile分开。使用Entity公共字段与TearFlags 128位（+0x428，共16 bytes）；不可截为64位或共用EntityFlags。Tear Init RVA 0x2660A0、Update等虚槽见下表，当前没有独立的Tear专用宿主Hook；其出生/移除由共享Entity链观察。

专用State/Timeout尚未登记；Laser+0x458不能移植到Tear。短命泪弹可能在两份快照之间生成并消失，Registry保留其生命周期历史；必要表现另由带tick的事件表达，伤害只在Authority裁决。

tx-03-12[派生绘制研究](../research/tx-03-12-derived-render-sprite-lifecycle.md)：Tear `+0x14 → RVA 0x2790C0`，`+0x18/+0x34` 共用基类。主绘制有附加 Sprite、颜色／关系条件，且可进入 ID=41 的 Lua 分派；全部资源／变体未闭合，不能仅同步主 Sprite 就宣称 Tear 展示完整。

## 源码声明的补丁、调用依赖与自有表

当前没有本模块专用的源码安装点；共享路径见[Entity](entity.md)、[游戏循环](game-loop.md)。下列逆向入口不因此升级为已安装Hook。

## 当前 RTTI 与虚表候选

来源：[tx-01-64 RTTI原始记录](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json)。槽0..3只按原始表登记；Grid槽语义与Entity不同，不统一命名。下列槽并非全部被patch。

| 类名 | vtable RVA | TypeDescriptor IDA VA | slot0 / slot1 / slot2 / slot3 原函数RVA |
| --- | --- | --- | --- |
| ??_7Entity_Tear@IsaacRepentancePlus@@6B@ | 0x764eac | 0xc5d900 | 0x00265EA0 / 0x002660A0 / 0x002AE7D0 / 0x002670F0 |

## 已登记字段

字段位置可读不等于可安全写回。RAW保留原位串；bool为byte，引用不发送裸指针；专用字段需满足初始化与当前generation读取依据。

| 字段 | 对象相对偏移 | 字节 | 证据范围 |
| --- | --- | --- | --- |
| TearFlags | 0x428 | 16 | CURRENT_SOURCE_MIN_READ |

主要依据：[实际采集Spec](../../cs-t0/native/src/world_record.cpp)、[偏移修订](../../docs/refactor/cs-t0/t0-06/tx-01-69-offset-reconciliation.md)、[读取与引用规格](../../docs/refactor/cs-t0/t0-06/tx-01-71-sampling-design.md)。派生Entity共享字段另见[Entity](entity.md)。

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x000177C0 | 0x004177C0 | sub_4177C0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00265ED0 | 0x00665ED0 | Tear pool element destructor | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002660A0 | 0x006660A0 | Tear Init | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x0047FC70 | 0x0087FC70 | ToTear registration | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x0049CC10 | 0x0089CC10 | Lua ToTear RTTI conversion | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。
