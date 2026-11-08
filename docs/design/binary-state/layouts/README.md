# 生成布局索引

只修改 `mod/binding/state/`。未知和未核声明照实保留，不能据本表宣称原生同步已可用。

| 类型 | 布局编号 | 原生字节 | 未知字节 | 完整 |
|---|---|---|---|---|
| [Vector](Vector.md) | 1 | 8 | 0 | 是 |
| [ColorMod](ColorMod.md) | 2 | 44 | 0 | 是 |
| [KColor](KColor.md) | 3 | 20 | 0 | 是 |
| [BlendMode](BlendMode.md) | 4 | 20 | 0 | 是 |
| [AnimationState](AnimationState.md) | 5 | 32 | 3 | 否 |
| [LayerState](LayerState.md) | 6 | 160 | 4 | 否 |
| [ANM2](ANM2.md) | 7 | 276 | 14 | 否 |
| [RNG](RNG.md) | 8 | 16 | 0 | 是 |
| [BitSet128](BitSet128.md) | 9 | 16 | 0 | 是 |
| [EntityPtr](EntityPtr.md) | 10 | 4 | 0 | 是 |
| [ColorParams](ColorParams.md) | 11 | 60 | 2 | 否 |
| [ActiveItemDesc](ActiveItemDesc.md) | 12 | 32 | 0 | 是 |
| [Weapon](Weapon.md) | 13 | 68 | 20 | 否 |
| [TemporaryEffect](TemporaryEffect.md) | 14 | 16 | 4 | 否 |
| [Entity](Entity.md) | 100 | 1040 | 1032 | 否 |
| [EntityEffect](EntityEffect.md) | 101 | 1216 | 1119 | 否 |
| [EntityEffect_TearFlags](EntityEffect_TearFlags.md) | 102 | 1216 | 1119 | 否 |
| [EntityEffect_GridDesc](EntityEffect_GridDesc.md) | 103 | 1216 | 1106 | 否 |
