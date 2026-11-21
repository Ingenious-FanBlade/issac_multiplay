# tx-01-49：B 死亡与复活专项 E2E

日期：2026-10-16。用户明确要求“先测试 B 的死亡和复活”，并确认可以重启准备、手动完成 B 死亡/复活。沿用隔离 Offline 场景与已有操作授权。tx-01-48 的完整结果保留为 35 PASS、7 NOT_OBSERVED；本轮只补验死亡/复活，不重跑整轮或改变其结果。

## 实现前场景与失败判据

不修改生产 DLL、controller 或设备生命周期实现，不构建、不提前试跑。使用 tx-01-48 已构建产物，增加专项场景输入、统一执行入口和只读取本次捕获的专项报告。一次入口启动一个 controller，只有一次注入、正常 shutdown 和报告，不自动重试。任何补充场景不用于宣称完整 T0-05 完成。

操作顺序：

1. 用户完全退出旧游戏进程，重新打开隔离 Offline run，初始仅普通 A；确认准备好后才启动入口。旧 PID 35976 不可复用（旧 DLL 已 pin）。
2. 注入 tx-01-48 已构建 DLL，观察 A/B 绑定、单次注册和原生 Join。短脚本只发送 B 移动/射击、中性以及 wrong-generation 输入，不使用 Bomb/Item/Pill/Drop，也不触发冲突退休。
3. 用户让 B 死亡，保留 A 存活，暂不复活。必须用 ordinal=1 的原生 Death 事件确认，用户口头确认不能代替事件。
4. 保留死亡状态至少三个 Authority tick，观察 B Resolve 不可见、中性且 command=0，没有 B 接受输入、有效 Query 或有效 consumer apply；A 仍有有效 Resolve、Authority 继续提交。当前 Player TLS 对失效玩家可能记 ordinal=UINT32_MAX，此类 query 不冒充已精确归属 B 的证据。
5. 确认死亡窗口后再请用户复活 B。必须观察 B Respawn、新 generation、重新唯一绑定到相同 owned device/control/K 和冻结 ClientId=2/ordinal=1。允许原生 Player 地址变化，不要求地址恒定。
6. 发送 fresh script，确认新 sequence/当前 generation 输入被接受、产生 B 有效 Query/Movement 或 Shooting witness；旧 generation 的输入被拒绝，旧 sequence 不重新应用。A 继续有效推进。
7. 用户返回主菜单且保留进程；观察原生 Teardown，然后正常 stop，确认 pair 缺席、root 保留、47 槽原字节恢复和 DLL pin。死亡与复活不是设备退休或第二次创建。

以下情况记 FAIL：

- nativeError、事件覆盖/传输丢失、进程退出、未正常恢复、重复 Resolve/world commit/consumer apply/accepted sequence 或不一致 query。
- 已观察到 Death，但死亡窗口仍有 B 可见 Resolve、非中性 current、有效 Query/consumer apply、接受新输入，或复活前重新绑定。
- Respawn 的 generation 没有增加、重绑身份不匹配、复活后应用旧 sequence，或 wrong-generation 输入被接受。
- 第二次设备注册、B actual create 或 Join 发布；退休/恢复释放 root，或 Hook 原字节不一致。

没有观察到 Death、Respawn、至少三个死亡 tick、A 推进、复活后新输入/世界变化或恢复证据，均记 NOT_OBSERVED，不靠人工标记判 PASS。死亡后没有精确 ordinal=1 query 时，报告其证据限制；以原生 B Resolve 观察中性 current，不宣称拥有已归属的 query 样本。

## 入口与产物

入口：`cs-t0/accept-b-death-respawn-e2e.ps1`。默认复用 tx-01-48 的 Native/helper 及现有 release controller；只在文件不存在或显式参数指定时报告相应情况，不自动构建或修复。输入：`cs-t0/testdata/tx-01-49-b-death-respawn-script.json`。

本轮新目录：`artifacts/cs-t0/tx-01-49-b-death-respawn-live-01`。产物包括 invocation、场景输入、复用二进制路径/尺寸/时间、controller.log、capture 中原始事件/loop/操作/helper/restoration/result，以及专项 result.json。原始 controller 的 feature=tx-01-45 结果不覆盖；专项报告明确 scope/tx，逐项附事件序号。后处理器不连接游戏，不作为额外测试运行。不计算哈希。

## 结果

2026-10-16 运行一次专项入口，复用 tx-01-48 产物，没有构建或改动生产代码。新 PID 37348，creation time 134366370853835746；A/B 绑定、注册及 Join 成功。用户在进行死亡操作前报告 A（阿萨泻勒，ID=7，键盘）的移动很慢、疑似不能完成蓄力攻击。随即暂缓死亡/复活，用同一次捕获定位输入异常，并正常结束本轮；没有重跑。

**死亡/复活专项为 NOT_OBSERVED：3 PASS、8 NOT_OBSERVED，B Death/Respawn 均未实际发生。**记录 519,940 条事件、7,980 Authority tick；nativeError=0，transportFailure=null，重复 Resolve/world commit/eligible consumer/accepted sequence 和不一致 query 均为 false。用户确认主菜单后，#519889/519890 原生 Teardown，正常 shutdown 成功，47 槽恢复、root 和 DLL pin 保留，同一游戏进程仍在。

本轮发现的输入缺陷不被上述窄范围判定覆盖，另记为已观察的实现问题：

- A 连续物理 aim/Shoot held 能进入 Resolve；固定范围捕获中 2,447 个 sample target、2,446 次 Resolve、1,366 个非零 sample，没有非零 sample 在对应 target Resolve 成中性，也没有同 generation 相同 held sample 之间的缺输入中性空洞。这里不能推出所有输入路径均无问题。
- 同 tick 的额外 Movement/Shooting 原生调用发生在 `g_tick==0`、current 已不可见的窗口，`Evaluate` 直接返回 0；它们仍实际调用 original，仍改变世界字段。旧 analyzer 只检查 decision=1 的重复 consumer，因此没有将其报告成重复 apply 或 Failure。
- 例如 tick 799，#51863 有效 Shooting、Shoot held=1，原生 +0x162C..+0x1638 witness 从 `[0xFFFFFFFF,0,0,0]` 变为 `[3,0,0x3F800000,0]`；#51898 同 tick decision=0 的 Shooting 又清回 `[0xFFFFFFFF,0,0,0]`。tick 800 的 #51928/#51963、801 的 #51993/#52028、802 的 #52058/#52093 重复该模式。
- 这些实际更新和中性覆盖能解释用户报告的症状；现有 witness 不是蓄力完成/发射计数，不能声称蓄力状态字段已被完整证明。额外调用的完整原生 owner 仍需静态核对后修复，不能简单让 A 旁路 scheduler 或在失效 current 下读取旧输入。
- 只读检查安装目录发现 TimeMachine 源码含 `MC_POST_UPDATE` 内的 `pl:Update()`，但用户明确确认本轮未启用该模组；不能将其作为本次额外调用的原因。当前捕获未记录 consumer 的原生 return address/phase，完整 owner 尚未确定；没有据此猜改生产边界，也没有运行第二次实验。

产物：[专项结果](../../../../../../artifacts/cs-t0/tx-01-49-b-death-respawn-live-01/result.json)、[原始 controller 结果](../../../../../../artifacts/cs-t0/tx-01-49-b-death-respawn-live-01/capture/result.json)、[恢复记录](../../../../../../artifacts/cs-t0/tx-01-49-b-death-respawn-live-01/capture/restoration.json)、[A 输入观察](../../../../../../artifacts/cs-t0/tx-01-49-b-death-respawn-live-01/a-input-observation.json)、[额外 consumer 观察](../../../../../../artifacts/cs-t0/tx-01-49-b-death-respawn-live-01/a-world-observation.json)。前者为固定字节范围的 capture-only 分析，不是第二次实机试验。完整 tx-01-48 结果保持原样；后续需要先修复本缺陷，再执行包含 A 蓄力和 B 死亡/复活的下一次整体验收。
