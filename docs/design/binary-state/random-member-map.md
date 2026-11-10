# 第8步 Random：旧公开成员逐行回映射

2026-11-10修订2；[职责](random-synchronization-review.md)、[第7步交接](room-random-handoff.md)、[JSON](random-member-map.json)。原54行及全部来源／签名／重复ID保留，仍51条Host、3条实际显示结果交接。

| 原全局序号／field | 原ID与参数 | 旧归属 | 新决定／去向 | 原因 |
|---|---|---|---|---|
| 3395／3396 | `ItemPool.AddBibleUpgrade` ( int Add, ItemPoolType PoolType ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3396／3397 | `ItemPool.AddCollectible` ( ItemPoolType PoolType, table ／ table[] PoolItems ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3397／3398 | `ItemPool.AddRoomBlacklist` ( CollectibleType Item ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3398／3399 | `ItemPool.AddTemporaryCollectible` ( ItemPoolType PoolType, table ／ table[] PoolItems ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3399／3400 | `ItemPool.CanSpawnCollectible` ( CollectibleType Collectible, boolean ignoreLocked ) | NotReplicated | authority；HostOnly | 没有已核必要原生显示消费者；池配置/权重/数量/过滤/历史留Host或诊断，不为旧目录原值相等复制给Client |
| 3400／3401 | `ItemPool.ForceAddPillEffect` ( PillEffect ID ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3401／3402 | `ItemPool.GetBibleUpgrades` ( ItemPoolType PoolType ) | NotReplicated | authority；HostOnly | 没有已核必要原生显示消费者；池配置/权重/数量/过滤/历史留Host或诊断，不为旧目录原值相等复制给Client |
| 3402／3403 | `ItemPool.GetCard` ( int Seed, boolean Playing, boolean Rune, boolean OnlyRunes ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3403／3404 | `ItemPool.GetCardEx` ( int Seed, int SpecialChance, int RuneChance, int SuitChance, boolean AllowNonCards ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3404／3405 | `ItemPool.GetCollectible` ( ItemPoolType PoolType, boolean Decrease = false, int Seed = Random(), CollectibleType DefaultItem = CollectibleType.COLLECTIBLE_NULL ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3405／3406 | `ItemPool.GetCollectible` ( ItemPoolType PoolType, boolean Decrease = false, int Seed = Random(), CollectibleType DefaultItem = CollectibleType.COLLECTIBLE_NULL, GetCollectibleFlag Flags = 0 ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3406／3407 | `ItemPool.GetCollectible` ( ItemPoolType PoolType, boolean Decrease = false, int Seed = Random(), CollectibleType DefaultItem = CollectibleType.COLLECTIBLE_NULL, ItemPoolType BackupPoolType = ItemPoolType.POOL_NULL ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接；保留旧Rep+签名原文，但本地官方1.1.3只有flags版ABI，不声称该重载可调用 |
| 3407／3408 | `ItemPool.GetCollectibleFromList` ( CollectibleType[] ItemList, int Seed = Random(), CollectibleType DefaultItem = CollectibleType.COLLECTIBLE_BREAKFAST, boolean AddToBlacklist = true, boolean ExcludeActiveItems = false ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3408／3409 | `ItemPool.GetCollectiblesFromPool` ( ItemPoolType PoolType ) | NotReplicated | authority；HostOnly | 没有已核必要原生显示消费者；池配置/权重/数量/过滤/历史留Host或诊断，不为旧目录原值相等复制给Client |
| 3409／3410 | `ItemPool.GetLastPool` ( ) | AuthorityOnly | authority；HostOnly | 没有已核必要原生显示消费者；池配置/权重/数量/过滤/历史留Host或诊断，不为旧目录原值相等复制给Client |
| 3410／3411 | `ItemPool.GetNumAvailableTrinkets` ( ) | AuthorityOnly | authority；HostOnly | 没有已核必要原生显示消费者；池配置/权重/数量/过滤/历史留Host或诊断，不为旧目录原值相等复制给Client |
| 3411／3412 | `ItemPool.GetNumItemPools` ( ) | AuthorityOnly | authority；HostOnly | 没有已核必要原生显示消费者；池配置/权重/数量/过滤/历史留Host或诊断，不为旧目录原值相等复制给Client |
| 3412／3413 | `ItemPool.GetPill` ( int Seed ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3413／3414 | `ItemPool.GetPillColor` ( PillEffect ID ) | NotReplicated | display_result；Presentation actual body/Pocket subject/use/shape/slot result | 不发基础映射/鉴定表；真实身体及槽位的揭示、effect和名称/图像结果须有合法来源；禁止额外Getter/回调/随机补值。逆查GetPillColor自身不构成原生显示消费证据 |
| 3414／3415 | `ItemPool.GetPillEffect` ( PillColor PillColor, EntityPlayer Player = nil ) | NotReplicated | display_result；Presentation actual body/Pocket subject/use/shape/slot result | 不发基础映射/鉴定表；真实身体及槽位的揭示、effect和名称/图像结果须有合法来源；禁止额外Getter/回调/随机补值。逆查GetPillColor自身不构成原生显示消费证据 |
| 3415／3416 | `ItemPool.GetPoolForRoom` ( RoomType RoomType, int Seed ) | NotReplicated | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3416／3417 | `ItemPool.GetRemovedCollectibles` ( ) | AuthorityOnly | authority；HostOnly | 没有已核必要原生显示消费者；池配置/权重/数量/过滤/历史留Host或诊断，不为旧目录原值相等复制给Client |
| 3417／3418 | `ItemPool.GetRoomBlacklistedCollectibles` ( ) | AuthorityOnly | authority；HostOnly | 没有已核必要原生显示消费者；池配置/权重/数量/过滤/历史留Host或诊断，不为旧目录原值相等复制给Client |
| 3418／3419 | `ItemPool.GetTrinket` ( boolean DontAdvanceRNG = false ) | AuthorityOnly | authority；HostOnly | DontAdvanceRNG=true不能作为全函数纯读合同：无可用项可先重置，仍有回调；只允许真实Host调用，最终饰品由实体/库存承接 |
| 3419／3420 | `ItemPool.HasCollectible` ( CollectibleType Collectible ) | NotReplicated | authority；HostOnly | 没有已核必要原生显示消费者；池配置/权重/数量/过滤/历史留Host或诊断，不为旧目录原值相等复制给Client |
| 3420／3421 | `ItemPool.HasTrinket` ( TrinketType Trinket ) | NotReplicated | authority；HostOnly | 没有已核必要原生显示消费者；池配置/权重/数量/过滤/历史留Host或诊断，不为旧目录原值相等复制给Client |
| 3421／3422 | `ItemPool.IdentifyPill` ( PillColor PillColor ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3422／3423 | `ItemPool.IsPillIdentified` ( PillColor PillColor ) | NotReplicated | display_result；Presentation actual body/Pocket subject/use/shape/slot result | 不发基础映射/鉴定表；真实身体及槽位的揭示、effect和名称/图像结果须有合法来源；禁止额外Getter/回调/随机补值。逆查GetPillColor自身不构成原生显示消费证据 |
| 3423／3424 | `ItemPool.PickCollectible` ( ItemPoolType PoolType, boolean Decrease = false, RNG RNG = RNG(), GetCollectibleFlag Flags = 0 ) | AuthorityOnly | authority；HostOnly | 官方Lua实现推进传入RNG；未传则全局生成器提供临时seed；Decrease=false不消除选择/回调/默认求值副作用 |
| 3424／3425 | `ItemPool.RemoveCollectible` ( CollectibleType Collectible ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3425／3426 | `ItemPool.RemoveTemporaryCollectible` ( ItemPoolType PoolType, table ／ table[] PoolItems ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3426／3427 | `ItemPool.RemoveTrinket` ( TrinketType Trinket ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3427／3428 | `ItemPool.ResetCollectible` ( CollectibleType Collectible ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3428／3429 | `ItemPool.ResetRoomBlacklist` ( ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3429／3430 | `ItemPool.ResetTrinkets` ( ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3430／3431 | `ItemPool.SetLastPool` ( ItemPoolType ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3431／3432 | `ItemPool.UnidentifyPill` ( PillColor Pill ) | AuthorityOnly | authority；HostOnly | Host抽选/修改/生成/恢复；不下发操作、池或随机状态，结果由实际Entity/资源/Presentation承接 |
| 3482／3483 | `RNG.GetSeed` ( ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| 3483／3484 | `RNG.GetShiftIdx` ( ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| 3484／3485 | `RNG.Next` ( ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| 3485／3486 | `RNG.PhantomFloat` ( ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md；Phantom系列实现不同，正常返回不推进原状态不等于正式采集许可；PhantomNext还会短暂改原seed |
| 3486／3487 | `RNG.PhantomInt` ( int Max ) | NotReplicated | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md；Phantom系列实现不同，正常返回不推进原状态不等于正式采集许可；PhantomNext还会短暂改原seed |
| 3487／3488 | `RNG.PhantomNext` ( ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md；Phantom系列实现不同，正常返回不推进原状态不等于正式采集许可；PhantomNext还会短暂改原seed |
| 3488／3489 | `RNG.PhantomPrevious` ( ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md；Phantom系列实现不同，正常返回不推进原状态不等于正式采集许可；PhantomNext还会短暂改原seed |
| 3489／3490 | `RNG.PhantomVector` ( ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md；Phantom系列实现不同，正常返回不推进原状态不等于正式采集许可；PhantomNext还会短暂改原seed |
| 3490／3491 | `RNG.Previous` ( ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| 3491／3492 | `RNG.RNG` ( ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| 3492／3493 | `RNG.RNG` ( int seed = 2853650767, int shiftIdx = 35 ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| 3493／3494 | `RNG.RandomFloat` ( ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| 3494／3495 | `RNG.RandomInt` ( int Max ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| 3495／3496 | `RNG.RandomInt` ( int Min, int Max ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| 3496／3497 | `RNG.RandomVector` ( ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| 3497／3498 | `RNG.SetSeed` ( int Seed, int ShiftIdx ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |
| 3498／3499 | `RNG.SetSeed` ( int Seed, int ShiftIdx = 35 ) | AuthorityOnly | authority；HostOnly | 普通owner的16B状态和随机API归Host；GetSeed/Shift是观察信息而非网络字段。两个具名纯绘制实例发送已产生alpha／资格，seed不上线，见room-random-handoff.md |

## 原54行之外的具体补充

- `ItemPool.GetRandomPool`：旧54行未包含；官方LuaItemPool.cpp消耗传入RNG并按池权重选择，归Host
- `WeightedOutcomePicker.*`：旧随机目录之外；8个Wiki构造/成员签名另核，PickOutcome wrapper额外rng->Next，候选与权重不发
- `LootList / LootListEntry.*`：有序预览结果归Presentation/既有Pickup合同，掉落计划留Host；RNG*可空且未确立owner，绝不复制裸指针
- `::Random / ::RandomFloat / math.random / math.randomseed / CRT.rand/srand`：四类不同引擎；独立payload和线程/VM owner，不合并seed、不进入正式帧
- `Entity/Player/Grid/Level Get*RNG`：沿用各域成员身份，本账本owner视图记录；Game/Level generation实际别名不得重复建流
- `Presentation draw results`：旧13B D0 seed草稿退出；D0仅Manager相位／paused共5B，正常已产生的逐用途alpha与资格另承接，不推进Client随机流
- `SFXManager::Play private sample RNG`：BD0F40的16B私有流归Host；实际sample/voice与逻辑音频结果另承接，不调用Client随机Play
