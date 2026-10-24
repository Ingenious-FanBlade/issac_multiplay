param(
    [Parameter(Mandatory=$true)][string]$GameExe,
    [Parameter(Mandatory=$true)][string]$ArtifactDirectory,
    [Parameter(Mandatory=$true)][string]$Authorization,
    [string]$Python='python'
)
$ErrorActionPreference='Stop'
$entryFailure=$null
try {
    & (Join-Path $PSScriptRoot 'accept-virtual-device-e2e.ps1') -GameExe $GameExe -ArtifactDirectory $ArtifactDirectory -Authorization $Authorization -Transaction 'tx-01-50-source-capture' -Script (Join-Path $PSScriptRoot 'testdata\tx-01-50-source-capture-script.json')
}
catch { $entryFailure=$_.Exception.Message }
# The original feature report is retained, including NOT_OBSERVED and exit failure.
# Source attribution has its own report and is never labeled a feature PASS.
if (-not (Test-Path -LiteralPath (Join-Path $ArtifactDirectory 'capture\restoration.json'))) {
    throw "Source capture did not reach confirmed restoration: $entryFailure"
}
& $Python (Join-Path $PSScriptRoot 'report-native-player-source.py') --artifact-dir $ArtifactDirectory
if ($LASTEXITCODE -ne 0) { throw 'Source attribution missing or capture failed; inspect source-result.json; no retry.' }
