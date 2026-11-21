# MusicEntry

由唯一布局源生成。布局编号 332；布局版本 90；原生大小 0x60；未知 2B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

修订4据7D60A0解析/实际存储及7D5CC0消费者补证；直属UNKNOWN72→2B。长解析函数bad-SP告警不作异常ABI闭合。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 u32 config_id【本地】XML曲目ID，解析写入（4B；已核） | 0x4 std::string name【本地】XML曲目名称（24B；已核） | 0x1C std::string intro【权威】已解析intro文件；存在时先播并排入main；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（24B；已核） | 0x34 std::string main【权威】已解析主文件路径；+44是该string长度成员；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（24B；已核） | 0x4C bool loop【权威】主文件loop配置；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（1B；已核） | 0x4D u8 layer_mode【权威】分层混音模式；模式2可抑制主声部（1B；已核） | 0x4E bytes[2] unknown_4E_4F【未知】未取得字段用途或填充证明（2B；未知） | 0x50 f32 gain【权威】XML mul；复制到主流+18（4B；已核） | 0x54 f32 layer_fade_speed【权威】XML layerfadespeed；复制到流+24（4B；已核） | 0x58 MusicLayerDefinition* layer_defs【本地】已分配34h步长层定义数组；含new[]计数头（4B；已核） | 0x5C u32 layer_count【本地】配置层数；消费端只处理6物理layer槽（4B；已核） |
|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `config_id`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `name`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `intro`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `main`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `loop`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `layer_mode`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `unknown_4E_4F`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `gain`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `layer_fade_speed`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `layer_defs`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `layer_count`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
