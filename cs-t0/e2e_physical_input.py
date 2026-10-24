"""Owned physical keyboard input for the single T0-08 E2E invocation.

This module only finds the unique prepared game window and injects Win32
scan-code keyboard events. It never writes Native input state, Player fields,
canonical queue state, or game memory.
"""
import ctypes
import time
from ctypes import wintypes as w
from datetime import datetime, timezone


USER32 = ctypes.WinDLL('user32', use_last_error=True)

GA_ROOT = 2
SW_RESTORE = 9
INPUT_KEYBOARD = 1
KEYEVENTF_EXTENDEDKEY = 0x0001
KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_SCANCODE = 0x0008
FOCUS_POLL_COUNT = 51
FOCUS_POLL_INTERVAL = 0.010
ULONG_PTR = ctypes.c_size_t

KEY_SCAN_CODES = {
    'W': (0x11, False),
    'A': (0x1E, False),
    'S': (0x1F, False),
    'D': (0x20, False),
}


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [
        ('dx', w.LONG), ('dy', w.LONG), ('mouseData', w.DWORD),
        ('dwFlags', w.DWORD), ('time', w.DWORD), ('dwExtraInfo', ULONG_PTR),
    ]


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [
        ('wVk', w.WORD), ('wScan', w.WORD), ('dwFlags', w.DWORD),
        ('time', w.DWORD), ('dwExtraInfo', ULONG_PTR),
    ]


class HARDWAREINPUT(ctypes.Structure):
    _fields_ = [('uMsg', w.DWORD), ('wParamL', w.WORD), ('wParamH', w.WORD)]


class INPUTUNION(ctypes.Union):
    _fields_ = [('mi', MOUSEINPUT), ('ki', KEYBDINPUT), ('hi', HARDWAREINPUT)]


class INPUT(ctypes.Structure):
    _anonymous_ = ('value',)
    _fields_ = [('type', w.DWORD), ('value', INPUTUNION)]


WNDENUMPROC = ctypes.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
USER32.EnumWindows.argtypes = [WNDENUMPROC, w.LPARAM]
USER32.EnumWindows.restype = w.BOOL
USER32.IsWindowVisible.argtypes = [w.HWND]
USER32.IsWindowVisible.restype = w.BOOL
USER32.GetAncestor.argtypes = [w.HWND, w.UINT]
USER32.GetAncestor.restype = w.HWND
USER32.GetWindowThreadProcessId.argtypes = [w.HWND, ctypes.POINTER(w.DWORD)]
USER32.GetWindowThreadProcessId.restype = w.DWORD
USER32.GetForegroundWindow.argtypes = []
USER32.GetForegroundWindow.restype = w.HWND
USER32.SetForegroundWindow.argtypes = [w.HWND]
USER32.SetForegroundWindow.restype = w.BOOL
USER32.ShowWindow.argtypes = [w.HWND, ctypes.c_int]
USER32.ShowWindow.restype = w.BOOL
USER32.IsIconic.argtypes = [w.HWND]
USER32.IsIconic.restype = w.BOOL
USER32.SendInput.argtypes = [w.UINT, ctypes.POINTER(INPUT), ctypes.c_int]
USER32.SendInput.restype = w.UINT


def utc_now():
    return datetime.now(timezone.utc).isoformat(timespec='milliseconds').replace('+00:00', 'Z')


def _hwnd_value(hwnd):
    return int(hwnd) if hwnd else 0


def _foreground():
    hwnd = USER32.GetForegroundWindow()
    pid = w.DWORD(0)
    if hwnd:
        USER32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    return _hwnd_value(hwnd), int(pid.value)


def _window_pid(hwnd):
    pid = w.DWORD(0)
    USER32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    return int(pid.value)


def find_game_window(game_pid):
    matches = []

    @WNDENUMPROC
    def callback(hwnd, _lparam):
        if (_window_pid(hwnd) == game_pid and USER32.IsWindowVisible(hwnd) and
                _hwnd_value(USER32.GetAncestor(hwnd, GA_ROOT)) == _hwnd_value(hwnd)):
            matches.append(_hwnd_value(hwnd))
        return 1

    if not USER32.EnumWindows(callback, 0):
        return None, f'EnumWindows failed: {ctypes.get_last_error()}'
    if len(matches) != 1:
        return None, f'expected exactly one visible root window for pid {game_pid}; observed {matches}'
    return matches[0], None


def focus_game(game_pid, hwnd):
    if USER32.IsIconic(hwnd):
        USER32.ShowWindow(hwnd, SW_RESTORE)
    for attempt in range(FOCUS_POLL_COUNT):
        USER32.SetForegroundWindow(hwnd)
        foreground_hwnd, foreground_pid = _foreground()
        if foreground_hwnd == hwnd and foreground_pid == game_pid:
            return dict(focused=True, foreground_hwnd=foreground_hwnd,
                        foreground_pid=foreground_pid, attempts=attempt + 1)
        if attempt + 1 < FOCUS_POLL_COUNT:
            time.sleep(FOCUS_POLL_INTERVAL)
    foreground_hwnd, foreground_pid = _foreground()
    return dict(focused=False, foreground_hwnd=foreground_hwnd,
                foreground_pid=foreground_pid, attempts=FOCUS_POLL_COUNT,
                error='SetForegroundWindow did not foreground the game within 500ms')


def _send_events(events):
    if not events:
        return True, None
    array = (INPUT * len(events))()
    for index, event in enumerate(events):
        array[index].type = INPUT_KEYBOARD
        array[index].ki = KEYBDINPUT(0, event['scan'], event['flags'], 0, 0)
    written = int(USER32.SendInput(len(events), ctypes.cast(array, ctypes.POINTER(INPUT)),
                                   ctypes.sizeof(INPUT)))
    if written != len(events):
        return False, f'SendInput wrote {written} of {len(events)} events; error {ctypes.get_last_error()}'
    return True, None


def _send_key(scan, down, extended=False):
    flags = KEYEVENTF_SCANCODE | (KEYEVENTF_EXTENDEDKEY if extended else 0)
    if not down:
        flags |= KEYEVENTF_KEYUP
    return _send_events([dict(scan=scan, flags=flags)])


def _release_pressed(pressed, foreground_pid):
    released = []
    for scan, extended in reversed(pressed):
        _send_key(scan, False, extended)
        released.append(dict(scan=scan, extended=extended))
    focused = (lambda: _foreground()[1] == foreground_pid)
    return dict(released=released, foreground_ok=bool(focused()))


def run_sequence(game_pid, creation_time_100ns, steps, hwnd=None):
    """Run one bounded physical sequence and return a JSON-serializable record."""
    started_utc = utc_now()
    started_monotonic = time.monotonic()
    result = dict(
        is_error=True, error=None, tool='owned_win32_sendinput',
        pid=game_pid, creation_time_100ns=str(creation_time_100ns),
        hwnd=None, steps_executed=0, estimated_duration_ms=0,
        started_utc=started_utc, finished_utc=None,
        foreground_samples=[], step_traces=[], released_on_failure=[],
    )
    pressed = []
    try:
        if not isinstance(steps, list) or not 1 <= len(steps) <= 64:
            raise ValueError('physical input requires 1..64 steps')
        result['estimated_duration_ms'] = sum(
            int(step.get('hold_ms', 0)) + int(step.get('delay_after_ms', 0)) for step in steps)
        if hwnd is None:
            hwnd, error = find_game_window(game_pid)
            if error:
                raise RuntimeError(error)
        result['hwnd'] = _hwnd_value(hwnd)
        focus = focus_game(game_pid, hwnd)
        result['foreground_samples'].append(dict(phase='initial', **focus))
        if not focus['focused']:
            raise RuntimeError(focus.get('error', 'game is not foreground'))
        for step_index, step in enumerate(steps):
            keys = step.get('keys')
            hold_ms = int(step.get('hold_ms', 0))
            delay_after_ms = int(step.get('delay_after_ms', 0))
            if not isinstance(keys, list) or not 1 <= len(keys) <= 4 or len(set(keys)) != len(keys):
                raise ValueError(f'step {step_index} has invalid keys')
            if not 10 <= hold_ms <= 5000 or not 0 <= delay_after_ms <= 5000:
                raise ValueError(f'step {step_index} has invalid hold/delay')
            scans = []
            for key in keys:
                normalized = str(key).upper()
                if normalized not in KEY_SCAN_CODES:
                    raise ValueError(f'unsupported physical key {key}')
                scans.append(KEY_SCAN_CODES[normalized])
            foreground_hwnd, foreground_pid = _foreground()
            result['foreground_samples'].append(dict(phase='before-step', step=step_index,
                foreground_hwnd=foreground_hwnd, foreground_pid=foreground_pid))
            if foreground_hwnd != _hwnd_value(hwnd) or foreground_pid != game_pid:
                raise RuntimeError(f'focus lost before step {step_index}')
            for scan, extended in scans:
                ok, error = _send_key(scan, True, extended)
                if not ok:
                    raise RuntimeError(error)
                pressed.append((scan, extended))
            time.sleep(hold_ms / 1000.0)
            foreground_hwnd, foreground_pid = _foreground()
            result['foreground_samples'].append(dict(phase='before-release', step=step_index,
                foreground_hwnd=foreground_hwnd, foreground_pid=foreground_pid))
            release_ok = True
            while pressed:
                scan, extended = pressed.pop()
                ok, error = _send_key(scan, False, extended)
                release_ok = release_ok and ok
                if not ok:
                    raise RuntimeError(error)
            foreground_hwnd, foreground_pid = _foreground()
            result['foreground_samples'].append(dict(phase='after-release', step=step_index,
                foreground_hwnd=foreground_hwnd, foreground_pid=foreground_pid))
            result['step_traces'].append(dict(index=step_index, keys=keys, hold_ms=hold_ms,
                delay_after_ms=delay_after_ms, released=release_ok,
                foreground_ok=(foreground_hwnd == _hwnd_value(hwnd) and foreground_pid == game_pid)))
            if not result['step_traces'][-1]['foreground_ok']:
                raise RuntimeError(f'focus lost after step {step_index}')
            result['steps_executed'] = step_index + 1
            if delay_after_ms:
                time.sleep(delay_after_ms / 1000.0)
        result['is_error'] = False
    except Exception as error:
        result['error'] = str(error)
        if pressed:
            try:
                result['released_on_failure'].append(_release_pressed(pressed, game_pid))
            except Exception as release_error:
                result['released_on_failure'].append(dict(error=str(release_error)))
    finally:
        result['finished_utc'] = utc_now()
        result['actual_duration_ms'] = int((time.monotonic() - started_monotonic) * 1000)
        result['foreground_end_hwnd'], result['foreground_end_pid'] = _foreground()
    return result
