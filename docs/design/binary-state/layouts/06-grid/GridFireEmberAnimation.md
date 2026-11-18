# GridFireEmberAnimation

由唯一布局源生成。布局编号 203；布局版本 84；原生大小 0x20；未知 3B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

版本51：Fire第三ANM2的主动画视图，最小/固定3B；SetFrame覆写帧与两组选择器，但不写is_playing，官方InterpolV2会读该开关，必须保留。未知3B保留；SetFrame会写事件位，Client逐粒子绘制scratch/恢复，绝不派发Host动画事件。不能推广到通用AnimationState或overlay。

原生字节归属：

| 0x0 ANM2* animation_owner【本地】回指所属 ANM2；由上层对象关系确定（4B；已核） | 0x4 AnimationData* active_animation【资源】当前动画资源项；序号由 ANM2 文件表解析，空指针使用资源空值标记（4B；已核） | 0x8 int* layer_frames【派生】Fire Render逐粒子在主AnimationData非空时调用SetFrame：从该粒子age/maxLife与动画frameCount算出整帧，清零并重新求图层/空层选择器；旧缓存不传，空动画保持本地安全空数组（4B；已核） | 0xC int* null_layer_frames【派生】Fire Render逐粒子在主AnimationData非空时调用SetFrame：从该粒子age/maxLife与动画frameCount算出整帧，清零并重新求图层/空层选择器；旧缓存不传，空动画保持本地安全空数组（4B；已核） | 0x10 f32 anim_frame【派生】Fire Render逐粒子在主AnimationData非空时调用SetFrame：从该粒子age/maxLife与动画frameCount算出整帧，清零并重新求图层/空层选择器；旧缓存不传，空动画保持本地安全空数组（4B；已核） | 0x14 bool is_playing【状态】播放标志兼绘制插值输入；直接安装最终值，不授权Client推进动画（1B；SDK声明） | 0x15 bytes[3] unknown_15_17【未知】SDK 字段间未解释的三个字节；不认作填充（3B；未知） | 0x18 i32 currently_triggered_events【权威】Host动画事件消费位；Client安装最终姿态，不推进或重放事件，构造／换动画后本地清零（4B；已核） | 0x1C i32 previously_triggered_events【权威】Host动画事件历史位；不表示可靠事件队列，不进入Client状态或漂移比较（4B；已核） |
|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 3—3B，不含第 9 步记录头）：

块 0：粒子模板主动画资源与插值开关（body，3—3B）：

| +0x0 u16 active_animation：当前动画资源项；序号由 ANM2 文件表解析，空指针使用资源空值标记 | +0x2 u8 is_playing：播放标志兼绘制插值输入；直接安装最终值，不授权Client推进动画 |
|---|---|

证据：

- `animation_owner`：J273 VA 0x404390（RVA 0x4390）SetLayerFrame；VA 0x4040B0（RVA 0x40B0）AdvanceNullLayerPosition；官方 AnimationState.zhl；SetLayerFrame VA0x40439E取state+0 owner，VA0x4043A0访问owner+0x80层数
- `active_animation`：J273 VA 0x404390（RVA 0x4390）SetLayerFrame；VA 0x4040B0（RVA 0x40B0）AdvanceNullLayerPosition；官方 AnimationState.zhl
- `layer_frames`：docs/design/binary-state/grid-eight-subclasses-evidence.md
- `null_layer_frames`：docs/design/binary-state/grid-eight-subclasses-evidence.md
- `anim_frame`：docs/design/binary-state/grid-eight-subclasses-evidence.md
- `is_playing`：J273 VA 0x404390（RVA 0x4390）SetLayerFrame；VA 0x4040B0（RVA 0x40B0）AdvanceNullLayerPosition；官方 AnimationState.zhl；REPENTOGON/Patches/InterpolV2.cpp:34读取_isPlaying；版本32复核见docs/design/binary-state/animation-resource-synchronization-review.md
- `unknown_15_17`：AnimationState.zhl 在 bool +0x14 后对齐至 i32 +0x18；未证明这些字节永不变化
- `currently_triggered_events`：J273 AnimationState::IsEventTriggered 读取 this+0x18（imagebase 0x400000）；官方 AnimationState.zhl；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md
- `previously_triggered_events`：J273 AnimationState::WasEventTriggered 读取 this+0x1C（imagebase 0x400000）；官方 AnimationState.zhl；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md
