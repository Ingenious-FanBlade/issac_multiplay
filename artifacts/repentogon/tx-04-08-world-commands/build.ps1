param([string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path)
$ErrorActionPreference = 'Stop'
# 仅纯编译/链接 DLL；不执行测试，不启动游戏，不部署到游戏目录。
$buildDir = Join-Path $RepoRoot 'build/repentogon-tx-04-08-world-commands'
& cmake -S (Join-Path $RepoRoot 'mod') -B $buildDir -G 'Visual Studio 17 2022' -A Win32 -T v143
if ($LASTEXITCODE) { throw 'DLL configure failed' }
& cmake --build $buildDir --config Release --target zhlIsaacCS -- /m:4 /v:minimal
if ($LASTEXITCODE) { throw 'complete DLL build failed' }
