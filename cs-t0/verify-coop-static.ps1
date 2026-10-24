[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$GameExe,
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot),
    [string]$Controller = 'cs-t0/target/release/IsaacCsT0.exe',
    [string]$TargetEquivalence = 'out/cs-t0/target-equivalence-inspect/unpacked-target-equivalence-v1-release-final.json'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$failures = [System.Collections.Generic.List[string]]::new()

function Add-Failure {
    param([string]$Message)
    $failures.Add($Message)
}

function Assert-Check {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        Add-Failure -Message $Message
    }
}

function Resolve-RepoPath {
    param([string]$RelativePath)
    if ([System.IO.Path]::IsPathRooted($RelativePath)) {
        throw "Expected repository-relative path: $RelativePath"
    }
    $platformPath = $RelativePath.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
    return Join-Path -Path $RepositoryRoot -ChildPath $platformPath
}

function Get-Sha256 {
    param([string]$RelativePath)
    return (Get-FileHash -LiteralPath (Resolve-RepoPath $RelativePath) -Algorithm SHA256).Hash
}

function Read-Json {
    param([string]$RelativePath)
    return Get-Content -Raw -LiteralPath (Resolve-RepoPath $RelativePath) | ConvertFrom-Json
}

function Assert-Bindings {
    param([object[]]$Bindings, [string]$BasePath, [string]$Label)
    foreach ($binding in $Bindings) {
        Assert-Check -Condition (-not [System.IO.Path]::IsPathRooted([string]$binding.path)) -Message "$Label contains an absolute path: $($binding.path)"
        $relative = if ([string]::IsNullOrEmpty($BasePath)) {
            "cs-t0/$($binding.path)"
        }
        else {
            "cs-t0/$BasePath/$($binding.path)"
        }
        $actual = Get-Sha256 -RelativePath $relative
        Assert-Check -Condition ($actual -eq $binding.sha256) -Message "$Label hash mismatch: $($binding.path)"
    }
}

$v1Hashes = [ordered]@{
    'cs-t0/coop-bootstrap-decision-v1.json' = '7EB5004E368A478EBB3CC83149BB05E70F7876C4384A511557EEC9188D1D7598'
    'cs-t0/coop-catalog-v1.json' = '206C715996B262A7A185E92652D0DC3108DC4E97553B5B7E902C620CD51E109A'
    'cs-t0/coop-hypothesis-index-v1.json' = 'E892F698BFE82F00516B40C6F1DC7354ABA68C7D98B78F37126E2CEB6CB5FC96'
    'cs-t0/coop-matrix-v1.template.json' = 'C3BB9A97188708E9BCFE9E25E8D9DAFF37EBBA5656CEA52CEFE04F9EEE1D1100'
    'cs-t0/coop-plan-v1.json' = '112B551F666E101C1F644B48F7B6B7BCB18F2DAD74A8464574E9A04855CE59A5'
    'cs-t0/coop-predecessor-binding-v1.json' = '5D29826CD9E92E84D4C97C14E511066EF1FD8576B2DA5CC4639861BD9812DB90'
    'cs-t0/coop-verification-v1.json' = 'A894867D5B5388C9540B024083F659D1EDBC1CB461D362301519FB18E48ABB48'
    'cs-t0/ledger/isaac-cs-t0-coop-ledger.json' = 'DC2E8F5B81929B84AB281779CF3F056F96BAE5446B7F3ED2173200A52421227C'
    'cs-t0/verify-coop-bootstrap.ps1' = 'FA9739C90A86BB504C2170F9A4946A8CFFA93B588D56EDB93DE544CE1F119F8A'
}

foreach ($entry in $v1Hashes.GetEnumerator()) {
    Assert-Check -Condition ((Get-Sha256 $entry.Key) -eq $entry.Value) -Message "Frozen v1 input changed: $($entry.Key)"
}

$jsonPaths = @(
    'cs-t0/coop-static-evidence-v1.json',
    'cs-t0/coop-static-graph-v1.json',
    'cs-t0/coop-static-inspection-v1.json',
    'cs-t0/coop-static-decision-v1.json',
    'cs-t0/ledger/isaac-cs-t0-coop-ledger-v2.json',
    'cs-t0/coop-catalog-v2.json',
    'cs-t0/coop-plan-v2.json',
    'cs-t0/coop-hypothesis-index-v2.json',
    'cs-t0/coop-verification-v2.json'
)

foreach ($path in $jsonPaths) {
    $fullPath = Resolve-RepoPath $path
    Assert-Check -Condition (Test-Path -LiteralPath $fullPath -PathType Leaf) -Message "Missing static artifact: $path"
    if (Test-Path -LiteralPath $fullPath -PathType Leaf) {
        $raw = Get-Content -Raw -LiteralPath $fullPath
        Assert-Check -Condition (-not ($raw -match '(?i)(?:[A-Z]:[\\/]|\\\\Users\\\\|/home/|file://)')) -Message "Absolute path leak: $path"
        Assert-Check -Condition (-not ($raw -match '(?i)\"(?:pid|processId|platformHandle|rawPeerIdentity|pointerValue)\"\s*:')) -Message "Runtime identity leak: $path"
        $null = $raw | ConvertFrom-Json
    }
}

if ($failures.Count -eq 0) {
    $evidence = Read-Json 'cs-t0/coop-static-evidence-v1.json'
    $graph = Read-Json 'cs-t0/coop-static-graph-v1.json'
    $inspection = Read-Json 'cs-t0/coop-static-inspection-v1.json'
    $decision = Read-Json 'cs-t0/coop-static-decision-v1.json'
    $ledger = Read-Json 'cs-t0/ledger/isaac-cs-t0-coop-ledger-v2.json'
    $catalog = Read-Json 'cs-t0/coop-catalog-v2.json'
    $plan = Read-Json 'cs-t0/coop-plan-v2.json'
    $hypotheses = Read-Json 'cs-t0/coop-hypothesis-index-v2.json'
    $verification = Read-Json 'cs-t0/coop-verification-v2.json'

    Assert-Check -Condition ($evidence.blockingMissingEvidence.Count -eq 0) -Message 'Static evidence still has blocking claims.'
    Assert-Check -Condition (($graph.partitions.id | Sort-Object) -join '|' -eq 'LOCAL_COOP_POSITIVE|OFFLINE_SHARED|ORIGINAL_ONLINE_FORBIDDEN') -Message 'Graph partition set differs.'
    Assert-Check -Condition ($graph.requiredRoutes.Count -eq 2) -Message 'Graph must contain join and exit routes.'
    Assert-Check -Condition (@($graph.requiredRoutes.nodes | Where-Object { $_ -contains 'online-netinput-guard' }).Count -eq 0) -Message 'A positive route crosses the online partition.'
    Assert-Check -Condition ($graph.unresolvedClaims.Count -eq 0) -Message 'Graph has unresolved blocking claims.'
    Assert-Check -Condition ($graph.permissions.activeInputAllowed -eq $false -and $graph.permissions.runtimeMutationAllowed -eq $false -and $graph.permissions.t005Unlocked -eq $false) -Message 'Graph permissions widened.'

    Assert-Check -Condition ($catalog.supersedesSha256 -eq $v1Hashes['cs-t0/coop-catalog-v1.json']) -Message 'Catalog supersedes hash mismatch.'
    Assert-Check -Condition ($catalog.graphSha256 -eq (Get-Sha256 'cs-t0/coop-static-graph-v1.json')) -Message 'Catalog graph hash mismatch.'
    Assert-Check -Condition ($catalog.evidenceSha256 -eq (Get-Sha256 'cs-t0/coop-static-evidence-v1.json')) -Message 'Catalog evidence hash mismatch.'
    Assert-Check -Condition ($catalog.inheritedHookTransaction.existingSlotCount -eq 45) -Message 'Existing slot count is not 45.'
    Assert-Check -Condition ($catalog.inheritedHookTransaction.combinedSlotCount -eq 53) -Message 'Combined slot count is not 53.'
    Assert-Check -Condition (($catalog.inheritedHookTransaction.newSlots.ordinal -join ',') -eq ((45..52) -join ',')) -Message 'New slot ordinals are not 45..52.'
    Assert-Check -Condition (($catalog.inheritedHookTransaction.installOrder -join '|') -eq (($catalog.inheritedHookTransaction.newSlots.id) -join '|')) -Message 'Install order is not ascending.'
    $reversed = @($catalog.inheritedHookTransaction.newSlots.id)
    [array]::Reverse($reversed)
    Assert-Check -Condition (($catalog.inheritedHookTransaction.restoreOrder -join '|') -eq ($reversed -join '|')) -Message 'Restore order is not strict reverse.'
    Assert-Check -Condition (@($catalog.excludedObservations | Where-Object { $_.evidenceLabel -ne 'UNKNOWN' -or $null -ne $_.rva -or $null -ne $_.abi }).Count -eq 0) -Message 'UNKNOWN observation contains guessed data.'

    Assert-Check -Condition ($ledger.supersedesSha256 -eq $v1Hashes['cs-t0/ledger/isaac-cs-t0-coop-ledger.json']) -Message 'Ledger supersedes hash mismatch.'
    Assert-Bindings -Bindings $ledger.artifactBindings -BasePath 'ledger' -Label 'Ledger'
    Assert-Check -Condition ($ledger.status -eq 'PASS' -and $ledger.staticContract.missingEvidence.Count -eq 0) -Message 'Ledger is not a complete PASS.'
    Assert-Check -Condition ($plan.supersedesSha256 -eq $v1Hashes['cs-t0/coop-plan-v1.json']) -Message 'Plan supersedes hash mismatch.'
    Assert-Bindings -Bindings $plan.artifactBindings -BasePath '' -Label 'Plan'
    Assert-Check -Condition ($hypotheses.supersedesSha256 -eq $v1Hashes['cs-t0/coop-hypothesis-index-v1.json']) -Message 'Hypothesis supersedes hash mismatch.'
    Assert-Bindings -Bindings $hypotheses.artifactBindings -BasePath '' -Label 'Hypothesis index'

    Assert-Check -Condition ($inspection.result -eq 'PASS' -and $inspection.missingEvidence.Count -eq 0) -Message 'Inspection is not PASS.'
    Assert-Check -Condition (@($inspection.checks | Where-Object { $_.passed -ne $true }).Count -eq 0) -Message 'Inspection contains a failed check.'
    Assert-Check -Condition ($inspection.featureResults.Count -eq 8) -Message 'Inspection does not contain eight feature results.'
    foreach ($feature in $inspection.featureResults) {
        Assert-Check -Condition ($feature.matchRvas.Count -eq 1) -Message "Feature is not unique: $($feature.slotId)"
    }

    Assert-Check -Condition ($verification.supersedesSha256 -eq $v1Hashes['cs-t0/coop-verification-v1.json']) -Message 'Verification supersedes hash mismatch.'
    Assert-Bindings -Bindings $verification.artifactBindings -BasePath '' -Label 'Verification'
    Assert-Check -Condition ($verification.status -eq 'PASS' -and $verification.staticContractComplete -eq $true -and $verification.runtimeContractComplete -eq $false) -Message 'Verification state is inconsistent.'

    Assert-Bindings -Bindings $decision.artifactBindings -BasePath '' -Label 'Decision'
    Assert-Check -Condition ($decision.status -eq 'PASS' -and $decision.missingEvidence.Count -eq 0) -Message 'Decision is not PASS.'
    Assert-Check -Condition ($decision.csT00502Unlocked -eq $true -and $decision.t005Unlocked -eq $false) -Message 'Decision unlock state is inconsistent.'
    Assert-Check -Condition ($decision.activeInputAllowed -eq $false -and $decision.runtimeMutationAllowed -eq $false) -Message 'Decision permissions widened.'

    $controllerPath = Resolve-RepoPath $Controller
    $targetEquivalencePath = Resolve-RepoPath $TargetEquivalence
    $cliArgs = @(
        'coop-inspect',
        '--game-exe', $GameExe,
        '--target-equivalence', $targetEquivalencePath,
        '--bootstrap-decision', (Resolve-RepoPath 'cs-t0/coop-bootstrap-decision-v1.json'),
        '--ledger', (Resolve-RepoPath 'cs-t0/ledger/isaac-cs-t0-coop-ledger-v2.json'),
        '--catalog', (Resolve-RepoPath 'cs-t0/coop-catalog-v2.json'),
        '--graph', (Resolve-RepoPath 'cs-t0/coop-static-graph-v1.json')
    )
    $cliText = & $controllerPath @cliArgs
    Assert-Check -Condition ($LASTEXITCODE -eq 0) -Message 'coop-inspect CLI did not pass.'
    if ($LASTEXITCODE -eq 0) {
        $cliInspection = $cliText | ConvertFrom-Json
        $frozenCompact = $inspection | ConvertTo-Json -Depth 100 -Compress
        $cliCompact = $cliInspection | ConvertTo-Json -Depth 100 -Compress
        Assert-Check -Condition ($cliCompact -ceq $frozenCompact) -Message 'coop-inspect stdout is not deterministic or differs from frozen inspection.'
    }

    $inspectionPath = Resolve-RepoPath 'cs-t0/coop-static-inspection-v1.json'
    $beforeOverwrite = (Get-FileHash -LiteralPath $inspectionPath -Algorithm SHA256).Hash
    $overwriteText = & $controllerPath @cliArgs '--output' $inspectionPath 2>&1
    Assert-Check -Condition ($LASTEXITCODE -eq 2) -Message 'coop-inspect did not refuse an existing output with usage exit 2.'
    $afterOverwrite = (Get-FileHash -LiteralPath $inspectionPath -Algorithm SHA256).Hash
    Assert-Check -Condition ($afterOverwrite -eq $beforeOverwrite) -Message 'Existing inspection output changed after overwrite refusal.'
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

[PSCustomObject]@{
    WorkPackage = 'CS-T0-05-01'
    Status = 'PASS'
    StaticRoutes = 2
    ObserverSlots = 8
    CombinedSlots = 53
    MissingEvidence = 0
    CsT00502Unlocked = $true
    ActiveInputAllowed = $false
    RuntimeMutationAllowed = $false
    NextWorkPackage = 'CS-T0-05-02'
} | Format-List
