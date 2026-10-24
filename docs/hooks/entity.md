# Entity 公共对象与实体列表：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

普通创建链为 Game::Spawn → Factory acquire → 派生 Init → Room 序号 → EntityList::Publish → Spawn 后处理完成。工厂池满可以 Remove/ClearReferences 后返回同一地址；地址、Index、seed 都不能作为永久网络身份。Player 创建绕过通用工厂，Grid 使用独立对象域。

Remove 只设置 exists/dead 并执行回调/资源清理；清引用、valid=false、摘表与析构是不同阶段。EntityList::Reset 保留部分对象，不等同全体删除。getter、QueryType、GetRoomEntities 可能操作 Lua/scratch/cache，不作为只读采集接口。

公共 HP 两个 f32 字段位置已确认，但“各派生类别都以其表示有效生命”仍是 USER_ACCEPTED_ASSUMPTION；Player hearts 另行处理。当前位置 +0x33C/+0x340、staged +0x344/+0x348、PositionOffset +0x34C/+0x350 不可互换。slot4（+0x10，基类RVA 0x2B0EB0）会写位置，不标记为纯渲染。FrameCount 为同批 Game+0x264F8 减 Entity+0x328 的 raw32 计算值。

tx-03-10[当前分派复核](../research/tx-03-10-replica-update-presentation-boundary.md)澄清：odd helper的VA `0x6FD609`（Type==1，slot3）与`0x6FD62D`（Type!=1，slot4）互斥，不能表述为每个PlayerUpdate后再调用slot4。基类slot4还会调用`0x6AB440`或`0x6A9C90`，其中前者存在条件性Grid构造/槽写入代码；只读采集偏移不赋予这些调用Replica写回资格。旧研究原文和原始证据保留。

tx-03-11[实体绘制与动画事件](../research/tx-03-11-logic-presentation-separation.md)新证据：基类虚表 `+0x14 → RVA 0x2B10F0` 写绘制坐标缓存并调 Sprite 绘制；`+0x18 → RVA 0x2B2000` 属另一绘制 pass，临时写 Sprite 字段。EntityList `RVA 0x199D0` 的特定 pass 会临时翻转 PositionOffset 并在正常返回后恢复。动画 state 位于本次可见 Entity+`0x78/0x98`，目标帧和推进均会影响可被 gameplay 消费的事件位；全部派生渲染和资源副作用尚未闭合，不作为已可调用的 Replica adapter。

tx-03-12[资源生命周期研究](../research/tx-03-12-derived-render-sprite-lifecycle.md)确认独立 Sprite 构造不经 Entity 工厂；其内部 owner、自指针、共享 ANM 数组和图像引用不能按裸对象复制。Load／LoadGraphics／Reset／析构是不同操作，IsLoaded 只读标志。独立 Sprite 仍没有可直接启用的完整资源／线程／失败恢复契约。

tx-03-15[Sprite 换图与重载研究](../research/tx-03-15-sprite-replacement-reload.md)进一步确认 Replace 只改实例 layer 路径并清 `+0x30`，LoadGraphics 按层处理且正常失败不恢复旧 pair；Reload 重建默认 layer，覆盖实例换图。稳定缓存下有额外 ANM 计数推导，Load 换到未命中新路径失败可留旧借用关系；不能据 IsLoaded 决定继续发布。draw 几何 helper 还有条件性共享 RNG 写入，其 gameplay 用途未知；特殊视觉标志需单独限定。

tx-03-16[独立 Sprite 展示字段](../research/tx-03-16-independent-sprite-display-fields.md)区分 main／overlay state、资源 visual track 与实例 layer；相同总帧可因逐层 key 索引覆盖而显示不同姿态。仅按动画名和整数 GetFrame 不够；固定姿态候选不调用 Update、不消费 native 事件，并限定特殊 flags 与 renderer 外部环境。采集／写回与资源应用 owner 尚未实现。

tx-03-17[独立 Sprite owner 研究](../research/tx-03-17-independent-sprite-owner-lifecycle.md)明确未发布新候选的资源与姿态准备、正常退休的限定顺序。copy 会重置 Offset／flags，swap 不交换 Offset／overlay 顺序；不能当完整 pose 提交。Reset、native dtor、Lua GC 和外层存储释放各自有 owner，失败 cleanup 仍要求稳定 cache／context；未实现客户端写回。

tx-03-18[单 Sprite 适配合同](../research/tx-03-18-single-sprite-adapter-contract.md)将资源就绪限定为最终 image/control 关联与已知 PNG 逻辑 loaded；不是 GPU 成功。descriptor+38 起的 layer 映射只有 64 项，direct draw 对 track 的 layer ordinal 没有自身范围检查。固定姿态方案先完整验证再在 reset 后填本地索引，保持停止／零事件；自有 registry、runtime guard 和客户端写回仍未实现。

tx-03-19[绘制窗口研究](../research/tx-03-19-sprite-render-entry-window.md)把独立 Sprite 对应到最终 present 前的屏幕叠加候选，明确 ECX payload 与三个 Vec2 指针的 Render ABI。正常 batch 复制图像强引用及 shader／uniform 状态，Sprite 调用返回、batch flush、资源最后释放与 GPU 完成不同；该候选不提供 Entity 房间遮挡或完整 Replica。

tx-03-20[资源与连续姿态研究](../research/tx-03-20-sprite-resource-pose-closure.md)确认基类 render 的 Sprite receiver=Entity+48；shadow pass 临时改 Color 后恢复，但 layer visible 尾部直接写0。持久姿态应纳入 render 前同一完整 Capture，不能按它推断全部合成外观。Poof 静态文件关联 Type1000/variant200 只作来源，不新增 Effect 支持；同 descriptor 原地更新／变更候选仍为设计。

状态同步用途：生命周期观察维护 Authority Registry；最终字段在完整采样阶段复制。客户端创建/写回/移除尚无完整闭环，不能把已安装 observer 当成 applier。

## 源码声明的补丁、调用依赖与自有表

| 安装家族 / 序号 | 类型 | site RVA → 原目标 RVA | 用途 | 字节 / 跨度 | 源码 |
| --- | --- | --- | --- | --- | --- |
| loop_runtime.cpp [16] | REL32_CALL | 0x00403E70 → 0x000186C0 | Room EntityList::Update | E8 4B 48 C1 FF / 5 | [loop_runtime.cpp:45](../../cs-t0/native/src/loop_runtime.cpp) |
| boundary_runtime.cpp [3] | REL32_CALL | 0x002FBB66 → 0x000147F0 | 边界实体更新调用 | E8 85 8C D1 FF / 5 | [boundary_runtime.cpp:42](../../cs-t0/native/src/boundary_runtime.cpp) |
| boundary_runtime.cpp [8] | REL32_CALL | 0x00028B3D → 0x002FE410 | Spawn 包装层调用 Game::Spawn 实现 | E8 CE 58 2D 00 / 5 | [boundary_runtime.cpp:42](../../cs-t0/native/src/boundary_runtime.cpp) |
| world_record.cpp [0] | ENTRY_PATCH | 0x002F0090 → 0x002F0090 | Factory acquire | 55 8B EC 51 8B 45 08 / 7 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [1] | ENTRY_PATCH | 0x002A93A0 → 0x002A93A0 | Entity::Init | 55 8B EC 83 EC 30 / 6 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [2] | ENTRY_PATCH | 0x00018500 → 0x00018500 | EntityList::Publish | 55 8B EC 53 8B 5D 08 / 7 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [3] | ENTRY_PATCH | 0x002ACB00 → 0x002ACB00 | Entity::Remove | 51 56 8B F1 57 / 5 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [4] | ENTRY_PATCH | 0x002ACB60 → 0x002ACB60 | Entity::ClearReferences | 55 8B EC 51 53 / 5 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [5] | ENTRY_PATCH | 0x002A8850 → 0x002A8850 | Entity destructor | 55 8B EC 6A FF / 5 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |
| world_record.cpp [18] | ENTRY_PATCH | 0x002FE410 → 0x002FE410 | Game::Spawn owner | 55 8B EC 83 E4 F8 / 6 | [world_record.cpp:1839](../../cs-t0/native/src/world_record.cpp) |

入口桥最小ABI（转发约定，不是完整高层参数语义）：

| RVA | 约定 |
| --- | --- |
| 0x002F0090 | ECX=this；2个raw32栈参数；ret 8 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x002A93A0 | ECX=this；4个raw32栈参数；ret 16 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x00018500 | ECX=this；1个raw32栈参数；ret 4 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x002ACB00 | ECX=this；0个raw32栈参数；ret 0 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x002ACB60 | ECX=this；0个raw32栈参数；ret 0 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x002A8850 | ECX=this；0个raw32栈参数；ret 0 bytes；GPR/FLAGS/FPU/SSE按bridge保存 |
| 0x002FE410 | ECX=this；8个raw32栈参数；ret 32 bytes；Spawn八栈词高层语义未全映射；GPR/FLAGS/FPU/SSE按bridge保存 |

## 当前 RTTI 与虚表候选

来源：[tx-01-64 RTTI原始记录](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json)。槽0..3只按原始表登记；Grid槽语义与Entity不同，不统一命名。下列槽并非全部被patch。

| 类名 | vtable RVA | TypeDescriptor IDA VA | slot0 / slot1 / slot2 / slot3 原函数RVA |
| --- | --- | --- | --- |
| ??_7Entity@IsaacRepentancePlus@@6B@ | 0x76730c | 0xc5d5dc | 0x002A8820 / 0x002A93A0 / 0x002AE7D0 / 0x002AE820 |

## 已登记字段

字段位置可读不等于可安全写回。RAW保留原位串；bool为byte，引用不发送裸指针；专用字段需满足初始化与当前generation读取依据。

| 字段 | 对象相对偏移 | 字节 | 证据范围 |
| --- | --- | --- | --- |
| Index | 0x20 | 4 | CURRENT_SOURCE_MIN_READ |
| Type | 0x28 | 4 | CURRENT_SOURCE_MIN_READ |
| Variant | 0x2C | 4 | CURRENT_SOURCE_MIN_READ |
| SubType | 0x30 | 4 | CURRENT_SOURCE_MIN_READ |
| EntityFlags | 0x168 | 8 | CURRENT_SOURCE_MIN_READ |
| valid | 0x170 | 1 | CURRENT_SOURCE_MIN_READ |
| visible | 0x171 | 1 | CURRENT_SOURCE_MIN_READ |
| exists | 0x172 | 1 | CURRENT_SOURCE_MIN_READ |
| dead | 0x173 | 1 | CURRENT_SOURCE_MIN_READ |
| removedByFactory | 0x177 | 1 | CURRENT_SOURCE_MIN_READ |
| Position.x | 0x33C | 4 | CURRENT_SOURCE_MIN_READ |
| Position.y | 0x340 | 4 | CURRENT_SOURCE_MIN_READ |
| Velocity.x | 0x360 | 4 | CURRENT_SOURCE_MIN_READ |
| Velocity.y | 0x364 | 4 | CURRENT_SOURCE_MIN_READ |
| HP | 0x380 | 4 | CURRENT_SOURCE_MIN_READ |
| MaxHP | 0x384 | 4 | CURRENT_SOURCE_MIN_READ |
| InitSeed | 0x3EC | 4 | CURRENT_SOURCE_MIN_READ |
| DropRNG word0 | 0x3DC | 4 | CURRENT_SOURCE_MIN_READ |
| DropRNG word1 | 0x3E0 | 4 | CURRENT_SOURCE_MIN_READ |
| DropRNG word2 | 0x3E4 | 4 | CURRENT_SOURCE_MIN_READ |
| DropRNG word3 | 0x3E8 | 4 | CURRENT_SOURCE_MIN_READ |
| Parent | 0x3BC | 4 | CURRENT_SOURCE_MIN_READ |
| Child | 0x3C0 | 4 | CURRENT_SOURCE_MIN_READ |
| Target | 0x3C4 | 4 | CURRENT_SOURCE_MIN_READ |
| Spawner | 0x3C8 | 4 | CURRENT_SOURCE_MIN_READ |
| SpawnerType | 0x34 | 4 | CURRENT_SOURCE_MIN_READ |
| SpawnerVariant | 0x38 | 4 | CURRENT_SOURCE_MIN_READ |
| birth frame | 0x328 | 4 | CURRENT_SOURCE_MIN_READ |

主要依据：[实际采集Spec](../../cs-t0/native/src/world_record.cpp)、[偏移修订](../../docs/refactor/cs-t0/t0-06/tx-01-69-offset-reconciliation.md)、[读取与引用规格](../../docs/refactor/cs-t0/t0-06/tx-01-71-sampling-design.md)。派生Entity共享字段另见[Entity](entity.md)。

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x00017260 | 0x00417260 | sub_417260 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00017270 | 0x00417270 | sub_417270 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00017280 | 0x00417280 | sub_417280 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00017290 | 0x00417290 | Position copy getter, Entity +33C/+340 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x000172D0 | 0x004172D0 | Velocity copy getter, Entity +360/+364 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x000172F0 | 0x004172F0 | PositionOffset two-component setter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00017310 | 0x00417310 | PositionOffset address getter, +34C | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00017420 | 0x00417420 | sub_417420 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00017430 | 0x00417430 | sub_417430 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00017470 | 0x00417470 | sub_417470 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00017480 | 0x00417480 | sub_417480 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x000174B0 | 0x004174B0 | sub_4174B0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x00017630 | 0x00417630 | EL push_back | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x00017670 | 0x00417670 | EL remove matching pointer | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x00017E50 | 0x00417E50 | EntityList destructor | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x00018230 | 0x00418230 | EntityList::Reset | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x00018500 | 0x00418500 | EntityList publication and updateEL conditions | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x000186C0 | 0x004186C0 | EntityList update/removal windows | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x00019750 | 0x00419750 | sub_419750 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00019E70 | 0x00419E70 | Spatial/collision processing and derived effectEL | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x0001A790 | 0x0041A790 | QueryRadius: partitions, filters and scratch state | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x0001AF60 | 0x0041AF60 | QueryType: ordinary, cache and wisp branches | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x000288F0 | 0x004288F0 | Position setter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00028910 | 0x00428910 | Velocity setter | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x00028B20 | 0x00428B20 | Game::Spawn wrapper | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x00035CD0 | 0x00435CD0 | sub_435CD0 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00056370 | 0x00456370 | sub_456370 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x00105B70 | 0x00505B70 | sub_505B70 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-68-static/ida-evidence.json) |
| 0x002A8180 | 0x006A8180 | sub_6A8180 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-69-static/ida-evidence.json) |
| 0x002A8290 | 0x006A8290 | Entity base constructor | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002A8820 | 0x006A8820 | Entity scalar deleting destructor | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002A8850 | 0x006A8850 | Entity base destructor | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002A93A0 | 0x006A93A0 | Entity Init: basic classification offsets | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002A98B0 | 0x006A98B0 | Entity config loading path used by virtual slot 0x64 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x002A9C10 | 0x006A9C10 | sub_6A9C10 | [ida-evidence.json](../../artifacts/cs-t0/tx-01-67-static/ida-evidence.json) |
| 0x002A9C30 | 0x006A9C30 | Entity HasFullHealth | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x002A9C50 | 0x006A9C50 | Entity AddHealth | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x002AC920 | 0x006AC920 | Entity Kill base implementation | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x002ACB00 | 0x006ACB00 | Entity base Remove | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002ACB60 | 0x006ACB60 | Entity base ClearReferences | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002ACE60 | 0x006ACE60 | Entity TakeDamage base implementation | [ida-evidence.json](../../artifacts/cs-t0/tx-01-65-static/ida-evidence.json) |
| 0x002AE820 | 0x006AE820 | Entity Update: queued damage is applied to HP, then possible virtual Kill | [primary-evidence.json](../../artifacts/cs-t0/tx-01-65-static/primary-evidence.json) |
| 0x002B0EB0 | 0x006B0EB0 | Position slot4: flag2 skip path, otherwise staging and movement | [ida-evidence.json](../../artifacts/cs-t0/tx-01-66-static/ida-evidence.json) |
| 0x002B7DB0 | 0x006B7DB0 | Append 56-byte damage record to Entity+8 vector | [primary-evidence.json](../../artifacts/cs-t0/tx-01-65-static/primary-evidence.json) |
| 0x002F0090 | 0x006F0090 | EntityFactory type selection | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002F2A10 | 0x006F2A10 | Pool construction: offsets, strides, classes | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002F3060 | 0x006F3060 | Game destruction including EntityFactory pools | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x002FE410 | 0x006FE410 | Game::Spawn implementation | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x0046FB20 | 0x0086FB20 | Native GetRoomEntities: updateEL source | [ida-evidence.json](../../artifacts/cs-t0/tx-01-63-static/ida-evidence.json) |
| 0x004704F0 | 0x008704F0 | Native FindInRadius Lua wrapper | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |
| 0x004707B0 | 0x008707B0 | Native FindByType Lua wrapper | [ida-evidence.json](../../artifacts/cs-t0/tx-01-64-static/ida-evidence.json) |

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。

## 源码原生定位常量补录

以下逐项保留当前源码的RVA/对象偏移命名和值，包括全局槽、vtable/COL、返回位置和只读诊断定位。常量不全是实际patch，旧命名不覆盖模块正文消歧；同一值在不同family中重复保留。`OBJECT_OFFSET`不加模块基址。

| 原始符号 | 类型 | 值 | 源码位置 |
| --- | --- | --- | --- |
| kEntityContainerOffset | OBJECT_OFFSET | `0x00018300` | [boundary_runtime.cpp:26](../../cs-t0/native/src/boundary_runtime.cpp) |
| kEntityListOffset | OBJECT_OFFSET | `0x00001218` | [boundary_runtime.cpp:27](../../cs-t0/native/src/boundary_runtime.cpp) |
| kEntityCountOffset | OBJECT_OFFSET | `0x0000004C` | [boundary_runtime.cpp:28](../../cs-t0/native/src/boundary_runtime.cpp) |
