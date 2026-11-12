# 各域正文数字编解码计划

布局版本 71；由同一布局源生成，不是运行测试报告。

共352种布局、2838个字段计划、96个条件字段；最大根字段数117。无负载的Host权威类型只登记空值布局，不补造状态。

条件、资源空间、整数／浮点校验、计数组和规范零负载在构建时转成数字规则；正文热路径不按名称解释。字段／引用的图关系、资源ready和原生应用成功仍由完整目标处理。

当前禁用正文布局：305 SurfacePixelsDescriptor、306 ResourceBlobChunkHeader。保留固定字节表不表示允许媒体下行。

| 域源 | 布局数（含继承／变体） | 字段数 |
|---|---|---|
| base.json | 16 | 84 |
| entity.json | 2 | 44 |
| entity-effect.json | 18 | 670 |
| entity-classes.json | 26 | 826 |
| entity-text-dummy.json | 3 | 131 |
| player.json | 48 | 209 |
| grid.json | 5 | 40 |
| grid-classes.json | 27 | 206 |
| room.json | 54 | 209 |
| level.json | 18 | 41 |
| run.json | 9 | 40 |
| presentation.json | 77 | 211 |
| hud-local.json | 2 | 12 |
| random.json | 24 | 0 |
| wire.json | 23 | 115 |

## 条件数字规则

集合mask的第n位表示允许值n；item-config的mask表示配置type集合，operand表示配置id。

| 布局／字段 | 先行selector | 操作 | operand | 集合mask |
|---|---|---|---|---|
| 164/1 current_node | 0 | equal | 1 | 0 |
| 164/2 sprite | 0 | equal | 1 | 0 |
| 180/2 cooldown | 0 | item-config | 704 | 26 |
| 214/10 position | 9 | equal | 1 | 0 |
| 215/13 main_state | 12 | equal | 1 | 0 |
| 239/24 shockwave_overlay_sprite | 23 | equal | 1 | 0 |
| 240/5 floor_sprite | 4 | any-bits | 1 | 0 |
| 240/6 wall_detail_sprite | 4 | any-bits | 2 | 0 |
| 240/7 controls_sprite | 4 | any-bits | 4 | 0 |
| 240/8 controls_buttons_sprite | 4 | any-bits | 8 | 0 |
| 242/1 scene | 0 | equal | 1 | 0 |
| 253/1 sprite | 0 | equal | 1 | 0 |
| 253/2 display_offset | 0 | equal | 1 | 0 |
| 287/1 payload | 0 | equal | 1 | 0 |
| 288/1 shadow_fill | 0 | any-bits | 1 | 0 |
| 288/2 light_tint | 0 | any-bits | 2 | 0 |
| 288/3 lighting_alpha | 0 | any-bits | 4 | 0 |
| 288/4 special_background | 0 | any-bits | 8 | 0 |
| 289/1 a_world_center | 0 | any-bits | 1 | 0 |
| 289/2 a_radius_used | 0 | any-bits | 1 | 0 |
| 289/3 a_strength | 0 | any-bits | 1 | 0 |
| 289/4 a_radius_step | 0 | any-bits | 1 | 0 |
| 289/5 a_elapsed | 0 | any-bits | 1 | 0 |
| 289/6 a_duration | 0 | any-bits | 1 | 0 |
| 289/7 b_world_center | 0 | any-bits | 2 | 0 |
| 289/8 b_radius_used | 0 | any-bits | 2 | 0 |
| 289/9 b_strength | 0 | any-bits | 2 | 0 |
| 289/10 b_radius_step | 0 | any-bits | 2 | 0 |
| 289/11 b_elapsed | 0 | any-bits | 2 | 0 |
| 289/12 b_duration | 0 | any-bits | 2 | 0 |
| 290/1 focus | 0 | equal | 1 | 0 |
| 291/1 group | 0 | equal | 1 | 0 |
| 293/1 payload | 0 | equal | 1 | 0 |
| 295/1 payload | 0 | equal | 1 | 0 |
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
| 300/1 focus_safe_index | 0 | equal | 1 | 0 |
| 300/2 descriptor_member | 0 | equal | 1 | 0 |
| 310/1 visible_payload | 0 | equal | 1 | 0 |
| 311/1 visual_id | 0 | equal | 2 | 0 |
| 311/2 item_image | 0 | equal | 2 | 0 |
| 312/1 visual_id | 0 | equal | 2 | 0 |
| 312/3 image | 0 | equal | 2 | 0 |
| 312/4 draw_center | 0 | equal | 2 | 0 |
| 319/1 main_text | 0 | equal | 1 | 0 |
| 319/2 sub_text | 0 | equal | 1 | 0 |
| 319/3 streak | 0 | equal | 1 | 0 |
| 353/5 overlay_animation | 0 | equal | 1 | 0 |
| 354/9 row_8 | 0 | equal | 1 | 0 |
| 357/1 alpha | 0 | equal | 1 | 0 |
| 358/1 alpha | 0 | equal | 1 | 0 |
| 359/1 color | 0 | not-equal | 0 | 0 |
| 359/2 effect | 0 | equal | 2 | 0 |
| 359/3 name_image | 0 | not-equal | 0 | 0 |
| 359/4 pill_image | 0 | not-equal | 0 | 0 |
| 361/1 items | 0 | equal | 1 | 0 |
| 367/1 amount | 0 | equal | 1 | 0 |
| 368/1 amount | 0 | equal | 1 | 0 |
| 369/1 amount | 0 | equal | 1 | 0 |
| 370/1 amount | 0 | equal | 1 | 0 |
| 372/2 image | 0 | equal | 1 | 0 |
| 374/1 sprites | 0 | equal | 1 | 0 |
| 375/1 sprites | 0 | equal | 1 | 0 |
| 375/2 images | 0 | equal | 1 | 0 |
| 376/1 body | 0 | equal | 1 | 0 |
| 376/2 killer_config | 0 | equal | 1 | 0 |
| 376/3 sprites | 0 | equal | 1 | 0 |
| 376/4 images | 0 | equal | 1 | 0 |
| 378/1 overlay_id | 0 | not-equal | 0 | 0 |
| 378/2 player | 0 | equal | 2 | 0 |
| 378/3 sprite | 0 | equal | 1 | 0 |
| 378/4 mega_mush | 0 | equal | 2 | 0 |
| 379/1 sprite | 0 | equal | 1 | 0 |
| 380/1 text_0 | 0 | equal | 1 | 0 |
| 380/2 text_1 | 0 | equal | 1 | 0 |
| 380/3 text_2 | 0 | equal | 1 | 0 |
| 380/4 text_3 | 0 | equal | 1 | 0 |
| 380/5 text_4 | 0 | equal | 1 | 0 |
| 380/6 selection | 0 | equal | 1 | 0 |
| 380/7 paper | 0 | equal | 1 | 0 |
| 383/1 parameters | 0 | one-of | 0 | 19 |
| 422/1 font | 0 | equal | 1 | 0 |
| 422/2 style | 0 | equal | 1 | 0 |
| 422/3 color | 0 | equal | 1 | 0 |
| 422/4 color_space | 0 | equal | 1 | 0 |
| 422/5 utf8 | 0 | equal | 1 | 0 |
| 423/1 lines | 0 | equal | 1 | 0 |
| 433/3 intro | 1 | equal | 1 | 0 |
