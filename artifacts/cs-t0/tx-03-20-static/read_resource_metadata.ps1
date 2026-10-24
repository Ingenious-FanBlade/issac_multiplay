# Static local-file metadata acquisition only. No native loader, image decode,
# CRC/hash, rendering, expected-result assertions or pass/fail decisions.
param([string]$OutputPath=(Join-Path $PSScriptRoot 'sample-resource-observation.json'))
$ErrorActionPreference='Stop'
$assetRoot='F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth/extracted_resources/resources'
$anmPath=Join-Path $assetRoot 'gfx/1000.011_bullet poof static.anm2'
$pngPath=Join-Path $assetRoot 'gfx/effects/effect_003_bloodtearstatic.png'
$entitiesPath=Join-Path $assetRoot 'entities2.xml'
function Read-Attributes([System.Xml.XmlNode]$Node){
    $result=[ordered]@{}
    if($Node){foreach($attribute in $Node.Attributes){$result[$attribute.Name]=$attribute.Value}}
    return $result
}
function Read-U32BE([byte[]]$Bytes,[int]$Offset){
    return [uint64]$Bytes[$Offset]*16777216+[uint64]$Bytes[$Offset+1]*65536+[uint64]$Bytes[$Offset+2]*256+[uint64]$Bytes[$Offset+3]
}
$anm=[System.Xml.XmlDocument]::new()
$anm.Load($anmPath)
$entities=[System.Xml.XmlDocument]::new()
$entities.Load($entitiesPath)
$entity=$entities.SelectSingleNode('/entities/entity[@id="1000" and @variant="200"]')
$entityLine=Select-String -LiteralPath $entitiesPath -Pattern 'name="Bullet Poof \(Static\)"'
$header=[byte[]]::new(29)
$pngStream=[IO.File]::OpenRead($pngPath)
try{$headerRead=$pngStream.Read($header,0,$header.Length)}finally{$pngStream.Dispose()}
$animations=@(foreach($animation in $anm.SelectNodes('/AnimatedActor/Animations/Animation')){
    $tracks=@(foreach($track in $animation.SelectNodes('./LayerAnimations/LayerAnimation')){
        $end=0L;$key=0
        $frames=@(foreach($frame in $track.SelectNodes('./Frame')){
            $start=$end;$end+=[long]$frame.GetAttribute('Delay')
            [ordered]@{xml_frame_ordinal=$key;start=$start;end=$end;attributes=(Read-Attributes $frame)}
            $key++
        })
        [ordered]@{attributes=(Read-Attributes $track);xml_frame_count=$frames.Count;frames=$frames;delay_sum=$end}
    })
    [ordered]@{attributes=(Read-Attributes $animation);root_frames=@($animation.SelectNodes('./RootAnimation/Frame')|ForEach-Object {Read-Attributes $_});tracks=$tracks;null_track_count=$animation.SelectNodes('./NullAnimations/NullAnimation').Count;trigger_count=$animation.SelectNodes('./Triggers/*').Count}
})
$result=[ordered]@{
    tx='tx-03-20';evidence='STATIC_LOCAL_FILES_READ_ONLY';date='2026-10-23';timezone='Asia/Shanghai'
    source_paths=[ordered]@{anm=$anmPath;png=$pngPath;entity_config=$entitiesPath}
    anm=[ordered]@{
        info=(Read-Attributes $anm.SelectSingleNode('/AnimatedActor/Info'))
        spritesheets=@($anm.SelectNodes('/AnimatedActor/Content/Spritesheets/Spritesheet')|ForEach-Object {Read-Attributes $_})
        layers=@($anm.SelectNodes('/AnimatedActor/Content/Layers/Layer')|ForEach-Object {Read-Attributes $_})
        null_count=$anm.SelectNodes('/AnimatedActor/Content/Nulls/*').Count
        event_count=$anm.SelectNodes('/AnimatedActor/Content/Events/*').Count
        default_animation=$anm.SelectSingleNode('/AnimatedActor/Animations').GetAttribute('DefaultAnimation')
        animations=$animations
    }
    png=[ordered]@{header_byte_count=$headerRead;header_bytes=($header|ForEach-Object {$_.ToString('x2')}) -join ' ';width=(Read-U32BE $header 16);height=(Read-U32BE $header 20);bit_depth=$header[24];color_type=$header[25];compression_method=$header[26];filter_method=$header[27];interlace=$header[28]}
    entity_config=[ordered]@{root=(Read-Attributes $entities.DocumentElement);entry=(Read-Attributes $entity);entry_line=$entityLine.LineNumber;role='Local config association only; not native Entity lifecycle support'}
    limitations=@('Read existing extracted resources only; did not unpack or copy assets',
        'XML frame ordinals are not asserted to equal native runtime key indices',
        'Native parser may adjust keys for interpolation and root frames',
        'Disk paths and metadata do not establish runtime resource resolution or cross-machine equality',
        'No target/native helper, PNG decode, CRC/hash or tests executed')
}
$stream=[IO.FileStream]::new($OutputPath,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
try{$writer.Write(($result|ConvertTo-Json -Depth 22))}finally{$writer.Dispose()}
[ordered]@{output=$OutputPath;animation_count=$animations.Count;layer_count=$result.anm.layers.Count;xml_frame_counts=@($animations.tracks.xml_frame_count);png=$result.png}|ConvertTo-Json -Depth 4
