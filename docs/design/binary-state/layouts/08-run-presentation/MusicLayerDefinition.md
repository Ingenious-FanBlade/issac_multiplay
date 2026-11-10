# MusicLayerDefinition

由唯一布局源生成。布局编号 333；布局版本 58；原生大小 0x34；未知 0B；完整：是。

音乐分层资源定义，不是MusicStream；两个24B字符串及mul，直属未知0B。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 std::string intro【权威】分层intro路径；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（24B；已核） | 0x18 std::string main【权威】分层main路径；具体下行职责见具名结果，不直接复制该范围。；不另发原始生产／重复负载，唯一归属：AudioVoiceParameters／AudioVoiceState／AudioWorldEvent；原生混音和选样本留Host（24B；已核） | 0x30 f32 gain【权威】分层mul，复制到对应MusicStream+18（4B；已核） |
|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `intro`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `main`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
- `gain`：本地Wiki MusicManager/SFXManager/xml/music/xml/sounds；官方Music.zhl/SoundEffects.zhl及LuaMusicManager.cpp；J273 7D5860/7D60A0/7D7A30/7D7B90/9070A0/907C60；presentation-audio-sources.md
