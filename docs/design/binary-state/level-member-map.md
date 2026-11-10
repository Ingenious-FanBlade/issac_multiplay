# 第7步 Level：公开成员逐行回映射

2026-11-10；[专门核对](room-descriptor-synchronization-review.md)；[机器源](level-member-map.json)。

原始ordinal零基，原ownership／参数／重载／field／wire_name在JSON保留；不改写原目录，不表示运行Getter兼容。

决定数量：authority=90，converted=12，delegated=8，derived=4，field=8，local=1，reference=1，resource=2，sdk_conflict=1，structural=4。

| stream ordinal | 应用目录ordinal | ID | 原参数 | 旧归属 | 本轮决定 | 目标 | 理由 |
|---|---|---|---|---|---|---|---|
| 2781 | 161 | Level.AddAngelRoomChance | " float Chance " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2782 | 162 | Level.AddCurse | " LevelCurse Curse, boolean ShowName " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2783 | 163 | Level.ApplyBlueMapEffect | " " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2784 | 164 | Level.ApplyCompassEffect | " boolean Persistent " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2785 | 165 | Level.ApplyMapEffect | " " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2786 | 166 | Level.CanOpenChallengeRoom | " int RoomIndex " | NotReplicated | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2787 | 167 | Level.CanPlaceRoom | " RoomConfigRoom RoomConfigToPlace, int GridIndex, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false, boolean AllowNoNeighbors = false " | NotReplicated | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2788 | 168 | Level.CanPlaceRoom | " RoomShape RoomShape, DoorMask DoorMask, int GridIndex, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false, boolean AllowNoNeighbors = false " | NotReplicated | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2789 | 169 | Level.CanPlaceRoomAtDoor | " RoomConfigRoom RoomConfigToPlace, RoomDescriptor NeighborRoomDescriptor, DoorSlot DoorSlot, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | NotReplicated | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2790 | 170 | Level.CanPlaceRoomAtDoor | " RoomShape RoomShape, DoorMask DoorMask, RoomDescriptor NeighborRoomDescriptor, DoorSlot DoorSlot, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | NotReplicated | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2791 | 171 | Level.CanSpawnDevilRoom | " " | ReplicatedState | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2792 | 172 | Level.CanSpawnDoorOutline | " int RoomIdx, DoorSlot DoorSlot " | NotReplicated | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2793 | 173 | Level.CanStageHaveCurseOfLabyrinth | " LevelStage Stage " | NotReplicated | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2794 | 174 | Level.ChangeRoom | " int RoomIndex, int Dimension = -1 " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2795 | 175 | Level.DisableDevilRoom | " " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2796 | 176 | Level.DungeonReturnPosition | null | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2797 | 177 | Level.DungeonReturnRoomIndex | null | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2798 | 178 | Level.EnterDoor | null | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2799 | 179 | Level.FindValidRoomPlacementLocations | " RoomConfigRoom RoomConfig, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2800 | 180 | Level.FindValidRoomPlacementLocations | " RoomShape RoomShape = 1, DoorMask DoorMask = -1, Dimension Dimension = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2801 | 181 | Level.ForceHorsemanBoss | " int Seed " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2802 | 182 | Level.GetAbsoluteStage | " " | ReplicatedState | derived | @L0.stage/stage_type + @Run.difficulty + @Presentation | 纯元数据/显示语义，不增设原始Getter字段；完整GetAbsoluteStage公式未在本轮宣称实现 |
| 2803 | 183 | Level.GetAngelRoomChance | " " | ReplicatedState | delegated | @Presentation.StatHUD.final_values | 本API angel chance是182F4权重，不等于最终HUD概率；只同步正常Host显示缓存，不重算 |
| 2804 | 184 | Level.GetCanSeeEverything | " " | ReplicatedState | authority | @HostVisibility -> L1.display_flags/L2 | 可见性生产输入；最终DisplayFlags、有效诅咒和地图结果承接，不复制Host查看/更新状态 |
| 2805 | 185 | Level.GetCurrentRoom | " " | ReplicatedState | reference | @CurrentWorld.Room | 已有当前Room owner与结构身份；不复制地址，也不运行初始化 |
| 2806 | 186 | Level.GetCurrentRoomDesc | " " | ReplicatedState | structural | L1/L3 + @OrderedOrdinaryDescriptorMembers | 活动有序成员和物理源槽/三维映射承接；不调用73AD10的fallback初始化或随机/递归路由补采 |
| 2807 | 187 | Level.GetCurrentRoomIndex | " " | ReplicatedState | field | L0.current_entry_index | 保留入口象限／负特殊房间，不用GridIndex代替 |
| 2808 | 188 | Level.GetCurseName | " " | ReplicatedState | delegated | @Presentation.LevelName/CurseText | 保留实际已选文本/资源；challenge44逐房主题和官方CustomStageName不能仅由L0复算；实际显示批次待接线 |
| 2809 | 189 | Level.GetCurses | " " | ReplicatedState | converted | L0.effective_curses | 742450的最终组合；原始+C、temporary和seed强制/阻止均不独立下发 |
| 2810 | 190 | Level.GetDevilAngelRoomRNG | " " | AuthorityOnly | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2811 | 191 | Level.GetDimension | " " | ReplicatedState | converted | L0.dimension | 原生i32核0..2后编码u8；不发送默认-1 |
| 2812 | 192 | Level.GetDungeonPlacementSeed | " " | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2813 | 193 | Level.GetEnterPosition | " " | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2814 | 194 | Level.GetForceSpecialQuest | " " | ReplicatedState | authority | @LevelASM.ForceSpecialQuest | 对象外生成参数，非Game偏移；结果由实际拓扑/门/显示承接 |
| 2815 | 195 | Level.GetGenerationRNG | " " | AuthorityOnly | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2816 | 196 | Level.GetGreedWavesClearedWithoutRedHeartDamage | " " | ReplicatedState | sdk_conflict | @SDK18204 != @Native18374 (HostOnly) | 官方LuaLevel.cpp错误读取dimension2/grid120映射；真实计数由7455D7/80BEAB核到18374，二者均不作为Client状态 |
| 2817 | 197 | Level.GetHeartPicked | " " | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2818 | 198 | Level.GetLastBossRoomListIndex | " " | ReplicatedState | converted | L0.current_is_last_boss | 原始索引留Host；L0匹配bool在所有模式保留：97D0EA Greed HUD及803313普通TrapDoor Light都需要 |
| 2819 | 199 | Level.GetLastRoomDesc | " " | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2820 | 200 | Level.GetMyosotisPickups | " " | ReplicatedState | authority | @Host.MyosotisQueue + @Entity.CreatedPickup | 跨层保存/生成留Host，实际创建的Pickup独立同步，不在Client Restore |
| 2821 | 201 | Level.GetName | " " | ReplicatedState | delegated | @Presentation.LevelName/CurseText | 保留实际已选文本/资源；challenge44逐房主题和官方CustomStageName不能仅由L0复算；实际显示批次待接线 |
| 2822 | 202 | Level.GetNeighboringRooms | " int GridIndex, RoomShape RoomShape, Dimension Dimension = -1 " | ReplicatedState | derived | @DescriptorTopology | 从同world已安装配置形状、三维占用和源槽关系作具名纯结构查询；不是重跑放房/连门 |
| 2823 | 203 | Level.GetNonCompleteRoomIndex | " " | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2824 | 204 | Level.GetPlanetariumChance | " " | ReplicatedState | delegated | @Presentation.StatHUD.final_values | 本API angel chance是182F4权重，不等于最终HUD概率；只同步正常Host显示缓存，不重算 |
| 2825 | 205 | Level.GetPreviousRoomIndex | " " | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2826 | 206 | Level.GetRandomRoomIndex | " boolean IAmErrorRoom, int Seed " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2827 | 207 | Level.GetRoomByIdx | " int RoomIdx, int Dimension = -1 " | ReplicatedState | structural | L1/L3 + @OrderedOrdinaryDescriptorMembers | 活动有序成员和物理源槽/三维映射承接；不调用73AD10的fallback初始化或随机/递归路由补采 |
| 2828 | 208 | Level.GetRoomCount | " " | ReplicatedState | structural | L1/L3 + @OrderedOrdinaryDescriptorMembers | 活动有序成员和物理源槽/三维映射承接；不调用73AD10的fallback初始化或随机/递归路由补采 |
| 2829 | 209 | Level.GetRooms | " " | ReplicatedState | structural | L1/L3 + @OrderedOrdinaryDescriptorMembers | 活动有序成员和物理源槽/三维映射承接；不调用73AD10的fallback初始化或随机/递归路由补采 |
| 2830 | 210 | Level.GetStage | " " | ReplicatedState | field | L0.stage | 多个原生Room/HUD/地图消费者需要当前stage，不能仅传楼层名称 |
| 2831 | 211 | Level.GetStageType | " " | ReplicatedState | field | L0.stage_type | 当前全局stageType参与镜面与显示；挑战44逐房主题另由资源结果承接 |
| 2832 | 212 | Level.GetStartingRoomIndex | " " | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2833 | 213 | Level.GetStateFlag | " LevelStateFlag LevelStateFlag" | NotReplicated | authority | @HostLevelFlags -> @FinalWorldResults | 24个flags原值均不作为通用查询字典下发；最终地图/门/实体/音画仍必需 |
| 2834 | 214 | Level.GreedModeWave | null | ReplicatedState | field | L0.greed_wave | 97D371显示%d/%d；只读真实Greed当前wave，非Greed规范0 |
| 2835 | 215 | Level.HasAbandonedMineshaft | " " | ReplicatedState | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2836 | 216 | Level.HasBossChallenge | " " | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2837 | 217 | Level.HasMirrorDimension | " " | ReplicatedState | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2838 | 218 | Level.HasPhotoDoor | " " | ReplicatedState | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2839 | 219 | Level.InitializeDevilAngelRoom | " boolean ForceAngel, boolean ForceDevil " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2840 | 220 | Level.IsAltStage | " " | ReplicatedState | derived | @L0.stage/stage_type + @Run.difficulty + @Presentation | 纯元数据/显示语义，不增设原始Getter字段；完整GetAbsoluteStage公式未在本轮宣称实现 |
| 2841 | 221 | Level.IsAscent | " " | ReplicatedState | delegated | @Run.ascent_display_bit48 | 第8步已核bit48显示投影，Level不重复复制Game全flags |
| 2842 | 222 | Level.IsDevilRoomDisabled | " " | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2843 | 223 | Level.IsNextStageAvailable | " " | ReplicatedState | authority | @HostGenerationDecision -> @FinalTopology/Entity | 玩法/放房/路线谓词；官方镜面/矿井还受ForceSpecialQuest影响；只同步已裁决结构与实际表现，不在Client重跑决策 |
| 2844 | 224 | Level.IsPreAscent | " " | ReplicatedState | authority | @HostRoute + @Presentation.FinalTransition | 路线/生成资格留Host；实际Nightmare/背景/转场由显示owner承接，不恢复Game bit47原值 |
| 2845 | 225 | Level.IsStageAvailable | " LevelStage Level, StageType Stage " | AuthorityOnly | authority | @HostStageUnlockEligibility | 本地Wiki列出解锁资格查询；只由Host决定路线，不查Client本地成就/解锁，不新增返回值字段 |
| 2846 | 226 | Level.LeaveDoor | null | ReplicatedState | authority | @HostLevelSimulation | 返回/过门/生成/计数/随机/玩法选择输入；同步最终结构、Entity和转场，无必要复制原始值或为Getter额外执行程序 |
| 2847 | 227 | Level.MakeRedRoomDoor | " int CurrentRoomIdx, DoorSlot Slot " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2848 | 228 | Level.PlaceRoom | " LevelGeneratorEntry Room, RoomConfigRoom RoomConfig, int Seed " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2849 | 229 | Level.QueryRoomTypeIndex | " RoomType RoomType, boolean Visited, RNG rng, boolean IgnoreGroup = false " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2850 | 230 | Level.RemoveCompassEffect | " " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2851 | 231 | Level.RemoveCurses | " LevelCurse Curses " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2852 | 232 | Level.SetCanSeeEverything | " boolean Value " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2853 | 233 | Level.SetForceSpecialQuest | " SpecialQuest Quest " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2854 | 234 | Level.SetGreedWavesClearedWithoutRedHeartDamage | " int WavesCleared " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2855 | 235 | Level.SetHeartPicked | " " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2856 | 236 | Level.SetName | " string Name " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2857 | 237 | Level.SetNextStage | " " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2858 | 238 | Level.SetRedHeartDamage | " " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2859 | 239 | Level.SetStage | " int StageOffset, int StageTypeOffset " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2860 | 240 | Level.SetStateFlag | " LevelStateFlag LevelStateFlag, boolean Val " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2861 | 241 | Level.ShowMap | " " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2862 | 242 | Level.ShowName | " boolean Sticky " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2863 | 243 | Level.TryPlaceRoom | " RoomConfigRoom RoomConfigToPlace, int GridIndex, Dimension Dimension = -1, int Seed = 0, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false, boolean AllowNoNeighbors = false " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2864 | 244 | Level.TryPlaceRoomAtDoor | " RoomConfigRoom RoomConfigToPlace, RoomDescriptor NeighborRoomDescriptor, DoorSlot DoorSlot, int Seed = 0, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2865 | 245 | Level.UncoverHiddenDoor | " int CurrentRoomIdx, DoorSlot Slot " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2866 | 246 | Level.Update | " " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 2867 | 247 | Level.UpdateVisibility | " " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 3090 | 470 | RoomDescriptor.AddRestrictedGridIndex | " int GridIndex " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 3091 | 471 | RoomDescriptor.AllowedDoors | null | ReplicatedState | converted | L1.allowed_doors | 官方修复为有效低八位mask；Void地图框线消费；不同于实际门存在性 |
| 3092 | 472 | RoomDescriptor.AwardSeed | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3093 | 473 | RoomDescriptor.BossDeathSeed | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3094 | 474 | RoomDescriptor.ChallengeDone | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3095 | 475 | RoomDescriptor.Clear | null | ReplicatedState | converted | L1.display_bits.bit0 | 当前clear位为地图/Greed HUD消费；不因此生成清理奖励 |
| 3096 | 476 | RoomDescriptor.ClearCount | null | ReplicatedState | authority | @HostRoomHistory | 累计计数不发送；当前clear由display_bits承接 |
| 3097 | 477 | RoomDescriptor.CreateGroup | " string groupName " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 3098 | 478 | RoomDescriptor.Data | null | ReplicatedState | resource | L1.data + @RoomConfigurationResource | 已选基础配置，独立nullable u16资源；实际配置表待注册 |
| 3099 | 479 | RoomDescriptor.DecorationSeed | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3100 | 480 | RoomDescriptor.DeliriumDistance | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3101 | 481 | RoomDescriptor.DisplayFlags | null | ReplicatedState | field | L1.display_flags | 最终地图显示值；不执行原生可见性生产器 |
| 3102 | 482 | RoomDescriptor.Doors | null | ReplicatedState | field | L1.doors | 八个i32结构目标，保留特殊路由；目标存在不等于实际门存在 |
| 3103 | 483 | RoomDescriptor.Flags | null | ReplicatedState | converted | L1.display_bits | 五位原生0/10/11/18/5→线上0..4；sacrifice/pitch-black最终lighting及音乐/背景另承接；不回放全flags |
| 3104 | 484 | RoomDescriptor.GetDecoSaveState | " " | ReplicatedState | authority | @HostRoomSave + @Entity.CurrentEffects | 离房效果保存不复制，当前显示Effect由Entity域承接；无已核地图消费者 |
| 3105 | 485 | RoomDescriptor.GetDimension | " " | ReplicatedState | converted | L1.dimension | 原生i32核0..2后发送u8 |
| 3106 | 486 | RoomDescriptor.GetEntitiesSaveState | " " | ReplicatedState | delegated | L2.icon_mask/selected_frames | Host过滤后按原生OR/max聚合；不发保存数量/顺序/type/variant/subtype；存在时仅3..12B，保留Slot default和全部选中帧 |
| 3107 | 487 | RoomDescriptor.GetErrorTrinketEffect | " " | ReplicatedState | converted | D1.error_is_modeling_clay + @Presentation.ActualImage/ColorContext; numeric result HostOnly | 官方效果范围恒为1..189（零seed为75），>0无需发；实际Image已另传，仅需1B是否等于ModelingClay供颜色adapter；不替代数值Getter或推进RNG |
| 3108 | 488 | RoomDescriptor.GetGridEntitiesSaveState | " " | ReplicatedState | delegated | L2.icon_mask.bit14 | 20h有效元素任一type18只设置楼梯bit；固定帧0不发，不复制保存体/头/seed |
| 3109 | 489 | RoomDescriptor.GetGroup | "" | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3110 | 490 | RoomDescriptor.GetGroupByName | " string groupName " | NotReplicated | local | @OfficialLocalGroupRegistry | 官方进程内分组名称注册表；分组用于Host传送抽选，不复制注册表或私有ID |
| 3111 | 491 | RoomDescriptor.GetNeighboringRooms | " " | ReplicatedState | derived | @DescriptorTopology | 从同world已安装配置形状、三维占用和源槽关系作具名纯结构查询；不是重跑放房/连门 |
| 3112 | 492 | RoomDescriptor.GetRestrictedGridIndexes | " " | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3113 | 493 | RoomDescriptor.GetTaintedKeeperCoinSpawns | " " | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3114 | 494 | RoomDescriptor.GetValidNeighborPlacementLocations | " RoomConfigRoom RoomConfig, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | NotReplicated | authority | @HostRoomPlacement | 玩法放房候选查询，不是Client拓扑所需字段；不运行放房判断补源 |
| 3115 | 495 | RoomDescriptor.GetValidNeighborPlacementLocations | " RoomShape RoomShape = 1, DoorMask DoorMask = -1, boolean AllowMultipleDoors = true, boolean AllowSpecialNeighbors = false " | NotReplicated | authority | @HostRoomPlacement | 玩法放房候选查询，不是Client拓扑所需字段；不运行放房判断补源 |
| 3116 | 496 | RoomDescriptor.GridIndex | null | ReplicatedState | field | L1.grid_index | 描述的原始GridIndex，允许L型缺口，不能充当入口象限 |
| 3117 | 497 | RoomDescriptor.HasWater | null | ReplicatedState | converted | L1.display_bits.bit4 + @Room.FinalWater | 801BE0的Pit及804500 Backdrop32重绘直接检查原bit5；不能只用waterAmount/水色/WaterClip代替 |
| 3118 | 498 | RoomDescriptor.InitSeeds | " RNG RNG " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 3119 | 499 | RoomDescriptor.ListIndex | null | ReplicatedState | converted | @PhysicalSourceSlot + L0.current_is_last_boss | 不以可变ListIndex作存储下标；物理源槽独立，L0匹配bool供所有模式的HUD/TrapDoor Light |
| 3120 | 500 | RoomDescriptor.NoReward | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3121 | 501 | RoomDescriptor.OverrideData | null | ReplicatedState | resource | L1.override_data + @RoomConfigurationResource | 与基础配置独立；不合并成单个Type/Variant或随机重选 |
| 3122 | 502 | RoomDescriptor.PitsCount | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3123 | 503 | RoomDescriptor.PoopCount | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3124 | 504 | RoomDescriptor.PressurePlatesTriggered | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3125 | 505 | RoomDescriptor.SacrificeDone | null | ReplicatedState | delegated | @Room.FinalLighting/Pass | 原bit2在803E20影响献祭房最终shadow fill；不发原位，Room实际lighting必须承接，不归为无显示历史 |
| 3126 | 506 | RoomDescriptor.SafeGridIndex | null | ReplicatedState | field | L1.safe_grid_index | 原始安全格子索引；不能代替完整占用映射 |
| 3127 | 507 | RoomDescriptor.SetGroup | " int group " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 3128 | 508 | RoomDescriptor.SetTaintedKeeperCoinSpawns | " int Num " | AuthorityOnly | authority | HostOnly | 具名修改/生成/抽选/更新命令，只在Host执行；结果走正式状态，不向Client回放调用 |
| 3129 | 509 | RoomDescriptor.ShopItemDiscountIdx | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3130 | 510 | RoomDescriptor.ShopItemIdx | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3131 | 511 | RoomDescriptor.SpawnSeed | null | ReplicatedState | converted | D1.error_is_modeling_clay; raw seed/effect ID HostOnly | 出生生产seed及完整效果ID留Host；从合法当前seed按官方纯整数式求是否ModelingClay，D1仅1B；实际图像另承接，不在离房描述重复 |
| 3132 | 512 | RoomDescriptor.SurpriseMiniboss | null | ReplicatedState | authority | @HostGeneration/RoomSave -> @FinalWorldResults | 生成/奖励/防刷/传送选择/保存历史留Host；实际Entity/Grid/地图/音画继续由具名owner承接 |
| 3133 | 513 | RoomDescriptor.VisitedCount | null | ReplicatedState | converted | L1.visited | 地图只检查非零，累计历史留Host |

正式来源、资源和显示adapter待接线；当前生产版本56不接收本轮草稿。官方Greed Getter源码18204与真实18374的冲突仍见Level核对，不要求对照相等。
