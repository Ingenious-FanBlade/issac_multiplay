# tx-03-04：Hook 校正结果

日期：2026-10-21（Asia/Shanghai）。结果：**固定点位静态校正完成；唯一整体文件/源码验收 165/165 PASS；四个构建全部退出 0。实机 Hook 安装/恢复未运行。**

## 结论与修改

当前 Steam 脱壳 `isaac-ng.exe` 的固定 Hook 地址、调用字节、目标、虚表槽与 19 个入口覆盖长度和源码一致；没有发现需要替换的 RVA。107 个去重观察位置包含 Hook、COL、调用锚点及原生依赖，不能将它表述为 107 个同时安装的 Hook。当前 C/S catalog 仍为基础 51 槽，启用 world observer 后为 70 槽。

本轮实际校正：

1. `runtime/input/boundary/raw_apply` 四个旧模式统一到既有精确镜像 profile，修正只接受 `0x92C000` 而拒绝当前 `0x8FE000` 脱壳文件的问题；RVA 边界使用匹配 profile 的实际尺寸。
2. 输入观测 11 槽和 RawApply 12 槽补齐事务内 `expected_original`，保证安装时再次要求精确原函数。
3. 初始化失败保存原始 loop 返回码及具体阶段/索引/Win32 错误/已知点位 RVA，image/conflict 早退发布失败事件；T09 日志补齐 `valueBits`。
4. 澄清 live-05 的泛化 `Error::Hook` 不能证明 exact-site 拒绝；保留原始失败，未猜测根因或关闭检查。

GPT 6 Luna / max 完成静态逆向及变更审阅，主智能体补充原始 IDA 导出并复核。用户原指定的 GPT 6.1 Luna 不在工具模型列表中；在实际返回 Unknown model 后，用户明确选用 GPT 6 Luna / max。

## 唯一整体执行

实际执行且只执行一次：

```powershell
python cs-t0/accept-native-hook-calibration.py `
  --game-exe 'F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe' `
  --artifact-dir artifacts/cs-t0/tx-03-04-acceptance-01 `
  --runtime-readonly
```

退出码 **0**，没有重跑。全部分析和修改结束后才调用该入口；没有新增/运行单元测试，未运行旧含多组测试及哈希校验的构建入口。

| 检查类别 | 数量 | 结果 |
| --- | ---: | --- |
| relative call 声明（包含跨家族重复点及锚点） | 79 | 全部匹配实际 PE 字节及目标 |
| 固定指针/内嵌绝对地址 | 33 | 全部匹配 |
| world 入口 | 19 | 全部匹配 |
| COL 指针 | 6 | 全部匹配 |
| Lua IAT 调用者 | 10 | 全部匹配 |
| 主循环 IAT RVA | 3 | 全部匹配 |
| 命名导入与所属 DLL | 8 | 全部匹配 |
| menu 指令片段 | 3 | 全部匹配 |
| owned device ctor/Init 特征 | 3 | 全部匹配 |
| PE 元数据 | 1 | 匹配现有 profile |
| 合计 | **165** | **165/165 PASS** |

同一入口内纯编译 `IsaacCsT0T09`、普通 Native、`WorldE2E=true` Native 和 Helper，四项均退出 **0**。WorldE2E 仅编译，没有执行其实机场景。Rust 编译保留两条已有 `unused_mut` 警告，未为此扩大修改范围。

## 当前进程观察与限制

只读采集结果为 `NOT_READ`，原因是没有发现运行中的 `isaac-ng.exe`（`candidate_count=0`）。没有读取到实际内存点位、IAT owner、驻留模块或当前 header ImageBase，也没有为此启动游戏、重试、注入、发送输入、暂停线程或附着调试器。

因此本次不能证明当前进程可安装，也不能给出 live-05 的实际失败阶段。没有新的主线程时序、exactly-once 或恢复实测结论；历史恢复结果仍按其原场景保留。

仍为 UNKNOWN：动态 Steam 模块三个 vtable 槽的当前目标/ABI、world18 八个参数的完整高层语义、自有 control vtable 各槽的 native caller 语义。world18 的实际 `ret 0x20`、wrapper 参数词数和清栈已确认；这些语义未知不等于地址或栈布局已发现不符。既有 NEVER 回收和模块常驻策略未改变。

## 产物与复现

统一产物目录：`artifacts/cs-t0/tx-03-04-acceptance-01/`。

- `summary.json`：实际计数、构建命令/退出码、执行边界与只读采集结果。
- `sites.json`：逐条源码位置/符号、RVA、期望与实际字节、相对调用目标和结果。
- `target-image.bin`：此次读取的真实 PE 字节副本，仅作数据保存，未加载执行。
- `native-source/`、`authority_game.rs`、验收器副本：此次源码快照。
- `Rust-build.log`、`Native-build.log`、`Native-WorldE2E-build.log`、`Helper-build.log`：完整构建记录。
- `runtime-readonly.json`：未找到游戏的原始观察。

复现同一次文件检查时，可将 `--game-exe` 改为保存的 `target-image.bin`，并使用新的产物目录；若源码已变化，应与保存的源码快照对应。不自动执行复现，也不使用哈希。

目标与实施细节见 [校正说明](tx-03-04-hook-calibration.md)，ABI 与原始 IDA 索引见 [逆向记录](tx-03-04-hook-calibration-reverse.md)。
