# Font

由唯一布局源生成。布局编号 338；布局版本 84；原生大小 0x4C；未知 14B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

Font不是唯一全局单例：Manager内嵌九份，Lua还可构造独立实例；不能跨owner套用字体。 原生字节事实保留；委派范围按本表具名结果承接，不按原生owner全量复制。

原生字节归属：

| 0x0 bool loaded【本地】本地字体准备结果（1B；已核） | 0x1 bytes[3] unknown_1_3【未知】未取得字段用途或填充证明（3B；未知） | 0x4 i16 size【派生】匹配.fnt资源中的字体度量/定义（2B；SDK声明） | 0x6 u8 info_flags【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0x7 u8 charset【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0x8 u16 stretch【派生】匹配.fnt资源中的字体度量/定义（2B；SDK声明） | 0xA u8 aa【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0xB u8 padding_up【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0xC u8 padding_right【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0xD u8 padding_down【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0xE u8 padding_left【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0xF u8 spacing_x【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0x10 u8 spacing_y【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0x11 u8 outline【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0x12 bytes[2] unknown_12_13【未知】未取得字段用途或填充证明（2B；未知） | 0x14 u16 line_height【派生】匹配.fnt资源中的字体度量/定义（2B；SDK声明） | 0x16 u16 baseline【派生】匹配.fnt资源中的字体度量/定义（2B；SDK声明） | 0x18 u16 scale_w【派生】匹配.fnt资源中的字体度量/定义（2B；SDK声明） | 0x1A u16 scale_h【派生】匹配.fnt资源中的字体度量/定义（2B；SDK声明） | 0x1C u16 pages【派生】匹配.fnt资源中的字体度量/定义（2B；SDK声明） | 0x1E u8 common_flags【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0x1F u8 alpha_channel【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0x20 u8 red_channel【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0x21 u8 green_channel【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0x22 u8 blue_channel【派生】匹配.fnt资源中的字体度量/定义（1B；SDK声明） | 0x23 bytes[1] unknown_23_23【未知】未取得字段用途或填充证明（1B；未知） | 0x24 u32 char_count【派生】匹配.fnt资源中的字体度量/定义（4B；SDK声明） | 0x28 FontCharBlock* chars【本地】字体字符资源数组（4B；已核） | 0x2C vector<FontKerningPair> kerning【本地】字体字距资源（12B；已核） | 0x38 bytes[8] unknown_38_3F【未知】未取得字段用途或填充证明（8B；未知） | 0x40 map<char,int> char_map【本地】字符索引查找表（8B；已核） | 0x48 i32 missing_char【本地】这个Font实例的缺字替换选择（4B；SDK声明） |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

| 无发送字节：该类型没有可复制状态 |
|---|

证据：

- `loaded`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `unknown_1_3`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `size`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `info_flags`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `charset`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `stretch`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `aa`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `padding_up`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `padding_right`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `padding_down`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `padding_left`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `spacing_x`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `spacing_y`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `outline`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `unknown_12_13`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `line_height`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `baseline`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `scale_w`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `scale_h`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `pages`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `common_flags`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `alpha_channel`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `red_channel`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `green_channel`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `blue_channel`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `unknown_23_23`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `char_count`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `chars`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `kerning`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `unknown_38_3F`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `char_map`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680
- `missing_char`：F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/Font.zhl/Global.zhl；原版/官方Wiki Font.md；J273 9E8D20/9E9680

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3197 | Font.DrawString | " string String, float PositionX, float PositionY, KColor RenderColor, int BoxWidth = 0, boolean Center = false " / "void" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3198 | Font.DrawString | " string String, float PositionX, float PositionY, float sizeX, float sizeY, KColor RenderColor, FontRenderSettings settings " / "void" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3199 | Font.DrawStringScaled | " string String, float PositionX, float PositionY, float ScaleX, float ScaleY, KColor RenderColor, int BoxWidth = 0, boolean Center = false " / "void" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3200 | Font.DrawStringScaledUTF8 | " string String, float PositionX, float PositionY, float ScaleX, float ScaleY, KColor RenderColor, int BoxWidth = 0, boolean Center = false " / "void" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3201 | Font.DrawStringUTF8 | " string String, float PositionX, float PositionY, KColor RenderColor, int BoxWidth = 0, boolean Center = false " / "void" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3202 | Font.Font | " " / "Font" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3203 | Font.Font | " string FontPath " / "Font,boolean" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3204 | Font.GetBaselineHeight | " " / "int" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3205 | Font.GetCharacterWidth | " char Character " / "int" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3206 | Font.GetLineHeight | " " / "int" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3207 | Font.GetStringWidth | " string String " / "int" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3208 | Font.GetStringWidthUTF8 | " string String " / "int" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3209 | Font.IsLoaded | " " / "boolean" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3210 | Font.Load | " string FilePath " / "void" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3211 | Font.SetMissingCharacter | " char MissingCharacter " / "void" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
| docs/design/tx-04-21/stream-catalog.json:3212 | Font.Unload | " " / "void" | not_applicable → ；本地匹配字体资源／安全文字绘制调用 | Font有多个owner；度量、loaded由本地资源准备/查询，不要求Host原始字节相等；声明UI的内容/资源recipe另有owner |
