# 第7步 Exploration：原始成员逐行回映射

2026-11-10；全部99条原始`source_schema=isaac.cs.exploration`记录。职责／API差异／生命周期与证据见[核对说明](room-save-exploration-synchronization-review.md)，完整来源与实际API类型见[机器表](exploration-member-map.json)。

source_ordinal为stream-catalog的0-based全局行号；原continuity行号单列。参数按原JSON显示，保留null／空字符串／空格，重载不合并；旧归属保留用于说明修订。未新增线字段、生产owner或运行接线，版本仍56。

| 全局行／原行／field | 原成员与参数 | 原类型／旧归属 | 当前决定／去向 | 理由 |
|---|---|---|---|---|
| 643 / 643 / 644 | Level.AddAngelRoomChance params=" float Chance " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 644 / 644 / 645 | Level.AddCurse params=" LevelCurse Curse, boolean ShowName " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 645 / 645 / 646 | Level.ApplyBlueMapEffect params=" " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 646 / 646 / 647 | Level.ApplyCompassEffect params=" boolean Persistent " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 647 / 647 / 648 | Level.ApplyMapEffect params=" " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 648 / 648 / 649 | Level.CanOpenChallengeRoom params=" int RoomIndex " | boolean / NotReplicated | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 649 / 649 / 650 | Level.CanPlaceRoom params=" RoomConfigRoom RoomConfigToPlace, int GridIndex, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false, boolean AllowNoNeighbors = false " | boolean / NotReplicated | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 650 / 650 / 651 | Level.CanPlaceRoom params=" RoomShape RoomShape, DoorMask DoorMask, int GridIndex, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false, boolean AllowNoNeighbors = false " | boolean / NotReplicated | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 651 / 651 / 652 | Level.CanPlaceRoomAtDoor params=" RoomConfigRoom RoomConfigToPlace, RoomDescriptor NeighborRoomDescriptor, DoorSlot DoorSlot, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | boolean / NotReplicated | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 652 / 652 / 653 | Level.CanPlaceRoomAtDoor params=" RoomShape RoomShape, DoorMask DoorMask, RoomDescriptor NeighborRoomDescriptor, DoorSlot DoorSlot, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | boolean / NotReplicated | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 653 / 653 / 654 | Level.CanSpawnDevilRoom params=" " | boolean / ReplicatedState | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 654 / 654 / 655 | Level.CanSpawnDoorOutline params=" int RoomIdx, DoorSlot DoorSlot " | boolean / NotReplicated | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 655 / 655 / 656 | Level.CanStageHaveCurseOfLabyrinth params=" LevelStage Stage " | boolean / NotReplicated | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 656 / 656 / 657 | Level.ChangeRoom params=" int RoomIndex, int Dimension = -1 " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 657 / 657 / 658 | Level.DisableDevilRoom params=" " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 658 / 658 / 659 | Level.DungeonReturnPosition params=null | Vector / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 659 / 659 / 660 | Level.DungeonReturnRoomIndex params=null | int / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 660 / 660 / 661 | Level.EnterDoor params=null | int / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 661 / 661 / 662 | Level.FindValidRoomPlacementLocations params=" RoomConfigRoom RoomConfig, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | int[] / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 662 / 662 / 663 | Level.FindValidRoomPlacementLocations params=" RoomShape RoomShape = 1, DoorMask DoorMask = -1, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | int[] / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 663 / 663 / 664 | Level.ForceHorsemanBoss params=" int Seed " | boolean / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 664 / 664 / 665 | Level.GetAbsoluteStage params=" " | LevelStage / ReplicatedState | derived → @L0.stage/stage_type + @Run.difficulty + @Presentation | 纯元数据/显示语义，不增设原始Getter字段；完整GetAbsoluteStage公式未在本轮宣称实现 |
| 665 / 665 / 666 | Level.GetAngelRoomChance params=" " | float / ReplicatedState | delegated → @Presentation.StatHUD.final_values | 本API angel chance是182F4权重，不等于最终HUD概率；只同步正常Host显示缓存，不重算 |
| 666 / 666 / 667 | Level.GetCanSeeEverything params=" " | boolean / ReplicatedState | authority → @HostVisibility -> L1.display_flags/L2 | 可见性生产输入；最终DisplayFlags、有效诅咒和地图结果承接，不复制Host查看/更新状态 |
| 667 / 667 / 668 | Level.GetCurrentRoom params=" " | Room / ReplicatedState | reference → @CurrentWorld.Room | 已有当前Room owner与结构身份；不复制地址，也不运行初始化 |
| 668 / 668 / 669 | Level.GetCurrentRoomDesc params=" " | RoomDescriptor / ReplicatedState | structural → L1/L3 + @OrderedOrdinaryDescriptorMembers | 活动有序成员和物理源槽/三维映射承接；不调用73AD10的fallback初始化或随机/递归路由补采 |
| 669 / 669 / 670 | Level.GetCurrentRoomIndex params=" " | int / ReplicatedState | field → L0.current_entry_index | 保留入口象限／负特殊房间，不用GridIndex代替 |
| 670 / 670 / 671 | Level.GetCurseName params=" " | string / ReplicatedState | delegated → @Presentation.LevelName/CurseText | 保留实际已选文本/资源；challenge44逐房主题和官方CustomStageName不能仅由L0复算；实际显示批次待接线 |
| 671 / 671 / 672 | Level.GetCurses params=" " | LevelCurse / ReplicatedState | converted → L0.effective_curses | 742450的最终组合；原始+C、temporary和seed强制/阻止均不独立下发 |
| 672 / 672 / 673 | Level.GetDevilAngelRoomRNG params=" " | RNG / AuthorityOnly | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 673 / 673 / 674 | Level.GetDimension params=" " | Dimension / ReplicatedState | converted → L0.dimension | 原生i32核0..2后编码u8；不发送默认-1 |
| 674 / 674 / 675 | Level.GetDungeonPlacementSeed params=" " | int / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 675 / 675 / 676 | Level.GetEnterPosition params=" " | Vector / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 676 / 676 / 677 | Level.GetForceSpecialQuest params=" " | SpecialQuest / ReplicatedState | authority → @LevelASM.ForceSpecialQuest | 对象外生成参数，非Game偏移；结果由实际拓扑/门/显示承接 |
| 677 / 677 / 678 | Level.GetGenerationRNG params=" " | RNG / AuthorityOnly | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 678 / 678 / 679 | Level.GetGreedWavesClearedWithoutRedHeartDamage params=" " | int / ReplicatedState | sdk_conflict → @SDK18204 != @Native18374 (HostOnly) | 官方LuaLevel.cpp错误读取dimension2/grid120映射；真实计数由7455D7/80BEAB核到18374，二者均不作为Client状态 |
| 679 / 679 / 680 | Level.GetHeartPicked params=" " | boolean / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 680 / 680 / 681 | Level.GetLastBossRoomListIndex params=" " | int / ReplicatedState | converted → L0.current_is_last_boss | 原始索引留Host；L0匹配bool在所有模式保留：97D0EA Greed HUD及803313普通TrapDoor Light都需要 |
| 681 / 681 / 682 | Level.GetLastRoomDesc params=" " | RoomDescriptor / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 682 / 682 / 683 | Level.GetMyosotisPickups params=" " | EntitiesSaveStateVector / ReplicatedState | authority → @Host.MyosotisQueue + @Entity.CreatedPickup | 跨层保存/生成留Host，实际创建的Pickup独立同步，不在Client Restore |
| 683 / 683 / 684 | Level.GetName params=" " | string / ReplicatedState | delegated → @Presentation.LevelName/CurseText | 保留实际已选文本/资源；challenge44逐房主题和官方CustomStageName不能仅由L0复算；实际显示批次待接线 |
| 684 / 684 / 685 | Level.GetNeighboringRooms params=" int GridIndex, RoomShape RoomShape, Dimension Dimension = -1 " | table / ReplicatedState | derived → @DescriptorTopology | 从同world已安装配置形状、三维占用和源槽关系作具名纯结构查询；不是重跑放房/连门 |
| 685 / 685 / 686 | Level.GetNonCompleteRoomIndex params=" " | int / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 686 / 686 / 687 | Level.GetPlanetariumChance params=" " | float / ReplicatedState | delegated → @Presentation.StatHUD.final_values | 本API angel chance是182F4权重，不等于最终HUD概率；只同步正常Host显示缓存，不重算 |
| 687 / 687 / 688 | Level.GetPreviousRoomIndex params=" " | int / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 688 / 688 / 689 | Level.GetRandomRoomIndex params=" boolean IAmErrorRoom, int Seed " | int / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 689 / 689 / 690 | Level.GetRoomByIdx params=" int RoomIdx, int Dimension = -1 " | RoomDescriptor / ReplicatedState | structural → L1/L3 + @OrderedOrdinaryDescriptorMembers | 活动有序成员和物理源槽/三维映射承接；不调用73AD10的fallback初始化或随机/递归路由补采 |
| 690 / 690 / 691 | Level.GetRoomCount params=" " | int / ReplicatedState | structural → L1/L3 + @OrderedOrdinaryDescriptorMembers | 活动有序成员和物理源槽/三维映射承接；不调用73AD10的fallback初始化或随机/递归路由补采 |
| 691 / 691 / 692 | Level.GetRooms params=" " | RoomDescriptor List / ReplicatedState | structural → L1/L3 + @OrderedOrdinaryDescriptorMembers | 活动有序成员和物理源槽/三维映射承接；不调用73AD10的fallback初始化或随机/递归路由补采 |
| 692 / 692 / 693 | Level.GetStage params=" " | LevelStage / ReplicatedState | field → L0.stage | 多个原生Room/HUD/地图消费者需要当前stage，不能仅传楼层名称 |
| 693 / 693 / 694 | Level.GetStageType params=" " | StageType / ReplicatedState | field → L0.stage_type | 当前全局stageType参与镜面与显示；挑战44逐房主题另由资源结果承接 |
| 694 / 694 / 695 | Level.GetStartingRoomIndex params=" " | int / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 695 / 695 / 696 | Level.GetStateFlag params=" LevelStateFlag LevelStateFlag" | boolean / NotReplicated | authority → @HostLevelFlags -> @FinalWorldResults | 24个flags原值均不作为通用查询字典下发；最终地图/门/实体/音画仍必需 |
| 696 / 696 / 697 | Level.GreedModeWave params=null | int / ReplicatedState | field → L0.greed_wave | 97D371显示%d/%d；只读真实Greed当前wave，非Greed规范0 |
| 697 / 697 / 698 | Level.HasAbandonedMineshaft params=" " | boolean / ReplicatedState | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 698 / 698 / 699 | Level.HasBossChallenge params=" " | boolean / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 699 / 699 / 700 | Level.HasMirrorDimension params=" " | boolean / ReplicatedState | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 700 / 700 / 701 | Level.HasPhotoDoor params=" " | boolean / ReplicatedState | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 701 / 701 / 702 | Level.InitializeDevilAngelRoom params=" boolean ForceAngel, boolean ForceDevil " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 702 / 702 / 703 | Level.IsAltStage params=" " | boolean / ReplicatedState | derived → @L0.stage/stage_type + @Run.difficulty + @Presentation | 纯元数据/显示语义，不增设原始Getter字段；完整GetAbsoluteStage公式未在本轮宣称实现 |
| 703 / 703 / 704 | Level.IsAscent params=" " | boolean / ReplicatedState | delegated → @Run.ascent_display_bit48 | 第8步已核bit48显示投影，Level不重复复制Game全flags |
| 704 / 704 / 705 | Level.IsDevilRoomDisabled params=" " | boolean / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 705 / 705 / 706 | Level.IsNextStageAvailable params=" " | boolean / ReplicatedState | authority → @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 706 / 706 / 707 | Level.IsPreAscent params=" " | boolean / ReplicatedState | authority → @HostRoute + @Presentation.FinalTransition | 路线/生成资格留Host；实际Nightmare/背景/转场由显示owner承接，不恢复Game bit47原值 |
| 707 / 707 / 708 | Level.IsStageAvailable params=" LevelStage Level, StageType Stage " | void / AuthorityOnly | authority → @HostStageUnlockEligibility | 本地Wiki列出解锁资格查询；只由Host决定路线，不查Client本地成就/解锁，不新增返回值字段 |
| 708 / 708 / 709 | Level.LeaveDoor params=null | int / ReplicatedState | authority → @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 709 / 709 / 710 | Level.MakeRedRoomDoor params=" int CurrentRoomIdx, DoorSlot Slot " | boolean / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 710 / 710 / 711 | Level.PlaceRoom params=" LevelGeneratorEntry Room, RoomConfigRoom RoomConfig, int Seed " | boolean / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 711 / 711 / 712 | Level.QueryRoomTypeIndex params=" RoomType RoomType, boolean Visited, RNG rng, boolean IgnoreGroup = false " | int / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 712 / 712 / 713 | Level.RemoveCompassEffect params=" " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 713 / 713 / 714 | Level.RemoveCurses params=" LevelCurse Curses " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 714 / 714 / 715 | Level.SetCanSeeEverything params=" boolean Value " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 715 / 715 / 716 | Level.SetForceSpecialQuest params=" SpecialQuest Quest " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 716 / 716 / 717 | Level.SetGreedWavesClearedWithoutRedHeartDamage params=" int WavesCleared " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 717 / 717 / 718 | Level.SetHeartPicked params=" " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 718 / 718 / 719 | Level.SetName params=" string Name " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 719 / 719 / 720 | Level.SetNextStage params=" " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 720 / 720 / 721 | Level.SetRedHeartDamage params=" " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 721 / 721 / 722 | Level.SetStage params=" int StageOffset, int StageTypeOffset " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 722 / 722 / 723 | Level.SetStateFlag params=" LevelStateFlag LevelStateFlag, boolean Val " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 723 / 723 / 724 | Level.ShowMap params=" " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 724 / 724 / 725 | Level.ShowName params=" boolean Sticky " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 725 / 725 / 726 | Level.TryPlaceRoom params=" RoomConfigRoom RoomConfigToPlace, int GridIndex, Dimension Dimension = -1, int Seed = 0, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false, boolean AllowNoNeighbors = false " | RoomDescriptor / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 726 / 726 / 727 | Level.TryPlaceRoomAtDoor params=" RoomConfigRoom RoomConfigToPlace, RoomDescriptor NeighborRoomDescriptor, DoorSlot DoorSlot, int Seed = 0, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | RoomDescriptor / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 727 / 727 / 728 | Level.UncoverHiddenDoor params=" int CurrentRoomIdx, DoorSlot Slot " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 728 / 728 / 729 | Level.Update params=" " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 729 / 729 / 730 | Level.UpdateVisibility params=" " | void / AuthorityOnly | authority → HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 730 / 730 / 731 | Minimap.GetDisplayedSize params=" " | Vector / ReplicatedState | derived → @LocalMinimap.DisplayedSize | 视口／展开alpha／本地config bounds派生；Wiki未展开时47×47不构成Host状态字段 |
| 731 / 731 / 732 | Minimap.GetHoldTime params=" " | int / ReplicatedState | local → @LocalMinimap.ViewInput | 本地Map长按历史，不强制其它端展开，不复制Host输入 |
| 732 / 732 / 733 | Minimap.GetIconsSprite params=" " | Sprite / ReplicatedState | resource → @Presentation.Minimap.IconsAppearance + L2 | 只保留实际资源／必要可变图层外观；每个房间图标由L1/L2驱动，不把指针或最后一帧当完整地图 |
| 733 / 733 / 734 | Minimap.GetItemIconsSprite params=" " | Sprite / ReplicatedState | resource → @Presentation.Minimap.ItemIconsAppearance + @Presentation.MapBadgeList | 道具／诅咒徽标必须有最终有序列表；逐项SetFrame使最后Sprite帧不等价整列；资源唯一owner承接 |
| 734 / 734 / 735 | Minimap.GetShakeDuration params=" " | int / ReplicatedState | authority → @HostMinimap.ShakeProducer | 只控制随机offset生产，Client使用最终Vector，不重放倒计时／随机 |
| 735 / 735 / 736 | Minimap.GetShakeOffset params=" " | Vector / ReplicatedState | delegated → @Presentation.Minimap.ShakeOffset | 同world最终二维偏移由Render直接消费；只有这一份owner，不从Client RNG重建 |
| 736 / 736 / 737 | Minimap.GetState params=" " | MinimapState / ReplicatedState | local → @LocalMinimap.ViewState | NORMAL／EXPANDED／EXPANDED_OPAQUE是本地查看状态，world显示限制另施加 |
| 737 / 737 / 738 | Minimap.Refresh params=" " | void / AuthorityOnly | local → @SafeMinimapCacheAdapter; native Refresh forbidden | 命令不下行；原生965D60→9635B0有FBO及challenge32随机选择，只能使用同world已核结果的受控缓存adapter |
| 738 / 738 / 739 | Minimap.SetHoldTime params=" int Time " | void / AuthorityOnly | local → @LocalMinimap.ViewInput | 本地UI写入不网络回放，不经原版Online动作历史 |
| 739 / 739 / 740 | Minimap.SetShakeDuration params=" int Duration " | void / AuthorityOnly | authority → @HostMinimap.ShakeProducer -> @Presentation.Minimap.ShakeOffset | Host生产控制命令不下行；Client只应用实际最终offset |
| 740 / 740 / 741 | Minimap.SetShakeOffset params=" Vector Offset " | void / AuthorityOnly | authority → @HostMinimap.ShakeProducer -> @Presentation.Minimap.ShakeOffset | 不回放Setter命令；结果由唯一Presentation状态owner承接 |
| 741 / 741 / 742 | Minimap.SetState params=" MinimapState State " | void / AuthorityOnly | local → @LocalMinimap.ViewState | 各端自行查看，Host命令不强制访客状态；world强制显示资格独立处理 |

补充项不计入上述原始行数：

| 补充范围 | 决定与原因 |
|---|---|
| MinimapConfig.borderSprite[2] | resource：小／大地图边框独立实际资源，公开12行缺席不等于可省略 |
| MapBadgeList | converted：966300的最多7项最终有序icons／curses，1B count + i8[count]；不能只发Sprite最后一帧或揭图flags |
| L1 exploration / L2 save icons / challenge32 map context | delegated：唯一地图事实：完整DisplayFlags、visited非零、五位flags、OR/max图标、Host已选锚点／窗口；不另建exploration重复块 |
| LevelStateFlag map/blue-map/full-map/compass | authority：原四bool仅为观测；正式同步用最终显示事实，实际道具徽标独立，不能从这些flag补值 |

直接归属计数：authority=69；derived=4；delegated=6；reference=1；structural=4；field=4；converted=3；sdk_conflict=1；local=5；resource=2。同一结果有多个API入口时复用唯一owner，不按行数增加负载。
