# SoundSample

由唯一布局源生成。布局编号 335；布局版本 58；原生大小 0x38；未知 4B；完整：否。

修订4直属UNKNOWN16→4B；资源加载/声音实例身份分开，sample+28为pan，+34为配置weight。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 std::string path【权威】已选sample文件元数据；实际加载解析路径另绑定；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（24B；已核） | 0x18 SoundSourceBase* audio_source【本地】本地source强引用对象指针（4B；已核） | 0x1C ReferenceCount* audio_control【本地】9F76E0/9F7CA0建立的本地强引用control（4B；已核） | 0x20 bool loop【权威】当前source循环请求；StopLooping还查后端有效loop；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（1B；已核） | 0x21 bool pending_play【本地】Play提交后置1；正常playing查询可清零（1B；已核） | 0x22 bool paused【本地】与通用source wrapper相同的暂停缓存（1B；已核） | 0x23 bytes[1] unknown_23_23【未知】未取得字段用途或填充证明（1B；未知） | 0x24 f32 backend_volume【本地】乘Host Options后后端音量；不同正常路径clamp不同（4B；已核） | 0x28 f32 pan【权威】9F7970及Room sound460内联路径写入；非mono可能不生效；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（4B；已核） | 0x2C f32 pitch【权威】当前source pitch；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（4B；已核） | 0x30 bool load_on_play【本地】1走Play内正常加载，0可预载；旧streaming名称撤销（1B；已核） | 0x31 bytes[3] unknown_31_33【未知】未取得字段用途或填充证明（3B；未知） | 0x34 i32 config_weight【权威】907AA6将XML weight的atoi结果写此；907C60选样本未读本字（4B；已核） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `path`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `audio_source`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `audio_control`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `loop`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `pending_play`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `paused`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `unknown_23_23`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `backend_volume`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `pan`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `pitch`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `load_on_play`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `unknown_31_33`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `config_weight`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
