# tx-03-03：新 build relocated-canonical 实验 profile

日期：2026-10-21（Asia/Shanghai）。用户明确要求：没有旧版，先直接尝试新版。
本文记录该请求下的最小安全扩展；它不是 T0-01 新 baseline 的正式批准，也不把新 build
结果自动写成 T0-09 PASS。

## 静态事实

用户重装后的 host `isaac-ng.exe`：

- size `9362440`（`0x8EDC08`）
- PE timestamp `0x69E6E3A7`
- 6 sections，`.text/.rdata/.data/.rsrc/.reloc/.bind` 的名称、VA、raw size/off、
  characteristics 与 `target_image::kSections` 完全一致
- `SizeOfImage=0x0092C000`、entry RVA `0x008FE310`、DllCharacteristics `0x8140`
- header preferred image base `0x00400000`
- 实际加载 base 为 `0x790000`，即 Windows loader 将同一 canonical image 重定位

原 approved canonical profile 要求 loaded base 必须等于 preferred base，因此返回
`coop_active::Error::Image=2`。该拒绝是 image 检查行为，不是 Hook/ABI 运行失败。

## 用户导向的最小扩展

第一次 profile 迭代后，live-04 观察到 in-memory `ImageBase` 也会被改写为实际加载基址，
因此又新增 `CanonicalRuntimeRebasedExperimental`。当前 T0-09 实验接受两类：

1. `RelocatedCanonicalExperimental`：header base 仍为 preferred、loaded base 64KB 对齐重定位；
2. `CanonicalRuntimeRebasedExperimental`：header base 已改写为 loaded base，但 section 表仍是
   6 个 canonical sections、canonical SizeOfImage/entry。

两者都只通过 `IsApprovedT09` 暴露，且不放松 exact hook/site 检查。

原初始扩展（保留作历史）：

新增 `target_image::Profile::RelocatedCanonicalExperimental`：

- 仅接受与 approved canonical 完全相同的 PE header/section/entry/size 元数据；
- header image base 必须仍为 `0x00400000`；
- loaded base 必须非 preferred、64KB 对齐；
- 不放松 unpacked profile；
- 该 profile 只通过 `IsApprovedT09` 暴露给 T0-09 Authority/Remote；
- 既有非 T09 模式继续使用严格 `IsApproved`。

这不是新 T0-01 批准。运行仍必须经过既有 exact hook/site 准备检查；任何 target byte、
RVA、IAT/vtable、thread 或 ABI 不匹配必须让 Prepare/Fail 停止，不得继续写 world。

## 结果解释边界

下一次 live F0 如果运行：

- 成功只能标记为 **T0-09-B user-directed experimental new-build F0**；
- 不能自动标记为正式 T0-09 PASS 或新 build approved；
- 仍需后续 T0-01/T0-05 级别重新批准，才能把新 build 作为正式基线；
- 如果 hook/site 检查失败，保留失败 artifact，不绕过检查。

## 当前阻塞

Host 游戏在第三次失败时已 resident 加载 Native DLL；VM 本次未执行脚本。下一次尝试前
仍需正常重启 host（建议 VM 也重启），恢复游戏到 seed `PY1JDB20`、以撒、初始房间，
并由用户重新确认开始。
