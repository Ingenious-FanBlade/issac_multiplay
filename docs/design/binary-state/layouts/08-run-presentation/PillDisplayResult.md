# PillDisplayResult

由唯一布局源生成。布局编号 359；布局版本 58；原生大小 0x0；未知 0B；完整：是。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：身体Pocket显示 | subject：body＋固定槽／用途 | producer：8378C0→83830A／838320正常身体相关分支及实际图像 | adapter：pill_display_fixed_adapter | dependencies：身体key/use/shape、对应Pocket槽、成员／资源与P0—P6必要绘制拓扑；不要求完整地图。 | dependency_types：EntityPlayer | sequence：body | atomic_scope：body-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：逐身体／槽位正常结果缓存待接线；GetPillColor本身无原生显示消费者证明，不执行它补值。 |
|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤13B；body组≤65536B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 1—13B，不含第 9 步记录头）：

块 0：PillDisplayResult具名当前结果（body，1—13B）：

| +0x0 u8（C/S自有元数据，无原生偏移） state：0=正常无药丸显示；1=未知；2=该身体／槽已揭示；缺源不编码0。 | +0x1 i32（C/S自有元数据，无原生偏移）（仅pill_display_result_state_state__0(state)成立时存在） color：实际普通／马药外观标识，不据此查ItemPool。 | 顺序变长偏移 i32（C/S自有元数据，无原生偏移）（仅pill_display_result_state_state__2(state)成立时存在） effect：真实身体修饰后已揭示effect；未知分支不读不发送。 | 顺序变长偏移 u16（C/S自有元数据，无原生偏移）（仅pill_display_result_state_state__0(state)成立时存在） name_image：实际名称／???最终图像；空与资源未就绪分开。 | 顺序变长偏移 u16（C/S自有元数据，无原生偏移）（仅pill_display_result_state_state__0(state)成立时存在） pill_image：实际药丸外观图像；保持马药与未知外观。 |
|---|---|---|---|---|

证据：



逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3414 | ItemPool.GetPillColor | " PillEffect ID " / "PillColor" | derived → color；Presentation actual body/Pocket subject/use/shape/slot result | 不发基础映射/鉴定表；真实身体及槽位的揭示、effect和名称/图像结果须有合法来源；禁止额外Getter/回调/随机补值。逆查GetPillColor自身不构成原生显示消费证据 |
| docs/design/tx-04-21/stream-catalog.json:3415 | ItemPool.GetPillEffect | " PillColor PillColor, EntityPlayer Player = nil " / "PillEffect" | derived → effect；Presentation actual body/Pocket subject/use/shape/slot result | 不发基础映射/鉴定表；真实身体及槽位的揭示、effect和名称/图像结果须有合法来源；禁止额外Getter/回调/随机补值。逆查GetPillColor自身不构成原生显示消费证据 |
| docs/design/tx-04-21/stream-catalog.json:3423 | ItemPool.IsPillIdentified | " PillColor PillColor " / "boolean" | derived → state；Presentation actual body/Pocket subject/use/shape/slot result | 不发基础映射/鉴定表；真实身体及槽位的揭示、effect和名称/图像结果须有合法来源；禁止额外Getter/回调/随机补值。逆查GetPillColor自身不构成原生显示消费证据 |
