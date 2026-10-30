# tx-04-10 实现与整体验收

状态：2026-10-30 全目标实现完成，构建通过，同一 tx 的统一 E2E 连续修复后最终 **5/5 PASS**。公共边界、采集／录制分离、Entity／旧 Grid 拆分及完整 Entity schema 已交付；运行范围与未观察路径如下分别陈述，不代表完整世界或可玩 Replica 已完成。

## 公共边界与迁移

`world/common/contracts.*` 提供 DomainId、独立 IdentitySpace、SubjectKey、上下文、引用、字段有效性、DomainResult、预算、CapturePlan 和 envelope；`common/service.*` 提供激活、消费端 Attach／Detach、计划更新、Prepare／Commit、Invalidate 及停止。普通后态只接受 AuthorityCommitted；独立生命周期、显式查询和 render 使用自己的 scope。未实现域的请求明确拒绝。

`common/native_source.*` 只提供受保护的值复制和已核定 RTTI 分类；`common/membership.*` 提供有界容器读取及两遍一致性。Entity 域负责 Entity 来源和 Player 保留关系；Grid 域负责 Grid／Door 来源、实际索引及初始化资格。协调器继续拥有 Authority 阶段、调用配对、RNG 窗口、公共身份、来源／root 末尾复核和唯一提交次序。来源复核失败及全局生命周期边界均撤销全部身体锚点，修复旧路径只撤销前两个槽的问题。阶段能力明确为 Entity 的 OrdinaryPost/Lifecycle/Render/ActualCall 和旧 Grid 的 OrdinaryPost/Lifecycle，未接入的 PlayerPost/Transition 请求拒绝。

旧 profile 字段目录和 native 字段复制移入 `compatibility/min_read_v1*`，旧域编码保存在 `min_read_v1_encode.inc`，通过 writer 私有纯值 JSON sink 编译；旧字段 ID／原因／身份规则不变。新来源和新域不扩展旧 RawObject。Entity/Grid 扩展投影及 v1 都取同一个已复制 native 批次，Lua 在同一普通复制窗口内部执行，随后复核来源、root 和 RNG。

## 采集与录制所有权

`WorldCapture` 拥有 Collector、IdentityTable 和 Service，其 epoch 与 Recorder generation 独立。后台唯一 dispatcher 读取 native owned values；Recorder 仅拥有自己的队列和文件。无 Recorder 时正式消费者继续取得纯值。文件错误保存在 writer／Recorder 自己的首因，不写入采集 storage 的故障状态。

停止录制在主线程移除其消费需求，后台等录制关闭边界之前的槽、事件及 gap 复制完成后 drain；采集不等待磁盘。完整停止另行关闭采集，等待正常 callback／pending 收口并排空后台。Hook 随 DLL 启动安装后常驻，默认没有消费者时不做全图扫描；不卸载或恢复字节。

正式消费可用 `cs observe start|status|stop`；录制沿用 `cs world ...`。录制中途加入已有 epoch 时，v1 的历史无法精确重建，兼容投影明确给出 CONTINUITY 不完整，不重新制造身份／出生；v2 仍使用实际已观察历史。world 状态附 observation_epoch 与 v2 计数。

## Entity 来源与记录

正式 Lua 模块位于 `mod/observation/isaac-cs-observation`，独立于 UI 及测试夹具。成员目录为 [entity-catalog.md](entity-catalog.md)／[JSON](entity-catalog.json)。公开字段、只读 Getter、有限槽参数查询、动态 Sprite／图层／动画／资源描述、Weapon、TemporaryEffects、PathFinder、配置等使用各自 schema 复制；关系指向同一 Entity 身份。表保留稀疏键、共享／循环引用，非有限值及预算超限有明确状态，不输出本地指针字符串。

[Text／Dummy 补充](entity-native-supplements.md)使用重新核定的 J273 来源；[Slot 变体表](../../../mod/observation/isaac-cs-observation/domains/entity/slot_variants.lua)只解释原始值。Mod 自定义表、随机流内容、独立 HUD、跨房保存分别标注负责域／tx，不发布空的完整状态。

录制目录保留旧 v1 文件，在独立 `observation-v2/` 写入 v2 manifest、目录、records.jsonl、result.json。离线入口为 `harness observation-read --capture <observation-v2目录>`；逐 schema／scope 归约，未知 schema 报 Unsupported，部分和 gap 保留，不将完整 Entity 当作完整世界。

总预算上限调整为 512 MiB：旧槽／身份／队列按原计划记账；新增 pending／最新值／编码在途预留 112 MiB，录制值队列至多 64 MiB，v2 发布队列至多 32 MiB／4096 项，使用单生产者／单消费者环而不争用互斥锁。每 scope 16 MiB、16384 主体、262144 值节点、深度 12，最多 131072 次只读成员查询；消费端不足明确损失序号，游戏主线程不等待。字节不足时裁剪已复制值并修正被裁掉的图目标引用，不重调 Getter。该配置声明容量，不承诺任意规模始终完整或任意负载帧率；共享游戏 Lua 分配器的物理峰值未独立测量。

## 统一 E2E

入口 `scripts/accept-tx-04-10.ps1`，Build 与 Run 属于同一整体验收。全部目标实现前未运行测试；不补单元测试。前两次按规范取得准备确认；之后用户在本会话明确要求测试无需询问、直接开始，第三／四次按该授权启动，并分别提示停止手动操作和退出。

统一场景覆盖公开类别与 Delirium／Text、典型 Slot 变体与真实接触、Morph、同窗口创建移除、Bomb／Laser／Knife、动态外观、Grid 同格替换、四参与者／八身体、显式查询、暂停及切房、录制关闭／重启、真实文件故障后的正式采集继续、v1/v2 同读取身份投影及调用进入返回配对、正常退出。Dummy 和未触发的变体须分别记录实际未观察范围，不能写成运行通过。

临时部署并恢复 DLL／正式观察 Lua／UI／夹具／配置及 Helper 标记，沿用第二存档种子测试局的 D18/D20/D27 变化范围。成功清理临时材料，必要中断才保留诊断和恢复信息。失败在同一 tx 修复续测，不因首次失败默认停下。

## 本轮验收过程

- 全目标实现前未构建、运行测试或 probe。进入统一验收后首次构建发现 NativeSource 缺少 FaultSwitches 的声明依赖，补齐后构建通过。沙箱内 MSBuild 没有启动编译工作，已停止并使用审批通道构建。
- 第一次实机已进入第二存档种子局；录制启动阶段失败，尚未执行 Entity 类别场景。原因是 REPENTOGON 工作目录与游戏根目录不同，成员目录资源定位错误；v2 result 路径又在 Windows 扩展路径中包含内部正斜杠。现按进程可执行文件定位兄弟 mods 资源，每个路径组成段分别拼接，并保留 Recorder 打开异常日志。
- 第一次实机正常退出，临时部署恢复无错误；本次存档变化检查通过，未出现新崩溃文件。失败材料只供本 tx 续测，最终成功后一并清理。
- 历史 tx-04-04 快照已清理，本 tx 不恢复或制造旧快照。D20 直接比较本次前后文件：第一／三槽仅尾部 Counter/Checksum 两个 u32 的字节变化可接受；不计算、不校验哈希或 Checksum，其余变化保持失败。
- 第二次实机进入四人和 Entity 场景，但夹具断言了无返回值的 NPC Morph，未继续剩余场景。修正为调用后复核实际类型；另修复发布队列锁竞争损失、节点预算不足、Tear/Knife 夹具存续及两项 J273 接口差异。GetBombFlags 的实际 RNG 路径与 SampleLaser 的类型事实见[补充研究](entity-native-supplements.md)，原失败不改判。
- 第三次完整场景及原 4 项归约通过，临时材料自动清理。收尾检查发现全局生命周期撤销仍遗留双槽限制及阶段 descriptor 不精确，修正后在同一 tx 续测；增加直接验证 Morph/外观/Slot/Grid 和完整八身体后态的字段断言。
- 第四次最终 **5/5 PASS**，进程正常退出 0，部署／Helper／配置恢复及存档范围检查通过，无新崩溃文件。所有本 tx 测试临时目录、截图、录制及诊断材料清理，保留本文事实与正式产品资源。

## 最终实际覆盖

| 验收项 | 最终结果 |
| --- | --- |
| 公开 Entity 类别 | Complete 后态覆盖 Player、NPC、Tear、Projectile、Bomb、Laser、Knife、Pickup、Familiar、Effect、Slot、NativeText，NPC 专用 EntityDelirium 字段亦实际出现 |
| 普通完整后态 | 首段 10 条，录制故障恢复后 4 条；后 4 条均含 8 个 EntityPlayer、四组 ordinal 各两个身体，节点数 91996—105775 |
| 生命周期与原调用 | 同窗口 Tear 创建／移除事件均出现；3709 组 native 调用各进入／返回一次；NPC/Pickup Morph 保留 local_id/use 并增加 revision，后态 Type/Variant 与实际结果一致 |
| 外观／交互／Grid | 已复制 Sprite Rotation=37、FlipX=true、Scale.x≈1.15；真实乞丐接触的 Touch/DonationValue 在 Entity 记录内出现；同一 Grid 槽替换取得两个不同主体身份 |
| 真实返回值 | Pickup、Fireplace、Shopkeeper 的真实 LootList 返回值各记录一次，不为观测补调 Getter |
| 录制分离 | 停录后文件不再增长、正式 scope 继续；真实文件句柄故障后 Recorder 关闭为 FILE_ERROR，正式采集保持同一 epoch 并继续 Complete；重启录制成功 |
| 兼容与 v2 读取 | 1433 个对象的 v1/v2 同读取身份、修订、字段状态和非引用字段原始字节一致；首段 20455 条、恢复段 3929 条 v2 记录，未知 schema 为零，两段 delivery_complete=true，发布／scope 损失均为零 |
| 输入与生命周期 | 四参与者八身体、独立短移动、暂停、切房、输入退休、观测停止、退菜单和正常退出完成；trace_dropped/query_mismatches/cleanup_failed 均为零 |

Partial 与 NotScheduled 记录没有删除：旧 native-v1 Entity 范围仍不含新类别的完整派生字段，初始化回调的关联目标也可能尚无当前 scope 依据；这些记录保留其实际覆盖，不冒充普通 Complete 后态。Grid 仅迁移旧范围，完整 Grid 仍为 tx-04-11。

Slot 夹具请求的代表变体为 1、4、6、9、13、15、16、17、18，记录保留实际返回值而不按请求覆盖；真实接触及字段语义断言针对实际变体 4。NativeDummy、其他 Slot 变体、全部道具／角色组合、全部显式参数域及极端负载未逐一运行。目录完整不等于所有 API、所有实例路径都执行过；未接入的 random/mod/presentation/room_save 等仍明确归后续 tx，reader 的 world_complete=false。不主张性能验收通过或已经实现世界同步。
