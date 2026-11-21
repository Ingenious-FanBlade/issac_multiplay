# tx-01-60：B=1 角色与剩余动作实机 E2E

日期：2026-10-17。用户要求现在开始E2E，B选择ID=1，并确认已完全重启、隔离Offline初始仅A、安全房间，可为B准备药丸/卡牌。沿用本会话实际实机操作授权，允许本目标的单次注入、短移动/射击、原生拾取/使用、暂停/恢复、B冲突退休、回菜单和正常恢复；不自动生成资源或修改存档/解锁。

目标：复用tx-01-58同轮成功构建的controller/native/helper，验证请求/工厂/实际B角色均为1（Magdalene）；关闭此前未观察的B Pill/Card consumer资源变化和action10 held；必要双端输入、队列拒绝、纯读与B退休/A继续、菜单teardown和51Hook恢复在同一捕获中观察。此前已完成的Loading、死亡/复活与异常Join专项结论不重跑、不外推。

分析与修改阶段零测试。统一入口 `cs-t0/accept-character-pill-e2e.ps1` 只执行一次，目录 `artifacts/cs-t0/tx-01-60-live-01`；首次进程/窗口检查、桌面观察、注入、输入、恢复与报告均属于这次验收。失败不重跑，不卸载/重新注入常驻DLL。

## 预先列出的失败方式

- 不是唯一新游戏进程、已驻留旧DLL、非Offline或非仅A场景；不猜测重注入。
- B请求/工厂参数/实际subtype不是1，角色不可选仍调用工厂；A身份/Join健康变化，或重复创建。
- 只有Pill/Card edge或查询返回值，没有资源字段变化；空槽不能判实际消费PASS。
- action10 held没有实读，或把诊断companion Pressed误称为原生引擎主动调用Pressed。
- Resolve缺accepted command/generation、重复apply/consumer、A/B串扰、旧输入恢复；事件覆盖或原生/传输错误。
- B未持有资源就结束测试；缺观察记NOT_OBSERVED。恢复不完整或进程异常记FAIL。

## 场景与产物

入口先记录唯一进程及模块，停在scene-check。观察安全场景后同一次入口继续注入。短脚本自动执行一次，B稳定后由用户准备可实际消费的普通药丸/卡牌；记录资源就绪标记，开启`queries`，发送`script`，新sequence走同scheduler。脚本移动/射击短暂；tick60发送Pill held+edge和重复同包、随后保持30tick再释放，之后Item及非法/过期/旧generation/wrong-session输入。没有资源时的初次意图不记实际消费。

每轮script只是同一捕获中的正常新鲜输入，不新建session/设备/Player。完成实际消费和短A输入后发`conflict-b`，观察B退休、A继续，再回主菜单`stop`。桌面操作/工具结果写`desktop-operations.jsonl`；原始capture报告保留为`full-feature-result.json`，本轮报告只判本目标并列明未在本轮覆盖的完整功能项。

静态说明：原生Pill consumer RVA7A3D30通过9561E0查询Triggered10；`queries`可在真实consumer/action/exact-binding上下文执行另外两种纯读接口，source=2明确标为controlled companion。本轮held10若仅由companion覆盖，将如实记录，不能写成引擎主动Pressed10。Pill实际消费要求同一有效consumer前后资源字段（17A0/17A4/17A8）变化，不以动作flag单独代替。

## 本轮唯一入口结果

入口一次执行，在process-and-artifacts阶段退出1：受限终端Get-Process观察到0个目标，未注入、未安装Hook、未创建B、未发送游戏输入。`artifacts/cs-t0/tx-01-60-live-01/result.json`保留FAIL和原始原因。后续Isaac MCP只读诊断返回实际唯一游戏PID45496、creation_time_100ns=134367129629180556；可访问宿主进程的只读Get-Process同样确认该PID及游戏路径，因此这是执行环境可见性失败，不能说游戏未运行。

用户补充B=1自带药丸，可用于后续实际消费。本轮不重跑；后续实质修复入口身份绑定与执行环境，见[tx-01-61](tx-01-61-live-process-visibility.md)。
