# LevelASM

由唯一布局源生成。布局编号 279；布局版本 87；原生大小 0x4；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

没有核定官方DLL全局RVA；不按Game偏移采集。 原始静态证据保持；运行reader／writer尚待第10/13步接线。

原生字节归属：

| 0x0 i32 force_special_quest【权威】官方对象外生成参数，level init前重置；同步决定：留Host；不作为Client状态或操作命令；仅同步具名最终显示／结构结果（4B；已核） |
|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `force_special_quest`：repentogon/LuaInterfaces/Level.h与LuaLevel.cpp；ASMLevel.cpp
