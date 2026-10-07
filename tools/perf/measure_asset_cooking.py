"""Serial full cooks with process CPU time, sampled memory peaks and byte-for-byte comparison (Windows)."""
from pathlib import Path
import argparse
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]


class Counters(ctypes.Structure):
    _fields_ = [('cb', wintypes.DWORD), ('faults', wintypes.DWORD)] + [(name, ctypes.c_size_t) for name in
        ('peak_working_set', 'working_set', 'quota_peak_paged', 'quota_paged', 'quota_peak_nonpaged', 'quota_nonpaged',
         'pagefile', 'peak_pagefile', 'private')]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--before-cooker', type=Path, required=True)
    parser.add_argument('--runs', type=int, default=5)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    query = ctypes.windll.psapi.GetProcessMemoryInfo
    query.argtypes = [wintypes.HANDLE, ctypes.POINTER(Counters), wintypes.DWORD]
    process_times = ctypes.windll.kernel32.GetProcessTimes
    process_times.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
    executables = {'before': args.before_cooker.resolve(), 'after': ROOT/'build/windows-vs/Release/gloom_asset_cooker.exe'}
    env = os.environ.copy()
    env['PATH'] = str(ROOT/'build/windows-vs/Release') + os.pathsep + env['PATH']
    cases = [('small', ROOT/'assets', 'game:/tests/material_surface/scene.gltf'),
             ('factory', ROOT/'assets', 'game:/legacy/factory.gltf'),
             ('hound', ROOT/'.cache/hound-production-v17/source', 'game:/h07-v17/hound.gltf')]
    rows, expected_hashes = [], {}
    for trial in range(args.runs):
        for case, source, path in cases:
            for label, exe in executables.items():
                directory = output/label/case
                directory.mkdir(parents=True, exist_ok=True)
                started = time.perf_counter()
                with (output/f'{trial}-{case}-{label}.log').open('w', encoding='utf-8') as log:
                    # Direct handle belongs to the cooker; a wrapper would measure a different process.
                    process = subprocess.Popen([str(exe), str(source), str(directory), path, 'cache:/scene.gasset'],
                                               cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
                    working = private = 0
                    while process.poll() is None:
                        counters = Counters()
                        counters.cb = ctypes.sizeof(counters)
                        if query(wintypes.HANDLE(int(process._handle)), ctypes.byref(counters), counters.cb):
                            working = max(working, counters.peak_working_set)
                            private = max(private, counters.private)
                        time.sleep(.002)
                seconds = time.perf_counter() - started
                if process.returncode: raise RuntimeError((trial, case, label, process.returncode))
                created, exited, kernel, user = (wintypes.FILETIME() for _ in range(4))
                if not process_times(wintypes.HANDLE(int(process._handle)), ctypes.byref(created), ctypes.byref(exited),
                                     ctypes.byref(kernel), ctypes.byref(user)):
                    raise ctypes.WinError()
                cpu = sum((stamp.dwHighDateTime << 32) | stamp.dwLowDateTime for stamp in (kernel, user)) / 10_000_000
                hashes = {p.relative_to(directory).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                          for p in sorted(directory.rglob('*.gasset'))}
                if case in expected_hashes: assert hashes == expected_hashes[case], (trial, case, label)
                else: expected_hashes[case] = hashes
                rows.append(dict(trial=trial, case=case, label=label, seconds=seconds, process_cpu_seconds=cpu,
                                 peak_working_set=working, sampled_peak_private=private, outputs=len(hashes)))
                document = dict(executables={name: hashlib.sha256(binary.read_bytes()).hexdigest()
                                            for name, binary in executables.items()}, hashes=expected_hashes, runs=rows)
                (output/'measurements.json').write_text(json.dumps(document, indent=2)+'\n', encoding='utf-8')
                print(trial, case, label, f'wall={seconds:.3f}s CPU={cpu:.3f}s WS={working/1048576:.2f}MiB', flush=True)


if __name__ == '__main__':
    main()
