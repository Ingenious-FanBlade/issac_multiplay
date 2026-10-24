#requires -Version 7.2
param(
    [Parameter(Mandatory=$true)][string]$GameExe,
    [Parameter(Mandatory=$true)][string]$BuildDirectory,
    [Parameter(Mandatory=$true)][string]$ArtifactDirectory,
    [Parameter(Mandatory=$true)][string]$Authorization,
    [double]$StartDelaySeconds=0,
    [string]$Python='D:\miniconda3\python.exe'
)
$ErrorActionPreference='Stop'
$casePath=[IO.Path]::GetFullPath($ArtifactDirectory)
if(Test-Path -LiteralPath $casePath) {throw 'ArtifactDirectory must be new; an acceptance is never automatically repeated.'}
if([string]::IsNullOrWhiteSpace($Authorization)) {throw 'Record the actual user authorization for this real-game experiment.'}
$buildPath=[IO.Path]::GetFullPath($BuildDirectory)
foreach($relative in @('Rust\IsaacCsT0.exe','Native\bin\IsaacCsT0.Native.dll','Helper\bin\IsaacCsT0.Helper.exe')) {
    $candidate=Join-Path $buildPath $relative
    if(-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {throw "Missing build artifact: $candidate"}
}
# The operator must already have one isolated game process in a normal active
# run with A present. The controller itself enforces unique process/module checks.
& $Python (Join-Path $PSScriptRoot 'run-authority-source-stop-e2e.py') `
    --game-exe $GameExe --build-dir $buildPath --artifact-dir $casePath --authorization $Authorization `
    --start-delay-seconds $StartDelaySeconds
$acceptanceExit=$LASTEXITCODE
if($acceptanceExit -ne 0) {
    Write-Output 'Operation 9 failed or was incomplete; artifacts are retained and must not be retried automatically.'
}
exit $acceptanceExit
