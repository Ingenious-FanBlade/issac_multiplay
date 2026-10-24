# Player 玩家与本地合作：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

PlayerManager 位于 Game+0x1BAA8，roster 是 begin/end/capacity 三指针 vector；+0x80 另有四个 Esau 保留槽。Player 对象的 SubPlayer、Twin、backup、replaced 必须独立记录；保留对象不自动算活跃玩家。

普通工厂拒绝 Type=1；原生玩家创建 owner RVA 0x5B9CD0 为唯一现行创建路径。Join 的三处 Baby predicate 只对自有K选择普通完整角色分支，不能全局改 run flag。CreateHook 拒绝续段 RVA 0x5BB561 是控制流依赖，不是独立安装点。原退出路径的 Variant!=0 guard 不适用于普通 Variant=0 B，不能把它记为已实现的普通退出。

PlayerUpdate 槽 RVA 0x76BDDC 指向 0x382AF0；六个 consumer 含主动道具额外 int32 栈参数。标准Room与奇数帧helper共用 Authority scheduler；特殊暂停/FSM更新不能冒充tick。旧caller返回RVA 0x2FD4AC/0x2FD54D 及切房0x4319CC/0x431A14/0x431A34可能更新Player，不代表正常模拟窗口。

输入 witness：assignment +0x1618/4，movement direction +0x1624/4，shooting direction +0x162C/4、aim +0x1630/+0x1634、aim state +0x1638，bomb +0x1378 bit0，drop counter +0x17C0/4。VelocityBeforeUpdate +0x1660 是内嵌向量；武器指针 +0x13DC（备用 +0x13E0 起四槽）不是蓄力计数。

真实心量与通用HP分别记录；BlackHearts是mask，不能与SoulHearts直接相加。首版Replica不能调用TakeDamage/AddHearts重演服务器结果。exact-pair跨一次正常PlayerUpdate的当前字段读取依据为用户采纳值3，不补造出生历史。

## 源码声明的补丁、调用依赖与自有表

| 安装家族 / 序号 | 类型 | site RVA → 原目标 RVA | 用途 | 字节 / 跨度 | 源码 |
| --- | --- | --- | --- | --- | --- |
| active_targets.cpp [0] | REL32_CALL | 0x00385BC2 → 0x00379B40 | 移动 consumer | E8 79 3F FF FF / 5 | [active_targets.cpp:10](../../cs-t0/native/src/active_targets.cpp) |
| active_targets.cpp [1] | REL32_CALL | 0x00385BC9 → 0x00380F40 | 炸弹 consumer | E8 72 B3 FF FF / 5 | [active_targets.cpp:10](../../cs-t0/native/src/active_targets.cpp) |
| active_targets.cpp [2] | REL32_CALL | 0x00385BD2 → 0x00391A40 | 主动道具 consumer | E8 69 BE 00 00 / 5 | [active_targets.cpp:10](../../cs-t0/native/src/active_targets.cpp) |
| active_targets.cpp [3] | REL32_CALL | 0x00385BD9 → 0x003A3D30 | 卡牌/药丸 consumer | E8 52 E1 01 00 / 5 | [active_targets.cpp:10](../../cs-t0/native/src/active_targets.cpp) |
| active_targets.cpp [4] | REL32_CALL | 0x00385BE0 → 0x0037B020 | 射击 consumer | E8 3B 54 FF FF / 5 | [active_targets.cpp:10](../../cs-t0/native/src/active_targets.cpp) |
| active_targets.cpp [5] | REL32_CALL | 0x00385BE7 → 0x003A42B0 | 丢弃 consumer | E8 C4 E6 01 00 / 5 | [active_targets.cpp:10](../../cs-t0/native/src/active_targets.cpp) |
| raw_apply_runtime.cpp [0] | REL32_CALL | 0x00385BC2 → 0x00379B40 | 移动 consumer | E8 79 3F FF FF / 5 | [raw_apply_runtime.cpp:59](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [1] | REL32_CALL | 0x00385BC9 → 0x00380F40 | 炸弹 consumer | E8 72 B3 FF FF / 5 | [raw_apply_runtime.cpp:59](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [2] | REL32_CALL | 0x00385BD2 → 0x00391A40 | 主动道具 consumer | E8 69 BE 00 00 / 5 | [raw_apply_runtime.cpp:59](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [3] | REL32_CALL | 0x00385BD9 → 0x003A3D30 | 卡牌/药丸 consumer | E8 52 E1 01 00 / 5 | [raw_apply_runtime.cpp:59](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [4] | REL32_CALL | 0x00385BE0 → 0x0037B020 | 射击 consumer | E8 3B 54 FF FF / 5 | [raw_apply_runtime.cpp:59](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [5] | REL32_CALL | 0x00385BE7 → 0x003A42B0 | 丢弃 consumer | E8 C4 E6 01 00 / 5 | [raw_apply_runtime.cpp:59](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| coop_passive_runtime.cpp [0] | REL32_CALL | 0x002FADA7 → 0x005BAC40 | Join manager pass | E8 94 FE 2B 00 / 5 | [coop_passive_runtime.cpp:58](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [1] | REL32_CALL | 0x005BB415 → 0x005B9CD0 | 普通 Player 创建 | E8 B6 E8 FF FF / 5 | [coop_passive_runtime.cpp:58](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [2] | REL32_CALL | 0x005BAFFA → 0x005B9CD0 | 另一创建 caller | E8 D1 EC FF FF / 5 | [coop_passive_runtime.cpp:58](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [3] | REL32_CALL | 0x005BB22A → 0x005BA000 | 原生退出 caller（普通 Variant=0 路线不适用） | E8 D1 ED FF FF / 5 | [coop_passive_runtime.cpp:58](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [4] | REL32_CALL | 0x005BB424 → 0x003A6450 | 普通加入 assignment | E8 27 B0 DE FF / 5 | [coop_passive_runtime.cpp:58](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [5] | REL32_CALL | 0x005BB004 → 0x003A6450 | 另一 assignment caller | E8 47 B4 DE FF / 5 | [coop_passive_runtime.cpp:58](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [6] | REL32_CALL | 0x005BB4FD → 0x005A8620 | HUD 加入更新 | E8 1E D1 FE FF / 5 | [coop_passive_runtime.cpp:58](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [7] | REL32_CALL | 0x005BA752 → 0x005A8620 | HUD 退出更新 | E8 C9 DE FE FF / 5 | [coop_passive_runtime.cpp:58](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| active_targets.cpp [6] | VTABLE_SLOT | 0x0076BDDC → 0x00382AF0 | Player::Update vtable+0x0C | F0 2A 78 00 / 4 | [active_targets.cpp:26](../../cs-t0/native/src/active_targets.cpp) |
| coop_active_runtime.cpp [6] | VTABLE_SLOT | 0x0076BDDC → 0x00382AF0 | Player::Update vtable+0x0C | F0 2A 78 00 / 4 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_passive_runtime.cpp [6] | VTABLE_SLOT | 0x0076BDDC → 0x00382AF0 | Player::Update vtable+0x0C | F0 2A 78 00 / 4 | [coop_passive_runtime.cpp:412](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| raw_apply_runtime.cpp [11] | VTABLE_SLOT | 0x0076BDDC → 0x00382AF0 | Player::Update vtable+0x0C | F0 2A 78 00 / 4 | [raw_apply_runtime.cpp:42](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| coop_active_runtime.cpp [0] | REL32_CALL | 0x00385BC2 → 0x00379B40 | 移动 consumer | E8 79 3F FF FF / 5 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [1] | REL32_CALL | 0x00385BC9 → 0x00380F40 | 炸弹 consumer | E8 72 B3 FF FF / 5 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [2] | REL32_CALL | 0x00385BD2 → 0x00391A40 | 主动道具 consumer | E8 69 BE 00 00 / 5 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [3] | REL32_CALL | 0x00385BD9 → 0x003A3D30 | 卡牌/药丸 consumer | E8 52 E1 01 00 / 5 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [4] | REL32_CALL | 0x00385BE0 → 0x0037B020 | 射击 consumer | E8 3B 54 FF FF / 5 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [5] | REL32_CALL | 0x00385BE7 → 0x003A42B0 | 丢弃 consumer | E8 C4 E6 01 00 / 5 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [6] | REL32_CALL | 0x002FADA7 → 0x005BAC40 | Join pass | E8 94 FE 2B 00 / 5 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [7] | REL32_CALL | 0x005BB415 → 0x005B9CD0 | 唯一普通 Player 创建 | E8 B6 E8 FF FF / 5 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [8] | REL32_CALL | 0x00555241 → 0x002FD3F0 | 奇数帧 Player-only 模拟 helper | E8 AA 81 DA FF / 5 | [coop_active_runtime.cpp:1702](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [9] | REL32_CALL | 0x005BAE7E → 0x005BA980 | JoinHealthMode | E8 FD FA FF FF / 5 | [coop_active_runtime.cpp:1709](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [10] | REL32_CALL | 0x005BB081 → 0x005BA980 | JoinDeviceMode | E8 FA F8 FF FF / 5 | [coop_active_runtime.cpp:1709](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [11] | REL32_CALL | 0x005BB288 → 0x005BA980 | JoinSelectorMode | E8 F3 F6 FF FF / 5 | [coop_active_runtime.cpp:1709](../../cs-t0/native/src/coop_active_runtime.cpp) |
| world_record.cpp [6] | ENTRY_PATCH | 0x00353DA0 → 0x00353DA0 | Player constructor | 53 8B DC 83 EC 08 / 6 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [7] | ENTRY_PATCH | 0x005B9CD0 → 0x005B9CD0 | PlayerManager::Create | 55 8B EC 6A FF / 5 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [8] | ENTRY_PATCH | 0x005BCAA0 → 0x005BCAA0 | PlayerManager restore | 55 8B EC 6A FF / 5 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [9] | ENTRY_PATCH | 0x005BEE80 → 0x005BEE80 | ReplacePlayer | 53 8B DC 83 EC 08 / 6 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [10] | ENTRY_PATCH | 0x003AE4B0 → 0x003AE4B0 | Player restore | 53 8B DC 83 EC 08 / 6 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |
| active_runtime.cpp [0] | REL32_CALL | 0x00385BC2 → 0x00379B40 | 移动 consumer | E8 79 3F FF FF / 5 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |
| coop_passive_runtime.cpp [0] | REL32_CALL | 0x00385BC2 → 0x00379B40 | 移动 consumer | E8 79 3F FF FF / 5 | [coop_passive_runtime.cpp:2060](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| active_runtime.cpp [1] | REL32_CALL | 0x00385BC9 → 0x00380F40 | 炸弹 consumer | E8 72 B3 FF FF / 5 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |
| coop_passive_runtime.cpp [1] | REL32_CALL | 0x00385BC9 → 0x00380F40 | 炸弹 consumer | E8 72 B3 FF FF / 5 | [coop_passive_runtime.cpp:2060](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| active_runtime.cpp [2] | REL32_CALL | 0x00385BD2 → 0x00391A40 | 主动道具 consumer | E8 69 BE 00 00 / 5 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |
| coop_passive_runtime.cpp [2] | REL32_CALL | 0x00385BD2 → 0x00391A40 | 主动道具 consumer | E8 69 BE 00 00 / 5 | [coop_passive_runtime.cpp:2060](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| active_runtime.cpp [3] | REL32_CALL | 0x00385BD9 → 0x003A3D30 | 卡牌/药丸 consumer | E8 52 E1 01 00 / 5 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |
| coop_passive_runtime.cpp [3] | REL32_CALL | 0x00385BD9 → 0x003A3D30 | 卡牌/药丸 consumer | E8 52 E1 01 00 / 5 | [coop_passive_runtime.cpp:2060](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| active_runtime.cpp [4] | REL32_CALL | 0x00385BE0 → 0x0037B020 | 射击 consumer | E8 3B 54 FF FF / 5 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |
| coop_passive_runtime.cpp [4] | REL32_CALL | 0x00385BE0 → 0x0037B020 | 射击 consumer | E8 3B 54 FF FF / 5 | [coop_passive_runtime.cpp:2060](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| active_runtime.cpp [5] | REL32_CALL | 0x00385BE7 → 0x003A42B0 | 丢弃 consumer | E8 C4 E6 01 00 / 5 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |
| coop_passive_runtime.cpp [5] | REL32_CALL | 0x00385BE7 → 0x003A42B0 | 丢弃 consumer | E8 C4 E6 01 00 / 5 | [coop_passive_runtime.cpp:2060](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| active_runtime.cpp [6] | VTABLE_SLOT | 0x0076BDDC → 0x00382AF0 | Player::Update vtable+0x0C | F0 2A 78 00 / 4 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |

入口桥最小ABI（转发约定，不是完整高层参数语义）：

| RVA | 约定 |
| --- | --- |
| 0x00353DA0 | ECX=this；0个raw32栈参数；ret 0 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x005B9CD0 | ECX=this；1个raw32栈参数；ret 4 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x005BCAA0 | ECX=this；1个raw32栈参数；ret 4 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x005BEE80 | ECX=this；2个raw32栈参数；ret 8 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x003AE4B0 | ECX=this；1个raw32栈参数；ret 4 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |

## 当前 RTTI 与虚表候选

来源：[tx-01-64 RTTI原始记录](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json)。槽0..3只按原始表登记；Grid槽语义与Entity不同，不统一命名。下列槽并非全部被patch。

| 类名 | vtable RVA | TypeDescriptor IDA VA | slot0 / slot1 / slot2 / slot3 原函数RVA |
| --- | --- | --- | --- |
| ??_7Entity_Player@IsaacRepentancePlus@@6B@ | 0x76bdd0 | 0xc5d6dc | 0x00354D60 / 0x00355470 / 0x002AE7D0 / 0x00382AF0 |

## 已登记字段

字段位置可读不等于可安全写回。RAW保留原位串；bool为byte，引用不发送裸指针；专用字段需满足初始化与当前generation读取依据。

| 字段 | 对象相对偏移 | 字节 | 证据范围 |
| --- | --- | --- | --- |
| PlayerType | 0x13C0 | 4 | CURRENT_SOURCE_MIN_READ |
| PlayerIndex | 0x161C | 4 | CURRENT_SOURCE_MIN_READ |
| DamageCooldown | 0x13BC | 4 | CURRENT_SOURCE_MIN_READ |
| MaxHearts | 0x1340 | 4 | CURRENT_SOURCE_MIN_READ |
| RedHearts | 0x1344 | 4 | CURRENT_SOURCE_MIN_READ |
| EternalHearts | 0x1348 | 4 | CURRENT_SOURCE_MIN_READ |
| SoulHearts | 0x134C | 4 | CURRENT_SOURCE_MIN_READ |
| BlackHearts mask | 0x1350 | 4 | CURRENT_SOURCE_MIN_READ |
| BoneHearts | 0x1D88 | 4 | CURRENT_SOURCE_MIN_READ |
| GoldenHearts | 0x194C | 4 | CURRENT_SOURCE_MIN_READ |
| BrokenHearts | 0x1DA0 | 4 | CURRENT_SOURCE_MIN_READ |
| RottenHearts | 0x1DA4 | 4 | CURRENT_SOURCE_MIN_READ |
| SubPlayer | 0x1D98 | 4 | CURRENT_SOURCE_MIN_READ |
| Twin | 0x1E68 | 4 | CURRENT_SOURCE_MIN_READ |
| backup | 0x1E6C | 4 | CURRENT_SOURCE_MIN_READ |
| replaced | 0x26C8 | 4 | CURRENT_SOURCE_MIN_READ |

主要依据：[实际采集Spec](../../cs-t0/native/src/world_record.cpp)、[偏移修订](../../docs/refactor/cs-t0/t0-06/tx-01-69-offset-reconciliation.md)、[读取与引用规格](../../docs/refactor/cs-t0/t0-06/tx-01-71-sampling-design.md)。派生Entity共享字段另见[Entity](entity.md)。

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x00017820 | 0x00417820 | GetOtherTwin: current offset | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x000253B0 | 0x004253B0 | GetPlayerType: current offset | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x00028B10 | 0x00428B10 | Player GetHearts direct getter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x0002A310 | 0x0042A310 | Player GetMaxHearts direct getter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x001B1700 | 0x005B1700 | Player GetBoneHearts direct getter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x001B1710 | 0x005B1710 | Player GetRottenHearts direct getter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x001B1740 | 0x005B1740 | Player GetSoulHearts direct getter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x001B1750 | 0x005B1750 | Player GetBlackHearts direct getter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x001B1770 | 0x005B1770 | Player GetGoldenHearts direct getter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x0020D080 | 0x0060D080 | sub_60D080 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x0025CF20 | 0x0065CF20 | Player GetBrokenHearts direct getter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x002DAD60 | 0x006DAD60 | VelocityBeforeUpdate address getter, inline Player+1660 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x002DAD70 | 0x006DAD70 | Player GetEternalHearts direct getter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x002DAE10 | 0x006DAE10 | GetSubPlayer: separate relation | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x002FD3F0 | 0x006FD3F0 | sub_6FD3F0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00353DA0 | 0x00753DA0 | Player constructor | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x00354D60 | 0x00754D60 | Player scalar deleting destructor | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x00354ED0 | 0x00754ED0 | Player destructor | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x00355470 | 0x00755470 | Player Init | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x00357A40 | 0x00757A40 | Player ClearReferences override | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x003588A0 | 0x007588A0 | Player AddMaxHearts | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x00358A70 | 0x00758A70 | Player AddHearts | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x00358C20 | 0x00758C20 | Player AddEternalHearts | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x00358D00 | 0x00758D00 | Player AddSoulHearts | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x00358F90 | 0x00758F90 | Player AddBlackHearts | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x003729D0 | 0x007729D0 | Player TakeDamage override | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x00379B40 | 0x00779B40 | sub_779B40 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x0037B020 | 0x0077B020 | sub_77B020 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x00380F40 | 0x00780F40 | sub_780F40 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x00391A40 | 0x00791A40 | sub_791A40 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x003A3D30 | 0x007A3D30 | sub_7A3D30 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x003A42B0 | 0x007A42B0 | sub_7A42B0 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x003AC7E0 | 0x007AC7E0 | Esau stored-player creation and replacement caller | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x003AE4B0 | 0x007AE4B0 | sub_7AE4B0 | [primary-evidence.json](../../artifacts/cs-t0/tx-01-71-static/primary-evidence.json) |
| 0x003CA840 | 0x007CA840 | Player AddBoneHearts | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x003CAFE0 | 0x007CAFE0 | Get health type from PlayerType; no player pointer redirection | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x003CB060 | 0x007CB060 | EffectiveMaxHearts: same object, MaxHearts plus optional 2*BoneHearts | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x003D2A40 | 0x007D2A40 | Player AddBrokenHearts | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x003D2FC0 | 0x007D2FC0 | Player AddRottenHearts | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x00466960 | 0x00866960 | Entity and EntityPlayer Lua property/method registration | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x0049D610 | 0x0089D610 | Lua ToPlayer RTTI conversion | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x005B9CD0 | 0x009B9CD0 | ordinary local-coop Player allocation and publication | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x005BA000 | 0x009BA000 | sub_9BA000 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x005BA980 | 0x009BA980 | sub_9BA980 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x005BAC40 | 0x009BAC40 | sub_9BAC40 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x005BCAA0 | 0x009BCAA0 | PlayerManager restore: backup and four Esau slots | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x005BEE80 | 0x009BEE80 | ReplacePlayer: temporary peer, backup and roster relations | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。

## 源码原生定位常量补录

以下逐项保留当前源码的RVA/对象偏移命名和值，包括全局槽、vtable/COL、返回位置和只读诊断定位。常量不全是实际patch，旧命名不覆盖模块正文消歧；同一值在不同family中重复保留。`OBJECT_OFFSET`不加模块基址。

| 原始符号 | 类型 | 值 | 源码位置 |
| --- | --- | --- | --- |
| kPlayerManagerOffset | OBJECT_OFFSET | `0x0001BAA8` | [manager_identity_reader.hpp:9](../../cs-t0/native/include/isaac_cs_t0/manager_identity_reader.hpp) |
| kPlayerVectorOffset | OBJECT_OFFSET | `0x0001BAA8` | [active_runtime.cpp:38](../../cs-t0/native/src/active_runtime.cpp) |
| kPlayerOwnerKeyOffset | OBJECT_OFFSET | `0x00001618` | [active_runtime.cpp:39](../../cs-t0/native/src/active_runtime.cpp) |
| kPlayerManagerOffset | OBJECT_OFFSET | `0x0001BAA8` | [coop_passive_runtime.cpp:37](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kHudOffset | OBJECT_OFFSET | `0x0001DA04` | [coop_passive_runtime.cpp:39](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kPlayerVtableRva | RVA | `0x0076BDD0` | [coop_passive_runtime.cpp:44](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kPlayerColRva | RVA | `0x007AE31C` | [coop_passive_runtime.cpp:45](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kPlayerAssignmentOffset | OBJECT_OFFSET | `0x00001618` | [coop_passive_runtime.cpp:46](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kPlayerOrdinalOffset | OBJECT_OFFSET | `0x0000161C` | [coop_passive_runtime.cpp:47](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kPlayerTwinOffset | OBJECT_OFFSET | `0x00001E68` | [coop_passive_runtime.cpp:48](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kPlayerVariantOffset | OBJECT_OFFSET | `0x0000002C` | [coop_passive_runtime.cpp:49](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kPlayerParentOffset | OBJECT_OFFSET | `0x000003BC` | [coop_passive_runtime.cpp:50](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kPlayerVectorOffset | OBJECT_OFFSET | `0x0001BAA8` | [raw_apply_runtime.cpp:25](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kPlayerOwnerKeyOffset | OBJECT_OFFSET | `0x00001618` | [raw_apply_runtime.cpp:26](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kPlayerVtableSlotRva | RVA | `0x0076BDDC` | [raw_apply_runtime.cpp:42](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kPlayerUpdateRva | RVA | `0x00382AF0` | [raw_apply_runtime.cpp:43](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kPlayerManagerOffset | OBJECT_OFFSET | `0x0001BAA8` | [coop_passive_main.cpp:172](../../cs-t0/native/tests/coop_passive_main.cpp) |
| kHudOffset | OBJECT_OFFSET | `0x0001DA04` | [coop_passive_main.cpp:174](../../cs-t0/native/tests/coop_passive_main.cpp) |
| kPlayerVtableRva | RVA | `0x0076BDD0` | [coop_passive_main.cpp:177](../../cs-t0/native/tests/coop_passive_main.cpp) |
| kPlayerColRva | RVA | `0x007AE31C` | [coop_passive_main.cpp:178](../../cs-t0/native/tests/coop_passive_main.cpp) |
| kPlayerAssignmentOffset | OBJECT_OFFSET | `0x00001618` | [coop_passive_main.cpp:179](../../cs-t0/native/tests/coop_passive_main.cpp) |
| kPlayerOrdinalOffset | OBJECT_OFFSET | `0x0000161C` | [coop_passive_main.cpp:180](../../cs-t0/native/tests/coop_passive_main.cpp) |
| kPlayerTwinOffset | OBJECT_OFFSET | `0x00001E68` | [coop_passive_main.cpp:181](../../cs-t0/native/tests/coop_passive_main.cpp) |
