# tx-04-19 I01—I03：目录、纯值目标与正式来源

日期：2026-11-03，Asia/Singapore。本次按用户要求只实施 I01—I03；不将其记作整个 tx-04-19 完成。原生对象计划、构造／写回／绘制／退休、Scene 提交以及统一 E2E 仍由 I04—I35 继续。下列为代码与静态来源合同，运行覆盖另行记录。

## I01：应用目录

`mod/tools/gen_entity_application_catalog.py` 从正式 `entity-catalog.json`、15 的复制归属函数和 P1—P6 逐成员发送策略生成产品目录；可审阅版本为 [entity-application.json](entity-application.json)。每个原成员／重载保留 ordinal、规范 id、别名签名、Wiki／原生来源及版本、适用类别、责任、目标位置、本地承载、缺失语义、依赖、实施任务和 E2E 对应。公共继承只由 Entity 定义；目录不把待写回字段标成已经原生应用。

分类采用公开派生接口或已核定的目标 RTTI。NPC 不按 Type 数值区间推断，Delirium 是同一 NPC 的 special 接口。NativeDummy 与 Entity_Dummy 对应，旧能力声明 NativeDarkness 是命名残留，既不建立别名，也不虚构另一类 Darkness。Player 和相关装备／多身体状态具名交给 20，公共 Entity 字段仍有 Player 责任边界。Slot 1—18 的真实语义入口保留于 `slot_variants.lua`，未知变体不替换成普通老虎机。

## I02：正式 T 与能力

`mod/src/replica/entity_target.*` 接受正式模型 T 的 Entity component，不要求代表夹具 profile，也不另建测试协议。`EntityTarget` 深复制源组件，按类别 variant 保存公共／派生／special 纯值。图引用仍以完整源 component 为解析域，不把分段副本误认成闭合图。

解析保留 int64 精度、完整 bitset128 两半、原始字节串、稀疏键与图关系、资源姿态、不同身份空间引用和状态占位。数值使用自身语义：有符号字段不统一变成非负数，bits128 接受 Lua signed int64 表达的同宽位模式。对象身份的初始 shape revision 合法为 0。Null、空集合、字段缺席、NotApplicable、NotScheduled 和失败／未知状态分别保留，不补零。

原成员数、全部对象、Player Key 和非 Player Key 分别保存；来源 coverage、原始全成员枚举和非 Player 字段完整性分别表达。Exists=false、IsDead、退休行及未知类别不会被夹具过滤掉。退休行不读取已退休对象的派生成员；未知对象不回退成 NPC。解析不会声明任何原生提交或网络 ACK。

模型 v2 保留；新增必需协商 `entity-target-v1`，来源扩展为 `entity-source-v1`。旧必需版本、未知 required contract 和 NativeDarkness 显式拒绝。能力分别声明完整模型表达、尚未完成的非 Player 原生应用、现有 Player 代表范围与待 I34 进行的运行验收；不设置完整 Entity 域完成位。

## I03：普通窗口与独立表现来源

普通 Entity 采集声明 `same-ordinary-window`，在同一正式窗口复制主 Sprite、Tear 的三份辅助 Sprite、Bomb 的五个 costume Sprite、Pickup 价格和 Slot 奖品资源。官方 wrapper 均返回真实成员，未加载 ANM 表达 Null；不为读取执行 Load／Reset／Update。Bomb 参数固定为官方 wrapper 允许的 0—4，发送清单恢复该查询的参数、值、Null 与失败；其余已省略查询保持逐成员复核责任。

Laser 继续读取公开 GetSamples／GetNonOptimizedSamples 的当前集合，不调用 CalculateSamples／RecalculateSamplesNextUpdate，也不把不透明 HomingLaser 当成另一 Entity。Color／状态效果、配置、关系、容器和公开派生字段复用正式 schema。Effect136 通过具名引用交给同窗口 Grid.lifted_descriptors，不额外读写通用 VarData 或补造原 Grid Key。

主审还独立核对 Laser Init／Render、RTTI TD `RVA 0x7D140C` 与三个初始化写点。新增正式窄来源保存 `sample_mode_raw:int8`（`+45D`）和 `start_offset_mode_opaque32:int32`（`+4F0`）：前者 Init 为 -1，Render 按有符号负数停显，不能被公开 bool 吞掉；后者 Init 为 0，draw 依据 `<=0` 选择起点修正，语义名称继续 UNKNOWN。两项只对已观察创建 owner 完成、当前 use 未退休的真正 Laser 读取。公开 LaserLength 与 `+460` 的属性绑定也已静态对应，原生样本/私有 Bezier 不重复复制。

NativeText 新增完整 20 字节 KColor：RGBA 加被原生字体路径消费的末字 opaque32；后者语义仍 UNKNOWN，不当作 padding。counter、duration 的读取依据与文本缓冲的读取依据分开。Init 没有初始化字符串缓冲，因此不能沿用原先仅凭创建 owner 就读文本的过宽规则。

唯一 Hook 清单新增 Text Update／Render 常驻观察 wrapper，original 各一次。Update 只在以下真实路径正常完成且当前 local/use/revision 未变化后登记写入见证：variant1／2 的 pre-counter=0 格式化分支，或已有 `%s` 写入完成的具名 helper 返回点。主审独立读取反汇编后纠正了子智能体最初将 counter 判断说成 buffer 首字节判断的结论。未观察 writer 的 FIRST_OBSERVED／池复用对象保持 NotCaptured，不读取缓冲，不补造初始化。见证表最多 16384 项，旧 use／revision 不继承。

Text Render 独立发布 `native-text-render-v1` 的入口输入：Manager frame、公开 Game.IsPaused、派生 subframe、字体逻辑 owner、render offset、文本字节、计数与完整 KColor。Manager frame 与 Game frame／Authority tick 不等价。源来自实际 Render 调用的入口，静态链确认中间无 gameplay callback／帧推进；不宣称抓取了原生局部变量。普通 T 只保存该独立来源的具名依赖，后续消费者必须匹配 Key／shape／counter／text，不能把旧 Render 拼成原子快照。原有每 30 次回调的 Sprite 样本也明确保留独立节流语义。

正式消费者保存 Text Render 最新样本，按当前 room 分开，最多 4096 主体／4 MiB；不足增加 formal_loss，不伪装普通 Complete。关闭 Recorder 不关闭这些来源。游戏对象／Lua 操作仍在主线程，后台只取得自有值。没有加入 Gameplay 调用、RNG 选择或持久写入。

## 构建与验收边界

目录和纯值解析已接入正式 CMake 库及产品打包；生成目录覆盖 1793 个原成员／重载行。按本 tx 计划，I34 的完整实现与统一 E2E 尚未到达；本次不编写或运行单元测试、不启动游戏、不部署、不执行提前分项测试。

纯编译：普通沙箱中的 MSBuild 停在文件扫描，已中止；经审批通道执行 `scripts/build.ps1` 后，Win32 Release 全部库与 `out/repentogon/zhlIsaacCS.dll` 已成功编译链接并完成产品资源复制。静态审阅完善参数查询／多返回值的类型校验后，同一纯构建再次成功。保留已有 RapidJSON／SDK／ASMPatch 与既有源码警告，不将其写成零警告构建；未执行分项程序测试。
