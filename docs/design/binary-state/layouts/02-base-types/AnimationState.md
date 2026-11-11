# AnimationState

由唯一布局源生成。布局编号 5；布局版本 60；原生大小 0x20；未知 3B；完整：否。

两个帧数组是独立选择器，不是 _animFrame 的派生值。SetLayerFrame 与 NullFrame 路径分别校验并更新数组；长度来自当前 AnimationData 的 layer_count/null_layer_count，不能使用 ANM2 总层数。 版本32：两组事件位归Host，块3显式预留；is_playing与anim_frame保留，官方InterpolV2.cpp绘制插值读取两者，不以Client停用Update为由删除。

原生字节归属：

| 0x0 ANM2* animation_owner【本地】回指所属 ANM2；由上层对象关系确定（4B；已核） | 0x4 AnimationData* active_animation【资源】当前动画资源项；序号由 ANM2 文件表解析，空指针使用资源空值标记（4B；已核） | 0x8 int* layer_frames【容器】各图层 AnimationFrame 内部序号选择器，长度取当前 AnimationData._layerCount（4B；已核） | 0xC int* null_layer_frames【容器】各空层 AnimationFrame 内部序号选择器，长度取当前 AnimationData._nullLayerCount（4B；已核） | 0x10 f32 anim_frame【状态】动画当前帧值（4B；已核） | 0x14 bool is_playing【状态】播放标志兼绘制插值输入；直接安装最终值，不授权Client推进动画（1B；SDK声明） | 0x15 bytes[3] unknown_15_17【未知】SDK 字段间未解释的三个字节；不认作填充（3B；未知） | 0x18 i32 currently_triggered_events【权威】Host动画事件消费位；Client安装最终姿态，不推进或重放事件，构造／换动画后本地清零（4B；已核） | 0x1C i32 previously_triggered_events【权威】Host动画事件历史位；不表示可靠事件队列，不进入Client状态或漂移比较（4B；已核） |
|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 11—524291B，不含第 9 步记录头）：

块 0：动画选择（body，2—2B）：

| +0x0 u16 active_animation：当前动画资源项；序号由 ANM2 文件表解析，空指针使用资源空值标记 |
|---|

块 1：帧选择器（body，4—524284B）：

| +0x0 u16 count + count×i32（上限65535） layer_frames：各图层 AnimationFrame 内部序号选择器，长度取当前 AnimationData._layerCount | 顺序变长偏移 u16 count + count×i32（上限65535） null_layer_frames：各空层 AnimationFrame 内部序号选择器，长度取当前 AnimationData._nullLayerCount |
|---|---|

块 2：播放位置（body，5—5B）：

| +0x0 f32 anim_frame：动画当前帧值 | +0x4 u8 is_playing：播放标志兼绘制插值输入；直接安装最终值，不授权Client推进动画 |
|---|---|

块 3：原事件掩码（预留）（body，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：

- `animation_owner`：J273 VA 0x404390（RVA 0x4390）SetLayerFrame；VA 0x4040B0（RVA 0x40B0）AdvanceNullLayerPosition；官方 AnimationState.zhl；SetLayerFrame VA0x40439E取state+0 owner，VA0x4043A0访问owner+0x80层数
- `active_animation`：J273 VA 0x404390（RVA 0x4390）SetLayerFrame；VA 0x4040B0（RVA 0x40B0）AdvanceNullLayerPosition；官方 AnimationState.zhl
- `layer_frames`：J273 VA 0x404390 SetLayerFrame 使用 state+0x08 并按当前动画层映射写选择器；AnimationState.zhl 注释；数组不可从 _animFrame 重建
- `null_layer_frames`：J273 VA 0x4040B0 AdvanceNullLayerPosition 读写 state+0x0C；计数从当前 AnimationData+0x24 读取；数组不可从 _animFrame 重建
- `anim_frame`：J273 AdvancePosition VA0x4041A0/RVA0x41A0：VA0x4041B2以movss读+0x10、VA0x404239写floor(frame)+1；主审已复读
- `is_playing`：J273 VA 0x404390（RVA 0x4390）SetLayerFrame；VA 0x4040B0（RVA 0x40B0）AdvanceNullLayerPosition；官方 AnimationState.zhl；REPENTOGON/Patches/InterpolV2.cpp:34读取_isPlaying；版本32复核见docs/design/binary-state/animation-resource-synchronization-review.md
- `unknown_15_17`：AnimationState.zhl 在 bool +0x14 后对齐至 i32 +0x18；未证明这些字节永不变化
- `currently_triggered_events`：J273 AnimationState::IsEventTriggered 读取 this+0x18（imagebase 0x400000）；官方 AnimationState.zhl；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md
- `previously_triggered_events`：J273 AnimationState::WasEventTriggered 读取 this+0x1C（imagebase 0x400000）；官方 AnimationState.zhl；版本32同步职责复核：docs/design/binary-state/animation-resource-synchronization-review.md
