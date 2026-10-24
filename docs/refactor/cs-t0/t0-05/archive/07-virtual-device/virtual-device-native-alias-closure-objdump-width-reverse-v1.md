> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# CS-T0-05-05 objdump width mismatch — independent REVERSE diagnostic v1

Date: 2026-09-09  
Role: independent REVERSE  
Scope: read-only diagnosis of `SERVICE_GLOBAL_REFERENCE_ROW:SERVICE-GLOBAL-REF-0001` only  
Alias verdict impact: none; `UNKNOWN` and `dynamicAliasUniverseClosed=false` remain unchanged

## 1. Fixed identities and safety boundary

- fixed PE: `F:\SteamLibrary\steamapps\common\The Binding of Isaac Rebirth\isaac-ng.exe`
- PE SHA-256: `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`
- fixed `objdump`: `D:\x86_64-15.1.0-release-posix-seh-ucrt-rt_v12-rev0\mingw64\bin\objdump.exe`
- `objdump` SHA-256: `4769530BEB3AF2C2B61EC8BB45B7E4EDD63116E5CAF8B7C5188C11DB9D299064`
- banner: `GNU objdump (GNU Binutils) 2.44`
- image base: `0x00400000`

This diagnostic did not modify the manifest, verifier, decision, production code, PE, IDB, or any existing evidence. It did not enumerate, open, start, attach to, or otherwise access the game process, and did not use the game, x32dbg, or runtime observation. This create-new handoff is the only file publication resulting from the diagnosis.

## 2. Two read-only commands already executed

The first command ran the verifier-equivalent default-width whole-image disassembly once and retained only decoded lines containing the exact case-sensitive substring `0xc57b64`:

```powershell
& $objdump -d $gameExe |
    Select-String -SimpleMatch '0xc57b64' |
    ForEach-Object { $_.Line }
```

Purpose: reproduce the whole-`.text` service-global substring census, including its actual order and the exact raw/text columns emitted under the verifier's `objdump -d` invocation.

The second and only targeted disassembly covered VA `0x00402348..0x00402360`:

```powershell
& $objdump -d --start-address=0x402348 --stop-address=0x402360 $gameExe
```

Purpose: expose the raw-byte continuation immediately after the first matching line and determine whether the failure came from the PE bytes, decoded instruction text, or output formatting. No further `objdump` invocation was made for this diagnostic.

## 3. Actual whole-image substring results

The whole-`.text` substring filter returned exactly these four rows in this order:

```text
  40234c: c7 05 64 7b c5 00 5c  movl   $0xb9ee5c,0xc57b64
  924863: a1 64 7b c5 00        mov    0xc57b64,%eax
  924868: b9 64 7b c5 00        mov    $0xc57b64,%ecx
  b16c4d: b9 64 7b c5 00        mov    $0xc57b64,%ecx
```

After subtracting image base, their RVAs are:

1. `0x0000234C`
2. `0x00524863`
3. `0x00524868`
4. `0x00716C4D`

The count, order, RVAs, and decoded instruction texts agree with the four stored `serviceGlobalAliasCensus.decodedReferenceRows`.

## 4. Exact first-row output and continuation mechanism

The targeted output around RVA `0x0000234C` was:

```text
  40234c: c7 05 64 7b c5 00 5c  movl   $0xb9ee5c,0xc57b64
  402353: ee b9 00
  402356: c7 05 6c 7b c5 00 00  movl   $0x0,0xc57b6c
  40235d: 00 00 00
```

The fixed PE contains the complete ten-byte instruction at RVA `0x0000234C`:

```text
C7 05 64 7B C5 00 5C EE B9 00
```

GNU `objdump`'s default non-wide display wraps long byte columns at approximately 80 output columns. It therefore emits the first seven bytes with the decoded text at VA `0x0040234C`, followed by the remaining three bytes at VA `0x00402353` without instruction text.

The verifier regex parses each printed line independently. At lines 783–789 it constructs the first `Instruction` from only `C7 05 64 7B C5 00 5C`. The continuation line has empty text and is discarded by the `if not text: continue` branch. The parsed raw value for `SERVICE-GLOBAL-REF-0001` is consequently seven bytes even though the PE instruction and manifest evidence are correctly ten bytes.

## 5. Exact failed comparison

The checks at verifier lines 2316–2322 have these dispositions for the first zipped row:

- lines 2317–2318, instruction RVA: match (`0x0000234C`);
- line 2319, raw bytes: **mismatch**;
  - expected manifest value: `C7 05 64 7B C5 00 5C EE B9 00`;
  - observed parser value: `C7 05 64 7B C5 00 5C`;
- lines 2320–2321, whitespace-normalized and case-folded decode: match (`movl $0xb9ee5c,0xc57b64`).

No comment, symbol suffix, relocation annotation, or mnemonic/operand difference is present. The stored `independentDecodedInstruction` and the runtime `objdump` text are equal under the verifier's normalization. The thrown label `SERVICE_GLOBAL_REFERENCE_ROW:SERVICE-GLOBAL-REF-0001` is therefore caused specifically by line 2319.

## 6. Minimal repair recommendation

Do not truncate or otherwise change the manifest's correct ten-byte PE evidence. The minimal general repair is to invoke the same fixed tool in wide mode:

```python
[str(resolved), "-d", "-w", str(game_exe.resolve())]
```

GNU `objdump --help` defines `-w, --wide` as formatting output for more than 80 columns. Wide mode keeps a long instruction's complete byte sequence on its decoded line, allowing the existing regex and raw-byte comparison to observe all ten bytes. Whitespace is already normalized, so the extra output width does not require a stored decode-text change.

A more invasive alternative would buffer and merge byte-only continuation lines before constructing `Instruction`; that is not necessary for this minimum repair. A row-specific exception or evidence truncation would weaken the independent disk-byte check and is not recommended.

This is a verifier output-format repair only. It supplies no new alias closure evidence and does not change any receiver/pair frontier, drainability result, owner-through-drain result, formal decision, permission, or unlock.
