# tx-04-04 退菜单修订的静态依据

2026-10-24，用户要求先修复第六次 E2E 进入 Options 的按键导航，再进行下一次整体 E2E。本目录只有对现有 v1.9.7.12 IDB 的静态读取，没有启动游戏、运行探针或计算哈希。

- 目标映像基址 0x400000。SDK `PauseScreen.zhl` 的 Show 特征对应 VA 0x98ED80（RVA 0x58ED80），见 pause-show.json/.c。原生 Show 在 0x98EE29 写 state=1，在 0x98EE31 写 selectedelement=1。
- SDK `Game::_pauseScreen` 偏移 0x2363C。已有 manager-update.c 的 VA 0x92E7E4/0x92E7F5 将 Game+144956（0x2363C）分别传入 PauseScreen 更新和渲染，和 SDK 一致。PauseScreen 内 state=+0、selectedelement=+8。
- pause-update.json/.c 为 VA 0x98CCC0。state=1 是暂停主菜单；0x98CED3/0x98CF03 对 selectedelement 向上/下移动并在 0..2 循环。确认输入分支在 0x98D204 起：1 调用暂停关闭；2 进入退出流程；0 在 0x98D404 设置 state=2 并打开 Options。state=2 的返回输入在 0x98D443 将 state 改为 1。
- 因此从 Show 的默认 1 连续 DOWN 两次会得到 0，确认后进入 Options。退出的选择编号为 2；从 1 只需一次 DOWN，从 0 一次 UP。
- 普通非 daily run 的退出选择直接进入原生退出过渡；daily run 另有 state=3 的确认分支。本次只覆盖现有普通 New Run 场景，不添加自动处理 daily/未知弹窗。
- pause-close.json/.c 和 game-exit.json/.c 保留暂停关闭/退出的原始反编译，不能用构建成功替代实机退出观察。

实现只增加主线程发布的 pause_state/pause_selection 只读字段。harness 保持物理按键：先按观测状态打开暂停菜单或离开 Options，等待 state=1；每次最多移动一格，观察 selection=2 后才确认；未知状态或输入未达到预期即失败，不重发按键。保存初始/暂停/退出选项/返回状态及截图，并继续原有 Game::Exit 计数检查。未触及原版 Online、世界对象写入或存档允许清单。

整体运行已在用户确认准备好后执行一次，目录为 `../tx-04-04-e2e-07/`。两次启动分别提示跳过动画，到标题/选档页后停止手动操作。原始步骤与截图确认 selection=2 后退出到 menu_id=3，Game::Exit 计数 0→1；整体仍 FAIL（E3/E5/E7），完整说明见结果文档。没有自动重跑。
