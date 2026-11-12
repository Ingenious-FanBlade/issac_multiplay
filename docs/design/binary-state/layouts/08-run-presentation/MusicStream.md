# MusicStream

由唯一布局源生成。布局编号 330；布局版本 71；原生大小 0x34；未知 4B；完整：否。

修订4撤销旧+0C=pitch及+2C播放游标推断；直属UNKNOWN24→4B，后端对象ABI另计。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 SoundSourceBase* audio_source【本地】本地音频source强引用的对象指针（4B；已核） | 0x4 ReferenceCount* audio_control【本地】强引用pair的本地control；9F76E0/9F7CA0建立（4B；已核） | 0x8 bool loop【权威】当前source循环请求；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（1B；已核） | 0x9 bool pending_play【本地】Play提交后置1，正常后端playing查询可清零；并非已出声（1B；已核） | 0xA bool paused【权威】正常Pause/Resume维护的source暂停标志；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（1B；已核） | 0xB bytes[1] unknown_B_B【未知】未取得字段用途或填充证明（1B；未知） | 0xC f32 backend_volume【本地】后端音量缓存；旧pitch误名已撤销（4B；已核） | 0x10 f32 pan【权威】当前pan请求/缓存；非mono有效分支另核；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（4B；已核） | 0x14 f32 pitch【权威】真实source pitch；7D5C40写+14并调用vtable+58；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（4B；已核） | 0x18 f32 config_gain【权威】音乐/分层配置mul，参与最终混音（4B；已核） | 0x1C f32 layer_gain【权威】当前声部权重，不是后端最终volume（4B；已核） | 0x20 f32 target_layer_gain【权威】声部权重目标；由layer mask等生产（4B；已核） | 0x24 f32 layer_fade_rate【权威】当前声部权重平滑速率（4B；已核） | 0x28 bool active【本地】流活动维护标志；不等价于后端已出声（1B；已核） | 0x29 bytes[3] unknown_29_2B【未知】未取得字段用途或填充证明（3B；未知） | 0x2C u32 update_count【本地】7D7B90正常Update计数；>10才查询后端活动状态（4B；已核） | 0x30 MusicStreamSet* parent_set【本地】最终gain读取父组+174；无父时不乘组gain（4B；已核） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `audio_source`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `audio_control`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `loop`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `pending_play`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `paused`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `unknown_B_B`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `backend_volume`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `pan`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `pitch`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `config_gain`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `layer_gain`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `target_layer_gain`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `layer_fade_rate`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `active`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `unknown_29_2B`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `update_count`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `parent_set`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
