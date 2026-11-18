# AudioVoiceState

由唯一布局源生成。布局编号 382；布局版本 84；原生大小 0x0；未知 0B；需求覆盖：完整。

需求覆盖只表示清单字段已生成计划，不表示原生字节全部已知、Host/Client处理器已接通或第15步验收通过。旧成员映射与原生说明保留为参照，当前发送字段以下方状态表为准。

具名显示结果合同（接线声明，不表示运行实现）：

| owner：逻辑音频voice | subject：稳定voice实例／代次 | producer：907C60节流后已选sample意图；音乐正常Options前控制／混音结果 | adapter：audio_voice_state_adapter | dependencies：run／world逻辑用途、实际音频资源、实例／代次及事件水位；重复state不重启，baseline不补播历史one-shot。 | dependency_types： | sequence：world | atomic_scope：world-target | validity：同session/run、来源修订、owner代次及必要成员／资源／拓扑依赖均成立；内容不变不代表来源仍有效。 | invalidation：切房／换层／恢复、subject/use/shape或绑定变化、用途退休、资源替换按实际影响撤销；瞬态贡献不得跨来源窗口复用。 | contribution_identity：subject/use/shape＋固定用途＋pass＋branch/layer/track＋Host批内有序贡献身份；第9步登记外层精确封装，不用Client访问计数。 | source_gap：全部音乐Options前writer／逻辑包络、world用途退休、segment与seek恢复及视频音轨仍未闭合；必要来源不全则Gap，不从后端补成功／Stop。 | envelope：{'mode': 'group', 'subject_type': 'world', 'cacheable': True, 'clock': False, 'topology': False, 'paired_streams': 0, 'passes': ['Context', 'Value'], 'subject_field': ''} |
|---|---|---|---|---|---|---|---|---|---|---|---|---|

记录编码预算≤23B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 0—0B，不含第 9 步记录头）：

块 0：AudioVoiceState具名当前结果（world，0—0B）：

| 无发送字节：显式预留块 |
|---|

证据：



逐行公开目录回映射（来源目录＋一基行号；保留重载和原签名）：

| 来源行 | ID | parameters / returns（JSON原值） | 处置／正式来源 | 依据 |
|---|---|---|---|---|
| docs/design/tx-04-21/stream-catalog.json:3450 | MusicManager.GetCurrentJingleID | " " / "Music" | derived → parameters；实际world jingle资源/逻辑实例；无独立getter字段 | 官方ID可能由未生效调用更新；最终voice资源/事件承接，不单独发ID或倒计时 |
| docs/design/tx-04-21/stream-catalog.json:3451 | MusicManager.GetCurrentMusicID | " " / "Music" | derived → parameters；实际并存逻辑voice的资源引用 | 两个crossfade组可能并存；voice已有资源时不另发currentId Getter |
| docs/design/tx-04-21/stream-catalog.json:3454 | MusicManager.IsEnabled | " " / "boolean" | derived → parameters；world voice最终gain/pause/存续；本地Mute独立 | 不复制混合enabled字段；没有单独网络getter或覆写个人静音 |
