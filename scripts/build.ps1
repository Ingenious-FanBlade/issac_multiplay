param([string]$RepentogonRoot='F:/projects_isaac/REPENTOGON-1.1.3', [switch]$BuildSdk, [string]$HarnessOutput)
$ErrorActionPreference='Stop'
$repoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
# 只构建；全部实现完成后另行执行唯一一次整体 E2E。
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
# 连续验收允许新编排器输出到另一文件，不覆盖仍在等待远端的房主进程。
if(-not $HarnessOutput){$HarnessOutput=Join-Path $repoRoot 'out/repentogon/harness.exe'}
elseif(-not [IO.Path]::IsPathRooted($HarnessOutput)){$HarnessOutput=Join-Path $repoRoot $HarnessOutput}
Copy-Item -LiteralPath (Join-Path $repoRoot 'build/harness/i686-pc-windows-msvc/release/harness.exe') -Destination $HarnessOutput
Copy-Item -LiteralPath (Join-Path $repoRoot 'build/repentogon/generated/hook-manifest.json') -Destination (Join-Path $repoRoot 'out/repentogon/hook-manifest.json')
Copy-Item -LiteralPath (Join-Path $repoRoot 'build/repentogon/generated/hook-manifest.md') -Destination (Join-Path $repoRoot 'out/repentogon/hook-manifest.md')
# tx-04-08：布局清单（字段偏移/来源/RTTI/常量）与 Hook 清单一同落到产物目录。
Copy-Item -LiteralPath (Join-Path $repoRoot 'build/repentogon/generated/world-layout.json') -Destination (Join-Path $repoRoot 'out/repentogon/world-layout.json')
Copy-Item -LiteralPath (Join-Path $repoRoot 'build/repentogon/generated/replication-catalog.json') -Destination (Join-Path $repoRoot 'out/repentogon/replication-catalog.json')
Copy-Item -LiteralPath (Join-Path $repoRoot 'build/repentogon/generated/room-grid-application.json') -Destination (Join-Path $repoRoot 'out/repentogon/room-grid-application.json')

# 正式观察 Lua 作为产品资源单独打包，测试夹具与 Rust 不混入模组目录。
$observationOutput=Join-Path $repoRoot 'out/repentogon/isaac-cs-observation'
New-Item -ItemType Directory -Path $observationOutput -Force | Out-Null
Copy-Item -Path (Join-Path $repoRoot 'mod/observation/isaac-cs-observation/*') -Destination $observationOutput -Recurse -Force
