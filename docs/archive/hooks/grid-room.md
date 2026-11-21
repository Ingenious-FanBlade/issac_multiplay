# Room、Grid 与门：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

Game→Room* 为+0x18300，Room内嵌EntityList为+0x1218。Room Grid数组位于+0x24，共448个4-byte槽；门视图+0x724，共8槽；width/height为+0xC/+0x10。width×height不代替448槽边界。

Grid不是Entity派生类，不套读Entity HP/flags/引用。SpawnGrid顺序为分配新对象→释放旧槽→写新槽/参数→Init，与动态实体Init后发布不同。MakeDoor可把同一Grid对象同时写入门视图，不能双计。GridFree immediate与Room析构会物理释放，不使用Entity延后摘表模型。

Grid坐标由明确来源slot及Room width推导，不能把来源slot与对象GridIndex恒等视为事实。初始化+0x1C是byte，SpawnFrame +0x28为raw32。Grid耐久与所有派生变体仍UNKNOWN。

首版只支持固定单房间；切房/换层时退休该Replica并报告超出首版范围。后续采用room_epoch更新、资源准备和新baseline事务，不把已有RoomLifecycle观察当成已实现状态重建。

## 源码声明的补丁、调用依赖与自有表

| 安装家族 / 序号 | 类型 | site RVA → 原目标 RVA | 用途 | 字节 / 跨度 | 源码 |
| --- | --- | --- | --- | --- | --- |
| loop_runtime.cpp [6] | REL32_CALL | 0x0033F5B9 → 0x00465180 | NewRoom 回调 | E8 C2 5B 12 00 / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [11] | REL32_CALL | 0x00559B4D → 0x00402980 | alternate Room::Update | E8 2E 8E EA FF / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [15] | REL32_CALL | 0x002FBA4D → 0x00402980 | 普通 Room::Update | E8 2E 6F 10 00 / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [23] | REL32_CALL | 0x002FB4F0 → 0x0052F1C0 | 房间生命周期 FSM owner | E8 CB 3C 23 00 / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [24] | REL32_CALL | 0x0052FC77 → 0x00402980 | 生命周期嵌套 Room::Update | E8 04 2D ED FF / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [25] | REL32_CALL | 0x002FB51C → 0x004318A0 | 普通门切房状态机 owner | E8 7F 63 13 00 / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [26] | REL32_CALL | 0x002FE37C → 0x00402980 | 普通门嵌套 Room::Update | E8 FF 45 10 00 / 5 | [loop_runtime.cpp:45](../../../cs-t0/native/src/loop_runtime.cpp) |
| boundary_runtime.cpp [2] | REL32_CALL | 0x002FB51C → 0x004318A0 | 房间变化 owner | E8 7F 63 13 00 / 5 | [boundary_runtime.cpp:42](../../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [11] | REL32_CALL | 0x0033F5B9 → 0x00465180 | NewRoom callback | E8 C2 5B 12 00 / 5 | [boundary_runtime.cpp:42](../../../cs-t0/native/src/boundary_runtime.cpp) |
| world_record.cpp [13] | ENTRY_PATCH | 0x0030B2E0 → 0x0030B2E0 | Grid factory | 55 8B EC 6A FF / 5 | [world_record.cpp:1839](../../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [14] | ENTRY_PATCH | 0x00309DB0 → 0x00309DB0 | Grid Init | 55 8B EC 8B 45 08 / 6 | [world_record.cpp:1839](../../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [15] | ENTRY_PATCH | 0x003EBCA0 → 0x003EBCA0 | Room SpawnGrid | 55 8B EC 53 56 / 5 | [world_record.cpp:1839](../../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [16] | ENTRY_PATCH | 0x003EEA90 → 0x003EEA90 | Room MakeDoor | 55 8B EC 6A FF / 5 | [world_record.cpp:1839](../../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [17] | ENTRY_PATCH | 0x0041E980 → 0x0041E980 | Room GridFree immediate | 55 8B EC 8B 55 08 / 6 | [world_record.cpp:1839](../../../cs-t0/native/src/world_record.cpp) |

入口桥最小ABI（转发约定，不是完整高层参数语义）：

| RVA | 约定 |
| --- | --- |
| 0x0030B2E0 | ECX=Grid type，EDX=index；无显式栈参数；EAX返回对象；不能当ECX=this普通成员函数 |
| 0x00309DB0 | ECX=this；1个raw32栈参数；ret 4 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x003EBCA0 | ECX=this；5个raw32栈参数；ret 20 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x003EEA90 | ECX=this；1个raw32栈参数；ret 4 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x0041E980 | ECX=this；3个raw32栈参数；ret 12 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |

## 当前 RTTI 与虚表候选

来源：[tx-01-64 RTTI原始记录](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json)。槽0..3只按原始表登记；Grid槽语义与Entity不同，不统一命名。下列槽并非全部被patch。

| 类名 | vtable RVA | TypeDescriptor IDA VA | slot0 / slot1 / slot2 / slot3 原函数RVA |
| --- | --- | --- | --- |
| ??_7GridEntity_Teleporter@IsaacRepentancePlus@@6B@ | 0x7685d0 | 0xc5e7a8 | 0x00309CB0 / 0x0000C200 / 0x00438C30 / 0x00311F30 |
| ??_7GridEntity_Statue@IsaacRepentancePlus@@6B@ | 0x7685f8 | 0xc5e73c | 0x0030BDD0 / 0x0000C200 / 0x00438520 / 0x0002CDF0 |
| ??_7GridEntity_PressurePlate@IsaacRepentancePlus@@6B@ | 0x768620 | 0xc5e814 | 0x0030BD00 / 0x0000C200 / 0x00583330 / 0x00585C10 |
| ??_7GridEntity_Poop@IsaacRepentancePlus@@6B@ | 0x768648 | 0xc5d770 | 0x0030BC60 / 0x00318DD0 / 0x00314770 / 0x003151E0 |
| ??_7GridEntity_Wall@IsaacRepentancePlus@@6B@ | 0x768670 | 0xc5e7e0 | 0x00309CB0 / 0x00321310 / 0x0000C200 / 0x00320FE0 |
| ??_7GridEntity_Door@IsaacRepentancePlus@@6B@ | 0x768698 | 0xc5d964 | 0x0030BB20 / 0x0000C200 / 0x0030C5C0 / 0x0030D1E0 |
| ??_7GridEntity_Decoration@IsaacRepentancePlus@@6B@ | 0x7686c0 | 0xc5e770 | 0x00309CB0 / 0x0000C200 / 0x0059A8E0 / 0x0059AC00 |
| ??_7GridEntity@IsaacRepentancePlus@@6B@ | 0x7686e8 | 0xc5d7a4 | 0x00309CB0 / 0x0000C200 / 0x006F05D3 / 0x006F05D3 |
| ??_7GridEntity_Fire@IsaacRepentancePlus@@6B@ | 0x768710 | 0xc5e850 | 0x0030BA00 / 0x0000C200 / 0x0027E0F0 / 0x0027E880 |
| ??_7GridEntity_Rock@IsaacRepentancePlus@@6B@ | 0x768738 | 0xc5d8cc | 0x0030B950 / 0x0031A760 / 0x00318FE0 / 0x00319AD0 |
| ??_7GridEntity_Lock@IsaacRepentancePlus@@6B@ | 0x768e84 | 0xc5e884 | 0x0030BDD0 / 0x0000C200 / 0x00311950 / 0x00311F30 |
| ??_7GridEntity_Pit@IsaacRepentancePlus@@6B@ | 0x768fc0 | 0xc5e8b8 | 0x003120D0 / 0x00314320 / 0x00312180 / 0x00313440 |
| ??_7GridEntity_Spikes@IsaacRepentancePlus@@6B@ | 0x7692c0 | 0xc5d804 | 0x0030BDD0 / 0x0000C200 / 0x0031E9B0 / 0x0031EDB0 |
| ??_7GridEntity_TNT@IsaacRepentancePlus@@6B@ | 0x769300 | 0xc5e8ec | 0x0030BDD0 / 0x0000C200 / 0x0031F090 / 0x0031EDB0 |
| ??_7GridEntity_TrapDoor@IsaacRepentancePlus@@6B@ | 0x76946c | 0xc5e920 | 0x0030BDD0 / 0x0000C200 / 0x0031FEF0 / 0x0031EDB0 |
| ??_7GridEntity_Web@IsaacRepentancePlus@@6B@ | 0x769558 | 0xc5e958 | 0x00309CB0 / 0x0000C200 / 0x00321780 / 0x0031EDB0 |
| ??_7GridEntity_Gravity@IsaacRepentancePlus@@6B@ | 0x77d99c | 0xc5f674 | 0x00309CB0 / 0x0059B520 / 0x0059B980 / 0x0031EDB0 |
| ??_7GridEntity_Stairs@IsaacRepentancePlus@@6B@ | 0x77da10 | 0xc5f6ac | 0x00309CB0 / 0x0000C200 / 0x0059BD50 / 0x0031EDB0 |

## 已登记字段

字段位置可读不等于可安全写回。RAW保留原位串；bool为byte，引用不发送裸指针；专用字段需满足初始化与当前generation读取依据。

| 字段 | 对象相对偏移 | 字节 | 证据范围 |
| --- | --- | --- | --- |
| Grid.Type | 0x4 | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.Variant | 0x8 | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.State | 0xC | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.SpawnCount | 0x10 | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.SpawnSeed | 0x14 | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.VarData | 0x18 | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.Initialized | 0x1C | 1 | CURRENT_SOURCE_MIN_READ |
| Grid.VariableSeed | 0x20 | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.GridIndex | 0x24 | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.SpawnFrame | 0x28 | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.RNG word0 | 0x2C | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.RNG word1 | 0x30 | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.RNG word2 | 0x34 | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.RNG word3 | 0x38 | 4 | CURRENT_SOURCE_MIN_READ |
| Grid.CollisionClass | 0x3C | 4 | CURRENT_SOURCE_MIN_READ |

主要依据：[实际采集Spec](../../../cs-t0/native/src/world_record.cpp)、[偏移修订](../refactor/cs-t0/t0-06/tx-01-69-offset-reconciliation.md)、[读取与引用规格](../refactor/cs-t0/t0-06/tx-01-71-sampling-design.md)。派生Entity共享字段另见[Entity](entity.md)。

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x000073C0 | 0x004073C0 | sub_4073C0 | [field-evidence.json](../../../artifacts/cs-t0/tx-01-69-static/field-evidence.json) |
| 0x0000C2E0 | 0x0040C2E0 | GetGridHeight | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x000176F0 | 0x004176F0 | GetGridWidth | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x00036020 | 0x00436020 | GetDoor: separate pointer view | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x00036040 | 0x00436040 | Get door-array base | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x00036060 | 0x00436060 | GetGridEntity: separate array and bounds | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x0003EEC0 | 0x0043EEC0 | sub_43EEC0 | [field-evidence.json](../../../artifacts/cs-t0/tx-01-69-static/field-evidence.json) |
| 0x000566A0 | 0x004566A0 | GetGridSize: width times height | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x000E4830 | 0x004E4830 | sub_4E4830 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00265B70 | 0x00665B70 | sub_665B70 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x0027EFC0 | 0x0067EFC0 | sub_67EFC0 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00309C40 | 0x00709C40 | sub_709C40 | [binding-evidence.json](../../../artifacts/cs-t0/tx-01-67-static/binding-evidence.json) |
| 0x00309CB0 | 0x00709CB0 | GridEntity scalar deleting destructor | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x00309DB0 | 0x00709DB0 | Grid initialization fields | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x00309DF0 | 0x00709DF0 | Grid position computed from stored Grid index | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x0030B2E0 | 0x0070B2E0 | Grid factory: separate classes/type namespace | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x0030B950 | 0x0070B950 | Rock scalar deleting destructor example | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x003E9BA0 | 0x007E9BA0 | Room destructor: 448 grid slots | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x003EBCA0 | 0x007EBCA0 | Room::SpawnGridEntity | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x003EEA90 | 0x007EEA90 | MakeDoor: grid and door-slot aliases | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x00402980 | 0x00802980 | sub_802980 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00412F00 | 0x00812F00 | Grid position computed from Room width and supplied index | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x0041E980 | 0x0081E980 | Room::RemoveGridEntityImmediate | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x004318A0 | 0x008318A0 | sub_8318A0 | [root-ida-abi.json](../../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x00465180 | 0x00865180 | sub_865180 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x0048AF40 | 0x0088AF40 | sub_88AF40 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x004C08F0 | 0x008C08F0 | sub_8C08F0 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x0052F1C0 | 0x0092F1C0 | sub_92F1C0 | [ida-evidence.json](../../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |

## 证据与限制

[最近固定点位与ABI复核](../refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../refactor/project-conventions.md)。全量历史研究保留于 `docs/archive/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。
