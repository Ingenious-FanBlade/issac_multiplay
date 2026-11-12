# GenericPromptResult

由唯一布局源生成。布局编号 380；布局版本 71；原生大小 0x0；未知 0B；完整：否。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：共享Prompt显示实例 | subject：world／显示代次 | producer：正常已完成Prompt纸张／文字消费 | adapter：generic_prompt_fixed_adapter | dependencies：同world显示代次、实际文字／有限样式、paper本地资源；不依赖Host文字图。 | dependency_types：PresentationDrawClock | sequence：world | atomic_scope：world-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：五槽实际文字／字体与安全本地生成仍待第10／13步接线；缺源报Gap，不能读旧图或调用提交／Lua补值。 | envelope：{'mode': 'group', 'subject_type': 'world', 'cacheable': True, 'clock': True, 'topology': False, 'paired_streams': 0, 'passes': ['Context', 'Value'], 'subject_field': ''} |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤1048576B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—1048576B，不含第 9 步记录头）：

块 0：GenericPromptResult具名当前结果（world，1—8923277B）：

| +0x0 u8（C/S自有元数据，无原生偏移） present：只包括共享world Prompt；个人菜单Prompt不发送。 | +0x1 DisplayTextRun（见该类型状态表）（伴随源，无根对象偏移）（仅genericpromptresult_present_is_one(present)成立时存在） text_0：固定文字槽0，前二标题／后三说明；保留缺席与实际空串；字体／颜色由合法来源封存。 | 顺序变长偏移 DisplayTextRun（见该类型状态表）（伴随源，无根对象偏移）（仅genericpromptresult_present_is_one(present)成立时存在） text_1：固定文字槽1，前二标题／后三说明；保留缺席与实际空串；字体／颜色由合法来源封存。 | 顺序变长偏移 DisplayTextRun（见该类型状态表）（伴随源，无根对象偏移）（仅genericpromptresult_present_is_one(present)成立时存在） text_2：固定文字槽2，前二标题／后三说明；保留缺席与实际空串；字体／颜色由合法来源封存。 | 顺序变长偏移 DisplayTextRun（见该类型状态表）（伴随源，无根对象偏移）（仅genericpromptresult_present_is_one(present)成立时存在） text_3：固定文字槽3，前二标题／后三说明；保留缺席与实际空串；字体／颜色由合法来源封存。 | 顺序变长偏移 DisplayTextRun（见该类型状态表）（伴随源，无根对象偏移）（仅genericpromptresult_present_is_one(present)成立时存在） text_4：固定文字槽4，前二标题／后三说明；保留缺席与实际空串；字体／颜色由合法来源封存。 | 顺序变长偏移 i32（C/S自有元数据，无原生偏移）（仅generic_prompt_result_present_present__1(present)成立时存在） selection：当前共享显示选择，不执行提交。 | 顺序变长偏移 ANM2（见该类型状态表）（伴随源，无根对象偏移）（仅generic_prompt_result_present_present__1(present)成立时存在） paper：共享Prompt当前纸张资源／姿态。 |
|---|---|---|---|---|---|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `paper` → `ANM2`：共享Prompt当前纸张资源／姿态。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
- `text_0` → `DisplayTextRun`：固定文字槽0，前二标题／后三说明；保留缺席与实际空串；字体／颜色由合法来源封存。；docs/design/binary-state/local-media-presentation.md；第9步版本61文字／本地历史合同（已核）
- `text_1` → `DisplayTextRun`：固定文字槽1，前二标题／后三说明；保留缺席与实际空串；字体／颜色由合法来源封存。；docs/design/binary-state/local-media-presentation.md；第9步版本61文字／本地历史合同（已核）
- `text_2` → `DisplayTextRun`：固定文字槽2，前二标题／后三说明；保留缺席与实际空串；字体／颜色由合法来源封存。；docs/design/binary-state/local-media-presentation.md；第9步版本61文字／本地历史合同（已核）
- `text_3` → `DisplayTextRun`：固定文字槽3，前二标题／后三说明；保留缺席与实际空串；字体／颜色由合法来源封存。；docs/design/binary-state/local-media-presentation.md；第9步版本61文字／本地历史合同（已核）
- `text_4` → `DisplayTextRun`：固定文字槽4，前二标题／后三说明；保留缺席与实际空串；字体／颜色由合法来源封存。；docs/design/binary-state/local-media-presentation.md；第9步版本61文字／本地历史合同（已核）

逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3336 | GenericPrompt.GetCurrentSelection | " " / "int" | derived → selection；共享Prompt最终选择外观／本地Prompt本地 | Yes/No高亮与提交命令分开；不能把Host选择写入访客本地确认框 |
| docs/design/tx-04-21/stream-catalog.json:3337 | GenericPrompt.GetSprite | " " / "Sprite" | derived → paper；GenericPrompt.paper | 共享world Prompt最终资源/姿态；本地菜单Prompt不上线，不能跨owner串用 |
