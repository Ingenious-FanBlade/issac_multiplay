param([string]$OutputPath=(Join-Path $PSScriptRoot 'final-static-review.json'))
$ErrorActionPreference='Stop'
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$reviewOutput=[IO.Path]::GetFullPath($OutputPath)
if(Test-Path -LiteralPath $reviewOutput){throw 'Preserve previous result; choose a new output path.'}
$checks=[Collections.Generic.List[object]]::new()
function Record-Check([string]$Name,[bool]$Pass,[string]$Detail){
    $checks.Add([pscustomobject]@{name=$Name;pass=$Pass;detail=$Detail})
}
function Read-Hex([string]$Value){[Convert]::ToInt64($Value.Substring(2),16)}
# Only explicitly labeled address claims are pairs; field/flag slash lists are not.
function Read-ExplicitClaims([string]$Body){
    $claims=[Collections.Generic.List[object]]::new()
    foreach($pattern in @('`0x([0-9a-fA-F]+)`\s*（RVA\s*`0x([0-9a-fA-F]+)`）',
        'VA\s*`0x([0-9a-fA-F]+)`[，,]\s*RVA\s*`0x([0-9a-fA-F]+)`')){
        foreach($m in [regex]::Matches($Body,$pattern)){
            $claims.Add([pscustomobject]@{va=('0x'+$m.Groups[1].Value);rva=('0x'+$m.Groups[2].Value)})
        }
    }
    $headers=@();$vaColumn=-1;$rvaColumn=-1;$compactColumns=@()
    foreach($line in ($Body -split "`n")){
        if(-not $line.Trim().StartsWith('|')){$headers=@();$vaColumn=-1;$rvaColumn=-1;$compactColumns=@();continue}
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
            foreach($m in [regex]::Matches($cells[$column],'`0x([0-9a-fA-F]+)\s*[/／]\s*0x([0-9a-fA-F]+)`')){
                $claims.Add([pscustomobject]@{va=('0x'+$m.Groups[1].Value);rva=('0x'+$m.Groups[2].Value)})
            }
        }
    }
    $claims.ToArray()
}
$newDocuments=@('docs/research/tx-03-18-single-sprite-adapter-contract.md',
    'docs/research/tx-03-18-primary-review.md','docs/research/tx-03-18-luna-resource-readiness.md')
$entryDocuments=@('docs/README.md','docs/hooks/README.md','docs/hooks/entity.md','docs/design/state-synchronization.md')
foreach($relative in ($newDocuments+$entryDocuments)){
    $path=Join-Path $repository $relative
    $exists=Test-Path -LiteralPath $path -PathType Leaf
    Record-Check "document:$relative" $exists 'Document exists.'
    if(-not $exists){continue}
    $body=Get-Content -LiteralPath $path -Raw -Encoding utf8
    if($relative -in $entryDocuments){
        $body=(($body -split "`n")|Where-Object {$_ -match 'tx-03-18'}) -join "`n"
        Record-Check "entry:$relative" ($body.Length -gt 0) 'Current addition present; old tx records retained.'
    }
    foreach($m in [regex]::Matches($body,'\[[^\]]*\]\(([^)]+)\)')){
        $target=$m.Groups[1].Value.Trim('<','>')
        if($target -match '^[a-zA-Z]+://' -or $target.StartsWith('#')){continue}
        $target=($target -split '#')[0]
        $absolute=[IO.Path]::GetFullPath((Join-Path ([IO.Path]::GetDirectoryName($path)) $target))
        Record-Check "link:$relative->$target" (($absolute -eq $reviewOutput) -or (Test-Path -LiteralPath $absolute)) 'Reference resolves; this result is created at review completion.'
    }
    $claims=@(Read-ExplicitClaims $body);$valid=$true
    foreach($claim in $claims){$valid=$valid -and (((Read-Hex $claim.va)-0x400000) -eq (Read-Hex $claim.rva))}
    Record-Check "document-addresses:$relative" $valid "Explicit VA/RVA claims: $($claims.Count); offsets are not interpreted as addresses."
}
$allJson=@{}
foreach($file in (Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.json' -File)){
    $value=Get-Content -LiteralPath $file.FullName -Raw -Encoding utf8|ConvertFrom-Json -Depth 100
    $allJson[$file.Name]=$value
    Record-Check "json:$($file.Name)" $true 'Readable JSON.'
    if($value.evidence -ne 'STATIC_IDB_READ_ONLY'){continue}
    $valid=($value.imagebase -eq '0x400000') -and ($value.functions.Count -gt 0)
    if($file.Name -eq 'luna-image-readiness.json'){
        Record-Check 'preserved-acquisition-error:old-tx-label' ($value.tx -eq 'tx-03-17') 'Known wrong tx label retained, not adopted as current tx metadata. Correct v2 evidence required below.'
    }else{$valid=$valid -and ($value.tx -eq 'tx-03-18')}
    foreach($fn in $value.functions){
        if($fn.error -or $fn.decompile_error -or -not $fn.va){$valid=$false}
        if($fn.va){$valid=$valid -and (((Read-Hex $fn.va)-0x400000) -eq (Read-Hex $fn.rva))}
        foreach($insn in $fn.instructions){$valid=$valid -and (((Read-Hex $insn.va)-0x400000) -eq (Read-Hex $insn.rva))}
    }
    foreach($table in $value.tables){
        $valid=$valid -and (((Read-Hex $table.va)-0x400000) -eq (Read-Hex $table.rva))
        foreach($entry in $table.entries){
            if($table.va -eq '0xb9ff84' -and $entry.offset -eq '0x60'){
                Record-Check "classified-adjacent-data:$($file.Name)" (($entry.bytes -eq 'b4 14 bb 00') -and ($entry.target_va -eq '0xbb14b4')) 'Pointer to adjacent ImageJpg RTTI CompleteObjectLocator, not a PNG virtual function slot.'
            }
            $valid=$valid -and (((Read-Hex $entry.target_va)-0x400000) -eq (Read-Hex $entry.target_rva))
        }
    }
    Record-Check "raw-addresses:$($file.Name)" $valid 'Static function/instruction/table pointers have consistent VA/RVA; known old metadata error classified separately.'
}
foreach($name in @('primary-pose-validation.json','primary-descriptor-origin.json','primary-mapping-bound.json',
    'primary-readiness-review.json','primary-acquisition-notes.json','luna-image-readiness.json','luna-image-readiness-v2.json')){
    Record-Check "evidence:$name" ($allJson.ContainsKey($name)) 'Required current evidence or preserved acquisition record exists.'
}
foreach($name in @('export_static.py','luna-export-static.py')){
    Record-Check "source:$name" (Test-Path -LiteralPath (Join-Path $PSScriptRoot $name) -PathType Leaf) 'Read-only acquisition source supplied; not executed by this review.'
}
$anchors=@(
    @('primary-mapping-bound.json','0x40bec0','0x40bec6','b9 40 00 00 00'),
    @('primary-mapping-bound.json','0x40bec0','0x40becb','8d 7a 38'),
    @('primary-mapping-bound.json','0x40bec0','0x40bf17','f3 ab'),
    @('primary-descriptor-origin.json','0x40ed50','0x413927','8d 86 08 01 00 00'),
    @('primary-descriptor-origin.json','0x40ed50','0x41397f','8b 8e 08 01 00 00'),
    @('primary-pose-validation.json','0x40a5d0','0x40a62d','81 c6 3c 01 00 00'),
    @('primary-pose-validation.json','0x40a5d0','0x40a65b','83 c1 30'),
    @('primary-pose-validation.json','0x40a5d0','0x40a668','e8 c3 e1 ff ff'),
    @('primary-pose-validation.json','0x40aaf0','0x40ab7b','83 c1 50'),
    @('primary-pose-validation.json','0x40aaf0','0x40ab88','e8 a3 dc ff ff'),
    @('primary-pose-validation.json','0x408830','0x4088dd','89 46 08'),
    @('primary-pose-validation.json','0x408830','0x40891d','89 46 0c'),
    @('primary-pose-validation.json','0x408830','0x408935','c7 46 10 00 00 80 bf'),
    @('primary-pose-validation.json','0x408830','0x40893c','c6 46 14 00'),
    @('primary-pose-validation.json','0x408830','0x408940','c7 46 18 00 00 00 00'),
    @('primary-pose-validation.json','0x408830','0x408947','c7 46 1c 00 00 00 00'),
    @('primary-pose-validation.json','0x40a030','0x40a060','ff 34 b0'),
    @('primary-pose-validation.json','0x409120','0x409188','3b 41 08'),
    @('primary-pose-validation.json','0x409120','0x4091a7','8b 01'),
    @('primary-pose-validation.json','0x409120','0x4091af','03 7a 7c'),
    @('primary-readiness-review.json','0x40c000','0x40c077','ff d0'),
    @('primary-readiness-review.json','0x40c000','0x40c080','c6 45 ef 01'),
    @('primary-readiness-review.json','0x40c000','0x40c106','ff d0'),
    @('primary-readiness-review.json','0x40c000','0x40c10a','74 2c'),
    @('primary-readiness-review.json','0x40c000','0x40c146','88 4c 03 30'),
    @('primary-readiness-review.json','0x408590','0x4085d2','ff 50 44'),
    @('primary-readiness-review.json','0x408590','0x40860b','ff 50 48'),
    @('primary-readiness-review.json','0xa123b0','0xa123b0','8b 41 08'),
    @('primary-readiness-review.json','0xa12b80','0xa12b85','83 4e 08 01'),
    @('primary-readiness-review.json','0xa69b30','0xa69bd6','b0 01')
)
foreach($anchor in $anchors){
    $fn=$allJson[$anchor[0]].functions|Where-Object va -eq $anchor[1]
    $insn=$fn.instructions|Where-Object va -eq $anchor[2]
    Record-Check "instruction:$($anchor[2])" ($insn.bytes -eq $anchor[3]) "Reviewed raw instruction in $($anchor[0])."
}
foreach($address in @('0x40c000','0x40c550','0x408590','0x40c3b0','0x40c6b0','0xa123b0','0xa12b80','0xa69b30')){
    $primary=$allJson['primary-readiness-review.json'].functions|Where-Object va -eq $address
    $child=$allJson['luna-image-readiness-v2.json'].functions|Where-Object va -eq $address
    $p=($primary.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    $c=($child.instructions|ForEach-Object {"$($_.va):$($_.bytes)"}) -join '|'
    Record-Check "independent-raw-function:$address" (($p.Length -gt 0) -and ($p -eq $c)) 'Direct comparison of independently exported raw instructions; no hash. Semantic review recorded separately.'
}
foreach($tableAddress in @('0xb9ff84','0xb1a6e0')){
    $primary=$allJson['primary-readiness-review.json'].tables|Where-Object va -eq $tableAddress
    $child=$allJson['luna-image-readiness-v2.json'].tables|Where-Object va -eq $tableAddress
    $p=($primary.entries|ForEach-Object {"$($_.offset):$($_.bytes)"}) -join '|'
    $c=($child.entries|Where-Object {$_.offset -ne '0x60'}|ForEach-Object {"$($_.offset):$($_.bytes)"}) -join '|'
    Record-Check "independent-vtable:$tableAddress" (($p.Length -gt 0) -and ($p -eq $c)) 'PNG 24 function slots or control six slots; adjacent RTTI data excluded.'
}
$failures=@($checks|Where-Object {-not $_.pass})
$result=[ordered]@{
    tx='tx-03-18';date='2026-10-23';timezone='Asia/Shanghai'
    result=$(if($failures.Count -eq 0){'PASS_STATIC_DOCUMENT_REVIEW_ONLY'}else{'FAIL_STATIC_DOCUMENT_REVIEW'})
    execution='One final unified static document review after all research, child review and edits. No intermediate tests or experiments.'
    agent=@{model='gpt-6-luna';reasoning_effort='max';role='Final image pair readiness research, independently reviewed by primary'}
    semantic_review='Primary independently read final pair assignment versus earlier loaded flag, retain and getter side effects, PNG logical loaded and fixed upload success, 64-entry descriptor mapping boundary, exact name selection, reset allocation and index writes, direct draw ordinal access, supported pose and owner/publication/retirement limitations. All interfaces remain design proposals.'
    acquisition_anomalies=@('Old Luna JSON retained with wrong tx-03-17 metadata; corrected v2 is current evidence',
        'Luna table read includes adjacent ImageJpg RTTI pointer at PNG+0x60; excluded from PNG function slots, correct primary 24-slot export retained',
        'Loose asset search returned no .anm2/.png; resources-dlc3 absent, original read error recorded')
    limitations=@('No runtime GPU/pixel/thread/context, failure or crash observation',
        'Trusted local fixed resource scope; no full parser safety, OOM, EH, reentry, workers or hot reload closure',
        'No adapter, concrete asset manifest, wire schema, registry, runtime guard or E2E implemented',
        'Past tx-03-16 FAIL retained; no old acceptance rerun',
        'No hashes computed or verified')
    checks=$checks.ToArray();failures=$failures
}
$stream=[IO.FileStream]::new($reviewOutput,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::Read)
$writer=[IO.StreamWriter]::new($stream,[Text.UTF8Encoding]::new($false))
try{$writer.Write(($result|ConvertTo-Json -Depth 20))}finally{$writer.Dispose()}
[pscustomobject]@{result=$result.result;checks=$checks.Count;failures=$failures.Count;output=$reviewOutput}|ConvertTo-Json
if($failures.Count -ne 0){exit 1}
