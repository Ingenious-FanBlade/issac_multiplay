"""tx-01-86 saved-evidence analysis, called only by its unified acceptance."""
from __future__ import annotations

from collections import Counter
from .artifacts import append, load, rows, save

REASONS = ("NONE","PHASE_INCOMPLETE","ODD_PLAYER_ONLY","LIFECYCLE","READ_FAILED","BUDGET_EXCEEDED",
    "RELATION_BUDGET_EXCEEDED","BUFFER_BACKPRESSURE","UNKNOWN_CONTINUITY","WRONG_THREAD","NONFINITE",
    "SEQUENCE_EXHAUSTED","ROOT_CHANGED","EVENT_OVERFLOW","STOPPED","UNKNOWN_CLASS",
    "NOT_READY","SOURCE_CHANGED","UNPAIRED_SCOPE")


def identity(obj):
    return obj["local_id"], obj["use_generation"]


def latest_reference(path):
    """Inspect a bounded tail of controller-owned JSONL; never native pointers."""
    with path.open("rb") as stream:
        stream.seek(0,2)
        end = stream.tell()
        stream.seek(max(0,end-4*1024*1024))
        if stream.tell(): stream.readline()
        lines = stream.read().splitlines()
    from .artifacts import decode
    for raw in reversed(lines):
        try: record = decode(raw)
        except ValueError: continue  # Writer may be in its final, incomplete line.
        if record.get("record") != "capture": continue
        for obj in record["objects"]:
            if obj["birth"]["seed"] != 9730088: continue
            for field in obj["fields"]:
                if field["field_id"] == "0019" and field["status"] == "VALUE" and field["reference"]["status"] == "KNOWN_CURRENT":
                    return {"outer":record["outer_token"],"tick":record["authority_tick"],"key":identity(obj),
                        "field":field,"source":"SAVED_CONTROLLER_OWNED_COPY"}
        # Only the newest saved ordinary sample can establish the live premise.
        if record["authority_kind"] == 1: return None
    return None


def primary_proof(capture, output, evaluation, fixture, pause):
    output.mkdir()
    windows, copies, creation_points, ref_faults, lifecycles = {},set(),[],[],[]
    stack = []
    for offset,_,row in rows(capture / "e2e/trace.jsonl"):
        kind,tag = row["kind"],row["tag"]
        if kind == 3: stack.append({"hook":tag,"outer":row["outer"],"entry_offset":offset,"seed":None})
        elif kind == 5:
            if stack:
                frame=stack.pop()
                if frame["hook"] == 18 and frame["seed"] == 9730086:
                    creation_points.append({**frame,"address":f"{row['words'][7]:08x}","return_offset":offset})
        elif kind == 18 and tag == 18 and row["subject"] == "0" and stack and stack[-1]["hook"] == 18:
            stack[-1].update(seed=row["words"][6],arguments=row["words"],argument_offset=offset)
        elif kind == 7: copies.add(row["outer"])
        elif kind == 19:
            key = row["outer"],row["attempt"]
            if key in windows: raise ValueError("Duplicate same-token Window witness")
            windows[key] = {"initial_reason":tag,"words":row["words"],"tick":row["tick"],"offset":offset}
        elif kind == 20: ref_faults.append({"offset":offset,"event":row})
        elif kind == 12: lifecycles.append({"offset":offset,"event":row})
    events = [event for _,_,event in rows(capture / "world/lifecycle-events.jsonl")]
    births = [event for event in events if event["event_kind"] == 1 and any(
        event["diagnostics"]["address"] == p["address"] and event["outer_token"] == p["outer"] for p in creation_points)]
    short_key = identity(births[0]) if len(births) == 1 else None
    removed = [e for e in events if short_key and identity(e) == short_key and e["hook"] == "Remove" and not e["entering"] and e["event_kind"] == 0]
    cleared = [e for e in events if short_key and identity(e) == short_key and e["hook"] == "ClearReferences" and not e["entering"] and e["event_kind"] == 0]
    mismatch, counts, blocked, room_players = [],Counter(),Counter(),{}
    reference_before = reference_ambiguous = reference_after = short_absent = None
    teardown = [e for e in events if e["event_kind"] == 2 and e["diagnostics"]["result"] == "00000004"]
    retired_run = int(teardown[0]["run_epoch"]) if teardown else None
    retired_captures = []
    for offset,_,record in rows(capture / "world/captures.jsonl"):
        pair_key = record["outer_token"],record["attempt_seq"]
        witness = windows.get(pair_key)
        valid, why = witness is not None, ""
        if witness:
            initial, words = witness["initial_reason"],witness["words"]
            counts[REASONS[initial]] += 1
            flags,depth,retired,dirty = words[2],words[8],words[9],words[10]
            for mask,name in ((1,"pause"),(2,"loading"),(4,"lifecycle"),(8,"retired")):
                if flags & mask: blocked[name] += 1
            if record["record"] == "gap":
                valid &= record["reason"] == REASONS[initial] and record["outer_token"] not in copies
                if initial == 2: valid &= record["authority_kind"] == 2 and record["authority_tick"] is not None
                elif initial == 3: valid &= bool(depth or retired or dirty)
                elif initial == 1: valid &= words[11:14] != [1,1,1]
                else: valid = False
            else:
                valid &= initial == 0 and record["authority_kind"] == 1 and not (flags & 15 or depth or retired or dirty)
            if flags & 15 and record["outer_token"] in copies: valid = False
            why = "same token/attempt, actual initial branch, phase counts and companion-owned state"
        if not valid: mismatch.append({"offset":offset,"outer":record["outer_token"],"attempt":record["attempt_seq"],"witness":witness})
        append(output / "window-pairs.jsonl",{"offset":offset,"outer":record["outer_token"],"attempt":record["attempt_seq"],
            "record":record["record"],"reason":record["reason"],"witness":witness,"matched":bool(valid),"basis":why})
        if record["record"] != "capture": continue
        if retired_run is not None and int(record["run_epoch"]) >= retired_run: retired_captures.append(offset)
        players = {str(obj["ordinal"]):identity(obj) for obj in record["objects"] if obj["class"] == "Player" and obj["ordinal"] in (0,1)}
        if len(players) == 2:
            room_players.setdefault(record["room_epoch"],{"first":players,"first_offset":offset})
            room_players[record["room_epoch"]].update(last=players,last_offset=offset)
        if short_key and removed and cleared and int(record["outer_token"]) > int(cleared[-1]["outer_token"]):
            sources_complete = all(s["status"] == "NONE" for s in record["sources"] if s["source"] < 13)
            if sources_complete and all(identity(obj) != short_key for obj in record["objects"]) and short_absent is None:
                short_absent = {"offset":offset,"outer":record["outer_token"],"basis":"complete source coverage after actual Remove/ClearReferences; no physical destructor inferred"}
        for obj in record["objects"]:
            if obj["birth"]["seed"] != 9730088: continue
            field = next((f for f in obj["fields"] if f["field_id"] == "0019"),None)
            if not field or field["status"] != "VALUE": continue
            point = {"offset":offset,"outer":record["outer_token"],"key":identity(obj),"field":field}
            if field["reference"]["status"] == "KNOWN_CURRENT":
                if reference_before is None: reference_before = point
                elif reference_ambiguous and reference_after is None: reference_after = point
            elif field["reference"]["status"] == "AMBIGUOUS_REUSE" and reference_before:
                reference_ambiguous = reference_ambiguous or point
    lifecycle_stack, lifecycle_errors = [],[]
    for point in lifecycles:
        row=point["event"]
        if row["words"][0]: lifecycle_stack.append(row["tag"])
        elif not lifecycle_stack or lifecycle_stack.pop() != row["tag"]: lifecycle_errors.append(point)
    if lifecycle_stack: lifecycle_errors.append({"unclosed":lifecycle_stack})
    room_values=list(room_players.values())
    rooms_preserve = len(room_values) >= 2 and all(a["last"] == b["first"] for a,b in zip(room_values,room_values[1:]))
    pause_ok = pause and pause.get("tick_and_capture_stopped") is True
    loading_observed = any(p["event"]["tag"] == 2 for p in lifecycles)
    window_ok = (evaluation["trace_integrity"] and not evaluation["errors"] and not mismatch and not lifecycle_errors
        and counts["ODD_PLAYER_ONLY"] and counts["PHASE_INCOMPLETE"] and counts["LIFECYCLE"]
        and blocked["pause"] and pause_ok and loading_observed)
    reference_ok = bool(reference_before and reference_ambiguous and reference_after and len(ref_faults) == 1)
    if reference_ok:
        raw = [p["field"]["diagnostics"]["native_address"] for p in (reference_before,reference_ambiguous,reference_after)]
        words = ref_faults[0]["event"]["words"]
        reference_ok = (len(set(raw)) == 1 and raw[0] != "00000000" and words[0] == 0x19 and words[2:4] == [1,4]
            and reference_ambiguous["field"]["reference"].get("target") is None
            and reference_before["field"]["reference"]["target"] == reference_after["field"]["reference"]["target"])
    fixture_errors = [row for row in fixture if row["kind"] == "fixture_error"]
    short_ok = bool(len(births) == len(removed) == 1 and cleared and short_absent
        and int(births[0]["sequence"]) < int(removed[0]["sequence"]) < int(cleared[-1]["sequence"]))
    teardown_ok = bool(teardown and any(not e["entering"] for e in teardown) and not retired_captures)
    value = {"window_ok":bool(window_ok),"window_pairs_ok":bool(evaluation["trace_integrity"] and not evaluation["errors"] and not mismatch and not lifecycle_errors),
        "window_counts":dict(counts),"blocked_counts":dict(blocked),"short_spawn_bindings":creation_points,
        "window_mismatches":mismatch,"lifecycle_errors":lifecycle_errors,"loading_observed":loading_observed,
        "pause":pause,"room_players":room_players,"rooms_preserve":rooms_preserve,"teardown_ok":teardown_ok,
        "short_ok":short_ok,"short_births":births,"short_remove":removed,"short_clear":cleared,"short_absent":short_absent,
        "reference_ok":reference_ok,"reference_before":reference_before,"reference_ambiguous":reference_ambiguous,
        "reference_after":reference_after,"reference_faults":ref_faults,"fixture_errors":fixture_errors,
        "retired_captures":retired_captures,"physical_destroy_claimed_for_short":False}
    save(output / "proof.json",value)
    return value
