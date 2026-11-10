# 第8步 Random：原生字节单行账本

2026-11-10修订2；[职责](random-synchronization-review.md)；[第7步交接](room-random-handoff.md)；[机器源](random-byte-ledger.json)。设计排版，生产版本56未变，无测试。

登记24个类型／字段视图；UNKNOWN与外层ABI边界保持，旧D0草稿保留在JSON wire_history。

## RNG：`0x10`字节

RNG.zhl；J273 VA7DF4A0/7DF4E0/7DF540/7DF5B0；random-synchronization-review.md §3

实例随机游标留Host；具名纯绘制实例发送已产生结果，不能按通用RNG复制

根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32` `seed`【权威／已核】当前xorshift状态；推进覆盖本字，不是身份（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x4 `u32` `shift_1`【权威／已核】右移量；参数不是独立cursor（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x8 `u32` `shift_2`【权威／已核】左移量（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0xC `u32` `shift_3`【权威／已核】最后右移量（4B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|---|---|---|

## PickupRenderRNG：`0x10`字节

J273 VABD0CA4；6E3784/6E379A读、6E37CC写；6E3805写this+684；6E3A0A/6E3A12只初始化(4,3,27)，四字直接引用仅对应Render

原生16B物理布局不变；旧D0 seed重放草稿已由room-random-handoff.md的逐用途结果替代

根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32` `seed`【权威／已核】Host正常纯绘制流游标；当前产生的alpha由具体consumer承接（4B）；不发送seed；正常Host产生后复制具体alpha／资格，Client绕过原生随机段 | +0x4 `u32` `shift_1`【派生／已核】首次合法本地初始化后固定为4；文件初值0不作为绘制算法（4B）；不发送；原生本地初始化owner维护，正式受控分支不执行此流，不用SetSeed(index35)补状态 | +0x8 `u32` `shift_2`【派生／已核】首次合法本地初始化后固定为3；文件初值0不作为绘制算法（4B）；不发送；原生本地初始化owner维护，正式受控分支不执行此流，不用SetSeed(index35)补状态 | +0xC `u32` `shift_3`【派生／已核】首次合法本地初始化后固定为27；文件初值0不作为绘制算法（4B）；不发送；原生本地初始化owner维护，正式受控分支不执行此流，不用SetSeed(index35)补状态 |
|---|---|---|---|

## FamiliarRenderRNG：`0x10`字节

J273 VABD0674；61E533/61E549读、61E5A3写；61E604栈ColorMod.alpha；61F6A2/61F6AA只初始化(4,3,27)，四字直接引用仅对应Render

原生16B物理布局不变；旧D0 seed重放草稿已由room-random-handoff.md的逐用途结果替代

根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32` `seed`【权威／已核】Host正常纯绘制流游标；当前产生的alpha由具体consumer承接（4B）；不发送seed；正常Host产生后复制具体alpha／资格，Client绕过原生随机段 | +0x4 `u32` `shift_1`【派生／已核】首次合法本地初始化后固定为4；文件初值0不作为绘制算法（4B）；不发送；原生本地初始化owner维护，正式受控分支不执行此流，不用SetSeed(index35)补状态 | +0x8 `u32` `shift_2`【派生／已核】首次合法本地初始化后固定为3；文件初值0不作为绘制算法（4B）；不发送；原生本地初始化owner维护，正式受控分支不执行此流，不用SetSeed(index35)补状态 | +0xC `u32` `shift_3`【派生／已核】首次合法本地初始化后固定为27；文件初值0不作为绘制算法（4B）；不发送；原生本地初始化owner维护，正式受控分支不执行此流，不用SetSeed(index35)补状态 |
|---|---|---|---|

## RenderRNGInitializationGuard：`0x4`字节

两个实例分别BEE364 / BED310

只登记这4B独立变量，不把到RNG本体之间的地址空间当作对象

根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `i32` `local_static_guard`【本地／已核】C++线程安全静态初始化guard；含TLS/运行库协议（4B）；绝不复制Host guard；本地初始化owner独立维护 |
|---|

## MTRNG：`0x9C4`字节

MTRNG.zhl；J273 VA8DA340 Init / 8DA390 Next



根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32[624]` `state_words`【权威／已核】MT完整状态，不是一个seed（2496B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x9C0 `u32` `current_index`【权威／已核】624为twist边界，625为未初始化；不是网络序列（4B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|---|

## GlobalRandomMTWords：`0x9C0`字节

J273 Random VA6EB4B0 / seed入口6EB470；global_random.cpp

与index及mode不连续，不得合成跨间隙memcpy

根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32[624]` `state_words`【权威／已核】进程全局MT的624字（2496B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|

## GlobalRandomMTIndex：`0x4`字节

J273 Random VA6EB4B0 / seed入口6EB470；global_random.cpp



根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32` `current_index`【权威／已核】全局MT游标；625未初始化（4B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|

## GlobalRandomMode：`0x1`字节

J273 Random VA6EB4B0 / seed入口6EB470；global_random.cpp



根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u8` `use_crt`【权威／已核】非零使全局Random走本线程CRT rand；不能为展示切模式（1B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|

## ItemPool_Item：`0x34`字节

ItemPool_Item.zhl；ItemPoolManager.h；J273730F10/730C70逐31池



根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `f32` `total_weight`【权威／SDK声明】当前权重和（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x4 `std::vector<PoolItem>` `pool_list`【容器／SDK声明】Host有序池条目；外部元素见PoolItem（12B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x10 `RNG` `rng_1`【权威／SDK声明】第一条池流（16B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x20 `RNG` `rng_2`【权威／SDK声明】第二条池流（16B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x30 `u32` `bible_upgrade`【权威／SDK声明】Bible加入/替换生产计数（4B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|---|---|---|---|

## PoolItem：`0x18`字节

Global.zhl::PoolItem；J273730F10/730C70实际步长24



根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32` `item_id`【权威／SDK声明】候选item身份（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x4 `f32` `initial_weight`【权威／SDK声明】初始权重（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x8 `f32` `weight`【权威／SDK声明】当前权重（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0xC `f32` `decrease_by`【权威／SDK声明】抽选后权重递减量（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x10 `f32` `remove_on`【权威／SDK声明】移除阈值（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x14 `bool` `is_unlocked`【权威／SDK声明】Host候选资格；不能写访客进度（1B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x15 `bool` `is_special`【权威／SDK声明】Host特殊候选标志（1B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x16 `bytes[2]` `declared_pad`【填充／SDK声明】官方源码显式_pad[2]；仅保留声明等级（2B）；不发送、不读作状态；不升级为运行时实测 |
|---|---|---|---|---|---|---|---|

## TrinketPoolItem：`0x8`字节

ItemPool_TrinketPoolItem.zhl；J27372E260 / 730C70



根直接UNKNOWN：2B；外部／嵌套未知另计。

| +0x0 `u32` `id`【权威／已核】饰品候选ID（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x4 `bool` `in_pool`【权威／已核】当前池成员位，保存体按bit记录（1B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x5 `bool` `is_available`【权威／已核】Host资格位（1B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x6 `bytes[2]` `unknown_6_7`【未知／未知】尚未解释的原生字节；不读取、不发送，不猜填充（2B）；未知保留；未发送不等于语义已闭合 |
|---|---|---|---|

## ItemPool：`0x830`字节

ItemPool.zhl；J27372F4E0 Init / 72E260 GetTrinket / 730F10 Store / 730C70 Restore

根直接UNKNOWN259B；未知并未因不发送而消失；31池及外部元素独立登记

根直接UNKNOWN：259B；外部／嵌套未知另计。

| +0x0 `u32` `pool_seed`【权威／已核】池开局seed；Init直接写、Store/Restore保存（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x4 `ItemPool_Item[31]` `pools`【权威／已核】原生内嵌31池；不代表官方扩展总上限（1612B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x650 `i32` `last_pool`【权威／已核】上次抽选池，非必须显示状态（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x654 `RNG` `item_pool_rng`【权威／已核】全局池生成流；Init/Restore shifts=(1,9,29)（16B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x664 `RNG` `trinket_pool_rng`【权威／已核】SDK漏声明的独立饰品流；Init/Restore shifts=(1,5,19)（16B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x674 `bytes[8]` `unexplained_saved_values`【未知／未知】尚未解释的原生字节；不读取、不发送，不猜填充（8B）；未知保留；未发送不等于语义已闭合 | +0x67C `MsvcVectorBool32` `removed_collectibles`【容器／已核】Host全局移除位，外部bit存储不发（16B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x68C `MsvcVectorBool32` `room_blacklist`【容器／已核】Host房间黑名单位（16B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x69C `bytes[248]` `unknown_69c_793`【未知／未知】尚未解释的原生字节；不读取、不发送，不猜填充（248B）；未知保留；未发送不等于语义已闭合 | +0x794 `std::vector<TrinketPoolItem>` `trinket_items`【容器／已核】Host饰品池条目，实际步长8（12B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x7A0 `i32` `num_available_trinkets`【权威／已核】Host当前可用数量；GetTrinket可据此触发重置（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x7A4 `i32[15]` `pill_effects`【权威／已核】基础药丸映射；身体/挑战/回调还能改变最终显示（60B）；原表不发；具名真实Pocket显示结果另由Presentation承接 | +0x7E0 `bool[15]` `identified_pill_effects`【权威／已核】基础颜色鉴定位；不能泄露未知药丸（15B）；原表不发；实际subject/slot的最终揭示与名称图像必须保留 | +0x7EF `bytes[1]` `unknown_7ef`【未知／未知】尚未解释的原生字节；不读取、不发送，不猜填充（1B）；未知保留；未发送不等于语义已闭合 | +0x7F0 `u16[31]` `remaining_genesis_items`【权威／已核】Host Genesis生产/保存额度（62B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x82E `bytes[2]` `unknown_82e_82f`【未知／未知】尚未解释的原生字节；不读取、不发送，不猜填充（2B）；未知保留；未发送不等于语义已闭合 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## BossPool_Entry：`0x14`字节

BossPool.zhl / BossPool_Pool.zhl / BossPool_Entry.zhl



根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `i32` `id`【权威／SDK声明】候选Boss（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x4 `f32` `initial_weight`【权威／SDK声明】初始权重（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x8 `f32` `weight`【权威／SDK声明】当前权重（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0xC `i32` `achievement_id`【权威／SDK声明】Host解锁过滤输入（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x10 `i32` `room_variant_start`【权威／SDK声明】候选房间变体范围起点（4B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|---|---|---|---|

## BossPool_Pool：`0x3C`字节

BossPool.zhl / BossPool_Pool.zhl / BossPool_Entry.zhl



根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `std::string` `name`【权威／SDK声明】池配置名；字节/字符均不上线（24B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x18 `std::vector<BossPool_Entry>` `entries`【容器／SDK声明】Host候选列表与顺序（12B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x24 `f32` `total_weight`【权威／SDK声明】候选权重和（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x28 `RNG` `rng`【权威／SDK声明】该池独立抽选流（16B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x38 `i32` `double_trouble_variant_start`【权威／SDK声明】双Boss房间选择起点（4B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|---|---|---|---|

## BossPool：`0x8CC`字节

BossPool.zhl / BossPool_Pool.zhl / BossPool_Entry.zhl



根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `BossPool_Pool[37]` `pools`【权威／SDK声明】37个原生内嵌池；生成结果另域承接（2220B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x8AC `MsvcVectorBool32` `removed_bosses`【容器／SDK声明】Host移除位（16B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x8BC `MsvcVectorBool32` `level_blacklist`【容器／SDK声明】Host楼层黑名单位（16B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|---|---|

## GameStateItemPool：`0x2B4`字节

GameStateItemPool.zhl与J273730F10/730C70；+218声明错误，以指令逐bit存取为准

直接UNKNOWN11B；保存体不进入正式网络Baseline

根直接UNKNOWN：11B；外部／嵌套未知另计。

| +0x0 `std::vector<f32>[31]` `pool_item_weights`【容器／已核】31池当前权重保存，各项外部f32；不含展示负载（372B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x174 `u32[31]` `bible_upgrades`【权威／已核】保存Bible计数（124B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x1F0 `u32` `pool_seed`【权威／已核】Host保存/恢复值，不是Client baseline（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x1F4 `i32` `last_pool`【权威／已核】Host保存/恢复值，不是Client baseline（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x1F8 `u32` `item_pool_rng_seed`【权威／已核】Host保存/恢复值，不是Client baseline（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x1FC `u32` `trinket_pool_rng_seed`【权威／已核】Host保存/恢复值，不是Client baseline（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x200 `bytes[8]` `unexplained_saved_values`【未知／未知】尚未解释的原生字节；不读取、不发送，不猜填充（8B）；未知保留；未发送不等于语义已闭合 | +0x208 `MsvcVectorBool32` `removed_collectibles`【容器／已核】保存的移除位（16B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x218 `MsvcVectorBool32` `trinket_in_pool_bits`【容器／已核】实际保存每饰品inPool位；不是vector<TrinketPoolItem>（16B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x228 `i32[15]` `pill_effects`【权威／已核】Host基础效果保存（60B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x264 `bool[15]` `identified_pill_effects`【权威／已核】Host鉴定位保存（15B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x273 `bytes[1]` `unknown_273`【未知／未知】尚未解释的原生字节；不读取、不发送，不猜填充（1B）；未知保留；未发送不等于语义已闭合 | +0x274 `u16[31]` `remaining_genesis_items`【权威／已核】Host Genesis额度保存（62B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x2B2 `bytes[2]` `unknown_2b2_2b3`【未知／未知】尚未解释的原生字节；不读取、不发送，不猜填充（2B）；未知保留；未发送不等于语义已闭合 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

## WeightedOutcomePicker：`0xC`字节

WeightedOutcomePicker.zhl / WeightedOutcomePicker_Outcome.zhl；LuaWeightedOutcomePicker.cpp



根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `std::vector<WeightedOutcomePicker_Outcome>` `outcomes`【容器／SDK声明】Host候选列表；Lua选择后明确推进传入RNG（12B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|

## WeightedOutcomePicker_Outcome：`0x8`字节

WeightedOutcomePicker.zhl / WeightedOutcomePicker_Outcome.zhl；LuaWeightedOutcomePicker.cpp



根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32` `value`【权威／SDK声明】候选结果（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x4 `u32` `weight`【权威／SDK声明】整数权重；AddOutcomeFloat会缩放转换（4B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|---|

## LootListEntry：`0x14`字节

LootListEntry.zhl；LuaLootList.cpp / LuaLootListEntry.cpp

Loot deque/closure外壳不由本结构推导；预览与生成数据不可混为一谈

根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32` `type`【权威／SDK声明】预生成实体类型；不是已创建实体身份（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x4 `u32` `variant`【权威／SDK声明】预生成变体（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x8 `u32` `subtype`【权威／SDK声明】预生成子类型（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0xC `u32` `seed`【权威／SDK声明】Host预生成seed；不在Client重新spawn（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x10 `RNG*` `rng`【本地／SDK声明】可空非拥有指针；官方Getter不确立owner/lifetime（4B）；不发送、不为预览解引用未知指针；实际有序预览结果必须另同步 |
|---|---|---|---|---|

## MsvcVectorBool32：`0x10`字节

J273730F10/730C70对+0及+C读写；官方vector_bool；Win32发布ABI模型

不将此诊断ABI推广到任意STL编译版本；本次无reader或writer

根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `std::vector<u32>` `word_storage`【容器／模型】Host bit存储头；指向外部bit words（12B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0xC `u32` `bit_count`【权威／已核】有效bit数量，原生保存/恢复使用（4B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|---|

## RepentogonMsvcMTModel：`0x1388`字节

random_runtime_lua.cpp::FindMsvcEngine/ReadRandomWords；LuaInit.cpp::gen；旧诊断实现，非本轮DLL ABI核定

动态来源；不写死官方DLL偏移，不升级为本轮实测

根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32` `index`【权威／模型】现有诊断模型的双缓冲索引（4B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x4 `u32[1248]` `words`【权威／模型】MSVC引擎双缓冲；不是原生MTRNG（4992B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x1384 `u32` `mask`【权威／模型】现有模型尾掩码0xFFFFFFFF（4B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|---|---|

## LuaMathRandomPayload：`0x20`字节

REPENTOGON libs/lua/lmathlib.c::RanState / math_random；random_runtime_lua.cpp实际closure payload reader

只登记32B payload；GC/Userdata头、closure、registry均不在该范围，不冒充完整Lua对象

根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u64` `s0`【权威／SDK声明】xoshiro256**内部状态字；closure实际upvalue所有（8B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x8 `u64` `s1`【权威／SDK声明】xoshiro256**内部状态字；closure实际upvalue所有（8B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x10 `u64` `s2`【权威／SDK声明】xoshiro256**内部状态字；closure实际upvalue所有（8B）；留Host；原值、存储地址与控制历史不进入状态帧 | +0x18 `u64` `s3`【权威／SDK声明】xoshiro256**内部状态字；closure实际upvalue所有（8B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|---|---|---|

## CrtRandStateWord：`0x4`字节

random_runtime_lua.cpp::PrepareRuntimeRandomHook/CaptureRuntimeRandomWords；CRT IAT与PTD诊断reader

仅字段视图；不假设PTD总大小/固定偏移或跨线程共享

根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32` `thread_state`【权威／模型】CRT线程状态，PTD字段由当前实际正文解析（4B）；留Host；原值、存储地址与控制历史不进入状态帧 |
|---|

## SFXSampleSelectionRNG：`0x10`字节

907C60正常Play；907DA7写seed；907DBB按已选56B sample；shift初始(5,27,8)，guard C01050本地

音效随机选sample的私有16B字段视图，不是SFXManager完整ABI，不是第三个D0流

根直接UNKNOWN：0B；外部／嵌套未知另计。

| +0x0 `u32` `seed`【权威／已核】Host选sample当前游标（4B）；不上线；实际已选sample与逻辑voice另承接，不执行Client随机Play | +0x4 `u32` `shift_1`【权威／已核】正常初始化写入5的shift参数（4B）；不上线；实际已选sample与逻辑voice另承接，不执行Client随机Play | +0x8 `u32` `shift_2`【权威／已核】正常初始化写入27的shift参数（4B）；不上线；实际已选sample与逻辑voice另承接，不执行Client随机Play | +0xC `u32` `shift_3`【权威／已核】正常初始化写入8的shift参数（4B）；不上线；实际已选sample与逻辑voice另承接，不执行Client随机Play |
|---|---|---|---|

## 具名owner的随机字段视图

只定位已核随机字段，不代表整块对象或所有私有动态ABI已解释。

| owner | 范围／大小 | 寿命 | 职责／证据 |
|---|---|---|---|
| Game | 0x182D4／0x10 | run | HostOnly；generation_rng；官方Game.zhl；LuaLevel.cpp返回同一成员 |
| Seeds | 0x8／0x10 | run | HostOnly；generation_rng；Seeds.zhl；RunSeeds已核 |
| PlayerManager | 0xB8／0x10 | run | HostOnly；internal_rng；PlayerManager.zhl |
| Ambush | 0x2C／0x10 | run | HostOnly；selection_rng；run.json::RunAmbush；827EB0 |
| Ambush | 0x3C／0x10 | run | HostOnly；spawn_rng；Ambush.zhl；827EB0 |
| ItemPool | 0x654／0x10 | run | HostOnly；global_rng；本账本；J27372F4E0/730C70 |
| ItemPool | 0x664／0x10 | run | HostOnly；trinket_rng；本账本；J27372E260 |
| ItemPool._pools[0..30] | 0x10 / 0x20／0x10 each | run | HostOnly；two_pool_streams；ItemPool_Item.zhl；父数组+4、步长34h |
| BossPool._pool[0..36] | 0x28／0x10 | run | HostOnly；boss_pool_rng；BossPool_Pool.zhl；步长3Ch |
| Backdrop | 0x4F28／0x10 | room | HostOnly；backdrop_rng；Backdrop.zhl；实际图像/纹理结果交第7步，非D0例外 |
| Backdrop | 0x4F38／0x4 | room | HostOnly；generation_seed；Backdrop.zhl；纹理生成来源，不按seed生成Client背景 |
| Entity | 0x3DC／0x10 | subject/use | HostOnly；drop_rng；entity.json；Entity.zhl |
| EntityPlayer | 0x1760 / 0x176C / 0x1778 / 0x1784／0xC each + existing 0x10 elements | body/use | HostOnly；collectible/trinket/pill/card_rng；player-byte-ledger；既有已分配vector，不能为采集resize |
| GridEntity | 0x2C／0x10 | grid/use | HostOnly；grid_rng；grid.json；对象流不等于Light临时绘制输入 |
| GridEntityPressurePlate | 0x26C／0x10 | grid/use | HostOnly；greed_mode_rng；grid-classes.json::GridEntityPressurePlate；grid-pressure-door-evidence.md；J27395E8DB—95E8EE |
| LevelGenerator | 0x0／0x10 | native-call | HostOnly；temporary_generation_rng；LevelGenerator.zhl；只在owner实际存活期间观察 |
| Level.GetGenerationRNG | alias Game+0x182D4／0x10 | run | HostOnly；alias_not_new_owner；LuaLevel.cpp::Lua_LevelGetGenerationRNG |
| Level.GetDevilAngelRoomRNG | Game/Level+0x182E4／0x10 | floor/use within Game | HostOnly；devil_angel_rng；J273853542/878DB8绑定74D6F0返回this+182E4；743970推进四字 |
| EntitySaveState / Player保存体 | 各自已核seed字段／字段视图 | Host-save | HostOnly；saved_random_state；entity/player字节账本；保存随机状态不等于Client展示输入 |
| RNG stack / Lua userdata | owner-qualified field only／0x10 payload | native-call/Lua-owner | HostOnly；temporary_or_mod_stream；不能把地址/seed当身份；未知Mod由声明adapter负责 |
| SFXManager::Play private static | VA0xBD0F40／0x10 | process; Host normal Play | HostOnly；sample_selection_rng；907C60正常Play；907DA7写seed；907DBB按已选56B sample；shift初始(5,27,8)，guard C01050本地 |

## 未核完整ABI的外层owner

- REPENTOGON ItemPoolManager / ItemPool / CustomItemPool / PoolSaveState：Host动态池/独立m_RNG/VirtualItems/三保存槽/Genesis额度/磁盘扩展；完整Win32编译器ABI、边界与成员偏移本轮未核定；不伪造连续字节表；来源：repentogon/Patches/ItemPoolManager.h
- Lua closure / CRT PTD / std::random_device / runtime wrapper：进程/线程/VM本地owner；账本只包含已声明payload/字段视图，未覆盖整个运行库对象；来源：random_runtime_lua.cpp；LuaInit.cpp；libs/lua/lmathlib.c

## 状态二进制草稿

| +0 u32 manager_frame（4B） | +4 u8 paused（1B） |
|---|---|

D0现5B；两个绘制seed不上线，逐用途随机结果为1B资格＋有值时4B alpha。外层subject/use/shape/pass/occurrence与批次头另计，不能将5B误作完整实体包。D1 Error颜色谓词1B；若最终颜色已承接则不重复发。详见[正式交接](room-random-handoff.md)。
