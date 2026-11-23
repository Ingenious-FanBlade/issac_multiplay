# PoolItem

由唯一布局源生成。布局编号 392；布局版本 94；原生大小 0x18；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

 原生地址／guard只作证据，不成为线上身份；本类型无发送块。

原生字节归属：

| 0x0 u32 item_id【权威】候选item身份；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） | 0x4 f32 initial_weight【权威】初始权重；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） | 0x8 f32 weight【权威】当前权重；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） | 0xC f32 decrease_by【权威】抽选后权重递减量；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） | 0x10 f32 remove_on【权威】移除阈值；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（4B；SDK声明） | 0x14 bool is_unlocked【权威】Host候选资格；不能写访客进度；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（1B；SDK声明） | 0x15 bool is_special【权威】Host特殊候选标志；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（1B；SDK声明） | 0x16 bytes[2] declared_pad【填充】官方源码显式_pad[2]；仅保留声明等级；Host随机／池／恢复来源不发送；实际药丸、预览、alpha、地图及音频结果各归Presentation唯一owner。（2B；SDK声明） |
|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `item_id`：Global.zhl::PoolItem；J273730F10/730C70实际步长24
- `initial_weight`：Global.zhl::PoolItem；J273730F10/730C70实际步长24
- `weight`：Global.zhl::PoolItem；J273730F10/730C70实际步长24
- `decrease_by`：Global.zhl::PoolItem；J273730F10/730C70实际步长24
- `remove_on`：Global.zhl::PoolItem；J273730F10/730C70实际步长24
- `is_unlocked`：Global.zhl::PoolItem；J273730F10/730C70实际步长24
- `is_special`：Global.zhl::PoolItem；J273730F10/730C70实际步长24
- `declared_pad`：Global.zhl::PoolItem；J273730F10/730C70实际步长24
