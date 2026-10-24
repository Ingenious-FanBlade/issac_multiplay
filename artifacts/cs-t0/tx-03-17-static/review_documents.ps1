param([string]$OutputPath=(Join-Path $PSScriptRoot 'final-static-review.json'))
$ErrorActionPreference='Stop'
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$reviewOutput=[IO.Path]::GetFullPath($OutputPath)
if(Test-Path -LiteralPath $reviewOutput){throw 'Preserve earlier results; use a new output path.'}
$checks=[Collections.Generic.List[object]]::new()
function Record-Check([string]$Name,[bool]$Pass,[string]$Detail){
    $checks.Add([pscustomobject]@{name=$Name;pass=$Pass;detail=$Detail})
}
function Read-Hex([string]$Value){[Convert]::ToInt64($Value.Substring(2),16)}
# Slash alone is not address semantics. Only explicit inline RVA labels or
# columns explicitly headed VA/RVA establish an address claim.
function Read-ExplicitClaims([string]$Body){
    $claims=[Collections.Generic.List[object]]::new()
    foreach($pattern in @('`0x([0-9a-fA-F]+)`\s*（RVA\s*`0x([0-9a-fA-F]+)`）',
        'VA\s*`0x([0-9a-fA-F]+)`[，,]\s*RVA\s*`0x([0-9a-fA-F]+)`')){
        foreach($m in [regex]::Matches($Body,$pattern)){
            $claims.Add([pscustomobject]@{va=('0x'+$m.Groups[1].Value);rva=('0x'+$m.Groups[2].Value);source='explicit-inline-label'})
        }
    }
    $headers=@();$vaColumn=-1;$rvaColumn=-1;$compactColumns=@()
    foreach($line in ($Body -split "`n")){
        if(-not $line.Trim().StartsWith('|')){
            $headers=@();$vaColumn=-1;$rvaColumn=-1;$compactColumns=@();continue
        }
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
            $v=[regex]::Match($cells[$vaColumn],'0x[0-9a-fA-F]+')
            $r=[regex]::Match($cells[$rvaColumn],'0x[0-9a-fA-F]+')
            if($v.Success -and $r.Success){$claims.Add([pscustomobject]@{va=$v.Value;rva=$r.Value;source='separate-labeled-columns'})}
        }
        foreach($column in $compactColumns){
            foreach($m in [regex]::Matches($cells[$column],'`0x([0-9a-fA-F]+)\s*[/／]\s*0x([0-9a-fA-F]+)`')){
                $claims.Add([pscustomobject]@{va=('0x'+$m.Groups[1].Value);rva=('0x'+$m.Groups[2].Value);source='compact-labeled-column'})
            }
        }
    }
    $claims.ToArray()
}
$newDocuments=@('docs/research/tx-03-17-independent-sprite-owner-lifecycle.md',
    'docs/research/tx-03-17-primary-review.md','docs/research/tx-03-17-luna-sprite-retirement.md')
$entryDocuments=@('docs/README.md','docs/hooks/README.md','docs/hooks/entity.md','docs/design/state-synchronization.md')
foreach($relative in ($newDocuments+$entryDocuments)){
    $path=Join-Path $repository $relative
    $exists=Test-Path -LiteralPath $path -PathType Leaf
    Record-Check "document:$relative" $exists 'Document exists.'
    if(-not $exists){continue}
    $body=Get-Content -LiteralPath $path -Raw -Encoding utf8
    if($relative -in $entryDocuments){
        $body=(($body -split "`n")|Where-Object {$_ -match 'tx-03-17'}) -join "`n"
        Record-Check "entry:$relative" ($body.Length -gt 0) 'Entry updated; prior tx records retained.'
    }
    foreach($m in [regex]::Matches($body,'\[[^\]]*\]\(([^)]+)\)')){
        $target=$m.Groups[1].Value.Trim('<','>')
        if($target -match '^[a-zA-Z]+://' -or $target.StartsWith('#')){continue}
        $target=($target -split '#')[0]
        $absolute=[IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($path)) $target))
        Record-Check "link:$relative->$target" (($absolute -eq $reviewOutput) -or (Test-Path -LiteralPath $absolute)) 'Local reference resolves; result artifact created at completion.'
    }
    $claims=@(Read-ExplicitClaims $body);$valid=$true
    foreach($claim in $claims){$valid=$valid -and (((Read-Hex $claim.va)-0x400000) -eq (Read-Hex $claim.rva))}
    Record-Check "document-addresses:$relative" $valid "Explicitly labeled VA/RVA claims: $($claims.Count). Bare fields/VA lists/flag values are not pairs."
}
# The real previous false-positive documents are classification examples in
# this tx's single review; the old review is neither rerun nor overwritten.
foreach($case in @(@('docs/research/tx-03-16-independent-sprite-display-fields.md',11),
    @('docs/research/tx-03-16-luna-sprite-fields.md',3))){
    $body=Get-Content -LiteralPath (Join-Path $repository $case[0]) -Raw -Encoding utf8
    $claims=@(Read-ExplicitClaims $body);$valid=$claims.Count -eq $case[1]
    foreach($claim in $claims){$valid=$valid -and (((Read-Hex $claim.va)-0x400000) -eq (Read-Hex $claim.rva))}
    Record-Check "address-classification:$($case[0])" $valid "Expected labeled claims=$($case[1]); observed=$($claims.Count). Prior FAIL remains unchanged."
}
$allJson=@{}
foreach($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.json' -File)){
    $value=Get-Content -LiteralPath $file.FullName -Raw -Encoding utf8|ConvertFrom-Json -Depth 100
    $allJson[$file.Name]=$value
    Record-Check "json:$($file.Name)" $true 'Readable JSON.'
    if($value.evidence -ne 'STATIC_IDB_READ_ONLY'){continue}
    $base=if($value.imagebase){$value.imagebase}else{$value.database.imagebase}
    $valid=($base -eq '0x400000') -and ($value.tx -eq 'tx-03-17') -and ($value.functions.Count -gt 0)
    foreach($fn in $value.functions){
        if($fn.error -or $fn.decompile_error -or $fn.pseudocode_error -or -not $fn.va){$valid=$false}
        if($fn.va){$valid=$valid -and (((Read-Hex $fn.va)-0x400000) -eq (Read-Hex $fn.rva))}
        foreach($insn in $fn.instructions){$valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))}
    }
    foreach($table in $value.tables){
        $valid=$valid -and (((Read-Hex $table.va)-0x400000) -eq (Read-Hex $table.rva))
        foreach($entry in $table.entries){
            if($file.Name -eq 'primary-retirement-review.json' -and $table.va -eq '0xb73228' -and $entry.offset -eq '0x4'){
                Record-Check 'classified-vtable-overread:B7322C' ($entry.bytes -eq '49 73 42 75') 'Adjacent IsBusted string bytes, not a pointer slot. Mechanical target subtraction has no address meaning.'
                continue
            }
            $valid=$valid -and (((Read-Hex $entry.target_va)-0x400000) -eq (Read-Hex $entry.target_rva))
        }
    }
    Record-Check "addresses:$($file.Name)" $valid 'Static function/instruction and explicitly classified pointer-table addresses agree.'
}
foreach($name in @('primary-owner-initial.json','primary-owner-transfer.json','primary-copy-construction.json',
    'primary-pose-staging.json','primary-retirement-review.json','primary-userdata-owner.json',
    'primary-control-retirement.json','primary-allocation-wrapper.json','primary-acquisition-notes.json','luna-retirement-static.json')){
    Record-Check "evidence:$name" ($allJson.ContainsKey($name)) 'Required static evidence/acquisition record exists.'
}
foreach($name in @('export_static.py','luna-export-retirement.py')){
    Record-Check "source:$name" (Test-Path -LiteralPath (Join-Path $PSScriptRoot $name) -PathType Leaf) 'Read-only acquisition source supplied, not executed by this review.'
}
$anchors=@(
    @('primary-owner-initial.json','0x8bb720','0x8bb728','68 1c 01 00 00'),
    @('primary-owner-initial.json','0x8bb720','0x8bb744','c7 06 28 32 b7 00'),
    @('primary-owner-initial.json','0x8bb720','0x8bb74a','89 4e 04'),
    @('primary-owner-initial.json','0x8bb720','0x8bb756','ff 15 98 83 b1 00'),
    @('primary-owner-initial.json','0x8bb720','0x8bb762','e8 59 bd b4 ff'),
    @('primary-owner-initial.json','0x4074c0','0x407539','e8 f2 12 00 00'),
    @('primary-owner-initial.json','0x4074c0','0x407557','e8 d4 12 00 00'),
    @('primary-owner-initial.json','0x4074c0','0x40766d','e8 9e 08 00 00'),
    @('primary-owner-initial.json','0x407ae0','0x407e4a','89 b4 08 64 ff ff ff'),
    @('primary-owner-initial.json','0x407ae0','0x407e6f','89 94 06 64 ff ff ff'),
    @('primary-owner-transfer.json','0x407e90','0x407eb8','e8 23 fc ff ff'),
    @('primary-owner-transfer.json','0x407e90','0x407efc','c2 14 01'),
    @('primary-copy-construction.json','0x407690','0x407715','88 47 70'),
    @('primary-copy-construction.json','0x407690','0x407767','c7 87 98 00 00 00 00 00 00 00'),
    @('primary-copy-construction.json','0x407690','0x407771','c7 87 94 00 00 00 00 00 00 00'),
    @('primary-copy-construction.json','0x407690','0x407857','c7 87 10 01 00 00 00 00 00 00'),
    @('primary-copy-construction.json','0x407690','0x407866','e8 a5 68 00 00'),
    @('primary-retirement-review.json','0x408170','0x408196','e8 75 fd ff ff'),
    @('primary-retirement-review.json','0x408170','0x40819e','e8 fd 05 00 00'),
    @('primary-retirement-review.json','0x408170','0x4081a6','e8 f5 05 00 00'),
    @('primary-retirement-review.json','0x8c31f0','0x8c3207','6a 00'),
    @('primary-retirement-review.json','0x8c31f0','0x8c3209','ff 12'),
    @('primary-retirement-review.json','0x8c4ce0','0x8c4d16','e8 f5 31 b4 ff'),
    @('primary-retirement-review.json','0x8c4ce0','0x8c4d67','c2 04 00'),
    @('primary-allocation-wrapper.json','0xaef15c','0xaef162','e8 99 03 f2 ff'),
    @('primary-allocation-wrapper.json','0xa0f500','0xa0f52b','e8 80 53 05 00')
)
foreach($anchor in $anchors){
    $fn=$allJson[$anchor[0]].functions|Where-Object va -eq $anchor[1]
    $insn=$fn.instructions|Where-Object va -eq $anchor[2]
    Record-Check "instruction:$($anchor[2])" ($insn.bytes -eq $anchor[3]) "Reviewed raw instruction in $($anchor[0])."
}
$shared=@(@('primary-owner-initial.json','0x4074c0'),@('primary-owner-initial.json','0x8bb720'),
    @('primary-owner-initial.json','0x40bd50'),@('primary-pose-staging.json','0x40c000'),
    @('primary-retirement-review.json','0x407f10'),@('primary-retirement-review.json','0x408170'),
    @('primary-retirement-review.json','0x4087a0'),@('primary-retirement-review.json','0x407a60'),
    @('primary-retirement-review.json','0x8c31f0'),@('primary-retirement-review.json','0x8c4ce0'),
    @('primary-retirement-review.json','0x8a71b0'),@('primary-userdata-owner.json','0x85c3e0'))
foreach($item in $shared){
    $primary=$allJson[$item[0]].functions|Where-Object va -eq $item[1]
    $child=$allJson['luna-retirement-static.json'].functions|Where-Object va -eq $item[1]
    $p=($primary.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    $c=($child.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    Record-Check "independent-function:$($item[1])" (($p.Length -gt 0) -and ($p -eq $c)) 'Independent raw instructions agree directly; no hash. Semantic reasoning reviewed separately.'
}
$table=$allJson['primary-userdata-owner.json'].tables|Where-Object va -eq '0xb73228'
$entry=$table.entries|Where-Object offset -eq '0x0'
$childTable=$allJson['luna-retirement-static.json'].userdata_vtable
Record-Check 'userdata-vtable:slot0' (($entry.target_va -eq '0x8c4ce0') -and ($entry.bytes -eq 'e0 4c 8c 00')) 'Correct one-slot table and entry target.'
Record-Check 'independent-userdata-vtable:slot0' (($childTable.va -eq $table.va) -and ($childTable.slot0_target_va -eq $entry.target_va) -and ($childTable.slot0_bytes -eq $entry.bytes)) 'Child and primary table evidence agree.'
Record-Check 'userdata-vtable:RVA' ((((Read-Hex $childTable.va)-0x400000) -eq (Read-Hex $childTable.rva)) -and (((Read-Hex $childTable.slot0_target_va)-0x400000) -eq (Read-Hex $childTable.slot0_target_rva))) 'Child table base and target VA/RVA have explicit pointer meaning.'
$failures=@($checks|Where-Object {-not $_.pass})
$result=[ordered]@{
    tx='tx-03-17';date='2026-10-23';timezone='Asia/Shanghai'
    result=$(if($failures.Count -eq 0){'PASS_STATIC_DOCUMENT_REVIEW_ONLY'}else{'FAIL_STATIC_DOCUMENT_REVIEW'})
    execution='One final unified static document review after substantive owner research, child review and edits; no other tests/experiments.'
    agent=@{model='gpt-6-luna';reasoning_effort='max';role='Retirement and Lua GC researched by child and independently reviewed by primary'}
    semantic_review='Primary read constructor, copy, swap field omissions and actual transfer callers, staging and bounded failures, standalone versus wrapper destructors, control release versus optional callback, global defaults and allocation ownership. Candidate publication sequence is a design proposal.'
    prior_tx0316='FAIL result preserved unchanged. New classifier uses explicit labels/table headers; two old documents are classification examples only, not a rerun of the prior review.'
    acquisition_anomaly='Primary initially read one word beyond B73228 vtable into IsBusted string. Raw evidence retained, adjacent word explicitly classified as string data, correct slot-only export supplied.'
    limitations=@('No runtime resources, pixels, corruption, crash, thread or context observation',
        'Normal constructor/helper returns only; OOM, EH, cache topology changes and reentry not closed',
        'Lua outer allocation freed by Lua; raw native owner allocator not inferred from a destructor',
        'No adapter, registry, readiness predicate, wire schema or E2E implemented',
        'No hash computation or verification')
    checks=$checks.ToArray();failures=$failures
}
$stream=[IO.FileStream]::new($reviewOutput,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
try{$writer.Write(($result|ConvertTo-Json -Depth 20))}finally{$writer.Dispose()}
[pscustomobject]@{result=$result.result;checks=$checks.Count;failures=$failures.Count;output=$reviewOutput}|ConvertTo-Json
if($failures.Count -ne 0){exit 1}
