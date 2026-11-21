# 2：基础值类型与嵌套规则：生成布局

[返回布局总索引](../README.md) · [对应总计划步骤](../../../../docs/archive/design/binary-state/2-base-types.md)

按布局编号排列；共享类型通过引用复用，不重复生成。未知字节保留原生事实；需求覆盖只说明清单字段已生成计划，不表示采集、应用或验收已完成。

| 类型 | 布局编号 | 原生字节 | 未知字节 | 需求覆盖 |
|---|---|---|---|---|
| [Vector](Vector.md) | 1 | 8 | 0 | 是 |
| [ColorMod](ColorMod.md) | 2 | 44 | 0 | 是 |
| [KColor](KColor.md) | 3 | 20 | 0 | 是 |
| [BlendMode](BlendMode.md) | 4 | 20 | 0 | 是 |
| [AnimationState](AnimationState.md) | 5 | 32 | 3 | 是 |
| [LayerState](LayerState.md) | 6 | 160 | 4 | 是 |
| [ANM2](ANM2.md) | 7 | 276 | 14 | 是 |
| [RNG](RNG.md) | 8 | 16 | 0 | 是 |
| [BitSet128](BitSet128.md) | 9 | 16 | 0 | 是 |
| [EntityPtr](EntityPtr.md) | 10 | 4 | 0 | 是 |
| [ColorParams](ColorParams.md) | 11 | 60 | 2 | 是 |
| [ActiveItemDesc](ActiveItemDesc.md) | 12 | 32 | 0 | 是 |
| [Weapon](Weapon.md) | 13 | 68 | 20 | 是 |
| [TemporaryEffect](TemporaryEffect.md) | 14 | 16 | 4 | 是 |
| [WeaponUmbilicalWhip](WeaponUmbilicalWhip.md) | 135 | 72 | 20 | 是 |
| [TemporaryEffects](TemporaryEffects.md) | 136 | 24 | 2 | 是 |
