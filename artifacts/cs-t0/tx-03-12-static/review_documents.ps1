param([string]$OutputPath = (Join-Path $PSScriptRoot 'final-static-review.json'))
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$reviewOutput = [IO.Path]::GetFullPath($OutputPath)
if (Test-Path -LiteralPath $reviewOutput) { throw 'Choose a new output path; preserve previous review results.' }
$checks = [Collections.Generic.List[object]]::new()
function Record-Check([string]$Name,[bool]$Pass,[string]$Detail) {
    $checks.Add([pscustomobject]@{name=$Name;pass=$Pass;detail=$Detail})
}
$newDocuments = @('docs/research/tx-03-12-derived-render-sprite-lifecycle.md',
    'docs/research/tx-03-12-primary-review.md','docs/research/tx-03-12-luna-derived-render.md')
$entryDocuments = @('docs/README.md','docs/hooks/README.md','docs/hooks/entity.md',
    'docs/hooks/game-loop.md','docs/hooks/npc.md','docs/hooks/tear.md',
    'docs/hooks/projectile.md','docs/design/state-synchronization.md')
foreach ($relative in ($newDocuments+$entryDocuments)) {
    $path=Join-Path $repository $relative
    $exists=Test-Path -LiteralPath $path -PathType Leaf
    Record-Check "document:$relative" $exists 'Document exists.'
    if (-not $exists) { continue }
    $body=Get-Content -LiteralPath $path -Raw -Encoding utf8
    if ($relative -in $entryDocuments) {
        $body=(($body -split "`n") | Where-Object {$_ -match 'tx-03-12'}) -join "`n"
        Record-Check "entry:$relative" ($body.Length -gt 0) 'Contains this research update.'
    }
    foreach ($match in [regex]::Matches($body,'\[[^\]]*\]\(([^)]+)\)')) {
        $target=$match.Groups[1].Value.Trim('<','>')
        if ($target -match '^[a-zA-Z]+://' -or $target.StartsWith('#')) {continue}
        $target=($target -split '#')[0]
        $absolute=[IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($path)) $target))
        Record-Check "link:$relative->$target" (($absolute -eq $reviewOutput) -or (Test-Path -LiteralPath $absolute)) 'Local reference resolves; this result is created at completion.'
    }
}
$primary=@{}
foreach ($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.json' -File)) {
    $value=Get-Content -LiteralPath $file.FullName -Raw -Encoding utf8 | ConvertFrom-Json -Depth 100
    Record-Check "json:$($file.Name)" $true 'Readable JSON; raw acquisition errors remain evidence, not target-program tests.'
    if ($file.Name.StartsWith('primary-') -and $value.evidence -eq 'STATIC_IDB_READ_ONLY') {
        $primary[$file.Name]=$value
        $valid=$value.imagebase -eq '0x400000'
        foreach ($fn in $value.functions) {
            if ($fn.error -or $fn.decompile_error) {$valid=$false}
            if ($fn.va) {$valid=$valid -and (([Convert]::ToInt64($fn.va.Substring(2),16)-0x400000) -eq [Convert]::ToInt64($fn.rva.Substring(2),16))}
            foreach ($insn in $fn.instructions) {
                $valid=$valid -and (([Convert]::ToInt64($insn.va.Substring(2),16)-0x400000) -eq [Convert]::ToInt64($insn.rva.Substring(2),16))
            }
        }
        foreach ($table in $value.tables) {
            foreach ($entry in $table.entries) {
                $valid=$valid -and (([Convert]::ToInt64($entry.target_va.Substring(2),16)-0x400000) -eq [Convert]::ToInt64($entry.target_rva.Substring(2),16))
            }
        }
        Record-Check "addresses:$($file.Name)" $valid 'Static metadata, decompilation status, and VA/RVA conversion agree.'
    }
}
foreach ($name in @('primary-derived-tables.json','primary-derived-render.json','primary-render-callbacks.json',
    'primary-render-lua-chain.json','primary-sprite-initial.json','primary-sprite-resource.json',
    'primary-sprite-lifetime.json','primary-sprite-bindings.json','primary-cache-lifecycle.json',
    'primary-resource-release.json','primary-npc-geometry.json')) {
    Record-Check "evidence:$name" ($primary.ContainsKey($name)) 'Required primary evidence present.'
}
$anchors=@(
    @('primary-derived-render.json','0x6d2e50','0x6d4006','e8 95 cb 18 00'),
    @('primary-derived-render.json','0x6790c0','0x6793e7','e8 94 97 1e 00'),
    @('primary-derived-render.json','0x65cb60','0x65cedb','e8 90 57 20 00'),
    @('primary-derived-render.json','0x65cb60','0x65cb84','f3 0f 11 86 50 03 00 00'),
    @('primary-derived-render.json','0x65cb60','0x65ced1','f3 0f 11 86 50 03 00 00'),
    @('primary-render-lua-chain.json','0x872b10','0x872c02','ff 15 d8 83 b1 00'),
    @('primary-render-lua-chain.json','0x873740','0x873832','ff 15 d8 83 b1 00'),
    @('primary-render-callbacks.json','0x862b80','0x862ca5','ff 15 d8 83 b1 00'),
    @('primary-sprite-initial.json','0x40bd50','0x40bdb2','c6 86 09 01 00 00 01'),
    @('primary-sprite-initial.json','0x40bd50','0x40be99','c2 08 00'),
    @('primary-sprite-lifetime.json','0x420160','0x420160','8a 81 09 01 00 00'),
    @('primary-sprite-lifetime.json','0x8bb720','0x8bb728','68 1c 01 00 00'),
    @('primary-sprite-lifetime.json','0x408170','0x408196','e8 75 fd ff ff'),
    @('primary-sprite-resource.json','0x40e110','0x40e26e','ff 46 50'),
    @('primary-sprite-resource.json','0x40e2b0','0x40e42c','89 46 50'),
    @('primary-cache-lifecycle.json','0x40e470','0x40e4a0','83 7e 50 00'),
    @('primary-cache-lifecycle.json','0x40e470','0x40e4bb','e8 90 f3 ff ff')
)
foreach ($anchor in $anchors) {
    $fn=$primary[$anchor[0]].functions | Where-Object va -eq $anchor[1]
    $insn=$fn.instructions | Where-Object va -eq $anchor[2]
    Record-Check "instruction:$($anchor[2])" ($insn.bytes -eq $anchor[3]) "Reviewed instruction in $($anchor[0])."
}
$tableEntries=@(
    @('0xb67468','0x14','0x6d2e50'),@('0xb67468','0x18','0x6d4840'),@('0xb67468','0x34','0x6b1000'),
    @('0xb64eac','0x14','0x6790c0'),@('0xb64eac','0x18','0x6b2000'),@('0xb64eac','0x34','0x6b1000'),
    @('0xb64990','0x14','0x65cb60'),@('0xb64990','0x18','0x6b2000'),@('0xb64990','0x34','0x6b1000')
)
foreach ($expected in $tableEntries) {
    $table=$primary['primary-derived-tables.json'].tables | Where-Object va -eq $expected[0]
    $entry=$table.entries | Where-Object offset -eq $expected[1]
    Record-Check "table:$($expected[0])+$($expected[1])" ($entry.target_va -eq $expected[2]) 'Current class table; not extrapolated to all variants or other builds.'
}
$failures=@($checks | Where-Object {-not $_.pass})
$result=[ordered]@{
    tx='tx-03-12';date='2026-10-22';timezone='Asia/Shanghai'
    result=$(if($failures.Count -eq 0){'PASS_STATIC_DOCUMENT_REVIEW_ONLY'}else{'FAIL_STATIC_DOCUMENT_REVIEW'})
    execution='Single final static document review. No game, build, code tests, injection or debugger operation.'
    agent=@{model='gpt-6-luna';reasoning_effort='max';role='Derived-render research independently reviewed by primary agent'}
    semantic_review='Primary agent read native pseudocode, machine instructions, table entries, receiver/parameter flow, resource ownership and failures. Script checks delivery consistency only.'
    limitations=@('Replica and presentation adapter not implemented','Actual Lua bindings/effects UNKNOWN',
        'Image control ABI, release thread, GPU/concurrency and failures not closed',
        'All variants, Mods and Player composite render not covered','No runtime observation or hash computation')
    checks=$checks.ToArray();failures=$failures
}
$stream=[IO.FileStream]::new($reviewOutput,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
try {$writer.Write(($result | ConvertTo-Json -Depth 20))}finally{$writer.Dispose()}
[pscustomobject]@{result=$result.result;checks=$checks.Count;failures=$failures.Count;output=$reviewOutput} | ConvertTo-Json
if($failures.Count -ne 0){exit 1}
