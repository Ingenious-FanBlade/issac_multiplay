# 模组适配 E2E 夹具

用于[模组适配第 14 步](../../../docs/archive/design/mod-adaptation.md#第-14-步统一-e2e)。夹具与真实 SDK 通过公开接口交互，按正式 Mod 身份参加握手。产品包和标准 E2E 包使用各自的原有内容。

## 构建与选择

```powershell
pwsh -NoProfile -File scripts/build.ps1 -E2E
pwsh -NoProfile -File scripts/package.ps1 -ModAdaptation -OutputDirectory out/e2e-mod-adaptation
```

专用包内的 `mods/isaac-cs-mod-fixture` 随附产品包里的 SDK 薄壳 `isaaccs_sdk.lua`。做无联机层的单机对照时，只在 REPENTOGON 测试副本加载这个 Mod 目录，此时用的是薄壳的单机实现。安装联机层后在未建立会话时再做一次单机对照，此时用的是联机层的实现，两次结果应当一致。

双端可用现有 `isaac-dual.exe` 的 `--launch-only` 方式部署专用包。此包选择正式声明夹具，隔离副本内残留的旧玩法夹具由启动器禁用。需要切换各节点的 `fixture_config.lua` 时，在两端退出后改各自副本，再用同一启动器以已部署文件启动，或分别部署到两台隔离机。给同一次验收统一设置 `token`，为节点设置不同 `node`，并选择两份测试存档中解锁状态已知不同的 `achievement`。默认成就为死亡证明，默认过场是原生 ID 1。

## 控制

Host 或单机在局内控制台输入 `cs_mod_fixture <动作>`，然后关闭控制台。夹具在下一次普通游戏更新执行，日志先记 `action-queued`，再记 `action`。

| 动作 | 用途 |
| --- | --- |
| `spawn`、`remove` | 创建带两条独立声明的金币，删除并短暂保留旧引用，观察实体安装、移除及 Null |
| `bump` | 计分加 10，清空可选字段，删掉整数 map 的正键并改变字符串 map 构造顺序 |
| `mark`、`unmarked`、`cadence` | 显式标脏、只改 Host 值、记录三种频率的读取次数 |
| `read-error`、`wrong-type`、`too-many`、`long-string` | getter 抛错、返回函数、列表超项、字符串超字节 |
| `slow-read` | 只拖慢一次 World getter，记录该次耗时，并返回增加 1000 的合法计分 |
| `set-error`、`restore` | 让客机 setter 抛错，再用不同的合法值恢复。`restore` 也结束 Host 读失败注入 |
| `native`、`restore-look` | 申请自定义 cache 37.25、强制隐身，启用心上限和主动物品充能值覆盖，设置 Boss 血条填充值 0.375，然后解除本夹具的显示覆盖 |
| `floor`、`floor-config` | 依次测试 `Level:SetName` 与 `Isaac.SetCurrentFloorName`，后一项同时设置 Caves 背景供下一个房间对照 |
| `audio` | Host 请求换声、取消和改音乐，记录实际回调参数 |
| `cutscene`、`cutscene-clear` | 播放同一个原生过场，分别保留与结束本局。清场用例放到本局最后 |

| 本机键 | 用途 |
| --- | --- |
| `F4` | 输出本机当前 HUD、类型值、私有条目、实体、进度与原生显示快照 |
| `F5` | 等待在途结果回收后逐批发送，共尝试 24 条或达到 900 ms，检查每秒上限 |
| `F6`、`F7` | 请求成功计分或业务拒绝。实体已同步时，正常请求同时携带实体引用 |
| `F8`、`F9` | 同批提交 12 条检查 8 条在途上限，或提交 513 字节字符串检查参数拒绝 |
| `F10` | 客机在绘制回调里播放本地哔声 |
| `F11` | 客机故意改写已同步计分，开始观察恢复 |
| `K`、右移 | 在客机按住再松开，Host 在代表身体的 Player 更新回调读取扩展键和三种 `Input.*` 动作查询 |

## 第 14 步对照

日志前缀为 `[isaac-cs-mod-fixture]`，每行一个 JSON 对象。用 `token`、节点、角色、帧和本地 `serial` 关联。表中都是待观察项。

| 场景 | 对照证据 |
| --- | --- |
| 两种单机模式 | `loaded.native_sdk` 区分实现来源。`F6` 的 `intent-send.after` 当次比 `before` 多 3，`intent-result` 在不同帧出现；`F7` 返回业务失败 |
| 握手不一致 | 只改客机 `manifest = "different"` 时，目录、metadata 和版本相同，仅多一条声明，收集首个声明差异。改为 `"undeclared"` 时读取拒绝信息中的夹具名；此拒绝可以发生在本端生成清单阶段 |
| 所有声明类型和私有状态 | 在至少两个客机上取 `F4` 快照。两端比对数值、颜色、向量、引用与嵌套容器；客机快照的 `private` 只含自己序号，`public` 包含其他有效参与者。共享与自引用表只验证有限声明路径的值；客机两份新子表地址不同。再执行 `bump`，对照可选字段和旧键清空、本地字段和默认写回表身份保留 |
| 频率与失败隔离 | 对照两次 `cadence` 和各端 `F4`。先 `mark`，接着 `unmarked` 后等待，再 `mark`；客机只在标脏时看到新值。四种读取失败各观察上一条合法值和具名计数；`set-error` 期间另外执行 `bump`，对照计分仍能变化、`fragile` 保留旧值，最后 `restore` 恢复。执行 `slow-read`，对照该次读取耗时、诊断 `read_us` 和客机的新计分。还可从组装清单时的 `large_world_container` 日志确认 `large` 声明触发大容器诊断 |
| 实体与生命周期 | `spawn` 后核对两条声明、实体引用和 `entity-installed`；使用现有 E2E 写入故障 `cs client_test <token> fault binary-world-after-install` 触发恢复，记录是否真正重建同一 Host 成员。只有观察到对应 `entity-removed`／`entity-installed` 和内容恢复，才算覆盖实体重建。再执行 `remove` 观察目标变为 Null；同层换房、换层、新局分别对照事件和 `reset` |
| 输入与意图 | 从客机操作 `K` 和右移，对照 Host 的 `authority-input` 归属、按住和边沿。用 `F6/F7` 对照 `intent-handled` 和发起者的 `intent-result`；其他客机没有该结果。清空在途并等上一秒窗口结束后执行 `F8/F9`。低 RTT 下 `F5` 须在一秒内确实发到第三批才覆盖每秒限流，否则只记录实际覆盖 |
| 只读漂移 | 正常模式下用 `F11` 改写，Host 下一次 `bump` 后恢复。诊断模式重新做一遍，要求 Host 计分保持不变时也有 `drift-restored` 和 DLL 的具名改写日志 |
| 进度与原生显示 | 接入前分别在两份测试存档取得 `native.unlocked`。接入后确认客机 HUD 的锁状态与 Host 一致，其他进度查询逐项对照，结束后以保存副本的实际字节核对客机存档。Host 执行 `native` 后对照 cache、隐身画面、健康和充能查询、Boss 血条值；选带主动物品的角色以覆盖充能覆盖。执行两个楼层命名动作并跨门核对名称与背景 |
| 音频与过场 | Host 的 `sfx-pre` 记录改号和取消。对照实际听到的效果及客机回放期间的回调日志；用客机 `F10` 验证本地声音回调确实能触发。对照音乐当前 ID。两次过场分别观察客机画面与后续场景状态，清场时观察退休及退出 |

双端启动器的模组适配包默认开启诊断。第 14 步如需正常模式对照，在隔离副本的配置中关闭 `diagnostics.enabled` 再启动，保留使用同一夹具的输入和对照方式。
