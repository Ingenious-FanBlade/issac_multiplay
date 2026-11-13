# SoundEffects

由唯一布局源生成。布局编号 337；布局版本 75；原生大小 0x18；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

SFXManager.zhl的size0是空壳声明；Manager+2A160..2A177实际SoundEffects18B，下一字段OptionsConfig+2A178。 修订4另核+14为modifier恢复步长；直属UNKNOWN4→0B。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 vector<SoundEffect> sounds【本地】实际SFXManager底层类型（12B；已核） | 0xC i32 load_count【本地】加载计数（4B；已核） | 0x10 f32 volume_modifier【权威】Play/Adjust/ClearVolumeModifier消费的乘数；全部world/本地writer分类未闭合（4B；已核） | 0x14 f32 modifier_recovery_step【权威】Manager92EB03读取；未暂停时令+10向1恢复的步长，92A19E构造初值0.005（4B；已核） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `sounds`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/SoundEffects.zhl/Manager.zhl；J273 907C60
- `load_count`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/SoundEffects.zhl/Manager.zhl；J273 907C60
- `volume_modifier`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/SoundEffects.zhl/Manager.zhl；J273 907C60
- `modifier_recovery_step`：929E80构造92A194/92A19E；正常Manager Update92EAE4—92EB3E；presentation-audio-sources.md

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3500 | SFXManager.AdjustPitch | " SoundEffect ID, float Pitch " / "void" | authority → ；Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| docs/design/tx-04-21/stream-catalog.json:3501 | SFXManager.AdjustVolume | " SoundEffect ID, float Volume " / "void" | authority → ；Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| docs/design/tx-04-21/stream-catalog.json:3502 | SFXManager.GetAmbientSoundVolume | " SoundEffect ID " / "float" | not_applicable → ；本地音频后端／具名持续voice目标视图 | 后端是否播放/音量含本地设置，不要求Host原始查询结果相等；持续声音目标另同步 |
| docs/design/tx-04-21/stream-catalog.json:3503 | SFXManager.IsPlaying | " SoundEffect ID " / "boolean" | not_applicable → ；本地后端结果；world逻辑存续另有来源 | 不传播后端pending/设备失效/偷占成全体Stop；world用途与退休来源待核 |
| docs/design/tx-04-21/stream-catalog.json:3504 | SFXManager.Play | " SoundEffect ID, float Volume = 1, int FrameDelay = 2, boolean Loop = false, float Pitch = 1, float Pan = 0 " / "void" | authority → ；Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| docs/design/tx-04-21/stream-catalog.json:3505 | SFXManager.Preload | " SoundEffect ID " / "void" | authority → ；Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| docs/design/tx-04-21/stream-catalog.json:3506 | SFXManager.SFXManager | " " / "SFXManager" | authority → ；Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| docs/design/tx-04-21/stream-catalog.json:3507 | SFXManager.SetAmbientSound | " SoundEffect ID, float Volume, float Pitch " / "void" | authority → ；Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| docs/design/tx-04-21/stream-catalog.json:3508 | SFXManager.Stop | " SoundEffect ID " / "void" | authority → ；Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
| docs/design/tx-04-21/stream-catalog.json:3509 | SFXManager.StopLoopingSounds | " " / "void" | authority → ；Host正常SFX控制／本地后端执行adapter | 不回放采集命令；已选sample事件及loop/ambient当前voice集合分别承接，FrameDelay不是启动延迟 |
