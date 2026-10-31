param(
    [ValidateSet('All','Build','Run','Combine')][string]$Stage='All',
    [ValidateSet('Local','Host','Client')][string]$NodeRole='Local',
    [string]$GameRoot='F:/SteamLibrary/steamapps/common/The Binding of Isaac Rebirth',
    [string]$HostAddress='127.0.0.1', [string]$Cohort='local',
    [ValidateRange(1,65535)][int]$HostPort=27090, [string]$ResumeFrom,
    [ValidateRange(1,3)][int]$PeerCount=3,
    [switch]$WaitForPairStart,
    [string]$TemporaryDirectory, [string[]]$NodeDirectories
)
$ErrorActionPreference='Stop'
$tx16Root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if($Stage -eq 'Combine') {
    # 合并同轮实际节点，拒绝用本机多进程或输入工具替代四台真实游戏设备。
    if($NodeDirectories.Count -ne ($PeerCount+1)){throw 'Combine requires Host and the selected number of real Client directories'}
    $tx16Rows=@($NodeDirectories | ForEach-Object {Get-Content -LiteralPath (Join-Path $_ 'result.json') -Raw | ConvertFrom-Json})
    if(@($tx16Rows | Where-Object {$_.pass -ne $true -or $_.tx -ne 'tx-04-16' -or $_.cohort -ne $Cohort -or $_.peer_count -ne $PeerCount}).Count){throw 'A node failed, is incomplete, or belongs to another cohort'}
    if(@($tx16Rows.machine | Sort-Object -Unique).Count -ne ($PeerCount+1)){throw 'Cross-device acceptance requires distinct device names'}
    $tx16Hosts=@($tx16Rows | Where-Object node_role -eq 'Host')
    $tx16Clients=@($tx16Rows | Where-Object node_role -eq 'Client')
    if($tx16Hosts.Count -ne 1 -or $tx16Clients.Count -ne $PeerCount){throw 'Expected one Host and the selected number of real game Clients'}
    if((@($tx16Clients.participant | Sort-Object) -join ',') -ne ((1..$PeerCount) -join ',')){throw 'Client participant identities conflict'}
    if(@($tx16Rows.wire_session | Sort-Object -Unique).Count -ne 1){throw 'Nodes did not participate in the same authority session'}
    Write-Output "tx-04-16 $($PeerCount+1)-device cohort PASS; native control groups, processed ACK and role retirement covered."
    # 节点目录由各设备本地入口清理；此处不跨设备递归删除未经核定的路径。
    return
}
if($Stage -in @('All','Build')) {
    & (Join-Path $PSScriptRoot 'build.ps1')
    if($LASTEXITCODE){throw 'tx-04-16 acceptance build failed'}
}
if($Stage -eq 'Build'){return}
if(-not $TemporaryDirectory){$TemporaryDirectory=Join-Path $tx16Root ('out/tx-04-16-temp-'+[guid]::NewGuid().ToString('N'))}
$tx16Temporary=[IO.Path]::GetFullPath($TemporaryDirectory)
$tx16Output=[IO.Path]::GetFullPath((Join-Path $tx16Root 'out'))+[IO.Path]::DirectorySeparatorChar
if(-not $tx16Temporary.StartsWith($tx16Output,[StringComparison]::OrdinalIgnoreCase)){throw 'TemporaryDirectory must stay inside repository out/'}
Write-Output "即将启动本机 $NodeRole 游戏，不会自动抢焦点。窗口出现后请手动选中游戏一次，随后停止键鼠操作。"
Write-Output '本轮实测请先手动切换到英文输入法；VM端中文输入法曾导致按键采样全中性。'
if($NodeRole -eq 'Host'){Write-Output '房主完成开局后可切到虚拟机；测试收尾返回菜单时，再手动选中房主窗口。失焦暂停选项在本轮临时关闭，结束后恢复。'}
$tx16Args=@('e2e','tx-04-16','--repo',$tx16Root,'--game',$GameRoot,'--artifact-dir',$tx16Temporary,'--node-role',$NodeRole,'--host-address',$HostAddress,'--host-port',"$HostPort",'--cohort',$Cohort,'--peer-count',"$PeerCount")
if($ResumeFrom){$tx16Args+=@('--resume-from',[IO.Path]::GetFullPath($ResumeFrom))}
if($WaitForPairStart){
    if($NodeRole -ne 'Host'){throw 'WaitForPairStart is only valid for Host'}
    $tx16Args+=@('--host-hold',(Join-Path $tx16Temporary 'start-pair.signal'))
}
& (Join-Path $tx16Root 'out/repentogon/harness.exe') @tx16Args
if($LASTEXITCODE){throw "tx-04-16 needs continuous diagnosis/continuation: $tx16Temporary"}
if($NodeRole -eq 'Local'){Write-Output 'Local lifecycle cases PASS; this does not replace the four-device cohort.'}
else{Write-Output "Node cases passed; temporary continuation material for cohort Combine: $tx16Temporary"}
