# NightmareSnapshot

由唯一布局源生成。布局编号 350；布局版本 58；原生大小 0x20；未知 0B；完整：是。

只说明该32B记录的存储和用途，不宣称所指实体/配置对象内部完整。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 std::string config_text【权威】正常Show保存的配置字符串（24B；已核） | 0x18 pointer source0【本地】第一个已选显示来源对象/配置；kind由实际Item/Boss slot决定（4B；已核） | 0x1C pointer source1【本地】第二个已选显示来源对象/配置；含实体专用Sprite路径（4B；已核） |
|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `config_text`：8FC916分配20h；8FC932复制24B string
- `source0`：8FC93D；8FAF87；8FB0E3—8FB178
- `source1`：8FC940；8FAF87；8FB093—8FB0D6
