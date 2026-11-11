#!/usr/bin/env python3
"""Finite task API admission with an existing real SDK compiler invocation; no upload."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
POLICY = """#define BLINKER_TASK_CAPACITY 4
#define BLINKER_TASK_WINDOW_MS 60000U
#define BLINKER_TASK_WRITES 10U
#define BLINKER_TASK_MIN_START_MS 30000U
#define BLINKER_TASK_MAX_COMMIT_MS 100U
"""
ENABLE = "#define BLINKER_ENABLE_TASKS 1\n"
WIFI = '#include <Blinker.h>\n'
PROPERTY = 'BLINKER_PROPERTY(power, bool, blinker::readWrite());\n'
CALL = 'void configure() { Blinker.enableTasks(power); }\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compile-commands', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    args = parser.parse_args()
    out = args.out_dir.resolve()
    if not out.is_relative_to(ROOT / 'build') or out == ROOT / 'build':
        parser.error('use a dedicated repository build subdirectory')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'api-compile.json'
    if report.exists():
        parser.error('choose a new evidence directory')
    entries = json.loads(args.compile_commands.read_text(encoding='utf-8'))
    entry = next(e for e in entries if e['file'].endswith('.ino.cpp'))
    raw = iter(entry['arguments'][1:])
    command = [entry['arguments'][0], '-I' + str(ROOT / 'src')]
    for arg in raw:
        if arg == '-o':
            next(raw)
        elif arg not in ('-MMD', '-c', entry['file']):
            command.append(arg)
    command += ['-fsyntax-only']
    cases = [
        ('enabled', ENABLE + POLICY + WIFI + PROPERTY + CALL, None),
        ('disabled', WIFI + PROPERTY + CALL, None),
        ('profile-missing', ENABLE + WIFI + PROPERTY + CALL, 'maintainer-admitted BLINKER_TASK_CAPACITY'),
        ('policy-missing', ENABLE + '#define BLINKER_TASK_CAPACITY 4\n' + WIFI, 'explicit maintainer storage policy'),
        ('policy-invalid', ENABLE + POLICY.replace('WRITES 10U', 'WRITES 1U') + WIFI, 'policy values are out of range'),
        ('duplicate-targets', ENABLE + POLICY + WIFI + PROPERTY + CALL.replace('(power)', '(power, power)'), None), # runtime check
        ('empty', ENABLE + POLICY + WIFI + CALL.replace('(power)', '()'), 'no matching function'),
        ('action', ENABLE + POLICY + WIFI + 'BLINKER_ACTION(power);\n' + CALL, 'no matching function'),
        ('over-capacity', ENABLE + POLICY + WIFI + PROPERTY + CALL.replace('(power)', '(' + ','.join(['power'] * 33) + ')'), 'Select 1..32 task Properties'),
    ]
    for mode in ('BLE', 'WIFI_BLE'):
        cases.append((mode.lower(), ENABLE + f'#define BLINKER_PRODUCT_{mode} 1\n#include <BlinkerV2/api/BlinkerFacade.h>\n', 'admitted manual WiFi platform'))
    for arch in ('ARDUINO_ARCH_RENESAS_UNO', 'SEEED_WIO_TERMINAL'):
        cases.append((arch.lower(), ENABLE + f'#undef ARDUINO_ARCH_ESP32\n#define {arch} 1\n#define BLINKER_PRODUCT_WIFI 1\n#include <BlinkerV2/api/BlinkerFacade.h>\n', 'admitted manual WiFi platform'))
    for flag, error in [('BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL', 'Task pilot admits manual WiFi only'),
                        ('BLINKER_INTERNAL_LAN_WEBSOCKET', 'requires the shared LocalAccess cloud authority')]:
        cases.append((flag.lower(), ENABLE + POLICY + f'#define {flag} 1\n' + WIFI, error))
    rows = []
    for name, source, error in cases:
        file = out / (name + '.cpp')
        file.write_text(source, encoding='utf-8')
        result = subprocess.run(command + [str(file)], cwd=entry['directory'], stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, encoding='utf-8', errors='replace', timeout=120)
        (out / (name + '.log')).write_text(result.stdout, encoding='utf-8')
        if (error is None and result.returncode != 0) or (error is not None and (result.returncode == 0 or error not in result.stdout)):
            raise RuntimeError(f'{name} failed: {result.stdout[-2500:]}')
        rows.append({'name': name, 'accepted': error is None, 'expectedDiagnostic': error})
        print(name + ': passed', flush=True)
    report.write_text(json.dumps({'scope': 'syntax only; not linked firmware or hardware', 'cases': rows}, indent=2), encoding='utf-8')


if __name__ == '__main__':
    main()
