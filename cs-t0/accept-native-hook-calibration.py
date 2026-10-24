"""One offline acceptance: native source declarations -> real PE sites -> native build.

Reads the target file as bytes. With --runtime-readonly, also reads the unique
running game's modules and patch sites using a VM_READ handle. Never loads/calls
game code, injects, sends input, or computes a digest. Create a new artifact directory;
failure is recorded without retry. Instruction/ABI interpretation is recorded
separately by the static IDA review, not inferred from a successful build.
"""
from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import traceback

ROOT = Path(__file__).resolve().parent.parent
NATIVE = ROOT / "cs-t0/native"


class PE:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        nt = self.u32(0x3C)
        if self.data[:2] != b"MZ" or self.data[nt:nt + 4] != b"PE\0\0":
            raise ValueError("Expected PE image")
        if self.u16(nt + 4) != 0x14C:
            raise ValueError("Expected x86 image")
        opt = nt + 24
        if self.u16(opt) != 0x10B:
            raise ValueError("Expected PE32 optional header")
        self.base = self.u32(opt + 28)
        self.size = self.u32(opt + 56)
        self.headers = self.u32(opt + 60)
        self.entry = self.u32(opt + 16)
        self.timestamp = self.u32(nt + 8)
        self.sections = []
        start = opt + self.u16(nt + 20)
        for index in range(self.u16(nt + 6)):
            pos = start + index * 40
            self.sections.append({
                "name": self.data[pos:pos + 8].rstrip(b"\0").decode("ascii"),
                "virtual_size": self.u32(pos + 8), "rva": self.u32(pos + 12),
                "raw_size": self.u32(pos + 16), "raw_offset": self.u32(pos + 20),
                "characteristics": self.u32(pos + 36),
            })
        self.imports = {}
        descriptor_rva = self.u32(opt + 104)
        descriptor_size = self.u32(opt + 108)
        if descriptor_rva:
            for offset in range(0, descriptor_size, 20):
                desc = struct.unpack("<5I", self.read(descriptor_rva + offset, 20))
                if not any(desc):
                    break
                original, _, _, name, first = desc
                module = self.string(name)
                for index in range(self.size // 4):
                    value = struct.unpack("<I", self.read((original or first) + 4 * index, 4))[0]
                    if not value:
                        break
                    if value & 0x80000000:
                        continue
                    symbol = self.string(value + 2)
                    self.imports.setdefault(symbol, []).append({
                        "module": module, "rva": first + 4 * index,
                        "original_first_thunk": original,
                    })

    def u16(self, offset):
        return struct.unpack_from("<H", self.data, offset)[0]

    def u32(self, offset):
        return struct.unpack_from("<I", self.data, offset)[0]

    def read(self, rva, size):
        if 0 <= rva and rva + size <= self.headers:
            offset = rva
        else:
            section = next((s for s in self.sections
                            if s["rva"] <= rva and rva + size <= s["rva"] + s["raw_size"]), None)
            if section is None:
                raise ValueError(f"RVA {rva:#x}+{size} has no complete file backing")
            offset = section["raw_offset"] + rva - section["rva"]
        value = self.data[offset:offset + size]
        if len(value) != size:
            raise ValueError("Truncated PE read")
        return value

    def string(self, rva):
        value = bytearray()
        for offset in range(1024):
            char = self.read(rva + offset, 1)[0]
            if not char:
                return value.decode("ascii")
            value.append(char)
        raise ValueError("Unterminated PE string")


def source(name):
    value = (NATIVE / "src" / name).read_text(encoding="utf-8-sig")
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", value, flags=re.S)


def number(value):
    return int(re.sub(r"[uUlL]+$", "", value), 0)


def scalar(text, name):
    match = re.search(r"\b" + re.escape(name) + r"\s*=\s*(0x[0-9a-fA-F]+|\d+)[uUlL]*", text)
    if not match:
        raise ValueError(f"Missing source scalar {name}")
    return number(match[1])


def values(text, name):
    match = re.search(r"\b" + re.escape(name) + r"\b(?:\s*\[[^\]]*\])*\s*(?:=\s*)?\{", text)
    if not match:
        raise ValueError(f"Missing source initializer {name}")
    start, depth = match.end() - 1, 0
    for end in range(start, len(text)):
        depth += (text[end] == "{") - (text[end] == "}")
        if depth == 0:
            return [number(v) for v in re.findall(r"\b(?:0x[0-9a-fA-F]+|\d+)[uUlL]*\b", text[start:end + 1])]
    raise ValueError(f"Unterminated initializer {name}")


def run(pe, artifact):
    rows = []
    profile = (NATIVE / "include/isaac_cs_t0/target_image.hpp").read_text(encoding="utf-8-sig")
    rows.append({"kind": "pe_metadata", "source": "target_image.hpp",
                 "passed": pe.timestamp == scalar(profile, "kExpectedTimestamp") and
                 ((len(pe.sections) == 5 and pe.size == scalar(profile, "kUnpackedImageSize") and
                   pe.entry == scalar(profile, "kUnpackedEntryPointRva")) or
                  (len(pe.sections) == 6 and pe.size == scalar(profile, "kCanonicalImageSize") and
                   pe.entry == scalar(profile, "kCanonicalEntryPointRva")))})

    def record(kind, owner, rva, expected, **facts):
        actual = pe.read(rva, len(expected))
        rows.append({"kind": kind, "source": owner, "rva": rva,
                     "expected": expected.hex(" "), "actual": actual.hex(" "),
                     "passed": actual == expected, **facts})

    def relative(owner, rvas, expected, targets=None):
        if len(expected) != 5 * len(rvas) or (targets is not None and len(targets) != len(rvas)):
            raise ValueError(f"Incomplete source relative catalog: {owner}")
        for index, rva in enumerate(rvas):
            raw = bytes(expected[index * 5:index * 5 + 5])
            target = rva + 5 + struct.unpack("<i", raw[1:])[0]
            record("relative", f"{owner}[{index}]", rva, raw, target_rva=target)
            if raw[0] != 0xE8 or (targets is not None and target != targets[index]):
                rows[-1]["passed"] = False
                rows[-1]["error"] = "Source call opcode/target is inconsistent"

    for filename, rvas, targets, raw in [
        ("loop_runtime.cpp", "kCallRvas", "kTargetRvas", "kCallBytes"),
        ("active_targets.cpp", "kCallRvas", "kCallTargetRvas", "kCallBytes"),
        ("raw_apply_runtime.cpp", "kConsumerSiteRvas", "kConsumerTargetRvas", "kConsumerCallBytes"),
        ("boundary_runtime.cpp", "kCallRvas", "kTargetRvas", "kCallBytes"),
        ("coop_passive_runtime.cpp", "kCoopCallRvas", "kCoopTargetRvas", "kCoopBytes"),
    ]:
        text = source(filename)
        relative(filename, values(text, rvas), values(text, raw), values(text, targets))
    passive = source("coop_passive_runtime.cpp")
    relative("coop_passive_runtime.cpp:kDeviceCallRvas", values(passive, "kDeviceCallRvas"),
             values(passive, "kDeviceCallBytes"), [0x3A6450] * 3)

    mode = source("runtime.cpp")
    for label in ("Frame", "Dispatch", "PreGameExit"):
        relative(f"runtime.cpp:{label}", [scalar(mode, f"k{label}CallRva")],
                 values(mode, f"k{label}Bytes" if label != "PreGameExit" else "kPreGameExitCallBytes"))
    relative("runtime.cpp:kPostGameStartCallRvas", values(mode, "kPostGameStartCallRvas"),
             values(mode, "kPostGameStartCallBytes"), [0x464EE0] * 6)
    relative("runtime.cpp:kRunTeardownCallRvas", values(mode, "kRunTeardownCallRvas"),
             values(mode, "kRunTeardownCallBytes"), [0x2FA0C0] * 2)

    coop = source("coop_active_runtime.cpp").split("bool Prepare() noexcept", 1)[1]
    relative("coop_active_runtime.cpp:rvas", values(coop, "rvas"), values(coop, "bytes"), values(coop, "targets"))
    player_only = re.search(r"player_only\.call\s*=\s*g_image\s*\+\s*(0x[0-9A-Fa-f]+)", coop)
    if player_only is None:
        raise ValueError("Missing player-only call source")
    relative("coop_active_runtime.cpp:player_only", [int(player_only[1], 16)], values(coop, "player_only_bytes"), [0x2FD3F0])
    relative("coop_active_runtime.cpp:mode_rvas", values(coop, "mode_rvas"), values(coop, "mode_bytes"), [0x5BA980] * 3)

    active = source("active_targets.cpp")
    for index, (rva, target) in enumerate(zip(values(active, "kPointerRvas"), values(active, "kPointerTargetRvas"), strict=True)):
        record("pointer", f"active_targets.cpp:kPointerRvas[{index}]", rva, struct.pack("<I", pe.base + target), target_rva=target)
    for filename in ("input_runtime.cpp", "raw_apply_runtime.cpp"):
        text = source(filename)
        for table, pairs in [
            ("InputManager", ((0x40, "ManagerPressed"), (0x44, "ManagerTriggered"), (0x48, "ManagerScalar"))),
            ("Keyboard", ((0x30, "LogicalPressed"), (0x34, "LogicalTriggered"), (0x38, "LogicalScalar"), (0x4C, "LogicalSetter"))),
            ("Gamepad", ((0x30, "LogicalPressed"), (0x34, "LogicalTriggered"), (0x38, "LogicalScalar"), (0x4C, "LogicalSetter"))),
        ]:
            table_rva = scalar(text, f"k{table}VtableRva")
            for offset, target_name in pairs:
                target = scalar(text, f"k{target_name}Rva")
                record("pointer", f"{filename}:{table}+{offset:#x}", table_rva + offset,
                       struct.pack("<I", pe.base + target), target_rva=target)
        for table in ("InputManager", "Keyboard", "Gamepad"):
            record("col", f"{filename}:{table}", scalar(text, f"k{table}VtableRva") - 4,
                   struct.pack("<I", pe.base + scalar(text, f"k{table}ColRva")))
    loop = source("loop_runtime.cpp")
    for device in ("Keyboard", "Gamepad"):
        target = scalar(loop, f"k{device}PollRva")
        record("pointer", f"loop_runtime.cpp:{device}Poll", scalar(loop, f"k{device}VtableRva") + 0x78,
               struct.pack("<I", pe.base + target), target_rva=target)

    world = source("world_record.cpp").split("bool Prepare(", 1)[1]
    rvas, lengths = values(world, "rvas"), values(world, "lengths")
    # C++ zero-fills each seven-byte row. Parse each row, not a flattened padded array.
    raw_start = re.search(r"\bbytes\[19\]\[7\]\s*\{", world)
    if raw_start is None:
        raise ValueError("Missing world entry source rows")
    raw_rows = re.findall(r"\{([^{}]+)\}", world[raw_start.end():world.index("for(std::size_t i=0;i<19", raw_start.end())])
    if len(rvas) != 19 or len(lengths) != 19 or len(raw_rows) != 19:
        raise ValueError("Incomplete 19-entry world catalog")
    for index, (rva, length, row) in enumerate(zip(rvas, lengths, raw_rows, strict=True)):
        expected = bytes(number(v) for v in re.findall(r"\b(?:0x[0-9a-fA-F]+|\d+)[uUlL]*\b", row))
        record("entry", f"world_record.cpp:rvas[{index}]", rva, expected[:length], length=length)

    expected_modules = {"lua_newstate": "lua5.3.3r.dll", "luaL_newstate": "lua5.3.3r.dll",
                        "lua_close": "lua5.3.3r.dll", "lua_pcallk": "lua5.3.3r.dll",
                        "SteamInternal_ContextInit": "steam_api.dll", "PeekMessageW": "user32.dll",
                        "TranslateMessage": "user32.dll", "DispatchMessageW": "user32.dll"}
    for symbol, module in expected_modules.items():
        imports = pe.imports.get(symbol, [])
        rows.append({"kind": "import", "symbol": symbol, "imports": imports,
                     "passed": len(imports) == 1 and imports[0]["module"].lower() == module})
    for name, symbol in (("Peek", "PeekMessageW"), ("Translate", "TranslateMessage"), ("Dispatch", "DispatchMessageW")):
        expected_rva = scalar(loop, f"k{name}IatRva")
        rows.append({"kind": "iat_rva", "source": "loop_runtime.cpp", "symbol": symbol,
                     "rva": expected_rva, "passed": any(i["rva"] == expected_rva for i in pe.imports.get(symbol, []))})
    for filename in ("runtime.cpp", "boundary_runtime.cpp", "offline_attestation_runtime.cpp"):
        text = source(filename)
        for label, symbol in (("LuaNewstate", "lua_newstate"), ("LuaLNewstate", "luaL_newstate"), ("LuaClose", "lua_close"), ("LuaPcall", "lua_pcallk")):
            if not re.search(r"\bk" + label + r"CallRva\s*=", text):
                continue
            rva = scalar(text, f"k{label}CallRva")
            imports = pe.imports.get(symbol, [])
            actual = pe.read(rva, 6)
            expected = b"\xff\x15" + struct.pack("<I", pe.base + imports[0]["rva"]) if len(imports) == 1 else b""
            rows.append({"kind": "iat_call", "source": filename, "symbol": symbol,
                         "rva": rva, "expected": expected.hex(" "), "actual": actual.hex(" "),
                         "passed": actual == expected})

    offline = source("offline_attestation_runtime.cpp")
    menu = offline.split("bool MenuPumpMatches(", 1)[1]
    pump = scalar(offline, "kGenericPumpRva")
    prefix, middle = bytes(values(menu, "prefix")), bytes(values(menu, "middle"))
    record("instruction", "offline_attestation_runtime.cpp:menu-prefix", pump, prefix)
    record("pointer", "offline_attestation_runtime.cpp:cookie-address", pump + len(prefix),
           struct.pack("<I", pe.base + scalar(offline, "kSecurityCookieRva")))
    record("instruction", "offline_attestation_runtime.cpp:menu-middle", pump + len(prefix) + 4, middle)
    record("pointer", "offline_attestation_runtime.cpp:menu-peek-address", pump + len(prefix) + 4 + len(middle),
           struct.pack("<I", pe.base + scalar(offline, "kPeekMessageIatRva")))
    record("instruction", "offline_attestation_runtime.cpp:menu-call", scalar(offline, "kMenuPeekCallRva"), bytes(values(menu, "call")))
    owned = source("owned_virtual_device.cpp")
    for rva in (0x61E050, 0xCC10):
        record("dependency", "owned_virtual_device.cpp:ctor_start", rva, bytes(values(owned, "ctor_start")))
    record("dependency", "owned_virtual_device.cpp:init_start", 0x61E210, bytes(values(owned, "init_start")))

    (artifact / "sites.json").write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return rows


def build(artifact):
    vswhere = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio/Installer/vswhere.exe"
    query = subprocess.run([str(vswhere), "-latest", "-version", "[17.0,18.0)", "-products", "*",
                            "-requires", "Microsoft.Component.MSBuild", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                            "-property", "installationPath"], capture_output=True, text=True, check=True)
    vs = Path(query.stdout.strip())
    msbuild = vs / "MSBuild/Current/Bin/amd64/MSBuild.exe"
    results = []
    tools_version = (vs / "VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt").read_text().strip()
    build_env = dict(os.environ)
    build_env["PATH"] = str(vs / f"VC/Tools/MSVC/{tools_version}/bin/Hostx64/x64") + os.pathsep + build_env["PATH"]
    rust_command = ["cargo", "build", "--manifest-path", str(ROOT / "cs-t0/Cargo.toml"),
                    "--locked", "--release", "--bin", "IsaacCsT0T09",
                    "--target-dir", str(artifact / "build/Rust")]
    with (artifact / "Rust-build.log").open("xb") as log:
        result = subprocess.run(rust_command, stdout=log, stderr=subprocess.STDOUT, cwd=ROOT, env=build_env)
    results.append({"label": "Rust", "command": rust_command, "exit_code": result.returncode})
    if result.returncode:
        return results
    for label, project, extra in (("Native", "IsaacCsT0.Native.vcxproj", []),
                                  ("Native-WorldE2E", "IsaacCsT0.Native.vcxproj", ["/p:WorldE2E=true"]),
                                  ("Helper", "IsaacCsT0.Helper.vcxproj", [])):
        output = artifact / "build" / label
        command = [str(msbuild), str(NATIVE / project), "/m:1", "/nr:false", "/nologo", "/verbosity:minimal",
                   "/p:Configuration=Release", "/p:Platform=Win32", "/p:PlatformToolset=v143",
                   "/p:PreferredToolArchitecture=x64", f"/p:OutDir={output / 'bin'}\\", f"/p:IntDir={output / 'obj'}\\", *extra]
        with (artifact / f"{label}-build.log").open("xb") as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, cwd=ROOT)
        results.append({"label": label, "command": command, "exit_code": result.returncode})
        if result.returncode:
            break
    return results


def runtime_snapshot(pe, rows):
    """Observation only: no thread suspension, writes, patching, or callbacks."""
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)

    class ProcessEntry(ctypes.Structure):
        _fields_ = [("size", wintypes.DWORD), ("usage", wintypes.DWORD), ("pid", wintypes.DWORD),
                    ("heap", ctypes.c_size_t), ("module", wintypes.DWORD), ("threads", wintypes.DWORD),
                    ("parent", wintypes.DWORD), ("priority", wintypes.LONG), ("flags", wintypes.DWORD),
                    ("name", wintypes.WCHAR * 260)]

    class ModuleEntry(ctypes.Structure):
        _fields_ = [("size", wintypes.DWORD), ("id", wintypes.DWORD), ("pid", wintypes.DWORD),
                    ("global_usage", wintypes.DWORD), ("process_usage", wintypes.DWORD),
                    ("base", ctypes.c_void_p), ("image_size", wintypes.DWORD), ("handle", wintypes.HMODULE),
                    ("name", wintypes.WCHAR * 256), ("path", wintypes.WCHAR * 260)]

    kernel.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
    kernel.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    for name, entry_type in (("Process32FirstW", ProcessEntry), ("Process32NextW", ProcessEntry),
                             ("Module32FirstW", ModuleEntry), ("Module32NextW", ModuleEntry)):
        function = getattr(kernel, name)
        function.argtypes = [wintypes.HANDLE, ctypes.POINTER(entry_type)]
        function.restype = wintypes.BOOL
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.ReadProcessMemory.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p,
                                         ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
    kernel.ReadProcessMemory.restype = wintypes.BOOL
    kernel.CheckRemoteDebuggerPresent.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.BOOL)]
    kernel.CheckRemoteDebuggerPresent.restype = wintypes.BOOL
    invalid = ctypes.c_void_p(-1).value

    def enumerate_entries(flags, pid, entry_type, first, following):
        handle = kernel.CreateToolhelp32Snapshot(flags, pid)
        if handle == invalid:
            raise ctypes.WinError(ctypes.get_last_error())
        try:
            entry = entry_type()
            entry.size = ctypes.sizeof(entry)
            result = []
            found = getattr(kernel, first)(handle, ctypes.byref(entry))
            if not found and ctypes.get_last_error() != 18:
                raise ctypes.WinError(ctypes.get_last_error())
            while found:
                result.append(bytes(entry))
                found = getattr(kernel, following)(handle, ctypes.byref(entry))
            if ctypes.get_last_error() != 18:
                raise ctypes.WinError(ctypes.get_last_error())
            return [entry_type.from_buffer_copy(value) for value in result]
        finally:
            kernel.CloseHandle(handle)

    processes = enumerate_entries(2, 0, ProcessEntry, "Process32FirstW", "Process32NextW")
    matches = [p for p in processes if p.name.lower() == "isaac-ng.exe"]
    if len(matches) != 1:
        return {"status": "NOT_READ", "reason": "Unique running isaac-ng.exe required", "candidate_count": len(matches)}
    pid = matches[0].pid
    modules = enumerate_entries(0x18, pid, ModuleEntry, "Module32FirstW", "Module32NextW")
    game = next(m for m in modules if m.name.lower() == "isaac-ng.exe")
    process = kernel.OpenProcess(0x0410, False, pid)  # QUERY_INFORMATION | VM_READ
    if not process:
        raise ctypes.WinError(ctypes.get_last_error())

    def read(address, size):
        buffer = ctypes.create_string_buffer(size)
        count = ctypes.c_size_t()
        if not kernel.ReadProcessMemory(process, address, buffer, size, ctypes.byref(count)) or count.value != size:
            raise ctypes.WinError(ctypes.get_last_error())
        return buffer.raw

    try:
        base = game.base
        nt = struct.unpack("<I", read(base + 0x3C, 4))[0]
        headers = read(base + nt, 24 + 96)
        header_base = struct.unpack_from("<I", headers, 24 + 28)[0]
        image_size = struct.unpack_from("<I", headers, 24 + 56)[0]
        debugger = wintypes.BOOL()
        debugger_known = bool(kernel.CheckRemoteDebuggerPresent(process, ctypes.byref(debugger)))
        conflicts = [m.name for m in modules if m.name.lower() in {
            "isaacsync.native.dll", "tractor_beam_native_hook.dll", "x32dbg.dll", "x64dbg.dll"}]
        observed = []
        for row in rows:
            if row["kind"] not in ("relative", "pointer", "col", "entry", "iat_call", "instruction", "dependency"):
                continue
            expected = bytes.fromhex(row["expected"])
            if row["kind"] in ("pointer", "col"):
                expected = struct.pack("<I", struct.unpack("<I", expected)[0] - pe.base + base)
            elif row["kind"] == "iat_call" and expected[:2] == b"\xff\x15":
                expected = expected[:2] + struct.pack("<I", struct.unpack("<I", expected[2:])[0] - pe.base + base)
            actual = read(base + row["rva"], len(expected))
            observed.append({"source": row["source"], "kind": row["kind"], "rva": row["rva"],
                             "expected": expected.hex(" "), "actual": actual.hex(" "),
                             "matches_original": actual == expected})
        imports = []
        for symbol in ("PeekMessageW", "TranslateMessage", "DispatchMessageW", "lua_newstate",
                       "luaL_newstate", "lua_close", "lua_pcallk", "SteamInternal_ContextInit"):
            for slot in pe.imports.get(symbol, []):
                pointer = struct.unpack("<I", read(base + slot["rva"], 4))[0]
                owner = next((m.name for m in modules if m.base <= pointer < m.base + m.image_size), None)
                imports.append({"symbol": symbol, "rva": slot["rva"], "pointer": pointer, "owner": owner,
                                "expected_owner": slot["module"],
                                "owner_matches": owner is not None and owner.lower() == slot["module"].lower()})
        return {"status": "READ_ONLY_OBSERVED", "pid": pid, "loaded_base": base,
                "header_image_base": header_base, "header_base_matches_loaded": header_base == base,
                "image_size": image_size, "game_path": game.path,
                "debugger_present": bool(debugger.value) if debugger_known else None,
                "conflicting_modules": conflicts,
                "modules": [{"name": m.name, "base": m.base, "size": m.image_size} for m in modules],
                "sites": observed, "imports": imports,
                "all_original_bytes_match": all(r["matches_original"] for r in observed),
                "all_import_owners_match": all(r["owner_matches"] for r in imports),
                "installation_or_restore_executed": False}
    finally:
        kernel.CloseHandle(process)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-exe", required=True, type=Path)
    parser.add_argument("--artifact-dir", required=True, type=Path)
    parser.add_argument("--runtime-readonly", action="store_true")
    args = parser.parse_args()
    artifact = args.artifact_dir.resolve()
    artifact.relative_to(ROOT / "artifacts/cs-t0")
    artifact.mkdir(parents=True, exist_ok=False)
    summary = {"transaction": "tx-03-04", "scope": "PE_SOURCE_AND_RUNTIME_READONLY" if args.runtime_readonly else "OFFLINE_PE_AND_SOURCE_ONLY",
               "target": str(args.game_exe.resolve()), "automatic_retry": False,
               "game_loaded_or_executed": False, "runtime_abi_thread_restore": "NOT_RUN"}
    try:
        shutil.copytree(NATIVE, artifact / "native-source")
        shutil.copy2(__file__, artifact / Path(__file__).name)
        rust_source = artifact / "authority_game.rs"
        shutil.copy2(ROOT / "cs-t0/src/t09/authority_game.rs", rust_source)
        pe = PE(args.game_exe)
        (artifact / "target-image.bin").write_bytes(pe.data)
        summary["pe"] = {"image_base": pe.base, "image_size": pe.size, "entry_rva": pe.entry,
                         "timestamp": pe.timestamp, "sections": pe.sections}
        rows = run(pe, artifact)
        if args.runtime_readonly:
            try:
                summary["runtime_snapshot"] = runtime_snapshot(pe, rows)
            except Exception:
                summary["runtime_snapshot"] = {"status": "READ_FAILED", "error": traceback.format_exc()}
            (artifact / "runtime-readonly.json").write_text(json.dumps(summary["runtime_snapshot"], ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        summary["checks"] = len(rows)
        summary["passed"] = sum(row["passed"] for row in rows)
        summary["unique_locations"] = len({row["rva"] for row in rows if "rva" in row})
        summary["builds"] = build(artifact)
        summary["status"] = "PASS" if all(row["passed"] for row in rows) and len(summary["builds"]) == 4 and all(r["exit_code"] == 0 for r in summary["builds"]) else "FAIL"
    except Exception:
        summary["status"] = "FAIL"
        summary["error"] = traceback.format_exc()
    (artifact / "summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    snapshot = summary.get("runtime_snapshot", {})
    print(json.dumps({"status": summary["status"], "checks": summary.get("checks"),
                      "passed": summary.get("passed"), "artifact": str(artifact),
                      "builds": [{"label": r["label"], "exit_code": r["exit_code"]} for r in summary.get("builds", [])],
                      "runtime_snapshot": {key: snapshot.get(key) for key in ("status", "pid", "all_original_bytes_match", "all_import_owners_match", "header_base_matches_loaded", "conflicting_modules", "debugger_present")},
                      "error": summary.get("error")}, ensure_ascii=False))
    return 0 if summary["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
