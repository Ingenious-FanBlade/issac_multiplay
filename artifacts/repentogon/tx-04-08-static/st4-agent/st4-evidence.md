# tx-04-08 ST4 原始静态证据

本文件只保存 ST4 的静态摘录与来源定位，供主审逐项回读。IDA 数据库目标为 isaac-ng-v1.9.7.12.exe.unpacked.exe，imagebase 0x400000。VA→RVA = VA−0x400000。IDB 查询只读；没有修改 IDB，没有运行测试、构建、校验脚本、游戏或调试附着。

标记约定：FACT 是目标指令/SDK/原型中的直接内容；INFERENCE 是由多个事实推得；UNKNOWN 是现有静态证据不足以确认的语义。

## Grid 447/448 边界

### VA 0x431710 / RVA 0x031710 — Room::GetGridEntity

IDA 当前函数原始反汇编：

    0x431710  push ebp
    0x431711  mov ebp, esp
    0x431713  mov eax, [ebp+arg_0]
    0x431716  cmp eax, 1BFh
    0x43171B  ja short loc_431725
    0x43171D  mov eax, [ecx+eax*4+24h]
    0x431721  pop ebp
    0x431722  retn 4
    0x431725  xor eax, eax

Room.zhl:14 给出 SDK 签名 Room::GetGridEntity(uint32_t idx)。FACT：unsigned index 0x1BF 不走 ja，会读取 Room+0x24+index*4。最大有效 index 是 447，长度为 448。

### VA 0x7E21D0 / RVA 0x3E21D0 — Room Grid spawn/publish

    0x7E21DF  cmp esi, 1C0h
    ...
    0x7E21F6  cmp esi, 1BFh
    0x7E21FC  ja failure
    ...
    0x7E223D  mov ecx, [ebx+esi*4+24h]
    ...
    0x7E2253  mov [ebx+esi*4+24h], edi

Room.zhl 在 SpawnGridEntity/SpawnGridEntityDesc 附近声明 Grid 创建函数。FACT：成功处理的索引检查将 447 作为包含式上界，之后从 +0x24 读旧 Grid 指针并可能写新指针；前面的 cmp 1C0h 是另一分支，不可单独解释成 447 slots。

### VA 0x813BD0 / RVA 0x413BD0 — Room::RemoveGridEntityImmediate

    0x813BD9  cmp edx, 1BFh
    0x813BDF  ja loc_813C0D
    0x813BE1  mov ecx, [esi+edx*4+24h]
    ...
    0x813BF8  mov [esi+edx*4+76Ch], eax
    0x813BFF  mov dword ptr [esi+edx*4+24h], 0

SDK Room.zhl 声明 RemoveGridEntityImmediate(int GridIndex, int PathTrail, bool KeepDecoration)。FACT：对 447 有合法路径，并按 4-byte 槽访问 Grid pointer。另据函数分支，KeepDecoration 对特定 Grid type 的路径保留原 Grid 指针；remove/free 请求不是销毁完成事实。

### VA 0x7DF8F0 / RVA 0x3DF8F0 — Room constructor 初始化范围

主审独立 IDA 回读记录在 artifacts/repentogon/tx-04-08-static/st3-st5-review/main-room-constructor.json；关键反汇编为：

    0x7DFFF0  push 720h          ; Size
    0x7DFFF5  lea eax, [edi+24h]
    0x7DFFFC  push 0
    0x7DFFFF  call memset

FACT：0x744-0x24=0x720 bytes；0x720/4=456 pointer words。结合 SDK 布局，span 被划分为 448 Grid pointers (0x700 bytes) + 8 Door pointers (0x20 bytes)。该 memset 单独只证明连续零区，不能单独区分数组语义；与三个独立包含式 0x1BF 访问边界合并后结论确定为 448 Grid slots。

## Door 数组与 Grid 槽别名

### VA 0x7E4DE0 / RVA 0x3E4DE0 — Room::MakeDoor

    0x7E4E10  cmp dword ptr [edi+ebx*4+724h], 0
    0x7E4E21  mov esi, [edi+ebx*4+744h]     ; door slot -> grid index
    0x7E4E4D  push dword ptr [edi+esi*4+24h] ; read current Grid pointer
    ...
    0x7E4F7B  mov [edi+eax*4+24h], esi      ; publish/replace Grid slot
    ...
    0x7E4FE7  mov [edi+ebx*4+724h], esi     ; publish Door slot

SDK Room.zhl:92 声明 Room::MakeDoor(int slot)。FACT：Door slot、door-to-grid index、Grid slot 是三种地址域；同一个 Door pointer 可被写进 Room+0x724+doorSlot*4 和 Room+0x24+gridIndex*4。原始 source membership 应同时保留，但对象地址只建一个物理身份。主体没有显示完整可依赖的边界检查，采集器应先限制 door slot <8，再限制 grid index <448。

## Tile index 与坐标依据

### VA 0x808700 / RVA 0x408700 — Room::GetGridIndexByTile

    0x80870D  mov eax, [esi+0Ch]      ; width
    0x808710  cmp edx, eax            ; column range check
    ...
    0x80871B  cmp ecx, [esi+10h]     ; row < height
    0x808720  imul eax, ecx           ; row * width
    0x808724  add eax, edx            ; + column

FACT：row-major slot index 为 column + row*width，并以 width/height 做维度边界检查。SDK Room.zhl:182-183 声明 width +0xC、height +0x10。

### VA 0x8087A0 / RVA 0x4087A0 — Room::GetGridPosition

主审独立回读 Lua 注册与函数本体，证据归档在 artifacts/repentogon/tx-04-08-static/st3-st5-review/main-subplayer-gridposition.json：

    0x852F99  push sub_8087A0
    0x852F9E  push string GetGridPosition (string VA 0xB24440)
    0x8087A0  push ebp
    0x8087A1  mov ebp, esp
    0x8087A3  mov eax, [ebp+arg_4]    ; index
    0x8087A6  cdq
    0x8087A7  idiv dword ptr [ecx+0Ch]; quotient=row, remainder=column
    0x8087AA  movss xmm1, dword_B4073C
    0x8087B2  mov ecx, eax
    0x8087B4  mov eax, [ebp+arg_0]    ; result Vector*
    0x8087B7  movd xmm0, edx          ; remainder
    0x8087BE  mulss xmm0, xmm1
    0x8087C2  addss xmm0, xmm1
    0x8087C6  movss [eax], xmm0       ; x = remainder*40 + 40
    0x8087CA  movd xmm0, ecx          ; quotient
    0x8087D1  mulss xmm0, xmm1
    0x8087D5  addss xmm0, dword_B40830
    0x8087DD  movss [eax+4], xmm0     ; y = quotient*40 + 120
    0x8087E3  retn 8

函数本体直接以 Grid index 和 Room width 做 remainder/division，分别乘 tile step 40，再加 x=40、y=120 后返回 Vector，没有 render/camera 变换。FACT：GridPosition getter 语义为 x=(index%width)*40+40、y=(index/width)*40+120。旧原型从槽 index 得出的公式与该 getter 相同。UNKNOWN：此派生 GridPosition 是否与 Entity raw Position 字段语义相同。

注意：该 getter 没有读取 Room height；GetGridIndexByTile 的 height 检查属于独立的 row/column 到 index API，不能用来缩减固定 448 个 Grid 槽，也不能作为 GetGridPosition 的 i<width*height 条件。应用公式时须有非零有界 width、index<448，以及该对象与 Grid 槽的唯一稳定关联；height 只可独立记录/诊断。

### VA 0x705110 / RVA 0x305110 — GridEntity::GetRenderPosition

    0x705116  mov eax, [ecx+24h]      ; grid index
    0x705130  mov ecx, [ecx+18300h]   ; current Room through Game object
    0x705147  idiv dword ptr [ecx+0Ch]; divide by Room width
    0x705164  mulss xmm1, dword_B4073C; tile step constant
    0x70516C  addss xmm1, dword_B4073C; x basis constant
    0x705174  subss xmm1, dword_B40788; render adjustment
    0x70519F  call floor
    ...
    0x7051C5  mulss xmm1, dword_B4073C; row tile step
    0x7051DD  addss xmm1, dword_B40830; y basis constant
    0x7051ED  subss xmm1, dword_B4085C; render adjustment
    0x705219  call floor

IDA constant references for the step/basis correspond to the old prototype's tile basis 40, x=40 and y=120; function subsequently applies render/camera/scale/floor work. FACT：render position depends on gridIndex / Room.width and remainder. INFERENCE：old prototype formula x=40*(i%width)+40; y=40*(i/width)+120 matches the untransformed cell basis. UNKNOWN：whether this formula equals the Entity raw Position field; the render function includes further transforms and is not an assignment to that field.

## Player relation evidence

### VA 0x7B9FD0 / RVA 0x3B9FD0 — relation reads by PlayerType

    0x7B9FDB  cmp eax, 28h
    0x7B9FE0  mov eax, [ecx+1E4Ch]
    0x7B9FE6  test eax, eax
    ...
    0x7B9FEE  cmp eax, 11h
    0x7B9FF3  mov eax, [ecx+1D84h]
    0x7B9FF9  test eax, eax
    ...
    0x7BA007  cmp dword ptr [ecx+1D98h], 63h

FACT：+1E4C and +1D84 are pointer-tested in type branches; +1D98 is compared as a scalar.

### VA 0x7C1F40 / RVA 0x3C1F40 — +1D84 lifetime/update use

    0x7C1F91  mov ecx, [edi+1D84h]
    0x7C1F97  test ecx, ecx
    0x7C1FA4  mov eax, [ecx]
    0x7C1FA6  call dword ptr [eax+44h]
    ...
    0x7C1FB9  mov dword ptr [edi+1D84h], 0
    ...
    0x7C212B  mov [edi+1D84h], eax

主审独立回读 Lua 注册，记录在 artifacts/repentogon/tx-04-08-static/st3-st5-review/main-subplayer-gridposition.json：

    0x84FEBC  push sub_6D86D0
    0x84FEC1  push string GetSubPlayer (string VA 0xB22E24)
    0x6D86D0  mov eax, [ecx+1D84h]
    0x6D86D6  retn

FACT：原生 Lua API 名称 GetSubPlayer 且 getter 直接返回 Player+0x1D84；SubPlayer 字段名称与偏移可确认。其关系目标在各生命周期阶段是否仍具有采集资格仍 UNKNOWN。

### +1D98 disproval

At VA 0x7BA007 / RVA 0x3BA007, target code compares [Player+0x1D98] with 0x63; additional references observed in numeric compare/add contexts include 0x77A487, 0x78960C, and 0x7C7319. FACT：do not treat target +1D98 as a pointer. Old J460 reference offset +1D98 cannot be carried forward.

## Roster size and SDK layout excerpts

IDA PlayerManager::GetPlayer VA 0x98F930 / RVA 0x58F930 reads vector begin and end, computes their byte difference divided by four (0x98F93C-0x98F943) and checks caller index against that size (0x98F95E-0x98F965). Capacity is not the element count.

SDK excerpts (target root F:/projects_isaac/REPENTOGON-1.1.3/libzhl/functions/):

    EntityList.zhl:52-59
    EntityList_EL _mainEL          : 0x20;
    EntityList_EL _persistentEL    : 0x30;
    EntityList_EL _updateEL         : 0x40;
    EntityList_EL _renderEL         : 0x50;
    EntityList_EL _effectEL         : 0x60;
    EntityList_EL _tempEL           : 0x70;
    EntityList_EL _bufferTemplateEL : 0x80;
    EntityList_EL _wispEL           : 0x90;
    EntityList.zhl:61  EntityList size 0x400

    EntityList_EL.zhl (tail)
    bool _sublist       : 0x0;
    Entity** _data      : 0x4;
    unsigned _capacity : 0x8;
    unsigned _size     : 0xc;
    EntityList_EL size 0x10

    PlayerManager.zhl:70-74
    vector_EntityPlayerPtr _playerList : 0x0;
    Entity_Player* _esauJrState[4] : 0x80;
    RNG _rng : 0xb8;
    PlayerManager size 0xc8

    Game.zhl:290
    PlayerManager _playerManager : 0x1baa8;
    Global.zhl:82
    typedef std::vector<Entity_Player*> vector_EntityPlayerPtr;
    ExtraTypes:197-200
    vector_EntityPlayerPtr Size 12; Align 4;

    Room.zhl:182-187
    uint32_t _gridWidth : 0xC;
    uint32_t _gridHeight : 0x10;
    GridEntity* _gridEntities[0x1BF] : 0x24;
    GridEntity* _doors[8] : 0x724;
    uint32_t _doorGridPositions[8] : 0x744;
    int _gridPaths[456] : 0x76c;

    EntityPlayer.zhl:830-831
    EntityPtr _twinPlayer : 0x1e4c;
    Entity_Player* _backupPlayer : 0x1e50;
    EntityPlayer.zhl:861
    Entity_Player* _replacedPlayer : 0x267c;

EntitySaveStateManagement.cpp:223-251 walks roster, player->_backupPlayer, and four _esauJrState entries; this is evidence for separate serialized/retained sources, not evidence that each pointer is currently an active world entity.

## Existing prototype source excerpt locations

cs-t0/native/src/world_record.cpp (historical J460-derived prototype under review):

    1444-1448  kPlayer fields include four Encoding::Reference offsets:
               0x1D98, 0x1E68, 0x1E6C, 0x26C8
    1551-1588  CopySource:
               EL descriptor is 16 bytes; vector descriptor is 12;
               size/capacity/pointer alignment/order, budget and overflow checks;
               pointer slots copied and then rechecked; descriptor reread.
    1723       for i in 0..7 CopySource(i, Room + 0x1218 + 0x20 + i*0x10)
    1724       source 8 = Game + 0x1BAA8 vector roster
    1725-1729  source 9 = four pointers at Game + 0x1BB28 + i*4
    1734-1742  source 10 follows four old relation offsets on Player-kind objects
    1744-1749  source 11 scans 448 Grid slots; source 12 scans 8 Door slots
    1757-1760  source index used for old grid position formula (width-only guard)

These are locations in the old prototype, not evidence that its offsets are valid for J273.

## Remaining UNKNOWN items for the integrated report

1. Raw Entity world Position versus GridPosition-derived value/render position correspondence.
2. Esau/backup/replaced/SubPlayer candidate pointer safety and current eligibility in each lifecycle state; presence in a source alone is not a birth/active proof.
3. Constructor memset is broad contiguous evidence. The 448 Grid conclusion rests principally on the three independent inclusive max-index checks/direct +0x24 accessors.
