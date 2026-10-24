param([string]$RepentogonRoot='F:/projects_isaac/REPENTOGON-1.1.3', [switch]$BuildSdk)
$ErrorActionPreference='Stop'
$repoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
# Build only. E2E is a separate single invocation after implementation finishes.
if($BuildSdk) {
    & cmake --build (Join-Path $RepentogonRoot 'build') --config Release --target libzhl -- /m:4 /v:minimal
    if($LASTEXITCODE) { throw 'REPENTOGON SDK build failed' }
}
& cmake -S (Join-Path $repoRoot 'mod') -B (Join-Path $repoRoot 'build/repentogon') -G 'Visual Studio 17 2022' -A Win32 -T v143 "-DREPENTOGON_ROOT=$RepentogonRoot"
if($LASTEXITCODE) { throw 'IsaacCS configure failed' }
& cmake --build (Join-Path $repoRoot 'build/repentogon') --config Release -- /m:4 /v:minimal
if($LASTEXITCODE) { throw 'IsaacCS build failed' }
& cargo build --manifest-path (Join-Path $repoRoot 'tools/harness/Cargo.toml') --release --locked --target i686-pc-windows-msvc --target-dir (Join-Path $repoRoot 'build/harness')
if($LASTEXITCODE) { throw 'Harness build failed' }
Copy-Item -LiteralPath (Join-Path $repoRoot 'build/harness/i686-pc-windows-msvc/release/harness.exe') -Destination (Join-Path $repoRoot 'out/repentogon/harness.exe')
Copy-Item -LiteralPath (Join-Path $repoRoot 'build/repentogon/generated/hook-manifest.json') -Destination (Join-Path $repoRoot 'out/repentogon/hook-manifest.json')
Copy-Item -LiteralPath (Join-Path $repoRoot 'build/repentogon/generated/hook-manifest.md') -Destination (Join-Path $repoRoot 'out/repentogon/hook-manifest.md')
