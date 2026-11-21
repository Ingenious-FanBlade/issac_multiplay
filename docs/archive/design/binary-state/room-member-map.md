# 第7步 Room：公开成员回映射

修订2；保留原269行、重载／重复ID与签名。[JSON](room-member-map.json)；[剩余字节补证](room-residual-evidence.md)。delegated不表示本轮完成其他域。

| 原全局ordinal（0基） | field | 原成员／签名 | 决定 | 去向与理由 |
|---:|---:|---|---|---|
| 2739 | 2740 | Backdrop.GetControlsANM2 ( ) | resource | Backdrop实际Sprite／程序图像资源；实际采用分支才读；floor/wall是图像内容，不是只传ID／seed／名字；controls按钮按本地C/S设备语义 |
| 2740 | 2741 | Backdrop.GetControlsButtonsANM2 ( ) | resource | Backdrop实际Sprite／程序图像资源；实际采用分支才读；floor/wall是图像内容，不是只传ID／seed／名字；controls按钮按本地C/S设备语义 |
| 2741 | 2742 | Backdrop.GetDetailANM2 ( ) | resource | Backdrop实际Sprite／程序图像资源；实际采用分支才读；floor/wall是图像内容，不是只传ID／seed／名字；controls按钮按本地C/S设备语义 |
| 2742 | 2743 | Backdrop.GetFloorANM2 ( ) | resource | Backdrop实际Sprite／程序图像资源；实际采用分支才读；floor/wall是图像内容，不是只传ID／seed／名字；controls按钮按本地C/S设备语义 |
| 2743 | 2744 | Backdrop.GetFloorImage ( ) | resource | Backdrop实际Sprite／程序图像资源；实际采用分支才读；floor/wall是图像内容，不是只传ID／seed／名字；controls按钮按本地C/S设备语义 |
| 2744 | 2745 | Backdrop.GetWallImage ( ) | resource | Backdrop实际Sprite／程序图像资源；实际采用分支才读；floor/wall是图像内容，不是只传ID／seed／名字；controls按钮按本地C/S设备语义 |
| 2745 | 2746 | Camera.IsClampEnabled ( ) | local | Camera本地owner／focus意图；IsClampEnabled来自官方roomCameraData外部布尔，不在Camera+94内；IsPosVisible是本地视口查询；写命令不重放 |
| 2746 | 2747 | Camera.IsPosVisible ( Vector Pos ) | local | Camera本地owner／focus意图；IsClampEnabled来自官方roomCameraData外部布尔，不在Camera+94内；IsPosVisible是本地视口查询；写命令不重放 |
| 2747 | 2748 | Camera.SetClampEnabled ( boolean Clamped ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2748 | 2749 | Camera.SetFocusPosition ( Vector Pos ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2749 | 2750 | Camera.SnapToPosition ( Vector Pos ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2750 | 2751 | Camera.Update ( boolean flag = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2751 | 2752 | ColorModifier.A (None) | delegated | 第8步Game.current ColorModState；同名颜色不能当同一实例；本地算术不是网络命令；本轮不复活target/lerp/config副本 |
| 2752 | 2753 | ColorModifier.B (None) | delegated | 第8步Game.current ColorModState；同名颜色不能当同一实例；本地算术不是网络命令；本轮不复活target/lerp/config副本 |
| 2753 | 2754 | ColorModifier.Brightness (None) | delegated | 第8步Game.current ColorModState；同名颜色不能当同一实例；本地算术不是网络命令；本轮不复活target/lerp/config副本 |
| 2754 | 2755 | ColorModifier.ColorModifier ( float R = 1, float G = 1, float B = 1, float A = 0, float Brightness = 0, float Contrast = 1 ) | local | 无状态本地值运算；颜色算术／比较不产生需要复制的owner，不重放调用日志 |
| 2755 | 2756 | ColorModifier.Contrast (None) | delegated | 第8步Game.current ColorModState；同名颜色不能当同一实例；本地算术不是网络命令；本轮不复活target/lerp/config副本 |
| 2756 | 2757 | ColorModifier.G (None) | delegated | 第8步Game.current ColorModState；同名颜色不能当同一实例；本地算术不是网络命令；本轮不复活target/lerp/config副本 |
| 2757 | 2758 | ColorModifier.R (None) | delegated | 第8步Game.current ColorModState；同名颜色不能当同一实例；本地算术不是网络命令；本轮不复活target/lerp/config副本 |
| 2758 | 2759 | ColorModifier.__add ( ColorModifier right ) | local | 无状态本地值运算；颜色算术／比较不产生需要复制的owner，不重放调用日志 |
| 2759 | 2760 | ColorModifier.__div ( ColorModifier right ) | local | 无状态本地值运算；颜色算术／比较不产生需要复制的owner，不重放调用日志 |
| 2760 | 2761 | ColorModifier.__eq ( ColorModifier right ) | local | 无状态本地值运算；颜色算术／比较不产生需要复制的owner，不重放调用日志 |
| 2761 | 2762 | ColorModifier.__mul ( ColorModifier right ) | local | 无状态本地值运算；颜色算术／比较不产生需要复制的owner，不重放调用日志 |
| 2762 | 2763 | ColorModifier.__sub ( ColorModifier right ) | local | 无状态本地值运算；颜色算术／比较不产生需要复制的owner，不重放调用日志 |
| 2763 | 2764 | FXLayers.AddPoopFx ( ColorMod Color ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2764 | 2765 | FXParams.ColorModifier (None) | conversion | FXParams用途表／R2最终water/light/current color；ColorModifier是配置副本，不是Game current；WaterEffectColor是Effect.Init输入，最终Entity颜色另承接 |
| 2765 | 2766 | FXParams.LightColor (None) | conversion | FXParams用途表／R2最终water/light/current color；ColorModifier是配置副本，不是Game current；WaterEffectColor是Effect.Init输入，最终Entity颜色另承接 |
| 2766 | 2767 | FXParams.ShadowAlpha (None) | conversion | FXParams用途表／R2最终water/light/current color；ColorModifier是配置副本，不是Game current；WaterEffectColor是Effect.Init输入，最终Entity颜色另承接 |
| 2767 | 2768 | FXParams.ShadowColor (None) | conversion | FXParams用途表／R2最终water/light/current color；ColorModifier是配置副本，不是Game current；WaterEffectColor是Effect.Init输入，最终Entity颜色另承接 |
| 2768 | 2769 | FXParams.UseWaterV2 (None) | conversion | FXParams用途表／R2最终water/light/current color；ColorModifier是配置副本，不是Game current；WaterEffectColor是Effect.Init输入，最终Entity颜色另承接 |
| 2769 | 2770 | FXParams.WaterColor (None) | conversion | FXParams用途表／R2最终water/light/current color；ColorModifier是配置副本，不是Game current；WaterEffectColor是Effect.Init输入，最终Entity颜色另承接 |
| 2770 | 2771 | FXParams.WaterColorMultiplier (None) | conversion | FXParams用途表／R2最终water/light/current color；ColorModifier是配置副本，不是Game current；WaterEffectColor是Effect.Init输入，最终Entity颜色另承接 |
| 2771 | 2772 | FXParams.WaterEffectColor (None) | conversion | FXParams用途表／R2最终water/light/current color；ColorModifier是配置副本，不是Game current；WaterEffectColor是Effect.Init输入，最终Entity颜色另承接 |
| 2772 | 2773 | LRoomAreaDesc.GetHighBottomRight ( ) | derived | 同world R0 bounds＋RoomConfig.Shape；按已核12形表的纯几何规则；不是额外Room私有对象 |
| 2773 | 2774 | LRoomAreaDesc.GetHighTopLeft ( ) | derived | 同world R0 bounds＋RoomConfig.Shape；按已核12形表的纯几何规则；不是额外Room私有对象 |
| 2774 | 2775 | LRoomAreaDesc.GetLowBottomRight ( ) | derived | 同world R0 bounds＋RoomConfig.Shape；按已核12形表的纯几何规则；不是额外Room私有对象 |
| 2775 | 2776 | LRoomAreaDesc.GetLowTopLeft ( ) | derived | 同world R0 bounds＋RoomConfig.Shape；按已核12形表的纯几何规则；不是额外Room私有对象 |
| 2776 | 2777 | LRoomTileDesc.GetHighBottomRight ( ) | derived | 同world R0 bounds＋RoomConfig.Shape；按已核12形表的纯几何规则；不是额外Room私有对象 |
| 2777 | 2778 | LRoomTileDesc.GetHighTopLeft ( ) | derived | 同world R0 bounds＋RoomConfig.Shape；按已核12形表的纯几何规则；不是额外Room私有对象 |
| 2778 | 2779 | LRoomTileDesc.GetLowBottomRight ( ) | derived | 同world R0 bounds＋RoomConfig.Shape；按已核12形表的纯几何规则；不是额外Room私有对象 |
| 2779 | 2780 | LRoomTileDesc.GetLowTopLeft ( ) | derived | 同world R0 bounds＋RoomConfig.Shape；按已核12形表的纯几何规则；不是额外Room私有对象 |
| 2780 | 2781 | LRoomTileDesc.GetRandomTile ( int Seed ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2868 | 2869 | RailManager.GetRailsSprite ( ) | resource | RailManager.sprite资源／必要层覆盖；main rail/pitrail与每格帧由实际轨道派生；不发推进状态或创造矿车 |
| 2869 | 2870 | Room.CanPickupGridEntity ( int GridIndex ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2870 | 2871 | Room.CanSpawnObstacleAtPosition ( int GridIndex, boolean Force ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2871 | 2872 | Room.CheckLine ( Vector Pos1, Vector Pos2, LinecheckMode Mode, int GridPathThreshold = 0, boolean IgnoreWalls = false, boolean IgnoreCrushable = false ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2872 | 2873 | Room.ClearBossHazards ( boolean IgnoreNPCs = true, Entity Source = nil ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2873 | 2874 | Room.DamageGrid ( int Index, int Damage ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2874 | 2875 | Room.DamageGridWithSource ( int Index, int Damage, EntityRef Source ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2875 | 2876 | Room.DestroyGrid ( int Index, boolean Immediate ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2876 | 2877 | Room.DestroyGridWithSource ( int Index, boolean Immediate, EntityRef Source ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2877 | 2878 | Room.DoLightningStrike ( int Seed = RandomSeed ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2878 | 2879 | Room.EmitBloodFromWalls ( int Duration, int Count ) | authority | Host command；452520写Room+7214 Duration和+7218 Count；只同步已产生效果。 |
| 2879 | 2880 | Room.FindFreePickupSpawnPosition ( Vector Pos, float InitialStep = 0, boolean AvoidActiveEntities = false, boolean AllowPits = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2880 | 2881 | Room.FindFreeTilePosition ( Vector Pos, float DistanceThreshold ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2881 | 2882 | Room.GetAliveBossesCount ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2882 | 2883 | Room.GetAliveEnemiesCount ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2883 | 2884 | Room.GetAwardSeed ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2884 | 2885 | Room.GetBackdrop ( ) | reference | Backdrop；转资源／正式成员或本地owner，禁止发送地址、STL头、完整候选与缓存 |
| 2885 | 2886 | Room.GetBackdropType ( ) | field | Backdrop.backdrop_id；保留具体显示消费者所需当前值；Getter本身不作为正式reader |
| 2886 | 2887 | Room.GetBossID ( ) | conversion | 第8步实际BossIntro资源（原ID归Host）；发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖 |
| 2887 | 2888 | Room.GetBossVictoryJingle ( ) | conversion | 第8步实际选中jingle voice/event（不为采集重新选择）；发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖 |
| 2888 | 2889 | Room.GetBottomRightPos ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2889 | 2890 | Room.GetBrokenWatchState ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2890 | 2891 | Room.GetCamera ( ) | reference | 本地Camera＋必要focus显示意图；转资源／正式成员或本地owner，禁止发送地址、STL头、完整候选与缓存 |
| 2891 | 2892 | Room.GetCenterPos ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2892 | 2893 | Room.GetChampionBossChance ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2893 | 2894 | Room.GetClampedGridIndex ( Vector Position ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2894 | 2895 | Room.GetClampedPosition ( Vector Pos, float Margin ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2895 | 2896 | Room.GetDecorationSeed ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2896 | 2897 | Room.GetDeliriumDistance ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2897 | 2898 | Room.GetDevilRoomChance ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2898 | 2899 | Room.GetDoor ( DoorSlot Slot ) | reference | R3 nullable成员引用；转资源／正式成员或本地owner，禁止发送地址、STL头、完整候选与缓存 |
| 2899 | 2900 | Room.GetDoorSlotPosition ( DoorSlot Slot ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2900 | 2901 | Room.GetDungeonRockIdx ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2901 | 2902 | Room.GetEffects ( ) | conversion | 效果481等实际显示资格；原容器留Host；发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖 |
| 2902 | 2903 | Room.GetEnemyDamageInflicted ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2903 | 2904 | Room.GetEntities ( ) | reference | Entity正式成员／顺序；EntityList的空间／碰撞／查询缓存与原版Online32帧历史不上线；正式成员与有序绘制关系承接。 |
| 2904 | 2905 | Room.GetFXLayers ( ) | reference | FXLayers actual visual records；转资源／正式成员或本地owner，禁止发送地址、STL头、完整候选与缓存 |
| 2905 | 2906 | Room.GetFXParams ( ) | reference | FXParams用途投影；转资源／正式成员或本地owner，禁止发送地址、STL头、完整候选与缓存 |
| 2906 | 2907 | Room.GetFloorColor ( ) | field | Room.floor_color；保留具体显示消费者所需当前值；Getter本身不作为正式reader |
| 2907 | 2908 | Room.GetFrameCount ( ) | field | R1.frame_baseline＋world.GameFrame；保留具体显示消费者所需当前值；Getter本身不作为正式reader |
| 2908 | 2909 | Room.GetGreedWaveTimer ( ) | field | R1.greed_wave_timer；保留具体显示消费者所需当前值；Getter本身不作为正式reader |
| 2909 | 2910 | Room.GetGridCollision ( int GridIndex ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2910 | 2911 | Room.GetGridCollisionAtPos ( Vector Pos ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2911 | 2912 | Room.GetGridEntity ( int Index ) | reference | 第6步正式Grid成员；转资源／正式成员或本地owner，禁止发送地址、STL头、完整候选与缓存 |
| 2912 | 2913 | Room.GetGridEntityFromPos ( Vector Position ) | reference | 第6步Grid成员＋R0几何；转资源／正式成员或本地owner，禁止发送地址、STL头、完整候选与缓存 |
| 2913 | 2914 | Room.GetGridHeight ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2914 | 2915 | Room.GetGridIndex ( Vector Position ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2915 | 2916 | Room.GetGridIndexByTile ( int GridRow, int GridColumn ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2916 | 2917 | Room.GetGridIndexByTile ( int[2] Tile ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2917 | 2918 | Room.GetGridPath ( int Index ) | authority | Host gameplay/query；原生DWORD表448项，SDK456错误；后接448个i16 NPC占用计时，两表均Host。 |
| 2918 | 2919 | Room.GetGridPathFromPos ( Vector Position ) | authority | Host gameplay/query；按几何定位到Host DWORD表；不为Client寻路查询复制表。 |
| 2919 | 2920 | Room.GetGridPosition ( int GridIndex ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2920 | 2921 | Room.GetGridSize ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2921 | 2922 | Room.GetGridWidth ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2922 | 2923 | Room.GetItemPool ( int Seed = Random(), boolean Raw = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2923 | 2924 | Room.GetLRoomAreaDesc ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2924 | 2925 | Room.GetLRoomTileDesc ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2925 | 2926 | Room.GetLaserTarget ( Vector Pos, Vector Dir ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2926 | 2927 | Room.GetLavaIntensity ( ) | field | Room.lava_intensity；保留具体显示消费者所需当前值；Getter本身不作为正式reader |
| 2927 | 2928 | Room.GetLightingAlpha ( ) | conversion | 实际lighting_alpha；发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖 |
| 2928 | 2929 | Room.GetLightningIntensity ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2929 | 2930 | Room.GetNumRainSpawners ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2930 | 2931 | Room.GetRail ( int GridIndex ) | reference | RailManager.grid_flags[actual_extent]；转资源／正式成员或本地owner，禁止发送地址、STL头、完整候选与缓存 |
| 2931 | 2932 | Room.GetRailManager ( ) | reference | RailManager；转资源／正式成员或本地owner，禁止发送地址、STL头、完整候选与缓存 |
| 2932 | 2933 | Room.GetRainIntensity ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2933 | 2934 | Room.GetRandomPosition ( float Margin ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2934 | 2935 | Room.GetRandomTileIndex ( int Seed ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2935 | 2936 | Room.GetRedHeartDamage ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2936 | 2937 | Room.GetRenderMode ( ) | conversion | 本地当前受控绘制pass；发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖 |
| 2937 | 2938 | Room.GetRenderScrollOffset ( ) | field | Room.render_scroll；保留具体显示消费者所需当前值；Getter本身不作为正式reader |
| 2938 | 2939 | Room.GetRenderSurfaceTopLeft ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2939 | 2940 | Room.GetRoomClearDelay ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2940 | 2941 | Room.GetRoomConfigStage ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2941 | 2942 | Room.GetRoomShape ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2942 | 2943 | Room.GetSecondBossID ( ) | conversion | 第8步实际第二BossIntro资源（原ID归Host）；发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖 |
| 2943 | 2944 | Room.GetSeededCollectible ( int Seed, bool NoDecrease = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2944 | 2945 | Room.GetShopItemPrice ( int EntityVariant, int EntitySubType, int ShopItemID ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2945 | 2946 | Room.GetShopLevel ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2946 | 2947 | Room.GetSpawnSeed ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2947 | 2948 | Room.GetTintedRockIdx ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2948 | 2949 | Room.GetTopLeftPos ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2949 | 2950 | Room.GetType ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2950 | 2951 | Room.GetWallColor ( ) | field | Room.wall_color；保留具体显示消费者所需当前值；Getter本身不作为正式reader |
| 2951 | 2952 | Room.GetWaterAmount ( ) | field | Room.water_amount；保留具体显示消费者所需当前值；Getter本身不作为正式reader |
| 2952 | 2953 | Room.GetWaterColor ( ) | conversion | WaterShaderResult.final_color（原端点getter归Host）；发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖 |
| 2953 | 2954 | Room.GetWaterColorMultiplier ( ) | conversion | WaterShaderResult.final_multiplier_rgb（原端点getter归Host）；发送当前实际采用显示结果；不为原始API返回值相等复制全部生产依赖 |
| 2954 | 2955 | Room.GetWaterCurrent ( ) | field | Room.water_current；保留具体显示消费者所需当前值；Getter本身不作为正式reader |
| 2955 | 2956 | Room.HasCurseMist ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2956 | 2957 | Room.HasLava ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2957 | 2958 | Room.HasSlowDown ( ) | authority | Host gameplay/query；852DA5→452540实际读Room+722C >0；SDK+7220不是它。 |
| 2958 | 2959 | Room.HasTriggerPressurePlates ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2959 | 2960 | Room.HasWater ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2960 | 2961 | Room.HasWaterPits ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2961 | 2962 | Room.InvalidatePickupVision ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2962 | 2963 | Room.IsAmbushActive ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2963 | 2964 | Room.IsAmbushDone ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2964 | 2965 | Room.IsChampionBossSeed ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2965 | 2966 | Room.IsClear ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2966 | 2967 | Room.IsCurrentRoomLastBoss ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2967 | 2968 | Room.IsDoorSlotAllowed ( DoorSlot Slot ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2968 | 2969 | Room.IsFirstEnemyDead ( ) | authority | Host gameplay/query；6B4380读取+72A0单byte；Host玩法资格，不是独立显示位。 |
| 2969 | 2970 | Room.IsFirstVisit ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2970 | 2971 | Room.IsInitialized ( ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2971 | 2972 | Room.IsLShapedRoom ( ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2972 | 2973 | Room.IsMirrorWorld ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2973 | 2974 | Room.IsPersistentRoomEntity ( EntityType Type, int Variant = 0 ) | authority | Host gameplay/query；寻路／碰撞／生成／计数／伤害／价格生产留Host；实际Entity/门/HUD/音频结果由各自域承接，不为目录相等下发 |
| 2974 | 2975 | Room.IsPositionInRoom ( Vector Pos, float Margin ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2975 | 2976 | Room.IsSacrificeDone ( ) | delegated | 第7步Level/RoomDescriptor或第8步具体显示owner；本轮只Room物理范围；描述flags／生成seed等不在这里重复上线，需对应域按消费者核定 |
| 2976 | 2977 | Room.KeepDoorsClosed ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2977 | 2978 | Room.MamaMegaExplosion ( Vector Position ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2978 | 2979 | Room.MamaMegaExplosion ( Vector Position = Vector.Zero, EntityPlayer Player = nil ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2979 | 2980 | Room.PickupGridEntity ( int GridIndex ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2980 | 2981 | Room.PlayMusic ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2981 | 2982 | Room.RemoveDoor ( DoorSlot Slot ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2982 | 2983 | Room.RemoveGridEntity ( int GridIndex, int PathTrail, boolean KeepDecoration ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2983 | 2984 | Room.RemoveGridEntityImmediate ( int GridIndex, int PathTrail, boolean KeepDecoration ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2984 | 2985 | Room.Render ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2985 | 2986 | Room.RespawnEnemies ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2986 | 2987 | Room.SaveState ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2987 | 2988 | Room.ScreenWrapPosition ( Vector Pos, float Margin ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 2988 | 2989 | Room.SetAmbushDone ( boolean Value ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2989 | 2990 | Room.SetBackdropType ( BackdropType, int Unknown ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2990 | 2991 | Room.SetBrokenWatchState ( int State ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2991 | 2992 | Room.SetCardAgainstHumanity ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2992 | 2993 | Room.SetClear ( boolean Clear ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2993 | 2994 | Room.SetFirstEnemyDead ( boolean Value ) | authority | Host command；6B4390写+72A0单byte，不安装到Client触发奖励。 |
| 2994 | 2995 | Room.SetFloorColor ( Color FloorColor ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2995 | 2996 | Room.SetGreedWaveTimer ( int Time ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2996 | 2997 | Room.SetGridPath ( int Index, int Value ) | authority | Host command；只在Host处理；不得越过448项写入后接WORD占用表。 |
| 2997 | 2998 | Room.SetItemPool ( ItemPoolType PoolType ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2998 | 2999 | Room.SetLavaIntensity ( float Intensity ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 2999 | 3000 | Room.SetLightningIntensity ( float Intensity ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3000 | 3001 | Room.SetPauseTimer ( int Duration ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3001 | 3002 | Room.SetRail ( int GridIndex, StbRailVariant RailVariant ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3002 | 3003 | Room.SetRainIntensity ( float Intensity ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3003 | 3004 | Room.SetRedHeartDamage ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3004 | 3005 | Room.SetRedHeartDamage ( boolean Value = true ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3005 | 3006 | Room.SetRoomClearDelay ( int Delay ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3006 | 3007 | Room.SetSacrificeDone ( boolean Done ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3007 | 3008 | Room.SetSlowDown ( int Duration ) | authority | Host command；852D9B→452550实际写Room+722C=max(current,arg)，不重放Client。 |
| 3008 | 3009 | Room.SetWallColor ( Color WallColor ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3009 | 3010 | Room.SetWaterAmount ( float Amount ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3010 | 3011 | Room.SetWaterColor ( KColor WaterColor ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3011 | 3012 | Room.SetWaterColorMultiplier ( KColor WaterColor ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3012 | 3013 | Room.SetWaterCurrent ( Vector WaterCurrent ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3013 | 3014 | Room.ShopReshuffle ( boolean KeepCollectibleIdx, boolean ReselectSaleItem ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3014 | 3015 | Room.ShopRestockFull ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3015 | 3016 | Room.ShopRestockPartial ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3016 | 3017 | Room.SpawnClearAward ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3017 | 3018 | Room.SpawnGridEntity ( int GridIndex, GridEntityDesc Descriptor) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3018 | 3019 | Room.SpawnGridEntity ( int GridIndex, GridEntityType Type, int Variant = 0, int Seed = nil, int VarData = 0 ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3019 | 3020 | Room.SpawnGridEntity ( int GridIndex, GridEntityType Type, int Variant, int Seed, int VarData ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3020 | 3021 | Room.StopRain ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3021 | 3022 | Room.TriggerClear ( boolean Silent = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3022 | 3023 | Room.TriggerOutput ( RoomEventOutput GroupIdx) | authority | Host command；818250遍历10组set并调用vector中的原生函数指针；整组／回调表绝不下发。 |
| 3023 | 3024 | Room.TriggerRestock ( int GridIndex, int ShopItemIdx) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3024 | 3025 | Room.TryGetShopDiscount ( int ShopItemIdx, int Price ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3025 | 3026 | Room.TryMakeBridge ( GridEntity pit, GridEntity rock ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3026 | 3027 | Room.TryPlaceLadder ( Vector PlayerPos, Vector PlayerVelocity, Entity Ladder ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3027 | 3028 | Room.TrySpawnBlueWombDoor ( boolean FirstTime = true, boolean IgnoreTime = false, boolean Force = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3028 | 3029 | Room.TrySpawnBossRushDoor ( boolean IgnoreTime = false, boolean Force = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3029 | 3030 | Room.TrySpawnDevilRoomDoor ( boolean Animate = false, boolean Force = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3030 | 3031 | Room.TrySpawnMegaSatanRoomDoor ( boolean Force = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3031 | 3032 | Room.TrySpawnSecretExit ( boolean Animate = false, boolean Force = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3032 | 3033 | Room.TrySpawnSecretShop ( boolean Force = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3033 | 3034 | Room.TrySpawnSpecialQuestDoor ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3034 | 3035 | Room.TrySpawnSpecialQuestDoor ( boolean IgnoreStageType = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3035 | 3036 | Room.TrySpawnTheVoidDoor ( boolean Force = false ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3036 | 3037 | Room.TurnGold ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3037 | 3038 | Room.Update ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3038 | 3039 | Room.UpdateColorModifier ( boolean Process = true, boolean Lerp = true, float Rate = 0.015 ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3039 | 3040 | Room.WorldToScreenPosition ( Vector WorldPos ) | derived | R0 actual geometry＋本地视口；只使用已核纯几何；不调用随机tile／寻路／碰撞／玩法更新 |
| 3040 | 3041 | RoomConfig.AddRooms ( StbType Stage, int Mode, table[] Rooms ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3041 | 3042 | RoomConfig.GetRandomRoom ( int Seed, boolean ReduceWeight, StbType Stage, RoomType Type, RoomShape Shape = RoomShape.NUM_ROOMSHAPES, int MinVariant = 0, int MaxVariant = -1, int MinDifficulty = 0, int MaxDifficulty = 10, int RequiredDoors = 0, int Subtype = -1, int Mode = -1 ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3042 | 3043 | RoomConfig.GetRoomByStageTypeAndVariant ( StbType Stage, RoomType Type, int Variant, int Mode = -1 ) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3043 | 3044 | RoomConfig.GetStage ( StbType Stage ) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3044 | 3045 | RoomConfig.LoadStb ( StbType Stage, int Mode, string StbFileName ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3045 | 3046 | RoomConfigEntry.Subtype (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3046 | 3047 | RoomConfigEntry.Type (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3047 | 3048 | RoomConfigEntry.Variant (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3048 | 3049 | RoomConfigEntry.Weight (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3049 | 3050 | RoomConfigRoom.Difficulty (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3050 | 3051 | RoomConfigRoom.Doors (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3051 | 3052 | RoomConfigRoom.Height (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3052 | 3053 | RoomConfigRoom.InitialWeight (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3053 | 3055 | RoomConfigRoom.Mode (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3054 | 3055 | RoomConfigRoom.Mode (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3055 | 3056 | RoomConfigRoom.Name (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3056 | 3057 | RoomConfigRoom.OriginalVariant (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3057 | 3058 | RoomConfigRoom.Shape (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3058 | 3059 | RoomConfigRoom.SpawnCount (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3059 | 3060 | RoomConfigRoom.Spawns (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3060 | 3061 | RoomConfigRoom.StageID (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3061 | 3062 | RoomConfigRoom.Subtype (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3062 | 3063 | RoomConfigRoom.Type (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3063 | 3064 | RoomConfigRoom.Variant (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3064 | 3065 | RoomConfigRoom.Weight (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3065 | 3066 | RoomConfigRoom.Width (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3066 | 3067 | RoomConfigSpawn.Entries (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3067 | 3068 | RoomConfigSpawn.EntryCount (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3068 | 3069 | RoomConfigSpawn.PickEntry ( float r ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3069 | 3070 | RoomConfigSpawn.SumWeights (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3070 | 3071 | RoomConfigSpawn.X (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3071 | 3072 | RoomConfigSpawn.Y (None) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3072 | 3073 | RoomConfigStage.GetBackdrop ( ) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3073 | 3074 | RoomConfigStage.GetBossSpot ( ) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3074 | 3075 | RoomConfigStage.GetDisplayName ( ) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3075 | 3076 | RoomConfigStage.GetID ( ) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3076 | 3077 | RoomConfigStage.GetMusic ( ) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3077 | 3078 | RoomConfigStage.GetPlayerSpot ( ) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3078 | 3079 | RoomConfigStage.GetRoomSet ( int Mode ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3079 | 3080 | RoomConfigStage.GetSuffix ( ) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3080 | 3081 | RoomConfigStage.GetXMLName ( ) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3081 | 3082 | RoomConfigStage.IsLoaded ( int mode = 0 ) | delegated | 第7步RoomDescriptor/RoomConfig资源核对（本轮未完成）；保留两份Data/Override区别与完整实际配置身份；不能仅stage/type/variant/subtype推断内容相同，不执行候选 |
| 3082 | 3083 | RoomConfigStage.SetBackdrop ( BackdropType Backdrop ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3083 | 3084 | RoomConfigStage.SetBossSpot ( string PngFilename ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3084 | 3085 | RoomConfigStage.SetDisplayName ( string Name ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3085 | 3086 | RoomConfigStage.SetMusic ( Music Music ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3086 | 3087 | RoomConfigStage.SetPlayerSpot ( string PngFilename ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3087 | 3088 | RoomConfigStage.SetSuffix ( string Suffix ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3088 | 3089 | RoomConfigStage.SetXMLName ( string name ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3089 | 3090 | RoomConfigStage.Unload ( ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |
| 3134 | 3135 | RoomTransition.GetPlayerExtraPortraitSprite ( ) | delegated | 第8步RoomTransition显示pass；已有Presentation字节表承接；不以StartBossIntro重放流程 |
| 3135 | 3136 | RoomTransition.GetTransitionMode ( ) | delegated | 第8步RoomTransition显示pass；已有Presentation字节表承接；不以StartBossIntro重放流程 |
| 3136 | 3137 | RoomTransition.GetVersusScreenSprite ( ) | delegated | 第8步RoomTransition显示pass；已有Presentation字节表承接；不以StartBossIntro重放流程 |
| 3137 | 3138 | RoomTransition.IsRenderingBossIntro ( ) | delegated | 第8步RoomTransition显示pass；已有Presentation字节表承接；不以StartBossIntro重放流程 |
| 3138 | 3139 | RoomTransition.StartBossIntro ( BossType BossID1, BossType BossID2 = 0 ) | authority | Host command；命令／构造／算术调用不作为状态重放；Render/Update/Save/随机选择同样不放行 |

原生补充：`Room::GetFloorColorAt VA96E3C0`为Host采样查询，见JSON supplementary_native；不是新增Lua reader。
