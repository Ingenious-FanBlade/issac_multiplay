# Q7：实际路径的显示作用域与 C/S 回放

日期：2026-11-12。纯静态研究及实现交接，尚未运行。

## 1. 同一份实际执行表

Host 显示作用域和 Client 放行／拆分表必须来自同一清单。候选表中的“将来希望运行”不算已执行；未接通／被 Client 拦截的函数不能在 Host 上先屏蔽其声音。作用域使用主线程可嵌套状态，离开时恢复进入前值，不能全局 bool 开关或只看回溯地址。

| 路径 | Host SFX 采集判定 |
| --- | --- |
| 已放行 Game::Render 全树、HUD／地图／相机等纯显示子调用 | 进入显示作用域；内部 SFX 由 Client 同路径本地播放，不记事件 |
| 实体／玩法 Update、伤害、拾取、道具使用、完整 ItemOverlay::Update | 不进入显示作用域，实际接受的 world SFX 进入事件／循环状态 |
| RoomTransition 原生 Start／Update／ChangeRoom | **不能整根算显示**。RE-9 已发现直接 Player 写入、EntityList／Player Update、环境声和换房。只对白名单拆出的显示子路径进入作用域 |
| Host 的 ChangeRoom→Game／Level／RoomInit、重生／传送／玩法子调用 | 在显示父范围内也要暂时暂停显示作用域；Client 不执行这些路径，因此其真实玩法声音必须采集 |
| C/S 事件回放 | Replay 范围优先于显示范围，播放但不再次采集，不自动归为“未经授权的本机声音” |
| 菜单、连接提示、本机 Options／设备恢复 | 按现有用途归本地，不从“显示作用域外”自动推导为 world 事件 |

后端启动成功不是 world 意图的充分来源，official POST_SFX 回调也不是成功通知；复用正常节流／最终参数采集点。Host 未播放（静音／设备失败）不等于 Client 应静音，反之重传不得绕过去重制造新声音。

既有正常 Play 的资格／节流段在 `0x907D2F—0x907D5A`，选 sample 在 `0x907D5D—0x907DC0`，最终 pan 在 `0x907EAE`；这些窗口用于观察已经通过正常逻辑判定的参数，不额外调用 Play 探测。Client 回放到达时间可能把 Host 分开的合法事件压到同一本机帧：若再次原样套用本机 FrameDelay，就可能二次节流漏音。后续 adapter 须区分已接受事件与本地普通 Play 的节流账本，在具名回放范围内按已核窄路径处理；不得以 void 返回视为播放成功，也不能全局关闭本地声音节流。该回放适配尚未实施。

## 2. 已核 SFX callsite

| J273 VA | 来源／语义 | 新方案职责 |
| --- | --- | --- |
| `0x97AE75→0x907C60` | HUD::Update 滑入／滑出；初始 sound252 经 `0x92FAD0` 处理 | 已放行显示，Client 本地播放，Host 排除事件；不能发送原 id 又让 Client 再转换 |
| `0x984296／0x984352／0x98447B／0x9845E7` | ItemOverlay::Update 的启动／动画事件音效 | 完整 Update 在 Client 不运行，Host 采集实际最终声音；不因 Render 在树内而排除 |
| `0x823CE1` | RoomTransition::Start 的 animation18；原始 id608／609 由目标 dimension 条件选择，再经 `0x92FAD0` | Client 若具名显示启动分支执行同一声音 leaf，才在 Host 对该 leaf 排除；否则走事件。整根 Start 的 Player 写入不因此放行 |
| `0x826D79` | Update mode3、animation7 的初始 sound1 经转换 | 只有 Client 实际保留同一纯声音子段时归本地，否则 Host 事件 |
| `0x826FBA` | Update mode4、animation3、counter==8 的初始 sound214 经转换 | 同上；等待／跳帧若改变本地触发次数，不能继续假设两边一定等次，须以实际拆分策略保持单一 owner |

本轮对 `Game::Render 0x6F7A80` 作**最多5层直接 call**的静态追踪，检查589个函数，未命中对 `0x907C60` 的直接调用链；Game::Render、Minimap／FX／相机等已查根也没有直接 Play。这个结果不包含更深层、虚调用、官方 DLL／Lua 动态调用，**不是整棵 Render 树绝无音效的证明**。Q7 使用整个实际显示作用域，后续 E37 观测中发现的显示音效自然仍按本地处理，不需要为了排除它重新穷尽 Render 静态闭包。

`RoomTransition::StartBossIntro 0x823D10` 的 `0x823D8C` 是 **Music::Play(81)**，不是 SFXManager::Play；Q7 的 SFX 规则不能替代音乐请求状态的去重。当前 SFX／音乐 API 和暂停范围见[采集与播放表](sources-and-payloads.md)。

## 3. 回放标志的位置

“回放”不是原生 SFX 的第七个参数，也不是网络 bool 直接控制任意原生行为。实现放在 C/S 主线程播放 adapter 的栈式上下文中：进入前保存旧的 replay 深度／事件身份，调用合法本机原生 Play，离开后恢复；网络线程只排队。嵌套官方回调中的 Play 仍可识别为同次 C/S 回放，不能再次产生 Host／Client 上行事件。

判定顺序：有效会话与 owner／线程 → C/S replay → 实际显示作用域 → 已核 world 玩法来源 → 本地／未知用途。未知来源先记具体缺口并停止相关采集，不把整个进程所有声音广播。官方回调分派按 Q6 继续执行，本项目 Client Lua 不注册玩法回调；本轮不重建逐 callback 取消表。

显示范围在混合子调用处暂停后，返回必须恢复原深度；早退／异常同样需要正常清理。生命周期／线程不确定时不继续调用世界或声音对象；会话结束先停 publication 和回放，排空在途，再退休本方循环／音乐状态，不恢复 Hook 字节。

## 4. 第15步交接

E37 检查 HUD252 本地一次、ItemOverlay 玩法声音事件一次、Render 新发现的声音、三个转场声音分支、同音效多实例、同曲重播／queued、循环空集合／重连、文字去重及跨语言。E39 检查换房等待／断线时作用域不泄漏，Host 被替换的子调用声音不丢。所有测试尚未执行；容量溢出、事件过期、资源未就绪、原生调用失败、设备丢失分别记录，不用“已回放”替代真实提交和用户可见结果。
