# ActiveItemDesc

由唯一布局源生成。布局编号 12；布局版本 20；原生大小 0x20；未知 0B；完整：是。

REP ZHL 的 ActiveItemDesc +0x04 _unk 在 J273 AddCollectible 写入路径中已核为 item_pool；SDK 的 PartialCharge int 声明错误，J273 Lua closure 按 float 读写 +0x18。

原生字节归属：

| 0x0 i32 item_id【状态】主动道具配置 ID（4B；已核） | 0x4 i32 item_pool【状态】加入道具时记录的 ItemPool 来源（4B；已核） | 0x8 i32 charge【状态】充能数（4B；已核） | 0xC i32 battery_charge【状态】电池充能数（4B；已核） | 0x10 i32 sub_charge【状态】子充能数（4B；已核） | 0x14 i32 timed_recharge_cooldown【状态】定时充能冷却（4B；已核） | 0x18 f32 partial_charge【状态】部分充能进度；公开类型为 float（4B；已核） | 0x1C i32 var_data【状态】道具关联 VarData（4B；已核） |
|---|---|---|---|---|---|---|---|

状态二进制布局（总计 32—32B，不含第 9 步记录头）：

块 0：主动道具描述（body，32—32B）：

| +0x0 i32 item_id：主动道具配置 ID | +0x4 i32 item_pool：加入道具时记录的 ItemPool 来源 | +0x8 i32 charge：充能数 | +0xC i32 battery_charge：电池充能数 | +0x10 i32 sub_charge：子充能数 | +0x14 i32 timed_recharge_cooldown：定时充能冷却 | +0x18 f32 partial_charge：部分充能进度；公开类型为 float | +0x1C i32 var_data：道具关联 VarData |
|---|---|---|---|---|---|---|---|

证据：

- `item_id`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71
- `item_pool`：docs/design/tx-04-20/tx-04-20-state-installation.md:38；J273 AddCollectible VA 0x758F00 于 VA 0x759864 写入最后一个 item pool 参数
- `charge`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71
- `battery_charge`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71
- `sub_charge`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71
- `timed_recharge_cooldown`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71
- `partial_charge`：player-i39-source.md:71；J273 VA 0x863B93 写 getter closure 的 offset 0x18、VA 0x863BB9 以 PartialCharge 注册；VA 0x863C4E 写 setter closure offset；VA 0x8A4510/0x8A4570 以 movss 读写（imagebase 0x400000）
- `var_data`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71
