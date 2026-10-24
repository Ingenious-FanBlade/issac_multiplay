# 自有 Virtual Device：Hook 与逆向点位

日期：2026-10-22；整理：tx-03-09。范围为当前仓库已知记录，未新增运行时证据。

地址约定：主模块RVA；IDA VA = RVA + `0x00400000`，运行时VA = 实际加载基址 + RVA。对象偏移不是RVA。`SOURCE_INSTALLABLE`表示源码具有安装路径，不表示本次或所有模式均已安装；`STATIC_EXPORT_INDEX`只表示原始导出存在。适用构建与全量索引见[总入口](README.md)。

## 模块语义与状态同步用途

自有38槽device表复制RVA 0x782758，COL模板槽RVA 0x782754；覆写slot0/3/12/13/14/30。自有control表6槽不属于宿主地址patch，不混入51/70槽计数。control native caller完整语义及所有worker/OOM/EH路径仍未闭合。

原生依赖：allocator RVA0x60F4C0为cdecl；device ctor0x61E050/control ctor0xCC10为ECX this；Init0x61E210为一栈参ret4；Register0x620DF0/Remove0x620EB0为manager ECX加object/control两栈参ret8。

从BASE_CONSTRUCTED起NEVER回收；首次原生base ctor前固定DLL。退休输出中性，正常终态UNREGISTERED并保留root strong，保留有效Mutex/CS；每session至多创建一次，失败不自动重试。自有current/previous与引用计数是进程内状态，不进入网络身份。

## 源码声明的补丁、调用依赖与自有表

| 安装家族 / 序号 | 类型 | site RVA → 原目标 RVA | 用途 | 字节 / 跨度 | 源码 |
| --- | --- | --- | --- | --- | --- |
| owned_virtual_device.cpp [0] | OWNED_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有device表 slot 0: ObjectDestroy；模板RVA 0x782758，非宿主槽patch | 动态/未声明 / 动态 | [owned_virtual_device.cpp:128](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [3] | OWNED_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有device表 slot 3: Name；模板RVA 0x782758，非宿主槽patch | 动态/未声明 / 动态 | [owned_virtual_device.cpp:129](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [12] | OWNED_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有device表 slot 12: Pressed；模板RVA 0x782758，非宿主槽patch | 动态/未声明 / 动态 | [owned_virtual_device.cpp:130](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [13] | OWNED_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有device表 slot 13: Triggered；模板RVA 0x782758，非宿主槽patch | 动态/未声明 / 动态 | [owned_virtual_device.cpp:131](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [14] | OWNED_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有device表 slot 14: Scalar；模板RVA 0x782758，非宿主槽patch | 动态/未声明 / 动态 | [owned_virtual_device.cpp:132](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [30] | OWNED_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有device表 slot 30: Poll；模板RVA 0x782758，非宿主槽patch | 动态/未声明 / 动态 | [owned_virtual_device.cpp:133](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [0] | OWNED_CONTROL_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有control表 slot 0: ControlDestroy；native caller完整语义UNKNOWN | 动态/未声明 / 动态 | [owned_virtual_device.cpp:134](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [1] | OWNED_CONTROL_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有control表 slot 1: TryStrong；native caller完整语义UNKNOWN | 动态/未声明 / 动态 | [owned_virtual_device.cpp:134](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [2] | OWNED_CONTROL_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有control表 slot 2: AddStrong；native caller完整语义UNKNOWN | 动态/未声明 / 动态 | [owned_virtual_device.cpp:134](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [3] | OWNED_CONTROL_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有control表 slot 3: ReleaseStrong；native caller完整语义UNKNOWN | 动态/未声明 / 动态 | [owned_virtual_device.cpp:134](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [4] | OWNED_CONTROL_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有control表 slot 4: AddWeak；native caller完整语义UNKNOWN | 动态/未声明 / 动态 | [owned_virtual_device.cpp:134](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [5] | OWNED_CONTROL_VTABLE | 动态/自有表 → 按导入名/运行时解析 | 自有control表 slot 5: ReleaseWeak；native caller完整语义UNKNOWN | 动态/未声明 / 动态 | [owned_virtual_device.cpp:134](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [allocator] | NATIVE_CALL_DEPENDENCY | 0x0060F4C0 → 0x0060F4C0 | allocator | 动态/未声明 / 动态 | [owned_virtual_device.cpp:138](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [device base ctor] | NATIVE_CALL_DEPENDENCY | 0x0061E050 → 0x0061E050 | device base ctor | 动态/未声明 / 动态 | [owned_virtual_device.cpp:138](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [control base ctor] | NATIVE_CALL_DEPENDENCY | 0x0000CC10 → 0x0000CC10 | control base ctor | 动态/未声明 / 动态 | [owned_virtual_device.cpp:138](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [device Init] | NATIVE_CALL_DEPENDENCY | 0x0061E210 → 0x0061E210 | device Init | 动态/未声明 / 动态 | [owned_virtual_device.cpp:138](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [InputManager Register] | NATIVE_CALL_DEPENDENCY | 0x00620DF0 → 0x00620DF0 | InputManager Register | 动态/未声明 / 动态 | [owned_virtual_device.cpp:138](../../cs-t0/native/src/owned_virtual_device.cpp) |
| owned_virtual_device.cpp [InputManager Remove] | NATIVE_CALL_DEPENDENCY | 0x00620EB0 → 0x00620EB0 | InputManager Remove | 动态/未声明 / 动态 | [owned_virtual_device.cpp:138](../../cs-t0/native/src/owned_virtual_device.cpp) |

## 已知静态函数入口索引

此表按RVA合并重复导出，原标签保留；早期导出的含义可能已被后续复核更正，不能单独当作当前语义或调用授权。完整来源列表在[逆向索引JSON](reverse-point-inventory.json)。

| RVA | IDA VA | 原始研究标签 | 原始证据 |
| --- | --- | --- | --- |
| 0x0000CC10 | 0x0040CC10 | sub_40CC10 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x0060F4C0 | 0x00A0F4C0 | sub_A0F4C0 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x0061E050 | 0x00A1E050 | sub_A1E050 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x0061E210 | 0x00A1E210 | sub_A1E210 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |
| 0x00620DF0 | 0x00A20DF0 | sub_A20DF0 | [root-ida-abi.json](../../artifacts/cs-t0/tx-03-04-static/root-ida-abi.json) |

## 证据与限制

[最近固定点位与ABI复核](../../docs/refactor/cs-t0/tx-03-04-hook-calibration-reverse.md)；[现行项目规范](../../docs/refactor/project-conventions.md)。全量历史研究保留于 `docs/refactor/cs-t0/`，旧目标地址、旧流程和失败不改写。分类和收录不表示snapshot apply、全部变体或跨版本运行已经完成。
