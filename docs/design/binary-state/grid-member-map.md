# 第6步：Grid全部119行公开成员回映射

日期：2026-11-10（Asia/Singapore），布局版本 **55**。目标为官方REPENTOGON 1.1.3／Repentance+ v1.9.7.12.J273／Win32。第6步的公共前缀、17派生、Plus来源ABI及公开目录静态工作现均已登记；UNKNOWN照实保留，实际reader／writer与绘制adapter仍归第10／13步，测试只在第15步。

## 范围与正式入口

**119行全部进入正式生成源**：[`grid.json`](../../../mod/binding/state/grid.json)与[`grid-classes.json`](../../../mod/binding/state/grid-classes.json)的`catalog_rows`。逐行处置、原签名、真实布局、字段、原生偏移／函数依据，见同源生成的[119行汇总表](layouts/grid-catalog-members.md)。不另维护一份设计JSON或人工字段镜像。

正式输入是[原Grid公开目录](../observation-expansion/grid-catalog.json)的全部119行；它也是[旧应用目录](../tx-04-18/room-grid-application.json)开头`source_schema=isaac.cs.grid`的119行来源。本文／生成表的一基行号N对应旧应用成员数组的一基行号N及其零基`source_ordinal=N-1`。旧应用目录后续Room／资源等655行不在本次范围；不改写其实施状态或历史事实，也不把它们纳入Grid的完成声明。

行身份使用**来源目录路径＋一基行号**，保留ID、owner、原始parameters和returns；签名中的null、空白、默认参数保持原值。`owner`沿用Grid公开目录声明，不从新布局的实际存放位置反推公开owner：例如ToDoor仍属于GridEntity，但其正式类关系存放在Door228。

| 原公开owner | 行数 | 正式布局归属 |
|---|---|---|
| GridEntity | 45 | 公共199的28行；17个To接口分别登记在真实派生布局 |
| GridEntityDesc | 8 | 198；224的SpawnSeed排除规则在同一来源行注明 |
| GridEntityDoor | 32 | 228；主Sprite226、extra201、描述224按真实nested_layout处理 |
| GridEntityLock | 1 | 217 |
| GridEntityPit | 4 | 218 |
| GridEntityPoop | 6 | 222，包含1行不适用 |
| GridEntityPressurePlate | 4 | 227 |
| GridEntityRock | 17 | 223；主Sprite215／图层214条件视图 |
| GridEntitySpikes／GridEntityTNT | 各1 | 219／221 |

最终处置为**field 16行、derived 28行、authority 74行、not_applicable 1行**。这是119个公开目录行的分工，不是119份线字段或完整API兼容承诺；一个字段可被多个公开别名引用，一个API也可能只保留实际显示投影。

## 必须保留的逐行区别

- **第2／8行GetSaveState**：公开名分别为Desc属性／GetSaveState方法，parameters分别为null／一个空格。J273 `74D6A0`返回`this+4`，两者同一32B描述；完整八项来源API留Host，实际显示子集走198／224，不把属性和方法合并成一行，也不恢复完整描述复制。
- **第61／63行ExtraSprite**：属性与GetExtraSprite方法共用ID。官方`LuaGridEntityDoor.cpp::Lua_GridEntityDoor_GetExtraSprite`返回真实`+168`内嵌对象；属性值副本不能充当真实owner姿态。两行都映射同一extra三块，不增加第二份Sprite或第二个身份。
- **第105／110行RubbleAnim**：GetRubbleAnim方法与属性共用ID；`8525B2`注册getter `5CCDD0`，`85257E`注册`+16C`属性。两行映射223的`rubble_animation`资源u16；只在state2且资源可绘时读取／换算，其它状态不读旧string，不要求旧字符串全值相等。
- **第92行ReducedSpawnRate**：本地原版Wiki明确该属性已在Repentance移除，J273 `852478—8524D9`只注册ReduceSpawnRate等现有接口。该行保留为不适用；方法第91行`452660`写的是Desc.VarData=1，不把`+171`或UNKNOWN `+172..173`重命名成旧属性。
- **第104行GetBigRockFrame**：原始指令`715683 cmp eax,3E8h`、`715688 jnb`按u32比较Variant位型；小于1000返回-1，否则32位减1000。负i32位型也会走后一个分支，不能简写成有符号`variant>=1000`。本轮补清既有members说明，不改变原生字段／线格式。
- **第67／68行IsLocked／IsOpen**：`431680`比较variant=1/2/3/4/5，`431670`比较state=2，可由已发送Desc派生。第66行钥匙跟班目标、第69行带参数IsRoomType、第70行IsTargetRoomArcade仍属Host查询；后者`70C2D0`还涉及玩家380道具与整局规则，不能都按门的目标房型简单派生。
- **第12／22／25行水面接口**：只发送每实例最终完整u32，Set／Reset留Host；有效覆盖0区别于未设置，Lock忽略官方Plus。当前六个C/S wrapper漏Lock，仍待第13步补第七点，不能用回映射完成声称已接通。

这里的地址均为**VA**，IDA imagebase为`0x400000`，RVA减该基址。补读了J273 `852050—8526D4`注册区和上述小函数的原始指令／反编译，没有修改IDB。原生描述、尾部及实际显示消费者的详细证据沿用[公共](grid-common-evidence.md)、[职责复核](grid-common-synchronization-review.md)、[八类](grid-eight-subclasses-evidence.md)、[七类](grid-seven-subclasses-evidence.md)、[PressurePlate／Door](grid-pressure-door-evidence.md)与[Plus来源](grid-plus-water-source-evidence.md)；每一生成行都带具体职责及证据定位。

## 专用布局及运行边界

公开继承接口不能反向恢复被专用视图排除的字节。第9行GetSprite按实际RTTI的nested_layout消费：Statue216无Sprite负载；五类主色从同world地板色派生；Rock215／214对state2主姿态和layer0位置条件派生，真实overlay／其它层保留；Door226只保留主alpha入口，extra仍保留自身完整颜色。第48行SpawnSeed对通用198为字段，对PressurePlate／Door224留Host，不能据通用公开行恢复这两类Seed传输。

TimerPlate第100行从真实`+154`内嵌ANM2读取，不以variant强制判空；其计时文字由同world Room计时／Game展示帧与震屏上下文承接。Door第82行目标索引保留有符号特殊值，解析依赖Level描述；第80行Slot原值留Host，Room八槽真实关系／door_aliases仍由第7步负责。

authority行保留原生字节或函数归属，但没有线字段依赖，不因旧ReplicatedState标签恢复传输。Destroy、Hurt、Init、PostInit、Update、SetType／Variant、门开闭／解锁／PlayAnimation、桥生产、奖励／刷波、Rock掉落／统计／声音等都不能当作Client安装RPC。真实产生的Grid／Entity成员和Presentation音画按各自owner发布；一次性声音不因状态重发再次播放。第21行公开Render调用留Host，第109行RenderTop仅表示已登记的Pillar展示派生职责；二者均不授权直接重放原生draw，实际纯绘制、pass来源、selector／颜色借用恢复仍待后续实现。

## 生成方式与失败方式

`version.json.catalogs`登记119行原目录；新的根级`catalog_complete`表示一个完整目录可分布在多个真实布局。生成器按来源行聚合，拒绝缺行或重复行，沿用原签名和正式字段／上下文引用检查；原有类型级`catalog_complete`行为保持。构建同时产出`state_members.inc`中的119条CatalogRow及按原顺序排列的汇总表，各类型页也包含自己的行。CMake增加原Grid目录依赖，不把Room／资源目录变成当前工作的前置条件。

本次需要防止：按ID合并三个重复项；将null与空白签名归一；把To接口当对象字段或裸指针发送；用公共201覆盖实际派生视图；用224恢复无消费者Seed；把已移除属性写进未知区；将Host命令／查询／历史缓存误当字段；将119行完成外推成774行应用目录、运行reader／writer或第15步验收完成。

本轮只进行资料分析、可逆源码编辑、同源表生成及普通Release／Win32状态库纯编译；未编写单元测试，未运行游戏、部署、注入、校验脚本、probe、smoke或E2E。生成及编译结果见第6步当前记录。既有所有UNKNOWN、Rock多pass来源、Plus实际绑定／最终水面reader、Lock第七覆盖点、Fire／Rock容器owner、Door目标解析及draw恢复仍按原职责保留，不以目录完成宣称功能已验收。
