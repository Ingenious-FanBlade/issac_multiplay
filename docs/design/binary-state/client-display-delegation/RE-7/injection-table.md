# 注入方式、原生存储与消费方

日期：2026-11-12；J273 静态结果。VA 基址 `0x400000`。`Wiki／SDK／SOURCE` 是 API／源码事实，`静态` 是指令与调用关系，`实现交接` 尚未接线。总范围见 [README](README.md)。

## 1. Wiki／SDK 优先来源

原版 [Room.md](F:/projects_isaac/IsaacDocs/docs/Room.md)、[Level.md](F:/projects_isaac/IsaacDocs/docs/Level.md)、[ItemPool.md](F:/projects_isaac/IsaacDocs/docs/ItemPool.md)、[EntityPickup.md](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)；官方 [ItemPool.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/ItemPool.zhl)、[Room.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Room.zhl)、[Game.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Game.zhl)、[ProceduralItem.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/ProceduralItem.zhl)、[ProceduralItemManager.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/ProceduralItemManager.zhl)、[RoomDescriptor.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/RoomDescriptor.zhl)、[EntitySaveState.zhl](F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/EntitySaveState.zhl)。

官方实现重点为 `repentogon/LuaInterfaces/LuaItemPool.cpp::Lua_ItemPoolUnidentifyPill`、`Room/LuaRoom.cpp::Lua_RoomSetWaterCurrent` 和 `ProcedualItems/LuaProceduralItemManager.cpp`。SDK 的 `_idendifiedPillEffects` 保留其原拼写。

## 2. 注入表

| 值 | J273 来源／消费者 | 安装交接与限制 |
| --- | --- | --- |
| 恶魔／天使房总出现率 T | `Room::GetDevilRoomChance VA 0x7EF200`（RVA `0x3EF200`），SDK `float __thiscall(Room*)`；`0x83D11D` 调用，`0x83D128 fstp` 接收 ST0。StatHUD 先 clamp，再用 `0x743800` 判断楼层／出口资格 | 会话内 getter 返回 Host 原始总率；不能把面板百分比直接返回本 getter。最终显示资格若依赖 Host 专属进度也须供给／注入，不能仅替换 T 就声称面板完整 |
| 天使占比 A | `0x7EF590`（RVA `0x3EF590`）；StatHUD `0x83D156` 调用后 `0x83D16E movaps xmm5,xmm0`。另有 `0x7439FD／0x83D761` caller | 独立原生 ABI shim 返回同步 f32 至 XMM0，无显式参数，不传伪 this，不用普通 `float(*)()` 强转。仅 Client 活跃且目标身份有效时替换，Host／停用时透传。它也被 Host 生成选择调用，绝不能全进程无条件覆盖 |
| Level 天使修正值 | `Game／Level+0x182F4`，原版 API 对应 `0x7DF730`，是 A 的生产输入 | 不替代 A，不为显示重算复制整局 flags／捐款／交易历史。旧概率桥按公式拆分的历史记录保留，新 getter 路径接通后评估退役，不能双供一个值 |
| 药丸效果表 | `Game+0x1A740` 的 ItemPool，`+0x7A4`，15×i32；SDK `GetPillEffect VA 0x72F030` | 直接安装 60B 有界表。原生 getter 还按 Player 的 PHD／Lucky Foot／Virgo／False PHD、挑战和回调解析；先装本目标 Player 库存，不能把每名玩家的最终效果覆盖成一张全局表。合法颜色低 11 位必须在已知 15 槽内，不利用 getter 的越界读取 |
| 药丸识别 | `+0x7E0..+0x7EE`，15B；Identify `0x713AF0`，IsIdentified `0x832630`；两者 mask `0x7FF` 后 clamp 至 0—14，`0x713B0D` 写 1，`0x83264D` 读 byte | 置 true 优先 IdentifyPill；清 false 用官方 UnidentifyPill 的等价 C++ 窄适配：校验索引后直接写 false。官方 Lua API 已存在，但没有对应独立 ZHL C++ 方法，不为正式安装调用 Lua。识别字节与 Player 效果解析分别维护，不把 Wiki 对最终显示行为的说明当成原始字节始终为 true |
| 有效诅咒 | 当前布局 `LevelGamePrefix.effective_curses`；`0x742450` 合并 Level+0xC、Game+0x25AC8 和 forced，再清 prevented 位，调用 `0x6F55C0／0x6F5750` | 保持 Host 有效值为唯一真值，库存后安装，原生 getter 在会话内注入；已知绕开 getter 的显示分支也从同一有效值适配。不能把有效值写进 raw_curses 再让本机强制／禁止规则重算一遍。raw/forced 的完整模拟不因此恢复 |
| 贪婪波次 | `Game+0x18334` u32；getter `0x7074C0`；原版属性 setter `0x74D700`（RVA `0x34D700`） | native setter 是有界普通赋值，将值限制至 0—12 后写 `0x74D717`；合法值可调用具名桥。若未来真实 Host 状态超出此 API 范围，须单独明确原值安装，不能静默截断。不是 SpawnGreedModeWave |
| Pickup Price／Keeper 交易 | 最终 `Pickup+0x534` i32；价格 Sprite `+0x410`。自动价格链 `0x80A790／0x80B5A0`；纯 GetPrice=`0x42D2B0` | 保留 Host 最终价与姿态，不在 Client 以本机买家重算。窄字段写入复用已核应用器；不得调用 `SetPrice 0x6DF890`，它会加载资源、选择动画，且 `0x6DFB22→Random`、`0x6DFB47→Spawn`。资源／数字图层另由 Sprite owner 安装 |
| TMTRAINER 定义 | `Game+0x669BC` 的 vector<ProceduralItem*>；项 `+0` ItemConfig_Item*、`+4` 负 id、`+0x10` targetItem、`+0x2C` effects vector；manager `+0x10` 图集 strong pair | 采选定显示定义：id、name／desc／gfx／可见配置及图集／资源依赖。effects、生成 RNG、属性算法不下发重演。Client 自有配置／资源 owner 建立稳定记录，负 id 查询从会话内注册表解析；不要 memcpy Host vector／指针，也不调用 CreateProceduralItem |
| 地图拾取图标 | `MinimapConfig::Render 0x9635B0` 的 `0x964572／0x964589` 取 descriptor；`0x96457F` 取 `+0x74` begin、`+0x78` end；`0x964B4D` 按 0x78 步长前进 | 选择地图刷新时提供 Host 图标摘要，按房间身份／dimension／描述版本绑定；Client 缓存适配只用摘要，不为图标 Restore／Spawn 保存实体，也不把摘要伪装为当前真实实体。适配尚未实施，原生刷新不会凭空得到摘要 |
| 水流 | `Room+0x7460/+0x7464` Vector；SDK GetWaterCurrent；官方 Lua SetWaterCurrent 直接 `*GetWaterCurrent()=*vector` | C++ 复用 SDK 指针写入两个有限 f32，符合 Q3；没有额外推进、随机或资源 owner。随 Room 目标提交，旧房指针不得复用 |
| 成就／计数等进度 | 复用现有 `client_hud_achievement_query` 等查询隔离和已有 Host 最终投影 | 只保留显示消费需要的 Host 结果；不复制整份 642／523 目录，不写 Client PGD／存档。新增消费者按具名查询接线，不以菜单／客户端原生成就状态替代 Host |

> **2026-11-12 审阅后标注**（原文保留）：
>
> - “有效诅咒”一行改为同步 raw：F=`0x6F55C0`（种子效果 56—62 及 stage／挑战／难度条件）与 P=`0x6F5750`（种子效果 63—70、79）只读 Seeds 对象 `Game+0x1BBD8` 与 stage／挑战／难度，均在 Run 同步清单中。`Level+0xC` 与 `Game+0x25AC8` 按普通字段随 world 安装后，原生 getter 在 Client 上算出与 Host 相同的有效值，直接读 raw 的消费者也一致；不注入、不挂 getter。把有效值写进 raw 仍然拒绝。现有布局 `LevelGamePrefix.effective_curses` 在第 7 步收缩时改为两个 raw 字段。
> - “地图拾取图标”一行的 Client 接法待定：镜像最小 SavedEntities 条目（让 `0x9635B0` 照常扫描 `+0x74/+0x78`），或原生构建后按摘要修补缓存，在第 8／13 步实施前选定（方案第 13 节）。

`0x7EF590` 返回已经 clamp 到 [0,1] 的占比。普通面板显示 `devil=(1−A)×T×100`、`angel=A×T×100`；Duality 分支两行均显示 T×100。`0x83CFD0` 还独立处理资格，不是总率 getter 自带全部资格。内部天使函数的入口字节为 `55 8B EC 83 EC 10 8B 15 F4 49 BE 00 F3 0F 10 05`（绝对地址随重定位处理）；尾部 `0x7EF81D minss xmm0,1`、`0x7EF828 retn`。不校验哈希，也不把静态字节当运行时定位已接线。

## 3. 三个不能直接调用的“setter／重建器”

**诅咒**：AddCurse `0x7442A0`（thiscall，两个栈参数，`retn 8`）先 OR raw+0xC，可选 ShowName，之后按挑战配置再强制／过滤并刷新 HUD。RemoveCurses `0x742B90` 的 `0x742BD0` 使用 `arg & 0xFFFFFF7D`，并非所有位都能移除；解除黑暗会写 Game 暗度计时，解除 Blind 会枚举 Pickup 并调 `0x6DF520` 重建外观。因此原方案“库存后按差异 Add／Remove 就一定安装最终值”的假设不成立，不能用重复 API 调用试到相等。现有有效值投影与 getter 注入遵循 Q3 的 Host 专属规则例外。

**价格**：`0x80B5A0` 枚举 PlayerManager，检查所有玩家生命／形态、Credit Card／饰品和商店折扣／涨价历史。它没有“当前买家 Player*”参数，不能把 isaac-lan 的每视图买家做法直接套到本工程同房原生多人世界。保留最终价／Sprite，购买裁决仍由 Host 完成。

**程序道具**：原生 `ItemConfig::GetCollectible 0x72A400` 对负 id 计算 `~id`，在 manager vector 中查项后返回其首指针；SDK／Wiki 的 CreateProceduralItem 是生成命令，不是按定义注册。自有记录必须先准备配置／资源、再供 getter，保持借用和退休寿命；原版程序图集的具体本地物化仍是资源实现事项，不能因找到了存储就宣称资源已可用。

## 4. 地图摘要应保留什么

原生循环先检查描述 `DisplayFlags+0x3C`，再读保存项 type／variant／subtype（+0／+4／+8）、`intStorage3+0x14` 和位置 +0x28/+0x2C；type5 与 type6 分别进入 Pickup／Slot 分类，按优先级去重、统计并选择 icon 动画／帧。药丸、心、硬币、钥匙、箱子和机器状态会参与不同分支，不能仅同步“有拾取物”一个 bool，也不能由当前房可见 Entity 数量代表全楼层。

Host 在正常地图缓存生产或等价已核只读投影中封存**每房间的有序最终图标选择及必要计数**；Client 在合法缓存刷新适配点读取。具体线格式／容量在版本72收缩时同源定义，不在本轮发明一个已运行的容器。该分支内联在较大的 MinimapConfig::Render，不存在已核的“SetPickupIcons”原生 setter，不能把中段地址强转为可调用函数。

挑战32的错误当前位置选择仍需 Host 选定结果：现有 `0x963680—0x9636D5` 会读 DecorationSeed 并调用 RNG，Q9 只同步当前 SpawnSeed，不能据此误认为 DecorationSeed 已获准同步或允许 Client 再抽。具体摘要 adapter／挑战32选择安装尚未实施，这些是明确的实现缺口，不是未知内存必须穷尽的要求。

## 5. 提交与验收交接

同目标 world／Player 库存 → 有效诅咒 → 药丸／波次／价格／概率与进度投影 → 地图缓存失效／刷新 → 显示推进。所有引用由当前 Client 身份解析，不下发 Player*；getter 只消费最近完整目标，缺目标时按既有 Client 隔离处理，不读取本机权威状态补值。

E38 在第15步覆盖两端库存差异、药丸识别往返、Host 专属概率资格、Keeper／多人价格、诅咒开关、Greed／Greedier、地图拾取／机器图标、挑战32和程序道具。尚无运行时正确性或性能结论；本轮没有恢复暂停的旧概率实现，也没有编写代码或测试。
