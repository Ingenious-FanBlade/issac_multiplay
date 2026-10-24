"""Read-only external evidence plus one bounded pause of our controller reader.

Failure cases are specified in tx-01-77-live-remaining.md before this code.
No work, process access, or API initialization occurs at import time.
"""
from __future__ import annotations

import ctypes as C
from ctypes import wintypes as W
import struct
import time
from pathlib import Path

from .artifacts import append, load, save, utc


class ModuleInfo(C.Structure):
    _fields_ = [("base", C.c_void_p), ("size", W.DWORD), ("entry", C.c_void_p)]


class Thread(C.Structure):
    _fields_ = [(name, W.DWORD) for name in ("dwSize", "cntUsage", "th32ThreadID", "th32OwnerProcessID")] + [
        ("tpBasePri", W.LONG), ("tpDeltaPri", W.LONG), ("dwFlags", W.DWORD)]


class ProcessEntry(C.Structure):
    _fields_ = [("dwSize",W.DWORD),("cntUsage",W.DWORD),("pid",W.DWORD),
        ("heap",C.c_size_t),("module",W.DWORD),("threads",W.DWORD),("parent",W.DWORD),
        ("priority",W.LONG),("flags",W.DWORD),("executable",W.WCHAR*260)]


class Api:
    def __init__(self):
        self.dll = C.WinDLL("kernel32", use_last_error=True)
        signatures = {
            "CloseHandle": ([W.HANDLE], W.BOOL), "OpenProcess": ([W.DWORD, W.BOOL, W.DWORD], W.HANDLE),
            "QueryFullProcessImageNameW": ([W.HANDLE, W.DWORD, W.LPWSTR, C.POINTER(W.DWORD)], W.BOOL),
            "GetProcessTimes": ([W.HANDLE] + [C.POINTER(W.FILETIME)] * 4, W.BOOL),
            "ReadProcessMemory": ([W.HANDLE, C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)], W.BOOL),
            "CreateToolhelp32Snapshot": ([W.DWORD, W.DWORD], W.HANDLE),
            "Process32FirstW": ([W.HANDLE,C.POINTER(ProcessEntry)],W.BOOL),
            "Process32NextW": ([W.HANDLE,C.POINTER(ProcessEntry)],W.BOOL),
            "K32EnumProcessModulesEx": ([W.HANDLE, C.POINTER(W.HMODULE), W.DWORD, C.POINTER(W.DWORD), W.DWORD], W.BOOL),
            "K32GetModuleInformation": ([W.HANDLE, W.HMODULE, C.POINTER(ModuleInfo), W.DWORD], W.BOOL),
            "K32GetModuleFileNameExW": ([W.HANDLE, W.HMODULE, W.LPWSTR, W.DWORD], W.DWORD),
            "Thread32First": ([W.HANDLE, C.POINTER(Thread)], W.BOOL),
            "Thread32Next": ([W.HANDLE, C.POINTER(Thread)], W.BOOL),
            "OpenThread": ([W.DWORD, W.BOOL, W.DWORD], W.HANDLE),
            "GetProcessIdOfThread": ([W.HANDLE], W.DWORD),
            "GetThreadDescription": ([W.HANDLE, C.POINTER(C.c_void_p)], W.LONG),
            "LocalFree": ([C.c_void_p], C.c_void_p),
            "SuspendThread": ([W.HANDLE], W.DWORD), "ResumeThread": ([W.HANDLE], W.DWORD),
            "OpenFileMappingW": ([W.DWORD, W.BOOL, W.LPCWSTR], W.HANDLE),
            "MapViewOfFile": ([W.HANDLE, W.DWORD, W.DWORD, W.DWORD, C.c_size_t], C.c_void_p),
            "UnmapViewOfFile": ([C.c_void_p], W.BOOL),
        }
        for name, (arguments, result) in signatures.items():
            function = getattr(self.dll, name)
            function.argtypes, function.restype = arguments, result
            setattr(self, name, function)

    @staticmethod
    def checked(value, operation):
        if not value or value == C.c_void_p(-1).value:
            raise OSError(C.get_last_error(), operation)
        return value

    def identity(self, handle):
        text, length = C.create_unicode_buffer(32768), W.DWORD(32768)
        self.checked(self.QueryFullProcessImageNameW(handle, 0, text, C.byref(length)), "query process path")
        times = [W.FILETIME() for _ in range(4)]
        self.checked(self.GetProcessTimes(handle, *(C.byref(value) for value in times)), "query process time")
        created = times[0].dwLowDateTime | (times[0].dwHighDateTime << 32)
        return Path(text.value).resolve(), str(created)

    def modules(self, handle):
        # Same PSAPI LIST_MODULES_32BIT route as src/win32.rs; bind to the
        # already identity-checked process handle and retain full module paths.
        modules, needed = (W.HMODULE * 4096)(), W.DWORD()
        self.checked(self.K32EnumProcessModulesEx(handle, modules, C.sizeof(modules), C.byref(needed), 1), "enumerate 32-bit modules")
        if needed.value == 0 or needed.value > C.sizeof(modules) or needed.value % C.sizeof(W.HMODULE):
            raise ValueError("Module count is empty/outside fixed 4096-module budget")
        result = []
        for module in modules[:needed.value // C.sizeof(W.HMODULE)]:
            info, path = ModuleInfo(), C.create_unicode_buffer(32768)
            self.checked(self.K32GetModuleInformation(handle, module, C.byref(info), C.sizeof(info)), "read module information")
            length = self.checked(self.K32GetModuleFileNameExW(handle, module, path, len(path)), "read module path")
            if length >= len(path):
                raise ValueError("Module path truncated")
            result.append({"name": Path(path.value).name, "path": path.value, "base": info.base, "size": info.size})
        return result


def discover_unique(executable,directory):
    """One read-only selection in the unified invocation, preserving failure data."""
    directory=Path(directory);directory.mkdir(parents=True,exist_ok=False)
    report={"at_utc":utc(),"api":"Toolhelp TH32CS_SNAPPROCESS + checked Win32 identity + PSAPI LIST_MODULES_32BIT",
        "executable":str(Path(executable).resolve()),"entry_bytes":C.sizeof(ProcessEntry),
        "candidates":[],"scanned":0,"error":None,"automatic_retry":False}
    api=None;snapshot=None;process=None
    try:
        api=Api()
        snapshot=api.checked(api.CreateToolhelp32Snapshot(2,0),"process snapshot")
        entry=ProcessEntry();entry.dwSize=C.sizeof(entry)
        found=api.Process32FirstW(snapshot,C.byref(entry))
        while found:
            report["scanned"]+=1
            if entry.executable.casefold()=="isaac-ng.exe":
                report["candidates"].append({"pid":int(entry.pid),"executable_name":entry.executable,
                    "parent_pid":int(entry.parent),"threads":int(entry.threads)})
            found=api.Process32NextW(snapshot,C.byref(entry))
        terminal=C.get_last_error();report["enumeration_end_win32_error"]=terminal
        if terminal!=18:raise OSError(terminal,"process enumeration ended unexpectedly")
        if len(report["candidates"])!=1:
            raise RuntimeError(f"Expected one isaac-ng.exe; Win32 returned {len(report['candidates'])} candidates; see {directory / 'enumeration.json'}")
        candidate=report["candidates"][0]
        process=api.checked(api.OpenProcess(0x1410,False,candidate["pid"]),"open candidate for identity/modules")
        path,created=api.identity(process)
        candidate["path"],candidate["creation_time_100ns"]=str(path),created
        if path!=Path(executable).resolve():raise RuntimeError("Unique game's executable differs from requested path")
        inventory=api.modules(process)
        save(directory / "modules.json",inventory)
        result={"pid":candidate["pid"],"creation_time_100ns":created,"executable":str(path),
            "modules":[item["name"] for item in inventory]}
        report["selected"]=result
        return result
    except BaseException as exc:
        report["error"]={"type":type(exc).__name__,"message":str(exc)}
        raise
    finally:
        if process:api.CloseHandle(process)
        if snapshot and snapshot!=C.c_void_p(-1).value:api.CloseHandle(snapshot)
        save(directory / "enumeration.json",report)


class Game:
    def __init__(self, api, expected, executable, catalogue, directory):
        self.api, self.expected, self.directory = api, expected, Path(directory)
        self.handle = api.checked(api.OpenProcess(0x100000 | 0x1000 | 0x400 | 0x10, False, expected["pid"]), "open read-only game")
        try:
            path, created = api.identity(self.handle)
            if path != Path(executable).resolve() or created != expected["creation_time_100ns"]:
                raise ValueError("Game path/creation-time mismatch")
            modules = api.modules(self.handle)
            save(self.directory / "module-inventory-before.json", {"at_utc": utc(), "api": "PSAPI_LIST_MODULES_32BIT",
                "process": expected, "modules": modules})
            candidates = [item for item in modules if Path(item["path"]).resolve() == path]
            if len(candidates) != 1:
                raise ValueError("No unique main module by checked full executable path; see module-inventory-before.json")
            self.image = candidates[0]
            self.targets = []
            for item in load(catalogue)["targets"]:
                if item["action"] == 2**32 - 1:
                    continue
                rva, width = item["action"], item["query"]
                if not 0 <= rva < self.image["size"] or not 1 <= width <= 16 or rva + width > self.image["size"]:
                    raise ValueError("Target RVA/width outside the observed image")
                old = struct.pack("<4I", *item["before"])[:width]
                actual = self.read(self.image["base"] + rva, width)
                # The three USER32 IAT slots are part of the same 70-slot
                # catalogue. Match the native owner rule, not a game-only rule.
                if width == 4:
                    pointer = int.from_bytes(actual, "little")
                    expected_owner = "user32.dll" if rva in {0x718714, 0x718650, 0x7185b8} else "isaac-ng.exe"
                    owners = [module["name"].lower() for module in modules if module["base"] <= pointer < module["base"] + module["size"]]
                    append(self.directory / "pointer-owners-before.jsonl", {"rva": rva, "actual": actual.hex(),
                        "pointer": pointer, "expected_owner": expected_owner, "observed_owners": owners})
                    if owners != [expected_owner]:
                        raise ValueError(f"Pre-injection pointer owner mismatch: RVA={rva:08x}, expected={expected_owner}, actual={owners}")
                elif actual != old:
                    raise ValueError("Unknown pre-injection entry/call patch; injection not attempted")
                self.targets.append({"rva": rva, "width": width, "original": actual.hex()})
            if len(self.targets) != 70 or len({item["rva"] for item in self.targets}) != 70:
                raise ValueError("Expected 70 unique restoration targets")
            save(self.directory / "before-injection.json", {"at_utc": utc(), "process": expected,
                "access": "PROCESS_VM_READ|PROCESS_QUERY_INFORMATION|PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE",
                "image": self.image, "targets": self.targets, "catalogue": str(Path(catalogue).resolve())})
        except BaseException:
            self.close()
            raise

    def read(self, address, size):
        buffer, received = C.create_string_buffer(size), C.c_size_t()
        self.api.checked(self.api.ReadProcessMemory(self.handle, address, buffer, size, C.byref(received)), "read game bytes")
        if received.value != size:
            raise ValueError("Short game memory read")
        return buffer.raw

    def snapshot(self, name):
        records = [{**item, "actual": self.read(self.image["base"] + item["rva"], item["width"]).hex()} for item in self.targets]
        result = {"at_utc": utc(), "process": self.expected, "targets": records,
                  "restored_count": sum(item["actual"] == item["original"] for item in records)}
        save(self.directory / (name + ".json"), result)
        return result

    def close(self):
        if self.handle:
            self.api.CloseHandle(self.handle)
            self.handle = None


class ReadMapping:
    def __init__(self, api, name, size):
        self.api, self.size, self.view = api, size, None
        self.handle = api.checked(api.OpenFileMappingW(4, False, name), "open read-only mapping")
        try:
            self.view = api.checked(api.MapViewOfFile(self.handle, 4, 0, 0, size), "map read-only view")
        except BaseException:
            self.close()
            raise

    def read(self, offset, length):
        if offset < 0 or length < 0 or offset + length > self.size:
            raise ValueError("Read outside fixed mapping layout")
        return C.string_at(self.view + offset, length)

    def close(self):
        if self.view:
            self.api.UnmapViewOfFile(self.view)
            self.view = None
        if self.handle:
            self.api.CloseHandle(self.handle)
            self.handle = None


class Observer:
    def __init__(self, api, metadata_path, game, child, controller, directory):
        self.api, self.game, self.directory = api, game, Path(directory)
        self.coop = self.world = None
        self.child = child
        try:
            meta = load(metadata_path)
            if (meta["version"] != 1 or meta["game_pid"] != game.expected["pid"] or meta["game_creation_time_100ns"] != game.expected["creation_time_100ns"]
                    or meta["controller_pid"] != child.pid or child.poll() is not None or meta["coop_version"] != 4 or meta["world_version"] != 2):
                raise ValueError("Observation metadata process/version mismatch")
            handle = api.checked(api.OpenProcess(0x1000 | 0x100000, False, child.pid), "query own controller")
            try:
                path, created = api.identity(handle)
                if path != Path(controller).resolve() or created != meta["controller_creation_time_100ns"]:
                    raise ValueError("Controller path/creation-time mismatch")
            finally:
                api.CloseHandle(handle)
            self.nonce = bytes.fromhex(meta["nonce"])
            if len(self.nonce) != 32 or meta["session"] != self.nonce[:16].hex():
                raise ValueError("Observation nonce/session mismatch")
            if meta["coop_mapping"] != "Local\\IsaacCsT0.CoopActive.v4." + meta["nonce"] or meta["world_mapping"] != "Local\\IsaacCsT0.World." + meta["nonce"]:
                raise ValueError("Unexpected mapping names")
            self.meta = meta
            self.coop = ReadMapping(api, meta["coop_mapping"], 4205096)
            self.world = ReadMapping(api, meta["world_mapping"], 136316688)
            self.snapshot()  # Identity validation precedes every fault action.
            save(self.directory / "observer-identity.json", {"at_utc": utc(), "metadata": meta,
                "access": "FILE_MAP_READ; never consume or mutate", "controller_pid": child.pid})
        except BaseException:
            self.close()
            raise

    def snapshot(self):
        c, w = self.coop.read(0, 360), self.world.read(0, 416)
        u32 = lambda raw, at: struct.unpack_from("<I", raw, at)[0]
        u64 = lambda raw, at: struct.unpack_from("<Q", raw, at)[0]
        if (u64(c, 0) != 0x315643414F4F4349 or u32(c, 8) != 4 or u32(c, 12) != 4205096
                or u32(c, 16) != self.meta["game_pid"] or u32(c, 96) != self.meta["controller_pid"] or c[40:72] != self.nonce
                or c[24:40] != self.nonce[:16] or str(u64(c, 88)) != self.meta["game_creation_time_100ns"]
                or u64(w, 0) != 0x31524F5743534949 or u32(w, 8) != 2 or u32(w, 12) != 136316688
                or u32(w, 16) != self.meta["game_pid"] or u32(w, 20) != self.meta["controller_pid"] or w[24:56] != self.nonce):
            raise ValueError("Read-only mapping identity/layout changed")
        return {"at_utc": utc(), "monotonic_ns": str(time.perf_counter_ns()),
            "coop": {name: u32(c, at) for name, at in (("state", 296), ("error", 300), ("stop", 304), ("callbacks", 316), ("pinned", 320), ("restored", 324))}
                    | {name: str(u64(c, at)) for name, at in (("produced", 328), ("consumed", 336), ("tick", 344), ("overwritten", 352))},
            "world": {name: u32(w, at) for name, at in (("enabled", 56), ("error", 60), ("callbacks", 64), ("closed", 68))}
                     | {name: str(u64(w, at)) for name, at in (("produced", 72), ("consumed", 80), ("backpressure", 88), ("attempts", 96), ("captures", 104))}
                     | {"enters": [str(v) for v in struct.unpack_from("<19Q", w, 112)], "returns": [str(v) for v in struct.unpack_from("<19Q", w, 264)],
                        "batch_states": [u32(self.world.read(5767584 + index * 43516368, 4), 0) for index in range(3)]},
            "raw_coop_header": c.hex(), "raw_world_header": w.hex()}

    def pause_reader(self):
        if self.child.poll() is not None:
            raise ValueError("Controller is no longer running")
        api = self.api
        snapshot = api.checked(api.CreateToolhelp32Snapshot(4, 0), "thread snapshot")
        handles = []
        try:
            entry = Thread(); entry.dwSize = C.sizeof(entry)
            found = api.Thread32First(snapshot, C.byref(entry))
            if not found:
                raise OSError(C.get_last_error(), "first thread")
            while found:
                if entry.th32OwnerProcessID == self.child.pid:
                    handle = api.checked(api.OpenThread(0x800 | 2, False, entry.th32ThreadID), "open own controller thread")
                    retained = False
                    try:
                        if api.GetProcessIdOfThread(handle) != self.child.pid:
                            raise ValueError("Thread owner changed")
                        text = C.c_void_p()
                        status = api.GetThreadDescription(handle, C.byref(text))
                        if status < 0:
                            raise OSError(status, "thread description")
                        try:
                            name = C.wstring_at(text) if text.value else ""
                        finally:
                            if text.value:
                                api.LocalFree(text)
                        if name == "world-reader":
                            handles.append((entry.th32ThreadID, handle)); retained = True
                    finally:
                        if not retained:
                            api.CloseHandle(handle)
                found = api.Thread32Next(snapshot, C.byref(entry))
            if C.get_last_error() != 18:
                raise OSError(C.get_last_error(), "thread enumeration did not complete")
            if len(handles) != 1:
                raise ValueError("Expected exactly one owned world-reader thread")
            tid, handle = handles[0]
            before = self.snapshot()
            if before["coop"]["state"] != 2 or before["coop"]["error"] or before["world"]["error"] or not before["world"]["enabled"]:
                raise ValueError("Fault requires a healthy active producer")
            suspended, record = False, {"controller_pid": self.child.pid, "thread_id": tid, "before": before, "duration_ms": 1000}
            try:
                previous = api.SuspendThread(handle)
                if previous == 0xffffffff:
                    raise OSError(C.get_last_error(), "pause world-reader")
                suspended = True
                record["prior_suspend_count"] = previous
                if previous != 0:
                    raise ValueError("Reader already suspended; undo only this invocation's increment")
                time.sleep(1.0)
                record["during"] = self.snapshot()
            finally:
                if suspended:
                    resumed = api.ResumeThread(handle)
                    record["resume_previous_count"] = resumed
                    save(self.directory / "reader-pause-action.json", record)
                    if resumed != 1:
                        raise RuntimeError("Reader resume count uncertain; stop further live work")
            time.sleep(0.5)
            record["after"] = self.snapshot()
            record["observed_backpressure_and_tick_progress"] = (
                int(record["during"]["world"]["backpressure"]) > int(before["world"]["backpressure"])
                and int(record["during"]["coop"]["tick"]) > int(before["coop"]["tick"])
                and record["after"]["coop"]["error"] == 0 and record["after"]["world"]["error"] == 0)
            save(self.directory / "reader-pause-outcome.json", record)
            return record
        finally:
            api.CloseHandle(snapshot)
            for _, handle in handles:
                api.CloseHandle(handle)

    def after_loss(self, baseline, native):
        deadline = time.monotonic() + 12
        observations = []
        while True:
            item = self.snapshot(); observations.append(item)
            append(self.directory / "post-loss-observations.jsonl", item)
            if item["coop"]["restored"] and item["world"]["closed"] or time.monotonic() >= deadline:
                break
            time.sleep(0.1)
        time.sleep(0.5)
        final = self.snapshot(); observations.append(final)
        append(self.directory / "post-loss-observations.jsonl", final)
        recovered = self.game.snapshot("after-controller-loss")
        modules = self.api.modules(self.game.handle)
        save(self.directory / "module-inventory-after-loss.json", {"api": "PSAPI_LIST_MODULES_32BIT", "modules": modules})
        resident = any(Path(item["path"]).resolve() == Path(native).resolve() for item in modules)
        previous = observations[-2]
        stable = all(final["world"][key] == previous["world"][key] for key in ("produced", "attempts", "captures", "enters", "returns"))
        coop, world = final["coop"], final["world"]
        start, end = int(baseline["coop"]["produced"]), int(coop["produced"])
        events = []
        if not start <= end <= start + 32768:
            raise ValueError("Loss event range exceeded the bounded ring")
        for sequence in range(start + 1, end + 1):
            offset = 10600 + ((sequence - 1) % 32768) * 128
            raw = self.coop.read(offset, 128)
            published = struct.unpack_from("<Q", raw)[0]
            if published != sequence or self.coop.read(offset, 8) != raw[:8]:
                raise ValueError("Loss event slot is not stable")
            events.append({"sequence": str(sequence), "kind": struct.unpack_from("<I", raw, 40)[0],
                "action": struct.unpack_from("<I", raw, 48)[0], "query": struct.unpack_from("<I", raw, 52)[0],
                "boolean": struct.unpack_from("<I", raw, 68)[0], "raw_le": raw.hex()})
        save(self.directory / "post-loss-events.json", events)
        pending = []
        if world["closed"] and world["callbacks"] == 0 and stable:
            for index, state in enumerate(world["batch_states"]):
                if state == 0:
                    continue
                base = 5767584 + index * 43516368
                meta = self.world.read(base, 464)
                objects, memberships = struct.unpack_from("<II", meta, 88)
                if objects > 16384 or memberships > 65536:
                    raise ValueError("Unconsumed batch exceeds fixed native budget")
                sections = [("metadata", 0, 464), ("objects", 464, objects * 2592),
                            ("memberships", 42467792, memberships * 16)]
                for name, offset, size in sections:
                    path = self.directory / f"unconsumed-{index}-{name}.bin"
                    with path.open("xb") as output:
                        for begin in range(0, size, 1024 * 1024):
                            output.write(self.world.read(base + offset + begin, min(size - begin, 1024 * 1024)))
                pending.append({"batch": index, "state": state, "objects": objects, "memberships": memberships,
                                "sections": sections, "format": "raw little-endian fixed-layout owned copy"})
        restored_events = [item for item in events if item["kind"] == 15 and item["action"] != 2**32 - 1]
        expected_targets = {item["rva"]: item for item in self.game.targets}
        event_bytes_match = len(restored_events) == 70 and {item["action"] for item in restored_events} == set(expected_targets)
        if event_bytes_match:
            for item in restored_events:
                expected = expected_targets[item["action"]]
                raw = bytes.fromhex(item["raw_le"])
                event_bytes_match &= (item["query"] == expected["width"]
                    and raw[96:96 + item["query"]].hex() == expected["original"]
                    and raw[112:112 + item["query"]].hex() == expected["original"])
        passed = (coop["state"] == 4 and coop["restored"] == 1 and coop["pinned"] == 1 and coop["callbacks"] == 0
            and coop["error"] == 7 and int(coop["overwritten"]) == 0
            and world["enabled"] == 0 and world["closed"] == 1 and world["callbacks"] == 0 and world["error"] == 0
            and world["enters"] == world["returns"] and stable and resident and recovered["restored_count"] == 70
            and len(restored_events) == 70 and len({item["action"] for item in restored_events}) == 70
            and all(item["boolean"] == 1 for item in restored_events) and event_bytes_match)
        outcome = {"status": "PASS" if passed else "FAIL", "observations": observations,
            "native_module_resident": resident, "raw_restored_targets": recovered["restored_count"],
            "publication_counts_stable": stable, "restoration_events": len(restored_events),
            "restoration_event_bytes_match_independent_baseline": event_bytes_match,
            "unconsumed_batches": pending, "controller_file_final_flush": "UNCONFIRMED_CONTROLLER_TERMINATED",
            "expected_coop_error": "Publication=7", "no_shared_values_written": True}
        save(self.directory / "controller-loss-outcome.json", outcome)
        return outcome

    def close(self):
        for mapping in (self.world, self.coop):
            if mapping:
                mapping.close()
