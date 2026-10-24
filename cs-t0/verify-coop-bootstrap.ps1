[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Split-Path -Parent $PSScriptRoot)
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$failures = [System.Collections.Generic.List[string]]::new()

function Add-CheckFailure {
    param([string]$Message)
    $failures.Add($Message)
}

function Assert-Check {
    param(
        [bool]$Condition,
        [string]$Message
    )
    if (-not $Condition) {
        Add-CheckFailure -Message $Message
    }
}

function Resolve-RepositoryPath {
    param([string]$RelativePath)
    if ([System.IO.Path]::IsPathRooted($RelativePath)) {
        throw "Expected a repository-relative path, got: $RelativePath"
    }
    $platformPath = $RelativePath.Replace('/', [System.IO.Path]::DirectorySeparatorChar)
    return Join-Path -Path $RepositoryRoot -ChildPath $platformPath
}

function Read-JsonFile {
    param([string]$RelativePath)
    $fullPath = Resolve-RepositoryPath -RelativePath $RelativePath
    return Get-Content -Raw -LiteralPath $fullPath | ConvertFrom-Json
}

function Get-Sha256 {
    param([string]$RelativePath)
    $fullPath = Resolve-RepositoryPath -RelativePath $RelativePath
    return (Get-FileHash -LiteralPath $fullPath -Algorithm SHA256).Hash
}

function Get-JsonPointerValue {
    param(
        [object]$Document,
        [string]$Pointer
    )
    if ($Pointer -eq '') {
        return $Document
    }
    if (-not $Pointer.StartsWith('/')) {
        throw "Invalid JSON pointer: $Pointer"
    }

    $current = $Document
    foreach ($encodedSegment in $Pointer.Substring(1).Split('/')) {
        $segment = $encodedSegment.Replace('~1', '/').Replace('~0', '~')
        if ($current -is [System.Array]) {
            $current = $current[[int]$segment]
            continue
        }
        $property = $current.PSObject.Properties[$segment]
        if ($null -eq $property) {
            throw "JSON pointer segment '$segment' was not found in $Pointer"
        }
        $current = $property.Value
    }
    return $current
}

function Find-ForbiddenPropertyNames {
    param(
        [object]$Value,
        [string]$JsonPath = '$'
    )
    if ($null -eq $Value -or $Value -is [string] -or $Value.GetType().IsPrimitive) {
        return
    }
    if ($Value -is [System.Array]) {
        for ($index = 0; $index -lt $Value.Count; $index++) {
            Find-ForbiddenPropertyNames -Value $Value[$index] -JsonPath "$JsonPath[$index]"
        }
        return
    }
    if (-not ($Value -is [System.Management.Automation.PSCustomObject])) {
        return
    }
    foreach ($property in $Value.PSObject.Properties) {
        if ($property.Name -match '^(arm|token|claim|rawPeerIdentity|peerAddress|absoluteUserPath)$') {
            Add-CheckFailure -Message "Forbidden bootstrap property $JsonPath.$($property.Name)"
        }
        Find-ForbiddenPropertyNames -Value $property.Value -JsonPath "$JsonPath.$($property.Name)"
    }
}

$jsonPaths = @(
    'cs-t0/coop-predecessor-binding-v1.json',
    'cs-t0/coop-hypothesis-index-v1.json',
    'cs-t0/coop-catalog-v1.json',
    'cs-t0/coop-plan-v1.json',
    'cs-t0/coop-matrix-v1.template.json',
    'cs-t0/ledger/isaac-cs-t0-coop-ledger.json',
    'cs-t0/coop-verification-v1.json',
    'cs-t0/coop-bootstrap-decision-v1.json'
)

$documents = @{}
foreach ($jsonPath in $jsonPaths) {
    $fullPath = Resolve-RepositoryPath -RelativePath $jsonPath
    Assert-Check -Condition (Test-Path -LiteralPath $fullPath -PathType Leaf) -Message "Missing artifact: $jsonPath"
    if (Test-Path -LiteralPath $fullPath -PathType Leaf) {
        $documents[$jsonPath] = Read-JsonFile -RelativePath $jsonPath
        $raw = Get-Content -Raw -LiteralPath $fullPath
        Assert-Check -Condition (-not ($raw -match '(?i)(?:[A-Z]:[\\/]|\\\\Users\\\\|/home/)')) -Message "Absolute user path found in $jsonPath"
        Find-ForbiddenPropertyNames -Value $documents[$jsonPath]
    }
}

if ($failures.Count -eq 0) {
    $predecessor = $documents['cs-t0/coop-predecessor-binding-v1.json']
    Assert-Check -Condition ($predecessor.bindings.Count -eq 13) -Message 'Predecessor binding count must be 13.'
    Assert-Check -Condition (($predecessor.bindings.id | Sort-Object -Unique).Count -eq 13) -Message 'Predecessor binding ids must be unique.'
    foreach ($binding in $predecessor.bindings) {
        Assert-Check -Condition (-not [System.IO.Path]::IsPathRooted([string]$binding.path)) -Message "Binding path must be relative: $($binding.id)"
        Assert-Check -Condition ($binding.status -eq 'PASS') -Message "Binding is not PASS: $($binding.id)"
        Assert-Check -Condition (-not [string]::IsNullOrWhiteSpace([string]$binding.schemaIdentity)) -Message "Binding schema identity is missing: $($binding.id)"
        if ($binding.bindingKind -eq 'FILE_SHA256') {
            $actual = Get-Sha256 -RelativePath $binding.path
            Assert-Check -Condition ($actual -eq $binding.expectedSha256) -Message "File hash mismatch: $($binding.id)"
            Assert-Check -Condition ($actual -eq $binding.actualSha256) -Message "Recorded actual hash mismatch: $($binding.id)"
        }
        elseif ($binding.bindingKind -eq 'JSON_FIELD') {
            $containerHash = Get-Sha256 -RelativePath $binding.path
            Assert-Check -Condition ($containerHash -eq $binding.containerSha256) -Message "Container hash mismatch: $($binding.id)"
            $source = Read-JsonFile -RelativePath $binding.path
            $actual = Get-JsonPointerValue -Document $source -Pointer $binding.selector
            Assert-Check -Condition ($actual -eq $binding.expectedSha256) -Message "JSON-field hash mismatch: $($binding.id)"
            Assert-Check -Condition ($actual -eq $binding.actualSha256) -Message "Recorded JSON-field hash mismatch: $($binding.id)"
        }
        else {
            Add-CheckFailure -Message "Unsupported binding kind: $($binding.bindingKind)"
        }
    }
    $equivalence = Read-JsonFile -RelativePath 'out/cs-t0/target-equivalence-inspect/unpacked-target-equivalence-v1-release-final.json'
    Assert-Check -Condition ($equivalence.status -eq 'PASS_EQUIVALENT_READ_ONLY') -Message 'Target equivalence status must be PASS_EQUIVALENT_READ_ONLY.'
    Assert-Check -Condition ($equivalence.canonicalTargetSha256 -eq 'C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30') -Message 'Canonical target identity differs from the frozen target.'
    Assert-Check -Condition ($equivalence.observedTargetSha256 -eq '1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C') -Message 'Approved-unpacked target identity differs from the frozen target.'
    $t004Decision = Read-JsonFile -RelativePath 'out/cs-t0/formal-active-gate-20260827-r1/active-gate-decision.json'
    Assert-Check -Condition ($t004Decision.status -eq 'PASS') -Message 'T0-04 final decision must be PASS.'
    Assert-Check -Condition ($t004Decision.t004Unlocked -eq $true) -Message 'T0-04 final decision must have t004Unlocked=true.'
    Assert-Check -Condition ($t004Decision.missingCases.Count -eq 0) -Message 'T0-04 final decision must have no missing cases.'

    $ledger = $documents['cs-t0/ledger/isaac-cs-t0-coop-ledger.json']
    $expectedLabels = @('EXPERIMENT_CONTRACT', 'REJECTED_HYPOTHESIS', 'RUNTIME_CONFIRMED', 'STATIC_CONFIRMED', 'UNKNOWN')
    $actualLabels = @($ledger.frozenEvidenceLabels | Sort-Object)
    Assert-Check -Condition (($actualLabels -join '|') -eq ($expectedLabels -join '|')) -Message 'Frozen evidence labels differ from the required set.'
    Assert-Check -Condition ($ledger.candidateInitialState.Count -eq 6) -Message 'Ledger must initialize six co-op candidates.'
    Assert-Check -Condition (@($ledger.candidateInitialState | Where-Object { $_.evidenceLabel -ne 'UNKNOWN' }).Count -eq 0) -Message 'Every new ledger candidate must remain UNKNOWN.'
    Assert-Check -Condition ($ledger.knownRuntimeGaps.Count -eq 2) -Message 'Ledger must record both inherited runtime gaps.'
    Assert-Check -Condition ($ledger.permissions.activeInputAllowed -eq $false) -Message 'Ledger activeInputAllowed must be false.'
    Assert-Check -Condition ($ledger.permissions.runtimeMutationAllowed -eq $false) -Message 'Ledger runtimeMutationAllowed must be false.'
    Assert-Check -Condition ($ledger.exitGate.result -eq 'PASS') -Message 'Ledger exit gate must be PASS.'

    $catalog = $documents['cs-t0/coop-catalog-v1.json']
    Assert-Check -Condition ($catalog.candidates.Count -eq 6) -Message 'Catalog must contain six new candidates.'
    foreach ($candidate in $catalog.candidates) {
        Assert-Check -Condition ($candidate.evidenceLabel -eq 'UNKNOWN') -Message "Catalog candidate is not UNKNOWN: $($candidate.id)"
        foreach ($field in @('rva', 'expectedBytes', 'targetRva', 'vtableRva', 'colRva', 'abi', 'slot')) {
            Assert-Check -Condition ($null -eq $candidate.$field) -Message "Catalog candidate field must be null: $($candidate.id).$field"
        }
    }
    Assert-Check -Condition ($catalog.inheritedHookTransaction.existingSlotCount -eq 45) -Message 'Inherited hook slot count must be 45.'
    Assert-Check -Condition ($catalog.inheritedHookTransaction.newSlots.Count -eq 0) -Message 'Bootstrap catalog must not invent new hook slots.'
    Assert-Check -Condition ($catalog.permissions.activeInputAllowed -eq $false) -Message 'Catalog activeInputAllowed must be false.'

    $matrix = $documents['cs-t0/coop-matrix-v1.template.json']
    Assert-Check -Condition ($matrix.basePrimaryCases.Count -eq 20) -Message 'Matrix must contain 20 base primary cases.'
    Assert-Check -Condition ($matrix.conditionalPrimaryCases.Count -eq 2) -Message 'Matrix must contain two conditional primary cases.'
    Assert-Check -Condition ($matrix.repeatabilityCases.Count -eq 2) -Message 'Matrix must contain two repeatability cases.'
    Assert-Check -Condition (($matrix.basePrimaryCases.ordinal -join ',') -eq ((1..20) -join ',')) -Message 'Matrix base ordinals must be exactly 1 through 20.'
    Assert-Check -Condition (@($matrix.basePrimaryCases | Where-Object { $null -ne $_.analysisPath }).Count -eq 0) -Message 'Bootstrap matrix must be unpopulated.'

    $verification = $documents['cs-t0/coop-verification-v1.json']
    foreach ($binding in $verification.artifactBindings) {
        $actual = Get-Sha256 -RelativePath ("cs-t0/{0}" -f $binding.path)
        Assert-Check -Condition ($actual -eq $binding.sha256) -Message "Verification artifact hash mismatch: $($binding.path)"
    }
    Assert-Check -Condition (@($verification.bootstrapChecks | Where-Object { $_.passed -ne $true }).Count -eq 0) -Message 'All bootstrap verification checks must pass.'
    Assert-Check -Condition ($verification.permissions.activeInputAllowed -eq $false) -Message 'Verification activeInputAllowed must be false.'

    $decision = $documents['cs-t0/coop-bootstrap-decision-v1.json']
    foreach ($binding in $decision.artifactBindings) {
        $actual = Get-Sha256 -RelativePath ("cs-t0/{0}" -f $binding.path)
        Assert-Check -Condition ($actual -eq $binding.sha256) -Message "Decision artifact hash mismatch: $($binding.path)"
    }
    Assert-Check -Condition ($decision.status -eq 'PASS') -Message 'Bootstrap decision must be PASS.'
    Assert-Check -Condition ($decision.csT00501Unlocked -eq $true) -Message 'Bootstrap decision must unlock CS-T0-05-01.'
    Assert-Check -Condition ($decision.t005Unlocked -eq $false) -Message 'Bootstrap decision must not unlock all of T0-05.'
    Assert-Check -Condition ($decision.activeInputAllowed -eq $false) -Message 'Bootstrap decision activeInputAllowed must be false.'
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

[PSCustomObject]@{
    WorkPackage = 'CS-T0-05-00'
    Status = 'PASS'
    PredecessorBindings = 13
    NewCandidatesUnknown = 6
    BasePrimaryCases = 20
    ConditionalPrimaryCases = 2
    RepeatabilityCases = 2
    ActiveInputAllowed = $false
    NextWorkPackage = 'CS-T0-05-01'
} | Format-List
