# 公共协议常量

布局版本 60；由唯一布局源生成。数值登记不表示运行编解码已经接通。

| 分类 | 名称 | 数值 | 语义 |
|---|---|---|---|
| message | ContextReady | 8 | 公共协议Kind编号；仅合同登记，运行stream_codec尚未接通 |
| message | AppliedAck | 9 | 公共协议Kind编号；仅合同登记，运行stream_codec尚未接通 |
| message | Resync | 10 | 公共协议Kind编号；仅合同登记，运行stream_codec尚未接通 |
| message | StateFrame | 21 | 公共协议Kind编号；仅合同登记，运行stream_codec尚未接通 |
| message | MemberTable | 22 | 公共协议Kind编号；仅合同登记，运行stream_codec尚未接通 |
| message | ResourceTable | 23 | 公共协议Kind编号；仅合同登记，运行stream_codec尚未接通 |
| message | ResourceBlobChunk | 24 | 公共协议Kind编号；仅合同登记，运行stream_codec尚未接通 |
| resource_table | anm2-animation-table | 0 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | image-table | 1 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | anm2-files | 2 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | item-config | 3 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | entity-configs | 4 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | room-config-visual | 5 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | shader-table | 6 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | audio-table | 7 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_kind | ANM2File | 0 | typed payload分派编号；完整payload及table/kind允许集由第9步后续登记 |
| resource_kind | ImageFile | 1 | typed payload分派编号；完整payload及table/kind允许集由第9步后续登记 |
| resource_kind | BuiltinShader | 2 | typed payload分派编号；完整payload及table/kind允许集由第9步后续登记 |
| resource_kind | RoomConfigVisual | 3 | typed payload分派编号；完整payload及table/kind允许集由第9步后续登记 |
| resource_kind | SurfaceRecipe | 4 | typed payload分派编号；完整payload及table/kind允许集由第9步后续登记 |
| resource_kind | SurfacePixels | 5 | typed payload分派编号；完整payload及table/kind允许集由第9步后续登记 |
| resource_kind | AnimationTable | 6 | typed payload分派编号；完整payload及table/kind允许集由第9步后续登记 |
| resource_kind | ItemConfig | 7 | typed payload分派编号；完整payload及table/kind允许集由第9步后续登记 |
| resource_kind | EntityConfig | 8 | typed payload分派编号；完整payload及table/kind允许集由第9步后续登记 |
| resource_kind | Audio | 9 | typed payload分派编号；完整payload及table/kind允许集由第9步后续登记 |
| stream | World | 0 | 具名公共封装枚举；组合约束见正式合同 |
| stream | Body | 1 | 具名公共封装枚举；组合约束见正式合同 |
| group_op | Replace | 0 | 具名公共封装枚举；组合约束见正式合同 |
| group_op | Reference | 1 | 具名公共封装枚举；组合约束见正式合同 |
| group_op | Retire | 2 | 具名公共封装枚举；组合约束见正式合同 |
| dependency | Member | 0 | 具名公共封装枚举；组合约束见正式合同 |
| dependency | Resource | 1 | 具名公共封装枚举；组合约束见正式合同 |
| dependency | PlayerTopology | 2 | 具名公共封装枚举；组合约束见正式合同 |
| dependency | ResultGroup | 3 | 具名公共封装枚举；组合约束见正式合同 |
| dependency | PairedTarget | 4 | 具名公共封装枚举；组合约束见正式合同 |
| clear_scope | Voice | 0 | 具名公共封装枚举；组合约束见正式合同 |
| clear_scope | OwnerPurpose | 1 | 具名公共封装枚举；组合约束见正式合同 |
| clear_scope | WorldPurpose | 2 | 具名公共封装枚举；组合约束见正式合同 |
| clear_scope | RunPurpose | 3 | 具名公共封装枚举；组合约束见正式合同 |
| budget | total_link_bits_per_second | 10000000 | Host向三端总上行链路预算，非每Peer额度 |
| budget | total_mod_bytes_per_second | 937500 | 三端模组持续7.5Mbps；包含全部通道／重传／封装 |
| budget | peer_bytes_per_second | 312500 | 默认每Peer持续额度，三端合计不得越过总额 |
| budget | world_bytes_per_second | 120000 | 每Peer world额度；30Hz平均4000B，不是每帧固定大小 |
| budget | body_bytes_per_second | 120000 | 每Peer body额度；60Hz平均2000B |
| budget | control_bytes_per_second | 25000 | 每Peer控制额度 |
| budget | reliable_bytes_per_second | 47500 | 每Peer可靠表／资源／准备／恢复额度 |
| budget | total_burst_bytes | 24576 | 总突发上限，仍逐实际发送扣费 |
| budget | peer_burst_bytes | 8192 | 逐Peer突发上限，不能借共享编码漏计三端发送 |
| budget | world_burst_bytes | 4096 | world桶突发上限，大逻辑消息可分片跨轮次发送 |
| budget | body_burst_bytes | 4096 | body桶突发上限 |
| budget | ordinary_message_protection_bytes | 65536 | 既有非可靠路径未压缩逻辑消息保护上限，绝非常态发送额度 |
| budget | first_preparation_wire_bytes | 1048576 | 每Peer首次准备状态／表／资源及协议封装合计最多1MiB，三端照实计费 |
| budget | first_preparation_ms | 10000 | 从发起到画面就绪最多10秒 |
| budget | legacy_decode_protection_bytes | 33554432 | 既有可靠大消息／解码内存保护；不得作为正常world发送许可 |
| budget | max_blocks_per_layout | 32 | 单个对象布局块号0—31，不限制同帧对象数 |
| budget | max_sections | 2 | 仅world/body两条确认序列 |
| budget | max_contribution_depth | 16 | 有界贡献父链；不得用递归溢出绕过payload预算 |
| result_purpose | PlayerHUD | 315 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | StatHUDPlayer | 317 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | StatHUD | 318 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | HistoryHUDPlayer | 321 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | HUD | 323 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | Minimap | 325 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | PresentationDrawClock | 356 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | PickupRandomAlphaResult | 357 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | FamiliarRandomAlphaResult | 358 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | PillDisplayResult | 359 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | PickupPreviewResult | 361 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | PlayerDrawImageBindings | 362 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | ControlGroupHudSelection | 364 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | ScreenBloomResult | 367 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | ScreenPixelationResult | 368 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | ScreenRetrovisionResult | 369 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | ScreenHallucinationResult | 370 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | RoomGlitchResult | 371 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | ScreenEffectsResult | 373 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | RoomTransitionResult | 374 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | NightmareSceneResult | 375 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | GameOverResult | 376 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | StageTransitionResult | 377 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | ItemOverlayResult | 378 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | BossOverlayResult | 379 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | GenericPromptResult | 380 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | AudioVoiceState | 382 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | AudioWorldEvent | 383 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| flag | Joint | 1 | Frame和AppliedAck bit0；双分段共同提交 |
| flag | Baseline | 1 | Section bit0，当且仅当base_version=0 |
