param([string]$OutputPath=(Join-Path $PSScriptRoot 'final-static-review.json'))
$ErrorActionPreference='Stop'
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$reviewOutput=[IO.Path]::GetFullPath($OutputPath)
if(Test-Path -LiteralPath $reviewOutput){throw 'Preserve previous results; do not rerun this acceptance.'}
$checks=[Collections.Generic.List[object]]::new()
function Record-Check([string]$Name,[bool]$Pass,[string]$Detail){
    $checks.Add([pscustomobject]@{name=$Name;pass=$Pass;detail=$Detail})
}
function Read-Hex([string]$Value){[Convert]::ToInt64($Value.Substring(2),16)}
function Read-ExplicitClaims([string]$Body){
    $plain=$Body.Replace('**','').Replace('`','')
    $claims=[Collections.Generic.List[object]]::new()
    foreach($pattern in @('0x([0-9a-fA-F]+)\s*（RVA\s*0x([0-9a-fA-F]+)',
        '0x([0-9a-fA-F]+)\s*[/／]\s*RVA\s*0x([0-9a-fA-F]+)',
        'VA\s*0x([0-9a-fA-F]+)[，,]\s*RVA\s*0x([0-9a-fA-F]+)')){
        foreach($m in [regex]::Matches($plain,$pattern)){
            $claims.Add([pscustomobject]@{va=('0x'+$m.Groups[1].Value);rva=('0x'+$m.Groups[2].Value)})
        }
    }
    $vaColumn=-1;$rvaColumn=-1;$compactColumns=@();$headers=@()
    foreach($line in ($plain -split "`n")){
        if(-not $line.Trim().StartsWith('|')){$vaColumn=-1;$rvaColumn=-1;$compactColumns=@();$headers=@();continue}
        $cells=@($line.Trim().Trim('|').Split('|')|ForEach-Object {$_.Trim()})
        $newVa=-1;$newRva=-1;$newCompact=@()
        for($i=0;$i -lt $cells.Count;$i++){
            if($cells[$i] -eq 'VA'){$newVa=$i}
            if($cells[$i] -eq 'RVA'){$newRva=$i}
            if($cells[$i] -match '\bVA\s*[/／]\s*RVA\b'){$newCompact+=,$i}
        }
        if(($newVa -ge 0 -and $newRva -ge 0) -or $newCompact.Count -gt 0){
            $headers=$cells;$vaColumn=$newVa;$rvaColumn=$newRva;$compactColumns=$newCompact;continue
        }
        if($headers.Count -ne $cells.Count){continue}
        if($vaColumn -ge 0 -and $rvaColumn -ge 0){
            $v=[regex]::Match($cells[$vaColumn],'0x[0-9a-fA-F]+');$r=[regex]::Match($cells[$rvaColumn],'0x[0-9a-fA-F]+')
            if($v.Success -and $r.Success){$claims.Add([pscustomobject]@{va=$v.Value;rva=$r.Value})}
        }
        foreach($column in $compactColumns){
            foreach($m in [regex]::Matches($cells[$column],'0x([0-9a-fA-F]+)\s*[/／]\s*0x([0-9a-fA-F]+)')){
                $claims.Add([pscustomobject]@{va=('0x'+$m.Groups[1].Value);rva=('0x'+$m.Groups[2].Value)})
            }
        }
    }
    $claims.ToArray()
}
$newDocuments=@('docs/research/tx-03-20-sprite-resource-pose-closure.md',
    'docs/research/tx-03-20-primary-review.md','docs/research/tx-03-20-luna-pose-capture-update.md')
$entryDocuments=@('docs/README.md','docs/hooks/README.md','docs/hooks/entity.md','docs/hooks/game-loop.md','docs/design/state-synchronization.md')
foreach($relative in ($newDocuments+$entryDocuments)){
    $path=Join-Path $repository $relative
    $exists=Test-Path -LiteralPath $path -PathType Leaf
    Record-Check "document:$relative" $exists 'Document exists.'
    if(-not $exists){continue}
    $body=Get-Content -LiteralPath $path -Raw -Encoding utf8
    if($relative -in $entryDocuments){
        $body=(($body -split "`n")|Where-Object {$_ -match 'tx-03-20'}) -join "`n"
        Record-Check "entry:$relative" ($body.Length -gt 0) 'Current addition present; historical entries retained.'
    }
    foreach($m in [regex]::Matches($body,'\[[^\]]*\]\(([^)]+)\)')){
        $target=$m.Groups[1].Value.Trim('<','>')
        if($target -match '^[a-zA-Z]+://' -or $target.StartsWith('#')){continue}
        $target=($target -split '#')[0]
        $absolute=[IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($path)) $target))
        Record-Check "link:$relative->$target" (($absolute -eq $reviewOutput) -or (Test-Path -LiteralPath $absolute)) 'Local reference resolves; current result written at completion.'
    }
    $claims=@(Read-ExplicitClaims $body);$valid=$true
    foreach($claim in $claims){$valid=$valid -and (((Read-Hex $claim.va)-0x400000) -eq (Read-Hex $claim.rva))}
    Record-Check "document-addresses:$relative" $valid "Explicit VA/RVA pairs: $($claims.Count). Unlabeled field offsets and slash-separated VA lists are excluded."
}
$allJson=@{}
foreach($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.json' -File)){
    try{$value=Get-Content -LiteralPath $file.FullName -Raw -Encoding utf8|ConvertFrom-Json -Depth 100}
    catch{Record-Check "json:$($file.Name)" $false $_.Exception.Message;continue}
    $allJson[$file.Name]=$value
    Record-Check "json:$($file.Name)" $true 'Readable JSON.'
    if($value.evidence -ne 'STATIC_IDB_READ_ONLY'){continue}
    $valid=($value.imagebase -eq '0x400000') -and ($value.tx -eq 'tx-03-20') -and ($value.functions.Count -gt 0)
    foreach($fn in $value.functions){
        if($fn.error -or $fn.decompile_error -or -not $fn.va){$valid=$false}
        if($fn.va){$valid=$valid -and (((Read-Hex $fn.va)-0x400000) -eq (Read-Hex $fn.rva))}
        foreach($insn in $fn.instructions){$valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))}
    }
    foreach($range in $value.ranges){
        foreach($insn in $range.instructions){$valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))}
    }
    foreach($table in $value.tables){
        $valid=$valid -and (((Read-Hex $table.va)-0x400000) -eq (Read-Hex $table.rva))
        foreach($entry in $table.entries){$valid=$valid -and (((Read-Hex $entry.target_va)-0x400000) -eq (Read-Hex $entry.target_rva))}
    }
    foreach($record in (@($value.xref_records)+@($value.symbols))){
        if($record.va -and $record.rva){$valid=$valid -and (((Read-Hex $record.va)-0x400000) -eq (Read-Hex $record.rva))}
    }
    Record-Check "raw-addresses:$($file.Name)" $valid 'Functions resolved without recorded export errors; raw function/instruction/range/table addresses agree.'
}
$required=@('primary-resource-binding.json','primary-sample-parse-detail.json','primary-pose-owner-review.json',
    'primary-png-profile.json','primary-capture-source.json','primary-capture-source-v2.json',
    'primary-acquisition-notes.json','sample-resource-observation.json','sample-support-declaration.json','luna-pose-capture.json')
foreach($name in $required){Record-Check "evidence:$name" ($allJson.ContainsKey($name)) 'Required evidence, declaration or preserved acquisition record exists.'}
foreach($name in @('export_static.py','read_resource_metadata.ps1','luna-export-pose-capture.py')){
    Record-Check "source:$name" (Test-Path -LiteralPath (Join-Path $PSScriptRoot $name) -PathType Leaf) 'Read-only acquisition source supplied; not executed by this review.'
}
$counts=@{'primary-resource-binding.json'=7;'primary-sample-parse-detail.json'=9;
    'primary-pose-owner-review.json'=10;'primary-png-profile.json'=7;'luna-pose-capture.json'=20}
foreach($name in $counts.Keys){
    $functions=@($allJson[$name].functions)
    $unique=@($functions.va|Sort-Object -Unique)
    Record-Check "actual-function-count:$name" (($functions.Count -eq $counts[$name]) -and ($unique.Count -eq $counts[$name])) 'Actual unique function starts; not dynamic coverage.'
}
$original=$allJson['primary-capture-source.json']
$oldLoop=@($original.sources[0].sections)
$preserved=($oldLoop.Count -eq 2) -and ($oldLoop[0].start_line -eq 1020) -and ($oldLoop[1].start_line -eq 1038)
foreach($section in $oldLoop){$preserved=$preserved -and ($null -eq $section.end_line) -and ($section.lines.Count -eq 0)}
Record-Check 'preserved-source-acquisition-error' $preserved 'Initial flattened loop ranges retained; not adopted as complete source evidence.'
$snapshot=$allJson['primary-capture-source-v2.json']
Record-Check 'source-correction-declared' (($snapshot.evidence -eq 'STATIC_SOURCE_READ_ONLY') -and ($snapshot.tx -eq 'tx-03-20') -and ($snapshot.acquisition_correction.Length -gt 0) -and ($snapshot.sources.Count -eq 3)) 'V2 explicitly identifies source acquisition correction.'
foreach($source in $snapshot.sources){
    $path=Join-Path $repository $source.path
    $exists=Test-Path -LiteralPath $path -PathType Leaf
    Record-Check "source-file:$($source.path)" $exists 'Referenced source exists.'
    if(-not $exists){continue}
    $current=Get-Content -LiteralPath $path -Encoding utf8
    foreach($section in $source.sections){
        $valid=$section.lines.Count -eq ($section.end_line-$section.start_line+1)
        foreach($line in $section.lines){$valid=$valid -and ($current[$line.line-1] -ceq $line.text)}
        Record-Check "source-excerpt:$($source.path):$($section.start_line)-$($section.end_line)" $valid 'One-based excerpt directly compared with source; no hash.'
    }
}
$anchors=@(
    @('primary-resource-binding.json','0x40ed50','0x40ef89','e8 e2 06 27 00'),
    @('primary-resource-binding.json','0x40ed50','0x411eda','68 70 b5 b1 00'),
    @('primary-sample-parse-detail.json','0x67f670','0x67f717','80 38 5c'),
    @('primary-sample-parse-detail.json','0x67f670','0x67f71c','c6 00 2f'),
    @('primary-sample-parse-detail.json','0x67f670','0x67f7d7','80 38 5c'),
    @('primary-sample-parse-detail.json','0x67f670','0x67f7dc','c6 00 2f'),
    @('primary-sample-parse-detail.json','0xa64a50','0xa64eda','c7 43 28 00 00 00 00'),
    @('primary-sample-parse-detail.json','0xa64a50','0xa64f01','c7 43 28 08 00 00 00'),
    @('primary-png-profile.json','0xa123b0','0xa123b0','8b 41 08'),
    @('primary-png-profile.json','0xa123b0','0xa123b3','83 e0 01'),
    @('primary-png-profile.json','0xa123d0','0xa123d0','8b 41 08'),
    @('primary-png-profile.json','0xa123d0','0xa123d3','83 e0 02'),
    @('primary-png-profile.json','0xa12420','0xa12420','83 79 28 00'),
    @('primary-png-profile.json','0xa12420','0xa12424','0f 94 c0'),
    @('primary-pose-owner-review.json','0x931050','0x9312ec','e8 df a3 07 00'),
    @('primary-pose-owner-review.json','0x931050','0x9312f1','e8 ca 42 02 00'),
    @('primary-pose-owner-review.json','0x6b10f0','0x6b120b','8d 4e 48'),
    @('primary-pose-owner-review.json','0x6b10f0','0x6b124d','e8 7e 8e d5 ff'),
    @('primary-pose-owner-review.json','0x6b2000','0x6b2106','8d 7e 48'),
    @('primary-pose-owner-review.json','0x6b2000','0x6b2153','8d b7 a8 00 00 00'),
    @('primary-pose-owner-review.json','0x6b2000','0x6b215d','c6 43 74 01'),
    @('primary-pose-owner-review.json','0x6b2000','0x6b21c2','e8 79 7f d5 ff'),
    @('primary-pose-owner-review.json','0x6b2000','0x6b21f5','c6 43 74 00'),
    @('primary-sample-parse-detail.json','0x40a5d0','0x40a668','e8 c3 e1 ff ff'),
    @('primary-sample-parse-detail.json','0x40a5d0','0x40a678','e8 f3 e2 ff ff'),
    @('primary-sample-parse-detail.json','0x40aaf0','0x40ab88','e8 a3 dc ff ff'),
    @('primary-sample-parse-detail.json','0x408830','0x408935','c7 46 10 00 00 80 bf'),
    @('primary-sample-parse-detail.json','0x408830','0x40893c','c6 46 14 00'),
    @('primary-sample-parse-detail.json','0x408830','0x408940','c7 46 18 00 00 00 00'),
    @('primary-sample-parse-detail.json','0x408830','0x408947','c7 46 1c 00 00 00 00'),
    @('primary-sample-parse-detail.json','0x40a7e0','0x40a84d','c7 04 b0 00 00 00 00'),
    @('primary-sample-parse-detail.json','0x40a030','0x40a060','ff 34 b0'),
    @('primary-sample-parse-detail.json','0x409120','0x409188','3b 41 08'),
    @('primary-sample-parse-detail.json','0x409120','0x409191','6b c0 6c'),
    @('primary-sample-parse-detail.json','0x409120','0x409194','03 41 04'),
    @('primary-sample-parse-detail.json','0x409120','0x4091af','03 7a 7c'),
    @('primary-sample-parse-detail.json','0x409120','0x4091b8','80 7f 74 00')
)
foreach($anchor in $anchors){
    $fn=@($allJson[$anchor[0]].functions|Where-Object va -eq $anchor[1])
    $insn=@($fn.instructions|Where-Object va -eq $anchor[2])
    Record-Check "instruction:$($anchor[2])" (($fn.Count -eq 1) -and ($insn.Count -eq 1) -and ($insn[0].bytes -eq $anchor[3])) "Reviewed raw instruction in $($anchor[0])."
}
$shared=@(@('primary-sample-parse-detail.json','0x408b90'),@('primary-sample-parse-detail.json','0x40a030'),
    @('primary-sample-parse-detail.json','0x409120'),@('primary-sample-parse-detail.json','0x40a5d0'),
    @('primary-sample-parse-detail.json','0x40aaf0'),@('primary-sample-parse-detail.json','0x408830'),
    @('primary-sample-parse-detail.json','0x40a7e0'),@('primary-pose-owner-review.json','0x409100'),
    @('primary-pose-owner-review.json','0x409030'))
foreach($item in $shared){
    $primary=$allJson[$item[0]].functions|Where-Object va -eq $item[1]
    $child=$allJson['luna-pose-capture.json'].functions|Where-Object va -eq $item[1]
    $p=($primary.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    $c=($child.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    Record-Check "independent-function:$($item[1])" (($p.Length -gt 0) -and ($p -ceq $c)) 'Independent raw exports directly compared; semantic review recorded in primary report, no hash.'
}
$pngTable=@($allJson['primary-png-profile.json'].tables|Where-Object va -eq '0xb9ff84')
$tableValid=($pngTable.Count -eq 1) -and ($pngTable[0].entries.Count -eq 24)
if($tableValid){
    for($i=0;$i -lt 24;$i++){$tableValid=$tableValid -and ((Read-Hex $pngTable[0].entries[$i].offset) -eq ($i*4))}
    $tableValid=$tableValid -and ($pngTable[0].entries[0].target_va -eq '0xa123b0') -and
        ($pngTable[0].entries[1].target_va -eq '0xa123d0') -and ($pngTable[0].entries[2].target_va -eq '0xa12420')
}
Record-Check 'png-property-table' $tableValid 'Exactly 24 contiguous slots; loaded, bit1 and indexed have distinct targets.'
$observation=$allJson['sample-resource-observation.json']
Record-Check 'sample-observation-scope' (($observation.evidence -eq 'STATIC_LOCAL_FILES_READ_ONLY') -and ($observation.tx -eq 'tx-03-20')) 'File data observation, no native/runtime claim.'
foreach($property in $observation.source_paths.PSObject.Properties){
    Record-Check "sample-source:$($property.Name)" (Test-Path -LiteralPath $property.Value -PathType Leaf) 'Existing resource source path; no load/decode/CRC/hash.'
}
$anm=$observation.anm
Record-Check 'xml-top-level-profile' (($anm.info.Version -eq '105') -and ($anm.info.Fps -eq '30') -and
    ($anm.spritesheets.Count -eq 1) -and ($anm.layers.Count -eq 1) -and ($anm.animations.Count -eq 1) -and
    ($anm.default_animation -ceq 'Poof') -and ($anm.null_count -eq 0) -and ($anm.event_count -eq 0)) 'Observed XML metadata only; not native shape.'
Record-Check 'xml-layer-sheet-association' (($anm.layers[0].Id -eq '0') -and ($anm.layers[0].Name -ceq 'head') -and
    ($anm.layers[0].SpritesheetId -eq '0') -and ($anm.spritesheets[0].Id -eq '0') -and
    ($anm.spritesheets[0].Path -ceq 'effects\Effect_003_BloodTearStatic.png')) 'One XML layer/sheet association with exact source spelling.'
$animation=$anm.animations[0]
Record-Check 'xml-animation-profile' (($animation.attributes.Name -ceq 'Poof') -and ($animation.attributes.FrameNum -eq '15') -and
    ($animation.attributes.Loop -eq 'false') -and ($animation.root_frames.Count -eq 1) -and
    ($animation.tracks.Count -eq 1) -and ($animation.null_track_count -eq 0) -and ($animation.trigger_count -eq 0)) 'Single animation/root/visual track, no nulls/triggers.'
$root=$animation.root_frames[0]
$rootExpected=@{XPosition='0';YPosition='0';XScale='100';YScale='100';Delay='15';Visible='true';
    RedTint='255';GreenTint='255';BlueTint='255';AlphaTint='255';RedOffset='0';GreenOffset='0';BlueOffset='0';Rotation='0';Interpolated='false'}
$valid=$true
foreach($key in $rootExpected.Keys){$valid=$valid -and ($root.$key -ceq $rootExpected[$key])}
Record-Check 'xml-constant-root' $valid 'Observed root attributes; interpolation/root adjustment still requires actual native inspection.'
$track=$animation.tracks[0]
Record-Check 'xml-track-profile' (($track.attributes.LayerId -eq '0') -and ($track.attributes.Visible -eq 'true') -and
    ($track.xml_frame_count -eq 15) -and ($track.frames.Count -eq 15) -and ($track.delay_sum -eq 15)) 'XML frame count and XML Delay sum, not runtime key count.'
$frameExpected=@{XPosition='-32';YPosition='-32';XPivot='0';YPivot='0';Width='64';Height='64';XScale='100';YScale='100';
    Delay='1';Visible='true';RedTint='255';GreenTint='255';BlueTint='255';AlphaTint='255';RedOffset='0';GreenOffset='0';BlueOffset='0';Rotation='0';Interpolated='false'}
for($i=0;$i -lt $track.frames.Count;$i++){
    $frame=$track.frames[$i];$attributes=$frame.attributes
    $valid=($frame.xml_frame_ordinal -eq $i) -and ($frame.start -eq $i) -and ($frame.end -eq ($i+1)) -and
        ([int]$attributes.XCrop -eq (($i%4)*64)) -and ([int]$attributes.YCrop -eq ([Math]::Floor($i/4)*64))
    foreach($key in $frameExpected.Keys){$valid=$valid -and ($attributes.$key -ceq $frameExpected[$key])}
    Record-Check "xml-frame:$i" $valid 'Observed frame attributes and rectangle sequence agree with report; not native decode.'
}
$png=$observation.png
Record-Check 'png-file-header-only' (($png.header_byte_count -eq 29) -and
    ($png.header_bytes -eq '89 50 4e 47 0d 0a 1a 0a 00 00 00 0d 49 48 44 52 00 00 01 00 00 00 01 00 08 06 00 00 00') -and
    ($png.width -eq 256) -and ($png.height -eq 256) -and ($png.bit_depth -eq 8) -and ($png.color_type -eq 6) -and
    ($png.compression_method -eq 0) -and ($png.filter_method -eq 0) -and ($png.interlace -eq 0)) 'Saved 29-byte header observation; no CRC, texel decode, driver upload or pixel proof.'
$config=$observation.entity_config
Record-Check 'config-file-association-only' (($config.root.anm2root -ceq 'gfx/') -and ($config.entry.id -eq '1000') -and
    ($config.entry.variant -eq '200') -and ($config.entry.anm2path -ceq '1000.011_Bullet Poof Static.anm2') -and ($config.entry_line -eq 5336)) 'XML association; no new Entity or lifecycle support.'
$declaration=$allJson['sample-support-declaration.json'];$shape=$declaration.expected_local_resource_shape
Record-Check 'support-is-design' (($declaration.evidence -eq 'DESIGN_PROPOSAL') -and ($declaration.tx -eq 'tx-03-20') -and
    ($declaration.logical_resource_key -ceq 'tx0320.poof.static.v1') -and ($declaration.resource_revision -eq 1) -and
    ($declaration.file_observation -eq 'sample-resource-observation.json') -and ($declaration.native_shape_rule -match 'not native loading')) 'Manual local name/revision and proposed native shape are explicitly separated from observation.'
Record-Check 'declaration-paths' (($declaration.configured_anm_name -ceq ('gfx/'+$config.entry.anm2path)) -and
    ($declaration.xml_spritesheet_path -ceq $anm.spritesheets[0].Path) -and
    ($declaration.derived_image_name -ceq 'gfx/effects/Effect_003_BloodTearStatic.png')) 'Configured name and derived relative image path retain exact case.'
Record-Check 'declaration-bounded-shape' (($shape.layer_count -eq 1) -and ($shape.layer_name -ceq 'head') -and ($shape.main_animation -ceq 'Poof') -and
    ($shape.xml_frame_count -eq 15) -and ($shape.native_visual_track_count_proposed -eq 1) -and ($shape.native_key_count_proposed -eq 15) -and
    ($null -eq $shape.overlay) -and ($shape.png_width -eq 256) -and ($shape.png_height -eq 256) -and
    ($shape.png_file_color_type -eq 6) -and ($shape.runtime_png_indexed -eq $false)) 'Native count and non-indexed state are proposed candidate conditions, not observed outputs.'
$pose=$declaration.explicit_initial_pose
Record-Check 'explicit-local-pose' (($pose.main_selected_keys.Count -eq 1) -and ($pose.main_selected_keys[0] -eq 0) -and
    ($pose.overlay_selected_keys.Count -eq 0) -and ($pose.main_and_overlay_playing -eq $false) -and ($pose.native_event_masks -eq 0) -and
    ($pose.native_frame_marker -eq -1) -and (($pose.sprite_color_11_f32 -join ',') -eq '1,1,1,1,0,0,0,0,0,0,0') -and
    (($pose.sprite_scale -join ',') -eq '1,1') -and ($pose.layer_visible -eq $true)) 'Explicit fixture defaults; no reliance on unknown process-global construction values.'
Record-Check 'update-and-rebuild-design' (($declaration.continuous_update -match 'publish local pose revision last') -and
    ($declaration.continuous_update -match 'No Update/SetFrame/SetLayerFrame') -and ($declaration.candidate_rebuild -match 'prepare outside final draw')) 'Same-resource complete apply and fresh-candidate branch are declared, not implemented.'
$notes=$allJson['primary-acquisition-notes.json']
Record-Check 'acquisition-issues-preserved' (($notes.preserved_acquisition_issues.Count -eq 3) -and
    ($notes.unused_inference -match 'not adopted') -and ($notes.scope -match 'One final static document review only')) 'rg failure, malformed source excerpt, prior search scope and rejected GL-helper inference remain recorded.'
$failures=@($checks|Where-Object {-not $_.pass})
$result=[ordered]@{
    tx='tx-03-20';date='2026-10-23';timezone='Asia/Shanghai'
    result=$(if($failures.Count -eq 0){'PASS_STATIC_DOCUMENT_REVIEW_ONLY'}else{'FAIL_STATIC_DOCUMENT_REVIEW'})
    execution='One final unified static document review after resource/source/IDA research, Luna review and all edits. No intermediate tests, builds or experiments.'
    agent=@{model='gpt-6-luna';reasoning_effort='max';role='Authority capture and continuous pose update research reviewed by primary'}
    semantic_review='Primary reviewed same-Capture source timing before FinishBatch, selected-key draw inputs and setter effects, same-descriptor complete apply versus fresh candidate, parser key expansion, relative path and runtime resolver distinction, PNG loaded/bit1/indexed slots, Entity shadow temporary appearance and owner/context/retirement limitations. Algorithms and support declaration remain proposals.'
    acquisition_anomalies=@('Initial Windows rg world* path error retained in notes',
        'Initial source snapshot loop ranges flattened and empty; retained, explicit-range v2 adopted',
        'Prior resources-only search did not cover existing sibling extracted_resources',
        'A18750 renderbuffer/FBO storage was investigated but not adopted as generic GL restoration')
    limitations=@('File metadata and IDB are static observations, no disk/IDB identity assertion or hashes',
        'No actual resolver hit, loaded native descriptor/key count, native PNG state or texel decode observed',
        'No adapter, fixture source, pose capture, registry, callback, Hook or wire implementation',
        'No game/debugger, native/OS helper, resource load, build, runtime thread/context, pixels, GPU or E2E',
        'Persistent single-Sprite pose does not imply complete Entity visuals, room occlusion or OOM/EH/worker/hotreload closure',
        'Acquisition scripts and historical acceptance entries not executed; failure is retained without automatic rerun')
    checks=$checks.ToArray();failures=$failures
}
$stream=[IO.FileStream]::new($reviewOutput,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
try{$writer.Write(($result|ConvertTo-Json -Depth 20))}finally{$writer.Dispose()}
[pscustomobject]@{result=$result.result;checks=$checks.Count;failures=$failures.Count;output=$reviewOutput}|ConvertTo-Json
if($failures.Count -ne 0){exit 1}
