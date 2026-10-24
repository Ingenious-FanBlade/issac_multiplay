# tx-04-04 第三次 main_thread_timeout：静态分析与候选修复

日期：2026-10-24。此目录记录静态分析与纯构建，不是一次 E2E。

- 已配置 ida-mcp 端点返回的 server_health 确认当前 IDB 是 v1.9.7.12，image base 0x400000；JSON 保留原始反编译，旁边的 .c 只是可读导出，没有作为程序执行。
- manager-update：RVA 0x52E3E0；isaac-main：RVA 0x50AC60，在 0x50AF08、0x50AF0D 依次调用 Manager::Update 与 Render。
- manager-render：RVA 0x52EC40，正常渲染路径包含 SwapBuffers；window-events：RVA 0x62AC40；native-window-pump：RVA 0x62A390，使用 PeekMessage/DispatchMessage；native-clock：RVA 0x5F9690，使用 QPC 或平台计时。
- manager-focus-loop 的文件名来自最初的调查假设；其内容实际为 RVA 0x5346B0 的设备/断连处理，不构成已确认的失焦等待循环。保留原始文件名和结果，不把假设写成事实。
- 快照的 PauseOnFocusLost=1、SkipIntro=0，以及 harness 在状态轮询之前不调用 focus，为候选修复提供依据；第三次没有关闭前的前台状态，无法确认实际失焦。IDA 也没有证明 Intro 必定阻塞 Update。
- 第三次第一条端口 command 到 shutdown 的 QPC 时间约 0.287 秒；源码的五秒截止不等同于已观察的五秒等待。新实现增加两侧耗时和请求生命周期日志，后续实机观察才能解释差异。
- build-first-attempt.log 保留 Rust Result 序列化 E0277；build-final.log 是修正后纯构建成功记录。没有运行单元测试、游戏试跑、哈希校验或其他实验。

完整修改、用户授权和验收结果见 docs/design/tx-04-04-results.md 的后续章节。前三次 E2E 的原始产物没有改动。
