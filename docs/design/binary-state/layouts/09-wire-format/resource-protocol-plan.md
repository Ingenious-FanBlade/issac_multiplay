# 资源协议数字合同

布局版本 71；同源生成，不是测试报告。

| table | kind | typed布局 | 编码 |
|---|---|---|---|
| 2 | 0 | 425 ResourcePath | 完整值 |
| 1 | 1 | 426 ResourceImageFile | 完整值 |
| 6 | 2 | 427 ResourceBuiltinShader | 完整值 |
| 5 | 3 | 263 RoomConfig_Room | 完整值 |
| 1 | 4 | 307 SurfaceRecipeHeader | 具名有序recipe |
| 0 | 6 | 428 ResourceAnimation | 完整值 |
| 3 | 7 | 431 ResourceItemConfig | 完整值 |
| 4 | 8 | 432 ResourceEntityConfig | 完整值 |
| 7 | 9 | 433 ResourceAudio | 完整值 |

blend_profiles：

| 编号 | 固定值 |
|---|---|
| 0 | [0, 1, 0, 1, 0] |
| 1 | [0, 1, 7, 1, 7] |
| 2 | [0, 1, 1, 1, 1] |
| 3 | [0, 0, 2, 0, 2] |
| 4 | [0, 4, 7, 4, 7] |
| 5 | [0, 0, 2, 1, 1] |
| 6 | [0, 4, 8, 4, 7] |
| 7 | [0, 6, 7, 1, 7] |
| 8 | [0, 6, 1, 1, 1] |

sampler_profiles：

| 编号 | 固定值 |
|---|---|
| 0 | [0, 0, 0, 0] |
| 1 | [1, 0, 0, 0] |
| 2 | [0, 1, 0, 0] |
| 3 | [1, 1, 0, 0] |
| 4 | [0, 0, 1, 0] |
| 5 | [1, 0, 1, 0] |
| 6 | [0, 1, 1, 0] |
| 7 | [1, 1, 1, 0] |
| 8 | [0, 0, 0, 1] |
| 9 | [1, 0, 0, 1] |
| 10 | [0, 1, 0, 1] |
| 11 | [1, 1, 0, 1] |
| 12 | [0, 0, 1, 1] |
| 13 | [1, 0, 1, 1] |
| 14 | [0, 1, 1, 1] |
| 15 | [1, 1, 1, 1] |

固定shader用途：

| 用途号 | 名称 | 内置shader |
|---|---|---|
| 0 | SurfaceQuad | [0] |
| 1 | PresentationImage | [0] |
| 2 | Sprite | [0, 10] |
| 3 | Bloom | [2] |
| 4 | Pixelation | [1] |
| 5 | Retrovision | [6] |
| 6 | Hallucination | [8] |
| 7 | ColorMod | [9] |
| 8 | Water | [7, 11, 13] |
| 9 | Wave | [5] |
| 10 | Background | [12] |
| 11 | Fade | [0] |

kind5、305／306及Kind24正文禁用；recipe首项完整Clear，只有Clear／Quad，不含像素或任意参数字典。表元数据、闭包、实际本地物化分开，未ready不能ACK。
