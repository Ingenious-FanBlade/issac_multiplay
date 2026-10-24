> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-this v7 independent test results v2

记录日期：2026-09-02

状态：`PRESEAL_SCHEMA_AND_HARNESS_PASS / FORMAL_V7_NOT_RUN`

## 1. 独立测试边界

本轮由 manager-this 独立测试角色执行，只修改 create-new v7 测试与本记录。未参与逆向，未运行或附加游戏、调试器，未修改 `admission_v7` production tooling、reader、builder、反向证据、v1-v6 或状态/计划文档。

正式 `cs-t0/coop-native-join-admission-contract-v7.json` 与固定 primary raw v2 文件尚不存在，因此本记录不声称正式 manager-this PASS，也不声称 `CS-T0-05-04` 总体 PASS。

## 2. Pre-seal schema 与 relationship 结果

执行：

```powershell
python -B .\cs-t0\tests\join_admission_v7\contract_cases_v7.py --synthetic
```

结果：

```json
{"failedCases":[],"identity":"IsaacCsT0-ManagerThisContractCases-v7","inspectorCases":37,"inspectorFailedCases":[],"positiveModels":2,"status":"PASS","totalCases":63}
```

- 2 个正向 schema 模型通过：完整 `STATIC_CONFIRMED`；单 alias frontier 未闭合、保留精确 scope/no-external assumption 的合法 `UNKNOWN`。
- 63 个 contract mutation 全部拒绝，包含精确 root/scope/frontier/ref、whole-image relation、reader/global claim 分离、零权限和 `noExternalModuleOrInjectionWriterAssumption` false/missing。
- 1 个 synthetic inspector 正向关系闭环通过。
- 37 个 inspector relationship mutation 全部拒绝，覆盖 sidecar formula/anchors/writers/normal exits、whole-image primary/support/raw roles，以及 raw v2 的 exact root、target、scope、evidence class、slot、component bindings、精确八 frontier、精确 final decision 和 permissions。

Synthetic inspector 仅替换 disk hash、`bound_file` 和 PE read，以独立执行 production inspector 的关系分支；它不是正式磁盘证据。

## 3. Production reader binary harness

以当前四个实际 Win32 binary 的即时 SHA-256 spec 执行 verifier 的 pre/post-rehash harness：

- Debug：410 assertions，`PASS`，exit 0，stderr empty。
- Release：410 assertions，`PASS`，exit 0，stderr empty。

Binary SHA-256：

- Debug DLL：`CD403FA9FCEFABB1C830B5D2F0EB0E8666767E3D6A6CBE919B932D90C0225EC6`
- Release DLL：`42630C73EDB112B25D0A23BC88AE340EEF1F7E54063D4CDBCB0D0A864ADBF520`
- Debug test EXE：`008EC2F50932A029F953247C81FE13C15D84880AFBD076CF0418288D68F53CE6`
- Release test EXE：`20E0B9A9C52017F325C042D13E210DBD9C2BB5865717FDC64774E61EC0D58E8E`

## 4. 本轮测试文件

- `cs-t0/tests/join_admission_v7/contract_cases_v7.py`：`5332C6916D0486421836C0FD7A988FCFF090594630A4F1CAB4A41E14B0AD2828`
- `cs-t0/tests/join_admission_v7/verify_join_admission_v7.py`：`244ED9337F3D90CD44CBB3FCA766EDAFFF6FF4609E2CFD315B249B560E7CE034`
- `cs-t0/tests/join_admission_v7/README.md`：`B36D7EC92908E43385FBDB079BDD665640157C77F8C1D709C0E42BD9A4294FBE`

## 5. 未封印阻断与待执行 final cases

当前明确缺失：

- `cs-t0/coop-native-join-admission-contract-v7.json`；
- `out/cs-t0/native-join-manager-this-whole-image-alias-v2.json`。当前仅存在旧 v1 raw 文件。

封印后必须重新执行：

1. 固定 contract SHA 与实际 63-case mutation；
2. 精确目标 PE hash/20 anchors；
3. sidecar、whole-image certificate、raw v2、reader exact schema 与所有 nested bindings；
4. raw v2 primary/support/component binding 对齐；
5. frozen v6 formal 的 523 个唯一输入逐文件 rehash；
6. 四个固定 Debug/Release binary 的执行前后 rehash 与 410 assertions；
7. inspector/verifier report 的精确 scope 与 no-external assumption；
8. 最终只能得到 `BLOCKED_REMAINING_MANAGER_CONTEXT_PREMISES`、exit 1、`overallAccepted=false`；八个 remaining manager-context premises 保持 `UNKNOWN`，所有权限保持 false。

## 6. 残余观察

未发现本轮新增关系分支可由所列 mutations 绕过。

规范已确认锁定为 production contract literal：raw v2 `evidenceClass == "STATIC_DISASSEMBLY_AND_PE_READ_ONLY"`。先前任务交接中的 `STATIC_READ_ONLY` 只是摘要简写；本测试按锁定 literal 建模，并确认简写不会被误接受。

结论：pre-seal schema/relationship 与现有 reader harness 通过；正式 v7 artifact 缺失，formal 结果仍为未开始，不能据此提升总体状态。
