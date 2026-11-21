# tx-04-14 剩余随机源统一 E2E 清单

编写顺序：原生与运行库观测实现完成后编写本清单及测试夹具，再统一构建、启动实机验收。不运行单元测试。目标 REPENTOGON 1.1.3 / J273；仍属于 tx-04-14，不另设 tx、gate、哈希或独立审计。

## 范围与判据

| 项 | 实机场景与操作 | 必须核对的实际结果 |
| --- | --- | --- |
| N01 原生owner清单 | 种子局中建立真实双Player；同窗读取现有owner和四类已分配vector | Game generation/Seeds/ItemPool主与31池双RNG/37 Boss池/PlayerManager/Ambush/Backdrop均出现；Player四类容器数量与原值一致；Entity/Grid引用对应当前主体，完整words不是seed代替全部状态 |
| N02 原生xorshift入口 | 在真实进程内调用原版constructor/SetSeed/Next/RandomInt/RandomFloat，包含提前缓存的Lua方法 | 真实调用原函数一次；用记录before重算返回及after；保留移位三元组、上界、f32位；0上界行为按目标原值核对 |
| N03 对象式MTRNG | 实际调用原版Init/Next 1300次；另由真实New Run/池初始化产生消费 | 624字及index完整，至少两次twist；与全局Random分流；记录实际初始化/返回，逐条重算状态；不以同seed合并主体 |
| N04 CRT | 真实srand后连续rand；并覆盖正常玩法时经游戏/REPENTOGON IAT的调用 | PTD完整32位状态符合214013/2531011递推；返回符合高15位；srand状态正确；原始函数只调用一次；不把它当作MT |
| N05 REPENTOGON RandomFloat | 调用全局RandomFloat及加载时缓存的C函数，连续跨MT批次 | 动态Hook/状态布局成功；完整MSVC双缓冲状态与真实返回匹配，不与RNG:RandomFloat混淆，观察器不额外抽取 |
| N06 Lua math源 | seed两个整数；调用无参、整数区间、全64位random(0)、缓存函数；制造一次合法pcall捕获的空区间错误，再继续 | 完整4×u64状态；重算xoshiro推进/投影及结果；多返回/64位精度保留；原错误透明，NonLocalLuaExit如实记录且后续继续，不遗留服务scope或C++析构 |
| N07 inline/copy/地址复用 | 原生测试夹具内对已声明call-scope RNG做inline推进及16字节复制；原生constructor复用同一storage | 实际前后值变化记录为ObservedStateChange，不伪造调用参数/次数；复制体独立身份；constructor见证退休旧主体；测试局部对象返回后不继续解引用 |
| N08 实际楼层生成 | 真实seed重建及stage命令换层 | Level::generate_dungeon的栈RNG、Game generation RNG和LevelGenerator临时上下文在真实C++函数返回前捕获；实际池MTRNG消费出现；不靠Lua伪表代替 |
| N09 当前实体/格子及Loot | 真实Pickup出现，取得普通样本后调用原版GetLootList(false/true)，消耗现有Grid RNG，再Remove/重建 | 只读分支不推进owner Drop RNG；真实推进后正式状态跟进；Loot只链接已限定owner，未知非拥有指针不解引用；退休/新主体与Entity/Grid生命周期一致 |
| N10 暂停与可选录制 | 稳定模拟后暂停，在暂停中触发实际随机调用；停录后继续正式观测 | 暂停中ActualCall独立、authority_tick为空，普通scope不推进；停录后正式状态/调用继续且文件长度不变 |
| N11 关闭/重启透传 | 停C/S与观测，继续原版随机函数；再启动观测 | 关闭时无publication、输出/推进仍与原版一致；Hook常驻不恢复字节；重新观测主体/epoch不借用旧历史 |
| N12 reader/清理 | 同次实机结束后流式归约全部记录 | 无未知schema、重复scope、损坏/截断JSON、发布丢失或未声明缺失；所有必要源有实际样本；正常关闭/环境恢复/保存许可与崩溃检查通过 |

## 执行与续测

统一入口为 scripts/accept-tx-04-14.ps1 -NativeRandom。夹具只由harness发出的F8触发玩法动作；Native helper只在专用token配置下开放。完整状态的夹具JSON按4000字节分块，严格检查缺失、重复和混合分块。Rust只消费复制后的值，以流式方式读取正式记录，不持有游戏指针。

失败先修复，再在本tx整体验收中继续受影响及未完成项；已通过且不受修改影响的项复用，不重放旧StageAPI/HUD/Mod图/达摩克里斯专项。正常完成后清理测试临时材料；只在必要中断时保留恢复信息。结果在 tx-04-14-implementation.md 追加，失败事实不改写。

CRT模块加载续测（`-CrtImports`）：在ModInit时REPENTOGON尚未加载的真实时序下，Lua注册边界补接其IAT，分别核对EXE/REP/self三个模块（mask=7，rand及srand均须满足）；用官方EntityEffect.CreateLight省略Scale的8次真实C++ rand验证返回投影、完整PTD推进以及正式记录逐条存在。已通过的其余6组复用，不重复执行。
