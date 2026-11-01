# tx-04-10 Entity 逐成员来源目录

由 `mod/tools/gen_entity_catalog.py` 从两套本地 Wiki 生成。实现与实机覆盖分别记录；目录行不等于运行通过。继承字段由 Entity 统一管理。参数查询保留完整签名与默认值；无界向量／字符串查询按显式请求采集，不假称遍历全部参数。

| 规范字段／接口 | 签名 | 分类及实现 | 来源 |
| --- | --- | --- | --- |
| `ActiveItemDesc.BatteryCharge` | `int BatteryCharge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PlayerTypes_ActiveItemDesc.md)（行23） |
| `ActiveItemDesc.Charge` | `int Charge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PlayerTypes_ActiveItemDesc.md)（行28） |
| `ActiveItemDesc.Item` | `CollectibleType Item` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PlayerTypes_ActiveItemDesc.md)（行33） |
| `ActiveItemDesc.PartialCharge` | `float PartialCharge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PlayerTypes_ActiveItemDesc.md)（行38） |
| `ActiveItemDesc.SubCharge` | `int SubCharge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PlayerTypes_ActiveItemDesc.md)（行44） |
| `ActiveItemDesc.TimedRechargeCooldown` | `int TimedRechargeCooldown` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PlayerTypes_ActiveItemDesc.md)（行49） |
| `ActiveItemDesc.VarData` | `int VarData` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PlayerTypes_ActiveItemDesc.md)（行54） |
| `AnimationData.GetAllLayers` | `AnimationLayer[] GetAllLayers ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationData.md)（行14） |
| `AnimationData.GetLayer` | `AnimationLayer GetLayer ( int LayerId )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationData.md)（行19） |
| `AnimationData.GetLength` | `int GetLength ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationData.md)（行24） |
| `AnimationData.GetName` | `string GetName ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationData.md)（行29） |
| `AnimationData.IsLoopingAnimation` | `boolean IsLoopingAnimation ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationData.md)（行33） |
| `AnimationFrame.GetColor` | `const Color GetColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行18） |
| `AnimationFrame.GetCrop` | `const Vector GetCrop ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行22） |
| `AnimationFrame.GetEndFrame` | `int GetEndFrame ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行26） |
| `AnimationFrame.GetHeight` | `float GetHeight ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行33） |
| `AnimationFrame.GetPivot` | `const Vector GetPivot ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行37） |
| `AnimationFrame.GetPos` | `const Vector GetPos ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行41） |
| `AnimationFrame.GetRotation` | `float GetRotation ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行45） |
| `AnimationFrame.GetScale` | `const Vector GetScale ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行49） |
| `AnimationFrame.GetStartFrame` | `int GetStartFrame ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行53） |
| `AnimationFrame.GetWidth` | `float GetWidth ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行57） |
| `AnimationFrame.IsInterpolated` | `boolean IsInterpolated ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行61） |
| `AnimationFrame.IsVisible` | `boolean IsVisible ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationFrame.md)（行65） |
| `AnimationLayer.GetFrame` | `AnimationFrame GetFrame ( int Frame )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationLayer.md)（行14） |
| `AnimationLayer.GetLayerID` | `int GetLayerID ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationLayer.md)（行18） |
| `AnimationLayer.IsVisible` | `boolean IsVisible ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/AnimationLayer.md)（行22） |
| `BitSet128.BitSet128` | `BitSet128 BitSet128 ( int Low = 0, int High = 0 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行19） |
| `BitSet128.Get` | `boolean Get ( int BitPosition )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行86） |
| `BitSet128.Set` | `void Set ( int BitPosition, boolean State )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行92） |
| `BitSet128.__band` | `BitSet128 __band ( BitSet128 Right )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行40） |
| `BitSet128.__bnot` | `BitSet128 __bnot ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行28） |
| `BitSet128.__bor` | `BitSet128 __bor ( BitSet128 Right )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行34） |
| `BitSet128.__bxor` | `BitSet128 __bxor ( BitSet128 Right )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行46） |
| `BitSet128.__eq` | `boolean __eq ( BitSet128 right )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行64） |
| `BitSet128.__le` | `boolean __le ( BitSet128 right )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行78） |
| `BitSet128.__lt` | `boolean __lt ( BitSet128 right )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行70） |
| `BitSet128.__shl` | `BitSet128 __shl ( int Shift )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行52） |
| `BitSet128.__shr` | `BitSet128 __shr ( int Shift )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行58） |
| `BitSet128.__tostring` | `void __tostring ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行99） |
| `BitSet128.h` | `int h` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行111） |
| `BitSet128.l` | `int l` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/BitSet128.md)（行116） |
| `BlendMode.AlphaDestinationFactor` | `BlendFactor AlphaDestinationFactor` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/BlendMode.md)（行53） |
| `BlendMode.AlphaSourceFactor` | `BlendFactor AlphaSourceFactor` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/BlendMode.md)（行61） |
| `BlendMode.Equation` | `BlendEquation Equation` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/BlendMode.md)（行85） |
| `BlendMode.New` | `static BlendMode New ( BlendFactor srcRGB, BlendFactor dstRGB, BlendFactor srcAlpha, BlendFactor dstAlpha, BlendEquation equation)` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/BlendMode.md)（行23） |
| `BlendMode.NewFromType` | `static BlendMode NewFromType ( BlendType blendType )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/BlendMode.md)（行28） |
| `BlendMode.RGBDestinationFactor` | `BlendFactor RGBDestinationFactor` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/BlendMode.md)（行69） |
| `BlendMode.RGBSourceFactor` | `BlendFactor RGBSourceFactor` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/BlendMode.md)（行77） |
| `BlendMode.SetMode` | `void SetMode ( BlendType Type )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/BlendMode.md)（行34） |
| `Capsule.Capsule` | `Capsule Capsule ( Vector Position, Vector SizeMult, float Rotation, float Size )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Capsule.md)（行17） |
| `Capsule.Capsule` | `Capsule Capsule ( Vector Position, Vector TargetPosition, float Size )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Capsule.md)（行18） |
| `Capsule.Collide` | `boolean Collide ( Capsule Capsule, Vector Point )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Capsule.md)（行23） |
| `Capsule.GetDirection` | `Vector GetDirection ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Capsule.md)（行27） |
| `Capsule.GetF1` | `float GetF1 ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Capsule.md)（行31） |
| `Capsule.GetF2` | `float GetF2 ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Capsule.md)（行36） |
| `Capsule.GetPosition` | `Vector GetPosition ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Capsule.md)（行40） |
| `Capsule.GetVec2` | `Vector GetVec2 ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Capsule.md)（行44） |
| `Capsule.GetVec3` | `Vector GetVec3 ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Capsule.md)（行49） |
| `Color.A` | `float A` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行107） |
| `Color.B` | `float B` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行112） |
| `Color.BO` | `float BO` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行117） |
| `Color.Color` | `Color Color ( float R = 1, float G = 1, float B = 1, float A = 1, float RO = 0, float GO = 0, float BO = 0, float RC = 0, float GC = 0, float BC = 0, float AC = 0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行17） |
| `Color.Color` | `Color Color ( float R, float G, float B, float A = 1, float RO = 0, float GO = 0, float BO = 0 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行27） |
| `Color.EmberFade` | `Color EmberFade` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行57） |
| `Color.G` | `float G` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行123） |
| `Color.GO` | `float GO` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行129） |
| `Color.GetColorize` | `table GetColorize ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行25） |
| `Color.GetOffset` | `table GetOffset ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行30） |
| `Color.GetTint` | `table GetTint ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行37） |
| `Color.LaserAlmond` | `Color LaserAlmond` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行69） |
| `Color.LaserChocolate` | `Color LaserChocolate` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行81） |
| `Color.LaserCoal` | `Color LaserCoal` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行93） |
| `Color.LaserFireMind` | `Color LaserFireMind` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行105） |
| `Color.LaserHoming` | `Color LaserHoming` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行117） |
| `Color.LaserIpecac` | `Color LaserIpecac` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行129） |
| `Color.LaserMother` | `Color LaserMother` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行141） |
| `Color.LaserNumberOne` | `Color LaserNumberOne` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行153） |
| `Color.LaserPoison` | `Color LaserPoison` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行177） |
| `Color.LaserSoy` | `Color LaserSoy` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行165） |
| `Color.Lerp` | `static Color Lerp ( Color m1, Color m2, float t )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行53） |
| `Color.Print` | `string Print ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行42） |
| `Color.ProjectileCageBlue` | `Color ProjectileCageBlue` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行189） |
| `Color.ProjectileCorpseClusterDark` | `Color ProjectileCorpseClusterDark` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行201） |
| `Color.ProjectileCorpseClusterLight` | `Color ProjectileCorpseClusterLight` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行213） |
| `Color.ProjectileCorpseGreen` | `Color ProjectileCorpseGreen` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行225） |
| `Color.ProjectileCorpsePink` | `Color ProjectileCorpsePink` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行239） |
| `Color.ProjectileCorpseWhite` | `Color ProjectileCorpseWhite` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行251） |
| `Color.ProjectileCorpseYellow` | `Color ProjectileCorpseYellow` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行263） |
| `Color.ProjectileFireWave` | `Color ProjectileFireWave` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行275） |
| `Color.ProjectileHoming` | `Color ProjectileHoming` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行287） |
| `Color.ProjectileHushBlue` | `Color ProjectileHushBlue` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行299） |
| `Color.ProjectileHushGreen` | `Color ProjectileHushGreen` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行311） |
| `Color.ProjectileHushYellow` | `Color ProjectileHushYellow` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行323） |
| `Color.ProjectileIpecac` | `Color ProjectileIpecac` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行335） |
| `Color.ProjectileMegaSatanBlack` | `Color ProjectileMegaSatanBlack` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行347） |
| `Color.ProjectileMegaSatanWhite` | `Color ProjectileMegaSatanWhite` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行359） |
| `Color.ProjectileSoy` | `Color ProjectileSoy` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行371） |
| `Color.ProjectileTar` | `Color ProjectileTar` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行383） |
| `Color.R` | `float R` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行135） |
| `Color.RO` | `float RO` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行141） |
| `Color.Reset` | `void Reset ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行59） |
| `Color.SetColorize` | `void SetColorize ( float Red, float Green, float Blue, float Amount )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行64） |
| `Color.SetOffset` | `void SetOffset ( float RedOffset, float GreenOffset, float BlueOffset )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行94） |
| `Color.SetTint` | `void SetTint ( float RedTint, float GreenTint, float BlueTint, float AlphaTint )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行100） |
| `Color.TearAlmond` | `Color TearAlmond` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行395） |
| `Color.TearChocolate` | `Color TearChocolate` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行407） |
| `Color.TearCoal` | `Color TearCoal` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行419） |
| `Color.TearCommonCold` | `Color TearCommonCold` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行431） |
| `Color.TearHoming` | `Color TearHoming` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行443） |
| `Color.TearIpecac` | `Color TearIpecac` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行455） |
| `Color.TearNumberOne` | `Color TearNumberOne` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行467） |
| `Color.TearScorpio` | `Color TearScorpio` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行479） |
| `Color.TearSerpentsKiss` | `Color TearSerpentsKiss` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行491） |
| `Color.TearSoy` | `Color TearSoy` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行503） |
| `Color.TearTar` | `Color TearTar` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行515） |
| `Color.__mul` | `Color __mul ( Color right )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Color.md)（行40） |
| `Color.__tostring` | `string __tostring ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Color.md)（行47） |
| `ColorParams.ColorParams` | `ColorParams ColorParams ( Color color, int priority, int duration1, int duration2, boolean fadeout, boolean shared )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行17） |
| `ColorParams.GetColor` | `Color GetColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行24） |
| `ColorParams.GetDuration` | `int GetDuration ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行28） |
| `ColorParams.GetFadeout` | `boolean GetFadeout ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行33） |
| `ColorParams.GetLifespan` | `int GetLifespan ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行37） |
| `ColorParams.GetPriority` | `int GetPriority ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行42） |
| `ColorParams.GetShared` | `boolean GetShared ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行46） |
| `ColorParams.SetColor` | `void SetColor ( Color Color )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行50） |
| `ColorParams.SetDuration` | `void SetDuration ( int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行54） |
| `ColorParams.SetFadeout` | `void SetFadeout ( boolean Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行58） |
| `ColorParams.SetLifespan` | `void SetLifespan ( int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行62） |
| `ColorParams.SetPriority` | `void SetPriority ( int Priority )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行66） |
| `ColorParams.SetShared` | `void SetShared ( boolean Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ColorParams.md)（行70） |
| `CostumeSpriteDesc.CanOverwriteColor` | `boolean CanOverwriteColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行20） |
| `CostumeSpriteDesc.GetDefaultSkinColor` | `int GetDefaultSkinColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行28） |
| `CostumeSpriteDesc.GetItemConfig` | `ItemConfig_Item GetItemConfig ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行32） |
| `CostumeSpriteDesc.GetPlayerType` | `int GetPlayerType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行36） |
| `CostumeSpriteDesc.GetPriority` | `int GetPriority ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行40） |
| `CostumeSpriteDesc.GetSkinColor` | `int GetSkinColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行24） |
| `CostumeSpriteDesc.GetSprite` | `Sprite GetSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行44） |
| `CostumeSpriteDesc.HasOverlay` | `boolean HasOverlay ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行48） |
| `CostumeSpriteDesc.HasSkinAlt` | `boolean HasSkinAlt ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行52） |
| `CostumeSpriteDesc.IsFlying` | `boolean IsFlying ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行56） |
| `CostumeSpriteDesc.IsItemAnimPlaying` | `boolean IsItemAnimPlaying ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行60） |
| `CostumeSpriteDesc.IsItemStateOnly` | `boolean IsItemStateOnly ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/CostumeSpriteDesc.md)（行64） |
| `EffectList.Get` | `TemporaryEffect Get ( int idx )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/CppContainer_Vector_EffectList.md)（行32） |
| `EffectList.Size` | `const int Size` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/CppContainer_Vector_EffectList.md)（行40） |
| `EffectList.__len` | `int __len ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/CppContainer_Vector_EffectList.md)（行24） |
| `Entity.AddBaited` | `void AddBaited ( EntityRef Source, int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行10） |
| `Entity.AddBleeding` | `void AddBleeding ( EntityRef Source, int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行17） |
| `Entity.AddBrimstoneMark` | `void AddBrimstoneMark ( EntityRef Source, int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行24） |
| `Entity.AddBurn` | `void AddBurn ( EntityRef Source, int Duration, float Damage )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行46） |
| `Entity.AddBurn` | `void AddBurn ( EntityRef Source, int Duration, float Damage, boolean IgnoreBosses )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行48） |
| `Entity.AddCharmed` | `void AddCharmed ( EntityRef sourceEntity, int Duration )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行75） |
| `Entity.AddCharmed` | `void AddCharmed ( EntityRef sourceEntity, int Duration, boolean IgnoreBosses )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行77） |
| `Entity.AddConfusion` | `void AddConfusion ( EntityRef Source, int Duration, boolean IgnoreBosses )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行97） |
| `Entity.AddEntityFlags` | `void AddEntityFlags ( int Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行118） |
| `Entity.AddFear` | `void AddFear ( EntityRef Source, int Duration )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行135） |
| `Entity.AddFear` | `void AddFear ( EntityRef Source, int Duration, boolean IgnoreBosses )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行137） |
| `Entity.AddFreeze` | `void AddFreeze ( EntityRef Source, int Duration )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行158） |
| `Entity.AddFreeze` | `void AddFreeze ( EntityRef Source, int Duration, boolean IgnoreBosses )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行160） |
| `Entity.AddHealth` | `void AddHealth ( float HitPoints )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行181） |
| `Entity.AddIce` | `void AddIce ( EntityRef Source, int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行31） |
| `Entity.AddKnockback` | `void AddKnockback ( EntityRef Source, Vector PushDirection, int Duration, boolean TakeImpactDamage )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行38） |
| `Entity.AddMagnetized` | `void AddMagnetized ( EntityRef Source, int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行45） |
| `Entity.AddMidasFreeze` | `void AddMidasFreeze ( EntityRef Source, int Duration )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行186） |
| `Entity.AddMidasFreeze` | `void AddMidasFreeze ( EntityRef Source, int Duration, boolean IgnoreBosses )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行188） |
| `Entity.AddPoison` | `void AddPoison ( EntityRef Source, int Duration, float Damage )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行210） |
| `Entity.AddPoison` | `void AddPoison ( EntityRef Source, int Duration, float Damage, boolean IgnoreBosses )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行212） |
| `Entity.AddShrink` | `void AddShrink ( EntityRef Source, int Duration )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行242） |
| `Entity.AddShrink` | `void AddShrink ( EntityRef Source, int Duration, boolean IgnoreBosses )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行244） |
| `Entity.AddSlowing` | `void AddSlowing ( EntityRef Source, int Duration, float SlowValue, Color SlowColor )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行264） |
| `Entity.AddSlowing` | `void AddSlowing ( EntityRef Source, int Duration, float SlowValue, Color SlowColor, boolean IgnoreBosses )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行266） |
| `Entity.AddVelocity` | `void AddVelocity ( Vector Velocity )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行283） |
| `Entity.AddVelocity` | `void AddVelocity ( Vector Velocity, boolean IgnoreTimeScale = false )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行285） |
| `Entity.AddWeakness` | `void AddWeakness ( EntityRef Source, int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行52） |
| `Entity.BloodExplode` | `void BloodExplode ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行293） |
| `Entity.CanDevolve` | `boolean CanDevolve ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行59） |
| `Entity.CanShutDoors` | `boolean CanShutDoors ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行298） |
| `Entity.Child` | `Entity Child` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行684） |
| `Entity.ClearEntityFlags` | `void ClearEntityFlags ( int Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行303） |
| `Entity.CollidesWithGrid` | `boolean CollidesWithGrid ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行309） |
| `Entity.CollisionDamage` | `float CollisionDamage` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行693） |
| `Entity.Color` | `Color Color` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行699） |
| `Entity.ComputeStatusEffectDuration` | `int ComputeStatusEffectDuration ( int InitialLength, EntityRef Source )` | explicit_readonly_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行63） |
| `Entity.CopyStatusEffects` | `void CopyStatusEffects ( Entity Target, boolean Overwrite = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行68） |
| `Entity.DepthOffset` | `float DepthOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行705） |
| `Entity.Die` | `void Die ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行315） |
| `Entity.DropSeed` | `const int DropSeed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行725） |
| `Entity.EntityCollisionClass` | `EntityCollisionClass EntityCollisionClass` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行731） |
| `Entity.Exists` | `boolean Exists ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行321） |
| `Entity.FlipX` | `boolean FlipX` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行737） |
| `Entity.ForceCollide` | `boolean ForceCollide ( Entity Entity, boolean Force )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行73） |
| `Entity.FrameCount` | `const int FrameCount` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行743） |
| `Entity.Friction` | `float Friction` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行748） |
| `Entity.GetBaitedCountdown` | `int GetBaitedCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行87） |
| `Entity.GetBleedingCountdown` | `int GetBleedingCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行91） |
| `Entity.GetBossID` | `int GetBossID ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行330） |
| `Entity.GetBossStatusEffectCooldown` | `int GetBossStatusEffectCooldown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行95） |
| `Entity.GetBrimstoneMarkCountdown` | `int GetBrimstoneMarkCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行99） |
| `Entity.GetBurnCountdown` | `int GetBurnCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行103） |
| `Entity.GetBurnDamageTimer` | `int GetBurnDamageTimer ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行107） |
| `Entity.GetCharmedCountdown` | `int GetCharmedCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行111） |
| `Entity.GetCollisionCapsule` | `Capsule GetCollisionCapsule ( Vector Vector )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行115） |
| `Entity.Color` | `const Color GetColor ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行338） |
| `Entity.GetColorParams` | `ColorParams[] GetColorParams ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行119） |
| `Entity.GetConfusionCountdown` | `int GetConfusionCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行124） |
| `Entity.GetDamageCountdown` | `int GetDamageCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行128） |
| `Entity.GetData` | `table GetData ( )` | actual_event_only mod_namespace:tx-04-14 | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行344） |
| `Entity.GetDebugShape` | `Shape GetDebugShape ( boolean Unknown )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行135） |
| `Entity.GetDropRNG` | `RNG GetDropRNG ( )` | owner_qualified_reference random:tx-04-14 | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行377） |
| `Entity.GetEntityConfigEntity` | `EntityConfigEntity GetEntityConfigEntity ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行139） |
| `Entity.GetEntityFlags` | `int GetEntityFlags ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行383） |
| `Entity.GetFearCountdown` | `int GetFearCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行144） |
| `Entity.GetFireDamageCooldown` | `int GetFireDamageCooldown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行148） |
| `Entity.GetFreezeCountdown` | `int GetFreezeCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行152） |
| `Entity.GetHitListIndex` | `int GetHitListIndex ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行156） |
| `Entity.GetIceCountdown` | `int GetIceCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行161） |
| `Entity.GetKnockbackCountdown` | `int GetKnockbackCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行165） |
| `Entity.GetKnockbackDirection` | `Vector GetKnockbackDirection ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行169） |
| `Entity.GetLastChild` | `Entity GetLastChild ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行398） |
| `Entity.GetLastParent` | `Entity GetLastParent ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行407） |
| `Entity.GetMagnetizedCountdown` | `int GetMagnetizedCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行173） |
| `Entity.GetMidasFreezeCountdown` | `int GetMidasFreezeCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行177） |
| `Entity.GetMinecart` | `EntityNPC GetMinecart ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行181） |
| `Entity.GetNullCapsule` | `Capsule GetNullCapsule ( string NullLayerName )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行189） |
| `Entity.GetNullOffset` | `Vector GetNullOffset ( string NullLayerName )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行193） |
| `Entity.GetPauseTime` | `int GetPauseTime ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行198） |
| `Entity.GetPoisonCountdown` | `int GetPoisonCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行202） |
| `Entity.GetPoisonDamageTimer` | `int GetPoisonDamageTimer ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行206） |
| `Entity.GetPosVel` | `PosVel GetPosVel ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行210） |
| `Entity.GetPredictedTargetPosition` | `Vector GetPredictedTargetPosition ( Entity Target, float Delay )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行215） |
| `Entity.GetShadowSize` | `float GetShadowSize ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行221） |
| `Entity.GetShrinkCountdown` | `int GetShrinkCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行225） |
| `Entity.GetSlowingCountdown` | `int GetSlowingCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行229） |
| `Entity.GetSpeedMultiplier` | `float GetSpeedMultiplier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行233） |
| `Entity.GetSprite` | `Sprite GetSprite ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行416） |
| `Entity.Type` | `EntityType GetType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行239） |
| `Entity.GetWaterClipFlags` | `WaterClipFlag GetWaterClipFlags ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行243） |
| `Entity.GetWeaknessCountdown` | `int GetWeaknessCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行248） |
| `Entity.GiveMinecart` | `EntityNPC GiveMinecart ( Vector Position, Vector Velocity )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行252） |
| `Entity.GridCollisionClass` | `EntityGridCollisionClass GridCollisionClass` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行754） |
| `Entity.HasCommonParentWithEntity` | `boolean HasCommonParentWithEntity ( Entity Other )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行422） |
| `Entity.HasEntityFlags` | `boolean HasEntityFlags ( int Flags )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行428） |
| `Entity.HasFullHealth` | `boolean HasFullHealth ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行443） |
| `Entity.HasMortalDamage` | `boolean HasMortalDamage ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行449） |
| `Entity.HitPoints` | `float HitPoints` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行763） |
| `Entity.IgnoreEffectFromFriendly` | `boolean IgnoreEffectFromFriendly ( EntityRef Source )` | explicit_readonly_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行256） |
| `Entity.Index` | `const int Index` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行772） |
| `Entity.InitSeed` | `const int InitSeed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行777） |
| `Entity.IsActiveEnemy` | `boolean IsActiveEnemy ( boolean includeDead )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行458） |
| `Entity.IsBoss` | `boolean IsBoss ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行463） |
| `Entity.IsDead` | `boolean IsDead ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行468） |
| `Entity.IsEnemy` | `boolean IsEnemy ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行474） |
| `Entity.IsFlying` | `boolean IsFlying ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行480） |
| `Entity.IsFrame` | `boolean IsFrame ( int Frame, int Offset )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行485） |
| `Entity.IsInvincible` | `boolean IsInvincible ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行490） |
| `Entity.IsVisible` | `boolean IsVisible ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行495） |
| `Entity.IsVulnerableEnemy` | `boolean IsVulnerableEnemy ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行500） |
| `Entity.Kill` | `void Kill ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行506） |
| `Entity.KillWithSource` | `void KillWithSource ( EntityRef Source )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行511） |
| `Entity.MakeBloodPoof` | `EntityEffect MakeBloodPoof ( Vector Position = self.Position, Color Color = default, float Scale = 1.0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行261） |
| `Entity.MakeGroundPoof` | `EntityEffect MakeGroundPoof ( Vector Position = self.Position, Color Color = default, float Scale = 1.0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行266） |
| `Entity.Mass` | `float Mass` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行783） |
| `Entity.MaxHitPoints` | `float MaxHitPoints` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行792） |
| `Entity.MultiplyFriction` | `void MultiplyFriction ( float Value )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行515） |
| `Entity.Parent` | `Entity Parent` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行798） |
| `Entity.Position` | `Vector Position` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行805） |
| `Entity.PositionOffset` | `const Vector PositionOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行810） |
| `Entity.PostRender` | `void PostRender ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行520） |
| `Entity.Remove` | `void Remove ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行525） |
| `Entity.RemoveStatusEffects` | `void RemoveStatusEffects ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行530） |
| `Entity.Render` | `void Render ( Vector Offset )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行536） |
| `Entity.RenderShadowLayer` | `boolean RenderShadowLayer ( Vector Offset )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行541） |
| `Entity.RenderZOffset` | `int RenderZOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行816） |
| `Entity.ResetWaterClipFlags` | `void ResetWaterClipFlags ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行271） |
| `Entity.SetBaitedCountdown` | `void SetBaitedCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行276） |
| `Entity.SetBleedingCountdown` | `void SetBleedingCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行280） |
| `Entity.SetBossStatusEffectCooldown` | `void SetBossStatusEffectCooldown ( int Cooldown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行284） |
| `Entity.SetBrimstoneMarkCountdown` | `void SetBrimstoneMarkCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行288） |
| `Entity.SetBurnCountdown` | `void SetBurnCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行292） |
| `Entity.SetBurnDamageTimer` | `void SetBurnDamageTimer ( int Timer )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行296） |
| `Entity.SetCharmedCountdown` | `void SetCharmedCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行300） |
| `Entity.SetColor` | `void SetColor ( Color Color, int Duration, int Priority, boolean Fadeout, boolean Share )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行547） |
| `Entity.SetColorParams` | `void SetColorParams ( ColorParams[] Params )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行304） |
| `Entity.SetConfusionCountdown` | `void SetConfusionCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行309） |
| `Entity.SetDamageCountdown` | `void SetDamageCountdown ( int countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行313） |
| `Entity.SetDead` | `void SetDead ( boolean IsDead )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行320） |
| `Entity.SetFearCountdown` | `void SetFearCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行324） |
| `Entity.SetFireDamageCooldown` | `void SetFireDamageCooldown ( int Cooldown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行328） |
| `Entity.SetFreezeCountdown` | `void SetFreezeCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行332） |
| `Entity.SetIceCountdown` | `void SetIceCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行336） |
| `Entity.SetInvincible` | `void SetInvincible ( boolean IsInvincible )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行340） |
| `Entity.SetKnockbackCountdown` | `void SetKnockbackCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行344） |
| `Entity.SetKnockbackDirection` | `void SetKnockbackDirection ( Vector Direction )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行348） |
| `Entity.SetMagnetizedCountdown` | `void SetMagnetizedCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行352） |
| `Entity.SetMidasFreezeCountdown` | `void SetMidasFreezeCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行356） |
| `Entity.SetPauseTime` | `void SetPauseTime ( int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行360） |
| `Entity.SetPoisonCountdown` | `void SetPoisonCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行364） |
| `Entity.SetPoisonDamageTimer` | `void SetPoisonDamageTimer ( int Timer )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行368） |
| `Entity.SetShadowSize` | `float SetShadowSize ( float Size )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行372） |
| `Entity.SetShrinkCountdown` | `void SetShrinkCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行376） |
| `Entity.SetSize` | `void SetSize ( float Size, Vector SizeMulti, int NumGridCollisionPoints )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行563） |
| `Entity.SetSlowingCountdown` | `void SetSlowingCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行380） |
| `Entity.SetSpeedMultiplier` | `void SetSpeedMultiplier ( float Amount )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行384） |
| `Entity.SetSpriteFrame` | `void SetSpriteFrame ( string AnimationName, int FrameNum )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行569） |
| `Entity.SetSpriteOverlayFrame` | `void SetSpriteOverlayFrame ( string AnimationName, int FrameNum )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行574） |
| `Entity.SetWaterClipFlags` | `void SetWaterClipFlags ( WaterClipFlag Flags )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行390） |
| `Entity.SetWeaknessCountdown` | `void SetWeaknessCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行399） |
| `Entity.Size` | `float Size` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行825） |
| `Entity.SizeMulti` | `Vector SizeMulti` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行831） |
| `Entity.SortingLayer` | `SortingLayer SortingLayer` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行838） |
| `Entity.SpawnBloodEffect` | `EntityEffect SpawnBloodEffect ( int SubType = 0, Vector position = self.Position, Vector Offset = Vector.Zero, Color Color = Default, Vector Velocity = Vector.Zero )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行403） |
| `Entity.SpawnGridIndex` | `const int SpawnGridIndex` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行863） |
| `Entity.SpawnWaterImpactEffects` | `void SpawnWaterImpactEffects ( Vector Position, Vector Velocity = Vector.Zero, float Strength )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行407） |
| `Entity.SpawnerEntity` | `Entity SpawnerEntity` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行845） |
| `Entity.SpawnerType` | `EntityType SpawnerType` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行851） |
| `Entity.SpawnerVariant` | `int SpawnerVariant` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行857） |
| `Entity.SplatColor` | `Color SplatColor` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行871） |
| `Entity.SpriteOffset` | `Vector SpriteOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行879） |
| `Entity.SpriteRotation` | `float SpriteRotation` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行885） |
| `Entity.SpriteScale` | `Vector SpriteScale` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行891） |
| `Entity.SubType` | `int SubType` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行899） |
| `Entity.TakeDamage` | `boolean TakeDamage ( float Damage, DamageFlag Flags, EntityRef Source, int DamageCountdown )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行579） |
| `Entity.Target` | `Entity Target` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行905） |
| `Entity.TargetPosition` | `Vector TargetPosition` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行911） |
| `Entity.TeleportToRandomPosition` | `void TeleportToRandomPosition ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行414） |
| `Entity.ToBomb` | `EntityBomb ToBomb ( )` | classification  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行587） |
| `Entity.ToDelirium` | `EntityDelirium ToDelirium ( )` | classification  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行418） |
| `Entity.ToEffect` | `EntityEffect ToEffect ( )` | classification  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行595） |
| `Entity.ToFamiliar` | `EntityFamiliar ToFamiliar ( )` | classification  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行604） |
| `Entity.ToKnife` | `EntityKnife ToKnife ( )` | classification  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行613） |
| `Entity.ToLaser` | `EntityLaser ToLaser ( )` | classification  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行622） |
| `Entity.ToNPC` | `EntityNPC ToNPC ( )` | classification  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行631） |
| `Entity.ToPickup` | `EntityPickup ToPickup ( )` | classification  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行640） |
| `Entity.ToPlayer` | `EntityPlayer ToPlayer ( )` | classification  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行649） |
| `Entity.ToProjectile` | `EntityProjectile ToProjectile ( )` | classification  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行658） |
| `Entity.ToSlot` | `EntitySlot ToSlot ( )` | classification  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行427） |
| `Entity.ToTear` | `EntityTear ToTear ( )` | classification  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行667） |
| `Entity.TryThrow` | `boolean TryThrow ( EntityRef Source, Vector Velocity, float Height )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Entity.md)（行436） |
| `Entity.Type` | `const EntityType Type` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行917） |
| `Entity.Update` | `void Update ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行676） |
| `Entity.Variant` | `int Variant` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行923） |
| `Entity.Velocity` | `Vector Velocity` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行929） |
| `Entity.Visible` | `boolean Visible` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Entity.md)（行935） |
| `EntityBomb.AddTearFlags` | `void AddTearFlags ( TearFlags Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityBomb.md)（行22） |
| `EntityBomb.ClearTearFlags` | `void ClearTearFlags ( TearFlags Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityBomb.md)（行27） |
| `EntityBomb.ExplosionDamage` | `float ExplosionDamage` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityBomb.md)（行43） |
| `EntityBomb.Flags` | `TearFlags Flags` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityBomb.md)（行48） |
| `EntityBomb.GetCostumeLayerSprite` | `Sprite GetCostumeLayerSprite ( BombCostumeLayer LayerID )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行12） |
| `EntityBomb.GetExplosionCountdown` | `int GetExplosionCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行16） |
| `EntityBomb.GetFallAcceleration` | `float GetFallAcceleration ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行20） |
| `EntityBomb.GetFallSpeed` | `float GetFallSpeed ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行24） |
| `EntityBomb.GetHitList` | `int[] GetHitList ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行28） |
| `EntityBomb.GetRocketAngle` | `float GetRocketAngle ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行33） |
| `EntityBomb.GetRocketSpeed` | `float GetRocketSpeed ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行38） |
| `EntityBomb.GetScale` | `float GetScale ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行43） |
| `EntityBomb.HasTearFlags` | `boolean HasTearFlags ( TearFlags Flags )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityBomb.md)（行32） |
| `EntityBomb.IsFetus` | `boolean IsFetus` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityBomb.md)（行54） |
| `EntityBomb.IsLoadingCostumes` | `boolean IsLoadingCostumes ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行48） |
| `EntityBomb.IsPrismTouched` | `boolean IsPrismTouched ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行52） |
| `EntityBomb.RadiusMultiplier` | `float RadiusMultiplier` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityBomb.md)（行59） |
| `EntityBomb.SetExplosionCountdown` | `void SetExplosionCountdown ( int Countdown )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityBomb.md)（行37） |
| `EntityBomb.SetFallAcceleration` | `void SetFallAcceleration ( float Acceleration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行57） |
| `EntityBomb.SetFallSpeed` | `void SetFallSpeed ( float Speed )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行61） |
| `EntityBomb.SetLoadCostumes` | `void SetLoadCostumes ( boolean Load = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行65） |
| `EntityBomb.SetPrismTouched` | `void SetPrismTouched ( boolean IsTouched )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行69） |
| `EntityBomb.SetRocketAngle` | `void SetRocketAngle ( float Angle )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行74） |
| `EntityBomb.SetRocketSpeed` | `void SetRocketSpeed ( float Speed )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行79） |
| `EntityBomb.SetScale` | `void SetScale ( float Scale )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行84） |
| `EntityBomb.UpdateDirtColor` | `void UpdateDirtColor ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityBomb.md)（行89） |
| `EntityConfigBaby.GetAchievementID` | `Achievement GetAchievementID ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigBaby.md)（行20） |
| `EntityConfigBaby.GetID` | `BabySubType GetID ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigBaby.md)（行24） |
| `EntityConfigBaby.GetName` | `string GetName ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigBaby.md)（行28） |
| `EntityConfigBaby.GetSpritesheetPath` | `string GetSpritesheetPath ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigBaby.md)（行32） |
| `EntityConfigEntity.CanBeChampion` | `boolean CanBeChampion ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行20） |
| `EntityConfigEntity.CanBeRerolledInto` | `boolean CanBeRerolledInto ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行24） |
| `EntityConfigEntity.CanShutDoors` | `boolean CanShutDoors ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行28） |
| `EntityConfigEntity.GetAnm2Path` | `string GetAnm2Path ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行32） |
| `EntityConfigEntity.GetBaseHP` | `float GetBaseHP ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行36） |
| `EntityConfigEntity.GetBestiaryAnimation` | `string GetBestiaryAnimation ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行40） |
| `EntityConfigEntity.GetBestiaryAnm2Path` | `string GetBestiaryAnm2Path ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行44） |
| `EntityConfigEntity.GetBestiaryFloorAlt` | `string GetBestiaryFloorAlt ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行48） |
| `EntityConfigEntity.GetBestiaryOffset` | `const Vector GetBestiaryOffset ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行52） |
| `EntityConfigEntity.GetBestiaryOverlay` | `string GetBestiaryOverlay ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行56） |
| `EntityConfigEntity.GetBestiaryScale` | `float GetBestiaryScale ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行60） |
| `EntityConfigEntity.GetBossID` | `BossType GetBossID ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行64） |
| `EntityConfigEntity.GetCollisionDamage` | `float GetCollisionDamage ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行68） |
| `EntityConfigEntity.GetCollisionInterval` | `int GetCollisionInterval ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行72） |
| `EntityConfigEntity.GetCollisionRadius` | `float GetCollisionRadius ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行76） |
| `EntityConfigEntity.GetCollisionRadiusMultiplier` | `const Vector GetCollisionRadiusMultiplier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行81） |
| `EntityConfigEntity.GetCustomTags` | `table GetCustomTags ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行86） |
| `EntityConfigEntity.GetDevolvedEntity` | `EntityConfigEntity GetDevolvedEntity ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行91） |
| `EntityConfigEntity.GetEntityTags` | `int GetEntityTags ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行98） |
| `EntityConfigEntity.GetFriction` | `float GetFriction ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行103） |
| `EntityConfigEntity.GetGibFlags` | `int GetGibFlags ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行107） |
| `EntityConfigEntity.GetGibsAmount` | `int GetGibsAmount ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行112） |
| `EntityConfigEntity.GetGridCollisionPoints` | `int GetGridCollisionPoints ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行116） |
| `EntityConfigEntity.GetMass` | `float GetMass ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行120） |
| `EntityConfigEntity.GetModName` | `string GetModName ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行124） |
| `EntityConfigEntity.GetName` | `string GetName ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行131） |
| `EntityConfigEntity.GetPortraitID` | `int GetPortraitID ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行135） |
| `EntityConfigEntity.GetShadowSize` | `float GetShadowSize ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行139） |
| `EntityConfigEntity.GetShieldStrength` | `float GetShieldStrength ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行144） |
| `EntityConfigEntity.GetStageHP` | `float GetStageHP ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行149） |
| `EntityConfigEntity.GetSubType` | `int GetSubType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行153） |
| `EntityConfigEntity.GetType` | `int GetType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行157） |
| `EntityConfigEntity.GetVariant` | `int GetVariant ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行161） |
| `EntityConfigEntity.HasCustomTag` | `boolean HasCustomTag ( string tag )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行165） |
| `EntityConfigEntity.HasEntityTags` | `boolean HasEntityTags ( int Tags )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行170） |
| `EntityConfigEntity.HasFloorAlts` | `boolean HasFloorAlts ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行175） |
| `EntityConfigEntity.HasGibFlags` | `boolean HasGibFlags ( int Flags )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行179） |
| `EntityConfigEntity.IsBoss` | `boolean IsBoss ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigEntity.md)（行184） |
| `EntityConfigPlayer.CanShoot` | `boolean CanShoot ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行20） |
| `EntityConfigPlayer.GetAchievementID` | `Achievement GetAchievementID ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行24） |
| `EntityConfigPlayer.GetBirthrightDescription` | `string GetBirthrightDescription ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行29） |
| `EntityConfigPlayer.GetBlackHearts` | `int GetBlackHearts ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行33） |
| `EntityConfigPlayer.GetBombs` | `int GetBombs ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行37） |
| `EntityConfigPlayer.GetBrokenHearts` | `int GetBrokenHearts ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行41） |
| `EntityConfigPlayer.GetCard` | `Card GetCard ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行45） |
| `EntityConfigPlayer.GetCoins` | `int GetCoins ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行54） |
| `EntityConfigPlayer.GetCollectibles` | `CollectibleType[] GetCollectibles ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行58） |
| `EntityConfigPlayer.GetCostumeID` | `int GetCostumeID ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行63） |
| `EntityConfigPlayer.GetCostumeSuffix` | `string GetCostumeSuffix ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行68） |
| `EntityConfigPlayer.GetExtraPortraitPath` | `string GetExtraPortraitPath ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行73） |
| `EntityConfigPlayer.GetKeys` | `int GetKeys ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行78） |
| `EntityConfigPlayer.GetModdedControlsSprite` | `Sprite GetModdedControlsSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行82） |
| `EntityConfigPlayer.GetModdedCoopMenuSprite` | `Sprite GetModdedCoopMenuSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行91） |
| `EntityConfigPlayer.GetModdedGameOverSprite` | `Sprite GetModdedGameOverSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行100） |
| `EntityConfigPlayer.GetModdedMenuBackgroundSprite` | `Sprite GetModdedMenuBackgroundSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行109） |
| `EntityConfigPlayer.GetModdedMenuPortraitSprite` | `Sprite GetModdedMenuPortraitSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行118） |
| `EntityConfigPlayer.GetName` | `string GetName ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行127） |
| `EntityConfigPlayer.GetNameImagePath` | `string GetNameImagePath ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行131） |
| `EntityConfigPlayer.GetPill` | `PillColor GetPill ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行136） |
| `EntityConfigPlayer.GetPlayerType` | `int GetPlayerType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行141） |
| `EntityConfigPlayer.GetPocketActive` | `CollectibleType GetPocketActive ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行145） |
| `EntityConfigPlayer.GetPortraitPath` | `string GetPortraitPath ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行150） |
| `EntityConfigPlayer.GetRedHearts` | `int GetRedHearts ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行155） |
| `EntityConfigPlayer.GetSkinColor` | `SkinColor GetSkinColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行159） |
| `EntityConfigPlayer.GetSkinPath` | `string GetSkinPath ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行163） |
| `EntityConfigPlayer.GetSoulHearts` | `int GetSoulHearts ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行168） |
| `EntityConfigPlayer.GetTaintedCounterpart` | `EntityConfigPlayer GetTaintedCounterpart ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行172） |
| `EntityConfigPlayer.GetTrinket` | `TrinketType GetTrinket ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行179） |
| `EntityConfigPlayer.IsHidden` | `boolean IsHidden ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行186） |
| `EntityConfigPlayer.IsTainted` | `boolean IsTainted ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityConfigPlayer.md)（行193） |
| `EntityDelirium.Angle` | `int8 Angle` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行53） |
| `EntityDelirium.AttackID` | `int AttackID` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行66） |
| `EntityDelirium.BossType` | `const int BossType` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行72） |
| `EntityDelirium.BossVariant` | `const int BossVariant` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行78） |
| `EntityDelirium.Cycle` | `int Cycle` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行83） |
| `EntityDelirium.GetTeleportationTimer` | `int GetTeleportationTimer ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行10） |
| `EntityDelirium.IsRedMode` | `boolean IsRedMode ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行15） |
| `EntityDelirium.RemainingAttacks` | `int RemainingAttacks` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行96） |
| `EntityDelirium.SetRedMode` | `void SetRedMode ( boolean On )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行24） |
| `EntityDelirium.SetTeleportationTimer` | `void SetTeleportationTimer ( int Timer )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行32） |
| `EntityDelirium.StateD` | `NpcState StateD` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行107） |
| `EntityDelirium.Transform` | `void Transform ( int Type, int Variant = 0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行37） |
| `EntityDelirium.TransformationTimer` | `int TransformationTimer` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDelirium.md)（行112） |
| `EntityDesc.EntityDesc` | `EntityDesc EntityDesc ( EntityType Type = 0, int Variant = 0, int Subtype = 0, ChampionColor ChampionId = -1, float Health = 0.0, float MaxHealth = 0.0, boolean PlayerControlled = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行14） |
| `EntityDesc.GetChampionId` | `ChampionColor GetChampionId ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行21） |
| `EntityDesc.GetHealth` | `float GetHealth ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行26） |
| `EntityDesc.GetMaxHealth` | `float GetMaxHealth ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行31） |
| `EntityDesc.GetSubtype` | `int GetSubtype ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行36） |
| `EntityDesc.GetType` | `EntityType GetType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行41） |
| `EntityDesc.GetVariant` | `int GetVariant ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行46） |
| `EntityDesc.IsPlayerControlled` | `boolean IsPlayerControlled ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行51） |
| `EntityDesc.SetChampionId` | `void SetChampionId ( ChampionColor ChampionId )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行56） |
| `EntityDesc.SetHealth` | `void SetHealth ( float Health )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行61） |
| `EntityDesc.SetMaxHealth` | `void SetMaxHealth ( float MaxHealth )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行66） |
| `EntityDesc.SetPlayerControlled` | `void SetPlayerControlled ( boolean PlayerControlled )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行71） |
| `EntityDesc.SetSubtype` | `void SetSubtype ( int Subtype )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行76） |
| `EntityDesc.SetType` | `void SetType ( EntityType Type )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行81） |
| `EntityDesc.SetVariant` | `void SetVariant ( int Variant )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityDesc.md)（行86） |
| `EntityEffect.AddTearFlags` | `void AddTearFlags ( TearFlags )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityEffect.md)（行12） |
| `EntityEffect.ClearTearFlags` | `void ClearTearFlags ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityEffect.md)（行18） |
| `EntityEffect.CreateLight` | `static EntityEffect CreateLight ( Vector Position, float Scale = RandomFloat[0.0-1.0], int Lifespan = -1, int State = 6, Color Color = Default)` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityEffect.md)（行24） |
| `EntityEffect.CreateLootPreview` | `EntityEffect CreateLootPreview ( Vector Position, EntityPickup Owner, EntityEffect Effect )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityEffect.md)（行30） |
| `EntityEffect.DamageSource` | `int DamageSource` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行47） |
| `EntityEffect.FallingAcceleration` | `float FallingAcceleration` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行52） |
| `EntityEffect.FallingSpeed` | `float FallingSpeed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行57） |
| `EntityEffect.FollowParent` | `void FollowParent ( Entity Parent )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行21） |
| `EntityEffect.GetGridEntityDesc` | `GridEntityDesc GetGridEntityDesc ( )` | owner_qualified_reference grid:isaac.cs.grid.lifted_descriptors | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityEffect.md)（行34） |
| `EntityEffect.GetTearFlags` | `TearFlags GetTearFlags ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityEffect.md)（行40） |
| `EntityEffect.HasTearFlags` | `boolean HasTearFlags ( TearFlags )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityEffect.md)（行46） |
| `EntityEffect.IsFollowing` | `boolean IsFollowing` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行62） |
| `EntityEffect.IsPlayerCreep` | `static boolean IsPlayerCreep ( EffectVariant Variant )` | explicit_readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行26） |
| `EntityEffect.LifeSpan` | `int LifeSpan` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行67） |
| `EntityEffect.MaxRadius` | `float MaxRadius` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行78） |
| `EntityEffect.MinRadius` | `float MinRadius` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行83） |
| `EntityEffect.ParentOffset` | `Vector ParentOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行89） |
| `EntityEffect.Rotation` | `float Rotation` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行95） |
| `EntityEffect.Scale` | `float Scale` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行100） |
| `EntityEffect.SetDamageSource` | `void SetDamageSource ( EntityType DamageSource )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行31） |
| `EntityEffect.SetRadii` | `void SetRadii ( float min, float max )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行36） |
| `EntityEffect.SetTearFlags` | `void SetTearFlags ( TearFlags )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityEffect.md)（行52） |
| `EntityEffect.SetTimeout` | `void SetTimeout ( int Timeout )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行41） |
| `EntityEffect.State` | `int State` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行105） |
| `EntityEffect.Timeout` | `int Timeout` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行111） |
| `EntityEffect.m_Height` | `float m_Height` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityEffect.md)（行72） |
| `EntityFamiliar.AddCoins` | `void AddCoins ( int Value )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行25） |
| `EntityFamiliar.AddHearts` | `void AddHearts ( int Hearts )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行30） |
| `EntityFamiliar.AddKeys` | `void AddKeys ( int Keys )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行35） |
| `EntityFamiliar.AddToDelayed` | `void AddToDelayed ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行40） |
| `EntityFamiliar.AddToFollowers` | `void AddToFollowers ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行45） |
| `EntityFamiliar.AddToOrbit` | `void AddToOrbit ( int Layer )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行50） |
| `EntityFamiliar.CanBeDamagedByEnemies` | `boolean CanBeDamagedByEnemies ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行12） |
| `EntityFamiliar.CanBeDamagedByLasers` | `boolean CanBeDamagedByLasers ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行16） |
| `EntityFamiliar.CanBeDamagedByProjectiles` | `boolean CanBeDamagedByProjectiles ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行20） |
| `EntityFamiliar.CanBlockProjectiles` | `boolean CanBlockProjectiles ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行24） |
| `EntityFamiliar.CanCharm` | `boolean CanCharm ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行28） |
| `EntityFamiliar.Coins` | `int Coins` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行151） |
| `EntityFamiliar.FireCooldown` | `int FireCooldown` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行156） |
| `EntityFamiliar.FireProjectile` | `EntityTear FireProjectile ( Vector Dir )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行55） |
| `EntityFamiliar.FollowParent` | `void FollowParent ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行63） |
| `EntityFamiliar.FollowPosition` | `void FollowPosition ( Vector Pos )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行68） |
| `EntityFamiliar.GetActiveWeaponEntity` | `Entity GetActiveWeaponEntity ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行33） |
| `EntityFamiliar.GetActiveWeaponNumFired` | `int GetActiveWeaponNumFired ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行40） |
| `EntityFamiliar.GetDirtColor` | `Color GetDirtColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行45） |
| `EntityFamiliar.GetFollowerPriority` | `FollowerPriority GetFollowerPriority ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行49） |
| `EntityFamiliar.GetItemConfig` | `ItemConfigItem GetItemConfig ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行53） |
| `EntityFamiliar.GetMoveDelayNum` | `int GetMoveDelayNum ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行60） |
| `EntityFamiliar.GetMultiplier` | `float GetMultiplier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行65） |
| `EntityFamiliar.OrbitDistance` | `static Vector GetOrbitDistance ( int Layer )` | explicit_readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行73） |
| `EntityFamiliar.GetOrbitPosition` | `Vector GetOrbitPosition ( Vector Pos )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行78） |
| `EntityFamiliar.GetPathfinder` | `PathFinder GetPathfinder ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行75） |
| `EntityFamiliar.GetRandomWisp` | `CollectibleType GetRandomWisp ( RNG RNG )` | actual_event_only rng_consuming_call | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行79） |
| `EntityFamiliar.GetWeapon` | `Weapon GetWeapon ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行85） |
| `EntityFamiliar.HeadFrameDelay` | `int HeadFrameDelay` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行161） |
| `EntityFamiliar.Hearts` | `int Hearts` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行166） |
| `EntityFamiliar.InvalidateCachedMultiplier` | `void InvalidateCachedMultiplier ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行90） |
| `EntityFamiliar.IsCharmed` | `boolean IsCharmed ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行95） |
| `EntityFamiliar.IsDelayed` | `boolean IsDelayed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行171） |
| `EntityFamiliar.IsFollower` | `boolean IsFollower` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行176） |
| `EntityFamiliar.IsLilDelirium` | `boolean IsLilDelirium ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行100） |
| `EntityFamiliar.Keys` | `int Keys` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行181） |
| `EntityFamiliar.LastDirection` | `Direction LastDirection` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行186） |
| `EntityFamiliar.MoveDelayed` | `void MoveDelayed ( int NumFrames )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行85） |
| `EntityFamiliar.MoveDiagonally` | `void MoveDiagonally ( float Speed )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行90） |
| `EntityFamiliar.MoveDirection` | `Direction MoveDirection` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行191） |
| `EntityFamiliar.OrbitAngleOffset` | `float OrbitAngleOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行196） |
| `EntityFamiliar.OrbitDistance` | `Vector OrbitDistance` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行215） |
| `EntityFamiliar.OrbitLayer` | `int OrbitLayer` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行221） |
| `EntityFamiliar.OrbitSpeed` | `float OrbitSpeed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行227） |
| `EntityFamiliar.PickEnemyTarget` | `void PickEnemyTarget ( float MaxDistance, int FrameInterval = 13, int Flags = 0, Vector ConeDir = Vector.Zero, float ConeAngle = 15 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行95） |
| `EntityFamiliar.PlayChargeAnim` | `void PlayChargeAnim ( Direction Dir )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行110） |
| `EntityFamiliar.PlayFloatAnim` | `void PlayFloatAnim ( Direction Dir )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行115） |
| `EntityFamiliar.PlayShootAnim` | `void PlayShootAnim ( Direction Dir )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行120） |
| `EntityFamiliar.Player` | `EntityPlayer Player` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行232） |
| `EntityFamiliar.RecalculateOrbitOffset` | `int RecalculateOrbitOffset ( int Layer, boolean Add )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行125） |
| `EntityFamiliar.RemoveFromDelayed` | `void RemoveFromDelayed ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行130） |
| `EntityFamiliar.RemoveFromFollowers` | `void RemoveFromFollowers ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行135） |
| `EntityFamiliar.RemoveFromOrbit` | `void RemoveFromOrbit ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行140） |
| `EntityFamiliar.RemoveFromPlayer` | `void RemoveFromPlayer ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行104） |
| `EntityFamiliar.RoomClearCount` | `int RoomClearCount` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行237） |
| `EntityFamiliar.SetLilDelirium` | `void SetLilDelirium ( boolean isLilDelirium )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行108） |
| `EntityFamiliar.SetMoveDelayNum` | `void SetMoveDelayNum ( int Delay )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行112） |
| `EntityFamiliar.Shoot` | `void Shoot ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行145） |
| `EntityFamiliar.ShootDirection` | `Direction ShootDirection` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行242） |
| `EntityFamiliar.State` | `int State` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityFamiliar.md)（行247） |
| `EntityFamiliar.TriggerRoomClear` | `void TriggerRoomClear ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行117） |
| `EntityFamiliar.TryAimAtMarkedTarget` | `Vector TryAimAtMarkedTarget ( Vector AimDirection, Direction Direction = Direction.NO_DIRECTION )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行121） |
| `EntityFamiliar.TryAimAtMarkedTarget` | `boolean, table TryAimAtMarkedTarget ( Vector AimDirection = nil, Direction Direction = Direction.NO_DIRECTION, Vector TargetPos = nil )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行122） |
| `EntityFamiliar.UpdateDirtColor` | `void UpdateDirtColor ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityFamiliar.md)（行130） |
| `EntityKnife.AddTearFlags` | `void AddTearFlags ( TearFlags Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行21） |
| `EntityKnife.Charge` | `float Charge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行72） |
| `EntityKnife.ClearTearFlags` | `void ClearTearFlags ( TearFlags Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行26） |
| `EntityKnife.FireSplitTear` | `EntityTear FireSplitTear ( Vector Position, Vector Velocity, float DamageMultiplier = 0.5, float SizeMultiplier = 0.6, int Variant = 0, SplitTearType splitType = SplitTearType.SPLIT_GENERIC )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行12） |
| `EntityKnife.GetHitList` | `int[] GetHitList ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行24） |
| `EntityKnife.GetHitboxParentKnife` | `EntityKnife GetHitboxParentKnife ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行19） |
| `EntityKnife.GetIsSpinAttack` | `boolean GetIsSpinAttack ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行29） |
| `EntityKnife.GetIsSwinging` | `boolean GetIsSwinging ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行33） |
| `EntityKnife.GetKnifeDistance` | `float GetKnifeDistance ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行31） |
| `EntityKnife.GetKnifeVelocity` | `float GetKnifeVelocity ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行36） |
| `EntityKnife.GetRenderZ` | `int GetRenderZ ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行41） |
| `EntityKnife.HasTearFlags` | `boolean HasTearFlags ( TearFlags Flags )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行46） |
| `EntityKnife.IsFlying` | `boolean IsFlying ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行51） |
| `EntityKnife.IsMultidimensionalTouched` | `boolean IsMultidimensionalTouched ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行37） |
| `EntityKnife.IsPrismTouched` | `boolean IsPrismTouched ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行42） |
| `EntityKnife.MaxDistance` | `float MaxDistance` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行77） |
| `EntityKnife.PathFollowSpeed` | `float PathFollowSpeed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行82） |
| `EntityKnife.PathOffset` | `float PathOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行87） |
| `EntityKnife.Reset` | `void Reset ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行56） |
| `EntityKnife.Rotation` | `float Rotation` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行92） |
| `EntityKnife.RotationOffset` | `float RotationOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行97） |
| `EntityKnife.Scale` | `float Scale` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行102） |
| `EntityKnife.SetHitboxParentKnife` | `void SetHitboxParentKnife ( EntityKnife Knife )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行47） |
| `EntityKnife.SetIsSpinAttack` | `void SetIsSpinAttack ( boolean isSpinAttack )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行54） |
| `EntityKnife.SetIsSwinging` | `void SetIsSwinging ( boolean isSwinging )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行58） |
| `EntityKnife.SetKnifeDistance` | `void SetKnifeDistance ( float Distance )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行62） |
| `EntityKnife.SetKnifeVelocity` | `void SetKnifeVelocity ( float Velocity )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行66） |
| `EntityKnife.SetMultidimensionalTouched` | `void SetMultidimensionalTouched ( boolean IsTouched )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行70） |
| `EntityKnife.SetPathFollowSpeed` | `void SetPathFollowSpeed ( float Speed )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行61） |
| `EntityKnife.SetPrismTouched` | `void SetPrismTouched ( boolean IsTouched )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityKnife.md)（行75） |
| `EntityKnife.Shoot` | `void Shoot ( float Charge, float Range )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行66） |
| `EntityKnife.TearFlags` | `TearFlags TearFlags` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityKnife.md)（行107） |
| `EntityLaser.AddTearFlags` | `void AddTearFlags ( TearFlags Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行24） |
| `EntityLaser.Angle` | `float Angle` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行149） |
| `EntityLaser.AngleDegrees` | `float AngleDegrees` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行154） |
| `EntityLaser.BlackHpDropChance` | `float BlackHpDropChance` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行159） |
| `EntityLaser.BounceLaser` | `Entity BounceLaser` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行164） |
| `EntityLaser.CalculateEndPoint` | `static Vector CalculateEndPoint ( Vector Start, Vector Dir, Vector PositionOffset, Entity Parent, float Margin )` | explicit_readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行29） |
| `EntityLaser.ClearTearFlags` | `void ClearTearFlags ( TearFlags Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行34） |
| `EntityLaser.CurveStrength` | `float CurveStrength` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行169） |
| `EntityLaser.DisableFollowParent` | `boolean DisableFollowParent` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行174） |
| `EntityLaser.EndPoint` | `Vector EndPoint` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行179） |
| `EntityLaser.FireSplitTear` | `EntityTear FireSplitTear ( Vector Position, Vector Velocity, float DamageMultiplier = 0.5, float SizeMultiplier = 0.6, int Variant = 0, SplitTearType splitType = SplitTearType.SPLIT_GENERIC )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行20） |
| `EntityLaser.FirstUpdate` | `boolean FirstUpdate` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行184） |
| `EntityLaser.GetDamageMultiplier` | `float GetDamageMultiplier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行27） |
| `EntityLaser.DisableFollowParent` | `boolean GetDisableFollowParent ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行31） |
| `EntityLaser.EndPoint` | `const Vector GetEndPoint ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行39） |
| `EntityLaser.GetHitList` | `int GetHitList ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行35） |
| `EntityLaser.GetNonOptimizedSamples` | `const VectorList GetNonOptimizedSamples ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行44） |
| `EntityLaser.GetNumChainedLasers` | `int GetNumChainedLasers ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行40） |
| `EntityLaser.OneHit` | `boolean GetOneHit ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行45） |
| `EntityLaser.GetRenderZ` | `int GetRenderZ ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行60） |
| `EntityLaser.GetSamples` | `const VectorList GetSamples ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行65） |
| `EntityLaser.GetScale` | `float GetScale ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行49） |
| `EntityLaser.Shrink` | `boolean GetShrink ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行53） |
| `EntityLaser.Timeout` | `int GetTimeout ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行57） |
| `EntityLaser.GridHit` | `boolean GridHit` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行189） |
| `EntityLaser.HasTearFlags` | `boolean HasTearFlags ( TearFlags Flags )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行84） |
| `EntityLaser.HomingLaser` | `HomingLaser HomingLaser` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行194） |
| `EntityLaser.HomingType` | `LaserHomingType HomingType` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行199） |
| `EntityLaser.HomingType` | `int HomingType` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行12） |
| `EntityLaser.IsActiveRotating` | `boolean IsActiveRotating` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行204） |
| `EntityLaser.IsCircleLaser` | `boolean IsCircleLaser ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行89） |
| `EntityLaser.IsMultidimensionalTouched` | `boolean IsMultidimensionalTouched ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行61） |
| `EntityLaser.IsPrismTouched` | `boolean IsPrismTouched ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行66） |
| `EntityLaser.SampleLaser` | `boolean IsSampleLaser ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行103） |
| `EntityLaser.LaserLength` | `float LaserLength` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行209） |
| `EntityLaser.LastAngleDegrees` | `float LastAngleDegrees` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行214） |
| `EntityLaser.MaxDistance` | `float MaxDistance` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行219） |
| `EntityLaser.OneHit` | `boolean OneHit` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行224） |
| `EntityLaser.ParentOffset` | `Vector ParentOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行229） |
| `EntityLaser.Radius` | `float Radius` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行234） |
| `EntityLaser.RecalculateSamplesNextUpdate` | `void RecalculateSamplesNextUpdate ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行71） |
| `EntityLaser.ResetSpriteScale` | `void ResetSpriteScale ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行76） |
| `EntityLaser.RotateToAngle` | `void RotateToAngle ( float Angle, float Speed = 8.0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行80） |
| `EntityLaser.RotationDegrees` | `float RotationDegrees` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行239） |
| `EntityLaser.RotationDelay` | `int RotationDelay` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行244） |
| `EntityLaser.RotationSpd` | `float RotationSpd` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行249） |
| `EntityLaser.SampleLaser` | `boolean SampleLaser` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行254） |
| `EntityLaser.SetActiveRotation` | `void SetActiveRotation ( int Delay, float AngleDegrees, float RotationSpd, boolean TimeoutComplete )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行108） |
| `EntityLaser.SetBlackHpDropChance` | `void SetBlackHpDropChance ( float Chance )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行113） |
| `EntityLaser.SetDamageMultiplier` | `void SetDamageMultiplier ( float DamageMult )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行84） |
| `EntityLaser.SetDisableFollowParent` | `void SetDisableFollowParent ( boolean Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行88） |
| `EntityLaser.SetHomingType` | `void SetHomingType ( LaserHomingType Type )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行118） |
| `EntityLaser.SetInitSound` | `void SetInitSound ( SoundEffect Sound )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行92） |
| `EntityLaser.SetMaxDistance` | `void SetMaxDistance ( float Distance )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行123） |
| `EntityLaser.SetMultidimensionalTouched` | `void SetMultidimensionalTouched ( boolean Value )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行128） |
| `EntityLaser.SetNumChainedLasers` | `void SetNumChainedLasers ( int Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行97） |
| `EntityLaser.SetOneHit` | `void SetOneHit ( boolean Value )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行133） |
| `EntityLaser.SetPrismTouched` | `void SetPrismTouched ( boolean IsTouched )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行102） |
| `EntityLaser.SetScale` | `void SetScale ( float Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行107） |
| `EntityLaser.SetShrink` | `void SetShrink ( boolean Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行111） |
| `EntityLaser.SetTimeout` | `void SetTimeout ( int Value )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行138）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityLaser.md)（行115） |
| `EntityLaser.ShootAngle` | `static EntityLaser ShootAngle ( int Variant, Vector SourcePos, float AngleDegrees, int Timeout, Vector PosOffset, Entity Source )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行143） |
| `EntityLaser.Shrink` | `boolean Shrink` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行259） |
| `EntityLaser.StartAngleDegrees` | `float StartAngleDegrees` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行264） |
| `EntityLaser.TearFlags` | `TearFlags TearFlags` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行270） |
| `EntityLaser.Timeout` | `int Timeout` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityLaser.md)（行274） |
| `EntityList.Get` | `Entity Get ( int idx )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/CppContainer_EntityList.md)（行32） |
| `EntityList.Size` | `const int Size` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/CppContainer_EntityList.md)（行40） |
| `EntityList.__len` | `int __len ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/CppContainer_EntityList.md)（行24） |
| `EntityNPC.AnimWalkFrame` | `void AnimWalkFrame ( string HorizontalAnim, string VerticalAnim, float SpeedThreshold )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行21） |
| `EntityNPC.ApplyTearflagEffects` | `void ApplyTearflagEffects ( Vector Position, TearFlags TearFlags, Entity Source = nil, float Damage = 3.5 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行25） |
| `EntityNPC.CalcTargetPosition` | `Vector CalcTargetPosition ( float DistanceLimit )` | explicit_readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行26） |
| `EntityNPC.CanBeDamagedFromVelocity` | `boolean CanBeDamagedFromVelocity ( Vector Velocity )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行31） |
| `EntityNPC.CanReroll` | `boolean CanReroll ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行36） |
| `EntityNPC.CanShutDoors` | `boolean CanShutDoors` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行177） |
| `EntityNPC.ChildNPC` | `const EntityNPC ChildNPC` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行182） |
| `EntityNPC.ClearFlyingOverride` | `void ClearFlyingOverride ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行32） |
| `EntityNPC.EntityRef` | `Entity EntityRef` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行187） |
| `EntityNPC.FireBossProjectiles` | `EntityProjectile FireBossProjectiles ( int NumProjectiles, Vector TargetPos, float TrajectoryModifier, ProjectileParams Params )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行41） |
| `EntityNPC.FireBossProjectilesEx` | `EntityProjectile[] FireBossProjectilesEx ( int NumProjectiles, Vector TargetPos, float TrajectoryModifier, ProjectileParams Params )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行37） |
| `EntityNPC.FireGridEntity` | `EntityProjectile FireGridEntity ( Sprite GridEntitySprite, GridEntityDesc GridEntityDesc, Vector Velocity, BackdropType Backdrop = BackdropType.BASEMENT )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行42） |
| `EntityNPC.FireProjectiles` | `void FireProjectiles ( Vector Pos, Vector Velocity, ProjectilesMode Mode, ProjectileParams Params )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行46） |
| `EntityNPC.FireProjectilesEx` | `EntityProjectile[] FireProjectilesEx (Vector Position, Vector Velocity, ProjectilesMode Mode, ProjectileParams Params)` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行46） |
| `EntityNPC.GetAliveEnemyCount` | `int GetAliveEnemyCount ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行63） |
| `EntityNPC.GetBossColorIdx` | `int GetBossColorIdx ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行69）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行51） |
| `EntityNPC.GetChampionColorIdx` | `ChampionColorIdx GetChampionColorIdx ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行78） |
| `EntityNPC.GetControllerId` | `int GetControllerId ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行56） |
| `EntityNPC.GetDarkRedChampionRegenTimer` | `int GetDarkRedChampionRegenTimer ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行61） |
| `EntityNPC.GetDirtColor` | `Color GetDirtColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行65） |
| `EntityNPC.GetFireplaceLoot` | `LootList GetFireplaceLoot ( )` | actual_event_only gameplay_loot_generation | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行70） |
| `EntityNPC.GetFlyingOverride` | `boolean GetFlyingOverride ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行75） |
| `EntityNPC.GetHitList` | `int[] GetHitList ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行79） |
| `EntityNPC.Pathfinder` | `Pathfinder GetPathfinder ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行13） |
| `EntityNPC.GetPlayerTarget` | `Entity GetPlayerTarget ( )` | actual_event_only gameplay_target_selection_and_optional_cache:J273-6BA580 | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行84） |
| `EntityNPC.GetShieldStrength` | `float GetShieldStrength ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行84） |
| `EntityNPC.GetShopkeeperLoot` | `LootList GetShopkeeperLoot ( )` | actual_event_only gameplay_loot_generation | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行88） |
| `EntityNPC.GetSirenPlayerEntity` | `EntityPlayer GetSirenPlayerEntity ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行93） |
| `EntityNPC.GroupIdx` | `int GroupIdx` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行192） |
| `EntityNPC.I1` | `int I1` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行197） |
| `EntityNPC.I2` | `int I2` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行205） |
| `EntityNPC.IsBoss` | `boolean IsBoss ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行89） |
| `EntityNPC.IsBossColor` | `boolean IsBossColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行97） |
| `EntityNPC.IsChampion` | `boolean IsChampion ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行94） |
| `EntityNPC.KillUnique` | `void KillUnique ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行99） |
| `EntityNPC.MakeChampion` | `void MakeChampion ( int Seed, ChampionColor ChampionColorIdx = -1, boolean Init = false )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行104） |
| `EntityNPC.MakeSplat` | `EntityEffect MakeSplat ( float Size )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行113） |
| `EntityNPC.Morph` | `boolean Morph ( EntityType type, int Variant, int SubType, int ChampionColorIdx )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行118） |
| `EntityNPC.ParentNPC` | `const EntityNPC ParentNPC` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行213） |
| `EntityNPC.Pathfinder` | `PathFinder Pathfinder` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行219） |
| `EntityNPC.PlaySound` | `void PlaySound ( SoundEffect ID, float Volume, int FrameDelay, boolean Loop, float Pitch )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行146） |
| `EntityNPC.PlaySound` | `void PlaySound ( int ID, float Volume = 1.0, int FrameDelay = 2, boolean Loop = true, float Pitch = 1.0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行17） |
| `EntityNPC.ProjectileCooldown` | `int ProjectileCooldown` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行224） |
| `EntityNPC.ProjectileDelay` | `int ProjectileDelay` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行230） |
| `EntityNPC.QueryNPCsGroup` | `EntityList QueryNPCsGroup ( int GroupIdx )` | explicit_readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行151） |
| `EntityNPC.QueryNPCsSpawnerType` | `EntityList QueryNPCsSpawnerType ( EntityType SpawnerType, EntityType Type, boolean OnlyEnemies )` | explicit_readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行156） |
| `EntityNPC.QueryNPCsType` | `EntityList QueryNPCsType ( EntityType Type, int Variant )` | explicit_readonly_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行161） |
| `EntityNPC.ReplaceSpritesheet` | `boolean ReplaceSpritesheet ( int LayerId, string PngFilename, boolean LoadGraphics = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行101） |
| `EntityNPC.ResetPathFinderTarget` | `void ResetPathFinderTarget ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行166） |
| `EntityNPC.Scale` | `float Scale` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行235） |
| `EntityNPC.SetControllerId` | `int SetControllerId ( int ControllerId )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行106） |
| `EntityNPC.SetFlyingOverride` | `void SetFlyingOverride ( boolean CanFly )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行111） |
| `EntityNPC.SetShieldStrength` | `void SetShieldStrength ( float Strength )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行116） |
| `EntityNPC.ShootMaggotProjectile` | `static const EntityNPC ShootMaggotProjectile ( Vector Position, Vector Target, float FallingSpeed = -8.0, float YOffset = -24.0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行120） |
| `EntityNPC.SpawnBloodCloud` | `EntityEffect SpawnBloodCloud ( Vector Position, Color Color )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行124） |
| `EntityNPC.SpawnBloodSplash` | `void SpawnBloodSplash ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行128） |
| `EntityNPC.State` | `NpcState State` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行240） |
| `EntityNPC.StateFrame` | `int StateFrame` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行245） |
| `EntityNPC.ThrowLeech` | `static const EntityNPC ThrowLeech ( Vector Position, Entity Source, Vector Target, float YPosOffset = -10.0, boolean Big = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行132） |
| `EntityNPC.ThrowMaggot` | `static const EntityNPC ThrowMaggot ( Vector Origin, Vector Velocity, float YOffset = -10.0, float FallSpeed = -8.0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行136） |
| `EntityNPC.ThrowMaggotAtPos` | `static const EntityNPC ThrowMaggotAtPos ( Vector Origin, Vector Target, float YOffset = -8.0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行140） |
| `EntityNPC.ThrowRockSpider` | `static const EntityNPC ThrowRockSpider ( Vector Position, Entity Source, Vector Velocity, int Variant = 0, float YPosOffset = -10.0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行144） |
| `EntityNPC.ThrowSpider` | `static const EntityNPC ThrowSpider ( Vector Position, Entity Spawner, Vector TargetPos, boolean Big, float YOffset )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行171） |
| `EntityNPC.ThrowStrider` | `static const EntityNPC ThrowStrider ( Vector Position, Entity Source, Vector Target )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行148） |
| `EntityNPC.TryForceTarget` | `boolean TryForceTarget ( Entity Target, int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行152） |
| `EntityNPC.TrySplit` | `boolean TrySplit ( float DefaultDamage, EntityRef Source, boolean DoScreenEffects = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行157） |
| `EntityNPC.TryThrow` | `boolean TryThrow ( EntityRef Source, Vector Direction, float Force )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行162） |
| `EntityNPC.UpdateDirtColor` | `void UpdateDirtColor ( boolean Immediate )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行169） |
| `EntityNPC.V1` | `Vector V1` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行250）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行178） |
| `EntityNPC.V2` | `Vector V2` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityNPC.md)（行256）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityNPC.md)（行182） |
| `EntityPickup.AddCollectibleCycle` | `boolean AddCollectibleCycle ( int id )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行12） |
| `EntityPickup.AppearFast` | `void AppearFast ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行21） |
| `EntityPickup.AutoUpdatePrice` | `boolean AutoUpdatePrice` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行65） |
| `EntityPickup.CanReroll` | `boolean CanReroll ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行26）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行16） |
| `EntityPickup.Charge` | `int Charge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行70） |
| `EntityPickup.GetAlternatePedestal` | `int GetAlternatePedestal ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行20） |
| `EntityPickup.GetCoinValue` | `int GetCoinValue ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行31） |
| `EntityPickup.GetCollectibleCycle` | `CollectibleType[] GetCollectibleCycle ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行24） |
| `EntityPickup.GetDropDelay` | `int GetDropDelay ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行29） |
| `EntityPickup.GetFlipCollectible` | `CollectibleType GetFlipCollectible ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行33） |
| `EntityPickup.GetLootList` | `LootList GetLootList ( boolean shouldAdvance = false )` | actual_event_only gameplay_loot_generation | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行38） |
| `EntityPickup.GetMegaChestLeftCollectible` | `EntityPickup GetMegaChestLeftCollectible ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行44） |
| `EntityPickup.GetMegaChestRightCollectible` | `EntityPickup GetMegaChestRightCollectible ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行54） |
| `EntityPickup.GetMegaChestRightCollectible` | `EntityPickup, boolean GetMegaChestRightCollectible ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行49） |
| `EntityPickup.GetPickupGhost` | `EntityEffect GetPickupGhost ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行59） |
| `EntityPickup.GetPriceSprite` | `Sprite GetPriceSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行64） |
| `EntityPickup.GetRandomPickupVelocity` | `Vector GetRandomPickupVelocity ( Vector Position, RNG RNG = nil, int VelocityType = 0 )` | actual_event_only rng_consuming_call | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行68） |
| `EntityPickup.GetVarData` | `int GetVarData ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行78） |
| `EntityPickup.HasFlipData` | `boolean HasFlipData ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行82） |
| `EntityPickup.InitFlipState` | `void InitFlipState ( CollectibleType = CollectibleType.COLLECTIBLE_NULL, boolean SetupCollectibleGraphics = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行87） |
| `EntityPickup.IsBlind` | `boolean IsBlind ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行92） |
| `EntityPickup.IsShopItem` | `boolean IsShopItem ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行36） |
| `EntityPickup.MakeShopItem` | `void MakeShopItem ( int ShopItemID )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行99） |
| `EntityPickup.Morph` | `void Morph ( EntityType Type, int Variant, int SubType, boolean KeepPrice = false, boolean KeepSeed = false, boolean IgnoreModifiers = false )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行41） |
| `EntityPickup.OptionsPickupIndex` | `int OptionsPickupIndex` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行75） |
| `EntityPickup.PlayDropSound` | `void PlayDropSound ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行49） |
| `EntityPickup.PlayPickupSound` | `void PlayPickupSound ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行54） |
| `EntityPickup.Price` | `int Price` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行85） |
| `EntityPickup.ReloadGraphics` | `void ReloadGraphics ( boolean IgnoreBlind )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行103） |
| `EntityPickup.RemoveCollectibleCycle` | `void RemoveCollectibleCycle ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行107） |
| `EntityPickup.SetAlternatePedestal` | `void SetAlternatePedestal ( int PedestalType )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行111） |
| `EntityPickup.SetDropDelay` | `void SetDropDelay ( int Delay )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行116） |
| `EntityPickup.SetForceBlind` | `void SetForceBlind ( boolean SetBlind )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行120） |
| `EntityPickup.SetNewOptionsPickupIndex` | `int SetNewOptionsPickupIndex ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行125） |
| `EntityPickup.SetVarData` | `void SetVarData ( int VarData )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行135） |
| `EntityPickup.SetupCollectibleGraphics` | `static void SetupCollectibleGraphics ( Sprite Sprite, integer Layer, CollectibleType Collectible, boolean Blind = false, integer Seed = Random(), boolean LoadGraphics = false  )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行130） |
| `EntityPickup.ShopItemId` | `int ShopItemId` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行97） |
| `EntityPickup.State` | `int State` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行110） |
| `EntityPickup.Timeout` | `int Timeout` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行115） |
| `EntityPickup.Touched` | `boolean Touched` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行122） |
| `EntityPickup.TriggerTheresOptionsPickup` | `void TriggerTheresOptionsPickup ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行139） |
| `EntityPickup.TryFlip` | `boolean TryFlip ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行144） |
| `EntityPickup.TryInitOptionCycle` | `boolean TryInitOptionCycle ( int NumCycle )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行149） |
| `EntityPickup.TryOpenChest` | `boolean TryOpenChest ( EntityPlayer Player = nil )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行59） |
| `EntityPickup.TryRemoveCollectible` | `boolean TryRemoveCollectible ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行154） |
| `EntityPickup.UpdatePickupGhosts` | `void UpdatePickupGhosts ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPickup.md)（行161） |
| `EntityPickup.Wait` | `int Wait` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPickup.md)（行127） |
| `EntityPlayer.AddActiveCharge` | `int AddActiveCharge ( int Charge, ActiveSlot Slot = ActiveSlot.SLOT_PRIMARY, boolean FlashHUD = true, boolean Overcharge = false, boolean Force = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行99） |
| `EntityPlayer.AddBlackHearts` | `void AddBlackHearts ( int BlackHearts )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行29） |
| `EntityPlayer.AddBloodCharge` | `void AddBloodCharge ( int Amount )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行44） |
| `EntityPlayer.AddBlueFlies` | `Entity AddBlueFlies ( int Amount, Vector Position, Entity Target )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行51） |
| `EntityPlayer.AddBlueSpider` | `Entity AddBlueSpider ( Vector Position )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行59） |
| `EntityPlayer.AddBombs` | `void AddBombs ( int Amount )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行75） |
| `EntityPlayer.AddBoneHearts` | `void AddBoneHearts ( int Hearts )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行90） |
| `EntityPlayer.AddBoneOrbital` | `EntityFamiliar AddBoneOrbital ( Vector Position )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行107） |
| `EntityPlayer.AddBrokenHearts` | `void AddBrokenHearts ( int BrokenHearts )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行105） |
| `EntityPlayer.AddCacheFlags` | `void AddCacheFlags ( CacheFlag CacheFlag )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行120） |
| `EntityPlayer.AddCacheFlags` | `void AddCacheFlags ( CacheFlag CacheFlag, boolean EvaluateItems = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行14） |
| `EntityPlayer.AddCandyHeartBonus` | `void AddCandyHeartBonus ( CacheFlag CacheFlag = 0, int Amount = 1 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行111） |
| `EntityPlayer.AddCard` | `void AddCard ( Card ID )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行134） |
| `EntityPlayer.AddCoins` | `void AddCoins ( int Amount )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行139） |
| `EntityPlayer.AddCollectible` | `void AddCollectible ( CollectibleType Type, int Charge = 0, boolean FirstTimePickingUp = true, ActiveSlot Slot = ActiveSlot.SLOT_PRIMARY, int VarData = 0)` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行154） |
| `EntityPlayer.AddCollectible` | `void AddCollectible ( CollectibleType Type, int Charge = 0, boolean FirstTimePickingUp = true, ActiveSlot Slot = ActiveSlot.SLOT_PRIMARY, int VarData = 0, ItemPoolType PoolType )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行156） |
| `EntityPlayer.AddCollectibleEffect` | `void AddCollectibleEffect ( CollectibleType CollectibleType, bool ApplyCostume = false, int Cooldown = VanillaCooldown, bool Additive = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行19） |
| `EntityPlayer.AddControlsCooldown` | `void AddControlsCooldown ( int Cooldown )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行196） |
| `EntityPlayer.AddCostume` | `void AddCostume ( ItemConfigItem Item, boolean ItemStateOnly )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行201） |
| `EntityPlayer.AddCurseMistEffect` | `void AddCurseMistEffect ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行206） |
| `EntityPlayer.AddDeadEyeCharge` | `void AddDeadEyeCharge ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行211） |
| `EntityPlayer.AddDollarBillEffect` | `void AddDollarBillEffect ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行216） |
| `EntityPlayer.AddEternalHearts` | `void AddEternalHearts ( int EternalHearts )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行221） |
| `EntityPlayer.AddFriendlyDip` | `void AddFriendlyDip ( int Subtype, Vector Position )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行238） |
| `EntityPlayer.AddGigaBombs` | `void AddGigaBombs ( int GigaBombs )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行258） |
| `EntityPlayer.AddGoldenBomb` | `void AddGoldenBomb ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行268） |
| `EntityPlayer.AddGoldenHearts` | `void AddGoldenHearts ( int Hearts )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行273） |
| `EntityPlayer.AddGoldenKey` | `void AddGoldenKey ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行288） |
| `EntityPlayer.AddHearts` | `void AddHearts ( int Hearts )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行293） |
| `EntityPlayer.AddInnateCollectible` | `void AddInnateCollectible ( CollectibleType Collectible, int Amount = 1, string GroupKey = "", int Duration = -1, bool AddCostume = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行123） |
| `EntityPlayer.AddInnateTrinket` | `void AddInnateTrinket ( TrinketType Trinket, int Amount = 1, string GroupKey = "", int Duration = -1, bool AddCostume = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行132） |
| `EntityPlayer.AddItemWisp` | `EntityFamiliar AddItemWisp ( CollectibleType Collectible, Vector Position, boolean AdjustOrbitLayer = false )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行308） |
| `EntityPlayer.AddJarFlies` | `void AddJarFlies ( int Flies )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行313） |
| `EntityPlayer.AddJarHearts` | `void AddJarHearts ( int Hearts )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行318） |
| `EntityPlayer.AddKeys` | `void AddKeys ( int Amount )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行323） |
| `EntityPlayer.AddLeprosy` | `void AddLeprosy ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行137） |
| `EntityPlayer.AddLocust` | `void AddLocust ( CollectibleType Collectible, Vector Position )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行144） |
| `EntityPlayer.AddMaxHearts` | `void AddMaxHearts ( int MaxHearts, boolean IgnoreKeeper )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行338） |
| `EntityPlayer.AddMinisaac` | `EntityFamiliar AddMinisaac ( Vector Position, boolean PlayAnim = true )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行368） |
| `EntityPlayer.AddNullCostume` | `void AddNullCostume ( NullItemID NullId )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行373） |
| `EntityPlayer.AddNullItemEffect` | `void AddNullItemEffect ( NullItemID NullItemID, bool ApplyCostume = false, int Cooldown = VanillaCooldown, bool Additive = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行24） |
| `EntityPlayer.AddPill` | `void AddPill ( PillColor Pill )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行378） |
| `EntityPlayer.AddPlayerFormCostume` | `void AddPlayerFormCostume ( PlayerForm Form )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行383） |
| `EntityPlayer.AddPoopMana` | `void AddPoopMana ( int Num )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行390） |
| `EntityPlayer.AddPrettyFly` | `void AddPrettyFly ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行397） |
| `EntityPlayer.AddRottenHearts` | `void AddRottenHearts ( int RottenHearts )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行402） |
| `EntityPlayer.AddSmeltedTrinket` | `boolean AddSmeltedTrinket ( TrinketType Trinket, boolean FirstTimePickingUp = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行168） |
| `EntityPlayer.AddSoulCharge` | `void AddSoulCharge ( int Amount )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行417） |
| `EntityPlayer.AddSoulHearts` | `void AddSoulHearts ( int SoulHearts )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行424） |
| `EntityPlayer.AddSoulLocketBonus` | `void AddSoulLocketBonus ( CacheFlag CacheFlag = 0, int Amount = 1 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行175） |
| `EntityPlayer.AddSwarmFlyOrbital` | `EntityFamiliar AddSwarmFlyOrbital ( Vector Position )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行439） |
| `EntityPlayer.AddTrinket` | `void AddTrinket ( TrinketType Type, boolean FirstTimePickingUp = true )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行444） |
| `EntityPlayer.AddTrinketEffect` | `void AddTrinketEffect ( TrinketType TrinketType, bool ApplyCostume = false, int Cooldown = VanillaCooldown, bool Additive = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行29） |
| `EntityPlayer.AddUrnSouls` | `void AddUrnSouls ( int Count = 0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行180） |
| `EntityPlayer.AddWisp` | `EntityFamiliar AddWisp ( CollectibleType Collectible, Vector Position, boolean AdjustOrbitLayer = false, boolean DontUpdate = false )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行465） |
| `EntityPlayer.AnimateAppear` | `void AnimateAppear ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行474） |
| `EntityPlayer.AnimateCard` | `void AnimateCard ( Card ID, string AnimName = "Pickup" )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行479） |
| `EntityPlayer.AnimateCollectible` | `void AnimateCollectible ( CollectibleType Collectible, string AnimName = "Pickup", string SpriteAnimName = "PlayerPickupSparkle" )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行484） |
| `EntityPlayer.AnimateHappy` | `void AnimateHappy ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行491） |
| `EntityPlayer.AnimateLightTravel` | `void AnimateLightTravel ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行505） |
| `EntityPlayer.AnimatePickup` | `void AnimatePickup ( Sprite sprite, boolean HideShadow = false, string AnimName = "Pickup" )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行520） |
| `EntityPlayer.AnimatePill` | `void AnimatePill ( PillColor Pill, string AnimName = "Pickup" )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行529） |
| `EntityPlayer.AnimatePitfallIn` | `void AnimatePitfallIn ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行534） |
| `EntityPlayer.AnimatePitfallOut` | `void AnimatePitfallOut ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行541） |
| `EntityPlayer.AnimateSad` | `void AnimateSad ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行548） |
| `EntityPlayer.AnimateTeleport` | `void AnimateTeleport ( boolean Up )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行562） |
| `EntityPlayer.AnimateTrapdoor` | `void AnimateTrapdoor ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行569） |
| `EntityPlayer.AnimateTrinket` | `void AnimateTrinket ( TrinketType Trinket, string AnimName = "Pickup", string SpriteAnimName = "PlayerPickupSparkle" )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行584） |
| `EntityPlayer.AreControlsEnabled` | `boolean AreControlsEnabled ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行589） |
| `EntityPlayer.AreOpposingShootDirectionsPressed` | `boolean AreOpposingShootDirectionsPressed ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行594） |
| `EntityPlayer.BabySkin` | `BabySubType BabySkin` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2015）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行85） |
| `EntityPlayer.BlockCollectible` | `void BlockCollectible ( CollectibleType Collectible )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行184） |
| `EntityPlayer.BlockTrinket` | `void BlockTrinket ( TrinketType Trinket )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行189） |
| `EntityPlayer.CalculateBagOfCraftingOutput` | `static CollectibleType, ItemPoolType CalculateBagOfCraftingOutput ( BagOfCraftingPickup[] pickups )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行298） |
| `EntityPlayer.CanAddCollectible` | `boolean CanAddCollectible ( CollectibleType Type = CollectibleType.COLLECTIBLE_NULL )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行600） |
| `EntityPlayer.CanAddCollectibleToInventory` | `boolean CanAddCollectibleToInventory ( CollectibleType Collectible )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行194） |
| `EntityPlayer.CanCrushRocks` | `boolean CanCrushRocks ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行199） |
| `EntityPlayer.CanFly` | `boolean CanFly` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2024） |
| `EntityPlayer.CanOverrideActiveItem` | `boolean CanOverrideActiveItem ( ActiveSlot Slot )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行212） |
| `EntityPlayer.CanPickBlackHearts` | `boolean CanPickBlackHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行605） |
| `EntityPlayer.CanPickBoneHearts` | `boolean CanPickBoneHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行611） |
| `EntityPlayer.CanPickGoldenHearts` | `boolean CanPickGoldenHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行617） |
| `EntityPlayer.CanPickRedHearts` | `boolean CanPickRedHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行623） |
| `EntityPlayer.CanPickRottenHearts` | `boolean CanPickRottenHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行628） |
| `EntityPlayer.CanPickSoulHearts` | `boolean CanPickSoulHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行634） |
| `EntityPlayer.CanPickupItem` | `boolean CanPickupItem ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行640） |
| `EntityPlayer.CanShoot` | `boolean CanShoot ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行646） |
| `EntityPlayer.CanTurnHead` | `boolean CanTurnHead ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行651） |
| `EntityPlayer.CanUsePill` | `boolean CanUsePill ( PillEffect ID )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行217） |
| `EntityPlayer.ChangePlayerType` | `void ChangePlayerType ( PlayerType PlayerType )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行657） |
| `EntityPlayer.CheckFamiliar` | `void CheckFamiliar ( FamiliarVariant FamiliarVariant, int TargetCount, RNG rng, ItemConfigItem SourceItemConfigItem = nil, int FamiliarSubType = -1 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行667） |
| `EntityPlayer.CheckFamiliarEx` | `EntityFamiliar[] CheckFamiliarEx ( int FamiliarVariant Familiar, int TargetCount, RNG rng, ItemConfigItem SourceItemConfigItem = nil, int FamiliarSubType = -1 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行222） |
| `EntityPlayer.ClearCollectibleAnim` | `void ClearCollectibleAnim ( CollectibleType Collectible)` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行228） |
| `EntityPlayer.ClearCostumes` | `void ClearCostumes ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行705） |
| `EntityPlayer.ClearDeadEyeCharge` | `void ClearDeadEyeCharge ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行711） |
| `EntityPlayer.ClearDeadEyeCharge` | `void ClearDeadEyeCharge ( boolean Force = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行34） |
| `EntityPlayer.ClearInnateItemGroup` | `void ClearInnateItemGroup ( string GroupKey )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行232） |
| `EntityPlayer.ClearQueueItem` | `void ClearQueueItem ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行237） |
| `EntityPlayer.ClearTemporaryEffects` | `void ClearTemporaryEffects ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行716） |
| `EntityPlayer.ControllerIndex` | `const int ControllerIndex` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2029） |
| `EntityPlayer.ControlsCooldown` | `int ControlsCooldown` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2034） |
| `EntityPlayer.ControlsEnabled` | `boolean ControlsEnabled` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2041） |
| `EntityPlayer.CreateAfterimage` | `void CreateAfterimage ( int Duration, Vector Position )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行241） |
| `EntityPlayer.Damage` | `float Damage` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2046） |
| `EntityPlayer.DischargeActiveItem` | `void DischargeActiveItem ( ActiveSlot ActiveSlot = ActiveSlot.SLOT_PRIMARY )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行723） |
| `EntityPlayer.DoZitEffect` | `void DoZitEffect ( Vector Direction )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行737） |
| `EntityPlayer.DonateLuck` | `void DonateLuck ( int Luck )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行730） |
| `EntityPlayer.DropCollectible` | `EntityPickup DropCollectible ( CollectibleType Collectible, EntityPickup ExistingPedestal = nil, boolean RemoveFromPlayerForm = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行246） |
| `EntityPlayer.DropCollectibleByHistoryIndex` | `EntityPickup DropCollectibleByHistoryIndex ( int Idx, EntityPickup ExistingPedestal = nil )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行252） |
| `EntityPlayer.DropPocketItem` | `void DropPocketItem ( int PocketNum, Vector Pos )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行744） |
| `EntityPlayer.DropTrinket` | `void DropTrinket ( Vector DropPos, boolean ReplaceTick )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行751） |
| `EntityPlayer.EnableWeaponType` | `void EnableWeaponType ( WeaponType Weapon, boolean Set )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行258） |
| `EntityPlayer.EvaluateItems` | `void EvaluateItems ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行756） |
| `EntityPlayer.FireBomb` | `EntityBomb FireBomb ( Vector Position, Vector Velocity, Entity Source = nil )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行775） |
| `EntityPlayer.FireBrimstone` | `EntityLaser FireBrimstone ( Vector Direction, Entity Source = nil, float DamageMultiplier = 1 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行780） |
| `EntityPlayer.FireBrimstoneBall` | `EntityEffect FireBrimstoneBall ( Vector Position, Vector Velocity, Vector Offset = Vector.Zero )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行262） |
| `EntityPlayer.FireDelay` | `float FireDelay` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2051） |
| `EntityPlayer.FireDelayedBrimstone` | `EntityLaser FireDelayedBrimstone ( float Angle, Entity Parent )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行785） |
| `EntityPlayer.FireKnife` | `EntityKnife FireKnife ( Entity Parent, float RotationOffset = 0, boolean CantOverwrite = false, int SubType = 0, int Variant = 0 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行790） |
| `EntityPlayer.FireTear` | `EntityTear FireTear ( Vector Position, Vector Velocity, boolean CanBeEye = true, boolean NoTractorBeam = false, boolean CanTriggerStreakEnd = true, Entity Source = nil, float DamageMultiplier = 1 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行809） |
| `EntityPlayer.FireTechLaser` | `EntityLaser FireTechLaser ( Vector Position, LaserOffset OffsetID, Vector Direction, boolean LeftEye, boolean OneHit = false, Entity Source = nil, float DamageMultiplier = 1 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行818） |
| `EntityPlayer.FireTechXLaser` | `EntityLaser FireTechXLaser ( Vector Position, Vector Direction, float Radius, Entity Source = nil, float DamageMultiplier = 1 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行827） |
| `EntityPlayer.FlushQueueItem` | `boolean FlushQueueItem ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行832） |
| `EntityPlayer.FriendBallEnemy` | `EntityDesc FriendBallEnemy` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行90） |
| `EntityPlayer.FriendBallEnemy` | `const EntityDesc FriendBallEnemy` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2060） |
| `EntityPlayer.FullCharge` | `boolean FullCharge ( ActiveSlot ActiveSlot = ActiveSlot.SLOT_PRIMARY, int Force = false )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行838） |
| `EntityPlayer.GetActionHoldDrop` | `int GetActionHoldDrop ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行269） |
| `EntityPlayer.GetActiveCharge` | `int GetActiveCharge ( ActiveSlot ActiveSlot = ActiveSlot.SLOT_PRIMARY )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行851） |
| `EntityPlayer.GetActiveItem` | `CollectibleType GetActiveItem ( ActiveSlot ActiveSlot = ActiveSlot.SLOT_PRIMARY )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行857） |
| `EntityPlayer.GetActiveItemDesc` | `ActiveItemDesc GetActiveItemDesc ( ActiveSlot Slot = ActiveSlot.SLOT_PRIMARY )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行273） |
| `EntityPlayer.GetActiveItemSlot` | `ActiveSlot GetActiveItemSlot ( CollectibleType Collectible )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行277） |
| `EntityPlayer.GetActiveMaxCharge` | `int GetActiveMaxCharge ( ActiveSlot Slot )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行281） |
| `EntityPlayer.GetActiveMinUsableCharge` | `int GetActiveMinUsableCharge ( ActiveSlot Slot )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行285） |
| `EntityPlayer.GetActiveSubCharge` | `int GetActiveSubCharge ( ActiveSlot ActiveSlot = ActiveSlot.SLOT_PRIMARY )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行864） |
| `EntityPlayer.GetActiveWeaponEntity` | `Entity GetActiveWeaponEntity ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行875） |
| `EntityPlayer.GetActiveWeaponNumFired` | `int GetActiveWeaponNumFired ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行289） |
| `EntityPlayer.GetAimDirection` | `const Vector GetAimDirection ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行880） |
| `EntityPlayer.BabySkin` | `BabySubType GetBabySkin ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行885） |
| `EntityPlayer.GetBagOfCraftingContent` | `BagOfCraftingPickup[] GetBagOfCraftingContent ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行293） |
| `EntityPlayer.GetBagOfCraftingOutput` | `CollectibleType GetBagOfCraftingOutput ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行302） |
| `EntityPlayer.GetBagOfCraftingSlot` | `BagOfCraftingPickup GetBagOfCraftingSlot ( int SlotID )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行306） |
| `EntityPlayer.GetBatteryCharge` | `int GetBatteryCharge ( ActiveSlot ActiveSlot = ActiveSlot.SLOT_PRIMARY )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行890） |
| `EntityPlayer.GetBlackHearts` | `int GetBlackHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行896） |
| `EntityPlayer.GetBladderCharge` | `int GetBladderCharge ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行310） |
| `EntityPlayer.GetBlinkLockTime` | `int GetBlinkLockTime ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行315） |
| `EntityPlayer.GetBloodCharge` | `int GetBloodCharge ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行938） |
| `EntityPlayer.GetBloodGushSprite` | `Sprite GetBloodGushSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行320） |
| `EntityPlayer.GetBloodLustCounter` | `int GetBloodLustCounter ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行325） |
| `EntityPlayer.GetBodyColor` | `SkinColor GetBodyColor ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行945） |
| `EntityPlayer.GetBodyMoveDirection` | `Vector GetBodyMoveDirection ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行329） |
| `EntityPlayer.GetBodySprite` | `Sprite GetBodySprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行338） |
| `EntityPlayer.GetBombFlags` | `int GetBombFlags ( )` | actual_event_only gameplay_attack_flags_and_rng_even_when_false:J273-779880 | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行950） |
| `EntityPlayer.GetBombFlags` | `int GetBombFlags ( boolean IsFetus = false )` | actual_event_only gameplay_attack_flags_and_rng_even_when_false:J273-779880 | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行952） |
| `EntityPlayer.GetBombPlaceDelay` | `int GetBombPlaceDelay ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行333） |
| `EntityPlayer.GetBombVariant` | `BombVariant GetBombVariant ( TearFlags TearFlags, boolean ForceSmallBomb )` | actual_event_only gameplay_bomb_selection | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行958） |
| `EntityPlayer.GetBoneHearts` | `int GetBoneHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行965） |
| `EntityPlayer.GetBrokenHearts` | `int GetBrokenHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行974） |
| `EntityPlayer.GetCambionConceptionState` | `int GetCambionConceptionState ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行343） |
| `EntityPlayer.GetCambionPregnancyLevel` | `int GetCambionPregnancyLevel ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行355） |
| `EntityPlayer.GetCandyHeartBonus` | `table GetCandyHeartBonus ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行348） |
| `EntityPlayer.GetCard` | `Card GetCard ( int SlotId )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行981） |
| `EntityPlayer.GetCardRNG` | `RNG GetCardRNG ( Card ID )` | owner_qualified_reference random:tx-04-14 | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行987） |
| `EntityPlayer.GetCharmOfTheVampireKills` | `int GetCharmOfTheVampireKills ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行360） |
| `EntityPlayer.GetCollectibleCount` | `int GetCollectibleCount ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行992） |
| `EntityPlayer.GetCollectibleNum` | `int GetCollectibleNum ( CollectibleType Collectible, boolean OnlyCountTrueItems = false, bool IgnoreSpoof = false )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行39） |
| `EntityPlayer.GetCollectibleNum` | `int GetCollectibleNum ( CollectibleType Type, boolean OnlyCountTrueItems = false )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行997） |
| `EntityPlayer.GetCollectibleRNG` | `RNG GetCollectibleRNG ( CollectibleType ID )` | owner_qualified_reference random:tx-04-14 | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1003） |
| `EntityPlayer.GetCollectiblesList` | `table GetCollectiblesList ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行364） |
| `EntityPlayer.GetConceptionFamiliarFlags` | `ConceptionFamiliarFlag GetConceptionFamiliarFlags ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行378） |
| `EntityPlayer.GetCostumeLayerMap` | `table GetCostumeLayerMap ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行383） |
| `EntityPlayer.GetCostumeNullPos` | `Vector GetCostumeNullPos ( string NullFrameName, boolean HeadScale, Vector Direction )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1019） |
| `EntityPlayer.GetCostumeSpriteDescs` | `CostumeSpriteDesc[] GetCostumeSpriteDescs ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行422） |
| `EntityPlayer.GetCustomCacheValue` | `float GetCustomCacheValue ( string CustomCacheTag )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行427） |
| `EntityPlayer.GetD8DamageModifier` | `float GetD8DamageModifier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行434） |
| `EntityPlayer.GetD8FireDelayModifier` | `float GetD8FireDelayModifier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行438） |
| `EntityPlayer.GetD8RangeModifier` | `float GetD8RangeModifier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行442） |
| `EntityPlayer.GetD8SpeedModifier` | `float GetD8SpeedModifier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行446） |
| `EntityPlayer.GetDamageCooldown` | `int GetDamageCooldown ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1024） |
| `EntityPlayer.GetDamageModifier` | `int GetDamageModifier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行450） |
| `EntityPlayer.GetDeadEyeCharge` | `int GetDeadEyeCharge ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行459） |
| `EntityPlayer.GetDeathAnimName` | `string GetDeathAnimName ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行463） |
| `EntityPlayer.GetDonateLuck` | `int GetDonateLuck ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行474） |
| `EntityPlayer.GetEdenDamage` | `float GetEdenDamage ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行478） |
| `EntityPlayer.GetEdenFireDelay` | `float GetEdenFireDelay ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行483） |
| `EntityPlayer.GetEdenLuck` | `float GetEdenLuck ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行488） |
| `EntityPlayer.GetEdenRange` | `float GetEdenRange ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行493） |
| `EntityPlayer.GetEdenShotSpeed` | `float GetEdenShotSpeed ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行498） |
| `EntityPlayer.GetEdenSpeed` | `float GetEdenSpeed ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行503） |
| `EntityPlayer.GetEffectiveBloodCharge` | `int GetEffectiveBloodCharge ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1033） |
| `EntityPlayer.GetEffectiveMaxHearts` | `int GetEffectiveMaxHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1040） |
| `EntityPlayer.GetEffectiveSoulCharge` | `int GetEffectiveSoulCharge ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1049） |
| `EntityPlayer.GetEffects` | `TemporaryEffects GetEffects ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1056） |
| `EntityPlayer.GetEnterPosition` | `Vector GetEnterPosition ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行508） |
| `EntityPlayer.GetEntityConfigPlayer` | `EntityConfigPlayer GetEntityConfigPlayer ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行512） |
| `EntityPlayer.GetEpiphoraCharge` | `int GetEpiphoraCharge ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行516） |
| `EntityPlayer.GetErrorTrinketEffect` | `TrinketType GetErrorTrinketEffect ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行520） |
| `EntityPlayer.GetEternalHearts` | `int GetEternalHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1061） |
| `EntityPlayer.GetEveSumptoriumCharge` | `int GetEveSumptoriumCharge ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行527） |
| `EntityPlayer.GetExtraLives` | `int GetExtraLives ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1068） |
| `EntityPlayer.GetFireDelayModifier` | `int GetFireDelayModifier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行532） |
| `EntityPlayer.GetFireDirection` | `Direction GetFireDirection ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1075） |
| `EntityPlayer.GetFlippedForm` | `EntityPlayer GetFlippedForm ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行541） |
| `EntityPlayer.GetFlyingOffset` | `Vector GetFlyingOffset ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1080） |
| `EntityPlayer.GetFocusEntity` | `Entity GetFocusEntity ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行548） |
| `EntityPlayer.GetFootprintColor` | `KColor GetFootprintColor ( boolean LeftFootprint )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行554） |
| `EntityPlayer.GetForgottenSwapFormCooldown` | `int GetForgottenSwapFormCooldown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行558） |
| `EntityPlayer.GetGlitchBabySubType` | `int GetGlitchBabySubType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行562） |
| `EntityPlayer.GetGlyphOfBalanceDrop` | `table GetGlyphOfBalanceDrop ( int Variant = -1, int SubType = -1 )` | actual_event_only gameplay_drop_selection | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行566） |
| `EntityPlayer.GetGnawedLeafTimer` | `int GetGnawedLeafTimer ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行570） |
| `EntityPlayer.GetGoldenHearts` | `int GetGoldenHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1085） |
| `EntityPlayer.GetGreedDonationBreakChance` | `float GetGreedDonationBreakChance ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1092） |
| `EntityPlayer.GetGreedsGulletHearts` | `int GetGreedsGulletHearts ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行574） |
| `EntityPlayer.GetHallowedGroundCountdown` | `int GetHallowedGroundCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行578） |
| `EntityPlayer.GetHeadColor` | `SkinColor GetHeadColor ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1097） |
| `EntityPlayer.GetHeadDirection` | `Direction GetHeadDirection ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1102） |
| `EntityPlayer.GetHeadDirectionLockTime` | `int GetHeadDirectionLockTime ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行583） |
| `EntityPlayer.GetHealthType` | `HealthType GetHealthType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行588） |
| `EntityPlayer.GetHeartLimit` | `int GetHeartLimit ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1107） |
| `EntityPlayer.GetHearts` | `int GetHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1112） |
| `EntityPlayer.GetHeldEntity` | `Entity GetHeldEntity ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行592） |
| `EntityPlayer.GetHeldSprite` | `Sprite GetHeldSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行598） |
| `EntityPlayer.GetHistory` | `History GetHistory ( )` | owner_qualified_reference player_history:tx-04-13 | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行603） |
| `EntityPlayer.GetImExcitedSpeedupCountdown` | `int GetImExcitedSpeedupCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行607） |
| `EntityPlayer.GetImmaculateConceptionState` | `int GetImmaculateConceptionState ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行611） |
| `EntityPlayer.GetInnateCollectibleCount` | `int GetInnateCollectibleCount ( CollectibleType Collectible, string GroupKey = "" )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行616） |
| `EntityPlayer.GetInnateCollectibleGroup` | `table GetInnateCollectibleGroup ( string GroupKey )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行621） |
| `EntityPlayer.GetInnateTrinketCount` | `int GetInnateTrinketCount ( TrinketType Trinket, string GroupKey = "" )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行628） |
| `EntityPlayer.GetInnateTrinketGroup` | `table GetInnateTrinketGroup ( string GroupKey )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行635） |
| `EntityPlayer.GetItemState` | `CollectibleType GetItemState ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1119） |
| `EntityPlayer.GetItemStateCooldown` | `int GetItemStateCooldown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行644） |
| `EntityPlayer.GetJarFlies` | `int GetJarFlies ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1124） |
| `EntityPlayer.GetJarHearts` | `int GetJarHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1129） |
| `EntityPlayer.GetKeepersSackBonus` | `int GetKeepersSackBonus ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行648） |
| `EntityPlayer.LaserColor` | `Color GetLaserColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行653） |
| `EntityPlayer.GetLaserOffset` | `Vector GetLaserOffset ( LaserOffset ID, Vector Direction )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1134） |
| `EntityPlayer.GetLastActionTriggers` | `int GetLastActionTriggers ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1139） |
| `EntityPlayer.GetLastDamageFlags` | `int GetLastDamageFlags ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1144） |
| `EntityPlayer.GetLastDamageSource` | `const EntityRef GetLastDamageSource ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1149） |
| `EntityPlayer.GetLastDirection` | `const Vector GetLastDirection ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1154） |
| `EntityPlayer.GetLuckModifier` | `int GetLuckModifier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行657） |
| `EntityPlayer.GetMaggyHealthDrainCooldown` | `int GetMaggyHealthDrainCooldown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行666） |
| `EntityPlayer.GetMaggySwingCooldown` | `int GetMaggySwingCooldown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行670） |
| `EntityPlayer.GetMainTwin` | `EntityPlayer GetMainTwin ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1159） |
| `EntityPlayer.GetMarkedTarget` | `EntityEffect GetMarkedTarget ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行675） |
| `EntityPlayer.GetMawOfTheVoidCharge` | `int GetMawOfTheVoidCharge ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行681） |
| `EntityPlayer.GetMaxBladderCharge` | `int GetMaxBladderCharge ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行685） |
| `EntityPlayer.GetMaxBombs` | `int GetMaxBombs ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行690） |
| `EntityPlayer.GetMaxCoins` | `int GetMaxCoins ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行695） |
| `EntityPlayer.GetMaxHearts` | `int GetMaxHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1171） |
| `EntityPlayer.GetMaxKeys` | `int GetMaxKeys ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行700） |
| `EntityPlayer.GetMaxPeeBurstCooldown` | `int GetMaxPeeBurstCooldown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行705） |
| `EntityPlayer.GetMaxPocketItems` | `int GetMaxPocketItems ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1178）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行710） |
| `EntityPlayer.GetMaxPoopMana` | `int GetMaxPoopMana ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1187） |
| `EntityPlayer.GetMaxTrinkets` | `int GetMaxTrinkets ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1194） |
| `EntityPlayer.GetMegaBlastDuration` | `int GetMegaBlastDuration ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行714） |
| `EntityPlayer.GetMetronomeCollectibleID` | `CollectibleType GetMetronomeCollectibleID ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行718） |
| `EntityPlayer.GetModelingClayEffect` | `CollectibleType GetModelingClayEffect ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1201） |
| `EntityPlayer.GetMontezumaRevengeCharge` | `int GetMontezumaRevengeCharge ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行722） |
| `EntityPlayer.GetMovementDirection` | `Direction GetMovementDirection ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1206） |
| `EntityPlayer.GetMovementInput` | `const Vector GetMovementInput ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1211） |
| `EntityPlayer.GetMovementJoystick` | `Vector GetMovementJoystick ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1216） |
| `EntityPlayer.GetMovementVector` | `Vector GetMovementVector ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1221） |
| `EntityPlayer.GetMovingBoxContents` | `EntitiesSaveStateVector GetMovingBoxContents ( )` | owner_qualified_reference room_save:tx-04-13 | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行726） |
| `EntityPlayer.GetMultiShotParams` | `MultiShotParams GetMultiShotParams ( WeaponType WeaponType )` | actual_event_only gameplay_attack_parameters | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行44） |
| `EntityPlayer.GetMultiShotParams` | `MultiShotParams GetMultiShotParams ( WeaponType WeaponType = WeaponType.WEAPON_TEARS )` | actual_event_only gameplay_attack_parameters | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1226） |
| `EntityPlayer.GetMultiShotPositionVelocity` | `PosVel GetMultiShotPositionVelocity ( int LoopIndex, WeaponType Weapon, Vector ShotDirection, float ShotSpeed, MultiShotParams params )` | actual_event_only gameplay_attack_parameters | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1234） |
| `EntityPlayer.GetMultiShotPositionVelocity` | `PosVel GetMultiShotPositionVelocity ( int LoopIndex, WeaponType WeaponType, Vector ShotDirection, float ShotSpeed, MultiShotParams Params )` | actual_event_only gameplay_attack_parameters | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行49） |
| `EntityPlayer.GetNPCTarget` | `Entity GetNPCTarget ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1252） |
| `EntityPlayer.GetName` | `string GetName ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1245） |
| `EntityPlayer.GetNextUrethraBlockFrame` | `int GetNextUrethraBlockFrame ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行731） |
| `EntityPlayer.GetNumBlueFlies` | `int GetNumBlueFlies ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1258） |
| `EntityPlayer.GetNumBlueSpiders` | `int GetNumBlueSpiders ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1263） |
| `EntityPlayer.GetNumBombs` | `int GetNumBombs ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1268） |
| `EntityPlayer.GetNumCoins` | `int GetNumCoins ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1273） |
| `EntityPlayer.GetNumGigaBombs` | `int GetNumGigaBombs ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1278） |
| `EntityPlayer.GetNumKeys` | `int GetNumKeys ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1283） |
| `EntityPlayer.GetOtherTwin` | `EntityPlayer GetOtherTwin ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1288） |
| `EntityPlayer.GetPeeBurstCooldown` | `int GetPeeBurstCooldown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行740） |
| `EntityPlayer.GetPill` | `PillColor GetPill ( int SlotId )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1301） |
| `EntityPlayer.GetPillRNG` | `RNG GetPillRNG ( PillEffect ID )` | owner_qualified_reference random:tx-04-14 | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1307） |
| `EntityPlayer.GetPlanCKillCountdown` | `int GetPlanCKillCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行736） |
| `EntityPlayer.GetPlayerFormCounter` | `int GetPlayerFormCounter ( PlayerForm PlayerFormID )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行750） |
| `EntityPlayer.GetPlayerHUD` | `PlayerHUD GetPlayerHUD ( )` | owner_qualified_reference presentation:tx-04-14 | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行755） |
| `EntityPlayer.GetPlayerIndex` | `int GetPlayerIndex ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行759） |
| `EntityPlayer.GetPlayerType` | `PlayerType GetPlayerType ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1312） |
| `EntityPlayer.GetPocketItem` | `PocketItem GetPocketItem ( PillCardSlot Slot )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行56） |
| `EntityPlayer.GetPocketItem` | `const PlayerPocketItem GetPocketItem ( int SlotId )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1317） |
| `EntityPlayer.GetPonyCharge` | `int GetPonyCharge ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行763） |
| `EntityPlayer.GetPoopMana` | `int GetPoopMana ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1327） |
| `EntityPlayer.GetPoopSpell` | `PoopSpellType GetPoopSpell ( int Position )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1334） |
| `EntityPlayer.GetPotatoPeelerUses` | `int GetPotatoPeelerUses ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行745） |
| `EntityPlayer.GetPurityState` | `PurityState GetPurityState ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行768） |
| `EntityPlayer.GetRUAWizardTimer` | `int GetRUAWizardTimer ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行806） |
| `EntityPlayer.GetRecentMovementVector` | `const Vector GetRecentMovementVector ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1341） |
| `EntityPlayer.GetRedStewBonusDuration` | `int GetRedStewBonusDuration ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行773） |
| `EntityPlayer.GetRevelationCharge` | `float GetRevelationCharge ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行778） |
| `EntityPlayer.GetRockBottomDamage` | `float GetRockBottomDamage ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行782） |
| `EntityPlayer.GetRockBottomLuck` | `float GetRockBottomLuck ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行786） |
| `EntityPlayer.GetRockBottomMaxFireDelay` | `float GetRockBottomMaxFireDelay ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行790） |
| `EntityPlayer.GetRockBottomMoveSpeed` | `float GetRockBottomMoveSpeed ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行794） |
| `EntityPlayer.GetRockBottomShotSpeed` | `float GetRockBottomShotSpeed ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行798） |
| `EntityPlayer.GetRockBottomTearRange` | `float GetRockBottomTearRange ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行802） |
| `EntityPlayer.GetRottenHearts` | `int GetRottenHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1348） |
| `EntityPlayer.GetShootingInput` | `Vector GetShootingInput ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1353） |
| `EntityPlayer.GetShootingJoystick` | `Vector GetShootingJoystick ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1364） |
| `EntityPlayer.GetShotSpeedModifier` | `int GetShotSpeedModifier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行810） |
| `EntityPlayer.GetSmeltedTrinketDesc` | `table GetSmeltedTrinketDesc ( TrinketType )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行831） |
| `EntityPlayer.GetSmeltedTrinkets` | `table GetSmeltedTrinkets ( TrinketType[] TrinketList = nil )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行819） |
| `EntityPlayer.GetSmoothBodyRotation` | `float GetSmoothBodyRotation ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1373） |
| `EntityPlayer.GetSoulCharge` | `int GetSoulCharge ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1378） |
| `EntityPlayer.GetSoulHearts` | `int GetSoulHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1385） |
| `EntityPlayer.GetSoulLocketBonus` | `table GetSoulLocketBonus ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行841） |
| `EntityPlayer.GetSpecialGridCollision` | `int GetSpecialGridCollision ( Vector Position = self.Position )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行848） |
| `EntityPlayer.GetSpeedModifier` | `int GetSpeedModifier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行852） |
| `EntityPlayer.GetSpoofedCollectiblesList` | `table[] GetSpoofedCollectiblesList ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行861） |
| `EntityPlayer.GetStatMultiplier` | `float GetStatMultiplier ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行871） |
| `EntityPlayer.GetSubPlayer` | `EntityPlayer GetSubPlayer ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1396） |
| `EntityPlayer.GetSuplexAimCountdown` | `int GetSuplexAimCountdown ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行880） |
| `EntityPlayer.GetSuplexLandPosition` | `Vector GetSuplexLandPosition ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行884） |
| `EntityPlayer.GetSuplexState` | `SuplexState GetSuplexState ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行888） |
| `EntityPlayer.GetSuplexTargetPosition` | `Vector GetSuplexTargetPosition ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行892） |
| `EntityPlayer.GetTearDisplacement` | `int GetTearDisplacement ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行896） |
| `EntityPlayer.GetTearHitParams` | `TearParams GetTearHitParams ( WeaponType WeaponType, float DamageScale = 1, int TearDisplacement = 1, Entity Source = nil )` | actual_event_only gameplay_attack_parameters | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1403） |
| `EntityPlayer.GetTearMovementInheritance` | `Vector GetTearMovementInheritance ( Vector ShotDirection )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1410） |
| `EntityPlayer.GetTearPoisonDamage` | `float GetTearPoisonDamage ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1415） |
| `EntityPlayer.GetTearRangeModifier` | `int GetTearRangeModifier ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1420）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行905） |
| `EntityPlayer.GetTearsCap` | `int GetTearsCap ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行914） |
| `EntityPlayer.GetTotalActiveCharge` | `int GetTotalActiveCharge ( ActiveSlot Slot )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行919） |
| `EntityPlayer.GetTotalDamageTaken` | `int GetTotalDamageTaken ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1427） |
| `EntityPlayer.GetTractorBeam` | `Entity GetTractorBeam ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1432） |
| `EntityPlayer.GetTrinket` | `TrinketType GetTrinket ( int TrinketIndex )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1437） |
| `EntityPlayer.GetTrinketMultiplier` | `int GetTrinketMultiplier ( TrinketType TrinketID )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1443） |
| `EntityPlayer.GetTrinketRNG` | `RNG GetTrinketRNG ( TrinketType TrinketID )` | owner_qualified_reference random:tx-04-14 | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1453） |
| `EntityPlayer.GetUrnSouls` | `int GetUrnSouls ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行923） |
| `EntityPlayer.GetVelocityBeforeUpdate` | `const Vector GetVelocityBeforeUpdate ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1458） |
| `EntityPlayer.GetVoidedCollectiblesList` | `CollectibleType[] GetVoidedCollectiblesList ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行927） |
| `EntityPlayer.GetWeapon` | `Weapon GetWeapon ( int Slot )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行932） |
| `EntityPlayer.GetWeaponModifiers` | `int GetWeaponModifiers ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行948） |
| `EntityPlayer.GetWildCardItem` | `int GetWildCardItem ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行953） |
| `EntityPlayer.GetWildCardItemType` | `PocketItemType GetWildCardItemType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行960） |
| `EntityPlayer.GetWispCollectiblesList` | `table GetWispCollectiblesList ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行967） |
| `EntityPlayer.GetZodiacEffect` | `CollectibleType GetZodiacEffect ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1463） |
| `EntityPlayer.HasCamoEffect` | `boolean HasCamoEffect ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行972） |
| `EntityPlayer.HasChanceRevive` | `boolean HasChanceRevive ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行976） |
| `EntityPlayer.HasCollectible` | `boolean HasCollectible ( CollectibleType Collectible, boolean IgnoreModifiers = false, boolean IgnoreSpoof = false )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行63） |
| `EntityPlayer.HasCollectible` | `boolean HasCollectible ( CollectibleType Type, boolean IgnoreModifiers = false )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1468） |
| `EntityPlayer.HasCurseMistEffect` | `boolean HasCurseMistEffect ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1474） |
| `EntityPlayer.HasForcedCamoEffect` | `boolean HasForcedCamoEffect ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行981） |
| `EntityPlayer.HasFullHearts` | `boolean HasFullHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1479） |
| `EntityPlayer.HasFullHeartsAndSoulHearts` | `boolean HasFullHeartsAndSoulHearts ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1484） |
| `EntityPlayer.HasGoldenBomb` | `boolean HasGoldenBomb ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1489） |
| `EntityPlayer.HasGoldenKey` | `boolean HasGoldenKey ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1494） |
| `EntityPlayer.HasGoldenTrinket` | `boolean HasGoldenTrinket ( TrinketType Trinket )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行985） |
| `EntityPlayer.HasInstantDeathCurse` | `boolean HasInstantDeathCurse ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行990） |
| `EntityPlayer.HasInvincibility` | `boolean HasInvincibility ( DamageFlag Flags = 0 )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1499） |
| `EntityPlayer.HasPlayerForm` | `boolean HasPlayerForm ( PlayerForm Form )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1504） |
| `EntityPlayer.HasPoisonImmunity` | `boolean HasPoisonImmunity ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行995） |
| `EntityPlayer.HasTimedItem` | `boolean HasTimedItem ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1509） |
| `EntityPlayer.HasTrinket` | `boolean HasTrinket ( TrinketType Type, boolean IgnoreModifiers = false )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1515） |
| `EntityPlayer.HasWeaponType` | `boolean HasWeaponType ( WeaponType WeaponType )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1521） |
| `EntityPlayer.HeadFrameDelay` | `int HeadFrameDelay` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2067） |
| `EntityPlayer.IBSCharge` | `float IBSCharge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2074） |
| `EntityPlayer.IncrementPlayerFormCounter` | `void IncrementPlayerFormCounter ( PlayerForm Form, int Count )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1006） |
| `EntityPlayer.InitBabySkin` | `void InitBabySkin ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1526） |
| `EntityPlayer.InitPostLevelInitStats` | `void InitPostLevelInitStats ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1011） |
| `EntityPlayer.InitTwin` | `EntityPlayer InitTwin ( PlayerType PlayerType )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1016） |
| `EntityPlayer.IsBlackHeart` | `boolean IsBlackHeart ( int Heart )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1531） |
| `EntityPlayer.IsBoneHeart` | `boolean IsBoneHeart ( int heart )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1574） |
| `EntityPlayer.IsCollectibleAnimFinished` | `boolean IsCollectibleAnimFinished ( CollectibleType Collectible, string Animation )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1021） |
| `EntityPlayer.IsCollectibleBlocked` | `boolean IsCollectibleBlocked ( CollectibleType Collectible )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1026） |
| `EntityPlayer.IsCollectibleCostumeVisible` | `boolean IsCollectibleCostumeVisible ( CollectibleType Collectible, int PlayerSpriteLayerID )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1031） |
| `EntityPlayer.IsCollectibleCostumeVisible` | `boolean IsCollectibleCostumeVisible ( CollectibleType Collectible, string PlayerSpriteLayerName )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1032） |
| `EntityPlayer.IsCoopGhost` | `boolean IsCoopGhost ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1621） |
| `EntityPlayer.IsEntityValidTarget` | `boolean IsEntityValidTarget ( Entity Entity )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1037） |
| `EntityPlayer.IsExtraAnimationFinished` | `boolean IsExtraAnimationFinished ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1627） |
| `EntityPlayer.IsFootstepFrame` | `boolean IsFootstepFrame ( int Foot = -1 )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1042） |
| `EntityPlayer.IsFullSpriteRendering` | `boolean IsFullSpriteRendering ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1632） |
| `EntityPlayer.IsHeadless` | `boolean IsHeadless ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1050） |
| `EntityPlayer.IsHeldItemVisible` | `boolean IsHeldItemVisible ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1637） |
| `EntityPlayer.IsHoldingItem` | `boolean IsHoldingItem ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1642） |
| `EntityPlayer.IsHologram` | `boolean IsHologram ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1055） |
| `EntityPlayer.IsInvisible` | `boolean IsInvisible ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1060） |
| `EntityPlayer.IsItemCostumeVisible` | `boolean IsItemCostumeVisible ( ItemConfig_Item Item, int PlayerSpriteLayerID )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1065） |
| `EntityPlayer.IsItemCostumeVisible` | `boolean IsItemCostumeVisible ( ItemConfig_Item Item, int PlayerSpriteLayerName )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1066） |
| `EntityPlayer.IsItemQueueEmpty` | `boolean IsItemQueueEmpty ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1647） |
| `EntityPlayer.IsLocalPlayer` | `boolean IsLocalPlayer ( )` | owner_qualified_reference isolated_original_online_locality;use_cs_control_relation | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1070） |
| `EntityPlayer.IsNullItemCostumeVisible` | `boolean IsNullItemCostumeVisible ( int nullItem, int layerID = 0 )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1075） |
| `EntityPlayer.IsNullItemCostumeVisible` | `boolean IsNullItemCostumeVisible ( int nullItem, string layerName )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1076） |
| `EntityPlayer.IsP2Appearing` | `boolean IsP2Appearing ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1652） |
| `EntityPlayer.IsPosInSpotLight` | `boolean IsPosInSpotLight ( Vector Position )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1657） |
| `EntityPlayer.IsPostLevelInitFinished` | `boolean IsPostLevelInitFinished ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1080） |
| `EntityPlayer.IsSubPlayer` | `boolean IsSubPlayer ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1663） |
| `EntityPlayer.IsTrinketBlocked` | `boolean IsTrinketBlocked ( TrinketType Trinket )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1084） |
| `EntityPlayer.IsUrethraBlocked` | `boolean IsUrethraBlocked ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1089） |
| `EntityPlayer.ItemHoldCooldown` | `int ItemHoldCooldown` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2079） |
| `EntityPlayer.LaserColor` | `Color LaserColor` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2084） |
| `EntityPlayer.Luck` | `float Luck` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2089） |
| `EntityPlayer.MaxFireDelay` | `float MaxFireDelay` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2094） |
| `EntityPlayer.MorphToCoopGhost` | `void MorphToCoopGhost ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1094） |
| `EntityPlayer.MoveSpeed` | `float MoveSpeed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2103） |
| `EntityPlayer.NeedsCharge` | `boolean NeedsCharge ( ActiveSlot ActiveSlot = ActiveSlot.SLOT_PRIMARY )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1671） |
| `EntityPlayer.PlayCollectibleAnim` | `void PlayCollectibleAnim ( CollectibleType Collectible, boolean CheckBodyLayers, string AnimationName, int Frame = -1 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1099） |
| `EntityPlayer.PlayDelayedSFX` | `void PlayDelayedSFX ( SoundEffect ID, int SoundDelay = 0, int FrameDelay = 2, float Volume = 1.0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1104） |
| `EntityPlayer.PlayExtraAnimation` | `void PlayExtraAnimation ( string Animation )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1678） |
| `EntityPlayer.PlayItemNullAnimation` | `boolean PlayItemNullAnimation ( string AnimationName )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1109） |
| `EntityPlayer.QueueExtraAnimation` | `void QueueExtraAnimation ( string Animation )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1683） |
| `EntityPlayer.QueueItem` | `void QueueItem ( ItemConfigItem Item, int Charge = 0, boolean Touched = false, boolean Golden = false, int VarData = 0 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1688） |
| `EntityPlayer.QueuedItem` | `QueueItemData QueuedItem` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2108） |
| `EntityPlayer.RemoveBlackHeart` | `void RemoveBlackHeart ( int BlackHeart )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1695） |
| `EntityPlayer.RemoveBlueFly` | `void RemoveBlueFly ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1700） |
| `EntityPlayer.RemoveBlueSpider` | `void RemoveBlueSpider ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1705） |
| `EntityPlayer.RemoveCollectible` | `void RemoveCollectible ( CollectibleType Type, boolean IgnoreModifiers = false, ActiveSlot ActiveSlot = ActiveSlot.SLOT_PRIMARY, boolean RemoveFromPlayerForm = true )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1710） |
| `EntityPlayer.RemoveCollectibleByHistoryIndex` | `void RemoveCollectibleByHistoryIndex ( int Index )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1114） |
| `EntityPlayer.RemoveCostume` | `void RemoveCostume ( ItemConfigItem Item )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1719） |
| `EntityPlayer.RemoveCurseMistEffect` | `void RemoveCurseMistEffect ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1734） |
| `EntityPlayer.RemoveGoldenBomb` | `void RemoveGoldenBomb ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1739） |
| `EntityPlayer.RemoveGoldenKey` | `void RemoveGoldenKey ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1744） |
| `EntityPlayer.RemoveInnateCollectible` | `int RemoveInnateCollectible ( CollectibleType Collectible, int Amount = 1, string GroupKey = "" )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1128） |
| `EntityPlayer.RemoveInnateTrinket` | `int RemoveInnateTrinket ( TrinketType Trinket, int Amount = 1, string GroupKey = "" )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1133） |
| `EntityPlayer.RemovePocketItem` | `void RemovePocketItem ( PillCardSlot Slot )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1119） |
| `EntityPlayer.RemovePoopSpell` | `void RemovePoopSpell ( int Position = 0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1123） |
| `EntityPlayer.RemoveSkinCostume` | `void RemoveSkinCostume ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1749） |
| `EntityPlayer.RenderBody` | `void RenderBody ( Vector position )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1755） |
| `EntityPlayer.RenderGlow` | `void RenderGlow ( Vector position )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1760） |
| `EntityPlayer.RenderHead` | `void RenderHead ( Vector position )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1765） |
| `EntityPlayer.RenderTop` | `void RenderTop ( Vector position )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1770） |
| `EntityPlayer.ReplaceCostumeSprite` | `void ReplaceCostumeSprite ( ItemConfigItem Item, string SpritePath, int SpriteId )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1775） |
| `EntityPlayer.RerollAllCollectibles` | `void RerollAllCollectibles ( RNG rng, boolean includeActiveItems )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1140） |
| `EntityPlayer.ResetDamageCooldown` | `void ResetDamageCooldown ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1780） |
| `EntityPlayer.ResetItemState` | `void ResetItemState ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1785） |
| `EntityPlayer.ResetPlayer` | `void ResetPlayer ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1145） |
| `EntityPlayer.RespawnFamiliars` | `void RespawnFamiliars ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1790） |
| `EntityPlayer.Revive` | `void Revive ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1796） |
| `EntityPlayer.ReviveCoopGhost` | `boolean ReviveCoopGhost ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1151） |
| `EntityPlayer.SalvageCollectible` | `void SalvageCollectible ( CollectibleType Collectible, Vector position = playerPosition, RNG rng = PlayerDropRNG, ItemPoolType Pool = ItemPoolType.POOL_NULL )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1156） |
| `EntityPlayer.SalvageCollectible` | `void SalvageCollectible ( EntityPickup Pickup, RNG rng = PickupDropRNG, ItemPoolType Pool = ItemPoolType.POOL_NULL)` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1155） |
| `EntityPlayer.SamsonBerserkCharge` | `int SamsonBerserkCharge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2119） |
| `EntityPlayer.SecondaryActiveItem` | `ActiveItemDesc SecondaryActiveItem` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2124） |
| `EntityPlayer.SetActionHoldDrop` | `void SetActionHoldDrop ( int duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1164） |
| `EntityPlayer.SetActiveCharge` | `void SetActiveCharge ( int Charge, ActiveSlot ActiveSlot = ActiveSlot.SLOT_PRIMARY )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1804） |
| `EntityPlayer.SetActiveVarData` | `void SetActiveVarData ( int VarData, ActiveSlot Slot )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1168） |
| `EntityPlayer.SetBagOfCraftingContent` | `void SetBagOfCraftingContent ( BagOfCraftingPickup[] ContentTable )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1172） |
| `EntityPlayer.SetBagOfCraftingOutput` | `void SetBagOfCraftingOutput ( CollectibleType Collectible )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1177） |
| `EntityPlayer.SetBagOfCraftingSlot` | `void SetBagOfCraftingSlot ( int SlotID, BagOfCraftingPickup PickupID )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1182） |
| `EntityPlayer.SetBlackHeart` | `void SetBlackHeart ( int BlackHeart )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1189） |
| `EntityPlayer.SetBladderCharge` | `void SetBladderCharge ( int Charge )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1193） |
| `EntityPlayer.SetBlinkLockTime` | `void SetBlinkLockTime ( int Time )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1201） |
| `EntityPlayer.SetBloodCharge` | `void SetBloodCharge ( int Amount )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1809） |
| `EntityPlayer.SetBloodLustCounter` | `void SetBloodLustCounter ( int Counter )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1205） |
| `EntityPlayer.SetBombPlaceDelay` | `void SetBombPlaceDelay ( int Delay )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1209） |
| `EntityPlayer.SetCambionConceptionState` | `void SetCambionConceptionState ( int State )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1213） |
| `EntityPlayer.SetCanShoot` | `boolean SetCanShoot ( boolean CanShoot )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1220） |
| `EntityPlayer.SetCard` | `void SetCard ( int SlotId, Card ID )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1816） |
| `EntityPlayer.SetCharmOfTheVampireKills` | `void SetCharmOfTheVampireKills ( int KillAmount )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1225） |
| `EntityPlayer.SetConceptionFamiliarFlags` | `void SetConceptionFamiliarFlags ( ConceptionFamiliarFlag Flags )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1229） |
| `EntityPlayer.SetControllerIndex` | `void SetControllerIndex ( int Idx, boolean IncludePlayerOwned = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1234） |
| `EntityPlayer.SetD8DamageModifier` | `void SetD8DamageModifier ( float Modifier )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1241） |
| `EntityPlayer.SetD8FireDelayModifier` | `void SetD8FireDelayModifier ( float Modifier )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1245） |
| `EntityPlayer.SetD8RangeModifier` | `void SetD8RangeModifier ( float Modifier )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1249） |
| `EntityPlayer.SetD8SpeedModifier` | `void SetD8SpeedModifier ( float Modifier )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1253） |
| `EntityPlayer.SetDamageModifier` | `void SetDamageModifier ( int Modifier )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1257） |
| `EntityPlayer.SetDonateLuck` | `void SetDonateLuck ( int Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1266） |
| `EntityPlayer.SetEdenDamage` | `void SetEdenDamage ( float Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1273） |
| `EntityPlayer.SetEdenFireDelay` | `void SetEdenFireDelay ( float Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1278） |
| `EntityPlayer.SetEdenLuck` | `void SetEdenLuck ( float Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1283） |
| `EntityPlayer.SetEdenRange` | `void SetEdenRange ( float Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1288） |
| `EntityPlayer.SetEdenShotSpeed` | `void SetEdenShotSpeed ( float Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1293） |
| `EntityPlayer.SetEdenSpeed` | `void SetEdenSpeed ( float Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1298） |
| `EntityPlayer.SetEveSumptoriumCharge` | `void SetEveSumptoriumCharge ( int ChargeNum )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1303） |
| `EntityPlayer.SetFireDelayModifier` | `void SetFireDelayModifier ( int Modifier )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1308） |
| `EntityPlayer.SetFootprintColor` | `void SetFootprintColor ( KColor color, boolean RightFoot = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1317） |
| `EntityPlayer.SetForceCamoEffect` | `void SetForceCamoEffect ( boolean Force )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1322） |
| `EntityPlayer.SetForgottenSwapFormCooldown` | `void SetForgottenSwapFormCooldown ( int Cooldown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1326） |
| `EntityPlayer.SetFullHearts` | `void SetFullHearts ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1822） |
| `EntityPlayer.SetGnawedLeafTimer` | `void SetGnawedLeafTimer ( int Timer )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1330） |
| `EntityPlayer.SetHallowedGroundCountdown` | `void SetHallowedGroundCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1334） |
| `EntityPlayer.SetHeadDirection` | `void SetHeadDirection ( Direction Direction, int Time, boolean Force = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1339） |
| `EntityPlayer.SetHeadDirectionLockTime` | `void SetHeadDirectionLockTime ( int Time )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1344） |
| `EntityPlayer.SetImExcitedSpeedupCountdown` | `void SetImExcitedSpeedupCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1349） |
| `EntityPlayer.SetImmaculateConceptionState` | `void SetImmaculateConceptionState ( int State )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1353） |
| `EntityPlayer.SetInnateCollectibleCount` | `int SetInnateCollectibleCount ( CollectibleType Collectible, int NewCount, string GroupKey = "", boolean AddCostume = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1362） |
| `EntityPlayer.SetInnateCollectibleGroup` | `void SetInnateCollectibleGroup ( string GroupKey, table NewCounts, boolean AddCostume = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1367） |
| `EntityPlayer.SetInnateTrinketCount` | `int SetInnateTrinketCount ( TrinketType Trinket, int NewCount, string GroupKey = "", boolean AddCostume = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1374） |
| `EntityPlayer.SetInnateTrinketGroup` | `void SetInnateTrinketGroup ( string GroupKey, table NewCounts, boolean AddCostume = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1381） |
| `EntityPlayer.SetItemState` | `void SetItemState ( CollectibleType Collectible )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1390） |
| `EntityPlayer.SetItemStateCooldown` | `void SetItemStateCooldown ( int Cooldown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1395） |
| `EntityPlayer.SetKeepersSackBonus` | `void SetKeepersSackBonus ( int Bonus )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1399） |
| `EntityPlayer.SetLaserColor` | `void SetLaserColor ( Color color )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1404） |
| `EntityPlayer.SetLuckModifier` | `void SetLuckModifier ( int Modifier )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1409） |
| `EntityPlayer.SetMaggyHealthDrainCooldown` | `void SetMaggyHealthDrainCooldown ( int Cooldown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1418） |
| `EntityPlayer.SetMaggySwingCooldown` | `void SetMaggySwingCooldown ( int Cooldown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1422） |
| `EntityPlayer.SetMawOfTheVoidCharge` | `void SetMawOfTheVoidCharge ( int Charge )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1427） |
| `EntityPlayer.SetMaxBladderCharge` | `void SetMaxBladderCharge ( int Charge )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1431） |
| `EntityPlayer.SetMegaBlastDuration` | `void SetMegaBlastDuration ( int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1436） |
| `EntityPlayer.SetMinDamageCooldown` | `void SetMinDamageCooldown ( int DamageCooldown )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1827） |
| `EntityPlayer.SetMontezumaRevengeCharge` | `void SetMontezumaRevengeCharge ( int Charge )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1444） |
| `EntityPlayer.SetNextUrethraBlockFrame` | `void SetNextUrethraBlockFrame ( int Frame )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1448） |
| `EntityPlayer.SetPill` | `void SetPill ( int SlotId, PillColor Pill )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1832） |
| `EntityPlayer.SetPlanCKillCountdown` | `void SetPlanCKillCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1453） |
| `EntityPlayer.SetPocketActiveItem` | `void SetPocketActiveItem ( CollectibleType Type, ActiveSlot Slot, boolean KeepInPools )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1839） |
| `EntityPlayer.SetPonyCharge` | `void SetPonyCharge ( int Time )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1457） |
| `EntityPlayer.SetPoopSpell` | `void SetPoopSpell ( int Slot, PoopSpellType PoopSpellType )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1462） |
| `EntityPlayer.SetPotatoPeelerUses` | `void SetPotatoPeelerUses ( int Amount )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1467） |
| `EntityPlayer.SetPurityState` | `void SetPurityState ( PurityState State )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1471） |
| `EntityPlayer.SetRUAWizardTimer` | `void SetRUAWizardTimer ( int timer )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1509） |
| `EntityPlayer.SetRedStewBonusDuration` | `void SetRedStewBonusDuration ( int Duration )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1476） |
| `EntityPlayer.SetRevelationCharge` | `void SetRevelationCharge ( float Charge )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1481） |
| `EntityPlayer.SetRockBottomDamage` | `void SetRockBottomDamage ( float Damage )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1485） |
| `EntityPlayer.SetRockBottomLuck` | `void SetRockBottomLuck ( float Luck )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1489） |
| `EntityPlayer.SetRockBottomMaxFireDelay` | `void SetRockBottomMaxFireDelay ( float MaxFireDelay )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1493） |
| `EntityPlayer.SetRockBottomMoveSpeed` | `void SetRockBottomMoveSpeed ( float MoveSpeed )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1497） |
| `EntityPlayer.SetRockBottomShotSpeed` | `void SetRockBottomShotSpeed ( float ShotSpeed )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1501） |
| `EntityPlayer.SetRockBottomTearRange` | `void SetRockBottomTearRange ( float TearRange )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1505） |
| `EntityPlayer.SetShootingCooldown` | `void SetShootingCooldown ( int Cooldown )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1852） |
| `EntityPlayer.SetShotSpeedModifier` | `void SetShotSpeedModifier ( int Modifier )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1513） |
| `EntityPlayer.SetSoulCharge` | `void SetSoulCharge ( int Amount )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1857） |
| `EntityPlayer.SetSpeedModifier` | `void SetSpeedModifier ( int Modifier )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1522） |
| `EntityPlayer.SetSuplexAimCountdown` | `void SetSuplexAimCountdown ( int Countdown )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1531） |
| `EntityPlayer.SetSuplexLandPosition` | `void SetSuplexLandPosition ( Vector Position )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1535） |
| `EntityPlayer.SetSuplexState` | `void SetSuplexState ( SuplexState State )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1539） |
| `EntityPlayer.SetSuplexTargetPosition` | `void SetSuplexTargetPosition ( Vector Position )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1543） |
| `EntityPlayer.SetTargetTrapDoor` | `void SetTargetTrapDoor ( GridEntity TrapDoor )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1864） |
| `EntityPlayer.SetTearDisplacement` | `void SetTearDisplacement ( int Displacement )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1547） |
| `EntityPlayer.SetTearPoisonDamage` | `void SetTearPoisonDamage ( float Damage )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1558） |
| `EntityPlayer.SetTearRangeModifier` | `void SetTearRangeModifier ( int Modifier )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1562） |
| `EntityPlayer.SetUrethraBlock` | `void SetUrethraBlock ( boolean Blocked )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1571） |
| `EntityPlayer.SetWeapon` | `void SetWeapon ( Weapon Weapon, int WeaponSlot )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1579） |
| `EntityPlayer.ShootBlueCandle` | `EntityEffect ShootBlueCandle ( Vector Direction )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1595） |
| `EntityPlayer.ShootRedCandle` | `EntityEffect ShootRedCandle ( Vector Direction )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行68） |
| `EntityPlayer.ShootRedCandle` | `void ShootRedCandle ( Vector Direction )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1871） |
| `EntityPlayer.ShotSpeed` | `float ShotSpeed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2131） |
| `EntityPlayer.ShuffleCostumes` | `void ShuffleCostumes ( int Seed = Random( ) )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1600） |
| `EntityPlayer.SpawnAquariusCreep` | `EntityEffect SpawnAquariusCreep ( TearParams TearParams = self.TearParams)` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1605） |
| `EntityPlayer.SpawnClot` | `void SpawnClot ( Vector pos, boolean AllowPlayerDeath = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1615） |
| `EntityPlayer.SpawnMawOfVoid` | `EntityLaser SpawnMawOfVoid ( int Timeout )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1876） |
| `EntityPlayer.SpawnSaturnusTears` | `int SpawnSaturnusTears ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1621） |
| `EntityPlayer.StopExtraAnimation` | `void StopExtraAnimation ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1881） |
| `EntityPlayer.SwapActiveItems` | `void SwapActiveItems ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1886） |
| `EntityPlayer.SwapForgottenForm` | `boolean SwapForgottenForm ( boolean Force = false, boolean NoEffects = false)` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1626） |
| `EntityPlayer.SyncConsumableCounts` | `void SyncConsumableCounts ( EntityPlayer Player, int CollectibleFlags )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1635） |
| `EntityPlayer.TearColor` | `Color TearColor` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2141） |
| `EntityPlayer.TearFallingAcceleration` | `float TearFallingAcceleration` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2146） |
| `EntityPlayer.TearFallingSpeed` | `float TearFallingSpeed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2151） |
| `EntityPlayer.TearFlags` | `TearFlags TearFlags` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2156） |
| `EntityPlayer.TearHeight` | `float TearHeight` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2172） |
| `EntityPlayer.TearRange` | `float TearRange` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2189） |
| `EntityPlayer.TearsOffset` | `Vector TearsOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2208） |
| `EntityPlayer.Teleport` | `void Teleport ( Vector Position, boolean DoEffects = true, boolean TeleportTwinPlayers = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1639） |
| `EntityPlayer.ThrowBlueSpider` | `Entity ThrowBlueSpider ( Vector Position, Vector Target )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1892） |
| `EntityPlayer.ThrowFriendlyDip` | `EntityFamiliar ThrowFriendlyDip ( int Subtype, Vector Position, Vector Target )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1897） |
| `EntityPlayer.ThrowHeldEntity` | `Entity ThrowHeldEntity ( Vector Velocity )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1916） |
| `EntityPlayer.TriggerBookOfVirtues` | `void TriggerBookOfVirtues ( CollectibleType Type = CollectibleType.COLLECTIBLE_NULL, int Charge = 0 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1921） |
| `EntityPlayer.TriggerRoomClear` | `void TriggerRoomClear ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1646） |
| `EntityPlayer.TryAddToBagOfCrafting` | `boolean TryAddToBagOfCrafting ( EntityPickup Pickup )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1651） |
| `EntityPlayer.TryDecreaseGlowingHourglassUses` | `void TryDecreaseGlowingHourglassUses ( int Uses, boolean ForceHourglass = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1656） |
| `EntityPlayer.TryFakeDeath` | `boolean TryFakeDeath ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1664） |
| `EntityPlayer.TryForgottenThrow` | `boolean TryForgottenThrow ( Vector Direction )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1669） |
| `EntityPlayer.TryHoldEntity` | `boolean TryHoldEntity ( Entity Entity )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1927） |
| `EntityPlayer.TryHoldTrinket` | `boolean TryHoldTrinket ( TrinketType Type )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1932） |
| `EntityPlayer.TryPreventDeath` | `boolean TryPreventDeath ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1674） |
| `EntityPlayer.TryRemoveCollectibleCostume` | `void TryRemoveCollectibleCostume ( CollectibleType Collectible, boolean KeepPersistent )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1937） |
| `EntityPlayer.TryRemoveNullCostume` | `void TryRemoveNullCostume ( NullItemID NullId )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1949） |
| `EntityPlayer.TryRemoveSmeltedTrinket` | `void TryRemoveSmeltedTrinket ( TrinketType ID )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1681） |
| `EntityPlayer.TryRemoveTrinket` | `boolean TryRemoveTrinket ( TrinketType Type )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1954） |
| `EntityPlayer.TryRemoveTrinketCostume` | `void TryRemoveTrinketCostume ( TrinketType Trinket )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1959） |
| `EntityPlayer.TryUseKey` | `boolean TryUseKey ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1964） |
| `EntityPlayer.UnblockCollectible` | `void UnblockCollectible ( CollectibleType Collectible )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1686） |
| `EntityPlayer.UnblockTrinket` | `void UnblockTrinket ( TrinketType Trinket )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1691） |
| `EntityPlayer.UpdateCanShoot` | `void UpdateCanShoot ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1969） |
| `EntityPlayer.UpdateIsaacPregnancy` | `void UpdateIsaacPregnancy ( boolean UpdateCambion )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1696） |
| `EntityPlayer.UseActiveItem` | `UseActiveItemResultFlags UseActiveItem ( CollectibleType Item, UseFlags UseFlags = 0, ActiveSlot Slot = -1, int CustomVarData = 0 )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行73） |
| `EntityPlayer.UseActiveItem` | `void UseActiveItem ( CollectibleType Item, UseFlags UseFlags = 0, ActiveSlot Slot = -1, int CustomVarData = 0 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1974） |
| `EntityPlayer.UseActiveItem` | `void UseActiveItem ( CollectibleType Item, boolean ShowAnim = false, boolean KeepActiveItem = false, boolean AllowNonMainPlayer = true, boolean ToAddCostume = false, ActiveSlot Slot = -1, int CustomVarData = 0 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1976） |
| `EntityPlayer.UseCard` | `void UseCard ( Card ID, UseFlags UseFlags = 0 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1986） |
| `EntityPlayer.UsePill` | `void UsePill ( PillEffect ID, PillColor PillColor, UseFlags UseFlags = 0  )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1991） |
| `EntityPlayer.UsePoopSpell` | `void UsePoopSpell ( PoopSpellType type )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行1996） |
| `EntityPlayer.VoidHasCollectible` | `boolean VoidHasCollectible ( CollectibleType Collectible )` | explicit_readonly_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityPlayer.md)（行1701） |
| `EntityPlayer.WillPlayerRevive` | `boolean WillPlayerRevive ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityPlayer.md)（行2002） |
| `EntityProjectile.Acceleration` | `float Acceleration` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行65） |
| `EntityProjectile.AddChangeFlags` | `void AddChangeFlags ( ProjectileFlags Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行21） |
| `EntityProjectile.AddFallingAccel` | `void AddFallingAccel ( float Value )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行27） |
| `EntityProjectile.AddFallingSpeed` | `void AddFallingSpeed ( float Value )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行32） |
| `EntityProjectile.AddHeight` | `void AddHeight ( float Value )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行37） |
| `EntityProjectile.AddProjectileFlags` | `void AddProjectileFlags ( ProjectileFlags Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行42） |
| `EntityProjectile.AddScale` | `void AddScale ( float Value )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行49） |
| `EntityProjectile.ChangeFlags` | `ProjectileFlags ChangeFlags` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行70） |
| `EntityProjectile.ChangeTimeout` | `int ChangeTimeout` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行86） |
| `EntityProjectile.ChangeVelocity` | `float ChangeVelocity` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行102） |
| `EntityProjectile.ClearProjectileFlags` | `void ClearProjectileFlags ( ProjectileFlags Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行54） |
| `EntityProjectile.CurvingStrength` | `float CurvingStrength` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行118） |
| `EntityProjectile.Damage` | `float Damage` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行123） |
| `EntityProjectile.Deflect` | `void Deflect ( Vector NewVelocity )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityProjectile.md)（行12） |
| `EntityProjectile.DepthOffset` | `float DepthOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行128） |
| `EntityProjectile.FallingAccel` | `float FallingAccel` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行133） |
| `EntityProjectile.FallingSpeed` | `float FallingSpeed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行138） |
| `EntityProjectile.HasProjectileFlags` | `boolean HasProjectileFlags ( ProjectileFlags Flags )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行59） |
| `EntityProjectile.Height` | `float Height` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行143） |
| `EntityProjectile.HomingStrength` | `float HomingStrength` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行150） |
| `EntityProjectile.ProjectileFlags` | `ProjectileFlags ProjectileFlags` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行155） |
| `EntityProjectile.Scale` | `float Scale` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行161） |
| `EntityProjectile.WiggleFrameOffset` | `int WiggleFrameOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityProjectile.md)（行166） |
| `EntitySlot.CreateDropsFromExplosion` | `void CreateDropsFromExplosion ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行14） |
| `EntitySlot.GetDonationValue` | `int GetDonationValue ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行19） |
| `EntitySlot.GetPrizeCollectible` | `CollectibleType GetPrizeCollectible ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行28） |
| `EntitySlot.GetPrizeSprite` | `Sprite GetPrizeSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行33） |
| `EntitySlot.GetPrizeType` | `int GetPrizeType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行38） |
| `EntitySlot.GetShellGameAnimationIndex` | `int GetShellGameAnimationIndex ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行58） |
| `EntitySlot.GetState` | `SlotState GetState ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行63） |
| `EntitySlot.GetTimeout` | `int GetTimeout ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行68） |
| `EntitySlot.GetTouch` | `int GetTouch ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行77） |
| `EntitySlot.GetTriggerTimerNum` | `int GetTriggerTimerNum ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行82） |
| `EntitySlot.RandomCoinJamAnim` | `string RandomCoinJamAnim ( )` | actual_event_only rng_consuming_call | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行91） |
| `EntitySlot.SetDonationValue` | `void SetDonationValue ( int DonationValue )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行96） |
| `EntitySlot.SetPrizeCollectible` | `void SetPrizeCollectible ( CollectibleType Collectible )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行101） |
| `EntitySlot.SetPrizeType` | `void SetPrizeType ( int PrizeType )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行106） |
| `EntitySlot.SetShellGameAnimationIndex` | `void SetShellGameAnimationIndex ( int index )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行111） |
| `EntitySlot.SetState` | `void SetState ( SlotState State )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行116） |
| `EntitySlot.SetTimeout` | `void SetTimeout ( int Timeout )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行121） |
| `EntitySlot.SetTouch` | `void SetTouch ( int Touch )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行126） |
| `EntitySlot.SetTriggerTimerNum` | `void SetTriggerTimerNum ( int num )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntitySlot.md)（行131） |
| `EntityTear.AddTearFlags` | `void AddTearFlags ( TearFlags Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行22） |
| `EntityTear.BaseDamage` | `const float BaseDamage` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行68） |
| `EntityTear.BaseScale` | `const float BaseScale` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行73） |
| `EntityTear.Bounced` | `boolean Bounced` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行78） |
| `EntityTear.CanTriggerStreakEnd` | `boolean CanTriggerStreakEnd` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行85） |
| `EntityTear.ChangeVariant` | `void ChangeVariant ( TearVariant NewVariant )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行27） |
| `EntityTear.ClearHitList` | `void ClearHitList ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行19） |
| `EntityTear.ClearTearFlags` | `void ClearTearFlags ( TearFlags Flags )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行32） |
| `EntityTear.ContinueVelocity` | `Vector ContinueVelocity` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行90） |
| `EntityTear.FallingAcceleration` | `float FallingAcceleration` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行95） |
| `EntityTear.FallingSpeed` | `float FallingSpeed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行100） |
| `EntityTear.FireSplitTear` | `EntityTear FireSplitTear ( Vector Position, Vector Velocity, float DamageMultiplier = 0.5, float SizeMultiplier = 0.6, int Variant = 0, SplitTearType splitType = SplitTearType.SPLIT_GENERIC )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行24） |
| `EntityTear.GetDeadEyeIntensity` | `float GetDeadEyeIntensity ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行31） |
| `EntityTear.GetDeadEyeSprite` | `Sprite GetDeadEyeSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行36） |
| `EntityTear.GetHitList` | `int[] GetHitList ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行41） |
| `EntityTear.GetTearEffectSprite` | `Sprite GetTearEffectSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行46） |
| `EntityTear.GetTearHaloSprite` | `Sprite GetTearHaloSprite ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行51） |
| `EntityTear.HasTearFlags` | `boolean HasTearFlags ( TearFlags Flags )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行37） |
| `EntityTear.Height` | `float Height` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行105） |
| `EntityTear.HomingFriction` | `float HomingFriction` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行110） |
| `EntityTear.IsMultidimensionalTouched` | `boolean IsMultidimensionalTouched ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行56） |
| `EntityTear.IsPrismTouched` | `boolean IsPrismTouched ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行61） |
| `EntityTear.KnockbackMultiplier` | `float KnockbackMultiplier` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行115） |
| `EntityTear.MakeMultidimensionalCopy` | `EntityTear MakeMultidimensionalCopy ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行66） |
| `EntityTear.ParentOffset` | `Vector ParentOffset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行120） |
| `EntityTear.PosDisplacement` | `const Vector PosDisplacement` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行125） |
| `EntityTear.ResetSpriteScale` | `void ResetSpriteScale ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行42） |
| `EntityTear.ResetSpriteScale` | `void ResetSpriteScale ( boolean Force = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行12） |
| `EntityTear.Rotation` | `float Rotation` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行130） |
| `EntityTear.Scale` | `float Scale` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行135） |
| `EntityTear.SetDeadEyeIntensity` | `void SetDeadEyeIntensity ( float Intensity )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行47） |
| `EntityTear.SetInitSound` | `void SetInitSound ( SoundEffect SoundID )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行81） |
| `EntityTear.SetKnockbackMultiplier` | `void SetKnockbackMultiplier ( float Multiplier )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行52） |
| `EntityTear.SetMultidimensionalTouched` | `void SetMultidimensionalTouched ( boolean IsTouched )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行71） |
| `EntityTear.SetParentOffset` | `void SetParentOffset ( Vector Offset )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行57） |
| `EntityTear.SetPrismTouched` | `void SetPrismTouched ( boolean IsTouched )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/EntityTear.md)（行76） |
| `EntityTear.SetWaitFrames` | `void SetWaitFrames ( int Value )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行62） |
| `EntityTear.StickDiff` | `Vector StickDiff` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行140） |
| `EntityTear.StickTarget` | `Entity StickTarget` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行145） |
| `EntityTear.StickTimer` | `int StickTimer` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行150） |
| `EntityTear.TearFlags` | `TearFlags TearFlags` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行155） |
| `EntityTear.TearIndex` | `const int TearIndex` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行160） |
| `EntityTear.WaitFrames` | `int WaitFrames` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/EntityTear.md)（行167） |
| `History.GetCollectiblesHistory` | `HistoryItems[] GetCollectiblesHistory ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/History.md)（行20） |
| `History.RemoveHistoryItemByIndex` | `boolean RemoveHistoryItemByIndex ( int Index )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/History.md)（行25） |
| `History.SearchCollectibles` | `HistoryItems[] SearchCollectibles ( CollectibleType[] IDs = nil )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/History.md)（行32） |
| `History.SearchTrinkets` | `HistoryItems[] SearchTrinkets ( TrinketType[] = nil )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/History.md)（行39） |
| `HistoryItem.GetItemID` | `int GetItemID ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/HistoryItem.md)（行20） |
| `HistoryItem.GetItemPoolType` | `ItemPoolType GetItemPoolType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/HistoryItem.md)（行24） |
| `HistoryItem.GetLevelStage` | `LevelStage GetLevelStage ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/HistoryItem.md)（行28） |
| `HistoryItem.GetRoomType` | `RoomType GetRoomType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/HistoryItem.md)（行32） |
| `HistoryItem.GetStageType` | `StageType GetStageType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/HistoryItem.md)（行36） |
| `HistoryItem.GetTime` | `int GetTime ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/HistoryItem.md)（行40） |
| `HistoryItem.IsTrinket` | `boolean IsTrinket ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/HistoryItem.md)（行44） |
| `Image.GetHeight` | `number GetHeight ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Image.md)（行24） |
| `Image.GetName` | `string GetName ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Image.md)（行48） |
| `Image.GetPaddedHeight` | `number GetPaddedHeight ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Image.md)（行40） |
| `Image.GetPaddedWidth` | `number GetPaddedWidth ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Image.md)（行32） |
| `Image.GetTexelRegion` | `string GetTexelRegion ( integer x, integer y, integer width, integer height )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Image.md)（行52） |
| `Image.GetWidth` | `number GetWidth ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Image.md)（行16） |
| `Image.Render` | `void Render ( SourceQuad sourceQuad, DestinationQuad destinationQuad, KColor color, Color colorMod = Color() )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Image.md)（行61） |
| `Image.RenderWithShader` | `void RenderWithShader ( SourceQuad sourceQuad, DestinationQuad destinationQuad, KColor color, Shader shader, table shaderParams )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Image.md)（行65） |
| `ItemConfigCard.AchievementID` | `int AchievementID` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行35） |
| `ItemConfigCard.AnnouncerDelay` | `int AnnouncerDelay` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行41） |
| `ItemConfigCard.AnnouncerVoice` | `int AnnouncerVoice` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行46） |
| `ItemConfigCard.CardType` | `int CardType` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行51） |
| `ItemConfigCard.ClearAvailabilityCondition` | `void ClearAvailabilityCondition ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Card.md)（行47） |
| `ItemConfigCard.Description` | `string Description` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行56） |
| `ItemConfigCard.GetAvailabilityCondition` | `function GetAvailabilityCondition ( )` | owner_qualified_reference mod_function:tx-04-14 | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Card.md)（行51） |
| `ItemConfigCard.GreedModeAllowed` | `boolean GreedModeAllowed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行65） |
| `ItemConfigCard.Hidden` | `const boolean Hidden` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Card.md)（行57） |
| `ItemConfigCard.HudAnim` | `string HudAnim` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行71） |
| `ItemConfigCard.ID` | `int ID` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行80） |
| `ItemConfigCard.InitialWeight` | `const float InitialWeight` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Card.md)（行60） |
| `ItemConfigCard.IsAvailable` | `boolean IsAvailable ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行19） |
| `ItemConfigCard.IsCard` | `boolean IsCard ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行24） |
| `ItemConfigCard.IsRune` | `boolean IsRune ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行29） |
| `ItemConfigCard.MimicCharge` | `int MimicCharge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行86） |
| `ItemConfigCard.ModdedCardFront` | `Sprite ModdedCardFront` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Card.md)（行63） |
| `ItemConfigCard.Name` | `string Name` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行91） |
| `ItemConfigCard.PickupSubtype` | `int PickupSubtype` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Card.md)（行100） |
| `ItemConfigCard.SetAvailabilityCondition` | `void SetAvailabilityCondition ( function AvailabilityCondition )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Card.md)（行10） |
| `ItemConfigCard.Weight` | `float Weight` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Card.md)（行66） |
| `ItemConfigCostume.Anm2Path` | `string Anm2Path` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Costume.md)（行18） |
| `ItemConfigCostume.HasOverlay` | `boolean HasOverlay` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Costume.md)（行23） |
| `ItemConfigCostume.HasSkinAlt` | `boolean HasSkinAlt` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Costume.md)（行28） |
| `ItemConfigCostume.ID` | `int ID` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Costume.md)（行33） |
| `ItemConfigCostume.IsFlying` | `boolean IsFlying` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Costume.md)（行38） |
| `ItemConfigCostume.OverwriteColor` | `boolean OverwriteColor` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Costume.md)（行43） |
| `ItemConfigCostume.Priority` | `int Priority` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Costume.md)（行48） |
| `ItemConfigCostume.SkinColor` | `SkinColor SkinColor` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Costume.md)（行53） |
| `ItemConfigCostume.SkinColor` | `int SkinColor` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Costume.md)（行10） |
| `ItemConfigItem.AchievementID` | `int AchievementID` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行61） |
| `ItemConfigItem.AddBlackHearts` | `int AddBlackHearts` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行67） |
| `ItemConfigItem.AddBombs` | `int AddBombs` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行73） |
| `ItemConfigItem.AddCoins` | `int AddCoins` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行79） |
| `ItemConfigItem.AddCostumeOnPickup` | `boolean AddCostumeOnPickup` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行85） |
| `ItemConfigItem.AddCustomCacheTag` | `void AddCustomCacheTag ( string Tag )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Item.md)（行10） |
| `ItemConfigItem.AddCustomTag` | `void AddCustomTag ( string Tag )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Item.md)（行15） |
| `ItemConfigItem.AddHearts` | `int AddHearts` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行91） |
| `ItemConfigItem.AddKeys` | `int AddKeys` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行97） |
| `ItemConfigItem.AddMaxHearts` | `int AddMaxHearts` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行103） |
| `ItemConfigItem.AddSoulHearts` | `int AddSoulHearts` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行109） |
| `ItemConfigItem.CacheFlags` | `int CacheFlags` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行115） |
| `ItemConfigItem.ChargeType` | `int ChargeType` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行121） |
| `ItemConfigItem.ClearEffectsOnRemove` | `boolean ClearEffectsOnRemove` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行134） |
| `ItemConfigItem.Costume` | `const Costume Costume` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行140） |
| `ItemConfigItem.CraftingQuality` | `int CraftingQuality` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行146） |
| `ItemConfigItem.Description` | `string Description` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行152） |
| `ItemConfigItem.DevilPrice` | `int DevilPrice` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行161） |
| `ItemConfigItem.Discharged` | `boolean Discharged` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行174） |
| `ItemConfigItem.GetCustomCacheTags` | `table GetCustomCacheTags ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Item.md)（行20） |
| `ItemConfigItem.GetCustomTags` | `table GetCustomTags ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Item.md)（行25） |
| `ItemConfigItem.GfxFileName` | `string GfxFileName` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行179） |
| `ItemConfigItem.HasCustomCacheTag` | `boolean HasCustomCacheTag ( string tag )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Item.md)（行30） |
| `ItemConfigItem.HasCustomTag` | `boolean HasCustomTag ( string tag )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Item.md)（行35） |
| `ItemConfigItem.HasTags` | `boolean HasTags ( int Tags )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行23） |
| `ItemConfigItem.Hidden` | `boolean Hidden` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行185） |
| `ItemConfigItem.ID` | `int ID` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行191） |
| `ItemConfigItem.InitCharge` | `int InitCharge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行197） |
| `ItemConfigItem.IsAvailable` | `boolean IsAvailable ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行35） |
| `ItemConfigItem.IsCollectible` | `boolean IsCollectible ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行42） |
| `ItemConfigItem.IsNull` | `boolean IsNull ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行48） |
| `ItemConfigItem.IsTrinket` | `boolean IsTrinket ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行54） |
| `ItemConfigItem.MaxCharges` | `int MaxCharges` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行204） |
| `ItemConfigItem.MaxCooldown` | `int MaxCooldown` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行210） |
| `ItemConfigItem.Name` | `string Name` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行216） |
| `ItemConfigItem.PassiveCache` | `boolean PassiveCache` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行225） |
| `ItemConfigItem.PersistentEffect` | `boolean PersistentEffect` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行231） |
| `ItemConfigItem.Quality` | `int Quality` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行237） |
| `ItemConfigItem.RemoveCustomCacheTag` | `void RemoveCustomCacheTag ( string Tag )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Item.md)（行40） |
| `ItemConfigItem.RemoveCustomTag` | `void RemoveCustomTag ( string Tag )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_Item.md)（行45） |
| `ItemConfigItem.ShopPrice` | `int ShopPrice` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行243） |
| `ItemConfigItem.Special` | `boolean Special` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行249） |
| `ItemConfigItem.Tags` | `int Tags` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行255） |
| `ItemConfigItem.Type` | `ItemType Type` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_Item.md)（行261） |
| `ItemConfigPillEffect.AchievementID` | `int AchievementID` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_PillEffect.md)（行25） |
| `ItemConfigPillEffect.AnnouncerDelay` | `int AnnouncerDelay` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_PillEffect.md)（行31） |
| `ItemConfigPillEffect.AnnouncerVoice` | `int AnnouncerVoice` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_PillEffect.md)（行36） |
| `ItemConfigPillEffect.AnnouncerVoiceSuper` | `int AnnouncerVoiceSuper` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_PillEffect.md)（行41） |
| `ItemConfigPillEffect.EffectClass` | `int EffectClass` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_PillEffect.md)（行46）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_PillEffect.md)（行10） |
| `ItemConfigPillEffect.EffectSubClass` | `int EffectSubClass` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_PillEffect.md)（行53）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/ItemConfig_PillEffect.md)（行15） |
| `ItemConfigPillEffect.GreedModeAllowed` | `boolean GreedModeAllowed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_PillEffect.md)（行60） |
| `ItemConfigPillEffect.ID` | `int ID` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_PillEffect.md)（行66） |
| `ItemConfigPillEffect.IsAvailable` | `boolean IsAvailable ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_PillEffect.md)（行19） |
| `ItemConfigPillEffect.MimicCharge` | `int MimicCharge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_PillEffect.md)（行72） |
| `ItemConfigPillEffect.Name` | `string Name` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/ItemConfig_PillEffect.md)（行77） |
| `KColor.Alpha` | `float Alpha` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/KColor.md)（行79） |
| `KColor.Blue` | `float Blue` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/KColor.md)（行84） |
| `KColor.Green` | `float Green` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/KColor.md)（行89） |
| `KColor.KColor` | `KColor KColor ( float red, float green, float blue, float alpha )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/KColor.md)（行21） |
| `KColor.Red` | `float Red` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/KColor.md)（行94） |
| `LayerState.ClearCustomChampionShader` | `void ClearCustomChampionShader ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行10） |
| `LayerState.ClearCustomShader` | `void ClearCustomShader ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行15） |
| `LayerState.GetBlendMode` | `BlendMode GetBlendMode ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行20） |
| `LayerState.GetColor` | `Color GetColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行24） |
| `LayerState.GetCropOffset` | `Vector GetCropOffset ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行28） |
| `LayerState.GetDefaultSpritesheetPath` | `string GetDefaultSpritesheetPath ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行32） |
| `LayerState.GetFlipX` | `boolean GetFlipX ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行36） |
| `LayerState.GetFlipY` | `boolean GetFlipY ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行40） |
| `LayerState.GetLayerID` | `int GetLayerID ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行44） |
| `LayerState.GetName` | `string GetName ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行48） |
| `LayerState.GetPos` | `Vector GetPos ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行52） |
| `LayerState.GetRenderFlags` | `AnimRenderFlags GetRenderFlags ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行56） |
| `LayerState.GetRotation` | `float GetRotation ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行60） |
| `LayerState.GetSize` | `Vector GetSize ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行64） |
| `LayerState.GetSpritesheet` | `Image? GetSpritesheet ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行172） |
| `LayerState.GetSpritesheetPath` | `string GetSpritesheetPath ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行68） |
| `LayerState.GetWrapSMode` | `int GetWrapSMode ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行72） |
| `LayerState.GetWrapTMode` | `int GetWrapTMode ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行80） |
| `LayerState.HasCustomChampionShader` | `boolean HasCustomChampionShader ( string ShaderPath )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行88） |
| `LayerState.HasCustomShader` | `boolean HasCustomShader ( string ShaderPath )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行93） |
| `LayerState.IsVisible` | `boolean IsVisible ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行98） |
| `LayerState.SetColor` | `void SetColor ( Color Color )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行102） |
| `LayerState.SetCropOffset` | `void SetCropOffset ( Vector Offset )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行106） |
| `LayerState.SetCustomChampionShader` | `void SetCustomChampionShader ( string ShaderPath )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行110） |
| `LayerState.SetCustomShader` | `void SetCustomShader ( string ShaderPath )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行119） |
| `LayerState.SetFlipX` | `void SetFlipX ( boolean Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行128） |
| `LayerState.SetFlipY` | `void SetFlipY ( boolean Value )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行132） |
| `LayerState.SetPos` | `void SetPos ( Vector Pos )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行136） |
| `LayerState.SetRenderFlags` | `void SetRenderFlags ( AnimRenderFlags Flags )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行140） |
| `LayerState.SetRotation` | `void SetRotation ( float Rotation )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行144） |
| `LayerState.SetSize` | `void SetSize ( Vector Size )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行148） |
| `LayerState.SetSpritesheet` | `void SetSpritesheet ( Image Spritesheet )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行176） |
| `LayerState.SetVisible` | `void SetVisible ( boolean Visible )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行152） |
| `LayerState.SetWrapSMode` | `void SetWrapSMode ( int Mode )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行156） |
| `LayerState.SetWrapTMode` | `void SetWrapTMode ( int Mode )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LayerState.md)（行164） |
| `LootList.GetEntries` | `LootListEntry[] GetEntries ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LootList.md)（行14） |
| `LootList.LootList` | `LootList LootList ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LootList.md)（行9） |
| `LootList.PushEntry` | `void PushEntry ( EntityType Type, int Variant, int SubType, int Seed = Random(), RNG RNG = nil )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LootList.md)（行19） |
| `LootListEntry.GetRNG` | `RNG GetRNG ( )` | owner_qualified_reference random:tx-04-14 | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LootListEntry.md)（行31） |
| `LootListEntry.GetSeed` | `int GetSeed ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LootListEntry.md)（行37） |
| `LootListEntry.GetSubType` | `int GetSubType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LootListEntry.md)（行41） |
| `LootListEntry.GetType` | `EntityType GetType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LootListEntry.md)（行45） |
| `LootListEntry.GetVariant` | `int GetVariant ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/LootListEntry.md)（行49） |
| `NativeText.alpha` | `float alpha` | native_copy J273 completed creation owner; see native supplements | [native-J273](F:/projects_isaac/issac_multiplay/docs/design/observation-expansion/entity-native-supplements.md)（行1） |
| `NativeText.blue` | `float blue` | native_copy J273 completed creation owner; see native supplements | [native-J273](F:/projects_isaac/issac_multiplay/docs/design/observation-expansion/entity-native-supplements.md)（行1） |
| `NativeText.color_word` | `int color_word` | native_copy J273 completed creation owner; see native supplements | [native-J273](F:/projects_isaac/issac_multiplay/docs/design/observation-expansion/entity-native-supplements.md)（行1） |
| `NativeText.counter` | `int counter` | native_copy J273 completed creation owner; see native supplements | [native-J273](F:/projects_isaac/issac_multiplay/docs/design/observation-expansion/entity-native-supplements.md)（行1） |
| `NativeText.duration` | `int duration` | native_copy J273 completed creation owner; see native supplements | [native-J273](F:/projects_isaac/issac_multiplay/docs/design/observation-expansion/entity-native-supplements.md)（行1） |
| `NativeText.green` | `float green` | native_copy J273 completed creation owner; see native supplements | [native-J273](F:/projects_isaac/issac_multiplay/docs/design/observation-expansion/entity-native-supplements.md)（行1） |
| `NativeText.red` | `float red` | native_copy J273 completed creation owner; see native supplements | [native-J273](F:/projects_isaac/issac_multiplay/docs/design/observation-expansion/entity-native-supplements.md)（行1） |
| `NativeText.text` | `string text` | native_copy J273 completed creation owner; see native supplements | [native-J273](F:/projects_isaac/issac_multiplay/docs/design/observation-expansion/entity-native-supplements.md)（行1） |
| `NullFrame.GetColor` | `Color GetColor ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/NullFrame.md)（行10） |
| `NullFrame.GetPos` | `Vector GetPos ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/NullFrame.md)（行14） |
| `NullFrame.GetRotation` | `float GetRotation ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/NullFrame.md)（行18） |
| `NullFrame.GetScale` | `Vector GetScale ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/NullFrame.md)（行22） |
| `NullFrame.IsVisible` | `boolean IsVisible ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/NullFrame.md)（行26） |
| `PathFinder.EvadeTarget` | `void EvadeTarget ( Vector TargetPos )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行15） |
| `PathFinder.FindGridPath` | `void FindGridPath ( Vector Pos, float Speed, int PathMarker, boolean UseDirectPath )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行22）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/PathFinder.md)（行25） |
| `PathFinder.GetEvadeMovementCountdown` | `int GetEvadeMovementCountdown ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行45） |
| `PathFinder.GetGridIndex` | `int GetGridIndex ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行50） |
| `PathFinder.HasDirectPath` | `boolean HasDirectPath ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行56） |
| `PathFinder.HasPathToPos` | `boolean HasPathToPos ( Vector Pos, boolean IgnorePoop )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行63） |
| `PathFinder.MoveRandomly` | `boolean MoveRandomly ( boolean IgnoreStatusEffects )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行68）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/PathFinder.md)（行31） |
| `PathFinder.MoveRandomlyAxisAligned` | `void MoveRandomlyAxisAligned ( float Speed, boolean IgnoreStatusEffects )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行75）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/PathFinder.md)（行37） |
| `PathFinder.MoveRandomlyBoss` | `void MoveRandomlyBoss ( boolean IgnoreStatusEffects )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行83）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/PathFinder.md)（行43） |
| `PathFinder.Reset` | `void Reset ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行90） |
| `PathFinder.ResetMovementTarget` | `void ResetMovementTarget ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行95） |
| `PathFinder.SetCanCrushRocks` | `void SetCanCrushRocks ( boolean value )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行100） |
| `PathFinder.UpdateGridIndex` | `void UpdateGridIndex ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PathFinder.md)（行108） |
| `PocketItem.GetSlot` | `int GetSlot ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/PocketItem.md)（行19） |
| `PocketItem.GetType` | `PocketItemType GetType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/PocketItem.md)（行41） |
| `PosVel.Position` | `Vector Position` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PlayerTypes_PosVel.md)（行17） |
| `PosVel.Velocity` | `Vector Velocity` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/PlayerTypes_PosVel.md)（行22） |
| `QueueItemData.Charge` | `int Charge` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/QueueItemData.md)（行23） |
| `QueueItemData.Item` | `ItemConfig Item Item` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/QueueItemData.md)（行28） |
| `QueueItemData.Touched` | `boolean Touched` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/QueueItemData.md)（行33） |
| `Shape.Capsule` | `void Capsule (Capsule Capsule )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Shape.md)（行28） |
| `Shape.Circle` | `void Circle ( Vector Position, float Size )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Shape.md)（行16） |
| `Shape.GetTimeout` | `int GetTimeout ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Shape.md)（行20） |
| `Shape.SetTimeout` | `void SetTimeout ( int Timeout )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/renderer/Shape.md)（行24） |
| `Sprite.ClearCustomChampionShader` | `void ClearCustomChampionShader ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行39） |
| `Sprite.ClearCustomShader` | `void ClearCustomShader ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行44） |
| `Sprite.Color` | `Color Color` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行426） |
| `Sprite.Continue` | `void Continue ( boolean ContinueOverlay = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行49） |
| `Sprite.ContinueOverlay` | `void ContinueOverlay ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行54） |
| `Sprite.Copy` | `Sprite Copy ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行35） |
| `Sprite.FlipX` | `boolean FlipX` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行431） |
| `Sprite.FlipY` | `boolean FlipY` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行436） |
| `Sprite.GetAllAnimationData` | `AnimationData[] GetAllAnimationData ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行59） |
| `Sprite.GetAllLayers` | `LayerStates[] GetAllLayers ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行64） |
| `Sprite.GetAnimation` | `string GetAnimation ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行30） |
| `Sprite.GetAnimationData` | `AnimationData GetAnimationData ( string AnimationName )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行69） |
| `Sprite.GetCurrentAnimationData` | `AnimationData GetCurrentAnimationData ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行73） |
| `Sprite.GetDefaultAnimation` | `string GetDefaultAnimation ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行36） |
| `Sprite.GetDefaultAnimationName` | `string GetDefaultAnimationName ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行53） |
| `Sprite.GetFilename` | `string GetFilename ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行70） |
| `Sprite.GetFrame` | `int GetFrame ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行85） |
| `Sprite.GetLayer` | `LayerState GetLayer ( int LayerId )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行77） |
| `Sprite.GetLayer` | `LayerState GetLayer ( string LayerName )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行78） |
| `Sprite.GetLayerCount` | `int GetLayerCount ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行91） |
| `Sprite.GetLayerFrameData` | `AnimationFrame GetLayerFrameData ( int Layer )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行83） |
| `Sprite.GetNullFrame` | `NullFrame GetNullFrame ( string LayerName )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行88） |
| `Sprite.GetOverlayAnimation` | `string GetOverlayAnimation ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行97） |
| `Sprite.GetOverlayAnimationData` | `AnimationData GetOverlayAnimationData ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行93） |
| `Sprite.GetOverlayFrame` | `int GetOverlayFrame ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行103） |
| `Sprite.GetOverlayLayerFrameData` | `AnimationFrame GetOverlayLayerFrameData ( int Layer )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行98） |
| `Sprite.GetOverlayNullFrame` | `NullFrame GetOverlayNullFrame ( string LayerName )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行103） |
| `Sprite.GetRenderFlags` | `AnimRenderFlags GetRenderFlags ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行108） |
| `Sprite.GetSpritesheet` | `Image? GetSpritesheet ( int Layer )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行165） |
| `Sprite.GetTexel` | `KColor GetTexel ( Vector SamplePos, Vector RenderPos, float AlphaThreshold, int LayerID = 0 )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行109） |
| `Sprite.HasCustomChampionShader` | `boolean HasCustomChampionShader ( string ShaderPath )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行112） |
| `Sprite.HasCustomShader` | `boolean HasCustomShader ( string ShaderPath )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行117） |
| `Sprite.IsEventTriggered` | `boolean IsEventTriggered ( string EventName )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行115） |
| `Sprite.IsFinished` | `boolean IsFinished ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行123） |
| `Sprite.IsFinished` | `boolean IsFinished ( string AnimationName )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行121） |
| `Sprite.IsLoaded` | `boolean IsLoaded ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行128） |
| `Sprite.IsOverlayEventTriggered` | `boolean IsOverlayEventTriggered ( string EventName )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行122） |
| `Sprite.IsOverlayFinished` | `boolean IsOverlayFinished ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行135） |
| `Sprite.IsOverlayFinished` | `boolean IsOverlayFinished ( string AnimationName )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行133） |
| `Sprite.IsOverlayPlaying` | `boolean IsOverlayPlaying ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行142） |
| `Sprite.IsOverlayPlaying` | `boolean IsOverlayPlaying ( string AnimationName )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行140） |
| `Sprite.IsPlaying` | `boolean IsPlaying ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行149） |
| `Sprite.IsPlaying` | `boolean IsPlaying ( string AnimationName )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行147） |
| `Sprite.Load` | `void Load ( string ANM2Path, boolean LoadGraphics )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行172） |
| `Sprite.LoadGraphics` | `void LoadGraphics ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行190） |
| `Sprite.Offset` | `Vector Offset` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行441） |
| `Sprite.Play` | `void Play ( string AnimationName, boolean Force )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行206） |
| `Sprite.PlayOverlay` | `void PlayOverlay ( string AnimationName, boolean Force )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行236） |
| `Sprite.PlayRandom` | `void PlayRandom ( int Seed )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行265） |
| `Sprite.PlaybackSpeed` | `float PlaybackSpeed` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行446） |
| `Sprite.Reload` | `void Reload ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行271） |
| `Sprite.RemoveOverlay` | `void RemoveOverlay ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行276） |
| `Sprite.Render` | `void Render ( Vector Position, Vector Vector TopLeftClamp = Vector.Zero, Vector BottomRightClamp = Vector.Zero )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行281） |
| `Sprite.RenderLayer` | `void RenderLayer ( int LayerId, Vector Position, Vector TopLeftClamp = Vector.Zero, Vector BottomRightClamp = Vector.Zero )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行305） |
| `Sprite.ReplaceSpritesheet` | `boolean ReplaceSpritesheet ( int LayerId, string PngFilename )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行330） |
| `Sprite.ReplaceSpritesheet` | `void ReplaceSpritesheet ( int LayerId, string PngFilename )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行328） |
| `Sprite.ReplaceSpritesheet` | `void ReplaceSpritesheet ( int LayerId, string PngFilename, boolean LoadGraphics = false )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行17） |
| `Sprite.Reset` | `void Reset ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行350） |
| `Sprite.Rotation` | `float Rotation` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行451） |
| `Sprite.Scale` | `Vector Scale` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行456） |
| `Sprite.SetAnimation` | `boolean SetAnimation ( string AnimationName, boolean Reset = true )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行355） |
| `Sprite.SetCustomChampionShader` | `void SetCustomChampionShader ( string ShaderPath )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行127） |
| `Sprite.SetCustomShader` | `void SetCustomShader ( string ShaderPath )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行138） |
| `Sprite.SetFrame` | `void SetFrame ( int FrameNum )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行364） |
| `Sprite.SetFrame` | `void SetFrame ( string AnimationName, int FrameNum )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行365） |
| `Sprite.SetLastFrame` | `void SetLastFrame ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行377） |
| `Sprite.SetLayerFrame` | `void SetLayerFrame ( int LayerId, int FrameNum )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行383） |
| `Sprite.SetOverlayAnimation` | `boolean SetOverlayAnimation ( string AnimationName, bool Reset = true )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行388） |
| `Sprite.SetOverlayFrame` | `void SetOverlayFrame ( int FrameNum )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行22） |
| `Sprite.SetOverlayFrame` | `void SetOverlayFrame ( string AnimationName, int FrameNum )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行397） |
| `Sprite.SetOverlayLayerFrame` | `void SetOverlayLayerFrame ( int Layer, int Frame )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行149） |
| `Sprite.SetOverlayRenderPriority` | `void SetOverlayRenderPriority ( boolean RenderFirst )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行402） |
| `Sprite.SetRenderFlags` | `void SetRenderFlags ( AnimRenderFlags Flags )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行153） |
| `Sprite.SetSpritesheet` | `void SetSpritesheet ( int Layer, Image Spritesheet )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行169） |
| `Sprite.Sprite` | `Sprite Sprite ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行24） |
| `Sprite.Sprite` | `Sprite,boolean Sprite ( string ANM2Path, boolean LoadGraphics = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行10） |
| `Sprite.Stop` | `void Stop ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行407） |
| `Sprite.Stop` | `void Stop ( boolean StopOverlay = true )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行27） |
| `Sprite.StopOverlay` | `void StopOverlay ( )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行157） |
| `Sprite.Update` | `void Update ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行412） |
| `Sprite.WasEventTriggered` | `boolean WasEventTriggered ( string EventName )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Sprite.md)（行419） |
| `Sprite.WasOverlayEventTriggered` | `boolean WasOverlayEventTriggered ( string EventName )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Sprite.md)（行161） |
| `TearParams.BombVariant` | `int BombVariant` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TearParams.md)（行23） |
| `TearParams.KnockbackMultiplier` | `float KnockbackMultiplier` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/TearParams.md)（行9） |
| `TearParams.MassMultiplier` | `float MassMultiplier` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/TearParams.md)（行13） |
| `TearParams.SpeedMultiplier` | `float SpeedMultiplier` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/TearParams.md)（行17） |
| `TearParams.TearColor` | `Color TearColor` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TearParams.md)（行28） |
| `TearParams.TearDamage` | `float TearDamage` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TearParams.md)（行33） |
| `TearParams.TearDisplacement` | `int TearDisplacement` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/TearParams.md)（行21） |
| `TearParams.TearFlags` | `TearFlags TearFlags` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TearParams.md)（行38） |
| `TearParams.TearHeight` | `float TearHeight` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TearParams.md)（行43） |
| `TearParams.TearScale` | `float TearScale` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TearParams.md)（行48） |
| `TearParams.TearVariant` | `int TearVariant` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TearParams.md)（行53） |
| `TemporaryEffect.Cooldown` | `int Cooldown` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffect.md)（行26） |
| `TemporaryEffect.Count` | `int Count` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffect.md)（行31） |
| `TemporaryEffect.Item` | `ItemConfig Item Item` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffect.md)（行36） |
| `TemporaryEffects.AddCollectibleEffect` | `void AddCollectibleEffect ( CollectibleType CollectibleType, boolean AddCostume = true, int Count = 1 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行23） |
| `TemporaryEffects.AddNullEffect` | `void AddNullEffect ( NullItemID NullId, boolean AddCostume = true, int Count = 1 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行50） |
| `TemporaryEffects.AddTrinketEffect` | `void AddTrinketEffect ( TrinketType TrinketType, boolean AddCostume = true, int Count = 1 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行55） |
| `TemporaryEffects.ClearEffects` | `void ClearEffects ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行60） |
| `TemporaryEffects.GetCollectibleEffect` | `const TemporaryEffect GetCollectibleEffect ( CollectibleType CollectibleType )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行65） |
| `TemporaryEffects.GetCollectibleEffectNum` | `int GetCollectibleEffectNum ( CollectibleType CollectibleType )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行70） |
| `TemporaryEffects.GetEffectsList` | `const EffectList GetEffectsList ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行75） |
| `TemporaryEffects.GetNullEffect` | `const TemporaryEffect GetNullEffect ( ItemConfigNullItemID NullId )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行80） |
| `TemporaryEffects.GetNullEffectNum` | `int GetNullEffectNum ( ItemConfigNullItemID NullId )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行85） |
| `TemporaryEffects.GetTrinketEffect` | `const TemporaryEffect GetTrinketEffect ( TrinketType TrinketType )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行90） |
| `TemporaryEffects.GetTrinketEffectNum` | `int GetTrinketEffectNum ( TrinketType TrinketType )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行95） |
| `TemporaryEffects.HasCollectibleEffect` | `boolean HasCollectibleEffect ( CollectibleType CollectibleType )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行100） |
| `TemporaryEffects.HasNullEffect` | `boolean HasNullEffect ( ItemConfigNullItemID NullId )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行105） |
| `TemporaryEffects.HasTrinketEffect` | `boolean HasTrinketEffect ( TrinketType TrinketType )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行110） |
| `TemporaryEffects.RemoveCollectibleEffect` | `void RemoveCollectibleEffect ( CollectibleType CollectibleType, int Count = 1 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行115） |
| `TemporaryEffects.RemoveNullEffect` | `void RemoveNullEffect ( ItemConfigNullItemID NullId, int Count = 1 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行120） |
| `TemporaryEffects.RemoveTrinketEffect` | `void RemoveTrinketEffect ( TrinketType TrinketType, int Count = 1 )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/TemporaryEffects.md)（行125） |
| `Vector.Clamp` | `void Clamp ( float MinX, float MinY, float MaxX, float MaxY )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行106） |
| `Vector.Clamped` | `Vector Clamped ( float MinX, float MinY, float MaxX, float MaxY )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行111） |
| `Vector.Cross` | `float Cross ( Vector second )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行116） |
| `Vector.Distance` | `float Distance ( Vector first, Vector second )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行121） |
| `Vector.DistanceSquared` | `float DistanceSquared ( Vector first, Vector second )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行133） |
| `Vector.Dot` | `float Dot ( Vector second )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行145） |
| `Vector.FromAngle` | `static Vector FromAngle ( float AngleDegrees )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行150） |
| `Vector.FromAngle` | `void FromAngle ( float Angle )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Vector.md)（行37） |
| `Vector.GetAngleDegrees` | `float GetAngleDegrees ( )` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行162） |
| `Vector.Length` | `float Length ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行186） |
| `Vector.LengthSquared` | `float LengthSquared ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行191） |
| `Vector.Lerp` | `Vector Lerp ( Vector first, Vector second, float t )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行196） |
| `Vector.Normalize` | `void Normalize ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行219） |
| `Vector.Normalized` | `Vector Normalized ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行224） |
| `Vector.Resize` | `void Resize ( float NewLength )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行229） |
| `Vector.Resized` | `Vector Resized ( float NewLength )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行234） |
| `Vector.Rotated` | `Vector Rotated ( float AngleDegrees )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行239） |
| `Vector.Vector` | `Vector Vector ( float , float )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行19） |
| `Vector.X` | `float X` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行254） |
| `Vector.Y` | `float Y` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行259） |
| `Vector.__add` | `Vector __add ( Vector Right )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行36）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Vector.md)（行8） |
| `Vector.__div` | `Vector __div ( Vector Right )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Vector.md)（行15） |
| `Vector.__div` | `Vector __div ( float Modifier )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行48）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Vector.md)（行14） |
| `Vector.__mul` | `Vector __mul ( Vector Modifier )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行62） |
| `Vector.__mul` | `Vector __mul ( Vector Right )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Vector.md)（行22） |
| `Vector.__mul` | `Vector __mul ( float Modifier )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行60）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Vector.md)（行21） |
| `Vector.__sub` | `Vector __sub ( Vector Right )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行81）<br>[repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Vector.md)（行28） |
| `Vector.__tostring` | `void __tostring ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行244） |
| `Vector.__unm` | `Vector __unm ( Vector Right )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/Vector.md)（行93） |
| `VectorList.Get` | `Vector Get ( int idx )` | bounded_parameter_query  | [vanilla](F:/projects_isaac/IsaacDocs/docs/CppContainer_Vector_VectorList.md)（行29） |
| `VectorList.Size` | `const int Size` | public_lua_copy  | [vanilla](F:/projects_isaac/IsaacDocs/docs/CppContainer_Vector_VectorList.md)（行35） |
| `VectorList.__len` | `int __len ( )` | actual_event_only  | [vanilla](F:/projects_isaac/IsaacDocs/docs/CppContainer_Vector_VectorList.md)（行21） |
| `Weapon.ClearItemAnim` | `void ClearItemAnim ( int ItemID )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行20） |
| `Weapon.GetCharge` | `float GetCharge ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行24） |
| `Weapon.GetDirection` | `Vector GetDirection ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行28） |
| `Weapon.GetFireDelay` | `float GetFireDelay ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行32） |
| `Weapon.GetMainEntity` | `Entity GetMainEntity ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行36） |
| `Weapon.GetMaxCharge` | `float GetMaxCharge ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行43） |
| `Weapon.GetMaxFireDelay` | `float GetMaxFireDelay ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行47） |
| `Weapon.GetModifiers` | `WeaponModifier GetModifiers ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行51） |
| `Weapon.GetNumFired` | `int GetNumFired ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行55） |
| `Weapon.GetOwner` | `Entity GetOwner ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行59） |
| `Weapon.GetWeaponType` | `WeaponType GetWeaponType ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行63） |
| `Weapon.IsAxisAligned` | `boolean IsAxisAligned ( )` | public_lua_copy  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行67） |
| `Weapon.IsItemAnimFinished` | `boolean IsItemAnimFinished ( int ItemID )` | bounded_parameter_query  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行71） |
| `Weapon.PlayItemAnim` | `void PlayItemAnim ( int ItemID, int AnimId, Vector Direction, float unk )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行75） |
| `Weapon.SetCharge` | `void SetCharge ( float Charge )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行79） |
| `Weapon.SetFireDelay` | `void SetFireDelay ( float Delay )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行83） |
| `Weapon.SetHeadLockTime` | `void SetHeadLockTime ( int Time )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行87） |
| `Weapon.SetModifiers` | `void SetModifiers ( WeaponModifier modifiers )` | actual_event_only  | [repentogon](F:/projects_isaac/REPENTOGON-1.1.3/docs/docs/Weapon.md)（行91） |
