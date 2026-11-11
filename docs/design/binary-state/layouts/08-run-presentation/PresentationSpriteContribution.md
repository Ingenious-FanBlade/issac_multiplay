# PresentationSpriteContribution

由唯一布局源生成。布局编号 365；布局版本 60；原生大小 0x0；未知 0B；完整：否。

记录编码预算≤1048576B；body组≤0B，超出body预算改ordinary body-joint，超记录或帧总预算报缺口，不截断。各块上限不能同时取满。

原生字节归属：

| 纯C/S协议记录：没有原生对象字节，不得向游戏地址加偏移 |
|---|

状态二进制布局（总计 160—1048576B，不含第 9 步记录头）：

块 0：PresentationSpriteContribution具名当前结果（world，160—8912920B）：

| +0x0 u16（C/S自有元数据，无原生偏移） branch：所属具名owner固定分支／slot编号；接收端按用途登记允许集。 | +0x2 i16（C/S自有元数据，无原生偏移） layer：实际layer；-1为整个固定Sprite用途，其余必须在该资源合法层内。 | +0x4 u8（C/S自有元数据，无原生偏移） track：0主、1叠加；具体贡献不得互借。 | +0x5 f32[2]（C/S自有元数据，无原生偏移） position：已核逻辑480×270 UI坐标；本地视口变换。来源仍为绝对像素且不能解释时Gap。 | +0xD ANM2（见该类型状态表）（伴随源，无根对象偏移） pose：正常临时变换后、本次CPU提交前的资源／姿态；不是函数返回后的最后Sprite。 |
|---|---|---|---|---|

证据：



伴随来源（不属于上述原生对象字节）：

- `pose` → `ANM2`：正常临时变换后、本次CPU提交前的资源／姿态；不是函数返回后的最后Sprite。；docs/design/binary-state/8-run-presentation.md；presentation-result-batches.md；presentation-source-synchronization-review.md（已核）
