# A 项主审只读复核

2026-10-27。主智能体在子智能体报告跨字段写后，直接使用 IDA disasm 复读 VA `0x6A4BD6` 起的 14 条指令：所属函数 `sub_6A4A20`，首条为 `mov word ptr [esi+176h], 0`，下一条从 `0x6A4BDF` 开始。这一 word store 覆盖 `+0x176/+0x177`。字节读取另见本轮 IDA 工具原始记录及代理材料；目标为 `isaac-ng-v1.9.7.12.exe.unpacked.exe`、imagebase `0x400000`。

主审采纳初始化更正，但不采纳“字段永远为零”或未观察对象已有 Init 历史的推论。当前读取仍采用本轮用户明确采纳的限定规则。静态读取不是运行验证，没有修改 IDB、运行目标或测试。

主审另外用 IDA `get_bytes` 回读上述原始字节为 `66 c7 86 76 01 00 00 00 00`。C 项复读 `0x813BD0` 的全部 23 条原始指令，确认 index 上界 `0x1BF`、type=1 且 keepDecoration 时早退、`0x813BF8` 写 `[esi+edx*4+76Ch]`、`0x813BFF` 清槽、`0x813C0B` 虚调 slot0(flags=1)、`ret 0Ch`。采用 PathTrail=0 只用于初始路径值已观察为0、夹具标签/代次/seed/type/variant均匹配且无door alias的自建rock；Lua标签及seed不是native物理连续性的证明，原生槽/身份仍由episode另审。

主审复读本地 `LuaEntityPlayer.cpp:3247–3298` 及既有 `ida-restore-replace-callers.json::functions.replace_caller`：703分支确实检查关联slot、创建Player并在有效替换对象下调用 `sub_995E10`；尚未确认 public UseFlags/slot 到该 owner 的完整映射，不把“未见标准Isaac禁用”推成可执行保证，默认禁用该特殊动作。Continue仍使用现有真实菜单保存/恢复流程。
