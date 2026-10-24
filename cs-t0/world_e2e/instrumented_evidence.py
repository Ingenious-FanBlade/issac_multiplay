"""Evidence evaluation inside the single acceptance invocation, never a preflight."""
from __future__ import annotations
import collections
import struct
from .artifacts import append, load, rows, save


def fields(obj):
    return {f["field_id"]: f for f in obj["fields"]}


def frame(record):
    source = next((s for s in record["sources"] if s["source"] == 13), None)
    return source["diagnostics"]["descriptor"][2] if source else None


def evaluate(capture, output):
    output.mkdir()
    probe = load(capture / "e2e" / "result.json")
    frequency = int(probe["qpc_frequency"])
    errors, counts, stages, phases = [], collections.Counter(), {}, {}
    stack, nested, copying, timings, ready, fx = [], False, None, [], [], {}
    call_frames, seed_init = [], {}
    rng, mt, reads = 0, 0, 0
    for expected, (_, _, row) in enumerate(rows(capture / "e2e" / "trace.jsonl"), 1):
        if int(row["sequence"]) != expected:
            errors.append({"sequence":row["sequence"], "error":"trace sequence"})
        kind, tag, words = row["kind"], row["tag"], row["words"]
        token = row["outer"]
        counts[f"{kind}:{tag}"] += 1
        if kind in (3, 4, 5) and copying is not None:
            errors.append({"sequence":row["sequence"], "error":"native entry during copy"})
        if kind == 3:
            nested |= tag == 18 and 18 in stack
            stack.append(tag)
            call_frames.append({"hook":tag,"self":f"{words[6]:08x}","outer":token,"sequence":row["sequence"]})
        elif kind == 5:
            if call_frames:call_frames.pop()
            if not stack or stack.pop() != tag or words[9] or words[10] != 1 or words[11] != words[12]:
                errors.append({"sequence":row["sequence"], "error":"return/stack/observer preservation"})
        elif kind == 4 and words[9]:
            errors.append({"sequence":row["sequence"], "error":"actual original entry ABI", "mask":words[9]})
        elif kind == 6:
            key = (tag, words[0])
            target = fx.setdefault(key, {})
            offset = words[1]
            if offset in target:
                errors.append({"sequence":row["sequence"], "error":"duplicate FX chunk"})
            target[offset] = struct.pack("<12I", *words[2:])[:min(48, 512-offset)]
        elif kind == 18 and row["subject"] == "0" and tag == 1 and call_frames:
            for seed in (9730003,9730004,9730005):
                if seed in words:seed_init.setdefault(str(seed),[]).append({**call_frames[-1],"arguments":words})
        elif kind == 1:
            if tag == 0 and words[0] == 1:
                stages[token] = {"order":[], "commits":[], "copies":0}
            elif tag == 0:
                current = stages.get(token)
                if current is None:
                    errors.append({"sequence":row["sequence"], "error":"outer end unpaired"})
                else:
                    current["reason"], current["end_sequence"] = words[1], row["sequence"]
            elif token in stages:
                stages[token]["order"].append([tag, words[0]])
        elif kind == 2 and token in stages:
            stages[token]["commits"].append({"tick":row["tick"], "kind":tag, "words":words})
        elif kind == 7:
            if copying is not None:
                errors.append({"sequence":row["sequence"], "error":"nested copy"})
            copying = row
        elif kind == 8:
            if copying is None or copying["outer"] != token:
                errors.append({"sequence":row["sequence"], "error":"copy end unpaired"})
            else:
                timings.append((int(row["qpc"])-int(copying["qpc"]))/frequency*1000)
                if token in stages: stages[token]["copies"] += 1
                reads += words[0] + (words[1]<<32)
            copying = None
        elif kind == 10:
            ready.append(row)
        elif kind == 15 and token in stages:
            current = stages.pop(token)
            current["top_sequence"] = row["sequence"]
            current["ordinary_valid"] = (current.get("reason") == 0 and current["copies"] == 1
                and current["order"] == [[1,1],[2,1],[2,0],[3,1],[3,0],[1,0]]
                and len(current["commits"]) == 1 and current["commits"][0]["kind"] == 1
                and current["commits"][0]["words"] == [1,1]
                and int(current["end_sequence"]) < int(row["sequence"]))
            phases[token] = current
            append(output / "phase-order.jsonl", {"outer":token, **current})
        elif kind == 16:
            rng += 1
            if tag or words[10] != 3 or words[2:6] != words[6:10]:
                errors.append({"sequence":row["sequence"], "error":"object RNG bytes changed/unreadable"})
        elif kind == 17 and tag == 2:
            mt += 1
            if words != [1,1,1,0,0]:
                errors.append({"sequence":row["sequence"], "error":"MT/index/rand flag changed or unavailable", "words":words})
    if copying is not None or stack:
        errors.append({"error":"unfinished copy/native call"})
    for hook in range(19):
        if counts[f"3:{hook}"] != counts[f"4:{hook}"] or counts[f"4:{hook}"] != counts[f"5:{hook}"]:
            errors.append({"error":"entry/original/return count", "hook":hook})
        if not counts[f"3:{hook}"]: continue
        raw = []
        for phase in range(3):
            chunks = fx.get((hook,phase), {})
            data = b"".join(chunks.get(i,b"") for i in range(0,512,48))
            if len(data) != 512:
                errors.append({"error":"missing full FX snapshot", "hook":hook, "phase":phase})
            raw.append(data)
        if all(len(r)==512 for r in raw):
            spans = [(0,32),(160,288), *[(32+i*16,42+i*16) for i in range(8)]]
            if any(raw[0][a:b] != raw[1][a:b] for a,b in spans):
                errors.append({"error":"actual input FX changed", "hook":hook})
    objects, reasons, source_failures, witnesses = {}, collections.Counter(), collections.Counter(), {}
    ordinary, phase_matches, retained, unknown, first_observed = 0, 0, None, None, False
    for offset, _, record in rows(capture / "world" / "captures.jsonl"):
        reasons[record["reason"]] += 1
        for source in record["sources"]:
            if source["status"] not in ("NONE","NOT_APPLICABLE"): source_failures[source["status"]] += 1
        if record["record"] == "capture" and record["authority_kind"] == 1:
            ordinary += 1
            proof = phases.get(record["outer_token"], {})
            if proof.get("ordinary_valid") and proof["commits"][0]["tick"] == record["authority_tick"]:
                phase_matches += 1
            else: errors.append({"error":"saved ordinary capture lacks phase proof", "offset":offset})
        for obj in record["objects"]:
            key = (obj["local_id"],obj["use_generation"])
            state = objects.setdefault(key, {"birth":None,"birth_event":None,"first":offset,"last":offset,
                "class":obj["class"],"address":obj["diagnostics"]["address"],"current":[],"sources":set()})
            state["last"] = offset
            if obj["birth_event"] is not None:
                if state["birth_event"] is not None and (state["birth_event"] != obj["birth_event"] or state["birth"] != obj["birth"]):
                    errors.append({"error":"frozen birth changed", "key":key, "offset":offset})
                state["birth"],state["birth_event"] = obj["birth"],obj["birth_event"]
            f = fields(obj)
            current = [obj["revision"], f.get("0002",{}).get("raw32"), f.get("0003",{}).get("raw32")]
            if current not in state["current"]: state["current"].append(current)
            sources = {m["source"] for m in obj["memberships"]}
            state["sources"].update(sources)
            if obj["class"] == "Player" and sources & {9,10} and 8 not in sources:
                retained = retained or {"offset":offset,"object":obj}
            if obj["class"] == "UnknownEntity": unknown = unknown or {"offset":offset,"object":obj}
            first_observed |= obj["ordinal"] == 0 and obj["origin"] == "FIRST_OBSERVED" and obj["birth_event"] is None
            for field in obj["fields"]:
                if field["status"] == "READ_FAILED": source_failures["FIELD_READ_FAILED"] += 1
            for seed in (9730003,9730004,9730005):
                if obj["birth"]["seed"] == seed:
                    witnesses.setdefault(str(seed), {"key":key,"first":offset})["last"] = offset
    events, by_address, births, publishes, membership = collections.Counter(), {}, collections.Counter(), collections.Counter(), 0
    short, short_keys, removes = None, [], []
    for offset, _, event in rows(capture / "world" / "lifecycle-events.jsonl"):
        key = (event["local_id"],event["use_generation"])
        events[f"{event['hook']}:{event['entering']}"] += 1
        if event["event_kind"] == 1: births[key] += 1
        if event["event_kind"] == 1 and any(w["self"]==event["diagnostics"]["address"] and w["outer"]==event["outer_token"] for w in seed_init.get("9730003",[])):
            short_keys.append({"key":key,"birth":event,"offset":offset})
        if event["hook"]=="Remove" and not event["entering"]:removes.append((key,offset,event))
        if event["hook"] == "Publish" and not event["entering"]: publishes[key] += 1
        if event["event_kind"] == 7: membership += 1
        if event["hook"] == "Factory" and not event["entering"] and event["local_id"]:
            by_address.setdefault(event["diagnostics"]["address"],set()).add(key)
        if event["hook"] == "Remove" and not event["entering"] and key == tuple(witnesses.get("9730003",{}).get("key",())):
            short = {"offset":offset,"event":event}
    if len(short_keys)==1:
        key=short_keys[0]["key"]
        matching=[{"offset":off,"event":event} for k,off,event in removes if k==key]
        if matching:short={"binding":short_keys[0],"remove":matching[0],"sampled":key in objects}
    ready_history = collections.defaultdict(list)
    for row in ready: ready_history[(row["subject"],row["use_generation"])].append(row["words"][:2])
    revoked = any([0,3] in values and [3,0] in values and values.index([3,0]) < len(values)-1
                  and [0,3] in values[values.index([3,0])+1:] for values in ready_history.values())
    result = {"trace_integrity":probe["closed"] and probe["error"]==0 and probe["overflow"]=="0" and probe["produced"]==probe["consumed"],
        "errors":errors,"counts":dict(counts),"hooks_covered":[i for i in range(19) if counts[f"3:{i}"]],
        "ordinary":ordinary,"phase_matches":phase_matches,"nested_spawn":nested,"copy_count":len(timings),
        "copy_ms":{"min":min(timings,default=None),"max":max(timings,default=None),"total":sum(timings)},
        "literal_reads":reads,"object_rng_records":rng,"global_rng_windows":mt,
        "gap_reasons":dict(reasons),"source_field_faults":dict(source_failures),"retained":retained,"unknown":unknown,
        "first_observed_preserved":first_observed,"readiness_revoked_reacquired":revoked,
        "lifecycle_hooks":dict(events),"reuse_groups":[{"address":a,"uses":sorted(v)} for a,v in by_address.items() if len(v)>1],
        "duplicate_births":[list(k) for k,v in births.items() if v>1],
        "repeat_publish":[{"key":k,"count":v,"births":births[k]} for k,v in publishes.items() if v>1],
        "membership_events":membership,"sampled_fixture_witnesses":witnesses,"short_remove":short,"fixture_init_arguments":seed_init}
    save(output / "readiness.json", ready)
    save(output / "identity-history.json", [{"key":k,**v,"sources":sorted(v["sources"])} for k,v in objects.items()])
    save(output / "evaluation.json",result)
    return result
