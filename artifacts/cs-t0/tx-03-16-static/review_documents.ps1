param([string]$OutputPath = (Join-Path $PSScriptRoot 'final-static-review.json'))
$ErrorActionPreference = 'Stop'
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$reviewOutput=[IO.Path]::GetFullPath($OutputPath)
if(Test-Path -LiteralPath $reviewOutput){throw 'Use a new output path; preserve earlier results.'}
$checks=[Collections.Generic.List[object]]::new()
function Record-Check([string]$Name,[bool]$Pass,[string]$Detail){
    $checks.Add([pscustomobject]@{name=$Name;pass=$Pass;detail=$Detail})
}
function Read-Hex([string]$Value){[Convert]::ToInt64($Value.Substring(2),16)}
$newDocuments=@('docs/research/tx-03-16-independent-sprite-display-fields.md',
    'docs/research/tx-03-16-primary-review.md','docs/research/tx-03-16-luna-sprite-fields.md')
$entryDocuments=@('docs/README.md','docs/hooks/README.md','docs/hooks/entity.md','docs/design/state-synchronization.md')
foreach($relative in ($newDocuments+$entryDocuments)){
    $path=Join-Path $repository $relative
    $exists=Test-Path -LiteralPath $path -PathType Leaf
    Record-Check "document:$relative" $exists 'Document exists.'
    if(-not $exists){continue}
    $body=Get-Content -LiteralPath $path -Raw -Encoding utf8
    if($relative -in $entryDocuments){
        $body=(($body -split "`n")|Where-Object {$_ -match 'tx-03-16'}) -join "`n"
        Record-Check "entry:$relative" ($body.Length -gt 0) 'Updated entry includes this tx; historical entries retained.'
    }
    foreach($match in [regex]::Matches($body,'\[[^\]]*\]\(([^)]+)\)')){
        $target=$match.Groups[1].Value.Trim('<','>')
        if($target -match '^[a-zA-Z]+://' -or $target.StartsWith('#')){continue}
        $target=($target -split '#')[0]
        $absolute=[IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($path)) $target))
        Record-Check "link:$relative->$target" (($absolute -eq $reviewOutput) -or (Test-Path -LiteralPath $absolute)) 'Local reference resolves; final artifact is created at completion.'
    }
    $pairs=@()
    foreach($pattern in @('`0x([0-9a-fA-F]+)`\s*（(?:RVA\s*)?`0x([0-9a-fA-F]+)`）',
        'VA\s*`0x([0-9a-fA-F]+)`[，,]\s*RVA\s*`0x([0-9a-fA-F]+)`',
        '`0x([0-9a-fA-F]+)\s*/\s*0x([0-9a-fA-F]+)`')){
        $pairs+=@([regex]::Matches($body,$pattern))
    }
    $valid=$true
    foreach($pair in $pairs){
        $va=[Convert]::ToInt64($pair.Groups[1].Value,16)
        $rva=[Convert]::ToInt64($pair.Groups[2].Value,16)
        $valid=$valid -and (($va-0x400000) -eq $rva)
    }
    Record-Check "document-addresses:$relative" $valid "Explicit VA/RVA pairs checked: $($pairs.Count). Fields/indices are not RVAs."
}
$allJson=@{}
foreach($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.json' -File)){
    $value=Get-Content -LiteralPath $file.FullName -Raw -Encoding utf8|ConvertFrom-Json -Depth 100
    $allJson[$file.Name]=$value
    Record-Check "json:$($file.Name)" $true 'Readable JSON.'
    if($value.evidence -ne 'STATIC_IDB_READ_ONLY'){continue}
    $valid=($value.imagebase -eq '0x400000') -and ($value.tx -eq 'tx-03-16') -and ($value.functions.Count -gt 0)
    foreach($fn in $value.functions){
        if($fn.error -or $fn.decompile_error -or $fn.failure -or $fn.pseudocode_failure -or -not $fn.va){$valid=$false}
        if($fn.va){$valid=$valid -and (((Read-Hex $fn.va)-0x400000) -eq (Read-Hex $fn.rva))}
        foreach($insn in $fn.instructions){
            $valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))
        }
        foreach($chunk in $fn.raw_chunk_bytes){
            $valid=$valid -and (-not $chunk.failure) -and ($null -ne $chunk.bytes)
            $valid=$valid -and (((Read-Hex $chunk.start_va)-0x400000) -eq (Read-Hex $chunk.start_rva))
        }
    }
    foreach($range in $value.ranges){
        foreach($insn in $range.instructions){
            $valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))
        }
    }
    Record-Check "addresses:$($file.Name)" $valid 'Actual function starts, instructions and child chunks have consistent VA/RVA. Interior requests are classified separately.'
}
foreach($name in @('primary-animation-initial.json','primary-named-animation.json','primary-state-selection.json',
    'primary-animation-rebind.json','primary-animation-binding.json','primary-draw-review.json',
    'primary-property-review.json','primary-color-composition.json','primary-acquisition-notes.json','luna-draw-fields.json')){
    Record-Check "evidence:$name" ($allJson.ContainsKey($name)) 'Required static evidence/acquisition record exists.'
}
foreach($name in @('export_static.py','luna-export-sprite-fields.py')){
    Record-Check "source:$name" (Test-Path -LiteralPath (Join-Path $PSScriptRoot $name) -PathType Leaf) 'Read-only extraction source supplied; not executed by this review.'
}
$interior=@(@('primary-state-selection.json','0x408930','0x408830'),
    @('primary-animation-rebind.json','0x407d30','0x407ae0'))
foreach($item in $interior){
    $fn=$allJson[$item[0]].functions|Where-Object requested_va -eq $item[1]
    Record-Check "classified-interior:$($item[1])" ($fn.va -eq $item[2]) 'Requested internal address is not a separate callable entry. Actual enclosing function retained in raw evidence.'
}
$anchors=@(
    @('primary-state-selection.json','0x40a030','0x40a058','8b 4a 18'),
    @('primary-state-selection.json','0x40a030','0x40a05e','03 cb'),
    @('primary-state-selection.json','0x40a030','0x40a066','e8 b5 f0 ff ff'),
    @('primary-state-selection.json','0x40a030','0x40a06f','83 c3 10'),
    @('primary-named-animation.json','0x40a690','0x40a6c0','e8 3b e7 ff ff'),
    @('primary-named-animation.json','0x40a690','0x40a6cc','c6 46 44 00'),
    @('primary-named-animation.json','0x40abb0','0x40abdf','e8 1c e2 ff ff'),
    @('primary-named-animation.json','0x40a7e0','0x40a81f','8b 74 b0 38'),
    @('primary-named-animation.json','0x40a7e0','0x40a837','e8 84 6a 60 00'),
    @('primary-named-animation.json','0x40a7e0','0x40a84d','c7 04 b0 00 00 00 00'),
    @('primary-animation-initial.json','0x408830','0x408935','c7 46 10 00 00 80 bf'),
    @('primary-animation-initial.json','0x408830','0x40893c','c6 46 14 00'),
    @('primary-animation-initial.json','0x408830','0x408940','c7 46 18 00 00 00 00'),
    @('primary-animation-initial.json','0x408830','0x408947','c7 46 1c 00 00 00 00'),
    @('primary-animation-initial.json','0x408e00','0x408e81','f3 0f 10 55 fc'),
    @('primary-animation-initial.json','0x408e00','0x408ec4','f3 0f 10 55 fc'),
    @('primary-animation-initial.json','0x408c90','0x408ce1','89 4e 18'),
    @('primary-animation-initial.json','0x408c90','0x408cf0','89 4e 1c'),
    @('primary-state-selection.json','0x40a5d0','0x40a668','e8 c3 e1 ff ff'),
    @('primary-state-selection.json','0x40a5d0','0x40a678','e8 f3 e2 ff ff'),
    @('primary-animation-rebind.json','0x408970','0x408a62','e8 78 7b 6e 00'),
    @('primary-animation-rebind.json','0x408970','0x408b2a','e8 b0 7a 6e 00'),
    @('primary-animation-rebind.json','0x408970','0x408b63','c7 46 0c 00 00 00 00'),
    @('primary-animation-rebind.json','0x408970','0x408b6a','89 5e 08'),
    @('primary-draw-review.json','0x4098a0','0x409af0','89 3d 14 39 c3 00'),
    @('primary-draw-review.json','0x4098a0','0x409b7d','a3 14 39 c3 00')
)
foreach($anchor in $anchors){
    $fn=$allJson[$anchor[0]].functions|Where-Object va -eq $anchor[1]
    $insn=$fn.instructions|Where-Object va -eq $anchor[2]
    Record-Check "instruction:$($anchor[2])" ($insn.bytes -eq $anchor[3]) "Reviewed raw instruction in $($anchor[0])."
}
$sharedFunctions=@(
    @('primary-draw-review.json','0x409120'),@('primary-draw-review.json','0x4098a0'),
    @('primary-draw-review.json','0x409cb0'),@('primary-animation-initial.json','0x40a0d0'),
    @('primary-animation-initial.json','0x40a140'),@('primary-state-selection.json','0x40a030'),
    @('primary-state-selection.json','0x40a080'))
foreach($va in @('0x49cb40','0x456340','0x417200','0x420170','0x417210','0x43ee80',
    '0x407410','0x4073d0','0x407420','0x435c60','0x407430','0x505b50','0x4071c0','0x40c6f0')){
    $sharedFunctions+=,@('primary-property-review.json',$va)
}
foreach($item in $sharedFunctions){
    $primaryFn=$allJson[$item[0]].functions|Where-Object va -eq $item[1]
    $childFn=$allJson['luna-draw-fields.json'].functions|Where-Object va -eq $item[1]
    $primaryRaw=($primaryFn.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    $childRaw=($childFn.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    Record-Check "independent-function:$($item[1])" (($primaryRaw.Length -gt 0) -and ($primaryRaw -eq $childRaw)) 'Direct raw instruction comparison only; no hash. Semantic review is separate.'
}
$failures=@($checks|Where-Object {-not $_.pass})
$result=[ordered]@{
    tx='tx-03-16';date='2026-10-23';timezone='Asia/Shanghai'
    result=$(if($failures.Count -eq 0){'PASS_STATIC_DOCUMENT_REVIEW_ONLY'}else{'FAIL_STATIC_DOCUMENT_REVIEW'})
    execution='One final unified static document review after all research, child review and edits; no other tests or experiments.'
    agent=@{model='gpt-6-luna';reasoning_effort='max';role='Draw fields and special flags researched by child and independently reviewed by primary'}
    semantic_review='Primary read actual animation/Lua dispatch, reset and non-reset array handling, target-frame events, layer mapping, raw track receiver, draw branches, property accessors and external rendering state; proposed a bounded frozen-pose description rather than implemented schema.'
    limitations=@('No runtime resources, draw, pixels, leak, corruption or crash observed',
        'Interior requests classified at actual enclosing starts; unrelated swap not used as an entry',
        'Static anomaly paths do not prove actual runtime occurrence',
        'Indirect targets, constructor defaults, OOM, EH, reentry and complete rendering environment remain limited',
        'No Replica, wire schema, capture or resource/pose adapter implemented',
        'No hash computation or verification')
    checks=$checks.ToArray();failures=$failures
}
$stream=[IO.FileStream]::new($reviewOutput,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
try{$writer.Write(($result|ConvertTo-Json -Depth 20))}finally{$writer.Dispose()}
[pscustomobject]@{result=$result.result;checks=$checks.Count;failures=$failures.Count;output=$reviewOutput}|ConvertTo-Json
if($failures.Count -ne 0){exit 1}
