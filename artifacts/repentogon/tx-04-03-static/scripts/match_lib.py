# Shared matching library for the v1.9.7.12 IDB (static analysis only).
exec(open(r"C:\Users\114514\AppData\Local\Temp\claude\F--projects-isaac-issac-multiplay\910d48ab-e91d-4daa-b1b5-2a86b9899653\scratchpad\fp_common.py").read())
import re, difflib, os, time

V17 = json.load(open(SP + r"\v17_fingerprints.json"))
ANCH = {k: int(v, 16) for k, v in json.load(open(SP + r"\v12\repentogon_anchor_rva.json")).items()}
ANCH_BY_RVA = {}
for k, v in ANCH.items(): ANCH_BY_RVA.setdefault(v, []).append(k)
STATE = SP + r"\v12_state.json"
MAXM = 2500

def load_state():
    if os.path.exists(STATE): return {int(k, 16): v for k, v in json.load(open(STATE)).items()}
    return {}
def save_state(m):
    json.dump({hx(k): v for k, v in sorted(m.items())}, open(STATE, "w"), indent=1)

_text = ida_segment.get_segm_by_name(".text")
_TEXT = None
def text_bytes():
    global _TEXT
    if _TEXT is None: _TEXT = ida_bytes.get_bytes(_text.start_ea, _text.end_ea - _text.start_ea)
    return _TEXT

def search(pattern, limit=8):
    rx = b"".join(b"." if t == "??" else re.escape(bytes([int(t, 16)])) for t in pattern.split())
    res = []
    for m in re.finditer(rx, text_bytes(), re.DOTALL):
        res.append(_text.start_ea + m.start())
        if len(res) >= limit: break
    return res

_ins_cache = {}
def insns(fs):
    if fs not in _ins_cache: _ins_cache[fs] = [(ea, idc.print_insn_mnem(ea)) for ea in func_items(fs)]
    return _ins_cache[fs]

def sim(m17, fs12):
    m12 = [m for _, m in insns(fs12)][:MAXM]
    return difflib.SequenceMatcher(None, m17[:MAXM], m12, autojunk=False).ratio()

def align_map(f17, fs12):
    fp = V17["funcs"][hx(f17)]
    m17 = fp["mnem"]; i12 = insns(fs12)
    sm = difflib.SequenceMatcher(None, m17, [m for _, m in i12], autojunk=False)
    mp = {}
    for tag, a0, a1, b0, b1 in sm.get_opcodes():
        if tag == "equal":
            for k in range(a1 - a0): mp[a0 + k] = i12[b0 + k][0]
    return mp

def setm(m, f17, ea12, how, s):
    cur = m.get(f17)
    if cur is None or s > cur["sim"]:
        m[f17] = {"v12": rva(ea12), "how": how, "sim": round(s, 4)}
