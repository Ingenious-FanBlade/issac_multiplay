param([string]$RepentogonRoot='F:/projects_isaac/REPENTOGON-1.1.3', [switch]$BuildSdk, [switch]$E2E, [string]$HarnessOutput)
$ErrorActionPreference='Stop'
$repoRoot=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
# 只构建；全部实现完成后另行执行唯一一次整体 E2E。
if($BuildSdk) {
    & cmake --build (Join-Path $RepentogonRoot 'build') --config Release --target libzhl -- /m:4 /v:minimal
    if($LASTEXITCODE) { throw 'REPENTOGON SDK build failed' }
}
$buildTree=if($E2E){'build/repentogon-e2e'}else{'build/repentogon'}
$e2eTools=if($E2E){'ON'}else{'OFF'}
$buildOutput=Join-Path $repoRoot $(if($E2E){'out/repentogon/e2e'}else{'out/repentogon'})
# 配置文件在configure阶段生成；完成凭据必须在整次构建/资源复制成功后才重新写入。
$buildReceipt=Join-Path $buildOutput 'build-complete.json'
if(Test-Path -LiteralPath $buildReceipt){Remove-Item -LiteralPath $buildReceipt -Force}
# Lua输出必须是本轮源树，不保留以前已删除的夹具或模块。递归清理前核对绝对路径。
function Copy-BuildTree([string]$source,[string]$destination){
    $resolved=[IO.Path]::GetFullPath($destination)
    $outputRoot=[IO.Path]::GetFullPath($buildOutput)+[IO.Path]::DirectorySeparatorChar
    if(-not $resolved.StartsWith($outputRoot,[StringComparison]::OrdinalIgnoreCase)){throw 'Build tree escaped selected flavor output'}
    if(Test-Path -LiteralPath $resolved){
        if((Get-Item -LiteralPath $resolved).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Refuse reparse build output'}
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
    New-Item -ItemType Directory -Path $resolved -Force | Out-Null
    Copy-Item -Path (Join-Path $source '*') -Destination $resolved -Recurse -Force
}
& cmake -S (Join-Path $repoRoot 'mod') -B (Join-Path $repoRoot $buildTree) -G 'Visual Studio 17 2022' -A Win32 -T v143 "-DREPENTOGON_ROOT=$RepentogonRoot" "-DISAACCS_BUILD_E2E_TOOLS=$e2eTools"
if($LASTEXITCODE) { throw 'IsaacCS configure failed' }
& cmake --build (Join-Path $repoRoot $buildTree) --config Release -- /m:4 /v:minimal
if($LASTEXITCODE) { throw 'IsaacCS build failed' }
if($E2E) {
& cargo build --manifest-path (Join-Path $repoRoot 'tools/harness/Cargo.toml') --release --locked --target i686-pc-windows-msvc --target-dir (Join-Path $repoRoot 'build/harness')
if($LASTEXITCODE) { throw 'Harness build failed' }
# 连续验收允许新编排器输出到另一文件，不覆盖仍在等待远端的房主进程。
if(-not $HarnessOutput){$HarnessOutput=Join-Path $buildOutput 'harness.exe'}
elseif(-not [IO.Path]::IsPathRooted($HarnessOutput)){$HarnessOutput=Join-Path $repoRoot $HarnessOutput}
New-Item -ItemType Directory -Path (Split-Path -Parent $HarnessOutput) -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot 'build/harness/i686-pc-windows-msvc/release/harness.exe') -Destination $HarnessOutput
}
Copy-Item -LiteralPath (Join-Path (Join-Path $repoRoot $buildTree) 'generated/hook-manifest.json') -Destination (Join-Path $buildOutput 'hook-manifest.json')
Copy-Item -LiteralPath (Join-Path (Join-Path $repoRoot $buildTree) 'generated/hook-manifest.md') -Destination (Join-Path $buildOutput 'hook-manifest.md')
# tx-04-08：布局清单（字段偏移/来源/RTTI/常量）与 Hook 清单一同落到产物目录。
Copy-Item -LiteralPath (Join-Path (Join-Path $repoRoot $buildTree) 'generated/world-layout.json') -Destination (Join-Path $buildOutput 'world-layout.json')
# 必需许可证跟随静态zstd发布；不需要另装zstd.dll。
Copy-Item -LiteralPath (Join-Path $repoRoot 'mod/third_party/zstd/LICENSE') -Destination (Join-Path $buildOutput 'zstd-LICENSE.txt')
if($E2E){
    $fixtureOutput=Join-Path $buildOutput 'isaac-cs-test'
    Copy-BuildTree (Join-Path $repoRoot 'tests/e2e-mods/isaac-cs-test') $fixtureOutput
    # 只在 E2E 包加载原版内容的 Lua 对照。
    Copy-BuildTree (Join-Path $repoRoot 'mod/observation/isaac-cs-observation') (Join-Path $buildOutput 'isaac-cs-observation')
}

$completedFiles=@()
foreach($file in Get-ChildItem -LiteralPath $buildOutput -File){$completedFiles+=@{name=$file.Name;bytes=$file.Length;modified_utc=$file.LastWriteTimeUtc.ToString('o')}}
@{format='isaac.cs.build.complete';version=1;flavor=if($E2E){'e2e'}else{'product'};files=$completedFiles;completed_utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $buildReceipt -Encoding utf8
