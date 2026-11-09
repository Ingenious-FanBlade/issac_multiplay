# NPC 敌人与 AI 状态：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

识别结合当前RTTI和原始分类，不用Type>9、10..999、敌我或可伤害条件代替NPC对象域。Morph RVA 0x2C0F30 在同一对象上修改分类和配置；不因Type/Variant变化自动分配新网络ID。其方法本体未直接改InitSeed，不据此宣称所有helper和Mod绝不改seed。

State/StateFrame/ProjectileCooldown/ProjectileDelay 的raw32位置已有具名绑定与Init写入锚点，但AI枚举、全部重置、计时单位及完整AI状态未恢复。NPC的ProjectileCooldown不是Projectile对象字段。状态复制优先同步服务器结果；完整NPC预测不在首版。

tx-03-12[派生绘制研究](../research/tx-03-12-derived-render-sprite-lifecycle.md)：当前 NPC 表 `+0x14 → RVA 0x2D2E50`、`+0x18 → RVA 0x2D4840`、`+0x34 → RVA 0x2B1000`。主绘制可条件进入 ID=28 的 Lua 分派；另有点列／曲线、附加 Sprite 和关联对象。不能用基类 Sprite 绘制覆盖所有 NPC 外观，也不能把全部 render 视作只读或无 Lua。

## 源码声明的补丁、调用依赖与自有表

| 安装家族 / 序号 | 类型 | site RVA → 原目标 RVA | 用途 | 字节 / 跨度 | 源码 |
| --- | --- | --- | --- | --- | --- |
| world_record.cpp [11] | ENTRY_PATCH | 0x002C0F30 → 0x002C0F30 | NPC::Morph | 55 8B EC 83 E4 F8 / 6 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |

入口桥最小ABI（转发约定，不是完整高层参数语义）：

| RVA | 约定 |
| --- | --- |
| 0x002C0F30 | ECX=this；4个raw32栈参数；ret 16 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |

## 当前 RTTI 与虚表候选

来源：[tx-01-64 RTTI原始记录](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json)。槽0..3只按原始表登记；Grid槽语义与Entity不同，不统一命名。下列槽并非全部被patch。

| 类名 | vtable RVA | TypeDescriptor IDA VA | slot0 / slot1 / slot2 / slot3 原函数RVA |
| --- | --- | --- | --- |
| ??_7Entity_NPC@IsaacRepentancePlus@@6B@ | 0x767468 | 0xc5d6ac | 0x002B88E0 / 0x002B8B20 / 0x002C4AC0 / 0x002C4B30 |

## 已登记字段

字段位置可读不等于可安全写回。RAW保留原位串；bool为byte，引用不发送裸指针；专用字段需满足初始化与当前generation读取依据。

| 字段 | 对象相对偏移 | 字节 | 证据范围 |
| --- | --- | --- | --- |
| StateFrame | 0x410 | 4 | CURRENT_SOURCE_MIN_READ |
| State | 0xB64 | 4 | CURRENT_SOURCE_MIN_READ |
| ProjectileCooldown | 0xBA0 | 4 | CURRENT_SOURCE_MIN_READ |
| ProjectileDelay | 0xBA4 | 4 | CURRENT_SOURCE_MIN_READ |

主要依据：[实际采集Spec](../../cs-t0/native/src/world_record.cpp)、[偏移修订](../../docs/refactor/cs-t0/t0-06/tx-01-69-offset-reconciliation.md)、[读取与引用规格](../../docs/refactor/cs-t0/t0-06/tx-01-71-sampling-design.md)。派生Entity共享字段另见[Entity](entity.md)。

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x002B8910 | 0x006B8910 | NPC pool element destructor | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002B8B20 | 0x006B8B20 | NPC Init | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x002BB790 | 0x006BB790 | NPC config loading override used by virtual slot 0x64 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x002C0F30 | 0x006C0F30 | regions | [agent-native-evidence.json](../../artifacts/cs-t0/tx-01-71-static/agent-native-evidence.json) |
| 0x0047F8B0 | 0x0087F8B0 | ToNPC registration | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x004877E0 | 0x008877E0 | sub_8877E0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x00487AD0 | 0x00887AD0 | sub_887AD0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x0049D390 | 0x0089D390 | Lua ToNPC RTTI conversion | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。

## J273二进制重构第4步（2026-11-09）

布局版本41，NPC142／Delirium143。当前J273 vtable VA B187D0、TypeDescriptor VA BD1344；ctor6B4520、Init6B4A50、Update6C03B0、Render6CEC90、Shadow6D0650、ClearReferences6B4E60。上述旧表为历史目标，不替代本轮核定。对象sizeof0xF60由池步长与删除析构参数互证，三deque位于BF0／C04／C18、双ANM2位于CD8／DEC，C48另有用途未核的第四头。详见[静态证据／传输职责](../design/binary-state/npc-evidence.md)与[实现说明](../design/binary-state/implementation-4-npc.md)。未新增Hook或运行测试。

版本42的[同步职责修正](../design/binary-state/npc-synchronization-review.md)取代版本41旧传输决定：Delirium只更新最终状态／资源，不因Host Boss选择器强制结构换代；+F1C只换算受控UI三态，不复制进程内设备键；+CD8 Scale在6CFFD1／6CFFDA由主Sprite null-frame覆写，新增144视图派生。实际reader／adapter仍待第10／13步，没有安装新Hook或运行测试。

版本43：[受控UI来源闭合与纠错](../design/binary-state/npc-control-ui-evidence.md)。Manager+4AFB8是原版NetplayManager，8E7550为IsIdxLocalPlayer，不是本地设备可用性。正式C/S reader只读NPC F1C，输出0/2，拒绝旧1；第13步NPC draw在6D025D／6D0314及Spawner分支6CF09D按整段绕开原版联机读取，不能只拦函数call。未安装新Hook或执行实机测试。
