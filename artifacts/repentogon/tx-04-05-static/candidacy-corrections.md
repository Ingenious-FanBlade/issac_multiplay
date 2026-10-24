# 静态候选文件更正记录

保留原始候选文件作为过程记录；下列文件名或函数身份曾被误配，不应作为结论依据。

| 原文件 | 实际内容 | 可用证据/更正 |
| --- | --- | --- |
| `t2_get_device_type.txt`、`t2_get_device_name.txt` | 两份均导出 RVA `0x5EED00` 的同一较大函数，不能据文件名称为 `GetDeviceType/Name`。 | 设备 id 映射实际证据为 `primary-ida-evidence.txt` 的 `[T2 lookup by device id]`：RVA `0x5EEC40` 遍历 vector 中的设备对象并比较对象 `+0x08`。官方声明见 tag1.1.3 `libzhl/functions/Input.zhl`。 |
| `t4_manager_action_value.txt` | RVA `0x5EE2C0` 的 bool 辅助函数；调用 vtable `+0x2C` 后取反。 | Scalar 的 InputManager 目标是 RVA `0x5EE2E0`；Manager 标量包装 helper 是 RVA `0x52F7B0`（IDA VA `0x92F7B0`），见 `luna_export_ida.py` 输出项和 `primary-ida-evidence.txt`。 |
| `t8_query_helper_52610.txt` | 内容确为 RVA `0x52F610`（VA `0x92F610`），文件名省略了 `F`。 | 语义为 Manager 层 Pressed 包装 helper；规范名 `t8_manager_pressed_wrapper`。保留原文件。 |
| `t6_roomchange_caller_341a45.txt` | 头部函数范围为 RVA `0x340440..0x34073D`，并不包含调用点 `0x341A45`。 | 不作 caller 证据。实际含调用指令的函数为 VA `0x7415E0`（RVA `0x3415E0`），调用点 RVA `0x341A45` 与 `0x341AA3`；对应复核导出项 `t6_other_room_load_owner`。 |
| `t1_odd_helper.txt` 中 `[DECOMPILE] None` | Hex-Rays 未给出伪代码。 | 只能引用同文件中的汇编与调用关系；不得描述为已有反编译。 |
| `luna-findings.md` 初稿 T5 使用 “Room net-vector” | RVA `0x2F76F8` 的条件实际检查 Manager 容器。 | 已改为“Manager 内对应 net container 为空”，不把该容器误归到 Room。 |
| `luna-findings.md` 初稿 T7 把 `+0x162C` 写成 Player Update store，并把 Q16 当字段类型 | store 实际在射击 consumer RVA `0x373990`，写入宽度为 byte/bool flag；Q16 是设计条目编号。末次主审又纠正 float store 的归属。 | `0x3794B0/0x3794B8/0x3794C0` 三条 store 均在射击 consumer 内，不在 Player Update 本体；原修订“float store 仍在 Player Update 内”的说法也已撤回。 |
| `luna-findings.md` 初稿 T8 推出必须 Hook Manager helper | 本轮不支持 gameplay 输入 Lua mod，已知 native fallback 经过设备槽。 | 已改为保持设备槽覆写，并将 Manager Lua 替代路径标为不可见/未观测；不从短路证据推出必须新增 Manager hook。 |
| `luna-findings.md` 初稿 T8 action 汇总不完整 | Parent 主审补充 movement/shooting 动态 action 常量表与 active 默认/特殊分支。 | 已记 movement `RVA 0x741020=[0,1,2,3]`、shooting `RVA 0x741120=[4,6,5,7]`；active 默认 action 9、特殊条件/slot 查询 action 10。 |

地址均以文档中的 RVA 书写；当前 IDB imagebase 为 `0x400000`，IDA VA = RVA + `0x400000`。此处只记录静态候选纠正，不修改 IDB，也不删除历史导出。
