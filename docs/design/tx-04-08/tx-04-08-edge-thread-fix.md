# tx-04-08 物理边沿与观察故障分类修复

日期：2026-10-29，Asia/Singapore。同一tx-04-08完整目标：修复e2e-12的703输入丢边沿、错误线程首因分类，然后执行一次统一整体E2E。分析修改期间零测试，不新增单元测试；保留原失败与未覆盖项。

## 物理边沿

直接读取e2e-12原始trace：seq41766的sample3757 / loop4420记录SPACE对应held2 / edge2；seq41767的下一采样sample3758 / loop4421变为edge0；随后seq41769首次submit只带held2 / edge0。实际tick69/70均未获得Triggered。旧source只有一份sample，非Authority轮次的上升沿被后续poll覆盖，不能用单次SPACE已发出证明原生Item consumer收到触发。

`game/input_a.cpp`增加16项固定FIFO，每项只保存尚未提交的edge mask和物理sample序号。只在当前有效会话、A唯一exact pair、无pause/loading/console/生命周期阻断时追加。每个新鲜Authority-pre从FIFO取最旧一项，连续量仍来自本轮新鲜样本，经原有MakeCommand/codec/AcceptInput/queue/Resolve提交；admission成功才消费一次。连续按住不重复追加，重复真实上升沿不按位合并丢失；每个Authority-pre最多一项，缓冲满则明确physical_edge_queue_full并有序停用，不静默丢边沿。trace新增edge_sample关联原始采样。

FIFO保存session/participant generation；失效、换代、设备/绑定变化、屏蔽、退休和失败清空。物理held历史照常推进，恢复不为旧held补造edge。仍要求本轮新鲜source/exact pair才能Submit；首份启动命令和lead2连续补帧沿用现有规则，补帧清edge。物理poll不写current/previous、不绕过统一scheduler，不更改B协议或消费者读取语义。

本修复不放宽703动作的条件，不改为直接Lua UseActiveItem，不重发SPACE，也不以角色类型变化代替真实Player Init与成功Replace证据。

## 故障分类

旧world桥在帧首把任意ObservationFailureSequence变化当WrongThread，提交前却统一标成UnpairedScope。实际worker线程通知可在帧内发生，因此e2e-12的线程故障首因随收束位置改变。

`world/observe.*`新增wrong_thread/native_exception分别累计的原子通知计数，主线程桥在attach时建立基线，帧首及提交前共用ObserveFailures。线程增量输出WRONG_THREAD / observation_wrong_thread；原生异常增量保留UNPAIRED_SCOPE / observation_native_exception，两者同时到达时后者优先，trace保存两类增量。历史累计故障不污染新录制，不访问worker线程中的游戏对象/身份表，不改变Host故障归属或原生original调用次数。原有collector首因仍保留。

验收仍要求thread得到WRONG_THREAD，不降低fault_evidence判据。异常展开、缓冲溢出及并发混合故障未因本修复提前宣称运行通过。

## 构建与执行

本次构建日志目录为artifacts/repentogon/tx-04-08-edge-thread-build，harness随源码冻结该目录。先完成纯DLL/Rust编译，再执行一次统一E2E至新artifact；仍采用主250/对照32份、30秒采集上限、原场景与最终全量归约。按会话既有指示无需询问准备，逐次启动前仍说明停止手动操作，失败不自动重跑。

实际编译、运行结果另记，不预记成功。
