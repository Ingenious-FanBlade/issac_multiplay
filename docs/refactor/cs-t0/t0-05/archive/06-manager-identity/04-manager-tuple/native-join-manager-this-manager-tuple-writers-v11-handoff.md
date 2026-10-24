> tx-01-44 归档（2026-10-16）：本页正文中的“当前”“下一步”和旧流程只代表记录当时。执行要求以[现行项目规范](../../../../../project-conventions.md)为准；今天的功能状态见[T0-05 当前入口](../../../README.md)。归档仅调整 Markdown 链接位置，历史事实、失败、反例和证据标签保留。

# T0-05-04 manager-tuple writers v11 handoff

## Disposition

`0x005B91EF` remains `UNKNOWN`. This round creates a finite, reproducible
alias-consumer/writer manifest for `DWORD[0x00C71678] + 0x1BAA8`, but it does
not claim complete writer closure and does not permit manager-this promotion.

The work was static and read-only: the game/process was not started or read,
no debugger was attached, and the IDB, production sources, prior evidence,
tests, index, and status documents were not modified.

## Frozen bindings

- critical-two-v10 JSON:
  `0A5834505E79B4575A2DF53C86FE647E3F6A83E3E8FD4328369C11CC257A65FF`
- copy-range-bounds-v7 JSON:
  `C07EA49809BC17DC5F9F9E8384BD8686E5300534EEBBE2CC20891BC133625D8D`
- reachable whole-image indirect-split-component-v2 JSON:
  `D1648905B0F2279CAC6A60B98DBE035F57340542EA9A08E02C8D762D33410981`
- component-v2 analyzer:
  `A5B09328D4B86B7B0F6B772980260FFA4D33722C06CB9C3BD9FCBC71231EACB6`
- frozen target image:
  `1FBA9A37BCEBF9276C9A80372E8C4DCAD59C8F652D2FD41727B396DCDCD6B89C`

The finite reachability input contains 5,646 functions.

## Classifier result

The analyzer distinguishes discovery candidates from alias evidence. The old
IDA immediate search count of 495 is recorded but is not accepted as closure.
The actual decoded-operand inventory contains 692 candidate instructions in
192 reachable functions.

Starting only from a decoded load of the frozen GameRoot slot, the bounded
classifier follows `MOV`, `LEA`, `ADD`, `SUB`, EBP/ESP spills, ECX forwarding,
and pushed arguments inside an IDA basic block. It stops at block boundaries
instead of guessing a merge and manifests every live-out alias.

- proven block-local alias formations: 389
- consumer/forwarder manifest rows: 303
  - block-local call/jump rows: 296
  - explicitly closed critical transitive rows: 7
- consumer functions represented by the block-local rows: 102
- block-local forward targets: 39
- primary manager-tuple writer functions: 6
- allocator/install support writer functions: 2
- unclassified source callsites: 282
- unclassified helper target equivalence classes: 36
- unclassified block-live-out contexts: 88

The JSON includes all 692 decoded candidates, all 389 alias formations, all
296 block-local consumer callsites, the seven critical transitive edges, the
282 unclassified source callsite addresses, and the 88 live-out contexts.

## Closed critical writer paths

The following six primary writers are classified at tuple-field granularity:

1. `sub_6F1020`: constructor zeroes begin/end/capacityEnd at
   `0x6F161D/0x6F1627/0x6F1631`.
2. `sub_9B8EA0 -> sub_4149D0`: `0xAFB562` forms
   `GameRoot+0x1BAA8`, `0xAFB568` tail-forwards it, `0x9B8EC6` sets
   end=begin, and the `0x9B8F37` tail call frees the prior allocation and
   zeroes begin/end/capacityEnd at `0x414A02/0x414A0B/0x414A12`.
3. `sub_9B8F40`: the `0x42A637` slot load and
   `0x42A63D/0x42A65A` tuple LEAs reach the `0x42A66C` call. In-place append
   changes only end; the empty-vector growth edge is the `0x9B901A` call to
   `sub_42C920`.
4. `sub_9B9150`: the exact `0x9B91EF` memmove is followed by the
   `0x9B91F4` end decrement; begin and capacityEnd are unchanged.
5. `sub_9B9CD0`: in-place append changes only end and the actual manager
   tuple growth is `0x9B9EBA -> sub_42C920`.
6. `sub_9BA000`: the manager tuple erase is `0x9BA5F9` plus the
   `0x9BA5FE` end decrement; begin and capacityEnd are unchanged.

Two supporting functions close the known begin-changing growth relation:

- `sub_42C920` allocates `4 * newCapacity` bytes through `sub_40CF00` and
  forwards the allocation, size, and capacity at `0x42C9E3`.
- `sub_41D150` frees the prior backing and installs
  `[begin, begin+4*size, begin+4*capacity]` at
  `0x41D18E/0x41D193/0x41D19C`.

The v10 correction is retained: `0x9B9D82` grows the distinct vector at
receiver `+0xD0/+0xD4/+0xD8`; it is not a manager-tuple begin writer.
Likewise, `sub_9BA000`'s `0x9BA135/0x9BA13A` path erases that nested vector,
whereas `0x9BA5F9/0x9BA5FE` is the manager tuple path.

## Minimum unclassified helper-target set

The 282 unresolved source callsites collapse to these 36 helper contexts:

```text
0x00417840  0x0044BFB0  0x004F8940  0x0060D180
0x009B92C0  0x009B9310  0x009B9380  0x009B9450
0x009B9480  0x009B94E0  0x009B98E0  0x009BAAA0
0x009BAC00  0x009BB840  0x009BB960  0x009BBA30
0x009BC000  0x009BC120  0x009BC690  0x009BE080
0x009BE140  0x009BE630  0x009BE6B0  0x009BE750
0x009BEA10  0x009BEA40  0x009BEA80  0x009BEB30
0x009BEBA0  0x009BEE80  0x009BF530  0x009BF930
0x009BF990  0x009BFA00  0x009BFA70  0x009BFB20
```

This target list, together with the 88 explicitly manifested block live-outs,
is the next-round frontier. Until each helper context is classified at
begin/end/capacity and transitive-forwarder granularity, the known allocator
capacity relation cannot exclude an omitted external-buffer or non-allocator
begin install.

## Reproduction and mutation gate

Run the analyzer inside the frozen, already-open IDA database:

```text
D:\projects\isaac_hook_reserach\out\cs-t0\analyze-manager-this-manager-tuple-writers-v11.py
```

The first run creates only the v11 JSON. Subsequent runs require exact
byte-for-byte equality. The current rerun result is:

```text
PASS verified consumers=303 primary_writers=6 unclassified_targets=36
```

Twelve semantic mutations are rejected, including attempted promotion,
writer/consumer count drift, removal of writer/support/unclassified rows,
the nested-vector misclassification, binding drift, and permission drift.

Pre-handoff artifact hashes:

- JSON: `D7BBF8A0EF3B7C46AFB7D04A831E400329BE8F3711D2B98D16E732E80C733716`
- analyzer: `287CACD9143432D61E8FB6EEE37453EDDF0702611D84AC618A0DFF737333C873`

Independent acceptance testing must be performed by a separate test agent;
this reverse-analysis task does not self-accept the promotion gate.
