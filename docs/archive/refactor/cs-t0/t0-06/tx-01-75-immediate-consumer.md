# tx-01-75：共享采样缓冲即时读取

后续运行更新（2026-10-18）：本轮实机确认连续完整普通采集 13,567、背压 0，消费管线和正常恢复子项通过；用户随后因全量 replay 耗时要求结束，最终 INTERRUPTED（10 PASS / 0 FAIL / 23 NOT_COVERED），没有自动重跑，完整运行验收尚未完成。见[实际结果](tx-01-75-e2e-results.md)。以下“本轮只实现/运行未执行”保留实现交付时的历史语境。

日期：2026-10-18。目标是收到就绪通知后由专用读取线程取走原始值，在编码和文件输出之前归还共享槽；本轮只实现与纯编译，不启动游戏、注入或重跑 E2E。tx-01-74 的 FAIL、18,868 次背压与最长连续 2 次的历史结果原样保留。

用户已确认：自有副本队列上限为 256 MiB、最多 256 个批次；满时不丢弃已取走的样本，保持明确背压，不阻塞游戏线程。实现将生命周期事件分块纳入同一预算，批次/事件块总数最多 256。预算包含读取中、排队及写入中的原始副本，编码临时对象和输出缓冲另计。

## 行为与边界

- 可选 world 内部 mapping 升为 v2，布局不变，增加同名 `.Ready` 自动复位通知事件。Native 完成 READY 发布后通知；生命周期事件从空变为非空、观察停止/错误及关闭也通知。通知只唤醒消费，不替代 release/acquire 状态交接。
- 专用读取线程在通知到达后扫描 READY，按 attempt 先后每次只认领一个槽，复制使用范围内的原始对象、关系和批次头，立即 FREE，再将自有副本交给写入线程。此路径不格式化字段、不校验 profile、不做 JSON 或文件 I/O，不受 coop 控制循环的 20ms 休眠影响。
- 读取线程等待自动复位事件，正常发布直接唤醒；100ms 等待超时只用于故障/停止兜底，不是正常采样消费周期。事件合并后继续扫描直到当前可取数据排空；生命周期从空变非空的判断在发布之后重读 consumed，避免检查旧值漏唤醒。
- 生命周期事件先复制到自有事件块，再推进 consumed；写入线程依次输出两个 JSONL 文件。消费者只访问共享值副本，不访问游戏对象。
- 写入线程执行既有字段转换、validate/canonicalize 与 JSONL 编码，使用缓冲文件输出；正常收尾显式 flush。原始副本预算直到该项处理完成才归还。
- 每个输出文件使用 1 MiB BufWriter，最多约 250ms 的周期主动 flush（线程调度和实际写入耗时可能延后），结束时再次 flush。计数区分原始已复制和编码/写入调用完成；只有最终 flush 成功、队列为空、三个共享槽为空、批次数等于 attempts−backpressure、事件数等于 produced 时才确认健康关闭。
- 队列预算不足时，读取线程保留尚未取走的 READY 槽，等待写入线程释放额度；Native 的三个槽仍按既有规则报告 BUFFER_BACKPRESSURE。没有覆盖、无声丢弃、扩展 odd 例外或用旧快照填补。
- 观察错误仅禁用 world 观察并记录 consumer failure，coop scheduler 不走失败路径。正常停止后先关闭发布/收敛，读取线程排空共享值，写入线程排空队列并 flush，最后 join 和释放 controller 侧 mapping。游戏侧 mapping、通知 handle 与模块延续常驻策略。
- v3 无 world 行为、coop v4 配置布局、70 槽事务、采样时刻、min-read-v1 文件 schema、身份与比较规则保持原约定；旧 JSONL 仍可离线读取。新旧 world v1/v2 二进制不能混用。

## 实现前列出的失败方式

1. 发布/等待交界丢唤醒；多个发布合并为一次通知；一次只处理一个通知导致 READY 遗留。
2. 错误地先 FREE 再复制，或保留共享引用到编码阶段；批次复用造成值撕裂。
3. 预先认领三个槽后顺序编码，使后两个仍被占用；消费顺序错乱。
4. 生命周期事件覆盖、推进 consumed 过早或与保存失败混淆，误报全部落盘。
5. 队列满、单批尺寸异常或分配失败时无限增长内存、丢已取走样本、等待游戏线程。
6. 后台异常/写入失败后线程互等、提前 unmap、遗漏 flush、未关闭发布或误判 CAPTURE_CLOSED。
7. 无 world 模式受影响，或新旧内部 mapping 错误混用。

以上按代码路径处理并保留诊断；运行保证与实际吞吐必须由后续用户安排的一次整体 E2E 验收，不以纯编译代替。

## 后续统一 E2E 需要保存的证据

用户随后要求继续 E2E。执行前为统一执行器补入 `world_record_live.rs` 源码快照，并新增 `W07.consumer_pipeline` 自动子项，检查 v2 的通知/复制/写入计数、队列预算、最终 flush 和排空；本轮因此共 33 个子项，历史 tx-01-74 的 32 项报告不变。该子项不替代实际唤醒延迟、只读/RNG 或故障场景的运行证明。没有单独试跑执行器或隔离测试。

沿用 W01–W13，新增检查 `world/consumer.json`：通知/读取/写入数量、复制最长耗时、队列峰值与预算等待次数、最终已消费/已写入数量及错误。确认原始复制结束先 FREE，再编码；稳定采集时等待不再依赖 20ms 主循环。覆盖通知合并、停止时排空和 flush、队列满时的明确缺口及最终恢复。W07 仍要求同 run/room 的 10,000 次连续普通完整采集，仅保留已确认 odd-only 例外。

| 场景 | 准备与操作 | 预期及失败判据 | 保存产物 |
| --- | --- | --- | --- |
| 正常即时消费 | 使用本轮匹配的构建；用户准备全新隔离 run，统一入口采集稳定房间 | 专用 reader 接收通知，复制后 FREE，writer 仅用自有副本；如编码/I/O 仍持有共享槽、序号错乱或值撕裂则失败。实际唤醒延迟另需运行时序证据，静态标签不充当证明 | world/consumer.json、captures.jsonl、lifecycle-events.jsonl、阶段/线程时序材料 |
| 通知合并 | 同一统一执行内覆盖多次发布合并、读取与再次等待的交界 | 一次唤醒排空当前可读取项，无 READY 或事件遗留至下一次无关通知；遗漏序号或无法唤醒为失败 | 原始样本、消费者计数、通知时序材料 |
| 队列饱和 | 未来统一入口安排明确的消费者延迟场景；本轮不添加或执行故障注入 | raw 预算不超过 256 MiB / 256 项；不覆盖已复制项，游戏不等待；READY 暂留、背压缺口保留；如无声丢弃、误报相等、预算超限或干扰 scheduler 则失败 | consumer.json、gap/事件、Authority 连续推进与延迟证据 |
| 正常停止及消费者错误 | 用户正常结束；故障路径须在未来统一执行前准备实际触发定义 | 正常排空、flush、join 后才 unmap，70 槽恢复；消费者错误仅禁用自身观察，错误与未写完计数明确；挂死、提前 unmap、错误后 CAPTURE_CLOSED 为失败 | consumer.json、consumer-failure.json（若触发）、result/restoration.json、恢复字节 |

没有运行时序或故障触发证据的场景保留 NOT_COVERED；本轮不新增单元测试，不运行隔离测试。

## 构建入口与本轮记录

本轮只调用以下纯编译入口，未调用 `build.ps1` 或任何测试脚本：

```powershell
cargo build --manifest-path .\cs-t0\Cargo.toml --locked --offline --release --target-dir .\out\cs-t0\tx-01-75-build\rust

$env:Path = 'D:\VisualStudio2022\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x86;' + $env:Path
& 'D:\VisualStudio2022\MSBuild\Current\Bin\amd64\MSBuild.exe' .\cs-t0\native\IsaacCsT0.Native.vcxproj /m:1 /nr:false /nologo /verbosity:minimal /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v143 /p:PreferredToolArchitecture=x64 /p:VCToolsVersion=14.44.35207 '/p:OutDir=D:\projects\isaac_hook_reserach\out\cs-t0\tx-01-75-build\native\' '/p:IntDir=D:\projects\isaac_hook_reserach\out\cs-t0\tx-01-75-build\native-v143-int\'
& 'D:\VisualStudio2022\MSBuild\Current\Bin\amd64\MSBuild.exe' .\cs-t0\native\IsaacCsT0.Helper.vcxproj /m:1 /nr:false /nologo /verbosity:minimal /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v143 /p:PreferredToolArchitecture=x64 /p:VCToolsVersion=14.44.35207 '/p:OutDir=D:\projects\isaac_hook_reserach\out\cs-t0\tx-01-75-build\helper\' '/p:IntDir=D:\projects\isaac_hook_reserach\out\cs-t0\tx-01-75-build\helper-v143-int\'
```

最初自动发现 VS2026 MSBuild 后，两项 C++ 源码编译完成，但链接均报 `LNK1101: MSPDB140.DLL 的版本不正确`，没有将其视为成功。改用本机已有 VS2022/v143、明确 Hostx64/x86 工具路径和版本后，Native、Helper Release 编译成功。没有安装/升级软件或更改项目工具集设置。

未来统一执行器应使用这组三项，不能与旧 world v1 Native 混搭：

- Controller：`out/cs-t0/tx-01-75-build/rust/release/IsaacCsT0.exe`
- Native：`out/cs-t0/tx-01-75-build/native/IsaacCsT0.Native.dll`
- Helper：`out/cs-t0/tx-01-75-build/helper/IsaacCsT0.Helper.exe`

构建日志保存于 [rust-build.log](../../../../../out/cs-t0/tx-01-75-build/rust-build.log)、[native-build.log](../../../../../out/cs-t0/tx-01-75-build/native-build.log)、[helper-build.log](../../../../../out/cs-t0/tx-01-75-build/helper-build.log)。[build-record.json](../../../../../out/cs-t0/tx-01-75-build/build-record.json)记录三个成功退出码、二进制路径/大小、采用的工具链及最初的链接失败，不计算或校验哈希。

状态：**实现已完成；Controller、Native、Helper Release 纯编译全部成功（exit=0）；运行验收未执行。** 没有执行单元测试、离线 CLI、游戏操作、注入或统一 E2E，没有自动重跑旧验收。编译成功不证明实际背压已经消除，不更改 tx-01-74 的失败和未覆盖结论。
