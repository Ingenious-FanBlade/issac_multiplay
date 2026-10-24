# T0-03R / T0-04 Passive Runtime Validation

Date: 2026-08-20  
Fixed target SHA-256: `C4118519BA5C051826FE16507715DE602B51BE4F2CD71534EA13037A0F9BAE30`

## Scope and safety

This run used the nil-return `MC_INPUT_ACTION` witness and read-only process-memory inspection. No native input hook, suppression, synthetic apply, loopback arm, remote call, breakpoint, pause, single-step, patch, register write, or process-memory write is valid evidence for this matrix.

The witness reports demand-driven action-query occurrence only. It does not report the returned bool/float, raw axis values, held/released state, canonical bytes, Authority tick, target tick, or world apply. User visual observations are recorded only as correlations.

## Debugger incident correction

Two earlier crashes are excluded from game/input evidence. Both faulted at runtime `0x008E1231` / RVA `0x531231` with single-step status. The debugger log showed persistent hardware execute breakpoints at that address and three additional locations; condition installation had failed, leaving the breakpoints live. The current attach-only session used no breakpoints and remained stable. The crashes are classified `DEBUGGER_INDUCED / INVALID_RUN`, not witness or controller failures.

## Passive witness matrix

| Scenario | Ownership / windows | Observation | Claim allowed |
|---|---|---|---|
| Controller idle | `c=1`, initial windows `1..3` | witness contract active, no drops | witness liveness and controller attribution |
| Controller move right | `c=1`, `234..250` | player visibly moved right | move-query envelope plus visual correlation |
| Controller shoot right | `c=1`, `500..511` | player visibly shot right | aim-query envelope plus visual correlation |
| Controller move + shoot | `c=1`, `673..686` | both actions worked | move/aim query families present in the same observed interval; no value or simultaneity proof |
| Same-run keyboard attempt | controller-owned run remained `c=1` | keyboard `D` did not move player | ownership remained fixed; not a canonical-equivalence failure |
| Pause / resume | `c=1`, emitted segments `947..956`, `963..966` | pause and resume were normal | query-envelope stop/resume correlation only |
| Focus loss | not run | game is known to auto-pause on focus loss | `SKIPPED_BY_KNOWN_PLATFORM_BEHAVIOR`, not PASS |
| Room transition | `c=1`, `1513..1522` | room transition marker and no visible old input leak | lifecycle/query-envelope correlation only |
| Run teardown / main menu | final witness at `1648`; later menu initialization | preceding segment included Game Over and another run initialization | teardown-to-menu observation; not a clean menu-return PASS |
| Keyboard ownership baseline | stable `c=0` from `1650` | `SetControllerId=0`, player assignment key `0` | independent keyboard ownership established |
| Keyboard move right | `c=0`, `1783..1794` | player visibly moved right | keyboard move-query envelope plus visual correlation |
| Keyboard shoot right | `c=0`, `1861..1877` | player visibly shot right | keyboard aim-query envelope plus visual correlation |
| Keyboard move + shoot | `c=0`, `1920..1931` | both actions worked | query-envelope plus visual correlation only |
| Keyboard rapid shoot tap | `c=0`, `2024..2036` | two visually separated shot pulses | press/release/press visual correlation; not T0-04 edge/target/apply proof |

All accepted witness increments had `droppedUnique=0`, declared totals equal to the sum of observation counts, action IDs restricted to `0..11`, and no Lua reload/error in the relevant ranges. Empty witness windows are intentionally not emitted, so gaps between emitted window ordinals cannot be interpreted as transport loss or complete no-query evidence.

## Read-only InputDevice evidence

RTTI and vtable inspection confirms concrete Keyboard and Gamepad types. Both use:

- vtable `+0x30`: logical bool/pressed query, `bool __thiscall(InputDeviceBase*, uint32 action)`, AL return, `ret 4`;
- vtable `+0x34`: logical triggered query;
- vtable `+0x38`: logical scalar query, `float __thiscall(InputDeviceBase*, uint32 action)`, x87 ST0 return, `ret 4`.

The stable owner chain is:

`module+0x857B18 input manager -> active shared_ptr vector -> InputDeviceBase+0x0C deviceKey -> module+0x871678 game root -> Player* vector -> player+0x1618 assignmentKey`.

For PID `20224`, the current keyboard object has `deviceKey=0` and uniquely joins the only Player whose assignment key is `0`. A prior XInput object was observed only transiently and later lay outside the active vector end; it is not accepted as a stable XInput owner.

## Artifacts

- `artifacts/cs-t0/20260820-t0-04-rpm-keyboard-idle-p20224-v3`: current schema-v2 capture; 287/287 coherent samples, zero unstable/read drops, and a keyboard unique-owner match in every sample. It double-collects vector headers and active spans, device structures and cached arrays, game root/player vector, player assignment keys, and Gamepad platform id. Offline RPM analysis passed all seven diagnostic checks. Its result supersedes the prototypes below.
- `artifacts/cs-t0/20260820-t0-04-witness-keyboard-c0-window1783-1794`: source-log SHA bound, window `1783..1794` and controller `0` filter recorded in the manifest; 217 query-only events; `eligibleForActiveGate=false`.
- `artifacts/cs-t0/20260820-t0-03r-loop-gate-blocked.json`: explicit non-overwritable `BLOCKED_BY_EVIDENCE` decision used by the witness artifact.
- `artifacts/cs-t0/20260820-t0-04-rpm-keyboard-idle-p20224`: superseded diagnostic prototype. Its `enabledFlags` field read a full word although the query checks a byte.
- `artifacts/cs-t0/20260820-t0-04-rpm-keyboard-idle-p20224-v2`: superseded schema-v1 prototype. It corrected `enabledByte` but did not yet double-collect active vector spans and player assignment keys. Both prototypes remain preserved for audit and must not be used as the current result.

## Historical gate decision and supersession

At this document's 2026-08-20 checkpoint, T0-03R was still `BLOCKED_BY_EVIDENCE` and T0-04 was not `PASS`. This witness/RPM matrix did not by itself prove a gapless loop envelope, stable XInput ownership, raw values, canonicalization, sequence/target ordering, suppression, exactly-once apply, or reject-without-world-change. That historical limitation remains part of the evidence record.

Subsequent V5 loop, schema-3/4 passive raw/apply, active runtime and formal replay work closed those gaps without rewriting the artifacts above. The current state is T0-03R V5 7/7 `PASS` and T0-04 schema-4 final `PASS`, with `t004Unlocked=true` and `activeInputAllowed=true`. Use [T0-04 逆向交接](../../../docs/refactor/cs-t0/t0-04/isaac-cs-t0-04-reverse-handoff.md) for current conclusions; this file remains the early passive witness/RPM source record.
