# T0-04 Offline Input Consumer / Apply Research

Date: 2026-08-22  
Fixed target SHA-256: `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30`

Current supersession (2026-08-27): the schema-3/4 passive matrix, Schema-4 active runtime, 18+2 formal matrix and final gate subsequently completed. T0-04 is now `PASS` with `activeInputAllowed=true`. Relative words such as “next”, “remaining” and “forbidden” below describe their historical checkpoint; current reusable conclusions are consolidated in [T0-04 逆向交接](../../../docs/refactor/cs-t0/t0-04/isaac-cs-t0-04-reverse-handoff.md).

## Safety and result

The x32dbg session remained running with zero breakpoints and zero patches. The investigation used module, memory, disassembly and xref reads only; it did not pause, run-to, step, modify registers or memory, or install a breakpoint.

At the initial static checkpoint, the primitive-dispatch ABI, per-device binding layout, one live Keyboard binding snapshot, a Player-owned natural-consumer envelope and six gameplay consumer edges were confirmed, but live schema-3 correlation was still missing. The later sections preserve how that gap was closed; this initial checkpoint must not be read as the current T0-04 state.

## Primitive dispatch ABI

`InputManager` is the fixed object at module RVA `0x857B18`; its vtable is RVA `0x782950`.

| vtable slot | target RVA | operation | ABI |
| ---: | ---: | --- | --- |
| `+0x40` | `0x620710` | primitive pressed | `bool __thiscall(manager*, u32 primitive, i32 controllerId, i32* selectedDeviceKeyOut)`, AL, `ret 0x0C` |
| `+0x44` | `0x6207B0` | primitive triggered | same bool ABI |
| `+0x48` | `0x620850` | primitive scalar | same arguments, float in x87 ST0, `ret 0x0C` |

All three dispatch through manager vslot `+0x74`, target RVA `0x620FB0`, which returns with `ret 0x14`. With `controllerId == -1` it enumerates the active 8-byte shared-pointer span at manager `+0x08..+0x0C`; otherwise it selects by device key through manager `+0x28`, RVA `0x620390`. A match may write `InputDeviceBase+0x0C` to `selectedDeviceKeyOut`.

The callback targets are RVA `0x620730` / `0x6207D0` / `0x620880`. Each receives a 16-byte caller-clean payload containing an 8-byte device shared pointer, a pointer to the primitive code, and the optional scalar output. They invoke concrete-device vslots `+0x24/+0x28/+0x2C`; the scalar callback also stores the float result and uses nonzero as the device-match result.

Evidence label: `STATIC_CONFIRMED`.

## Per-device logical binding table

The logical-action mapping belongs to each concrete device instance rather than to a global primitive switch:

- `device+0x10`: binding begin;
- `device+0x14`: binding end;
- `0x10` bytes per logical action, holding up to four `u32` primitive codes;
- `0xFFFFFFFF`: unused binding slot;
- logical action count: `(end-begin)/0x10`.

RVA `0x61E8D0` (logical pressed) bounds-checks the action, selects `begin + action*0x10`, skips `0xFFFFFFFF`, and queries each remaining primitive through device vslot `+0x24`. Triggered and scalar use the same binding model.

Evidence label: `STATIC_CONFIRMED`.

## Attach-only Keyboard snapshot

The observed process had module base `0x00870000`, one active Keyboard device at `0x0D7DCA8C`, vptr `module+0x7A1678`, device key `0`, and a 32-action binding table at `[0x0AE42644, 0x0AE42844)`.

| action | oracle name | observed first primitive |
| ---: | --- | ---: |
| 0 | Left | `0x41` |
| 1 | Right | `0x44` |
| 2 | Up | `0x57` |
| 3 | Down | `0x53` |
| 4 | ShootLeft | `0x107` |
| 5 | ShootRight | `0x106` |
| 6 | ShootUp | `0x109` |
| 7 | ShootDown | `0x108` |
| 8 | Bomb | `0x45` |
| 9 | Item | `0x20` |
| 10 | PillCard | `0x51` |
| 11 | Drop | `0x155` |

Evidence label: `RUNTIME_OBSERVED_SINGLE_SNAPSHOT`. This is not a default-keyboard, rebinding, Gamepad, local-coop, or gate contract.

Primitive code alone cannot identify a gameplay action. In the same snapshot, actions `4..7` alias `20..23`; action `8` aliases `25`; action `9` aliases `14`; action `10` aliases `24`; and action `11` aliases `29`. Treating a primitive code as an action `0..11` would admit UI/session actions and is a `REJECTED_HYPOTHESIS`.

## Caller and apply assessment

Immediate-reference enumeration found 19 pressed callbacks, 45 triggered callbacks, and two scalar callbacks. Representative owners include RVA `0x286AD0`, which implements pressed/triggered repeat for a caller-supplied primitive, plus thin primitive helpers at RVA `0x46FC70` and `0x46FCA0`. Other callers use UI/keyboard primitive constants through the same API.

No caller is statically unique to logical actions `0..11`. RVA `0x55B9CC/0x55BA56` proves device-to-player assignment only; RVA `0x5551D8` remains an unknown field copy; RVA `0x510980` remains an OriginalOnline oracle. The fixed build may use distributed on-demand consumers rather than one offline equivalent of the online frame apply.

The primitive-dispatch survey alone did not identify an apply boundary. The later Player consumer result below supersedes this as a static candidate, not as runtime proof.

## Historical next passive probe

The minimum safe probe must remain observation-only and original-once:

1. Observe logical query vslots `+0x30/+0x34/+0x38`, recording only explicit actions `0..11`, device key, result, return RVA, loop iteration and Authority phase. Historical zero-hit traces make these diagnostic, not sufficient.
2. Observe manager RVA `0x620710/0x6207B0/0x620850`, recording caller RVA, primitive, controller id, selected device key and original result. Emit an action only if the same coherent device binding table maps that primitive uniquely into `0..11` and nowhere into `12+`; otherwise redact it as ambiguous.
3. Optionally observe setter vslot `+0x4C`, RVA `0x61ED50`, only for explicit actions `0..11`. It remains a logical override/injected-layer candidate, not an apply boundary.
4. Correlate physical poll → query → unique Player owner → standard Authority transaction → world witness in a playable offline Run. UI callers and any action `12+` remain outside the artifact.

At this checkpoint, the passive experiment had to run without x32dbg attached because the Native runtime rejects debugged targets; active suppression, synthetic apply and loopback remained forbidden until later evidence and separate authorization closed that gate.

## Current passive live diagnostic

`artifacts/cs-t0/20260821T171600Z-t0-04-rpm-current-p18040-passive-01` contains 149/149 coherent samples, zero unstable/read-error drops, and 8/8 diagnostic checks. The game was at the main menu: only Keyboard was active, the Player vector was empty, and ownership was `KEYBOARD:UNKNOWN_NO_PLAYER_MATCH`. Its class is `RPM_DIAGNOSTIC_ONLY`, `eligibleForActiveGate=false`; it is not raw/apply evidence.

## Frozen v1 raw sample hypothesis and natural offline/local consumer candidates

The fixed-image, disk-static pass (no debugger execution and no breakpoints) established the following timing boundary and the original v1 hypothesis:

- Keyboard physical poll vtable `+0x78`: slot RVA `0x007A16F0` -> target RVA `0x0066C650`.
- Gamepad physical poll vtable `+0x78`: slot RVA `0x00782CF8` -> target RVA `0x00622940`.
- Common device update RVA `0x0061F280` copies `device+0xC0` to `+0xC4`, clears `+0xC0`, then calls concrete `+0x78` at `0x0061F307`. v1 hypothesized that the concrete poll or the remainder of this routine reconstructed an action-indexed physical projection before the existing V5 common-device-update wrapper returned.

Canary-03 disproved that v1 hypothesis. Every action `0..11` remained zero across 9,160 complete samples even while action `0..7` produced logical true results and movement/shooting witnesses changed. Static follow-up confirms that Keyboard RVA `0x0066C650` and Gamepad RVA `0x00622940` do not write `+0xC0/+0xC4`; logical pressed RVA `0x0061E8D0` scans the binding and calls the primitive query directly without writing `+0xC0`, while logical setter RVA `0x0061ED50` explicitly writes `+0xC0[action]`. These fields are logical override/cache state, not a physical raw projection. The schema-3 v1 runtime therefore must not relabel its `RAW_VALUE` records as raw input evidence.

The frozen v1 catalog and capture plan remain immutable historical inputs. Their raw-source hypothesis is now `REJECTED_BY_EVIDENCE`; a corrected concrete-state-to-action projection requires a create-new version and hash. It must be reconstructed read-only from the Keyboard/Gamepad concrete primitive caches plus a stable binding snapshot, without extra game queries and without persisting primitive identities, action `12+`, pointers, or platform handles.

Player vtable RVA `0x0076BDD0`, slot `+0x0C` at RVA `0x0076BDDC`, targets `Player::Update` RVA `0x00382AF0`. This slot is only an ownership/context envelope. The strict natural consumer edges inside it are:

| callsite RVA | target RVA | role | actions |
| ---: | ---: | --- | --- |
| `0x00385BC2` | `0x00379B40` | movement | `0..3` |
| `0x00385BC9` | `0x00380F40` | bomb | `8` |
| `0x00385BD2` | `0x00391A40` | item slot | `9` |
| `0x00385BD9` | `0x003A3D30` | pill/card | `10` |
| `0x00385BE0` | `0x0037B020` | shooting intent | `4..7` |
| `0x00385BE7` | `0x003A42B0` | drop | `11` |

Offline/local proof is deliberately stricter than seeing a call edge: the same Player transaction must contain a nested concrete-device logical query for the Player-owned device key. Movement, shooting, bomb, and drop additionally require a changed allowlisted before/after witness. Actions 9 and 10 have no frozen universal world field, so their claim stops at natural consumer plus nested concrete logical proof. Shooting intent is not projectile-spawn proof.

The frozen documents are `raw-apply-catalog-v1.json` (SHA-256 `C01E325A592876D0D2C0EC1354EFF1ED72C3B7E4D25E3B4FFAA0212F447A2D0C`) and `raw-apply-capture-plan-v1.json` (SHA-256 `DEE4C87298045174F76689D4D5DB4D97AE6F4977303D1E89E84AC598C6847805`). Historical v1 state: `FROZEN_REJECTED_HYPOTHESIS`; live closure state: `BLOCKED_BY_EVIDENCE`.

The create-new historical v2 documents are `raw-apply-catalog-v2.json` (SHA-256 `F64FA3FFF87F1B0D1AEA8C4FAE8C849E229459222CAB7CB1B9B1D232E2489266`) and `raw-apply-capture-plan-v2.json` (SHA-256 `6A7BC25ECDDCA0D556DE64BECAE8F2799B4AFC1E3D3618DF00E494CC6C3633CE`). The v2 sample moved emission to physical-poll original return on the main thread, in `InputQuery`, with return RVA `0x0061F30C`, and reconstructed concrete Keyboard/Gamepad state. It incorrectly reused the full-scope primitive-to-action classifier for an already action-indexed projection, so outside-gameplay aliases redacted valid gameplay slots.

The active create-new documents are now `raw-apply-catalog-v4.json` (SHA-256 `E22323047082E6289891393DE0B467CA5038E15EBD45310F39B5051FD0EB13D6`) and `raw-apply-capture-plan-v4.json` (SHA-256 `203F64825EA24EE4EFA21B731FD2CB8E15AB11DA87145F7654C545B676A45562`). v4 preserves every v3 hook, IPC, timing, concrete-state, and projection rule. It changes only analyzer cardinality: a consumer transaction with several candidates all belonging to the site's allowed family is ignored and provides no action or witness credit; a cross-family or wrong-site candidate is a whole-run failure; every required action must have a separate exactly-one-candidate transaction. A witness can therefore never be reused across a diagonal. v1/v2/v3 identities remain frozen and are rejected by the active v4 Controller and Native.

v4 passes 171/171 Rust tests, formatting, full-target clippy with warnings denied, Win32 Debug/Release-LTCG, and both Native harness configurations. Release Controller/Native/Helper SHA-256 values are `442064DE4D4CEAE30CF57BAE18D37A797B8061E544A775812167DABF9CBF753F`, `03751409B5BAD0D21717317030717E36B9EDEB0905E277C616CC07D5248FA954`, and `717E3498C7B75FBB427BC62AAA70CF673A8BDE68ED020D6307D5E29BFD551655`. No game or debugger was started for this implementation pass.

The historical v3 documents are `raw-apply-catalog-v3.json` (SHA-256 `258549E9B107316FDE02C44580741862476AAB7CCABE57AA1958F06729304F51`) and `raw-apply-capture-plan-v3.json` (SHA-256 `8908DABFF44E299185438CBC6092F7A3A6B5E308FB95C75BDFA795F0E0042C93`). v3 preserved v2 timing and concrete-state rules but separated the projection classifier from the manager classifier. Projection begins at an explicit action `0..11` slot, permits aliases owned only by actions `12..31`, and never reads or persists the outside action or primitive. A primitive shared by two gameplay actions still redacts every affected gameplay action. The manager primitive classifier remains strict across the complete `0..31` table. Keyboard/Gamepad out-of-bounds, `0xCDCC/0xCDCD`, negative/non-finite axes, invalid deadzone, and unstable snapshots remain fail closed.

## Schema-3 observation-only implementation status

`ICSI04V1` schema 3 adds a 128B event and 65,536-slot raw/apply ring without changing schema 2 or the V5 companion IPC. It combines the existing 32 V5 slots, 11 legacy input slots, the Player owner slot, and six natural consumer calls in one 50-slot transaction. All wrappers are original-once and return-preserving; no parameter, device state, queue, world, `ModeLatch`, suppression, synthetic input, or active apply path is modified.

The new artifact class is `PASSIVE_RAW_APPLY_RUNTIME`, permanently `eligibleForActiveGate=false`. Its analyzer requires post-poll sample linkage, Player ownership, nested concrete logical proof, consumer transaction linkage, allowlisted witnesses, complete producer shutdown, zero overwrite, and exact 50-slot restoration. Canonical/command/queue/resolve/applied-state fields remain empty, and `input-gate` rejects this evidence class even when all passive checks pass.

Automated verification initially completed with 164/164 Rust tests, `cargo fmt --check`, full-target clippy with warnings denied, Win32 Debug and Release/LTCG builds, and both Native harness configurations. No game process or debugger was used for that implementation pass.

The first separately confirmed `keyboard-held-release` canary produced a preserved `FAILED` artifact after 95,866 accepted raw/apply events. Native correctly emitted both approved movement witnesses: direction (`Player+0x1624`) and velocity (`Player+0x360/+0x364`); the Rust field-scope allowlist accidentally accepted only direction and rejected the first velocity `WorldWitnessBefore`. Shutdown still restored all 18 schema-3 plus 32 V5 slots byte-for-byte, overwrite and Native Failure counts were zero, and the game remained responsive with no T0 module resident. This is an analyzer contract defect, not offline/local closure evidence.

The minimal fix adds `PlayerVelocityF32x2` only at movement callsite `0x00385BC2`; wrong-site velocity, action `12+`, and persisted primitive payloads remain rejected. The same artifact recorded `observedRaw=0x1`, correctly showing that a Keyboard-only owner need not observe a Gamepad source: `requiredRaw=0x3` remains the installation contract, while runtime evidence requires owner bit `0x1` for Keyboard or `0x2` for Controller, permits extra known sources, and rejects unknown bits. The corrected suite is 167/167 PASS with fmt, full-target clippy, Release/LTCG, and both Native harness configurations. The historical artifact is not rewritten or upgraded. At that checkpoint, a new `keyboard-held-release` canary was still required; it was subsequently completed.

The separately confirmed second canary preserved another `FAILED` artifact after a real four-event ring overwrite. It confirmed that the scenario-specific source rule and both movement witness kinds now pass, but the command exited before user input was captured. The failure was controller I/O backpressure, not a Native threshold error: roughly 87.9 MiB of raw JSON was produced across 18.013 seconds while unbuffered serde writes kept the controller away from the next drain for long enough to fill 65,536 slots. The writer now uses a 4 MiB `BufWriter<File>` with explicit finish/fail flush and unchanged overwrite/cursor semantics. All 50 target slots were restored and the game had no T0 residue.

The separately confirmed canary-03 artifact `20260822T110518Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-canary-03` completed without overwrite: 703,040 raw/apply events, 201,036 companion V5 events, 9,160 complete loops, exact restoration of all 50 slots, and companion V5 18/18 PASS. It observed logical true results for action `0..7`, balanced natural consumer transactions, and changed movement/shooting witnesses, but all 109,920 `RAW_VALUE` records for action `0..11` were zero. It is consequently a safe rejected-hypothesis capture, not raw held/release closure and not T0-04 PASS.

The old analyzer also exposed two defects on this volume: repeated full-stream scans made closure analysis quadratic, and mere existence of a zero-valued `RAW_VALUE` could satisfy the old raw binding. The indexed analyzer now requires a finite non-zero, identity-aligned earlier raw sample; held/release additionally requires inactive-to-active-to-inactive transitions for every required action. Pairing and closure use pre-indexed identity/QPC/transaction groups rather than whole-stream rescans. The corrected strict replay finished canary-03 in about 1.95 seconds, passed all structural/restoration/companion checks, and failed only offline/local closure; analysis SHA-256 is `9F24183D2FACEDCB621688E6AB852A7E25F1F929776ACA751BF0D96140229C81`.

v2 passed 168/168 Rust tests, fmt, full-target clippy, Win32 Debug and Release/LTCG, and both Native harness configurations. Its Release Controller/Native/Helper SHA-256 values were `AB256929D1362271DD8E171A0973E8FA87C897F1075EC48A0FA8FE83CB0CE5E0`, `201A600CE74FE47A8A464A60E488E73A4680569F80AE897E72E3ABA5AA02C949`, and `717E3498C7B75FBB427BC62AAA70CF673A8BDE68ED020D6307D5E29BFD551655`.

The separately confirmed first v2 live run, `20260822T121256Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-v2-canary-04`, safely completed with 627,338 contiguous raw/apply events, 225,849 companion events, 10,555 complete loops, zero overwrite/failure/wrong-thread/error, and exact restoration of all 50 slots. The companion V5 analyzer passed 18/18. Actions `0..3` each recorded 10,555 projections, two rising edges, two falling edges, non-zero samples, and two complete raw/logical/consumer/witness proof cycles. Actions `4..7` recorded no `RAW_VALUE`, although their concrete logical pressed/scalar results were true and the shooting consumer/witnesses changed. The strict analyzer therefore failed only offline/local closure. This is a projection-classification gap isolated to the shooting bindings, not missing user input, poll timing, consumer activity, or restore safety. The artifact is retained and no retry or next scenario was started.

The canary-04 binding correlation showed shoot actions `4..7` using primitives `0x107/0x106/0x109/0x108`, respectively aliased by UI actions `20..23`; edge actions `8..11` have the same outside-alias shape. Because v2 classified each primitive across all 32 actions, it cleared the gameplay valid bit even though the projection started from a known gameplay slot and persisted neither primitive nor UI action. This is `FULL_SCOPE_ALIAS_CLASSIFIER_MISAPPLIED_TO_ACTION_INDEXED_PROJECTION`. v3's split classifier fixes only that layer; gameplay-to-gameplay collisions remain redacted, and raw closure still requires a same-sample nonzero gameplay projection plus nested concrete logical, natural Player consumer, and allowlisted witness.

v3 passed 168/168 Rust tests, fmt, full-target clippy, Win32 Debug and Release/LTCG, and both Native harness configurations. Release Controller/Native/Helper SHA-256 values are `63EEB616E43181C15ED7CAF272962652DD45A45050A2846EA315C2105136FD6D`, `79FC27358B7A11F0FE86BAEC211D43CF4DC9D0D1141AB7A8DAA7352B0BD6A725`, and `717E3498C7B75FBB427BC62AAA70CF673A8BDE68ED020D6307D5E29BFD551655`. At the v3 checkpoint, each live scenario still required separate confirmation and active input remained forbidden; later v4/Schema-4 evidence superseded that gate state.

The separately confirmed v3 keyboard-held-release run, `20260822T124555Z-t0-04-input-raw-passive-keyboard-held-release-t0-04-raw-apply-keyboard-held-release-v3-canary-05`, passed all seven strict raw checks and all 18 companion V5 checks. It recorded 635,300 contiguous raw/apply events and 209,973 companion events across 9,893 complete loops, with zero overwrite, wrong-thread, failure, error, persisted primitive, action `12+`, or active-pipeline field, and exact restoration of all 50 slots. Every action `0..7` recorded 9,893 projections, two rising and two falling edges, non-zero held samples, matching concrete logical results, natural consumer/witness activity, and two complete held-release proof cycles. In particular, actions `4..7`, absent under v2, now closed without exposing the aliased UI actions or primitives. This satisfies the v3 `keyboard-held-release` passive scenario, but the artifact remains permanently `eligibleForActiveGate=false`; the remaining keyboard-edge and two controller scenarios still require separate confirmations.

The separately confirmed v3 keyboard-edge run, `20260822T133033Z-t0-04-input-raw-passive-keyboard-edge-t0-04-raw-apply-keyboard-edge-v3-canary-06`, also passed all seven strict raw checks. It recorded 1,744,176 contiguous raw/apply events and 494,136 companion events across 23,098 complete loops, with zero overwrite, wrong-thread, failure, error, action `12+`, primitive, or active-pipeline payload and exact restoration of all 50 slots. Actions `8/9/10/11` completed `11/7/6/7` rising-to-falling cycles respectively, with both separated and rapid inter-rise gaps, non-zero same-sample projections, concrete logical results, and the required bomb/item/pill-card/drop natural consumers; bomb and drop additionally had their allowlisted changed witnesses. The raw analyzer's companion binding passed because the V5 stream was complete, structurally valid, restored, and hash-bound. The companion V5 analyzer itself was 17/18: only its frozen `scenario-evidence` check rejected this rapid-tap scenario outside the seven V5 gate scenarios, matching the earlier expected scope behavior and not modifying the existing V5 gate. Action `0` had one incidental 32-sample cycle while actions `1..7` stayed inactive; it is recorded but lies outside the keyboard-edge required action set and does not affect the strict result. This advanced the historical passive matrix to 2/4; both controller scenarios were completed later.

The separately confirmed v3 controller-held-release canary-07, `20260822T160527Z-t0-04-input-raw-passive-controller-held-release-t0-04-raw-apply-controller-held-release-v3-canary-07`, is permanently retained as FAIL. Actions `0..7` each had non-zero raw values, complete rising/falling cycles, and singleton candidate transactions, but 229 additional transactions contained same-family diagonal candidates: movement `0+3` (45), shooting `5+6` (88), and shooting `5+7` (96). The v3 analyzer failed the whole run on the first multi-action transaction. v4 does not reinterpret this historical result; it freezes the safer rule that a same-family/site multi-candidate transaction is ignored with zero score, while cross-family, wrong-site, only-diagonal, witness reuse, or a missing per-action singleton remains FAIL. Raw manifest/events/analysis SHA-256 are `98993BB28FFFDFD73987B5B5F08616814ADFF39CF00EA1B976645AA68646F4D0`, `DB770338CB6138C7FF401A6C5F8B4A2EC31FC01107498CC8B7B3A36DD439529F`, and `9F24183D2FACEDCB621688E6AB852A7E25F1F929776ACA751BF0D96140229C81`; companion manifest/events/analysis are `F69EEEF540ACD522E8FA127544FA83F0B5070E0048C21ED9613629668664D163`, `8A63E3A7076086BF45CC3860C56C7A7895198AA24702CDD8E5DBAFF883D23716`, and `6EA2CFE61598E6A4795D7FECB8E554AECF18B3BBCAA8E3D18757282B292DC7D1`. At that checkpoint a new v4 controller run was required; canary-08 later supplied it.

The separately confirmed v4 controller-held-release canary-08, `20260822T165606Z-t0-04-input-raw-passive-controller-held-release-t0-04-raw-apply-controller-held-release-v4-canary-08`, passed all seven strict raw checks and all 18 companion V5 checks. It recorded 470,556 contiguous raw events and 258,053 companion events across 11,162 complete loops, with zero overwrite, wrong-thread, failure, error, or residual module and exact restoration of all 50 slots. Actions `0..7` each had two to four complete rising/falling cycles, 116–224 singleton candidates, and one to three strict proof transactions. v4 ignored 37 movement `0+3` and seven shooting `5+7` same-family diagonals with zero credit; cross-family and wrong-site counts were zero. The difficult down actions 3 and 7 still retained 116/203 singleton candidates and 1/2 strict proofs. Raw manifest/events/analysis SHA-256 are `03E6BD9FF9346CE791F52A5219D70210C07258FA342624854B99D393E4E763FD`, `29B576F8904B6613B305E151A337791E4D1416AAD65FA57884E4431B4A6B9B95`, and `73CCECFF4C1C7435EECA91566F6654F1B3D8435D3A6958B0A4D5063EA6C20753`; companion manifest/events/analysis are `1AD3B1E6443573F79304638D6E88C1B5260E144B436568583C1BF691A1F2FE4D`, `1EF4DE57BBF2F19C03897AFD9A3942F729FE44608E6EC0BB62B22E557DF48AD2`, and `275683A8A2317836F7E2900BDE8C3A5C466629126A7167588F08B74DBD0882CF`. This advanced the historical passive matrix to 3/4; controller-edge was completed in canary-09. Every passive artifact remains permanently ineligible for the active gate by evidence class.

The separately confirmed v4 controller-edge canary-09, `20260822T171644Z-t0-04-input-raw-passive-controller-edge-t0-04-raw-apply-controller-edge-v4-canary-09`, passed all seven strict raw checks. It recorded 534,858 contiguous raw events and 275,545 companion events across 11,806 complete loops, with zero overwrite, wrong-thread, failure, error, or residual module and exact restoration of all 50 slots. Actions `8/9/10/11` completed `7/6/7/7` rising-to-falling cycles; every action contained both clearly separated gaps and a rapid 9–19-loop cluster. Singleton/proof counts were `7/7`, `77/77`, `7/7`, and `162/162`; bomb/drop witnesses changed, while actions 9 and 10 closed under their frozen consumer-only rule without a fabricated world witness. Multi-action and cross-family counts were zero. The companion V5 analyzer was the expected 17/18: only frozen `scenario-evidence` rejects this non-seven-scenario scope, while all structural, restoration, and hash-binding checks passed and the existing V5 gate remains unchanged. Raw manifest/events/analysis SHA-256 are `132C6E2E9AC4FE5E8C92B9F13D3E8C25A73B98492D54CA6E81651F568B64A9D7`, `CC61049B0CD90F39463F26EA2FF65263753419158E6F99B39F4D85AF5A397446`, and `73CCECFF4C1C7435EECA91566F6654F1B3D8435D3A6958B0A4D5063EA6C20753`; companion manifest/events/analysis are `490B3E7027C5D95BC78D39ED70F7383D62866292DE0E93AFD89FF99F92712BBB`, `FED45E85D5EAE0BFE0416F94A3FDC36F1DB4EC2C37F2D1182A01E0D34A130010`, and `C0C29A74543100AC4E89976732F542375CBDB7DD5E792CD4496BE12A8AA4864D`. This closed the four observation-only raw/apply scenarios at 4/4 PASS. Every passive artifact remains permanently ineligible for the active gate by evidence class; later separately authorized Schema-4 active evidence and final gate closed the active path.
