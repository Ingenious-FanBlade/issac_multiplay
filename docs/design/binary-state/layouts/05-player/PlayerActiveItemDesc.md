# PlayerActiveItemDesc

由唯一布局源生成。布局编号 179；布局版本 58；原生大小 0x20；未知 0B；完整：是。

Player专用P4四槽视图；空item时五值规范零且不读取旧payload。来源池和调度冷却留Host。

原生字节归属：

| 0x0 i32 item_id【状态】主动道具配置 ID；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。（4B；已核） | 0x4 i32 item_pool【权威】Host获得／保存／掉落时的来源池，AddCollectible VA0x759864写入；当前Player/HUD展示不读取。 不采集／编码／安装或比较原值。（4B；已核） | 0x8 i32 charge【换算】充能数；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。（4B；已核） | 0xC i32 battery_charge【换算】电池充能数；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。（4B；已核） | 0x10 i32 sub_charge【换算】子充能数；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。（4B；已核） | 0x14 i32 timed_recharge_cooldown【权威】Host主动道具充能更新输入，VA0x789E50/0x78A35B等消费；HUD读取最终charge／partial等，不推进本地充能。 不采集／编码／安装或比较原值。（4B；已核） | 0x18 f32 partial_charge【换算】部分充能进度；公开类型为 float；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。（4B；已核） | 0x1C i32 var_data【换算】道具关联 VarData；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。（4B；已核） |
|---|---|---|---|---|---|---|---|

状态二进制布局（总计 24—24B，不含第 9 步记录头）：

块 0：展示值（world，24—24B）：

| +0x0 i32 item_id：主动道具配置 ID；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 | +0x4 i32 charge：充能数；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 | +0x8 i32 battery_charge：电池充能数；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 | +0xC i32 sub_charge：子充能数；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 | +0x10 f32 partial_charge：部分充能进度；公开类型为 float；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 | +0x14 i32 var_data：道具关联 VarData；PlayerHUD::RenderActiveItem VA0x835B40直接或经7897C0/789810消费，保留最终值。 |
|---|---|---|---|---|---|

证据：

- `item_id`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `item_pool`：docs/design/tx-04-20/tx-04-20-state-installation.md:38；J273 AddCollectible VA 0x758F00 于 VA 0x759864 写入最后一个 item pool 参数；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `charge`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `battery_charge`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `sub_charge`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `timed_recharge_cooldown`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `partial_charge`：player-i39-source.md:71；J273 VA 0x863B93 写 getter closure 的 offset 0x18、VA 0x863BB9 以 PartialCharge 注册；VA 0x863C4E 写 setter closure offset；VA 0x8A4510/0x8A4570 以 movss 读写（imagebase 0x400000）；docs/design/binary-state/player-equipment-plus-synchronization-review.md
- `var_data`：F:\projects_isaac\REPENTOGON-1.1.3\libzhl\functions\ActiveItemDesc.zhl；docs/design/tx-04-20/tx-04-20-state-installation.md:37-38；player-i39-source.md:71；docs/design/binary-state/player-equipment-plus-synchronization-review.md
