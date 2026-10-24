param([string]$OutputPath = (Join-Path $PSScriptRoot 'final-static-review.json'))
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$reviewOutput = [IO.Path]::GetFullPath($OutputPath)
if (Test-Path -LiteralPath $reviewOutput) { throw 'Use a new output path; original reviews are preserved.' }
$checks = [Collections.Generic.List[object]]::new()
function Record-Check([string]$Name, [bool]$Pass, [string]$Detail) {
    $checks.Add([pscustomobject]@{name=$Name; pass=$Pass; detail=$Detail})
}
$newDocuments = @(
    'docs/research/tx-03-11-logic-presentation-separation.md',
    'docs/research/tx-03-11-primary-review.md',
    'docs/research/tx-03-11-luna-render-findings.md'
)
$entryDocuments = @('docs/README.md','docs/hooks/README.md','docs/hooks/entity.md',
    'docs/hooks/game-loop.md','docs/design/state-synchronization.md')
foreach ($relative in ($newDocuments + $entryDocuments)) {
    $path = Join-Path $repository $relative
    $exists = Test-Path -LiteralPath $path -PathType Leaf
    Record-Check "document:$relative" $exists 'Final delivery document exists.'
    if (-not $exists) { continue }
    $body = Get-Content -LiteralPath $path -Raw -Encoding utf8
    if ($relative -in $entryDocuments) {
        $body = (($body -split "`n") | Where-Object { $_ -match 'tx-03-11' }) -join "`n"
        Record-Check "index:$relative" ($body.Length -gt 0) 'Contains a tx-03-11 update.'
    }
    foreach ($match in [regex]::Matches($body, '\[[^\]]*\]\(([^)]+)\)')) {
        $target = $match.Groups[1].Value.Trim('<','>')
        if ($target -match '^[a-zA-Z]+://' -or $target.StartsWith('#')) { continue }
        $target = ($target -split '#')[0]
        $absolute = [IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($path)) $target))
        $valid = ($absolute -eq $reviewOutput) -or (Test-Path -LiteralPath $absolute)
        Record-Check "link:$relative->$target" $valid 'Local reference resolves; current review output is created at completion.'
    }
}
$primary = @{}
foreach ($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.json' -File)) {
    $value = Get-Content -LiteralPath $file.FullName -Raw -Encoding utf8 | ConvertFrom-Json -Depth 100
    Record-Check "json:$($file.Name)" $true 'JSON is readable; historical raw tool errors remain evidence, not target-program outcomes.'
    if ($file.Name.StartsWith('primary-')) {
        $primary[$file.Name] = $value
        $valid = $value.imagebase -eq '0x400000' -and $value.evidence -eq 'STATIC_IDB_READ_ONLY'
        foreach ($fn in $value.functions) {
            if ($fn.error -or $fn.decompile_error) { $valid = $false }
            if ($fn.va) {
                $valid = $valid -and (([Convert]::ToInt64($fn.va.Substring(2),16) - 0x400000) -eq [Convert]::ToInt64($fn.rva.Substring(2),16))
            }
            foreach ($insn in $fn.instructions) {
                $valid = $valid -and (([Convert]::ToInt64($insn.va.Substring(2),16) - 0x400000) -eq [Convert]::ToInt64($insn.rva.Substring(2),16))
            }
        }
        Record-Check "static-addresses:$($file.Name)" $valid 'IDA static label, imagebase, function/VA/RVA data and decompilation status agree.'
    }
}
$anchors = @(
    @('primary-animation.json','0x408c90','0x408ce1','89 4e 18'),
    @('primary-animation.json','0x408c90','0x408cf0','89 4e 1c'),
    @('primary-animation-consumers.json','0x409030','0x40905e','f3 0f 58 80 04 01 00 00'),
    @('primary-event-gameplay.json','0x40add0','0x40ae61','85 41 18'),
    @('primary-event-gameplay.json','0x40ae70','0x40af01','85 41 1c'),
    @('primary-event-gameplay.json','0x5583d0','0x5584b0','e8 1b 29 eb ff'),
    @('primary-event-gameplay.json','0x5583d0','0x55867d','e8 5e 2c 17 00'),
    @('primary-shoot-callee.json','0x6cb2e0','0x6cb394','e8 87 d7 d5 ff'),
    @('primary-spawn-wrapper.json','0x428b20','0x428b3d','e8 ce 58 2d 00'),
    @('primary-entity-main-render.json','0x4199d0','0x419a2e','ff 50 14'),
    @('primary-entity-main-render.json','0x4199d0','0x419b11','ff 50 14'),
    @('primary-room-render.json','0x80f200','0x810d0d','ff 50 14')
)
foreach ($anchor in $anchors) {
    $fn = $primary[$anchor[0]].functions | Where-Object va -eq $anchor[1]
    $insn = $fn.instructions | Where-Object va -eq $anchor[2]
    Record-Check "instruction:$($anchor[2])" ($insn.bytes -eq $anchor[3]) "Exported instruction supports the reviewed claim in $($anchor[0])."
}
$table = $primary['primary-entity-vtable.json'].tables[0]
foreach ($item in @(@('0x14','0x6b10f0','f0 10 6b 00'),@('0x18','0x6b2000','00 20 6b 00'))) {
    $entry = $table.entries | Where-Object offset -eq $item[0]
    Record-Check "vtable:$($item[0])" ($table.va -eq '0xb6730c' -and $entry.target_va -eq $item[1] -and $entry.bytes -eq $item[2]) 'Base table entry and target are explicitly recorded, not inferred for derived classes.'
}
$failures = @($checks | Where-Object { -not $_.pass })
$result = [ordered]@{
    tx='tx-03-11'; date='2026-10-22'; timezone='Asia/Shanghai'
    result=$(if ($failures.Count -eq 0) {'PASS_STATIC_DOCUMENT_REVIEW_ONLY'} else {'FAIL_STATIC_DOCUMENT_REVIEW'})
    execution='Single final static document review; no game, build, unit test, E2E, injection or debugger operation.'
    agent=@{model='gpt-6-luna';reasoning_effort='max';role='Render static research, independently reviewed by primary agent'}
    semantic_review='Primary agent read pseudocode, instruction context, vtable entries and receiver/parameter data; script checks document and evidence consistency only.'
    limitations=@('Complete native presentation safety remains UNKNOWN','Derived render/resource targets and Lua callback effects not closed','No Replica or baseline/delta adapter implemented','No runtime observations or restoration claim','No file or state hashing performed')
    checks=$checks.ToArray(); failures=$failures
}
$stream = [IO.FileStream]::new($reviewOutput,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$writer = [IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
try { $writer.Write(($result | ConvertTo-Json -Depth 20)) } finally { $writer.Dispose() }
[pscustomobject]@{result=$result.result;checks=$checks.Count;failures=$failures.Count;output=$reviewOutput} | ConvertTo-Json
if ($failures.Count -ne 0) { exit 1 }
