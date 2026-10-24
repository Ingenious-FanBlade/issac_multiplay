# 输入管理与物理设备：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

InputManager是RVA 0x857B18的内嵌对象；与读取Game指针的全局槽不同。Manager vtable 0x782950/COL 0x7B0424；Keyboard 0x7A1678/COL 0x7B191C；Gamepad 0x782C80/COL 0x7B0460。物理键盘状态源RVA 0x878C10。

Manager Pressed/Triggered/Scalar槽+0x40/+0x44/+0x48：ECX=this，primitive/controller/selectedKey三个栈参数，ret 0xC；Scalar返回x87 ST(0)。Logical查询槽+0x30/+0x34/+0x38：一个action栈参数，ret4；setter +0x4C为action/float、ret8。Poll槽+0x78为无显式栈参数的thiscall，返回bool。

物理poll只采样；Authority-pre发布current，正常返回后提交previous。A/B统一codec、sequence、queue、Resolve和apply；主机没有即时旁路，缺输入max-age=0中性。移动相反方向相消，射击按每轴后按者优先；不能混用两套规则。

## 源码声明的补丁、调用依赖与自有表

| 安装家族 / 序号 | 类型 | site RVA → 原目标 RVA | 用途 | 字节 / 跨度 | 源码 |
| --- | --- | --- | --- | --- | --- |
| loop_runtime.cpp [8] | REL32_CALL | 0x00554F7D → 0x0061FC00 | InputManager 完整更新采样 owner | E8 7E AC 0C 00 / 5 | [loop_runtime.cpp:45](../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [9] | REL32_CALL | 0x0061FC97 → 0x0061F280 | 公共设备更新 | E8 E4 F5 FF FF / 5 | [loop_runtime.cpp:45](../../cs-t0/native/src/loop_runtime.cpp) |
| boundary_runtime.cpp [0] | REL32_CALL | 0x00554F84 → 0x0055B310 | 边界研究输入采样 | E8 87 63 00 00 / 5 | [boundary_runtime.cpp:42](../../cs-t0/native/src/boundary_runtime.cpp) |
| coop_passive_runtime.cpp [0] | REL32_CALL | 0x005B27EB → 0x003A6450 | 设备分配/转换 | E8 60 3C DF FF / 5 | [coop_passive_runtime.cpp:79](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [1] | REL32_CALL | 0x0055B87F → 0x003A6450 | 设备断开分配 | E8 CC AB E4 FF / 5 | [coop_passive_runtime.cpp:79](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [2] | REL32_CALL | 0x0055BCA0 → 0x003A6450 | 设备重连分配 | E8 AB A7 E4 FF / 5 | [coop_passive_runtime.cpp:79](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| active_targets.cpp [0] | VTABLE_SLOT | 0x007A16A8 → 0x0061E8D0 | Keyboard Pressed | D0 E8 A1 00 / 4 | [active_targets.cpp:26](../../cs-t0/native/src/active_targets.cpp) |
| active_targets.cpp [1] | VTABLE_SLOT | 0x007A16AC → 0x0061E970 | Keyboard Triggered | 70 E9 A1 00 / 4 | [active_targets.cpp:26](../../cs-t0/native/src/active_targets.cpp) |
| active_targets.cpp [2] | VTABLE_SLOT | 0x007A16B0 → 0x0061EA20 | Keyboard Scalar | 20 EA A1 00 / 4 | [active_targets.cpp:26](../../cs-t0/native/src/active_targets.cpp) |
| active_targets.cpp [3] | VTABLE_SLOT | 0x00782CB0 → 0x0061E8D0 | Gamepad Pressed | D0 E8 A1 00 / 4 | [active_targets.cpp:26](../../cs-t0/native/src/active_targets.cpp) |
| active_targets.cpp [4] | VTABLE_SLOT | 0x00782CB4 → 0x0061E970 | Gamepad Triggered | 70 E9 A1 00 / 4 | [active_targets.cpp:26](../../cs-t0/native/src/active_targets.cpp) |
| active_targets.cpp [5] | VTABLE_SLOT | 0x00782CB8 → 0x0061EA20 | Gamepad Scalar | 20 EA A1 00 / 4 | [active_targets.cpp:26](../../cs-t0/native/src/active_targets.cpp) |
| coop_active_runtime.cpp [0] | VTABLE_SLOT | 0x007A16A8 → 0x0061E8D0 | Keyboard Pressed | D0 E8 A1 00 / 4 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [1] | VTABLE_SLOT | 0x007A16AC → 0x0061E970 | Keyboard Triggered | 70 E9 A1 00 / 4 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [2] | VTABLE_SLOT | 0x007A16B0 → 0x0061EA20 | Keyboard Scalar | 20 EA A1 00 / 4 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [3] | VTABLE_SLOT | 0x00782CB0 → 0x0061E8D0 | Gamepad Pressed | D0 E8 A1 00 / 4 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [4] | VTABLE_SLOT | 0x00782CB4 → 0x0061E970 | Gamepad Triggered | 70 E9 A1 00 / 4 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_active_runtime.cpp [5] | VTABLE_SLOT | 0x00782CB8 → 0x0061EA20 | Gamepad Scalar | 20 EA A1 00 / 4 | [coop_active_runtime.cpp:1684](../../cs-t0/native/src/coop_active_runtime.cpp) |
| coop_passive_runtime.cpp [0] | VTABLE_SLOT | 0x007A16A8 → 0x0061E8D0 | Keyboard Pressed | D0 E8 A1 00 / 4 | [coop_passive_runtime.cpp:412](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [1] | VTABLE_SLOT | 0x007A16AC → 0x0061E970 | Keyboard Triggered | 70 E9 A1 00 / 4 | [coop_passive_runtime.cpp:412](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [2] | VTABLE_SLOT | 0x007A16B0 → 0x0061EA20 | Keyboard Scalar | 20 EA A1 00 / 4 | [coop_passive_runtime.cpp:412](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [3] | VTABLE_SLOT | 0x00782CB0 → 0x0061E8D0 | Gamepad Pressed | D0 E8 A1 00 / 4 | [coop_passive_runtime.cpp:412](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [4] | VTABLE_SLOT | 0x00782CB4 → 0x0061E970 | Gamepad Triggered | 70 E9 A1 00 / 4 | [coop_passive_runtime.cpp:412](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| coop_passive_runtime.cpp [5] | VTABLE_SLOT | 0x00782CB8 → 0x0061EA20 | Gamepad Scalar | 20 EA A1 00 / 4 | [coop_passive_runtime.cpp:412](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| input_runtime.cpp [InputManager+0x40] | VTABLE_SLOT | 0x00782990 → 0x00620710 | InputManager ManagerPressed | 10 07 A2 00 / 4 | [input_runtime.cpp:25](../../cs-t0/native/src/input_runtime.cpp) |
| input_runtime.cpp [InputManager+0x44] | VTABLE_SLOT | 0x00782994 → 0x006207B0 | InputManager ManagerTriggered | B0 07 A2 00 / 4 | [input_runtime.cpp:25](../../cs-t0/native/src/input_runtime.cpp) |
| input_runtime.cpp [InputManager+0x48] | VTABLE_SLOT | 0x00782998 → 0x00620850 | InputManager ManagerScalar | 50 08 A2 00 / 4 | [input_runtime.cpp:25](../../cs-t0/native/src/input_runtime.cpp) |
| input_runtime.cpp [Keyboard+0x30] | VTABLE_SLOT | 0x007A16A8 → 0x0061E8D0 | Keyboard LogicalPressed | D0 E8 A1 00 / 4 | [input_runtime.cpp:30](../../cs-t0/native/src/input_runtime.cpp) |
| input_runtime.cpp [Keyboard+0x34] | VTABLE_SLOT | 0x007A16AC → 0x0061E970 | Keyboard LogicalTriggered | 70 E9 A1 00 / 4 | [input_runtime.cpp:30](../../cs-t0/native/src/input_runtime.cpp) |
| input_runtime.cpp [Keyboard+0x38] | VTABLE_SLOT | 0x007A16B0 → 0x0061EA20 | Keyboard LogicalScalar | 20 EA A1 00 / 4 | [input_runtime.cpp:30](../../cs-t0/native/src/input_runtime.cpp) |
| input_runtime.cpp [Keyboard+0x4c] | VTABLE_SLOT | 0x007A16C4 → 0x0061ED50 | Keyboard LogicalSetter | 50 ED A1 00 / 4 | [input_runtime.cpp:30](../../cs-t0/native/src/input_runtime.cpp) |
| input_runtime.cpp [Gamepad+0x30] | VTABLE_SLOT | 0x00782CB0 → 0x0061E8D0 | Gamepad LogicalPressed | D0 E8 A1 00 / 4 | [input_runtime.cpp:32](../../cs-t0/native/src/input_runtime.cpp) |
| input_runtime.cpp [Gamepad+0x34] | VTABLE_SLOT | 0x00782CB4 → 0x0061E970 | Gamepad LogicalTriggered | 70 E9 A1 00 / 4 | [input_runtime.cpp:32](../../cs-t0/native/src/input_runtime.cpp) |
| input_runtime.cpp [Gamepad+0x38] | VTABLE_SLOT | 0x00782CB8 → 0x0061EA20 | Gamepad LogicalScalar | 20 EA A1 00 / 4 | [input_runtime.cpp:32](../../cs-t0/native/src/input_runtime.cpp) |
| input_runtime.cpp [Gamepad+0x4c] | VTABLE_SLOT | 0x00782CCC → 0x0061ED50 | Gamepad LogicalSetter | 50 ED A1 00 / 4 | [input_runtime.cpp:32](../../cs-t0/native/src/input_runtime.cpp) |
| raw_apply_runtime.cpp [InputManager+0x40] | VTABLE_SLOT | 0x00782990 → 0x00620710 | InputManager ManagerPressed | 10 07 A2 00 / 4 | [raw_apply_runtime.cpp:28](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [InputManager+0x44] | VTABLE_SLOT | 0x00782994 → 0x006207B0 | InputManager ManagerTriggered | B0 07 A2 00 / 4 | [raw_apply_runtime.cpp:28](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [InputManager+0x48] | VTABLE_SLOT | 0x00782998 → 0x00620850 | InputManager ManagerScalar | 50 08 A2 00 / 4 | [raw_apply_runtime.cpp:28](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [Keyboard+0x30] | VTABLE_SLOT | 0x007A16A8 → 0x0061E8D0 | Keyboard LogicalPressed | D0 E8 A1 00 / 4 | [raw_apply_runtime.cpp:30](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [Keyboard+0x34] | VTABLE_SLOT | 0x007A16AC → 0x0061E970 | Keyboard LogicalTriggered | 70 E9 A1 00 / 4 | [raw_apply_runtime.cpp:30](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [Keyboard+0x38] | VTABLE_SLOT | 0x007A16B0 → 0x0061EA20 | Keyboard LogicalScalar | 20 EA A1 00 / 4 | [raw_apply_runtime.cpp:30](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [Keyboard+0x4c] | VTABLE_SLOT | 0x007A16C4 → 0x0061ED50 | Keyboard LogicalSetter | 50 ED A1 00 / 4 | [raw_apply_runtime.cpp:30](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [Gamepad+0x30] | VTABLE_SLOT | 0x00782CB0 → 0x0061E8D0 | Gamepad LogicalPressed | D0 E8 A1 00 / 4 | [raw_apply_runtime.cpp:32](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [Gamepad+0x34] | VTABLE_SLOT | 0x00782CB4 → 0x0061E970 | Gamepad LogicalTriggered | 70 E9 A1 00 / 4 | [raw_apply_runtime.cpp:32](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [Gamepad+0x38] | VTABLE_SLOT | 0x00782CB8 → 0x0061EA20 | Gamepad LogicalScalar | 20 EA A1 00 / 4 | [raw_apply_runtime.cpp:32](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| raw_apply_runtime.cpp [Gamepad+0x4c] | VTABLE_SLOT | 0x00782CCC → 0x0061ED50 | Gamepad LogicalSetter | 50 ED A1 00 / 4 | [raw_apply_runtime.cpp:32](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| loop_runtime.cpp [KeyboardPoll] | VTABLE_SLOT | 0x007A16F0 → 0x0066C650 | Keyboard Poll | 50 C6 A6 00 / 4 | [loop_runtime.cpp:30](../../cs-t0/native/src/loop_runtime.cpp) |
| loop_runtime.cpp [GamepadPoll] | VTABLE_SLOT | 0x00782CF8 → 0x00622940 | Gamepad Poll | 40 29 A2 00 / 4 | [loop_runtime.cpp:31](../../cs-t0/native/src/loop_runtime.cpp) |
| active_runtime.cpp [0] | VTABLE_SLOT | 0x007A16A8 → 0x0061E8D0 | Keyboard Pressed | D0 E8 A1 00 / 4 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |
| active_runtime.cpp [1] | VTABLE_SLOT | 0x007A16AC → 0x0061E970 | Keyboard Triggered | 70 E9 A1 00 / 4 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |
| active_runtime.cpp [2] | VTABLE_SLOT | 0x007A16B0 → 0x0061EA20 | Keyboard Scalar | 20 EA A1 00 / 4 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |
| active_runtime.cpp [3] | VTABLE_SLOT | 0x00782CB0 → 0x0061E8D0 | Gamepad Pressed | D0 E8 A1 00 / 4 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |
| active_runtime.cpp [4] | VTABLE_SLOT | 0x00782CB4 → 0x0061E970 | Gamepad Triggered | 70 E9 A1 00 / 4 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |
| active_runtime.cpp [5] | VTABLE_SLOT | 0x00782CB8 → 0x0061EA20 | Gamepad Scalar | 20 EA A1 00 / 4 | [active_runtime.cpp:1181](../../cs-t0/native/src/active_runtime.cpp) |

## 已知静态函数入口索引

目前没有额外的独立函数导出；已有虚表/安装点已在上文列明。未知入口不填猜测地址。

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。

## 源码原生定位常量补录

以下逐项保留当前源码的RVA/对象偏移命名和值，包括全局槽、vtable/COL、返回位置和只读诊断定位。常量不全是实际patch，旧命名不覆盖模块正文消歧；同一值在不同family中重复保留。`OBJECT_OFFSET`不加模块基址。

| 原始符号 | 类型 | 值 | 源码位置 |
| --- | --- | --- | --- |
| kInputManagerObjectRva | RVA | `0x00857B18` | [manager_identity_reader.hpp:10](../../cs-t0/native/include/isaac_cs_t0/manager_identity_reader.hpp) |
| kInputManagerDerivedVtableRva | RVA | `0x00782950` | [manager_identity_reader.hpp:11](../../cs-t0/native/include/isaac_cs_t0/manager_identity_reader.hpp) |
| kPhysicalPollReturnRva | RVA | `0x0061F30C` | [raw_apply_runtime.hpp:24](../../cs-t0/native/include/isaac_cs_t0/raw_apply_runtime.hpp) |
| kKeyboardVtableRva | RVA | `0x007A1678` | [active_runtime.cpp:34](../../cs-t0/native/src/active_runtime.cpp) |
| kGamepadVtableRva | RVA | `0x00782C80` | [active_runtime.cpp:35](../../cs-t0/native/src/active_runtime.cpp) |
| kKeyboardCurrentStateRva | RVA | `0x00878C10` | [active_runtime.cpp:36](../../cs-t0/native/src/active_runtime.cpp) |
| kPhysicalPollReturnRva | RVA | `0x0061F30C` | [active_runtime.cpp:40](../../cs-t0/native/src/active_runtime.cpp) |
| kKeyboardVtableRva | RVA | `0x007A1678` | [active_targets.cpp:36](../../cs-t0/native/src/active_targets.cpp) |
| kKeyboardColRva | RVA | `0x007B191C` | [active_targets.cpp:37](../../cs-t0/native/src/active_targets.cpp) |
| kGamepadVtableRva | RVA | `0x00782C80` | [active_targets.cpp:38](../../cs-t0/native/src/active_targets.cpp) |
| kGamepadColRva | RVA | `0x007B0460` | [active_targets.cpp:39](../../cs-t0/native/src/active_targets.cpp) |
| kInputManagerRva | RVA | `0x00857B18` | [coop_passive_runtime.cpp:33](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kDeviceAssignmentOffset | OBJECT_OFFSET | `0x0000000C` | [coop_passive_runtime.cpp:51](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kKeyboardVtableRva | RVA | `0x007A1678` | [coop_passive_runtime.cpp:52](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kGamepadVtableRva | RVA | `0x00782C80` | [coop_passive_runtime.cpp:53](../../cs-t0/native/src/coop_passive_runtime.cpp) |
| kInputManagerObjectRva | RVA | `0x00857B18` | [input_runtime.cpp:23](../../cs-t0/native/src/input_runtime.cpp) |
| kInputManagerVtableRva | RVA | `0x00782950` | [input_runtime.cpp:25](../../cs-t0/native/src/input_runtime.cpp) |
| kInputManagerColRva | RVA | `0x007B0424` | [input_runtime.cpp:26](../../cs-t0/native/src/input_runtime.cpp) |
| kManagerPressedRva | RVA | `0x00620710` | [input_runtime.cpp:27](../../cs-t0/native/src/input_runtime.cpp) |
| kManagerTriggeredRva | RVA | `0x006207B0` | [input_runtime.cpp:28](../../cs-t0/native/src/input_runtime.cpp) |
| kManagerScalarRva | RVA | `0x00620850` | [input_runtime.cpp:29](../../cs-t0/native/src/input_runtime.cpp) |
| kKeyboardVtableRva | RVA | `0x007A1678` | [input_runtime.cpp:30](../../cs-t0/native/src/input_runtime.cpp) |
| kKeyboardColRva | RVA | `0x007B191C` | [input_runtime.cpp:31](../../cs-t0/native/src/input_runtime.cpp) |
| kGamepadVtableRva | RVA | `0x00782C80` | [input_runtime.cpp:32](../../cs-t0/native/src/input_runtime.cpp) |
| kGamepadColRva | RVA | `0x007B0460` | [input_runtime.cpp:33](../../cs-t0/native/src/input_runtime.cpp) |
| kLogicalPressedRva | RVA | `0x0061E8D0` | [input_runtime.cpp:34](../../cs-t0/native/src/input_runtime.cpp) |
| kLogicalTriggeredRva | RVA | `0x0061E970` | [input_runtime.cpp:35](../../cs-t0/native/src/input_runtime.cpp) |
| kLogicalScalarRva | RVA | `0x0061EA20` | [input_runtime.cpp:36](../../cs-t0/native/src/input_runtime.cpp) |
| kLogicalSetterRva | RVA | `0x0061ED50` | [input_runtime.cpp:37](../../cs-t0/native/src/input_runtime.cpp) |
| kKeyboardVtableRva | RVA | `0x007A1678` | [loop_runtime.cpp:23](../../cs-t0/native/src/loop_runtime.cpp) |
| kKeyboardColRva | RVA | `0x007B191C` | [loop_runtime.cpp:24](../../cs-t0/native/src/loop_runtime.cpp) |
| kGamepadVtableRva | RVA | `0x00782C80` | [loop_runtime.cpp:25](../../cs-t0/native/src/loop_runtime.cpp) |
| kGamepadColRva | RVA | `0x007B0460` | [loop_runtime.cpp:26](../../cs-t0/native/src/loop_runtime.cpp) |
| kKeyboardPollRva | RVA | `0x0066C650` | [loop_runtime.cpp:30](../../cs-t0/native/src/loop_runtime.cpp) |
| kGamepadPollRva | RVA | `0x00622940` | [loop_runtime.cpp:31](../../cs-t0/native/src/loop_runtime.cpp) |
| kInputManagerObjectRva | RVA | `0x00857B18` | [raw_apply_runtime.cpp:27](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kInputManagerVtableRva | RVA | `0x00782950` | [raw_apply_runtime.cpp:28](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kInputManagerColRva | RVA | `0x007B0424` | [raw_apply_runtime.cpp:29](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kKeyboardVtableRva | RVA | `0x007A1678` | [raw_apply_runtime.cpp:30](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kKeyboardColRva | RVA | `0x007B191C` | [raw_apply_runtime.cpp:31](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kGamepadVtableRva | RVA | `0x00782C80` | [raw_apply_runtime.cpp:32](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kGamepadColRva | RVA | `0x007B0460` | [raw_apply_runtime.cpp:33](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kManagerPressedRva | RVA | `0x00620710` | [raw_apply_runtime.cpp:34](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kManagerTriggeredRva | RVA | `0x006207B0` | [raw_apply_runtime.cpp:35](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kManagerScalarRva | RVA | `0x00620850` | [raw_apply_runtime.cpp:36](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kLogicalPressedRva | RVA | `0x0061E8D0` | [raw_apply_runtime.cpp:37](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kLogicalTriggeredRva | RVA | `0x0061E970` | [raw_apply_runtime.cpp:38](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kLogicalScalarRva | RVA | `0x0061EA20` | [raw_apply_runtime.cpp:39](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kLogicalSetterRva | RVA | `0x0061ED50` | [raw_apply_runtime.cpp:40](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kKeyboardCurrentStateRva | RVA | `0x00878C10` | [raw_apply_runtime.cpp:41](../../cs-t0/native/src/raw_apply_runtime.cpp) |
| kInputManagerRva | RVA | `0x00857B18` | [coop_passive_main.cpp:168](../../cs-t0/native/tests/coop_passive_main.cpp) |
| kDeviceAssignmentOffset | OBJECT_OFFSET | `0x0000000C` | [coop_passive_main.cpp:182](../../cs-t0/native/tests/coop_passive_main.cpp) |
| kKeyboardVtableRva | RVA | `0x007A1678` | [coop_passive_main.cpp:183](../../cs-t0/native/tests/coop_passive_main.cpp) |
| kGamepadVtableRva | RVA | `0x00782C80` | [coop_passive_main.cpp:184](../../cs-t0/native/tests/coop_passive_main.cpp) |
