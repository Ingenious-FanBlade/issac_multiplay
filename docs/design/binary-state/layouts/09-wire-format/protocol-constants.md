# 公共协议常量

布局版本 87；由唯一布局源生成。数值登记不表示运行编解码已经接通。

| 分类 | 名称 | 数值 | 语义 |
|---|---|---|---|
| display_branch | normal | 0 | Q12终结分支：normal；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | console_hold | 1 | Q12终结分支：console_hold；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | pause_countdown | 2 | Q12终结分支：pause_countdown；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | game_over | 3 | Q12终结分支：game_over；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | leaderboard | 4 | Q12终结分支：leaderboard；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | stage_transition | 5 | Q12终结分支：stage_transition；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | room_transition | 6 | Q12终结分支：room_transition；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | active_overlay | 7 | Q12终结分支：active_overlay；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | generic_prompt | 8 | Q12终结分支：generic_prompt；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | unknown_ui_byte_24ecc | 9 | Q12终结分支：unknown_ui_byte_24ecc；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | secondary_fade | 10 | Q12终结分支：secondary_fade；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | pause_screen | 11 | Q12终结分支：pause_screen；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| display_branch | unknown_ui_word_24444 | 12 | Q12终结分支：unknown_ui_word_24444；按RE-1原生优先级在Update后状态选取；normal只在全部阻断均不成立时使用 |
| message | AppliedAck | 9 | 沿用线上编号，表示完整帧已经解码且接收快照可作基线 |
| message | Resync | 10 | 公共协议Kind编号；仅合同登记，运行stream_codec尚未接通 |
| message | StateFrame | 21 | 公共协议Kind编号；仅合同登记，运行stream_codec尚未接通 |
| message | ResourceTable | 23 | 公共协议Kind编号；仅合同登记，运行stream_codec尚未接通 |
| message | ResourceBlobChunk | 24 | 公共协议Kind24仅保留编号；版本61正文禁用306，不启用原始媒体／blob，也未接通运行stream_codec |
| resource_table | anm2-animation-table | 0 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | image-table | 1 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | anm2-files | 2 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | item-config | 3 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | entity-configs | 4 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | room-config-visual | 5 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | shader-table | 6 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_table | audio-table | 7 | 显式u8资源空间；不能因序号相等跨表替换 |
| resource_kind | ANM2File | 0 | 版本63已完成typed payload与table/kind数字矩阵；仅本地资源描述／有限recipe，不包含媒体文件内容。 |
| resource_kind | ImageFile | 1 | 版本63已完成typed payload与table/kind数字矩阵；仅本地资源描述／有限recipe，不包含媒体文件内容。 |
| resource_kind | BuiltinShader | 2 | 版本63已完成typed payload与table/kind数字矩阵；仅本地资源描述／有限recipe，不包含媒体文件内容。 |
| resource_kind | RoomConfigVisual | 3 | 版本63已完成typed payload与table/kind数字矩阵；仅本地资源描述／有限recipe，不包含媒体文件内容。 |
| resource_kind | SurfaceRecipe | 4 | 版本63已完成typed payload与table/kind数字矩阵；仅本地资源描述／有限recipe，不包含媒体文件内容。 |
| resource_kind | SurfacePixels | 5 | typed payload分派编号；完整payload及table/kind允许集由第9步后续登记；版本61仅保留编号，当前不启用、不接受原始媒体 |
| resource_kind | AnimationTable | 6 | 版本63已完成typed payload与table/kind数字矩阵；仅本地资源描述／有限recipe，不包含媒体文件内容。 |
| resource_kind | ItemConfig | 7 | 版本63已完成typed payload与table/kind数字矩阵；仅本地资源描述／有限recipe，不包含媒体文件内容。 |
| resource_kind | EntityConfig | 8 | 版本63已完成typed payload与table/kind数字矩阵；仅本地资源描述／有限recipe，不包含媒体文件内容。 |
| resource_kind | Audio | 9 | 版本63已完成typed payload与table/kind数字矩阵；仅本地资源描述／有限recipe，不包含媒体文件内容。 |
| resource_kind | ImagePath | 10 | 配置、历史栏及图层路径等只见到文件名时的图像资源；由 Client 加载本地文件并检查路径与本地几何。 |
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
| budget | total_mod_bytes_per_second | 1125000 | 三端模组持续9Mbps；包含全部通道／重传／封装 |
| budget | peer_bytes_per_second | 375000 | 默认每Peer持续额度，三端合计不得越过总额 |
| budget | world_bytes_per_second | 150000 | 每Peer world额度；30Hz平均5000B，含实际封装，不是每帧固定大小 |
| budget | body_bytes_per_second | 150000 | 每Peer body额度；60Hz平均2500B，含实际封装 |
| budget | control_bytes_per_second | 25000 | 每Peer控制额度 |
| budget | reliable_bytes_per_second | 50000 | 每Peer可靠表／资源／准备／恢复额度 |
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
| result_purpose | BossVersusResult | 374 | 版本61收窄旧374，仅Boss versus独立有序Sprite贡献；普通切房不建surface结果／依赖 |
| result_purpose | NightmareSceneResult | 375 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | GameOverResult | 376 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | StageTransitionResult | 377 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | ItemOverlayResult | 378 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | BossOverlayResult | 379 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | GenericPromptResult | 380 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | AudioVoiceState | 382 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| result_purpose | AudioWorldEvent | 383 | 用途号复用具名结果布局号；同用途不同实例由owner_instance/generation区分 |
| flag | Joint | 1 | Frame和AppliedAck bit0；对应双分段完整基线 |
| flag | Baseline | 1 | Section bit0，当且仅当base_version=0 |
| display_font | Latin | 0 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_font | Japanese | 1 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_font | Korean | 2 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_font | SimplifiedChinese | 3 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_font | TraditionalChinese | 4 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_font | Cyrillic | 5 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_style | BannerMain | 0 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_style | BannerSubtitle | 1 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_style | CurseMain | 2 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_style | CurseSubtitle | 3 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_style | Fortune | 4 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_style | PromptTitle | 5 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| display_style | PromptBody | 6 | 协议自有有限显示profile，非原生编号；本地字体／样式映射由第10／13步核定，缺失报Gap，不执行任意首码命令 |
| contribution_pass | Context | 0 | C/S自有有限贡献类别；不是原生枚举。Context只作父链，Value为根值，其余须命中对应typed节点；随机主／次／递归由第10步具名来源绑定，不按Client访问次数推断。 |
| contribution_pass | Value | 1 | C/S自有有限贡献类别；不是原生枚举。Context只作父链，Value为根值，其余须命中对应typed节点；随机主／次／递归由第10步具名来源绑定，不按Client访问次数推断。 |
| contribution_pass | Sprite | 2 | C/S自有有限贡献类别；不是原生枚举。Context只作父链，Value为根值，其余须命中对应typed节点；随机主／次／递归由第10步具名来源绑定，不按Client访问次数推断。 |
| contribution_pass | Image | 3 | C/S自有有限贡献类别；不是原生枚举。Context只作父链，Value为根值，其余须命中对应typed节点；随机主／次／递归由第10步具名来源绑定，不按Client访问次数推断。 |
| contribution_pass | LocalHistory | 4 | C/S自有有限贡献类别；不是原生枚举。Context只作父链，Value为根值，其余须命中对应typed节点；随机主／次／递归由第10步具名来源绑定，不按Client访问次数推断。 |
| contribution_pass | Fade | 5 | C/S自有有限贡献类别；不是原生枚举。Context只作父链，Value为根值，其余须命中对应typed节点；随机主／次／递归由第10步具名来源绑定，不按Client访问次数推断。 |
| contribution_pass | RandomPrimary | 6 | C/S自有有限贡献类别；不是原生枚举。Context只作父链，Value为根值，其余须命中对应typed节点；随机主／次／递归由第10步具名来源绑定，不按Client访问次数推断。 |
| contribution_pass | RandomSecondary | 7 | C/S自有有限贡献类别；不是原生枚举。Context只作父链，Value为根值，其余须命中对应typed节点；随机主／次／递归由第10步具名来源绑定，不按Client访问次数推断。 |
| contribution_pass | RandomRecursive | 8 | C/S自有有限贡献类别；不是原生枚举。Context只作父链，Value为根值，其余须命中对应typed节点；随机主／次／递归由第10步具名来源绑定，不按Client访问次数推断。 |
| presentation_budget | max_result_groups | 1024 | 整批数量保护；还须同时满足整消息字节、字段访问次数、调用者scratch及实际发送预算，不是常态发送额度。 |
| presentation_budget | max_result_dependencies | 8192 | 整批数量保护；还须同时满足整消息字节、字段访问次数、调用者scratch及实际发送预算，不是常态发送额度。 |
| presentation_budget | max_result_contributions | 8192 | 整批数量保护；还须同时满足整消息字节、字段访问次数、调用者scratch及实际发送预算，不是常态发送额度。 |
| presentation_budget | max_audio_events | 4096 | 整批数量保护；还须同时满足整消息字节、字段访问次数、调用者scratch及实际发送预算，不是常态发送额度。 |
| resource_graphics_use | SurfaceQuad | 0 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| resource_graphics_use | PresentationImage | 1 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| resource_graphics_use | Sprite | 2 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| resource_graphics_use | Bloom | 3 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| resource_graphics_use | Pixelation | 4 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| resource_graphics_use | Retrovision | 5 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| resource_graphics_use | Hallucination | 6 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| resource_graphics_use | ColorMod | 7 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| resource_graphics_use | Water | 8 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| resource_graphics_use | Wave | 9 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| resource_graphics_use | Background | 10 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| resource_graphics_use | Fade | 11 | 资源唯一源的固定shader用途；精确shader允许集见resource_protocol，不是任意GPU参数分派。 |
| acceptance_policy | binary_state_capability | 1 | 独立二进制状态能力；同时协商精确整数布局版本，不回退旧Range |
| acceptance_policy | binary_payload_flag | 1 | ICSR封套flags=1表示正文直接从50B二进制公共头开始；flags=0仍为旧compact控制，二者不混读；不增加或改变布局字段 |
| acceptance_policy | world_candidates | 1 | 普通world至多保留最新一个候选 |
| acceptance_policy | joint_candidates | 1 | joint至多保留最新一个完整候选 |
| acceptance_policy | inflight_applies | 1 | 同Peer一次最多一个原生应用批次 |
| acceptance_policy | member_entries | 65535 | 完整成员表上限，FFFF不占真实项 |
| acceptance_policy | frame_records | 65535 | 整帧记录总量保护，不替代实际消息及速率预算 |
| compression | world_body_zstd | 1 | 版本66：只在双方显式协商后允许world/body独立消息zstd；不改变可靠性／字段／64KiB解压保护 |
| compression | zstd_level | 3 | 沿用独立帧zstd level3；动态压缩结果严格更小时使用，否则保留原文 |
| compression | zstd_window_bytes | 8388608 | 沿用8MiB窗口保护；不使用跨消息字典，普通解压仍最多64KiB |
