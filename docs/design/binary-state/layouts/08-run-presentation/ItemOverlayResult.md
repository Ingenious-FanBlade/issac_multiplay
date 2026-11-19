# ItemOverlayResult

由唯一布局源生成。布局编号 378；布局版本 87；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：ItemOverlay显示实例 | subject：world／必要身体锚点 | producer：984660正常Render分支的当前结果 | adapter：item_overlay_fixed_adapter | dependencies：同world实例与实际身体／资源；不执行984110。 | dependency_types：EntityPlayer, PresentationDrawClock | sequence：world | atomic_scope：world-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：正常来源缓存与运行adapter尚待第10／13步接线。 | envelope：{'mode': 'group', 'subject_type': 'world', 'cacheable': True, 'clock': True, 'topology': False, 'paired_streams': 2, 'passes': ['Context', 'Value'], 'subject_field': ''} |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤2097152B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

块 0：ItemOverlayResult具名当前结果（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：



逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3391 | ItemOverlay.GetMegaMushPlayerSprite | " " / "Sprite" | derived → mega_mush；ItemOverlay.mega_mush_player | 实际独立Sprite |
| docs/design/tx-04-21/stream-catalog.json:3392 | ItemOverlay.GetOverlayID | " " / "Giantbook" | field → overlay_id；ItemOverlay.overlay_id | 最终类型影响Render分支，不回放Show |
| docs/design/tx-04-21/stream-catalog.json:3393 | ItemOverlay.GetPlayer | " " / "EntityPlayer" | field → player；ItemOverlay.player | MegaMush锚点实际StateRef |
| docs/design/tx-04-21/stream-catalog.json:3394 | ItemOverlay.GetSprite | " " / "Sprite" | derived → sprite；ItemOverlay.sprite | 实际主Sprite |
