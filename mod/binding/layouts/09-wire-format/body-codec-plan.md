# 各域正文数字编解码计划

布局版本 94；由同一布局源生成，不是运行测试报告。

共371种布局、2673个字段计划、25个条件字段；最大根字段数128。无负载的Host权威类型只登记空值布局，不补造状态。

条件、资源空间、整数／浮点校验、计数组和规范零负载在构建时转成数字规则；正文热路径不按名称解释。字段／引用的图关系、资源ready和原生应用成功仍由完整目标处理。

当前禁用正文布局：305 SurfacePixelsDescriptor、306 ResourceBlobChunkHeader。保留固定字节表不表示允许媒体下行。

| 域源 | 布局数（含继承／变体） | 字段数 |
|---|---|---|
| base.json | 16 | 84 |
| entity.json | 2 | 44 |
| entity-effect.json | 18 | 669 |
| entity-classes.json | 26 | 826 |
| entity-text-dummy.json | 3 | 131 |
| player.json | 48 | 223 |
| grid.json | 5 | 40 |
| grid-classes.json | 27 | 203 |
| room.json | 54 | 130 |
| level.json | 18 | 34 |
| run.json | 9 | 57 |
| presentation.json | 78 | 1 |
| hud-local.json | 2 | 0 |
| display.json | 16 | 97 |
| overlay-display.json | 1 | 8 |
| random.json | 24 | 0 |
| wire.json | 24 | 126 |

## 条件数字规则

集合mask的第n位表示允许值n；item-config的mask表示配置type集合，operand表示配置id。

| 布局／字段 | 先行selector | 操作 | operand | 集合mask |
|---|---|---|---|---|
| 164/1 current_node | 0 | equal | 1 | 0 |
| 164/2 sprite | 0 | equal | 1 | 0 |
| 180/2 cooldown | 0 | item-config | 704 | 26 |
| 214/10 position | 9 | equal | 1 | 0 |
| 215/13 main_state | 12 | equal | 1 | 0 |
| 239/19 shockwave_overlay_sprite | 18 | equal | 1 | 0 |
| 240/4 floor_sprite | 3 | any-bits | 1 | 0 |
| 240/5 wall_detail_sprite | 3 | any-bits | 2 | 0 |
| 240/6 controls_sprite | 3 | any-bits | 4 | 0 |
| 240/7 controls_buttons_sprite | 3 | any-bits | 8 | 0 |
| 253/1 sprite | 0 | equal | 1 | 0 |
| 253/2 display_offset | 0 | equal | 1 | 0 |
| 253/3 accumulated_offset | 0 | equal | 1 | 0 |
| 270/12 spawn_seed | 11 | equal | 1 | 0 |
| 297/1 frame_bit0 | 0 | any-bits | 1 | 0 |
| 297/2 frame_bit1 | 0 | any-bits | 2 | 0 |
| 297/3 frame_bit3 | 0 | any-bits | 8 | 0 |
| 297/4 frame_bit4 | 0 | any-bits | 16 | 0 |
| 297/5 frame_bit7 | 0 | any-bits | 128 | 0 |
| 297/6 frame_bit8 | 0 | any-bits | 256 | 0 |
| 297/7 frame_bit9 | 0 | any-bits | 512 | 0 |
| 297/8 frame_bit11 | 0 | any-bits | 2048 | 0 |
| 297/9 frame_bit13 | 0 | any-bits | 8192 | 0 |
| 298/1 payload | 0 | equal | 1 | 0 |
| 433/3 intro | 1 | equal | 1 | 0 |
