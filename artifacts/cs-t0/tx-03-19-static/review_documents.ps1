param([string]$OutputPath=(Join-Path $PSScriptRoot 'final-static-review.json'))
$ErrorActionPreference='Stop'
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$reviewOutput=[IO.Path]::GetFullPath($OutputPath)
if(Test-Path -LiteralPath $reviewOutput){throw 'Preserve previous results; choose a new output path.'}
$checks=[Collections.Generic.List[object]]::new()
function Record-Check([string]$Name,[bool]$Pass,[string]$Detail){
    $checks.Add([pscustomobject]@{name=$Name;pass=$Pass;detail=$Detail})
}
function Read-Hex([string]$Value){[Convert]::ToInt64($Value.Substring(2),16)}
function Read-ExplicitClaims([string]$Body){
    $plain=$Body.Replace('**','').Replace('`','')
    $claims=[Collections.Generic.List[object]]::new()
    foreach($pattern in @('0x([0-9a-fA-F]+)\s*（RVA\s*0x([0-9a-fA-F]+)',
        '0x([0-9a-fA-F]+)\s*[/／]\s*RVA\s*0x([0-9a-fA-F]+)',
        'VA\s*0x([0-9a-fA-F]+)[，,]\s*RVA\s*0x([0-9a-fA-F]+)')){
        foreach($m in [regex]::Matches($plain,$pattern)){
            $claims.Add([pscustomobject]@{va=('0x'+$m.Groups[1].Value);rva=('0x'+$m.Groups[2].Value)})
        }
    }
    $vaColumn=-1;$rvaColumn=-1;$compactColumns=@();$headers=@()
    foreach($line in ($plain -split "`n")){
        if(-not $line.Trim().StartsWith('|')){$vaColumn=-1;$rvaColumn=-1;$compactColumns=@();$headers=@();continue}
        $cells=@($line.Trim().Trim('|').Split('|')|ForEach-Object {$_.Trim()})
        $newVa=-1;$newRva=-1;$newCompact=@()
        for($i=0;$i -lt $cells.Count;$i++){
            if($cells[$i] -eq 'VA'){$newVa=$i}
            if($cells[$i] -eq 'RVA'){$newRva=$i}
            if($cells[$i] -match '\bVA\s*[/／]\s*RVA\b'){$newCompact+=,$i}
        }
        if(($newVa -ge 0 -and $newRva -ge 0) -or $newCompact.Count -gt 0){
            $headers=$cells;$vaColumn=$newVa;$rvaColumn=$newRva;$compactColumns=$newCompact;continue
        }
        if($headers.Count -ne $cells.Count){continue}
        if($vaColumn -ge 0 -and $rvaColumn -ge 0){
            $v=[regex]::Match($cells[$vaColumn],'0x[0-9a-fA-F]+');$r=[regex]::Match($cells[$rvaColumn],'0x[0-9a-fA-F]+')
            if($v.Success -and $r.Success){$claims.Add([pscustomobject]@{va=$v.Value;rva=$r.Value})}
        }
        foreach($column in $compactColumns){
            foreach($m in [regex]::Matches($cells[$column],'0x([0-9a-fA-F]+)\s*[/／]\s*0x([0-9a-fA-F]+)')){
                $claims.Add([pscustomobject]@{va=('0x'+$m.Groups[1].Value);rva=('0x'+$m.Groups[2].Value)})
            }
        }
    }
    $claims.ToArray()
}
$newDocuments=@('docs/research/tx-03-19-sprite-render-entry-window.md',
    'docs/research/tx-03-19-primary-review.md','docs/research/tx-03-19-luna-render-window-guard.md')
$entryDocuments=@('docs/README.md','docs/hooks/README.md','docs/hooks/entity.md','docs/hooks/game-loop.md','docs/design/state-synchronization.md')
foreach($relative in ($newDocuments+$entryDocuments)){
    $path=Join-Path $repository $relative
    $exists=Test-Path -LiteralPath $path -PathType Leaf
    Record-Check "document:$relative" $exists 'Document exists.'
    if(-not $exists){continue}
    $body=Get-Content -LiteralPath $path -Raw -Encoding utf8
    if($relative -in $entryDocuments){
        $body=(($body -split "`n")|Where-Object {$_ -match 'tx-03-19'}) -join "`n"
        Record-Check "entry:$relative" ($body.Length -gt 0) 'Current research addition present; historical entries retained.'
    }
    foreach($m in [regex]::Matches($body,'\[[^\]]*\]\(([^)]+)\)')){
        $target=$m.Groups[1].Value.Trim('<','>')
        if($target -match '^[a-zA-Z]+://' -or $target.StartsWith('#')){continue}
        $target=($target -split '#')[0]
        $absolute=[IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($path)) $target))
        Record-Check "link:$relative->$target" (($absolute -eq $reviewOutput) -or (Test-Path -LiteralPath $absolute)) 'Local reference resolves; current result artifact created at completion.'
    }
    $claims=@(Read-ExplicitClaims $body);$valid=$true
    foreach($claim in $claims){$valid=$valid -and (((Read-Hex $claim.va)-0x400000) -eq (Read-Hex $claim.rva))}
    Record-Check "document-addresses:$relative" $valid "Explicitly labeled VA/RVA claims: $($claims.Count); bold/code markup supported, bare field/VA lists excluded."
}
$allJson=@{}
foreach($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.json' -File)){
    $value=Get-Content -LiteralPath $file.FullName -Raw -Encoding utf8|ConvertFrom-Json -Depth 100
    $allJson[$file.Name]=$value
    Record-Check "json:$($file.Name)" $true 'Readable JSON.'
    if($value.evidence -ne 'STATIC_IDB_READ_ONLY'){continue}
    $valid=($value.imagebase -eq '0x400000') -and ($value.tx -eq 'tx-03-19') -and ($value.functions.Count -gt 0)
    foreach($fn in $value.functions){
        if($file.Name -eq 'luna-render-window-guard.json' -and $fn.requested_va -eq '0xa8fb7f'){
            Record-Check 'preserved-acquisition-failure:A8FB7F' ((-not $fn.va) -and ($fn.error -eq 'No IDA function at requested address')) 'Failed function request retained; only the followup range is used for instruction evidence.'
            continue
        }
        if($file.Name -eq 'luna-render-window-guard.json' -and $fn.requested_va -eq '0xa804b0'){
            Record-Check 'classified-internal-address:A804B0' ($fn.va -eq '0xa80010') 'Internal request resolves to A80010; not an additional unique function.'
        }
        if($fn.error -or $fn.decompile_error -or -not $fn.va){$valid=$false}
        if($fn.va){$valid=$valid -and (((Read-Hex $fn.va)-0x400000) -eq (Read-Hex $fn.rva))}
        foreach($insn in $fn.instructions){$valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))}
    }
    foreach($range in $value.ranges){
        foreach($insn in $range.instructions){$valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))}
    }
    foreach($record in (@($value.xref_records)+@($value.symbols))){
        if($record.va -and $record.rva){$valid=$valid -and (((Read-Hex $record.va)-0x400000) -eq (Read-Hex $record.rva))}
    }
    Record-Check "raw-addresses:$($file.Name)" $valid 'Resolved function/instruction/range/data addresses agree; explicitly retained failed request is not claimed as a complete function export.'
}
$required=@('primary-render-pass.json','primary-submit-flush.json','primary-batch-ownership.json',
    'primary-batch-bind-state.json','primary-context-guard-review.json','primary-guard-detail.json',
    'primary-loop-source.json','primary-acquisition-notes.json','luna-render-window-guard.json','luna-guard-followup.json')
foreach($name in $required){Record-Check "evidence:$name" ($allJson.ContainsKey($name)) 'Required static evidence or preserved acquisition record exists.'}
foreach($name in @('export_static.py','luna-export-render-window.py')){
    Record-Check "source:$name" (Test-Path -LiteralPath (Join-Path $PSScriptRoot $name) -PathType Leaf) 'Read-only acquisition source supplied; not executed by this review.'
}
$follow=$allJson['luna-guard-followup.json']
$unique=@($follow.functions.va | Sort-Object -Unique)
Record-Check 'followup-function-counts' (($follow.functions.Count -eq 12) -and ($unique.Count -eq 12) -and ($follow.counts.unresolved_function_seed_count -eq 0)) 'Twelve actual unique functions, zero unresolved function requests.'
Record-Check 'followup-other-counts' (($follow.ranges.Count -eq 5) -and ($follow.xref_records.Count -eq 8) -and ($follow.symbols.Count -eq 9) -and ($follow.tables.Count -eq 0)) 'Five ranges, eight xref records, nine symbols, no inferred virtual table.'
$snapshot=$allJson['primary-loop-source.json']
$currentSource=Get-Content -LiteralPath (Join-Path $repository $snapshot.source) -Encoding utf8
foreach($section in $snapshot.sections){
    $valid=$section.lines.Count -eq ($section.end_line-$section.start_line+1)
    foreach($line in $section.lines){$valid=$valid -and ($currentSource[$line.line-1] -ceq $line.text)}
    Record-Check "source-excerpt:$($section.start_line)-$($section.end_line)" $valid 'One-based source excerpt matches current source directly; no hash.'
}
$anchors=@(
    @('primary-context-guard-review.json','0x931050','0x9312f1','e8 ca 42 02 00'),
    @('primary-context-guard-review.json','0x8fb120','0x8fc32c','e8 8f 92 05 00'),
    @('primary-guard-detail.json','0x9aa040','0x9aaae3','e8 38 06 f5 ff'),
    @('primary-render-pass.json','0x9555c0','0x9560c6','ff 50 14'),
    @('primary-render-pass.json','0x9555c0','0x9560c9','6a 01'),
    @('primary-render-pass.json','0x9555c0','0x9560cb','b9 e0 98 c7 00'),
    @('primary-render-pass.json','0x9555c0','0x9560d0','e8 ab 30 0c 00'),
    @('primary-render-pass.json','0xa19180','0xa191a0','e8 4b a8 ff ff'),
    @('primary-render-pass.json','0xa19180','0xa1929d','83 7e 68 00'),
    @('primary-render-pass.json','0xa19180','0xa19312','8b 86 30 02 00 00'),
    @('primary-render-pass.json','0xa19180','0xa19319','ff d0'),
    @('primary-render-pass.json','0xa19180','0xa1933c','c2 04 00'),
    @('primary-render-pass.json','0x40a0d0','0x40a133','c2 0c 00'),
    @('primary-context-guard-review.json','0xa7fce0','0xa7fd99','a3 50 5d c7 00'),
    @('primary-context-guard-review.json','0xa7fce0','0xa7fdab','a3 54 5d c7 00'),
    @('primary-context-guard-review.json','0xa7fce0','0xa7fe68','ff 15 50 5d c7 00'),
    @('primary-context-guard-review.json','0xa7fce0','0xa7fe71','ff 15 54 5d c7 00'),
    @('primary-context-guard-review.json','0xa18e90','0xa190f9','83 0d 84 79 c3 00 01'),
    @('primary-context-guard-review.json','0xa18e90','0xa19102','83 4e 04 01'),
    @('primary-batch-bind-state.json','0xa66e30','0xa66eab','3b cf'),
    @('primary-batch-bind-state.json','0xa66e30','0xa66ee9','e8 62 56 9a ff'),
    @('primary-batch-bind-state.json','0xa66e30','0xa66f74','8d 4e 08'),
    @('primary-batch-bind-state.json','0xa66e30','0xa66f77','e8 34 54 9a ff'),
    @('primary-batch-bind-state.json','0xa66e30','0xa6701f','a1 b8 79 c3 00'),
    @('primary-batch-bind-state.json','0xa66e30','0xa67024','89 46 24'),
    @('primary-submit-flush.json','0xa139f0','0xa13a32','e8 99 41 05 00'),
    @('primary-submit-flush.json','0xa139f0','0xa13a8b','89 b3 84 00 00 00'),
    @('primary-batch-ownership.json','0xa67bd0','0xa67e41','ff 15 3c f9 c0 00'),
    @('primary-batch-ownership.json','0xa67bd0','0xa67e8a','80 66 04 fc'),
    @('primary-submit-flush.json','0x409120','0x409873','e8 d8 a7 60 00'),
    @('primary-render-pass.json','0xa14050','0xa140a8','a3 b8 79 c3 00')
)
foreach($anchor in $anchors){
    $fn=$allJson[$anchor[0]].functions|Where-Object va -eq $anchor[1]
    $insn=$fn.instructions|Where-Object va -eq $anchor[2]
    Record-Check "instruction:$($anchor[2])" ($insn.bytes -eq $anchor[3]) "Reviewed raw instruction in $($anchor[0])."
}
$rangeInsn=$allJson['primary-context-guard-review.json'].ranges.instructions|Where-Object va -eq '0xa8fb7f'
Record-Check 'resolver-write:A8FB7F' ($rangeInsn.bytes -eq 'a3 e0 35 c1 00') 'Known instruction range writes epoxy pointer; not claimed as a function.'
$shared=@(@('primary-render-pass.json','0x9555c0'),
    @('primary-context-guard-review.json','0x931050'),@('primary-context-guard-review.json','0x8fb120'),
    @('primary-context-guard-review.json','0xa18e90'),@('primary-context-guard-review.json','0xa19df0'),
    @('primary-context-guard-review.json','0xa6bbb0'),@('primary-context-guard-review.json','0xa7fa50'),
    @('primary-context-guard-review.json','0xa7fce0'),@('primary-context-guard-review.json','0xa80010'),
    @('primary-guard-detail.json','0x9aa040'),@('primary-guard-detail.json','0xa6a420'),@('primary-guard-detail.json','0xa6b570'))
foreach($item in $shared){
    $primary=$allJson[$item[0]].functions|Where-Object va -eq $item[1]
    $child=$follow.functions|Where-Object va -eq $item[1]
    $p=($primary.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    $c=($child.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    Record-Check "independent-function:$($item[1])" (($p.Length -gt 0) -and ($p -eq $c)) 'Independent raw instruction exports compared directly; no hash. Semantic review recorded separately.'
}
$failures=@($checks|Where-Object {-not $_.pass})
$result=[ordered]@{
    tx='tx-03-19';date='2026-10-23';timezone='Asia/Shanghai'
    result=$(if($failures.Count -eq 0){'PASS_STATIC_DOCUMENT_REVIEW_ONLY'}else{'FAIL_STATIC_DOCUMENT_REVIEW'})
    execution='One final unified static document review after research, child review and all edits. No intermediate tests, builds or experiments.'
    agent=@{model='gpt-6-luna';reasoning_effort='max';role='Hook/TLS/WGL guard research independently reviewed by primary'}
    semantic_review='Primary independently read final pre-present callsite and ABI, original exactly-once/source phase and healthy limitations, WGL/TLS versus OS current, distinct cache/renderer init flags, batch cache-derived strong image pair, shader/uniform snapshots, delayed GL submission and bounded default-pass restoration. All hooks and guards remain proposals.'
    acquisition_anomalies=@('First child export has an unresolved function request A8FB7F and internal-address duplicate A804B0->A80010; retained and classified, corrected 12-function followup adopted',
        'A8FB7F is an epoxy pointer resolver write in an instruction range, not a TLS getter',
        'Windows rg unexpanded wildcard path error retained; exact filename subsequently read',
        'Mixed parallel-VA table header corrected; real labeled address pairs preserved')
    limitations=@('No runtime thread/context/API/pass/pixel/GPU or failure observations',
        'Screen overlay candidate only; room occlusion, arbitrary shaders/backends and external raw-GL changes not covered',
        'Normal sequence only; OOM, EH, reentry/worker/hot-reload closure incomplete',
        'No adapter, Hook, runtime guard, registry, asset manifest or E2E implemented',
        'No hash computation/verification; no prior acceptance rerun')
    checks=$checks.ToArray();failures=$failures
}
$stream=[IO.FileStream]::new($reviewOutput,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
try{$writer.Write(($result|ConvertTo-Json -Depth 20))}finally{$writer.Dispose()}
[pscustomobject]@{result=$result.result;checks=$checks.Count;failures=$failures.Count;output=$reviewOutput}|ConvertTo-Json
if($failures.Count -ne 0){exit 1}
