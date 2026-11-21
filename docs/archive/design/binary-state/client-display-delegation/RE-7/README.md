# RE-7：不可算值的 J273 注入来源

日期：2026-11-12。状态：**静态来源、ABI 与注入交接完成；未实施、未测试**。范围为[方案 6.3](../../client-display-delegation.md#63-不可算值的注入方式re-7)。优先阅读本地原版／REPENTOGON Wiki、ZHL、官方 C++ 实现和已有项目证据，再补 J273 静态反编译。当前 IDB 为 v1.9.7.12.J273，imagebase=`0x400000`；下文地址为 VA，RVA＝VA−基址。

本次没有启动游戏、调用游戏函数、修改 IDB／目标内存、安装 Hook、生成布局、构建或运行测试。布局源仍为版本 71；字段收缩、getter 接线和实际安装留第 10／13 步，第 15 步唯一整体验收。旧概率实现的暂停记录不改写，本轮只完成其新方案所需的静态来源研究。

成果见[注入表与字段证据](injection-table.md)。关键结论：

1. 天使占比内部入口是 **VA `0x7EF590`／RVA `0x3EF590`**，无显式参数，依赖全局 Game，**通过 XMM0 返回 f32**。它不是 `Level:GetAngelRoomChance()` 的修正值，也不能用普通 x86 C++ float 返回 ABI 代替。
2. 药丸效果是 `ItemPool+0x7A4` 的 15 个 i32；识别是 `+0x7E0` 的 15 个 bool。官方 **已有 `UnidentifyPill` Lua 扩展**，其实现直接清合法识别字节；原方案“没有取消识别 API”已纠正。正式 C++ 安装无需调用 Lua。
3. `AddCurse／RemoveCurses` 含挑战过滤、HUD／黑暗和 Pickup 图形副作用，不能当作任意最终诅咒值的无副作用 setter。现有线格式发送 effective_curses，采用会话内有效值注入，保持库存先装；不把它误写成原始 curse 位。
4. Keeper／商店价格继续用 Host 最终 Price 与价格 Sprite。`0x80B5A0` 按全体玩家、生命、饰品和商店历史计算，不是“本机买家纯价差”；`SetPrice 0x6DF890` 还有随机／Spawn 分支，不能用于窄标量安装。
5. TMTRAINER 定义位于 `Game+0x669BC` 的 ProceduralItem* vector；负 id 对应 `~id` 下标，原生 `ItemConfig::GetCollectible 0x72A400` 返回该项 `+0` 的 ItemConfig。注册接口缺口不再误写成存储未知；显示定义／资源由本地 owner 承接，玩法 effects 留 Host。
6. 地图拾取图标不是 RoomDescriptor 的现成标量：`MinimapConfig::Render 0x9635B0` 扫描各 descriptor `SavedEntities+0x74`，步长 `0x78`，聚合 Pickup／Slot 状态。选择“**地图刷新时提供 Host 图标摘要**”，不发送整个保存世界，也不指望只装 DisplayFlags 后调用 Refresh 就能出现正确图标。具名缓存适配和挑战 32 的选中结果接线仍待实现。

上述“完成”仅指本任务的定位与交接，不代表 TMTRAINER 本地资源 owner、地图摘要适配、进度 hook 或概率注入已能运行。原生 API 的可用性不等于可在任意状态调用；具体限制与剩余实现见成果表。

> **2026-11-12 审阅后标注**（IDA J273 复核，原文保留）：
>
> - **结论 3 改为同步 raw 诅咒**：有效 getter `0x742450` 计算 `((Game+0x25AC8 | Level+0xC) | F) & ~P`，其中 F=`0x6F55C0`（种子效果 56—62 及 stage／挑战／难度条件）、P=`0x6F5750`（种子效果 63—70、79），二者只读 Seeds 对象 `Game+0x1BBD8` 与 stage／挑战／难度，这些已在 Run 同步清单中。因此 `Level+0xC` 与 `Game+0x25AC8` 按普通字段随 world 安装即可：原生 getter 在 Client 上算出与 Host 相同的有效值，直接读 raw `Level+0xC` 的消费者也一致，不需要注入或 getter hook。把有效值写进 raw 仍然拒绝；不调用 `AddCurse／RemoveCurses` 的结论不变。
> - **结论 6 的接法待定**：“Host 图标摘要”在 Client 上如何喂给原生构建尚未决定，候选为镜像最小 SavedEntities 条目（让 `0x9635B0` 照常读取），或原生构建完缓存后按摘要修补，在第 8／13 步实施前选定（方案第 5 节表后、第 13 节）。

> **2026-11-12 Q13 标注**（IDA J273 复核，原文保留）：`0x9635B0` 不是 `MinimapConfig::Render`（SDK 特征码对应 `0x4ECA30`），而是 `Minimap::Refresh 0x965D60` 对小／大 config 各调一次的 FBO 地图缓存构建（`sub_739AA0`／`sub_7415E0` 也直接调用）；图标聚合结果只是栈局部并立即绘制，没有可修补的数据缓存。它还扫描保存 Grid `+0x68/+0x6C`（步长 0x20，type 0x12 → bit14）。接法按 Q13 定为包装 `0x9635B0`、作用域内临时镜像 Host 摘要合成的最小条目，见[修改方案第 5 节表后](../../client-display-delegation.md#5-仍需同步的显示相关状态候选)。
