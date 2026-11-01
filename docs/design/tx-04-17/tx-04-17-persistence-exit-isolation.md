# tx-04-17 ST8：访客持久副作用与正常退出隔离

## 2026-11-01 当前设计与补充研究

现行合同与执行顺序见[tx-04-17主流程](tx-04-17-workflow.md)。R1—R4已补初始化、对象生命周期、维护绘制与资源借用；本轮[R5失败／退休／菜单交还](tx-04-17-failure-retirement.md)整合全过程保护及最后相关writer，补原生清理／状态转换的边界。R6已形成[模型／角色／应用器归并](tx-04-17-model-role-applier-integration.md)，具体接线与原生UNKNOWN仍未实现，未修改代码、构建或运行测试。

Client将在原生世界中构建与应用服务器状态，访客原有run／Continue／统计／成就／解锁和允许的本地设置仍各有归属。ST8关于保存、删除、直接进度写入和官方整链的静态事实继续有效。

**R5反馈（2026-11-01）**：选择自有退休编排，不调用Exit(false)取消，不用GameState Delete清运行时容器。正常Exit的Room重建先销毁旧owner后分配新对象，ResetState的官方ESSM包装又先访问有效backwards descriptors，均不保证半构造失败可恢复。保护始于首次服务器原生写入之前，覆盖Shutdown直接PGD／flush、early unload后资源及Manager析构SaveSyncing的来源；常驻持久归属策略与Client场景身份分开，只有服务器残留消费者／run状态已安全退尽才可在菜单转Off，不因最后writer要到进程末尾才存在而永久占用Client。Host／Off和访客随后合法run按正常归属透传，未知污染不能靠dirty／readonly／文件回滚消除。实际关闭接线、全部caller及异常仍有UNKNOWN，详见R5。

- 下文“Client不走任何Start／Restore”“必须独立场景”等属于旧设计限制。当前允许研究受控复用原生初始化子路径，但不允许借访客Continue建世界，不默认放行完整Start／Restore／Exit；是否存在可复用段仍由[R1](tx-04-17-workflow.md#r1)解释。
- 隔离须在原生构建前生效，覆盖临时初始化、create／remove／change、展示维护、失败／重建、stop与退出。原生场景有效不代表可以保存为访客run；仅拦磁盘写入或事后清dirty仍不足。
- [R5](tx-04-17-workflow.md#r5)沿新采用路径复查进度写入与清理顺序，明确保留的原生析构／菜单交还工作、禁止的保存删除以及最后保护owner。Exit(false)、readonly、write=false和early unload依然不是现成完整保护。
- [R6](tx-04-17-workflow.md#r6)扩展16退休条件并向31交付持久归属；源码现已有独立ClientRuntime，下文“当前没有Client”只代表研究时点。原有失败与UNKNOWN不因本次方案变更而改判。

**R1反馈（2026-11-01）**：主审原始指令确认Start在VA`0x6F18F2`直接写Manager`+0x15`的PGD readonly；不能先调用Start再恢复该位来保护访客。所选路径改为已有根上的自有准备编排，独立复用HUD资源／几何计算／受控Camera构造，替代整层生成、启动保存、Room模拟及game-start玩法Lua。Room Init几何前已有池位集和对象清理；PlayerManager在virtual Init之前已可发布列表，Camera替换在新分配之前已释放旧对象，失败不是“无写入”。这些事实要求从准备前保护到实际owner退休，不能让部分失败经Exit(false)或提前Off恢复原生接管；异常清理和完整返回菜单仍待R4／R5，不改判为隔离已实现。

**R2反馈（2026-11-01）**：主审独立回读NPC Morph：VA`0x6BD24F`以Manager`+0x14`的PGD调用`0x904DE0`。该helper在`0x904DEE`检查readonly；配置符合条件时，在`0x904E77`递增或`0x904E8F`写1到PGD`+0xF64` bestiary表项，随后还有条件通知。它证明change/Morph原生体可写进度，不仅外围Gameplay Lua存在副作用；具体统计项命名仍按静态依据保留，不从函数地址补造公开API。Client按应用上下文在这类消费者前隔离，不能用Morph返回、最终dirty或是否写盘证明全过程无影响。此补证不扩大readonly为完整保护，也不表示该分派已实现；其它派生Init／Spawn／Morph随实际采用路径继续补齐。

**R3反馈（2026-11-01）**：停止Gameplay Update后，原生PlayerHUD缓存更新仍可经ACTION_DROP查询进入MC_INPUT_ACTION；History即时重算会在官方super后分派1029，Room光照亦有带原生userdata的Lua。它们不能因调用点在维护／绘制阶段而获得通用Gameplay／PGD写权限，必要结构／资源维护与这些消费者分派分开。相机DoUpdate仍直达已知条件共享RNG路径，不能当本地展示时钟。R3没有新增“已穷尽全部持久写入”的结论；ST8整链保护、R2已证Morph写入和R4／R5的最后owner责任保持，显示失败或断连不提前Off。

## 2026-10-31 历史研究正文

以下保留当时事实、纠错、UNKNOWN和设计推导；冲突设计按上文及主流程执行，静态结论不升级为运行结果，历史分工不成为新要求。


日期：2026-10-31。状态：**指定范围静态研究与主智能体审阅完成；Client隔离与正常退休尚未实现。**必要逆向由用户指定的一个GPT-6 Luna／max子智能体完成，主智能体独立回读关键指令和源码，并补充官方PGD版本同步的关闭路径。目标为官方REPENTOGON 1.1.3与Repentance+ v1.9.7.12.J273。J273地址采用VA，`RVA=VA-0x400000`，主要入口同时列RVA。目标IDA输入及imagebase沿用[ST2场景研究](tx-04-17-client-scene-lifecycle.md)，仅只读反汇编／反编译／xref；没有改IDB、运行游戏、构建、测试、校验或保存artifact包。

本文把可直接读到的官方源码和J273指令列为`SOURCE`／`J273_STATIC`，基于它们得出的Client限制列为`DERIVED`，静态资料不能确定的项目列为`UNKNOWN`。这是持久设计研究文档，不代表功能交付或运行通过。

## 1 范围与先前研究

沿用本tx已完成的ST2—ST7合同：Client必须在`Manager::Update`消费启动请求之前隔离原生Start／Restore；完整Game／Room／Entity Update和未审定的Lua绘制回调不能继续驱动本地玩法；呈现资源则按ST6／ST7确认的主线程、Lua、ANM缓存和renderer/context寿命退休。本文只补访客进度、run保存／删除、文件／云、本地设置、Lua Mod数据与停止／进程退出。

已有J273研究已经确认：`Game::Exit(false)`不是无保存取消接口；当前`TryUnlock`、`IncreaseEventCounter`、本地／云保存及GameState读写／删除Hook主要用于观察且仍执行`super`；当前会话模式没有Client。详见[ST2](tx-04-17-client-scene-lifecycle.md)、[公共接口目录](tx-04-17-public-interface-catalog.md)、[ST6](tx-04-17-thread-context-lifetime.md)与[ST7](tx-04-17-lua-borrow-lifetime.md)。下文仅记录新增核验和与新源码对应的合同。

## 2 J273进度、run与退出调用

### 2.1 进度内存入口和只读／dirty边界

| 入口 | ABI、返回和静态行为 |
| --- | --- |
| `PersistentGameData::TryUnlock` VA `0x903930` / RVA `0x503930` | `__thiscall bool(int achievementID)`。原生函数在readonly或相应成就位已置位时可返回`false`；正常新解锁路径置成就位、调用后续helper并返回`true`。这只描述原生函数，不闭合REPENTOGON外层hook，也不表示`false`后所有caller都不再产生其它副作用。 |
| `PersistentGameData::IncreaseEventCounter` VA `0x903A50` / RVA `0x503A50` | `__thiscall void(int eventCounter,int amount)`。readonly为真时提前返回；否则更新`+0x2BC+4*index`的计数、置dirty byte `+0`，并按阈值调用`TryUnlock`。void入口没有可伪造的成功值。 |
| PGD布局 | REPENTOGON `PersistentGameData.zhl`：dirty／`changesmade` `+0`，readonly `+1`，achievement数组`+0x38`，eventCounter数组`+0x2BC`，`fileLoadedSuccessfully` `+0xF8C`。布局说明字段，不构成通用保护合同。 |

`J273_STATIC`：`TryUnlock`与`IncreaseEventCounter`的原生readonly检查覆盖各自部分路径。`Game::Exit`和`Isaac::Shutdown`另有直接保存、清dirty及只读位清除；REPENTOGON进度扩展还可临时绕过readonly或直接写自己的文件。因此，readonly不支配整个访客操作窗口，dirty终值为0也不能说明中途没有写入。

### 2.2 Shutdown本身先写进度，再保存

`Isaac::Shutdown` VA `0x982C90` / RVA `0x582C90` 在进入通用PGD保存分支前还有条件性进度路径。原始指令显示：

1. VA `0x982D1B`先要求Game非空；VA `0x982D1F`检查`Game+0x25BA8`且要求其大于零；VA `0x982D28`要求byte `Game+0x25BB0`为零；VA `0x982D31`要求dword `Game+0x25CB4`为零。
2. 满足这些条件时，VA `0x982D44`直接把`PGD+0x5BC`写为零。按PGD计数数组基址`+0x2BC`计算，这是eventCounter[192]；该写入绕过`IncreaseEventCounter`与readonly判断。
3. 随后VA `0x982D4E`调用`IncreaseEventCounter(522,1)`，VA `0x982D5C`调用`SaveLocally`。官方Wiki对counter 192的名称标注不用于推断本分支语义；counter 522及三个Game条件字段的产品含义均为`UNKNOWN`。

`J273_STATIC`：后续VA `0x982D72`开始另一条独立PGD保存分支：Manager相应指针存在，PGD dirty `+0`和`fileLoadedSuccessfully +0xF8C`均满足时，VA `0x982D91`先清dirty，VA `0x982D94`调用`0x9032A0`备份helper，再按Steam Cloud上下文和开关在VA `0x982DBD`调用云保存或VA `0x982DC6`调用本地保存。它没有以Game当前是否处于run状态作为该分支的守卫。随后VA `0x982DD0`比较Manager state，`0x982DD4`在不等于2时跳过；等于2才于VA `0x982DD8`调用`Game::Exit(true)`。原始patch位置是VA `0x982DDD` / RVA `0x582DDD`，五个原始字节为`B9 78 6F BE 00`。

`DERIVED`：Shutdown的Client隔离边界必须早于上述直接清零、计数器递增、PGD flush和`Game::Exit`。把Client拦截放在`MC_PRE_MOD_UNLOAD`或Lua关闭阶段已经太晚；该回调只能补做资源清理，不能作为第一道持久隔离。具体适配必须保留正常teardown，不得通过整个Shutdown直接return来绕过必要的对象、Lua与图形关闭；该角色条件适配尚未实现。

### 2.3 Game::Exit、run保存与条件删除

`Game::Exit(bool ShouldSave)` VA `0x6F6270` / RVA `0x2F6270`由ST2复用并重新核对，官方`Game.zhl:135`的ABI为`__stdcall void(bool)`。active分支在VA `0x6F635E／0x6F6363`分别检查PGD dirty和`fileLoadedSuccessfully`；满足时VA `0x6F636E`清dirty，VA `0x6F6371`调用备份helper，继而在VA `0x6F639A`走云保存或VA `0x6F63A3`走本地保存。这些调用早于VA `0x6F63B5`检查`ShouldSave`。随后`ShouldSave=true`在VA `0x6F63B7`调用Manager保存helper `0x932160`；false在VA `0x6F63BE`调用`0x9324A0`。退出后段VA `0x6F647B`将PGD readonly清零。

`0x9324A0`的条件性删除路径：当Game+`0x25BA8`为零且Manager+`0x4AFB8`与`+0x4AFBC`两个值相等时，VA `0x9324CD`调用`GameState::Delete` VA `0x99E7B0` / RVA `0x59E7B0`；否则不沿该分支删除，不根据比较本身推断容器类型。因此不能说每次`Exit(false)`都删除run，但false也绝不能被当作“no-save取消”。J273 `Delete`在本地路径调用文件删除并进入`0x99DE10`清理；云路径也条件删除后清理。`DeleteRerun` VA `0x99E870` / RVA `0x59E870`是独立的rerun删除入口，SDK/ZHL声明返回void，不把伪码推断的原始`remove`返回当业务结果。

`Manager::SaveGameState` helper VA `0x932160` / RVA `0x532160`在合格游戏态路径中先保存GameState，再写入IO。两个已核调用点VA `0x932300`和`0x9323F0`调用`GameState::write` VA `0x99F7A0` / RVA `0x59F7A0`；返回bool后caller没有检查，继续关闭IO及PGD dirty／备份／本地或云保存收尾。因此下层write被短路并返回false，不能证明上游caller已经停止、没有其它副作用或安全恢复。`GameState::read` VA `0x9A2240` / RVA `0x5A2240`的ZHL ABI为`__thiscall bool(GameStateIO**,bool isLocalRun)`；GameState write为`__thiscall bool(GameStateIO**)`；Delete和DeleteRerun为`__thiscall void()`。

`SOURCE`：官方[GameStateManagement.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/GameStateManagement.cpp:180)在vanilla成功后再序列化REPENTOGON sidecar；普通write可因VirtualRoomSet `WriteSave`失败返回false，rerun write对未知slot在vanilla成功后会返回true。read在vanilla读取后还验证／反序列化ESSM、VirtualRoom与ItemPool数据；`isLocalRun=false`有跳过sidecar读取的专门分支。Delete/DeleteRerun在调用原生super之后，还会清ItemPool、VirtualRoom与ESSM的对应保存数据（同文件:207、:261、:289、:316、:390）。这些bool只描述各层实现结果；vanilla已做的读写或上游继续执行不能由返回值回滚。

## 3 文件、云、扩展进度、设置和Mod数据

### 3.1 原版PGD、run文件及REPENTOGON扩展

| 数据类别 | 已核源码／二进制事实 | Client设计归属 |
| --- | --- | --- |
| 原版PGD | VA `0x903460` / RVA `0x503460` `SaveLocally()`和VA `0x902F00` / RVA `0x502F00` `SaveToSteamCloud()`都是void。原生路径开文件、序列化并flush；不存在可由hook返回的“保存失败”值。`Game::Exit`及Shutdown都有直接调用。 | Client不把服务器进度写入访客成就、计数或PGD文件／Cloud；Host、Off及明确归属的合法本地生命周期保留原有语义。不得翻readonly、清dirty或以最终文件未变作无副作用证据。 |
| GameState／Continue | 官方[SaveSyncing.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/SaveSyncing/SaveSyncing.cpp:46)给REPENTOGON PGD定义本地`Repentogon/rgon_persistentgamedataN.dat`与云端`rgon_steam_persistentgamedataN.dat`；GameState共享本地`gamestateN.dat`、云端`rep+gamestateN.dat`。这是源码路径映射，不是本轮运行时观察。 | Client禁止以本地GameState/Continue作为准备容器，也不得新建、覆盖或条件删除访客run；服务器世界只由Authority持有。 |
| REPENTOGON成就 | [AchievementsStuff.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/AchievementsStuff.cpp:63)会把自定义achievement map写入`GetRepentogonDataPath()+achievementsN.json`。扩展`TryUnlock`的base ID `0..640`在`forceunlock`时临时清readonly再调用super；自定义成就路径可直接改内存map、运行tracker并写JSON。需要弹窗时还会临时改PGD dummy achievement并再次临时清readonly。 | 不能只挡vanilla PGD写盘或信任readonly。Client不得把服务器解锁写入访客扩展JSON；扩展成就和renderer／游戏Lua callback也要受角色边界控制。 |
| CompletionTracker及挑战 | [CompletionTracker.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/CompletionTracker.cpp:641)将完成标记写入`GetRepentogonDataPath()+completionmarksN.json`，多个进度更新路径可调用该写入；[ChallengesStuff.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/Patches/ChallengesStuff.cpp:355)的`AddChallenge`扩展分支修改Challenges map、可调TryUnlock，再写`challengesN.json`，不经过原生分支的readonly检查。 | 即使没有第三方gameplay Mod，REPENTOGON自有扩展也存在条件性进度文件路径。未分类扩展数据不随Client场景写入访客档。实际是否触发是运行未知。 |

`SOURCE`，主审补充：官方[SaveSyncing.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/SaveSyncing/SaveSyncing.cpp:611)还有PGD版本同步。启动Hook在super之后加载同步配置、初始化三档所需文件并尝试自动同步；`Manager::SetSaveSlot`也会在super前尝试初始化缺失档。`Manager::destructor` Hook在super之前调用`PerformAutomaticVanillaSaveSynchronization(false)`。自动同步有启用及启动同步成功条件，满足时遍历1—3档，分别经自有`SaveFile::Save`写RGON／原版PGD，并写本地`savesyncstatus.json`或云端`rgon_savesyncstatus.json`；不是所有关闭都会写，也不是所有写入都经过当前PGD的SaveLocally／Cloud观察入口。源码使用的内部格式校验字段仅作为原有实现阅读，本轮不计算、执行或要求其校验。

`DERIVED`：原生启动前的用户本地存档初始化／版本同步属于需识别的本地生命周期，不能笼统归罪为Client新增写入；连接准备不得借SetSaveSlot主动触发访客进度导入。关闭时不能因early unload已完成就撤销持久归属管理，Manager析构整链仍可能有文件写入。正常本地同步可以保留其产品语义，前提是服务器世界从未污染其内存或文件输入；不能依赖关闭后再合并、回滚或清dirty纠正已经产生的服务器进度。具体角色接线及异常／部分成功处理仍未实现。

`DERIVED`：进度隔离必须落在做裁决／改变内存进度之前，并覆盖RGON自有扩展写入；只在`SaveLocally`/Cloud处挡文件不够，因为内存counter/achievement与RGON JSON可先行改变。只在`TryUnlock`处返回false也不够覆盖`IncreaseEventCounter`、直接字段写、CompletionTracker、挑战callback或Mod数据。

### 3.2 本地设置是另一归属

`SOURCE`：[OptionsSyncing.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/SaveSyncing/OptionsSyncing.cpp:92)的`OptionsConfig::Load`可初始化或同步`options.ini`与`repentogon/vanilla_options.ini`并写配置；`Save`在调用原生保存后同步两份INI、可能复制新文件，分支失败时可能已有前序文件操作。它是用户本地设置，不是访客run/服务器进度。`InputConfigs.cpp`将`SaveButtonMaps`/`LoadButtonMaps`路径分为`inputconfigs.dat`和`inputconfigs_rgon.dat`，且启动时可能首次导入并保存REPENTOGON本地选项。

`DERIVED`：Client可按明确产品策略允许有限的本地选项／输入映射写入；不能因此允许任意PGD、GameState、achievement、completion或Mod SaveData。设置保存流程会同步和复制文件，不能把调用名含`Load`的路径整体视为只读。

### 3.3 Lua Mod保存不是在线隔离，也不能笼统视为设置

- `PUBLIC_DOC`：原版公开`ModReference:SaveData/RemoveData/LoadData`与`Isaac.SaveModData/RemoveModData`接口。没有从目标二进制／当前官方源码重新闭合vanilla `Isaac.SaveModData`底层最终路径，故本文不采用Wiki描述代替真实写入位置；这类数据跨run持久化，具体内容与写入时机由Mod决定。
- `SOURCE`：REPENTOGON [`onlinestub.lua`](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/resources/scripts/repentogon_extras/onlinestub.lua:63)保存原函数到`UnsafeFuncs`。只有原版Online stub的`isonline`为真时，Save/Remove才转成内存stub；否则调用原函数。C/S是独立session与协议，不能依靠该OriginalOnline状态拦截Mod保存。
- `SOURCE`：另一套内部API在[EntitySaveStateManagement.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/SaveStateManagement/EntitySaveStateManagement.cpp:3374)：`_CBindings.ESSM.SaveData`路径由`mod._dataDirectory/EntitySaveStateManagement/(filename+".dat")`组成，`SaveData`用`ofstream`写；`DeleteData`用`filesystem::remove`。两者通过内部Lua binding注册（:3474），与vanilla ModReference SaveData不是同一API。ESSM Save不会向Lua返回写入结果，也未在此函数检查`file.write`状态。

`DERIVED`：C/S允许明确定义为本地UI配置的数据；opaque的第三方Mod存档、删除或卸载callback不自动归为设置白名单。Client运行时须禁用任意gameplay Mod callback或经过明确声明的安全presentation callback边界；任何保留的Mod保存行为需单独分类。REPENTOGON源码说明其提前`MC_PRE_MOD_UNLOAD`的patch发生在游戏保存后、Game/Manager销毁前，但当前安装和实际命中仍`UNKNOWN`；该callback可能再调用Lua保存，不能作为先行隔离。

## 4 允许／隔离矩阵

| 操作 | Host | Off／原生 | Client |
| --- | --- | --- | --- |
| Authority run、GameState、PGD／成就／计数／挑战／解锁 | 按原生规则保存房主自己的状态 | 原样透传 | 访客不创建或载入本地run，不执行本地裁决，不将服务器进度写入访客PGD、Cloud、GameState或RGON进度JSON |
| GameState write/read/delete、Continue／rerun与Exit | 房主合法Run生命周期 | 原生菜单和run操作 | 不用访客档当Replica；不得用`Exit(false)`取消；不得悄悄删掉、覆盖或带入用户已有Continue |
| Options／input配置及已声明本地UI偏好 | 原样支持 | 原样支持 | 仅允许明确列为客户端本地偏好的文件和字段；记录归属，限制副作用到该设置域 |
| Vanilla Mod SaveData、ESSM内部sidecar、卸载callback | 保持本机Host Mod政策 | 保持原native/Mod政策 | 默认隔离未分类持久数据；自有presentation配置需独立命名并明确本地产品语义，不能套用OriginalOnline stub |
| Client资源与状态退休 | 不适用 | 不适用 | 关闭输入和publication、停止新draw／游戏Lua callback，排空在途owner后按ST6／ST7顺序退休；直到安全完成一直保留Client隔离 |

矩阵表达设计归属，不代表当前代码已经实现这些短路。若以后以bool hook抑制写入，应如实返回失败而不伪造成功；void入口须由其已解释的调用边界阻止进入，而不是靠写后复原或dirty归零。

## 5 正常停止顺序与接线要求

1. **建立角色前保留访客现状。** 在进入Client准备之前，tx-16／31要记录并明确本地save slot、已有Continue和run归属。它们由访客原生玩法拥有；不得复制seed再`Start`一局、`Continue/Restore`后借作Replica，亦不得在连接／stop时偷偷删除或覆盖。既有run如何与房主session交接由16／31明确产品行为。
2. **Client角色在连接状态之外持续。** 在消费`Manager::ExecuteStartGame`之前切到Client隔离（Manager Update VA `0x92E3E0` / RVA `0x52E3E0`，早于Start helper调用点VA `0x92E46E`）；socket断开、准备失败、返回菜单或停服不会自动变成Off。`Retiring`/`Failed`未满足退休条件仍维持Client，不交还残留Game对象给本地模拟。
3. **禁止本地进度路径而保留限域设置。** Client不走Game Start/Restore、Manager SaveGameState、Game::Exit false及本地authority tick；Client期间不调用pgd progress入口、GameState保存／删除或未分类Mod持久API。Options和input配置按矩阵只开明确设置路径。Host、Off分支仍exactly-once调用原生链。
4. **Client Stop使用自身退休流程。** 先关输入与网络publication，再禁止新玩法/render Lua callback，让现有callback、draw和相应队列owner在已解释的主线程阶段收尾；之后按ST7先于Game/Manager/ANM cache、再于Lua state、最后ImageManager/renderer/context的各自截止点退休。不阻塞主线程，不等待网络／磁盘或在DllMain join；无法确认排空时停止相关新使用并保持必要隔离，不能伪造退休完成。不要调用`Game::Exit(false)`来取消场景，也不要在资源安全退休前先切Off。
5. **进程Shutdown的最早边界保留隔离。** Client在Shutdown入口前停止上述玩法/持久caller，包括VA `0x982D44`直接PGD写、`0x982D4E` counter调用、`0x982D5C` local save及VA `0x982D72`独立PGD/Cloud flush；仅PGD写盘Hook或late unload不足。Host的合法保存不受Client条件影响。
6. **早期卸载callback只做补充清理。** REPENTOGON `RunPreModUnloadCallbacks`源码patch点针对`Isaac::Shutdown`保存后、对象销毁前；实际安装未知。若运行时可用，它可以协助清除本方已停止接受新调用的回调／Lua持有资源；不能作为首次隔离点，也不能自动视作context或最终图像引用已退休。Manager析构官方PGD版本同步仍可能更晚运行，持久归属管理不能随该callback提前退休。Shutdown后的Game/Manager、Lua、ImageManager和context清理次序按ST6／ST7分别满足。

### 接现有观察实现的限制

当前`SessionMode`是`None/Solo/Host`，无Client角色。`gen_binding.py`生成的`Game::Exit` wrapper在记录`RunTeardownBeginning(true)`后无条件调用`super`，PGD/GameState wrapper亦保留原调用；`PersistenceScope`只观察dirty、readonly、loaded、相关字段与正常返回。`exit_patch.cpp`的进程退出patch受`WorldObservationArmed`/World capability条件保护，`WorldProcessExitDecision`服务于世界录制路径，不是Client持久拦截。故这些接口为记录依据，不是当前安全措施；添加Client角色后必须把Shutdown直接写入者、Native Exit、GameState上游caller和RGON侧carriers按同一生命周期接线。

## 6 向15／16／31反馈与`UNKNOWN`

### 反馈

- **tx-15状态所有权**：服务器Authority progress/run由Authority持有并合法保存；需要同步的进度展示值和已声明Mod状态进入Client私有展示／复制域，不能直接应用到guest原生PGD、Continue文件或Mod持久档。opaque值的观测不自动赋予持久写入语义，客户端本地选项与Authority进度分离，不因此缩小正式同步应覆盖的世界／HUD内容。
- **tx-16 Client生命周期**：角色身份不是socket状态；在`Manager::Update`的ExecuteStart前启用隔离，涵盖Preparing、Displaying、Retiring和Failed。角色交接前识别访客已有Continue归属；正常stop不是`Exit(false)`或Native run delete。
- **tx-31持久归属**：房主按原native语义保存；访客不得镜像服务器成就、counter、挑战、RGON自定义achievement或completion marks到自己档。显式允许的本地设置需与run/progress/Mod保存逐项分域；持久副作用观测须覆盖完整操作窗口，不能只比dirty/file首尾。
- **tx-18—20／27场景与展示适配**：对象准备、Player／Entity退休和HUD／render保留路径均沿用本合同；原生构造／析构、动画事件和Lua callback若能触发进度或Mod保存，不能仅因用于展示而透传。使用Authority值显示奖励／完成标记，不再触发本地进度裁决。

### 未闭合事实

1. Shutdown分支中Game+`0x25BA8`、`0x25BB0`、`0x25CB4`三个条件字段和counter 522的精确玩法含义；本静态研究不猜解。
2. 所有原生进度caller、全部动态Lua callback／已装载Mod、用户自定义SaveData内容及异步/异常写入未全量闭合；不据当前source callsite宣称无其它写入。
3. vanilla `Isaac.SaveModData`底层文件owner/path在本轮未由目标二进制或官方实现源码闭合；Wiki位置说明不替代该事实。ESSM内部`_CBindings` sidecar已按源码确认，但实际调用／写成功未知。
4. REPENTOGON自有的achievements／challenges／completionmarks JSON及PGD版本同步是否在本Client安装及特定场景实际运行、shutdown patch是否装载命中、Game/Manager hooks优先级是否包住现有链，均无实机观察；只阻断原生body不证明官方整链无副作用。
5. PGD/GameState/设置调用在此次操作窗口的实际命中、Steam cloud最终写入、内存／文件前后变化和访客原进度归属由tx-04-17完整实现后的统一E2E覆盖；静态研究不记录运行PASS。
6. Client场景壳、Shutdown角色条件适配、官方完整Hook链位置、保存请求caller失败处理及常驻持久归属协调尚未实现。若保留的presentation入口实际需要调用vanilla Mod保存，必须在接入前补其具体owner、返回和失败后态，不从当前UNKNOWN推导允许调用。

### 互核纠错记录

首轮笔记曾从Hex-Rays变量索引误算Shutdown guard字段。主审重新读取原始指令后，条件修正为Game+`0x25BA8`、`0x25BB0`、`0x25CB4`；同时补认VA `0x982D44`对PGD eventCounter[192]的直接清零发生在`IncreaseEventCounter(522,1)`之前，故readonly不能挡住该直接写。本文采用上述原始指令结论，撤回初稿偏移与条件含义，保留counter 522为UNKNOWN。

主审还纠正了AddChallenge扩展所属源码为ChallengesStuff.cpp，删除条件改记原始字段比较而不臆定容器类型，补明Game::Exit的stdcall／void ABI及官方PGD版本同步的启动、选档与Manager析构路径；并将正常teardown、主线程非阻塞退休和Authority展示值／访客持久档的区别写入合同。

本轮完成全部文档编辑后的统一人工静态回读，只核对引用、规则一致性和指定范围；未运行游戏、构建、测试、哈希或格式／语法校验器，无artifact包。ST8研究与主审完成不表示tx-04-17 Client隔离功能完成。

## 7 2026-11-02 I7实现与统一验收中的持久归属补充

本节续记tx-04-17实际实现和统一E2E中发现的具体问题；以上2026-10-31／11-01历史事实、失败与当时UNKNOWN原样保留。源码实现、已经观察到的个别运行结果和仍待完成的最终验收分别陈述。本节文档续写只读现有源码、trace与已产生的before／exit文件，未另行构建、运行测试、启动游戏或计算／验证任何校验值。统一验收最终覆盖与结论由主流程后续更新，不能据本节单独宣称E9或整个tx通过。

### 7.1 首次原生准备前的Lua与native保护

`IMPLEMENTED`：[client_native_bridge.cpp](../../../mod/src/game/client_native_bridge.cpp)将持久API保护接在官方初始化实际时序上，而不是等连接后再替换全局函数。

- `LuaEngine::RegisterClasses`外层正常返回后调用`ClientGuardLuaBindings(true)`，在vanilla主脚本取得别名前保护`Isaac`持久API及`PersistentGameData`元表方法；基础入口缺失或安装失败不能由后续连接时的包装补作“早期别名已保护”。
- 官方`LuaEngine::Init`在其super返回后才创建`_CBindings`，随后调用bundled `enums_ex.lua`／`main_ex.lua`。本方在bundled script之前保护`_CBindings.ESSM.SaveData/LoadData/DeleteData`，使ESSM模块捕获的local别名和onlinestub保存到`UnsafeFuncs`的原函数已经经过保护。官方`_LuaBindings`中的Entity／ESSM结构维护保持原链，不能把它们与持久sidecar写入混为一类全部跳过。
- 只有基础保护、ESSM早期保护和`main_ex.lua`正常完成均成立，才在所属VM私有registry表中记录最终owner及函数身份。连接时只核对该身份；被替换、缺失或VM不符时拒绝继续，不能重新包装后假称旧alias安全。身份表由VM拥有，未在C++保留跨VM的registry编号，不每帧重建。
- `PersistentGameData:Unlock/TryUnlock`的官方Lua函数会在调用native `TryUnlock`前修改`forceunlock/nextSkipAchiev`。本方因此在Lua元表入口之前拒绝；仅让下层native返回false不足以覆盖这两个官方全局状态。计数、bestiary、boss、挑战和completion setter亦各按已解释入口隔离，ModData读写／删除及ESSM持久API没有被归为本地设置白名单。

`IMPLEMENTED`：[hooks.json](../../../mod/binding/hooks.json)和[gen_binding.py](../../../mod/tools/gen_binding.py)中的Client持久分派在官方整链副作用之前拒绝Start／Restore／Exit、Manager保存／选档／完成标记、PGD进度及保存、GameState读写／删除／rerun等已采用入口。bool入口返回false，void入口不进入super；Host及正常Off保留原调用。主动拒绝不记作original正常返回，[persistence.cpp](../../../mod/src/game/persistence.cpp)另计`persist_client_denied_native/lua`并记录固定入口名，供完整窗口归因使用。

本轮补认`Manager::RecordPlayerCompletion`为必须保护的独立入口：官方`CompletionTracker.cpp:899`的priority `100` Hook可在super之前直接改变自定义角色`CompletionMarks`并保存JSON，不能由`IncreaseEventCounter` Hook代替。J273已核VA `0x933D40`／RVA `0x533D40`，SDK ABI为static `__stdcall void(int)`；本方清单采用priority `-20000`。官方libzhl的Hook表按priority降序安装，每次新detour包住旧入口，故较小priority最后安装在外层，覆盖官方`100`、`0`和`-9999`层；这项源码事实不意味着任意第三方动态补丁也受同一排序支配。本方不据此承诺任意第三方native IO隔离。

### 7.2 Shutdown直接写、ASLR修正与关闭来源的保护寿命

`IMPLEMENTED`：[exit_patch.cpp](../../../mod/src/hooks/exit_patch.cpp)中的常驻Shutdown桥分别处理counter直接store分支和dirty／backup／cloud／local flush分支，保留Options以及后续对象、Lua和图形关闭。不能整个return `Isaac::Shutdown`，也不能把late unload当第一道保护。

本轮修正了第二段补丁的ASLR操作数处理：原`A1`指令的四字节绝对Manager槽地址会被PE重定位，不能把首选基址常量当作每次运行的指令值。清单现用`A1 ?? ?? ?? ??`核对指令形状，安装时另读取实际操作数并要求其等于`Base + offset::Manager`；shim通过该实际槽载入Manager，在正常和跳过保存两条路径都保留原载入语义。不匹配时停用相关Client能力，不猜地址或绕过检查。

本轮还修复了关闭请求与Client身份解除之间的保护空档。窗口Close若始于Client，`ClientProcessExitDecision`立即登记进程关闭来源并置`client_exit_progress_guard`，再推进场景／输入／网络退休；直接进入Shutdown且Client仍在时也登记该来源。`ClientPersistenceBlocked()`同时检查活跃Client及该进程退出标志，因此场景已安全退休、`ClientRuntime`转Off后，余下Shutdown中的持久消费者仍保持隔离。trace用`process_exit_progress_guard`说明这一来源，不能只凭随后出现`event=off`就恢复进度写入。

该标志服务于已经开始的进程关闭，并非把正常菜单交还永久变成Client。正常stop完成后的合法本地玩法仍按其自身归属运行；源码与本节没有引入readonly翻转、dirty清零、存档快照恢复或退出后合并修复。设置仅保留已经声明的本机Options／输入配置路径及其本地来源，不把opaque ModData、进度JSON或run请求放宽为设置。

### 7.3 统一E2E第12次L：vanilla PGD改变的实际归因

`RUNTIME_OBSERVED`：本轮统一验收L的诊断位置为`out/tx17-e2e-20261102-l`。该次进程先运行SourceHost采集局，退出到菜单后才取得`client_before`，随后运行Client及正常窗口关闭。严格Client窗口的文件比较发现`steam_remote/387111160/rep+persistentgamedata2.dat`改变，RGON对应PGD未改变；该失败现象保留，未通过扩大白名单抹去。

本次续写只对已有文件读取并比较原始字节。vanilla before／exit长度均为16932，全部差异仅在`0x10–0x13`和`0x4220–0x4223`。官方`SaveFile.h`及`SaveFile::ReadGamestateChecksum/SetGamestateChecksum`表明，前者是PGD中保存的Continue／GameState关联字段；后者是文件最后四字节的原有校验字段。本方没有计算、验证或重签任何校验值。两者之间的成就、计数、bestiary、设置等数据块，以及倒数第二个四字节计数均未变；`rep+gamestate2.dat`在Client before／exit之间全部57276字节相同。

下表只列直接读取的`0x10`字段值，用于说明归属时序，不执行任何校验算法：

| 已有快照阶段 | vanilla PGD `0x10` | RGON PGD `0x10` |
| --- | --- | --- |
| `save_before`，本进程SourceHost前 | `014BBF6F` | `E4AC898A` |
| `client_before`，SourceHost已退菜单 | `E4AC898A` | `0242652A` |
| `client_after` | `E4AC898A` | `0242652A` |
| `client_exit` | `0242652A` | `0242652A` |

`RUNTIME_OBSERVED`：L的trace `seq10689`记录`process_exit_progress_guard connection=4`，之后依次为retiring、Off、`isaac_shutdown`、Lua销毁及正常Shutdown返回；关闭来源保护没有因先转Off而丢失。上述文件字段说明，新关联值`0242652A`在Client开始前已存在于SourceHost保存的RGON PGD；Client窗口内没有再生成另一代run关联，退出时只是vanilla追上该已有值。`rgon_savesyncstatus.json`的before／exit文本也相同。

`SOURCE/DERIVED`：官方[SaveSyncing.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/SaveSyncing/SaveSyncing.cpp:501)对`updateVanillaGamestateChecksum`作独立判断；即使`syncVanilla/syncRgon`均为false、不需要同步进度，只要Continue关联值不同仍可继续执行。在同文件`:557–574`更新该字段并调用`vanillaSave.Save()`；`:642`的Manager析构Hook发起关闭同步。实际最终写入经[SaveFile.cpp](F:/projects_isaac/REPENTOGON-1.1.3/repentogon/SaveSyncing/SaveFile.cpp:716)的`SaveToSteamCloud → SteamCloudFile::PushToSteamCloud`，不经过PGD的`SaveToSteamCloud`观察／拒绝入口。因此这次是实际发生的官方最后writer写入，但现有字节和调用合同共同将其归因为同进程SourceHost已存在Continue关联的延后同步，而非Client新增成就、统计或其它进度块漏写。它不是把整个PGD文件判成“允许改变”。

### 7.4 独立进程验证设计与保留边界

`PENDING`：后续统一验收将Source采集与Client正常关闭放在独立游戏进程。Source进程先按其合法Host生命周期保存并正常退出，让官方最后同步完成；Client进程随后从其自身启动／选档完成后的真实本地状态建立窗口基线，再执行Client准备、应用、失败／重连、退休和正常关闭。这一设计去除“在同一进程Client窗口之前已有Host待同步Continue”的归因混杂，仍要求严格检查完整Client窗口，不扩大PGD白名单，也不以跳过失败项记通过。本节写入时，该独立Source／Client进程方案的最终运行结果尚待主智能体统一记录。

Manager析构的官方合法本地同步继续保留。服务器来源从未污染其输入时，正常同步访客原有进度／Continue关联有独立本地归属；不能因存在最后writer就禁用整个析构、跨进程永久关闭同步或阻断必要teardown。若以后确有产品要求限制“Client来源关闭时的官方自动同步请求”，必须在该具名请求进入整链之前说明来源和失败后态，同时继续正常析构；不能只在底层拒绝文件写入后让上游冒充同步成功。

最终文件未变、仅元数据变化、dirty终值、readonly或某次拒绝计数都不能单独代替全过程理解。I7仍须与原生准备前的消费者分派、允许的本地设置来源、失败／断连后的场景隔离、Close来源、实际Shutdown及官方最后writer联合判断。现有结论限定于本tx已采用入口与实际观察窗口；未知第三方native IO、任意异常／强制终止和未采用的后续路径没有因此变成已闭合保证。

### 7.5 独立进程的最终运行结果

`RUNTIME_OBSERVED`：Source P正常退出后，独立Client S、T、最终U均完成14项自动用例；主智能体在U另完成画面对照，正常窗口关闭退出码为0。各自`client_before → client_after`及`client_before → client_exit`比较均通过：前者仅允许`documents/log.txt`，后者另允许本机`documents/options.ini`；PGD、Continue、官方JSON／附属文件及ModData没有被放宽。原生与Lua／Mod持久请求被前置拒绝，内存进度及RNG保持原值，关闭来源标志一直覆盖到Shutdown，Manager正常析构仍执行。

这验证了§7.4的独立Source／Client设计，§7.3的同进程L失败保留原事实。文件判定沿用用户D27决定排除每日`save_backups`，不将其称为逐字节验证通过；Shutdown备份消费者仍由相同关闭来源保护约束。临时快照和日志仅用于本轮归因，验收收束后清理，不保留artifact包。结果限定于主流程列明的代表场景与标准API，不外推第三方直接IO、强制终止或完整整局模式。
