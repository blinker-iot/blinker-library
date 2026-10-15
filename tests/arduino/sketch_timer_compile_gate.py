#!/usr/bin/env python3
"""Compile the current timer header/probe with an already installed real SDK.

Uses a previous Arduino compiler invocation for SDK include/board flags, but
puts the current repository's src first. This is syntax/ABI admission only,
not an installed ZIP, linked firmware or physical timing test.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compile-commands', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    args = parser.parse_args()
    out = args.out_dir.resolve()
    if not out.is_relative_to(ROOT / 'build') or out == ROOT / 'build':
        parser.error('use a dedicated repository build subdirectory')
    if (out / 'timer-compile.json').exists():
        parser.error('choose a new evidence directory')
    entries = json.loads(args.compile_commands.read_text(encoding='utf-8'))
    entry = next(e for e in entries if e['file'].endswith('.ino.cpp'))
    raw = iter(entry['arguments'][1:])
    command = [entry['arguments'][0], '-I' + str(ROOT / 'src')]
    for arg in raw:
        if arg in ('-o', '-MF', '-MT', '-MQ'):
            next(raw)
        elif arg not in ('-MMD', '-MD', '-c', entry['file']):
            command.append(arg)
    probe = ROOT / 'tests/arduino/SketchTimerProbe/SketchTimerProbe.ino'
    command += ['-fsyntax-only', '-x', 'c++', str(probe)]
    out.mkdir(parents=True, exist_ok=True)
    result = subprocess.run(command, cwd=entry['directory'], stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, encoding='utf-8', errors='replace', timeout=120)
    (out / 'timer-compile.log').write_text(result.stdout, encoding='utf-8')
    core_test = ROOT / 'tests/host/test_sketch_timer_main.cpp'
    core = subprocess.run([entry['arguments'][0], '-std=c++11', '-Wall', '-Wextra',
                           '-Wpedantic', '-Werror', '-I' + str(ROOT / 'src'),
                           '-fsyntax-only', str(core_test)], stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, encoding='utf-8', errors='replace', timeout=120)
    (out / 'core-cxx11.log').write_text(core.stdout, encoding='utf-8')
    report = {'scope': 'current header, SDK syntax and 32-bit ABI only',
              'compiler': entry['arguments'][0], 'probe': str(probe),
              'timerBytes': {'1': 36, '4': 108, '8': 204}, 'handleBytes': 12, 'facadeBytes': 12,
              'inputs': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                         for p in (probe, core_test, ROOT / 'src/BlinkerTimer.h',
                                   ROOT / 'src/BlinkerV2/arduino/ArduinoSketchTimer.h',
                                   ROOT / 'src/BlinkerV2/core/SketchTimer.h',
                                   ROOT / 'src/BlinkerV2/api/BlinkerFacade.h')},
              'coreCxx11Passed': core.returncode == 0,
              'passed': result.returncode == 0 and core.returncode == 0}
    (out / 'timer-compile.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    if result.returncode != 0:
        raise RuntimeError(result.stdout[-4000:])
    if core.returncode != 0:
        raise RuntimeError(core.stdout[-4000:])
    print('Sketch timer: real SDK syntax, 32-bit ABI and strict C++11 core passed')


if __name__ == '__main__':
    main()
