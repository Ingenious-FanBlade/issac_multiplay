param([string]$OutputPath)

# Static document acceptance only: no target execution, builds, tests or hashes.
$ErrorActionPreference = 'Stop'
$txReviewRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
if (-not $OutputPath) {
    $OutputPath = Join-Path $PSScriptRoot 'final-static-review.json'
}
$txReviewDocuments = @(
    'docs/research/tx-03-10-replica-update-presentation-boundary.md',
    'docs/research/tx-03-10-primary-review.md',
    'docs/README.md', 'docs/hooks/README.md', 'docs/hooks/game-loop.md',
    'docs/hooks/entity.md', 'docs/design/state-synchronization.md'
)
$txReviewErrors = [Collections.Generic.List[string]]::new()
$txReviewLinks = [Collections.Generic.List[object]]::new()
foreach ($txReviewRelative in $txReviewDocuments) {
    $txReviewPath = Join-Path $txReviewRoot $txReviewRelative
    if (-not [IO.File]::Exists($txReviewPath)) {
        $txReviewErrors.Add("Missing document: $txReviewRelative")
        continue
    }
    $txReviewText = [IO.File]::ReadAllText($txReviewPath)
    # Only newly authored research pages are scanned in full. Historical links
    # in migrated index pages retain their original scope and are not rewritten.
    if ($txReviewRelative.StartsWith('docs/research/')) {
        foreach ($txReviewMatch in [regex]::Matches($txReviewText, '\[[^\]]+\]\(([^)]+)\)')) {
            $txReviewTarget = $txReviewMatch.Groups[1].Value
            if ($txReviewTarget -match '^[a-zA-Z]+:|^#') { continue }
            $txReviewTarget = ($txReviewTarget -split '#', 2)[0]
            $txReviewResolved = [IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($txReviewPath)) $txReviewTarget))
            # This result is created by this single acceptance invocation below.
            $txReviewPendingResult = $txReviewResolved -eq [IO.Path]::GetFullPath($OutputPath)
            $txReviewExists = [IO.File]::Exists($txReviewResolved) -or $txReviewPendingResult
            $txReviewLinks.Add([ordered]@{ document=$txReviewRelative; target=$txReviewTarget; exists=$txReviewExists })
            if (-not $txReviewExists) { $txReviewErrors.Add("Missing new reference: $txReviewRelative -> $txReviewTarget") }
        }
    }
}
$txReviewJsonFiles = @(Get-ChildItem -LiteralPath $PSScriptRoot -File -Filter '*.json' | Where-Object Name -ne 'final-static-review.json')
foreach ($txReviewJsonFile in $txReviewJsonFiles) {
    try { $null = [IO.File]::ReadAllText($txReviewJsonFile.FullName) | ConvertFrom-Json }
    catch { $txReviewErrors.Add("Unreadable JSON $($txReviewJsonFile.Name): $($_.Exception.Message)") }
}
$txReviewExport = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'loop-render-export.json')) | ConvertFrom-Json
$txReviewRoomCall = @($txReviewExport.call_sites | Where-Object va -eq '0x6fba4d')
if ($txReviewRoomCall.Count -ne 1 -or $txReviewRoomCall[0].operand0_value -ne '0x802980') {
    $txReviewErrors.Add('Documented Room target is inconsistent with the static operand export.')
}
$txReviewStatus = if ($txReviewErrors.Count -eq 0) { 'STATIC_REVIEW_PASS' } else { 'STATIC_REVIEW_FAIL' }
$txReviewResult = [ordered]@{
    tx='tx-03-10'; date='2026-10-22'; timezone='Asia/Shanghai'; status=$txReviewStatus
    scope='Loop dispatch static research and document delivery only'
    human_review='Primary agent reread raw instructions, corrected subagent conclusions, and retained specific unknowns.'
    safe_replica_boundary='UNPROVEN'; replica_implementation='NOT_STARTED'
    game_execution='NOT_RUN'; build='NOT_RUN'; code_tests='NOT_RUN'; hash_operations='NONE'
    documents=$txReviewDocuments; parsed_json=@($txReviewJsonFiles.Name)
    new_references=$txReviewLinks.ToArray(); errors=$txReviewErrors.ToArray()
}
$txReviewStream = [IO.File]::Open([IO.Path]::GetFullPath($OutputPath), [IO.FileMode]::CreateNew, [IO.FileAccess]::Write)
$txReviewWriter = [IO.StreamWriter]::new($txReviewStream, [Text.UTF8Encoding]::new($false))
try { $txReviewWriter.WriteLine(($txReviewResult | ConvertTo-Json -Depth 12)) }
finally { $txReviewWriter.Dispose() }
[pscustomobject]@{ status=$txReviewStatus; documents=$txReviewDocuments.Count; json_files=$txReviewJsonFiles.Count; new_references=$txReviewLinks.Count; errors=$txReviewErrors.ToArray(); output=[IO.Path]::GetFullPath($OutputPath) } | ConvertTo-Json -Depth 4
