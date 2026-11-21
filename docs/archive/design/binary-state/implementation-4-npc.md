> **版本46 Delirium未决事项（2026-11-09）**：[静态证据与归属修订](delirium-residual-evidence.md)核定两个8B Host打包存储，BDD／BE4共7B移出独立未知；BD4—BD7的4B仍无填充证明，根未知597→590B，最小线状态仍710B、17／18空预留。池槽残留已有静态可达路径但未实测；补核4A13B0的独立companion创建／BE8登记，BE8继续留Host。实际接线仍归第10／13／14步，唯一功能验收在第15步。 下文保留旧版本记录；BE8仅49F3D0访问的旧结论已纠正。

> 版本43补充：[受控UI来源已闭合](npc-control-ui-evidence.md)，模式仅0/2，不读取原版NetplayManager；上轮误解与纠正保留。

> **版本42后续复核**：[NPC同步职责复核](npc-synchronization-review.md)取代下文版本41传输决定。BossType／Variant取消8B传输与shape；ControllerId不复制本地设备键，改1B受控UI模式；+CD8 OverlayEffect仅排除绘制前完全覆写的8B Scale，使用144号场景视图。NPC／Delirium最小状态均710B，UNKNOWN和SDK边界不变；实际控制UI来源／adapter仍待第10／13步。下文保留版本41原始静态事实与编译记录。

# 第4步：Entity_NPC实现说明

日期：2026-11-09（Asia/Singapore）。布局版本41。本轮完成NPC现有结构与专用绘制输入的布局迁移，不表示NPC字节完整、第4步全部完成或同步已经可运行。静态依据和未知逐段见[npc-evidence.md](npc-evidence.md)。

`mod/binding/state/entity-classes.json`新增EntityNPC 142及Type412选择的EntityNPC_Delirium 143。两类继承公共E0—E9，增加N0六项绘制标量、N1真实EntityRef／复合引用deque、N2／N3两组完整点列及N4—N6双辅助ANM2三投影。Delirium追加8B BossType／BossVariant结构shape，旧DL1的13B AI计数留Host，块号18显式预留。公开NPC34个唯一ReplicatedState及Delirium10个唯一成员均有字段／公共派生／Host／不适用的回映射。

schema和生成器新增 `storage=deque`，只接受本轮已核20B头及4／8B元素步长。`state/nested.h/.cpp` 的 `ReadDequeInto` 在合法主线程窗口将环形deque归并到调用者预分配区；校验容器头、每个被读block、数量/地址/缓冲边界及前后来源一致性。EntityPtr原生暂存只在当前主线程使用，必须立即经身份owner转换，不交后台；线上只有u16逻辑count及投影元素。SDK独立编译对象增加NPC与Pathfinder大小及公开偏移断言。

保留State／ProjectileDelay／V1／I2／ChampionColorIdx／ControllerId，是因为实际draw消费者读取它们。冷却、Scale输入、再生、群组、命中历史、DirtColor、五个私有AI／退休关系和Delirium高频AI值留Host；+F40拥有的Siren actor保持本地，不构造第二个玩法Player。Pathfinder1872B仍是SDK声明边界，内部语义未完整核清，不因不上线就把证据提升为逐成员已核。

生成的完整单行表：[EntityNPC](../../../../mod/binding/layouts/04-entity-classes/EntityNPC.md)、[EntityNPC_Delirium](../../../../mod/binding/layouts/04-entity-classes/EntityNPC_Delirium.md)。根未知分别586／597B，其中共同Entity335B、两份ANM2各14B、NPC本类223B，Delirium另11B。NPC本类未知共15段，包含+C48第四个deque、+C80颜色段等已见访问但用途未闭合的字节；不复制、不补零、不标填充。最小状态含公共391B后为721／729B，不含外层帧和实际变长资源／点列，不作为带宽实测。

第10步须把当前窗口、具名读取处理器、资源及引用预约接入正式采集；第12／13步须按数字字段解码并经真实容器／EntityPtr／ANM2 owner安装，依赖齐备后同版本提交，继续保护906／920子体资源与835真实NPC点列。951:3的邻点索引、835固定前两引用及Deathmatch的Player隔离继续按已核draw合同检查。第14步退役旧Lua／JSON NPC链；本轮没有切换运行链或新增Hook。Text／Dummy仍待第4步后续实施，UNKNOWN仍需研究及第15步统一覆盖。

普通Release／Win32／v143产品构建完成：基础库、SDK ABI编译对象及完整 `zhlIsaacCS` 编译／链接成功，`ISAACCS_BUILD_E2E_TOOLS=OFF`。SDK既有 `CMAKE_REPENTOGON_VERSION undefined` 提示保留。本轮没有运行测试、游戏、注入或部署，没有建立artifact包；临时编辑脚本用后删除。第15步仍为唯一统一整体验收。
