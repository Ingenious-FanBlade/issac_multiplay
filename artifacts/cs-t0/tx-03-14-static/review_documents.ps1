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
$newDocuments=@('docs/research/tx-03-14-renderer-context-lifecycle.md',
    'docs/research/tx-03-14-primary-review.md','docs/research/tx-03-14-luna-renderer-teardown.md')
$entryDocuments=@('docs/README.md','docs/hooks/README.md','docs/hooks/game-loop.md','docs/design/state-synchronization.md')
foreach($relative in ($newDocuments+$entryDocuments)){
    $path=Join-Path $repository $relative
    $exists=Test-Path -LiteralPath $path -PathType Leaf
    Record-Check "document:$relative" $exists 'Document exists.'
    if(-not $exists){continue}
    $body=Get-Content -LiteralPath $path -Raw -Encoding utf8
    if($relative -in $entryDocuments){
        $body=(($body -split "`n")|Where-Object {$_ -match 'tx-03-14'}) -join "`n"
        Record-Check "entry:$relative" ($body.Length -gt 0) 'Updated entry includes this research.'
    }
    foreach($match in [regex]::Matches($body,'\[[^\]]*\]\(([^)]+)\)')){
        $target=$match.Groups[1].Value.Trim('<','>')
        if($target -match '^[a-zA-Z]+://' -or $target.StartsWith('#')){continue}
        $target=($target -split '#')[0]
        $absolute=[IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($path)) $target))
        Record-Check "link:$relative->$target" (($absolute -eq $reviewOutput) -or (Test-Path -LiteralPath $absolute)) 'Local reference resolves; the result is created at completion.'
    }
    $pairs=@()
    foreach($pattern in @('`0x([0-9a-fA-F]+)`\s*（(?:RVA\s*)?`0x([0-9a-fA-F]+)`）',
                          'VA\s*`0x([0-9a-fA-F]+)`[，,]\s*RVA\s*`0x([0-9a-fA-F]+)`')){
        $pairs+=@([regex]::Matches($body,$pattern))
    }
    $valid=$true
    foreach($pair in $pairs){
        $va=[Convert]::ToInt64($pair.Groups[1].Value,16)
        $rva=[Convert]::ToInt64($pair.Groups[2].Value,16)
        $valid=$valid -and (($va-0x400000) -eq $rva)
    }
    Record-Check "document-addresses:$relative" $valid "Explicit VA/RVA pairs checked: $($pairs.Count). Historical typo descriptions are retained as text, not address claims."
}
$allJson=@{}
foreach($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.json' -File)){
    $value=Get-Content -LiteralPath $file.FullName -Raw -Encoding utf8|ConvertFrom-Json -Depth 100
    $allJson[$file.Name]=$value
    Record-Check "json:$($file.Name)" $true 'Readable JSON; acquisition notes are not target-program tests.'
    if($value.evidence -ne 'STATIC_IDB_READ_ONLY'){continue}
    $valid=$value.imagebase -eq '0x400000'
    foreach($fn in $value.functions){
        if($fn.error -or $fn.decompile_error){$valid=$false}
        if($fn.va){$valid=$valid -and (((Read-Hex $fn.va)-0x400000) -eq (Read-Hex $fn.rva))}
        foreach($insn in $fn.instructions){
            $valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))
        }
    }
    foreach($table in $value.tables){
        $valid=$valid -and (((Read-Hex $table.va)-0x400000) -eq (Read-Hex $table.rva))
        foreach($entry in $table.entries){
            if($file.Name -eq 'primary-platform-install.json' -and $table.va -eq '0xb4e880' -and $entry.offset -eq '0x0'){
                Record-Check 'mixed-platform-data:ID' (($entry.target_va -eq '0x60001') -and ($entry.bytes -eq '01 00 06 00')) 'First word is an ID, not a VA. Its mechanically generated negative target_rva is retained as an acquisition-format anomaly, never treated as an address.'
                continue
            }
            $valid=$valid -and (((Read-Hex $entry.target_va)-0x400000) -eq (Read-Hex $entry.target_rva))
        }
    }
    foreach($range in $value.table_ranges){
        foreach($entry in $range.entries){
            $valid=$valid -and (((Read-Hex $entry.va)-0x400000) -eq (Read-Hex $entry.rva))
        }
    }
    Record-Check "addresses:$($file.Name)" $valid 'Static functions/instructions and pointer-table VA/RVA agree. Child raw data ranges are not assumed to contain only function pointers.'
}
foreach($name in @('primary-context-initial.json','primary-context-paths.json','primary-wgl-current.json',
    'primary-wgl-install.json','primary-render-alternate.json','primary-window-dispatch.json',
    'primary-platform-install.json','primary-teardown-review.json','primary-cache-drain.json',
    'primary-manager-tables-review.json','primary-acquisition-notes.json',
    'luna-renderer-teardown.json','luna-window-close-chain.json')){
    Record-Check "evidence:$name" ($allJson.ContainsKey($name)) 'Required raw evidence or acquisition record is present.'
}
foreach($name in @('export_static.py','luna-export-teardown.py','luna-export-close-chain.py')){
    Record-Check "source:$name" (Test-Path -LiteralPath (Join-Path $PSScriptRoot $name) -PathType Leaf) 'Read-only extraction source supplied; not executed by this review.'
}
$anchors=@(
    @('primary-context-paths.json','0x931050','0x931155','e8 86 df 12 00'),
    @('primary-context-paths.json','0x931050','0x931206','e8 35 8e 07 00'),
    @('primary-context-paths.json','0x931050','0x9312f1','e8 ca 42 02 00'),
    @('primary-context-paths.json','0x931050','0x9313de','e8 1d a3 07 00'),
    @('primary-context-paths.json','0x931050','0x93140e','e8 5d 03 14 00'),
    @('primary-context-paths.json','0xa710a0','0xa71284','e8 07 7c fa ff'),
    @('primary-context-paths.json','0xa19180','0xa19312','8b 86 30 02 00 00'),
    @('primary-context-initial.json','0xa19df0','0xa1a046','ff 15 58 82 b1 00'),
    @('primary-context-initial.json','0xa19df0','0xa1a096','8b 86 2c 02 00 00'),
    @('primary-wgl-current.json','0xa7fa50','0xa7fa67','ff 15 58 5d c7 00'),
    @('primary-wgl-current.json','0xa7fa50','0xa7fa96','ff 15 00 82 b1 00'),
    @('primary-wgl-current.json','0xa7fa50','0xa7faaa','ff 15 58 5d c7 00'),
    @('primary-wgl-current.json','0xa7fb00','0xa7fb73','ff 15 88 81 b1 00'),
    @('primary-wgl-install.json','0xa6a420','0xa6a4be','b9 00 00 06 00'),
    @('primary-wgl-install.json','0xa80010','0xa804b0','c7 86 2c 02 00 00 50 fa a7 00'),
    @('primary-wgl-install.json','0xa80010','0xa804bf','c7 86 30 02 00 00 00 fb a7 00'),
    @('primary-wgl-install.json','0xa80010','0xa804e7','c7 86 40 02 00 00 b0 fc a7 00'),
    @('primary-platform-install.json','0xa81050','0xa81190','c7 85 54 ff ff ff 90 d3 a5 00'),
    @('primary-render-alternate.json','0x8fb120','0x8fc32c','e8 8f 92 05 00'),
    @('primary-window-dispatch.json','0x9aa040','0x9aaae3','e8 38 06 f5 ff'),
    @('primary-teardown-review.json','0xa13060','0xa13182','f6 05 84 79 c3 00 01'),
    @('primary-teardown-review.json','0xa13060','0xa13194','74 48'),
    @('primary-teardown-review.json','0xa13060','0xa131a1','c7 05 3c 16 c7 00 00 00 00 00'),
    @('primary-teardown-review.json','0xa13060','0xa131b3','e8 48 26 a0 ff'),
    @('primary-teardown-review.json','0xa19130','0xa19133','e8 28 9f ff ff'),
    @('primary-teardown-review.json','0xa19130','0xa19169','e8 22 1e 05 00'),
    @('primary-teardown-review.json','0xa71770','0xa71a48','e8 13 16 fa ff'),
    @('primary-teardown-review.json','0xa71770','0xa71a7a','e8 11 95 ff ff'),
    @('primary-teardown-review.json','0xa6af90','0xa6afec','ff 15 58 82 b1 00'),
    @('primary-teardown-review.json','0xa6af90','0xa6b03e','8b 80 2c 02 00 00'),
    @('primary-teardown-review.json','0xa6af90','0xa6b04c','ff 15 0c 37 c7 00'),
    @('primary-teardown-review.json','0xa5d4e0','0xa5d4fe','56'),
    @('primary-teardown-review.json','0xa5d4e0','0xa5d4ff','ff d0'),
    @('primary-teardown-review.json','0xa5d4e0','0xa5d549','ff 15 e8 86 b1 00'),
    @('primary-teardown-review.json','0xa7fcb0','0xa7fcc2','ff 15 48 5d c7 00'),
    @('primary-teardown-review.json','0xa7fcb0','0xa7fcc8','c7 86 68 02 00 00 00 00 00 00')
)
foreach($anchor in $anchors){
    $fn=$allJson[$anchor[0]].functions|Where-Object va -eq $anchor[1]
    $insn=$fn.instructions|Where-Object va -eq $anchor[2]
    Record-Check "instruction:$($anchor[2])" ($insn.bytes -eq $anchor[3]) "Reviewed raw instruction in $($anchor[0])."
    if($anchor[0] -eq 'primary-teardown-review.json'){
        $childName=if($anchor[1] -in @('0xa13060','0xa19130','0xa71770')){'luna-renderer-teardown.json'}else{'luna-window-close-chain.json'}
        $childFn=$allJson[$childName].functions|Where-Object va -eq $anchor[1]
        $childInsn=$childFn.instructions|Where-Object va -eq $anchor[2]
        Record-Check "independent-instruction:$($anchor[2])" ($childInsn.bytes -eq $insn.bytes) 'Primary independently exported and read the same native close instruction.'
    }
}
$tableEntries=@(@('0xb82600','0x4','0xa18e90','0xb82604'),
    @('0xb82600','0x8','0xa19130','0xb82608'),
    @('0xb82600','0x14','0xa19180','0xb82614'),
    @('0xb818b0','0x8','0xa11730','0xb818b8'))
foreach($expected in $tableEntries){
    $table=$allJson['primary-manager-tables-review.json'].tables|Where-Object va -eq $expected[0]
    $entry=$table.entries|Where-Object offset -eq $expected[1]
    Record-Check "table:$($expected[0])+$($expected[1])" ($entry.target_va -eq $expected[2]) 'Table base and entry address are distinct.'
    $childEntry=$allJson['luna-renderer-teardown.json'].table_ranges|ForEach-Object {$_.entries}|Where-Object va -eq $expected[3]
    Record-Check "independent-table:$($expected[3])" (($childEntry.value -eq $entry.target_va) -and ($childEntry.bytes -eq $entry.bytes)) 'Independent raw manager table entries agree.'
}
$failures=@($checks|Where-Object {-not $_.pass})
$result=[ordered]@{
    tx='tx-03-14';date='2026-10-22';timezone='Asia/Shanghai'
    result=$(if($failures.Count -eq 0){'PASS_STATIC_DOCUMENT_REVIEW_ONLY'}else{'FAIL_STATIC_DOCUMENT_REVIEW'})
    execution='One final unified static document review after all research and edits. No game, build, code tests, injection or debugger operation.'
    agent=@{model='gpt-6-luna';reasoning_effort='max';role='Normal renderer teardown researched by child and independently reviewed by primary'}
    semantic_review='Primary read normal main/init/frame/current-TLS/WGL and shutdown paths, raw register and stack flow, conditional cache drain, other render caller, native source hook scope and manager tables. Script checks static artifact consistency only.'
    acquisition_anomaly='B4E880 first word is a platform ID; raw generic negative target_rva is preserved and explicitly excluded from address claims.'
    limitations=@('Actual thread ID, TLS value and HGLRC not observed','No guarantee of OS GL/WGL/window API success',
        'Cross-thread detach, outstanding aliases, late object types, EH and OOM not closed',
        'Replica, resource queue, context guard and generation handling not implemented','No hash computation or verification')
    checks=$checks.ToArray();failures=$failures
}
$stream=[IO.FileStream]::new($reviewOutput,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
try{$writer.Write(($result|ConvertTo-Json -Depth 20))}finally{$writer.Dispose()}
[pscustomobject]@{result=$result.result;checks=$checks.Count;failures=$failures.Count;output=$reviewOutput}|ConvertTo-Json
if($failures.Count -ne 0){exit 1}
