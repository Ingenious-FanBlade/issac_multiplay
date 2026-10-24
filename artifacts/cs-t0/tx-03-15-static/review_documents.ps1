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
$newDocuments=@('docs/research/tx-03-15-sprite-replacement-reload.md',
    'docs/research/tx-03-15-primary-review.md','docs/research/tx-03-15-luna-sprite-replacement.md')
$entryDocuments=@('docs/README.md','docs/hooks/README.md','docs/hooks/entity.md','docs/design/state-synchronization.md')
foreach($relative in ($newDocuments+$entryDocuments)){
    $path=Join-Path $repository $relative
    $exists=Test-Path -LiteralPath $path -PathType Leaf
    Record-Check "document:$relative" $exists 'Document exists.'
    if(-not $exists){continue}
    $body=Get-Content -LiteralPath $path -Raw -Encoding utf8
    if($relative -in $entryDocuments){
        $body=(($body -split "`n")|Where-Object {$_ -match 'tx-03-15'}) -join "`n"
        Record-Check "entry:$relative" ($body.Length -gt 0) 'Updated entry includes this research; historical entries are not rewritten.'
    }
    foreach($match in [regex]::Matches($body,'\[[^\]]*\]\(([^)]+)\)')){
        $target=$match.Groups[1].Value.Trim('<','>')
        if($target -match '^[a-zA-Z]+://' -or $target.StartsWith('#')){continue}
        $target=($target -split '#')[0]
        $absolute=[IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($path)) $target))
        Record-Check "link:$relative->$target" (($absolute -eq $reviewOutput) -or (Test-Path -LiteralPath $absolute)) 'Local reference resolves; result artifact is created at completion.'
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
    Record-Check "document-addresses:$relative" $valid "Explicit VA/RVA pairs checked: $($pairs.Count). Object offsets are not RVAs."
}
$allJson=@{}
foreach($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.json' -File)){
    $value=Get-Content -LiteralPath $file.FullName -Raw -Encoding utf8|ConvertFrom-Json -Depth 100
    $allJson[$file.Name]=$value
    Record-Check "json:$($file.Name)" $true 'Readable JSON.'
    if($value.evidence -ne 'STATIC_IDB_READ_ONLY'){continue}
    $valid=($value.imagebase -eq '0x400000') -and ($value.tx -eq 'tx-03-15') -and ($value.functions.Count -gt 0)
    foreach($fn in $value.functions){
        if($fn.error -or $fn.decompile_error -or $fn.pseudocode_error -or -not $fn.va){$valid=$false}
        if($fn.va){$valid=$valid -and (((Read-Hex $fn.va)-0x400000) -eq (Read-Hex $fn.rva))}
        foreach($insn in $fn.instructions){
            $valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))
        }
    }
    foreach($range in $value.ranges){
        foreach($insn in $range.instructions){
            $valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))
        }
    }
    foreach($site in $value.registration_focus_sites){
        $valid=$valid -and (((Read-Hex $site.va)-0x400000) -eq (Read-Hex $site.rva))
        foreach($insn in $site.context){
            $valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))
        }
    }
    Record-Check "addresses:$($file.Name)" $valid 'Static function/instruction and registration-context VA/RVA agree. No pointer-table assumptions needed.'
}
foreach($name in @('primary-reload-initial.json','primary-anm-cache.json','primary-replacement-flow.json',
    'primary-loader-failure-context.json','primary-binding-context.json','primary-lua-wrappers.json',
    'primary-acquisition-notes.json','luna-sprite-replacement.json','luna-sprite-path-compare.json')){
    Record-Check "evidence:$name" ($allJson.ContainsKey($name)) 'Required evidence/acquisition record exists.'
}
foreach($name in @('export_static.py','luna-export-sprite-replacement.py','luna-export-sprite-path-compare.py')){
    Record-Check "source:$name" (Test-Path -LiteralPath (Join-Path $PSScriptRoot $name) -PathType Leaf) 'Read-only acquisition source supplied; not executed by this review.'
}
$anchors=@(
    @('primary-reload-initial.json','0x40bea0','0x40beab','e8 e0 1c 00 00'),
    @('primary-reload-initial.json','0x40bea0','0x40beb1','e8 5a 22 00 00'),
    @('primary-reload-initial.json','0x40bea0','0x40beb9','e8 42 01 00 00'),
    @('primary-reload-initial.json','0x40bd50','0x40bd60','e8 4b 25 00 00'),
    @('primary-reload-initial.json','0x40bd50','0x40bd83','e8 08 1e 00 00'),
    @('primary-reload-initial.json','0x40bd50','0x40bd89','e8 82 23 00 00'),
    @('primary-reload-initial.json','0x40bd50','0x40bda1','e8 8a ca ff ff'),
    @('primary-reload-initial.json','0x40bd50','0x40bdab','e8 80 ca ff ff'),
    @('primary-reload-initial.json','0x40bd50','0x40bdb2','c6 86 09 01 00 00 01'),
    @('primary-reload-initial.json','0x407f10','0x407f3d','e8 6e 63 00 00'),
    @('primary-reload-initial.json','0x40db90','0x40dde3','80 7e 14 00'),
    @('primary-reload-initial.json','0x40db90','0x40ddf9','e8 52 0f 00 00'),
    @('primary-reload-initial.json','0x40db90','0x40de85','68 60 7a 40 00'),
    @('primary-reload-initial.json','0x40db90','0x40df52','89 47 7c'),
    @('primary-reload-initial.json','0x40db90','0x40df91','e8 7a a3 ff ff'),
    @('primary-reload-initial.json','0x40db90','0x40e02e','e8 8d 32 60 00'),
    @('primary-reload-initial.json','0x40c000','0x40c043','80 7c 03 30 00'),
    @('primary-reload-initial.json','0x40c000','0x40c048','0f 85 61 01 00 00'),
    @('primary-reload-initial.json','0x40c000','0x40c0c9','e8 72 c5 ff ff'),
    @('primary-reload-initial.json','0x40c000','0x40c13b','e8 00 c5 ff ff'),
    @('primary-reload-initial.json','0x40c000','0x40c146','88 4c 03 30'),
    @('primary-reload-initial.json','0x40c000','0x40c14d','c6 44 03 31 00'),
    @('primary-reload-initial.json','0x40c000','0x40c1b0','81 c3 a0 00 00 00'),
    @('primary-anm-cache.json','0x40bf30','0x40bf87','e8 54 0b 00 00'),
    @('primary-anm-cache.json','0x40bf30','0x40bfb3','c6 44 08 30 00'),
    @('primary-anm-cache.json','0x40e110','0x40e26e','ff 46 50'),
    @('primary-anm-cache.json','0x40e2b0','0x40e42c','89 46 50'),
    @('primary-anm-cache.json','0x40e470','0x40e4a0','83 7e 50 00'),
    @('primary-anm-cache.json','0x40e470','0x40e4bb','e8 90 f3 ff ff'),
    @('primary-binding-context.json','0x89ef10','0x89ef1f','68 d0 99 8a 00'),
    @('primary-binding-context.json','0x89ef10','0x89ef25','c7 00 30 bf 40 00'),
    @('primary-binding-context.json','0x89f030','0x89f048','68 40 97 8a 00'),
    @('primary-lua-wrappers.json','0x8a99d0','0x8a9a40','ff 15 e4 83 b1 00'),
    @('primary-lua-wrappers.json','0x8a99d0','0x8a9ac8','ff 15 f4 83 b1 00'),
    @('primary-lua-wrappers.json','0x8a99d0','0x8a9b10','ff d6'),
    @('primary-lua-wrappers.json','0x8a99d0','0x8a9b36','b8 01 00 00 00'),
    @('primary-lua-wrappers.json','0x8a9740','0x8a9788','ff d0'),
    @('primary-lua-wrappers.json','0x8a9740','0x8a978b','33 c0'),
    @('primary-replacement-flow.json','0x4098a0','0x409af0','89 3d 14 39 c3 00'),
    @('primary-replacement-flow.json','0x4098a0','0x409b7d','a3 14 39 c3 00')
)
foreach($anchor in $anchors){
    $fn=$allJson[$anchor[0]].functions|Where-Object va -eq $anchor[1]
    $insn=$fn.instructions|Where-Object va -eq $anchor[2]
    Record-Check "instruction:$($anchor[2])" ($insn.bytes -eq $anchor[3]) "Reviewed raw instruction in $($anchor[0])."
}
$registration=@(@('0x86730a','68 a0 be 40 00'),@('0x86730f','e8 1c 7d 03 00'),
    @('0x86731f','68 30 bf 40 00'),@('0x867324','e8 e7 7b 03 00'),
    @('0x867334','68 f0 bf 40 00'),@('0x867339','e8 f2 7c 03 00'))
$childRegistration=$allJson['luna-sprite-replacement.json'].functions|Where-Object va -eq '0x866960'
foreach($site in $registration){
    $insn=$allJson['primary-reload-initial.json'].ranges.instructions|Where-Object va -eq $site[0]
    $childInsn=$childRegistration.instructions|Where-Object va -eq $site[0]
    Record-Check "registration:$($site[0])" ($insn.bytes -eq $site[1]) 'Named native binding instruction.'
    Record-Check "independent-registration:$($site[0])" ($insn.bytes -eq $childInsn.bytes) 'Independent primary range and child full-function bytes agree.'
}
$sharedFunctions=@(
    @('primary-anm-cache.json','luna-sprite-replacement.json','0x40bf30'),
    @('primary-reload-initial.json','luna-sprite-replacement.json','0x40bff0'),
    @('primary-reload-initial.json','luna-sprite-replacement.json','0x40c000'),
    @('primary-replacement-flow.json','luna-sprite-replacement.json','0x408640'),
    @('primary-replacement-flow.json','luna-sprite-replacement.json','0x408590'),
    @('primary-replacement-flow.json','luna-sprite-replacement.json','0x40c3b0'),
    @('primary-replacement-flow.json','luna-sprite-path-compare.json','0x40cae0'))
foreach($item in $sharedFunctions){
    $primaryFn=$allJson[$item[0]].functions|Where-Object va -eq $item[2]
    $childFn=$allJson[$item[1]].functions|Where-Object va -eq $item[2]
    $primaryRaw=($primaryFn.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    $childRaw=($childFn.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    Record-Check "independent-function:$($item[2])" (($primaryRaw.Length -gt 0) -and ($primaryRaw -eq $childRaw)) 'Direct raw instruction comparison only, no hash; semantic reasoning was reviewed separately.'
}
$failures=@($checks|Where-Object {-not $_.pass})
$result=[ordered]@{
    tx='tx-03-15';date='2026-10-23';timezone='Asia/Shanghai'
    result=$(if($failures.Count -eq 0){'PASS_STATIC_DOCUMENT_REVIEW_ONLY'}else{'FAIL_STATIC_DOCUMENT_REVIEW'})
    execution='One final unified static document review after all research and edits. No game, build, code tests, injection or debugger operation.'
    agent=@{model='gpt-6-luna';reasoning_effort='max';role='Sprite replacement researched by child and independently reviewed/corrected by primary'}
    semantic_review='Primary read registration and actual Lua closures, native ABI, bytewise comparison, candidate versus assigned pair state, separate pair owners, ANM bind/parse/count/reset/sweep and normal failure branches, plus bounded draw geometry. Script checks artifact consistency only.'
    revisions=@('Actual Lua closures found beyond the initial child scope','Separate old destination and by-value source cleanup','Candidate status does not validate final pair retain')
    limitations=@('No runtime resources, leak, UAF, crash, pixels, thread ID or HGLRC observed',
        'Conditional count and failed-Load borrowed-state derivations are not universal runtime findings',
        'Zero-layer, OOM, EH, invalid control, full XML failure and concurrency not closed',
        'Draw RNG gameplay uses unknown','No Replica, resource adapter or native rollback implemented',
        'No hash computation or verification')
    checks=$checks.ToArray();failures=$failures
}
$stream=[IO.FileStream]::new($reviewOutput,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
try{$writer.Write(($result|ConvertTo-Json -Depth 20))}finally{$writer.Dispose()}
[pscustomobject]@{result=$result.result;checks=$checks.Count;failures=$failures.Count;output=$reviewOutput}|ConvertTo-Json
if($failures.Count -ne 0){exit 1}
