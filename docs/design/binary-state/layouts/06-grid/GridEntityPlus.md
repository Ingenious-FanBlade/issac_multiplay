# GridEntityPlus

由唯一布局源生成。布局编号 229；布局版本 89；原生大小 0xC；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

版本54：仅登记官方1.1.3 DLL外部旁表元素的12B ABI，不是第19个Grid RTTI，不在Grid根范围内，不作为companion或独立下行记录。所有字节归属已核、0B UNKNOWN；没有任何发送块/字段。Grid199已有4B最终water_clip_flags承接真实展示需求，各Grid根UNKNOWN不变。官方六水面hook不含Lock，Client必须另补Lock最终输出覆盖点；实际Host绑定/reader及Clientwriter仍归第10/13步，第15步前无测试。

原生字节归属：

| 0x0 u32 vtable【本地】官方模块的本地虚表；静态数组CRT按真实构造函数建立，读者不得替换/析构/发地址。不是Grid对象+0或其中的holder。（4B；已核） | 0x4 u32 water_override_value【权威】Host旁表覆盖输入，仅+8 engaged为1时读取；未设置时payload不读不清零，不能单独上线。六个官方hook用它替换最终bitFlags；Lock完全不消费。（4B；已核） | 0x8 bool water_override_engaged【权威】optional是否设置，不是payload非零；0为未覆盖，1包括有效覆盖0。只用于Host选择最终水面结果；Client per-instance直接保存最终u32，不复制此标记。（1B；已核） | 0x9 bytes[3] optional_alignment【填充】optional<uint32_t>尾部对齐字节；uint32对齐4、payload4+bool1后补3，slot stride12，后无其它成员。setter可能连同标志写入无语义高24位，不能按整个DWORD比较或同步，不据旧内容构造状态。（3B；已核） |
|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `vtable`：官方GridEntityPlus.h:8-12；官方1.1.3 DLL ctor RVA261C00写vptr=module+5112C4，CRT RVA2A450按448项/12B构造
- `water_override_value`：官方LuaGridEntity.cpp:57-74；DLL setter RVA1174A8、handle261CEF及六hook261E28等；docs/design/binary-state/grid-plus-water-source-evidence.md
- `water_override_engaged`：DLL ctor261C08/reset1173B6只清本字节；handle261CE6先比较本字节，随后读+4；MSVC optional值/标志布局
- `optional_alignment`：GridEntityPlus.h只有vptr和optional<uint32_t>；MSVC include/optional:70-75；DLL getter261CA4/261CA7按12B寻址，setter1174AB写DWORD但六消费者只读BYTE+8
