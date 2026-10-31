# tx-04-14 整局、随机选择、表现与Mod观测

日期：2026-10-31，Asia/Singapore。状态：四域及具名StageAPI适配已实现；统一构建通过，连续修复、复用有效样本后的同tx整体E2E **6/6 PASS、21/21场景**。目标为四个正式观测域及真实StageAPI适配，不实现Replica或状态同步。

四域复用统一主线程协调器、session/epoch和提交窗口；新增run、random、presentation、mod普通scope各自有8MiB预算。独立真实调用、生命周期、过渡和Render没有借用的Authority tick。普通后态最终取消不删除已发布的真实事件。采集及最新值消费不依赖录制，生产默认不录制。Hook所有权、统一输入和原版Online/Isaac Sync隔离保持现行合同。

[逐成员目录](run-catalog.md)及JSON由两套本地Wiki生成。动作、真实消费、只读状态、显式参数查询和既有规范来源分别登记；目录不是实机场景覆盖。`Seeds.GetNextSeed`等消费方法不轮询；Level.GetAngelRoomChance独立标为modifier，Room.GetDevilRoomChance与PlanetariumChance保留公开最终值；Game帧、TimeCounter、应用帧与Authority tick不合并。

随机域以owner/用途/参数/寿命登记流，统一关联身份由原生服务分配，不使用seed或Lua地址。仅真实请求物品RNG时登记，避免初始化所有物品的内部流；固定Entity Drop、Grid、Level RNG保留当前只读状态。池列表、初始/当前权重、移除、黑名单、药丸映射/鉴定独立记录。常驻Lua wrapper保留原参数、nil洞、多返回、异常及原调用一次；观测器内部读取不会产生伪造消费。原生内部生成/AI/攻击/掉落消费绕过Lua时，明确声明来源缺口，不能由前后seed补造调用。

表现域复用Sprite资源姿态模型，读取HUD/PlayerHUD/心布局/HistoryHUD/提示和音频控制，独立Sprite通过具名reader注册。Minimap原值来源仍归exploration；Player资源引用Entity规范scope。Render与普通后态分别发布，调用参数和控制状态不冒充GPU像素/声卡采样。原生SFX post及音乐pre回调按实际语义记录，pre不宣称最终生效。

Mod适配需声明namespace、版本/schema、阶段、寿命、资源/缺口及只读reader。对象图使用next/rawget读取已有字段，有界处理共享、循环、稀疏键、非有限值、不可序列化值和截断，不遍历_G、不创建Entity数据、不主动保存。保存扩展只调用TryGet。未知userdata明确Unavailable，不能字符串化指针伪装状态。具名StageAPI 2.43直接读取LevelRooms、CustomGridEntities和CustomGrids，记录逻辑类型/TypeOverride、布局、Data/PersistentData、实际载体引用和无载体nil关系。逻辑实例以实际表对象的生命周期区分，原生主体不重复分配；实际框架spawn/unload/remove/room回调单列。

统一入口为`scripts/accept-tx-04-14.ps1`。完整实现后才构建及运行；不写单测、不提前试跑。场景顺序覆盖规则/概率/种子效果、真实随机消费及选择、池变化/黑名单/移除、药丸鉴定、HUD/音效/独立Sprite、循环/共享/稀疏图及预算/非序列化、真实StageAPI两种同原生类型逻辑房间、同BaseType不同BaseVariant网格、无BaseType实例、附加状态变化、同格移除重建、离房回访、换层、查询、退休、暂停及停录后继续正式采集。只检查新增和本次受影响部分，不重跑09—13已通过清单。测试夹具及Rust不随正式Mod发布，临时框架及部署/Mod数据使用后恢复；不留最终artifact。

## 统一验收结果

2026-10-31，官方REPENTOGON 1.1.3与游戏v1.9.7.12.J273；真实StageAPI来自[Meowlala/BOIStageAPI15](https://github.com/Meowlala/BOIStageAPI15)，实际metadata版本2.43，测试Mod namespace为`isaac.cs.e2e.tx14`、版本1。适配器直接旁听真实框架，未用同名自造表替代。

| 检查组 | 实际结果 |
| --- | --- |
| 整局规则、概率、时钟 | PASS：规则/商店计数、Angel修正与Planetarium原值、具名种子/种子效果、各时钟分列；暂停不推进普通scope |
| 随机流、真实选择及池 | PASS：真实RandomInt/Float/Vector/Next及物品/卡药/饰品选择，pool权重成员变化、黑名单/移除、药丸映射/鉴定；当前状态与真实消费分开，不额外抽取作对照 |
| HUD、独立Sprite及音频 | PASS：HUD状态/消息/心布局、独立Sprite和退休、真实提示及SFX事件、音频控制与Render来源；未将其称为声卡采样或录屏 |
| Mod图及StageAPI生命周期 | PASS：共享/循环/稀疏图、GetData既有数据和TryGet保存扩展；函数/线程/非有限值/深度预算明确Partial，恢复后Complete；两种逻辑房间、三种网格、状态变化、同格重建、离房回访、换层与退休 |
| Reader、同窗与交付 | PASS：21个动作场景，84个指定四域scope；82个Complete、2个为有意制造的图异常及随机登记预算Partial。原生载体另核对同窗Grid类型/变体与规范主体；新增房间关联核对同窗/离房Descriptor引用 |
| 暂停、录制隔离与恢复 | PASS：暂停不推进四域普通scope，停止录制后正式采集继续且epoch不变；输入/观测正常退休、回菜单/退出，部署/配置/Helper及保存范围恢复 |

最终归约读取四份来源：第三次崩溃前完整交付的选定记录前缀，第六次有效创建/修改记录，第七次移除/重建及后段记录，第九次版本/房间关联/登记预算补测。第八次也曾通过；版本读取修正影响适配分支，因此第九次只对这两项作必要续测。各来源独立检查session/epoch及scope，不跨进程拼接主体身份。第三次录制未正常关闭，末尾737字节不完整JSON行不计入交付；明确保留整份录制delivery_complete=false，只复用逐条确实存在、完整交付且已选定的scope。其他采用来源正常关闭并通过reader交付校验。最终累计95个实际presentation Render scope、219处图引用用于检查，原始记录中的Partial/Gap不改写为Complete。

逐成员目录含398条Wiki成员/签名、9份枚举、133条回调来源行，并另列8项当前绑定补充。目录数量不是398个运行场景。默认RNG只观察既有固定成员；物品RNG只在真实Getter请求时登记。原生RNG缓存不跨对象退休窗口解引用；owner键剔除解析状态等瞬时字段，seed不参与身份。在采用的正式原始样本中另核对Entity.GetData值73、TryGet保存扩展值97、稀疏键7，及同一Mod流恰为RandomInt/Float/Vector/Next四次真实消费，首前seed/首返回与末后seed/末返回匹配夹具原值。追加了马药颜色映射与有限饰品可用性读取；马药鉴定明确引用基础颜色来源。

StageAPI自定义类型为`tx14.room.A/B`，两者实际原生RoomType均为1。自定义逻辑名`tx14.rock.A/B`为不解释其语义的字符串，最终测试载体实际为Poop同一BaseType的Variant 0/1；另有`tx14.logic.only`不设置BaseType且GridEntity为nil。它们通过框架成员集合进入mod域。正式Mod的载体引用复用同窗Grid主体；当前逻辑房间直接关联同窗原生Descriptor，离房对象通过声明的LevelIndex及实际列表位置/维度复核关联。Data变化、PersistentData变化与回访重建分别读取；同格重建的旧/新实例用同一进程内真实remove事件核对身份更换，不比较不同录制的裸local_id。

## 失败与连续修复

验收前未构建试跑、未运行smoke/单测/校验器；进入统一验收后先构建。受限MSBuild曾停在启动处，已终止并用正常构建权限完成；DLL及Rust构建均通过。

1. 第一次：初始run/presentation Partial，缺Ambush/Font/ItemOverlay关联及当前HUDMessage接口，未计通过场景。补全对应公开根；当前绑定以GetMainMessage/GetStackedMessage/GetCoopPlayerMessage提供旧Wiki提示Sprite语义，并复制真实文本/显示状态。HUD Player先在本scope登记公开主体。
2. 第二次：wrapper分派对false/nil使用and/or备用分支，错误索引函数，导致Entity部分读取及Player/Presentation失败。改为显式分支，保留原nil/false、参数和返回值语义。
3. 第三次：完成15个场景采样后切房崩溃；仅前11项有效，四项StageAPI样本未满足实际载体判据，不采用。转储显示zhlREPENTOGON的RVA 0x1485D0函数在0x14869C调用链复制Game+0x25CF0的GenericPrompt/ANM2，命中未构造Lua userdata的字符串赋值路径；与Lua_GetGenericPrompt绑定`*toLua=*game->GetGenericPrompt()`一致。替换为自有主线程PromptState纯值桥接，直接读取活根公开selection/submitted/active和Sprite，不复制C++对象。新崩溃文件当时使保存范围检查失败，原FAIL保留。
4. 第四次：网格存活断言失败；修正StageAPI房间键为实际ListIndex、FromData为原生列表位置，并将生成推迟到清理后，补上续测准备后的普通窗口等待。正常退出/恢复，没有增加通过项。
5. 第五次：真实诊断显示Rock生成后的两个实际Variant为1和0，与请求0/1相反，StageAPI因BaseVariant不匹配主动移除逻辑实例。改用具有明确0/1变体的Poop，保留真实框架检查，不放宽断言。
6. 第六次：三个网格稳定存活，创建、Lua修改、逻辑移除取得样本；保留旧原生载体后直接换Variant重建失败。改为真实Remove(false)后创建新载体；采用本次创建/修改两项，逻辑移除因修改影响而续测。
7. 第七次：完成剩余7项与暂停/停录/退休/退出。归约遇到第三次崩溃录制尾行截断；只修复完整行前缀读取与离线Reduce入口，不重发游戏动作。重新归约6/6 PASS。复核又发现StageAPI原生Descriptor关系可补齐，继续同tx补测。
8. 第八次：只补测两种逻辑房间的新原生关联及回访，复用其他已通过项与第七次暂停/停录结果，当时6/6 PASS。
9. 最终静态核对补齐登记预算耗尽必须使random域Partial，并从真实StageAPI.LoadedMods/MarkLoaded读取版本，未知/未适配版本显式Unavailable。第九次只续测受版本检查影响的两个房间及登记预算新场景，其他项目全部复用。21场景/84scope归约通过后，i686 harness保存范围检查因旧实现一次载入两份包含历史转储的完整文件树而out of memory，总结果当时FAIL。改为64KiB流式逐字节对比，保留缺失、长度、大小写成员、任意字节差异、空文件和读取失败/符号链接判据，不计算摘要、不扩大允许范围；只重读已有原始录制和before/after快照、崩溃范围，最终6/6 PASS，无第十次游戏启动。

最终清理检查发现框架/夹具Mod数据实际按目录名`data/stageapi15`和`data/isaac-cs-test`保存，原备份路径曾按注册名拼写。两目录创建时间均落在第一次测试窗口，确认为本次新建；已按精确目标清理，并修正harness后续备份恢复路径。未删改测试前既有Mod数据。相关Rust构建通过，第九次已使用修正后的真实路径备份并恢复，cleanup_errors均为空。清理本线程新建数据与最终目录状态属于本次验收收尾，不重跑已通过游戏场景。

九次实机均由harness接管并分别提示；第三次原生异常结束，其余正常关闭并恢复。正常临时录制/快照/截图/源码下载和本次产生的诊断材料在完成后清理，不保留tx artifact；原有历史材料不修改。失败事实、原因与修复保留在本文。

## 实现、实际观察及公开边界

run/random/presentation/mod四域已接入正式采集、最新值消费、manifest、reader及产品Lua部署；11域均有实现。六个tx汇总：09四参与者/多身体输入、10 Entity、11 Grid、12 Room/Level、13探索/离房保存/玩家、14本四域均有各自实际验收记录；既有09—13已通过人数/输入、原生类型清单、206个特殊房间及Greed项目没有重跑。

本tx的公开状态、纯值身份/关系、scope与缺失模型可供后续同步设计参考；没有实现世界下行、Replica或可继续模拟的存档。`world_complete=false`继续保留，不能用11域已接线冒充整个引擎私有状态完整。

具体未暴露边界包括：绕过Lua的原生生成/AI/战斗/掉落随机消费及此前缓存的Lua函数；原生Loot条目RNG指针的独立owner/lifetime尚无公开合同，只保留原有UnsupportedTarget，不凭指针猜进度；Mod私有闭包/未注册对象、任意第三方即时绘制、GPU像素与声卡采样；公开保存容器不能证明逐对象SavedFrom/RestoredFrom，Hourglass私有备份容器仍Unavailable。它们明确登记，不把未知补成零或完整。StageAPI及测试Mod是本tx真实适配集合，不承诺任意第三方gameplay Mod已能在C/S或未来Replica运行。

保存/Continue没有为观测而主动触发，本次不外推该流程的完整玩法覆盖；未逐一执行全部难度/挑战/角色/全部资源/任意负载。Native内部调用全集、完整私有存档和任意未适配Mod若成为下一任务目标，需要补充对应来源；本次不以此伪造已观察事实。

登记压力场景的Mod RNG在16,385次用途登记前后seed均为1234567；正式样本明确保留Truncated及registration_failures，未将登记失败冒充完整成员集合。

## 2026-10-31 全局Random原生观测补充（同tx）

前轮通过范围不包含原版全局Random的C++调用与完整内部状态；这是实际缺口，不因原有6/6通过而改写。当前补齐公共入口与播种入口Hook、624字MT19937及索引/模式、独立原生调用和状态块、普通域完整快照、Formal与Recorder分离。全部Hook仍由binding/hooks.json统一注册，原始函数一次，关闭时透传；不调用Random来观测Random。

静态来源：目标J273的原版Random入口RVA 0x2EB4B0；种子入口0x2EB470，ECX整数参数；words 0x7E8720、index 0x7AB814、mode 0x7E72FF。达摩克里斯FamiliarVariant=202的更新helper中，call RVA 0x241A00，返回RVA 0x241A05，取模10000与激活SubType比较；这是专项真实C++消费断言。CRT模式及另一个REPENTOGON全局RandomFloat引擎与MT流分开，后者仍未接入。

验收入口：scripts/accept-tx-04-14.ps1 -GlobalRandom。夹具/归约器增加只读零消费、Lua与提前缓存函数、暂停独立scope、1300次原生调用跨多次twist、显式播种含零种子、真实使用达摩克里斯并实际受伤后的自然判定、停录Formal继续、观测关闭透传及重启、完整记录读取与交付。Rust用每次实际before状态重算返回值及after状态，夹具值亦独立核对。沿用前轮已通过且无改动影响的StageAPI/CustomGrid、HUD、Mod图等场景，不重跑其玩法清单。

本轮同tx统一E2E最终5/5 PASS。逐条重算并核对5,079次原生Random返回值及完整前后状态、2次实际播种（305419896和0）、10次twist；达摩克里斯受伤后自然更新产生45次真实判定call，来源返回RVA 0x241A05。另通过只读零消费、普通/提前缓存Lua函数、暂停中独立调用与普通scope不推进、停录后Formal继续24次、观测关闭透传24次、重启后24次、普通域完整624字状态及reader完整交付。此次通过不能外推为逐一实测全部原生调用点，也不覆盖其他RNG类消费、CRT内部状态或独立RandomFloat引擎。

连续验收共3次实机启动：第1次夹具把两份完整状态写入一行DebugString，超过游戏日志单行上限而截断；改为4000字节分块、严格检查分块完整性后继续。第2次在Join初期发送暂停键未生效；改为等待100个已完成模拟tick再暂停。诊断同时发现系统栈回溯无法越过ZHL生成的adapter，改用ZHL 1.1.3已核定的无参非void返回槽布局（+28），仅接纳游戏映像内且E8目标匹配入口的直接调用者，不扫描栈猜地址。第3次全项通过。构建期间的Lua桥函数/状态类型同名错误亦已修复。未用最终PASS抹去这些失败事实。

既有StageAPI/CustomGrid、HUD、Mod图等未受影响的已通过场景没有重复运行。游戏正常退出、环境恢复、保存字段许可及崩溃检查通过；成功轮临时材料由harness自动清理，失败轮诊断材料在本轮完成后清理，不保留artifact。

## 2026-10-31 其余原生随机源补充（同tx，当前验收）

按用户要求在全局Random之后继续补齐对象RNG、池/生成/Player私有已分配流、独立MTRNG、CRT及REPENTOGON/Lua运行库随机源。静态逆向由GPT-6 Luna（max）子智能体完成，未运行游戏/测试/构建，未改IDB。其点位与owner字段已进入代码、中央Hook清单与run-catalog.json；原版RNG.Next/RandomInt/RandomFloat分别RVA 0x3DF5B0/0x3DF4E0/0x3DF540，constructor/SetSeed为0x3DF450/0x3DF4A0；MTRNG Init/Next为0x4DA340/0x4DA390。

Player四类Getter在J273仅读已有vector，不分配/resize/seed；因此修正前轮“为避免lazy而只观察实际Getter”的保守范围，枚举当前已分配Collectible +0x1760、Trinket +0x176C、Pill +0x1778、Card +0x1784，均为三指针vector且元素16字节。Game、31个ItemPool双流、37个Boss池、Seeds、Ambush、PlayerManager及Backdrop均补入当前完整状态。Level.generate_dungeon (RVA0x33AF60) 和LevelGenerator.Generate (0x585230) 为真实生成边界，栈RNG只在原函数存活期间复制，不跨调用缓存解引用。

对象式MTRNG独立于全局Random；CRT按EXE/REPENTOGON/本DLL的实际IAT原子接入，并从已核定的实际rand正文解析PTD状态字段；全局RandomFloat读取其独立MSVC MT完整双缓冲状态；math.random读取实际C closure的xoshiro256** upvalue，包含缓存函数和Lua错误恢复。所有来源仍保持original一次、逻辑停用透传及主线程游戏对象访问。inline/copy/restore变化明确记录为ObservedStateChange，不伪造逐次调用；未知Loot非拥有指针不因可读就继续解引用。

观测实现完成后编写[统一E2E清单](tx-04-14-rng-e2e.md)和专用夹具/归约器。统一入口 scripts/accept-tx-04-14.ps1 -NativeRandom。旧已通过且不受影响的StageAPI/HUD/Mod图/达摩克里斯清单不重复执行。

本轮同tx统一验收最终6/6组PASS：精确核对23,160次实际原生调用的输出及完整前后状态，普通窗口2,315条owner流（含99个池内RNG），62次观测状态变化及27次退休。对象式MT和独立RandomFloat各实际消费1,300次跨twist；CRT实际播种及64次返回、math.random浮点/区间/64位返回、播种多返回值、Lua错误与恢复均已核对。真实seed重建、换层生成、Entity/Grid/Loot、inline/copy/constructor复用、暂停、停录后Formal继续、停用透传、重启、reader交付、环境恢复、保存许可及崩溃检查通过。普通Lua域的原生部分带独立source=1及同窗source_scope，完整words不经过Lua往返解析。

连续验收发生4次真实游戏启动。前2次在首次随机域采集后主线程阻塞，harness按超时关闭并恢复环境；诊断显示原生枚举/状态/编码约31ms完成，没有进入完整普通scope提交。原实现把2,314条原生流完整JSON交给Lua解码后再由R.Finish编码，并在主动枚举时重复触发包装Getter、注册和事件。修复为C++只编码一次并直接合并普通域payload，Lua仅取摘要/计数，同窗完整状态保留且计入同一字节预算；主动枚举使用Calls.Silent，实际玩法Getter仍正常观测。第3次原生部分约1.03MB、合并快照约1.88MB，Lua约116ms完成、随机域Complete，游戏正常推进，backpressure和publication/record loss均为0。故本次阻塞应归于主线程重复数据处理路径，不能归为磁盘录制队列已满。

第3次后续Loot夹具把LootList userdata当Lua数组取长度而失败，改为公开GetEntries()。续测启动前对已有样本归约时还发现游戏json库仅保留约14位数字，而夹具浮点容差过严；按实际传输精度修正，完整原生状态仍逐字精确核对，正式C++返回保留17位精度。该次在启动游戏前结束，不计为游戏启动。第4次复用前次未受影响的算法样本，只重建seed/实体前提后续测所有未完成项，并跨capture读取正式记录，6/6通过。临时Lua和逐窗口C++诊断输出随后移除；首窗口按diagnostics开启的原生阶段诊断保留。正常成功轮由harness自动清理，失败轮材料在本轮完成后清理，不保留artifact。

收尾静态核对还发现初始CRT接入计数只有2，而EXE/官方REP/self三者实际均导入rand和srand。旧boot记录的repentogon=unknown提示加载顺序缺口，不能以此前6组通过外推所有模块已接入。补为官方Lua注册边界重新接入尚缺的CRT模块，并按模块记录匹配；rand与srand只有三个模块均完成（mask=7）才报告installed，缺模块不再伪报Complete。已有槽保持常驻，原始target在改槽前发布，不触碰运行中的CRT正文。新增仅覆盖该改动的`-CrtImports`续测，不重放既有6组。

第5次真实游戏启动验证了ModInit时EXE/self各1槽、REP未加载（mask=5），约5秒后的Lua注册边界REP出现并接入，第0/1来源均成为3槽、mask=7。用官方EntityEffect.CreateLight省略Scale触发8次实际REP C++ rand，逐一核对32位PTD状态、15位返回投影和正式独立事件全部存在，另核对1次srand；reader交付、正常退出、环境恢复、保存许可和崩溃检查通过。此专项1/1 PASS，连同复用的6组全部必要验收完成。构建时追加harness的w名称遮蔽编译错误亦已修正。该模块加载问题与前2次主线程阻塞是不同缺陷，均如实保留。
