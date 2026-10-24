param([string]$OutputPath = (Join-Path $PSScriptRoot 'final-static-review.json'))
$ErrorActionPreference = 'Stop'
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$reviewOutput = [IO.Path]::GetFullPath($OutputPath)
if (Test-Path -LiteralPath $reviewOutput) { throw 'Use a new path; preserve previous results.' }
$checks = [Collections.Generic.List[object]]::new()
function Record-Check([string]$Name,[bool]$Pass,[string]$Detail) {
    $checks.Add([pscustomobject]@{name=$Name;pass=$Pass;detail=$Detail})
}
$newDocuments = @('docs/research/tx-03-13-sprite-image-resources.md',
    'docs/research/tx-03-13-primary-review.md','docs/research/tx-03-13-luna-image-control.md')
$entryDocuments = @('docs/README.md','docs/hooks/README.md','docs/design/state-synchronization.md')
foreach ($relative in ($newDocuments+$entryDocuments)) {
    $path=Join-Path $repository $relative
    $exists=Test-Path -LiteralPath $path -PathType Leaf
    Record-Check "document:$relative" $exists 'Document exists.'
    if (-not $exists) {continue}
    $body=Get-Content -LiteralPath $path -Raw -Encoding utf8
    if ($relative -in $entryDocuments) {
        $body=(($body -split "`n") | Where-Object {$_ -match 'tx-03-13'}) -join "`n"
        Record-Check "entry:$relative" ($body.Length -gt 0) 'Entry includes this research.'
    }
    foreach ($match in [regex]::Matches($body,'\[[^\]]*\]\(([^)]+)\)')) {
        $target=$match.Groups[1].Value.Trim('<','>')
        if ($target -match '^[a-zA-Z]+://' -or $target.StartsWith('#')) {continue}
        $target=($target -split '#')[0]
        $absolute=[IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($path)) $target))
        Record-Check "link:$relative->$target" (($absolute -eq $reviewOutput) -or (Test-Path -LiteralPath $absolute)) 'Local reference resolves; the result is created at completion.'
    }
}
$primary=@{}
$allJson=@{}
foreach ($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.json' -File)) {
    $value=Get-Content -LiteralPath $file.FullName -Raw -Encoding utf8 | ConvertFrom-Json -Depth 100
    $allJson[$file.Name]=$value
    Record-Check "json:$($file.Name)" $true 'Readable JSON; acquisition notes and tool errors are not target-program test results.'
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
            $valid=$valid -and (([Convert]::ToInt64($table.va.Substring(2),16)-0x400000) -eq [Convert]::ToInt64($table.rva.Substring(2),16))
            foreach ($entry in $table.entries) {
                $valid=$valid -and (([Convert]::ToInt64($entry.target_va.Substring(2),16)-0x400000) -eq [Convert]::ToInt64($entry.target_rva.Substring(2),16))
            }
        }
        Record-Check "addresses:$($file.Name)" $valid 'Static metadata, decompilation status and VA/RVA conversion agree.'
    }
}
foreach ($name in @('primary-image-initial.json','primary-image-loader.json','primary-image-release.json',
    'primary-image-retire-context.json','primary-image-tables.json','primary-image-png-lifecycle.json',
    'primary-image-gpu.json','primary-image-control-review.json')) {
    Record-Check "evidence:$name" ($primary.ContainsKey($name)) 'Required independent primary evidence present.'
}
Record-Check 'evidence:luna-control-core.json' ($allJson.ContainsKey('luna-control-core.json')) 'Child raw evidence present.'
Record-Check 'evidence:primary-acquisition-notes.json' ($allJson.ContainsKey('primary-acquisition-notes.json')) 'Acquisition diagnostics retained.'
foreach ($script in @('export_static.py','luna-export-control.py')) {
    Record-Check "script:$script" (Test-Path -LiteralPath (Join-Path $PSScriptRoot $script) -PathType Leaf) 'Read-only extraction source supplied; not executed by this review.'
}
$anchors=@(
    @('primary-image-control-review.json','0x40cc10','0x40cc42','c7 46 04 01 00 01 00'),
    @('primary-image-control-review.json','0x40c7f0','0x40c81a','6a 18'),
    @('primary-image-control-review.json','0x40c7f0','0x40c83e','c7 07 e0 a6 b1 00'),
    @('primary-image-control-review.json','0x40c6b0','0x40c6d6','ff 10'),
    @('primary-image-control-review.json','0x40c6b0','0x40c6dc','ff 50 08'),
    @('primary-image-control-review.json','0x40c630','0x40c649','66 89 46 04'),
    @('primary-image-control-review.json','0x40c630','0x40c666','ff 50 34'),
    @('primary-image-control-review.json','0x40c5c0','0x40c5f1','ff 10'),
    @('primary-image-control-review.json','0x408640','0x4086d0','c2 08 00'),
    @('primary-image-release.json','0xa12240','0xa12269','83 fe 01'),
    @('primary-image-release.json','0xa12240','0xa12270','e8 3b ff ff ff'),
    @('primary-image-release.json','0xa11700','0xa11712','c7 05 3c 16 c7 00 40 22 a1 00'),
    @('primary-image-release.json','0xa11730','0xa11741','c7 05 3c 16 c7 00 00 00 00 00'),
    @('primary-image-initial.json','0x40c000','0x40c146','88 4c 03 30'),
    @('primary-image-initial.json','0x9588a0','0x9589bf','c2 08 00'),
    @('primary-image-loader.json','0xa118d0','0xa11c68','c2 0c 00'),
    @('primary-image-png-lifecycle.json','0xa12b80','0xa12b85','83 4e 08 01'),
    @('primary-image-png-lifecycle.json','0xa69a30','0xa69a39','ff 15 d4 1d c1 00'),
    @('primary-image-gpu.json','0xa69b30','0xa69b94','ff 15 78 16 c1 00'),
    @('primary-image-gpu.json','0xa69b30','0xa69bc5','ff 15 fc 1e c1 00'),
    @('primary-image-gpu.json','0xa69b30','0xa69bd6','b0 01'),
    @('primary-image-gpu.json','0xa69b30','0xa69bdc','c2 04 00'),
    @('primary-image-gpu.json','0xa649d0','0xa64a08','ff 15 d4 1d c1 00'),
    @('primary-image-gpu.json','0x414d60','0x414db2','8b 40 0c')
)
foreach ($anchor in $anchors) {
    $fn=$primary[$anchor[0]].functions | Where-Object va -eq $anchor[1]
    $insn=$fn.instructions | Where-Object va -eq $anchor[2]
    Record-Check "instruction:$($anchor[2])" ($insn.bytes -eq $anchor[3]) "Reviewed raw instruction in $($anchor[0])."
}
$tableEntries=@(
    @('primary-image-control-review.json','0xb1a6e0','0x0','0x40cb50'),
    @('primary-image-control-review.json','0xb1a6e0','0x4','0x40c6b0'),
    @('primary-image-control-review.json','0xb1a6e0','0x8','0x40c690'),
    @('primary-image-control-review.json','0xb1a6e0','0xc','0x40c630'),
    @('primary-image-control-review.json','0xb1a6e0','0x10','0x40c610'),
    @('primary-image-control-review.json','0xb1a6e0','0x14','0x40c5c0'),
    @('primary-image-control-review.json','0xb81c0c','0xc','0xa157f0'),
    @('primary-image-control-review.json','0xb81c0c','0x10','0xa159a0'),
    @('primary-image-tables.json','0xb9ff84','0x0','0xa123b0'),
    @('primary-image-tables.json','0xb9ff84','0x34','0xa649d0'),
    @('primary-image-tables.json','0xb9ff84','0x38','0xa64a50'),
    @('primary-image-tables.json','0xb9ff84','0x3c','0xa69a30'),
    @('primary-image-tables.json','0xb9ff84','0x40','0xa12b80'),
    @('primary-image-tables.json','0xb9ff84','0x44','0xa69a50'),
    @('primary-image-tables.json','0xb9ff84','0x48','0xa69ab0'),
    @('primary-image-tables.json','0xb9ff84','0x58','0xa69b30'),
    @('primary-image-tables.json','0xb9ff84','0x5c','0x42cdf0')
)
foreach ($expected in $tableEntries) {
    $table=$primary[$expected[0]].tables | Where-Object va -eq $expected[1]
    $entry=$table.entries | Where-Object offset -eq $expected[2]
    Record-Check "table:$($expected[1])+$($expected[2])" ($entry.target_va -eq $expected[3]) 'Current receiver-specific table; no cross-version or other-format claim.'
    if ($expected[1] -eq '0xb1a6e0') {
        $childTable=$allJson['luna-control-core.json'].tables | Where-Object va -eq $expected[1]
        $childEntry=$childTable.entries | Where-Object offset -eq $expected[2]
        Record-Check "independent-table:$($expected[2])" (($childEntry.target_va -eq $entry.target_va) -and ($childEntry.bytes -eq $entry.bytes)) 'Primary independently re-exported and read the child control table.'
    }
}
$failures=@($checks | Where-Object {-not $_.pass})
$result=[ordered]@{
    tx='tx-03-13';date='2026-10-22';timezone='Asia/Shanghai'
    result=$(if($failures.Count -eq 0){'PASS_STATIC_DOCUMENT_REVIEW_ONLY'}else{'FAIL_STATIC_DOCUMENT_REVIEW'})
    execution='One final unified static document review after research and edits. No game, build, code tests, injection or debugger operation.'
    agent=@{model='gpt-6-luna';reasoning_effort='max';role='Image control research independently reviewed by primary'}
    semantic_review='Primary read pseudocode, raw instructions, receiver-specific tables, control return predicates, caller stack layout, two caches, loader flags and direct PNG GL calls. Script checks artifact consistency only.'
    limitations=@('No runtime observation; renderer owner thread and current GL context UNKNOWN',
        'Concurrency, shutdown, OOM and full EH cleanup not proven','Other image decoders and all Mods not covered',
        'Reload and replacement, presentation adapter and Replica not implemented','No hash computation or verification')
    checks=$checks.ToArray();failures=$failures
}
$stream=[IO.FileStream]::new($reviewOutput,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
try {$writer.Write(($result | ConvertTo-Json -Depth 20))}finally{$writer.Dispose()}
[pscustomobject]@{result=$result.result;checks=$checks.Count;failures=$failures.Count;output=$reviewOutput} | ConvertTo-Json
if($failures.Count -ne 0){exit 1}
