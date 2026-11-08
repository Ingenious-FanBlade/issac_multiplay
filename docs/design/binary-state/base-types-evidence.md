# 第 2 步：基础类型 J273 静态证据

目标为 REPENTOGON 1.1.3 SDK 与 Isaac v1.9.7.12.J273 Win32。以下 IDA 地址均为 VA，imagebase = 0x400000；RVA = VA − 0x400000。状态标记统一为：**已核**（J273 指令或官方实现直接确认）、**SDK声明**（官方 ZHL/源码声明，尚无独立 J273 访问证据）、**推断**（可解释且采取保守映射）、**未知**（语义未明，按字节保留，不视为填充）。

布局唯一输入已写入 mod/binding/state/base.json。本文记录可复核依据和未闭合项；未知项不以 SDK 名字、构造初值或对齐假设填成确定语义。

## 类型逐字节归属

### Vector，8 字节

- +0x00..+0x03：x，f32 状态；+0x04..+0x07：y，f32 状态。两分量均有限值校验。
- 两个成员来自官方 Vector2.zhl；公开 Vector.X/Y 直接对应分量，GetAngleDegrees 可由 x/y 计算。
- J273 直接证据：VA 0x9E5BC0（RVA 0x5E5BC0）中，0x9E5BE6 读取 `this[1]` 与 `other[1]` 作线性插值，0x9E5BEB 插值 `this[0]` 与 `other[0]`，0x9E5BEF 写回 `this[1]`。主审已复读这三条指令；这确认两个分量均为 f32。
- RTTI：非多态纯值类型，无 RTTI。

### ColorMod，44 字节；公开 owner 为 Color

- +0x00、+0x04、+0x08、+0x0C：tint RGBA 四个 f32。
- +0x10、+0x14、+0x18、+0x1C：colorize RGBA 四个 f32。
- +0x20、+0x24、+0x28：offset RGB 三个 f32。
- J273 直接证据：VA 0x402760（RVA 0x2760）构造函数将前四个 DWORD 写为 1.0，其余七个写 0；VA 0x67E940（RVA 0x27E940）按 float 数组处理 index 0..10：0..3 相乘、4..7 条件合并、8..10 相加。主审已复读构造初值及运算分组。该顺序与官方 `ColorMod::operator*=` 的 tint/colorize/offset 语义一致，11 个分量已由 J273 直接访问确认。
- REPENTOGON LuaColor.cpp 的 GetTint/GetColorize/GetOffset 分别逐项读取对应数组。实例属性 R/G/B/A 映射 tint，RO/GO/BO 映射 offset。
- Color 目录的 39 个具名常量（含 EmberFade）是静态全局颜色，不是实例状态，全部映射为 not_applicable。其余 10 个公开成员已按上列字段映射。
- RTTI：非多态纯值类型，无 RTTI。

### KColor，20 字节；公开 owner 为 KColor

- +0x00、+0x04、+0x08、+0x0C：red、green、blue、alpha，四个 f32。
- +0x10..+0x13：颜色空间 u32；J273 转换函数将 0、1、2 分别用于 RGB、sRGB、HSL。
- J273 VA 0x9E37B0（RVA 0x5E37B0）要求输入 +0x10=0，并报错文本“Specified color isn't in the RGB color space”；VA 0x9E39F0（RVA 0x5E39F0）要求值 1，报 sRGB 错误并将输出标记写为 0；VA 0x9E3BA0（RVA 0x5E3BA0）要求值 2，报 HSL 错误并将输出标记写为 0；VA 0x9E3DF0（RVA 0x5E3DF0）也检查值 2，成功转换分支于 VA 0x9E3EB0 写输出标记 1。转换分派表 VA 0xAB1E00（RVA 0x6B1E00）有三个完整的三项函数组，对应 0/1/2；本文只确认这三个被支持的标签，不宣称其他整数在全程序中都非法。
- +0x10 是有语义的值状态，不是指针或填充。KColor Lua 公开的 Red/Green/Blue/Alpha 分别映射四个分量；颜色空间标记没有单独 Lua 成员，但会影响转换路径，必须保留。
- RTTI：非多态纯值类型，无 RTTI。

### BlendMode，20 字节；公开 owner 为 BlendMode

- +0x00 equation、+0x04 rgbSourceFactor、+0x08 rgbDestinationFactor、+0x0C alphaSourceFactor、+0x10 alphaDestinationFactor：五个连续 u32。
- J273 VA 0x9E17B0（RVA 0x5E17B0）GetMode 写出五个 DWORD；VA 0x9E17D2 等指令可见首字段写入，VA 0x9E17EF 附近覆盖末字段。主审已复读确认五个偏移为 +0/+4/+8/+C/+10。
- 官方 Global.zhl 第 3340—3362 行定义 equation 合法范围 0..4（ADD、SUBTRACT、REVERSE_SUBTRACT、MIN、MAX），factor 合法范围 0..9（ZERO、ONE、SRC_COLOR、ONE_MINUS_SRC_COLOR、DEST_COLOR、ONE_MINUS_DST_COLOR、SRC_ALPHA、ONE_MINUS_SRC_ALPHA、DST_ALPHA、ONE_MINUS_DST_ALPHA）；REP LuaBlendMode.cpp 用对应 `NUM` 上限校验。布局与值域都已核。
- 当前 SDK `BlendMode.zhl` 已声明上述字段；旧计划中“SDK 未声明”的说明已纠正。LuaBlendMode.cpp 对应 getter/setter 按该布局访问。
- RTTI：非多态纯值类型，无 RTTI。

### AnimationState，32 字节

- +0x00..+0x03：所属 ANM2 回指，进程本地指针。
- +0x04..+0x07：active AnimationData 资源选择。传动画表序号，不传地址。
- +0x08..+0x0B：逐层帧选择器数组指针；长度来自当前 AnimationData 的 layer_count。
- +0x0C..+0x0F：逐空层帧选择器数组指针；长度来自当前 AnimationData 的 null_layer_count。
- +0x10..+0x13：animFrame f32；+0x14：isPlaying bool；+0x15..+0x17：三个未解释字节；+0x18..+0x1B：当前事件掩码；+0x1C..+0x1F：上一事件掩码。
- J273 VA 0x4041A0（RVA 0x41A0）直接以 f32 读写 +0x10：VA 0x4041B2 读取 `this+0x10`，VA 0x404239 将 `floor(this+0x10)+1.0f` 写回。主审已复读两条访问指令。两个帧选择器数组不能派生这个播放位置。
- J273 VA 0x404390（RVA 0x4390）SetLayerFrame 先用 ANM2 +0x80 验证公开 LayerID，再用 AnimationData +0x38 的映射取得内部层序号，并与 AnimationData +0x1C 比较，随后写入 state +0x08 的选择器数组。VA 0x4040B0（RVA 0x40B0）AdvanceNullLayerPosition 使用 state +0x0C 和 AnimationData +0x24 的 null count。两条路径分别选择。
- 两个数组不是 animFrame 的派生值：AnimationFrame 序号与用户帧号不同，单层 SetLayerFrame 可以独立改变选择器；空层另有计数、名称表和选择器。因此必须分别同步。计数必须取当前 AnimationData，而不是 ANM2 总图层数或空层数。
- 事件查询 J273 VA 0x406710（RVA 0x6710）读 +0x18，VA 0x4067B0（RVA 0x67B0）读 +0x1C；AdvancePosition VA 0x4041A0 在 VA 0x4041FE 清除 +0x18。SDK 未解释 +0x15..+0x17，保留未知，不当作 padding。
- RTTI：非多态类型，无 RTTI。

### LayerState，160 字节

- +0x00..+0x03：LayerData 资源指针，由文件和元素位置派生；+0x04..+0x07：所属 ANM2 本地指针。
- +0x08..+0x1F：当前 spritesheet 路径 std::string，转资源标识。
- +0x20、+0x24、+0x28、+0x2C：minFilter、magFilter、wrapS、wrapT 四个 u32 状态。
- +0x30：spriteSheetLoaded，原生 bool 状态；按 u8 保留，图层资源准备与它的重建关系仍须第 8 步闭合。
- +0x31：SDK 的 _unk。J273 LoadGraphics VA 0x407DA0（RVA 0x7DA0）在 VA 0x407F7A（RVA 0x7F7A）将其置 0；语义未知，不能当 padding。
- +0x32、+0x33：flipX、flipY bool。
- +0x34..+0x3B：size Vector；+0x3C..+0x3F：rotation f32；+0x40..+0x47：position Vector；+0x48..+0x73：ColorMod。
- +0x74：visible bool；+0x75..+0x77：三个未解释字节，不当 padding。
- +0x78..+0x8B：BlendMode 五 DWORD；+0x8C..+0x8F：render flags u32；+0x90..+0x97：cropOffset Vector。
- +0x98..+0x9F：KAGE_SmartPointer_ImageBase，由 image 指针和引用计数控制块指针组成。它是真实图像选择状态，不是可丢弃本地字段。
- GetName、GetLayerID、GetDefaultSpritesheetPath 依赖所属 ANM2 文件和元素序号，布局用父上下文映射；不把 LayerData* 地址发送。
- 重要区别：LuaSprite.cpp 的 SetSpritesheet 调用 LayerState::SetSpriteSheet。官方 LayerState.zhl 内联实现替换 +0x98 smart pointer，并更新 filter/wrap/loaded，不写 +0x08 路径。因此仅同步路径会漏掉同路径下的 Image 对象替换。KAGE_Graphics_Image.zhl 声明 ImageBase::_name 在 ImageBase +0x3C，首版用该名字做资源身份候选，但程序图像、重名和稳定唯一性还没有证实；需第 8 步核图像资源路径/身份接线。当前字段标记为推断，不宣称 LayerState 完整。
- HasCustomShader 与 HasCustomChampionShader 不在 J273 LayerState 字节中。REPENTOGON Anm2Extras.cpp:42-82、约 170-235 使用 ANM2*/LayerState* 旁表；不能映射到 LayerState +0x31 或 ANM2 +0x108/+0x10C。需第 8 步独立接入旁表状态。
- RTTI：无多态虚表，SDK 对象无 RTTI。

### ANM2，276 字节

- +0x000..+0x017：fileName std::string，文件资源身份。
- +0x018..+0x02F：animDefaultName std::string。它在 Load 阶段从 cache 拷贝，按有界字符串状态传输；不因当前未发现运行期 setter 就推成不可变派生字段。
- +0x030..+0x04F：主 AnimationState；+0x050..+0x06F：叠加 AnimationState。
- +0x070：overlayRenderFirst bool；+0x071..+0x073：三个未知字节。
- +0x074..+0x077：AnimationData* 资源数组；+0x078..+0x07B：动画数。
- +0x07C..+0x07F：LayerState* 数组；+0x080..+0x083：层数。图层实例通过该数组容器同步。
- +0x084..+0x087：null layer names 表指针；+0x088..+0x08B：空层数量。+0x08C..+0x08F：event names 表指针；+0x090..+0x093：事件数量。均由文件构造。
- +0x094..+0x09B：offset Vector；+0x09C..+0x0A3：scale Vector；+0x0A4..+0x0A7：rotation f32；+0x0A8..+0x0D3：ColorMod；+0x0D4..+0x0FF：championColor ColorMod。
- +0x100、+0x101：flipX、flipY bool；+0x102..+0x103：两个未知字节；+0x104..+0x107：playbackSpeed f32。
- +0x108：未知单字节。REP shader 状态属于旁表，不是该字段。
- +0x109：loaded bool，公开 Sprite.IsLoaded 对应的 u8 状态。J273 Load 在 VA 0x407B52 写 1；J273 VA 0x41B820（RVA 0x1B820）getter 直接返回 `this[265]`，即 +0x109。主审已复读 getter。
- +0x10A..+0x10B：两个未知字节。
- +0x10C..+0x10F：shadow layer index，文件派生 i32。J273 Load VA 0x407AF0 在 VA 0x407B59 初始化 -1；VA 0x407BC9 在扫描到名称 shadow 的层时写当前层序号。因此 +0x10C 不是 shader 状态。
- +0x110..+0x113：render flags u32。
- defaultName 赋值证据：J273 ANM2::Load VA 0x407AF0（RVA 0x7AF0）调用 sub_409680；sub_409680 在 VA 0x4099D4、0x4099E9 将 cache 中字符串拷贝到 this +0x18。ANM2 构造 VA 0x403530 初始化该 std::string；拷贝/交换路径 VA 0x402CD0 也复制它。SetAnimation VA 0x405F50 只切换当前 AnimationData，不修改 defaultName。结论：该字段确实可写且加载时被写；未发现 J273 运行期 setter，但没有足够证据断言运行时绝不变化，所以源按字符串状态保留。
- RTTI：无多态虚表，SDK 对象无 RTTI。

### RNG，16 字节

- +0x00 seed、+0x04/+0x08/+0x0C 三个 shift：四个 u32 全部属于随机数 authority state，全部不发送。
- J273 SetSeed VA 0x7DF4A0（RVA 0x3DF4A0）写 seed 和三个 shift；RandomInt VA 0x7DF4E0、RandomFloat VA 0x7DF540、Next VA 0x7DF5B0 读取 shift 并更新 seed。
- RTTI：非多态类型，无 RTTI。

### BitSet128，16 字节

- +0x00 ll、+0x04 lh、+0x08 hl、+0x0C hh：四个 u32，合计 128 位。
- 官方 BitSet128.zhl 的构造、AddFlags、RemoveFlags、HasAny 分别使用四个 DWORD；整值按位保留。
- J273 直接 ABI 证据：按 EntityTear.zhl 签名匹配的 SetTearFlags 位于 VA 0x672BF0（RVA 0x272BF0），输入 `BitSet128` 以 16 字节值传入。VA 0x673619（RVA 0x273619）用 `movups` 读取值参数，VA 0x67361D（RVA 0x27361D）将 16B 写到 Entity_Tear +0x428。主审独立复读了读写两条指令，直接确认尺寸和连续原样复制；四字顺序由官方构造签名明确。
- RTTI：非多态纯值类型，无 RTTI。

### EntityPtr，4 字节

- +0x00..+0x03：Entity* 引用，转为成员索引；不发送地址。
- J273 EntityPtr::SetReference VA 0x858760（RVA 0x458760）先从旧 Entity 引用链移除此对象，再记录并挂接新引用。客户端必须经对应引用机制恢复，不能只写指针。
- RTTI：非多态类型，无 RTTI。

### ColorParams，60 字节；公开 owner 为 ColorParams

- +0x00..+0x03：priority i32；+0x04..+0x2F：ColorMod。
- +0x30..+0x33：duration1，公开 Duration；+0x34..+0x37：duration2，公开 Lifespan。构造函数均初始化为 -1。
- +0x38：fadeout bool；+0x39：shared bool；+0x3A..+0x3B：两个未解释字节，不当 padding。
- 官方 LuaColorParams.cpp 的 Get/SetDuration 直接访问 _duration1；Get/SetLifespan 直接访问 _duration2；Get/SetFadeout 和 Get/SetShared 分别访问两个 bool。
- RTTI：非多态类型，无 RTTI。

第 3 步补充：J273 Entity 颜色更新在 VA `0x6AA122/0x6AA12A/0x6AA12E` 对 `+0x34` Lifespan 无条件有符号递减，负值也继续减，仅结果为零时移除元素；`+0x30` Duration 只作淡出比例分母。布局版本 10 将 Lifespan 改为条件、有符号、无夹限的帧锚点，Duration 仍直接复制。普通正倒计时的“负哨兵保持／到期夹零”不适用于此字段，见[第 3 步实现](implementation-3.md)。

### ActiveItemDesc，32 字节；公开 owner 为 ActiveItemDesc

- +0x00 item ID；+0x04 item_pool；+0x08 charge；+0x0C batteryCharge；+0x10 subCharge；+0x14 timedRechargeCooldown：六个 i32。
- +0x18 partialCharge：f32；+0x1C varData：i32。
- +0x04 虽在 SDK 记作 _unk，却由 J273 AddCollectible VA 0x758F00 的参数写入点 VA 0x759864 确认为 ItemPool 来源。
- SDK 把 +0x18 声明为 int 是错误的。公开目录与 Wiki 类型为 float；J273 Lua 注册 closure 在 VA 0x863B93 写入 getter upvalue 偏移 0x18、VA 0x863BB9 以 PartialCharge 名注册；VA 0x863C4E 写入 setter upvalue 偏移。通用 getter VA 0x8A4510 与 setter VA 0x8A4570 的 movss 读写由该注册闭包绑定。主审已复读注册和 movss 指令，确认成员名关联。
- RTTI：非多态类型，无 RTTI。

### Weapon，SDK 声明 64 字节；已核 base_prefix 68 字节

- RTTI 主审纠错：TypeDescriptor 为 VA 0xBD1218、RVA 0x7D1218；VA 0xBD1220 是其名字字符串起点，值为 `.?AVWeapon@IsaacRepentancePlus@@`。主审直接回读两字 header 和装饰名，并从 vtable 前项 VA 0xB31C60 得到 COL VA 0xB452C8，其 +0xC 指针确为 0xBD1218。首版把字符串起点误当 descriptor 的偏移已经纠正。
- +0x00 vtable 本地；+0x04 owner Entity* 引用；+0x08 未决 Entity*（SDK 曾注释为 _entity，构造初始化 null）。
- +0x0C fireDelay f32；+0x10 maxFireDelay f32；+0x14 charge f32。
- +0x18..+0x1F：两个 ctor 初始化值（1.0、0），语义未知。
- +0x20..+0x27 direction Vector。
- +0x28..+0x2F：两个 f32；IsAxisAligned VA 0x9C5090 读取这两个值，但具体意义和 owner 条件仍需完整解释。首版有限值校验并按 state 发送。
- +0x30 weaponType i32；+0x34 modifiers u32；+0x38 numFired i32；+0x3C 一个 ctor 写零的未命名 DWORD。
- +0x40..+0x43：SDK sizeof 之外但 ctor 写入的 DWORD，语义未明。
- ctor VA 0x9C4EB0（RVA 0x5C4EB0）在 VA 0x9C4F10 写 this+0x40；Init VA 0x9C5200 设置 owner/type。player-application.json:62582 已指出 SDK sizeof(Weapon)=0x40 不是分配大小。
- native_size 0x44 只标记基类构造足迹 base_prefix，不表示动态 Weapon 对象完整大小。Isaac::CreateWeapon VA 0x9CADB0 依注册 factory 创建不同子类；player-application R08 已列实际分配大小，具体子类边界由第 5 步静态核对。
- 已回映射的基类成员：GetOwner、GetFireDelay、GetMaxFireDelay、GetCharge、GetDirection、GetWeaponType、GetModifiers、GetNumFired。
- 未闭合公开成员：GetMaxCharge 是虚调用/子类语义，父类例程不能覆盖各子类；IsAxisAligned 依赖 +0x28/+0x2C 与 owner/道具条件；IsItemAnimFinished 查询 owner 动画状态；GetMainEntity 是虚函数/子类行为。故不声明 Weapon public_owner；没有把这些成员伪造为单纯 base-field 映射。
- Weapon vtable VA 0xB31C64（RVA 0x731C64）第 10 槽对应 GetMainEntity；基类槽指向 VA 0x9C4E70（RVA 0x5C4E70），其 `UserMathErrorFunction()` 固定返回 0，不读取 +0x08。故 SDK 注释掉的 `_entity` 不能等同于已证 GetMainEntity；派生类语义仍待动态工厂核对。
- RTTI：已核上述 Weapon type descriptor；子类分配大小不在本布局中声称完整。

### TemporaryEffect，16 字节；公开 owner 为 TemporaryEffect

- +0x00..+0x03：ItemConfig_Item*，按配置 ID 资源解析；+0x04..+0x07 count i32；+0x08..+0x0B cooldown i32。
- +0x0C..+0x0F：SDK 类型大小范围内但无字段声明的 DWORD。没有证据证明它是填充，按未知字节保留。
- J273 VA 0x90A150（RVA 0x50A150）AddCollectibleEffect 构造四 DWORD 临时记录，+0x0C 在 VA 0x90A194 明确置零；VA 0x90A090（RVA 0x50A090）插入新项时以 16 字节整体复制（VA 0x90A0D3）并把 count 写入元素 +0x04（VA 0x90A116）。合并重复项时只更新 +0x04 和 +0x08（VA 0x90A0E3、0x90A0E9），不解释 +0x0C。删除路径 VA 0x90A340 也构造末 DWORD 为零的记录。故 +0x0C 不是已证填充；普通新增为零，但通用 16B 插入保留来源值，字段用途仍未知，不能仅凭“通常写零”省略。
- RTTI：非多态类型，无 RTTI。

## 主审必须复读的关键指令

| 主题 | J273 VA / RVA | 需确认内容 |
| --- | --- | --- |
| 动画普通层选择器 | 0x404390 / 0x4390 | ANM2 +0x80 验证层 ID；AnimationData +0x38 映射内部层序号；与 +0x1C 层数比较；写 AnimationState +0x08 数组 |
| 动画空层选择器 | 0x4040B0 / 0x40B0 | AnimationState +0x0C 独立数组；计数从当前 AnimationData +0x24 来 |
| 动画帧 f32 | 0x4041A0 / 0x41A0；读 0x4041B2、写 0x404239 | 直接读写 AnimationState +0x10；普通事件掩码在 0x4041FE 清零 |
| 事件掩码 | 0x406710 / 0x6710；0x4067B0 / 0x67B0 | 分别读 +0x18 和 +0x1C |
| BlendMode 五 DWORD | 0x9E17B0 / 0x5E17B0；0x9E17D2、0x9E17EF 等 | 五个连续写入覆盖 +0、+4、+8、+C、+10 |
| Vector | 0x9E5BC0 / 0x5E5BC0；0x9E5BE6、0x9E5BEB、0x9E5BEF | 对 this[0]/[1] 与 other[0]/[1] 作 f32 插值，主审已复读 |
| ColorMod | 0x402760 / 0x2760；0x67E940 / 0x27E940；0x67E951—0x67EA4B | ctor 写 11 个 DWORD；运算函数确认 tint/colorize/offset 的 4/4/3 float 分组，主审已复读 |
| KColor 颜色空间 | 0x9E37B0、0x9E39F0、0x9E3BA0、0x9E3DF0 | +0x10 标签 0/1/2 对应 RGB/sRGB/HSL；9E3DF0 成功分支在 0x9E3EB0 写结果标签 1 |
| BitSet128 | SetTearFlags 0x672BF0 / 0x272BF0；读 0x673619、写 0x67361D | 输入 16 字节值整体复制到 Tear +0x428；官方构造签名确定 ll/lh/hl/hh 顺序 |
| ANM2 shadow 元数据 | 0x407AF0 / 0x7AF0；0x407B59、0x407BC9 | +0x10C 先为 -1；层名 shadow 匹配后写数组序号 |
| Sprite.IsLoaded | getter 0x41B820 / 0x1B820；Load 写 0x407B52 / 0x7B52 | getter 直接返回 ANM2 +0x109；主审已复读 |
| LayerState 未知 byte | 0x407DA0 / 0x7DA0；0x407F7A / 0x7F7A | LoadGraphics 写 LayerState +0x31=0 |
| defaultName | 0x4099D4、0x4099E9；调用源 0x407AF0 | Load 从 cache 写 +0x18 字符串；不得用 SetAnimation 不写它来证明不可变 |
| Weapon 构造边界 | 0x9C4EB0 / 0x5C4EB0；store 0x9C4F10 / 0x5C4F10 | 基类 ctor 写到 +0x40，native prefix 至少 0x44 |
| Weapon AxisAligned | 0x9C5090 / 0x5C5090 | 访问 +0x28/+0x2C，并依赖 owner 语境 |
| ActiveItemDesc item_pool | 0x758F00 / 0x358F00；store 0x759864 / 0x359864 | +0x04 是 ItemPool 来源，不是配置地址 |
| PartialCharge 绑定 | 0x863B93、0x863BB9、0x863C4E；getter/setter 0x8A4510、0x8A4570 | 注册闭包把偏移 0x18 绑定到 PartialCharge；movss 是 float |
| RNG 四状态 | 0x7DF4A0、0x7DF4E0、0x7DF540、0x7DF5B0 | seed 和三个 shift 的构造/推进读写 |
| EntityPtr 引用链 | 0x858760 / 0x458760 | SetReference 维护引用列表，不能 memcpy 地址 |

## 目前的闭合度与后续归属

- 可公开完整回映射：Vector、Color、KColor（公开 RGBA 成员；另含已核的 color_space 原生状态）、BlendMode、ColorParams、ActiveItemDesc、TemporaryEffect。Color 所有静态常量均是不适用实例字段。
- 尚不声明完整 owner：Sprite、LayerState 因 shader 旁表/实际 Image 资源身份未闭合；Weapon 因动态子类边界和四个虚/上下文成员未闭合。
- 明确仍未知：TemporaryEffect +0x0C；Weapon +0x08、+0x18..+0x1F、+0x3C、+0x40 的语义；LayerState 实际 ImageBase name 到可重建资源标识的唯一性；AnimationState +0x15..+0x17、LayerState +0x31/+0x75..+0x77、ANM2 +0x71..+0x73/+0x102..+0x103/+0x108/+0x10A..+0x10B 的含义。未知范围不按填充或 authority 编造。
- 本逆向子任务只做 SDK/官方源码读取和 IDA 静态分析，没有运行游戏、实验、哈希校验、生成器或测试。主集成另已在普通构建中运行生成器并完成 Release 纯编译/链接，没有启动游戏或执行测试。

## 第 1 步 EntityEffect 输入核对

- J273 Effect 池初始化位于 VA 0x6EEE60（RVA 0x2EEE60）。VA 0x6EEE96 压入元素数 0x400，VA 0x6EEE9B 压入元素步长 0x4C0，VA 0x6EEEA1 调用 MSVC EH vector constructor iterator；构造/析构回调分别为 VA 0x5CCEA0 和 0x5CCFD0。该池步长是 EntityEffect 第 1 步原生范围依据，覆盖到 `[0x000,0x4C0)`，与 SDK `Entity_Effect.zhl` 声明 0x490 不同。
- J273 ctor VA 0x5CCEA0（RVA 0x1CCEA0）调用 Entity 基类构造后，在 VA 0x5CCEAF 写 `IsaacRepentancePlus::Entity_Effect::vftable`，直接确认实际对象类。
- MSVC TypeDescriptor 位于 VA 0xBD1270（RVA 0x7D1270）；其名字字符串从 VA 0xBD1278 起，值为 `.?AVEntity_Effect@IsaacRepentancePlus@@`。TypeDescriptor 前 8 字节为两个 32 位字段，装饰名紧随其后。布局输入可据此用 RVA 0x7D1270 区分 EntityEffect RTTI。
- 该证据只确定 EntityEffect 类型身份、池步长和构造器边界；对象内部成员/变体臂按第 3、4 步证据继续核对，不能由 SDK 0x490 推出尾部是填充。
