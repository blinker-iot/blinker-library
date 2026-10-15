#!/usr/bin/env python3
"""Finite facade/object and ODR-token checks using an existing real R4 SDK command.

WiFi/task compilation loads the real product and its dependencies. BLE/WiFiBLE API
cases intentionally omit native product bindings; host multi-TU tests supply
their lifecycles, while the ordinary compile matrix owns native BLE linkage.
This gate neither links nor uploads firmware and needs no global product flag.
Use object compilation: this Core's GCC 7 crashes with -fsyntax-only on existing
constexpr PropertySpec initializers, including the old unrepaired facade.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PRODUCT_FLAGS = ('BLINKER_PRODUCT_WIFI', 'BLINKER_PRODUCT_BLE', 'BLINKER_PRODUCT_WIFI_BLE')
RESET = '#include <Arduino.h>\n' + ''.join('#undef ' + name + '\n' for name in PRODUCT_FLAGS) + '''#undef BLINKER_ENABLE_TASKS
#undef BLINKER_INTERNAL_UNO_R4_TASKS
#undef BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL
#undef BLINKER_INTERNAL_LAN_WEBSOCKET
'''
TASKS = '''#define BLINKER_ENABLE_TASKS 1
#define BLINKER_INTERNAL_UNO_R4_TASKS 1
#undef BLINKER_TASK_CAPACITY
#undef BLINKER_TASK_WINDOW_MS
#undef BLINKER_TASK_WRITES
#undef BLINKER_TASK_MIN_START_MS
#undef BLINKER_TASK_MAX_COMMIT_MS
#define BLINKER_TASK_CAPACITY 4
#define BLINKER_TASK_WINDOW_MS 60000U
#define BLINKER_TASK_WRITES 10U
#define BLINKER_TASK_MIN_START_MS 30000U
#define BLINKER_TASK_MAX_COMMIT_MS 1000U
'''
PROPERTY = 'BLINKER_PROPERTY(power, bool, blinker::readWrite());\n'
BLE = '#define BLINKER_V2_ARDUINO_UNO_R4_BLE_PRODUCT_H\n#include <BlinkerBLE.h>\n'
WIFI_BLE = '''#define ARDUINO_ARCH_ESP32 1
#define BLINKER_V2_ARDUINO_ESP32_WIFI_BLE_PRODUCT_H
#include <BlinkerWiFiBLE.h>
'''
WIFI_CALL = 'void setup() { Blinker.begin("key", "ssid", "password", power); }\n'
LOCAL_CALL = 'void setup() { Blinker.begin(power); }\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compile-commands', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    args = parser.parse_args()
    out = args.out_dir.resolve()
    if not out.is_relative_to(ROOT / 'build') or out == ROOT / 'build':
        parser.error('use a dedicated repository build subdirectory')
    out.mkdir(parents=True, exist_ok=True)
    report = out / 'facade-api-compile.json'
    if report.exists():
        parser.error('choose a new evidence directory')
    entries = json.loads(args.compile_commands.read_text(encoding='utf-8'))
    entry = next(e for e in entries if e['file'].endswith('.ino.cpp'))
    if not any('ARDUINO_ARCH_RENESAS' in arg for arg in entry['arguments']):
        parser.error('this finite admission gate requires an existing real R4 SDK command')
    raw = iter(entry['arguments'][1:])
    command = [entry['arguments'][0], '-I' + str(ROOT / 'src')]
    for arg in raw:
        if arg in ('-o', '-MF', '-MT', '-MQ'):
            next(raw)
        elif arg not in ('-MMD', '-MD', '-c', '-fstack-usage', entry['file']) and not any(
                arg.startswith('-D' + name) for name in PRODUCT_FLAGS):
            command.append(arg)
    cases = [
        ('wifi-property-only', '#include <Blinker.h>\n' + PROPERTY, None),
        ('wifi-literals', '#include <Blinker.h>\n' + PROPERTY + WIFI_CALL, None),
        ('wifi-views', '#include <BlinkerWiFi.h>\n' + PROPERTY +
         'void setup() { blinker::StringView key("key"), ssid("ssid"), password("password"); '
         'Blinker.begin(key, ssid, password, power); }\n', None),
        ('wifi-arrays', '#include <BlinkerWiFi.h>\n' + PROPERTY +
         'void setup() { char key[]="key", ssid[]="ssid", password[]="password"; '
         'Blinker.begin(key, ssid, password, power); }\n', None),
        ('wifi-reject-local', '#include <BlinkerWiFi.h>\n' + PROPERTY + LOCAL_CALL, 'no matching function'),
        ('wifi-reject-empty', '#include <BlinkerWiFi.h>\n' +
         'void setup() { Blinker.begin("key", "ssid", "password"); }\n', 'no matching function'),
        ('wifi-tasks-disabled', '#include <BlinkerWiFi.h>\n' + PROPERTY +
         'void setup() { Blinker.enableTasks(power); }\n', None),
        ('wifi-tasks-admitted-r4', TASKS + '#include <BlinkerWiFi.h>\n' + PROPERTY +
         'void setup() { Blinker.enableTasks(power); Blinker.begin("key", "ssid", "password", power); }\n', None),
        ('wifi-tasks-unadmitted-r4', TASKS.replace('#define BLINKER_INTERNAL_UNO_R4_TASKS 1\n', '') +
         '#include <BlinkerWiFi.h>\n', 'admitted manual WiFi platform'),
        ('ble-entry', BLE + PROPERTY + LOCAL_CALL, None),
        ('ble-reject-credentials', BLE + PROPERTY + WIFI_CALL, 'no matching function'),
        ('ble-reject-tasks', TASKS + BLE, 'admitted manual WiFi platform'),
        ('wifi-ble-entry', WIFI_BLE + PROPERTY + LOCAL_CALL, None),
        ('wifi-ble-reject-credentials', WIFI_BLE + PROPERTY + WIFI_CALL, 'no matching function'),
        ('wifi-ble-reject-tasks', TASKS + WIFI_BLE, 'admitted manual WiFi platform'),
        ('selectors-reject-mixed', '#include <BlinkerWiFi.h>\n#include <BlinkerBLE.h>\n', 'cannot be combined'),
        ('print-arduino-first', '#include <BlinkerV2/api/BlinkerFacade.h>\n'
         '#include <type_traits>\nstatic_assert(std::is_same<Print, arduino::Print>::value, "Canonical SDK Print");\n'
         'auto debugMember = static_cast<void (blinker::BlinkerClass::*)(arduino::Print&, blinker::LogLevel)>(&blinker::BlinkerClass::debug);\n', None),
        ('print-facade-first', '#include <BlinkerV2/api/BlinkerFacade.h>\n#include <Arduino.h>\n'
         '#include <type_traits>\nstatic_assert(std::is_same<Print, arduino::Print>::value, "Canonical SDK Print");\n'
         'auto debugMember = static_cast<void (blinker::BlinkerClass::*)(arduino::Print&, blinker::LogLevel)>(&blinker::BlinkerClass::debug);\n', None),
    ]
    rows = []
    for name, source, error in cases:
        file = out / (name + '.cpp')
        prefix = RESET.replace('#include <Arduino.h>\n', '') if name == 'print-facade-first' else RESET
        file.write_text(prefix + source, encoding='utf-8')
        result = subprocess.run(command + ['-c', '-o', str(out / (name + '.o')), str(file)], cwd=entry['directory'],
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                encoding='utf-8', errors='replace', timeout=120)
        (out / (name + '.log')).write_text(result.stdout, encoding='utf-8')
        if (error is None and result.returncode != 0) or (
                error is not None and (result.returncode == 0 or error not in result.stdout)):
            raise RuntimeError(f'{name} failed: {result.stdout[-2500:]}')
        rows.append({'name': name, 'accepted': error is None, 'expectedDiagnostic': error})
        print(name + ': passed', flush=True)
    definitions = []
    for mode in ('core', 'WIFI', 'BLE', 'WIFI_BLE'):
        file = out / ('definition-' + mode.lower() + '.cpp')
        selected = '' if mode == 'core' else '#define BLINKER_PRODUCT_' + mode + ' 1\n'
        file.write_text(RESET + selected + '#include <BlinkerV2/api/BlinkerFacade.h>\n', encoding='utf-8')
        result = subprocess.run(command + ['-E', '-P', str(file)], cwd=entry['directory'],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                encoding='utf-8', errors='replace', timeout=120)
        if result.returncode != 0:
            raise RuntimeError('definition preprocessing failed: ' + result.stderr[-2500:])
        body = result.stdout.split('class BlinkerClass {', 1)[1].split('\n};', 1)[0]
        definitions.append({'mode': mode, 'classTokensSha256': hashlib.sha256(
            ' '.join(body.split()).encode('utf-8')).hexdigest()})
    if len({row['classTokensSha256'] for row in definitions}) != 1:
        raise RuntimeError('BlinkerClass differs between Sketch/product and ordinary library TUs')
    inputs = [ROOT / 'src' / name for name in
              ('BlinkerWiFi.h', 'BlinkerBLE.h', 'BlinkerWiFiBLE.h', 'BlinkerV2/api/BlinkerFacade.h')]
    report.write_text(json.dumps({
        'scope': 'real R4 SDK object compilation/12 B ABI; BLE native bindings mocked; no firmware link or hardware',
        'buildWideProductFlag': False, 'cases': rows, 'definitions': definitions,
        'inputs': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                   for path in inputs}, 'passed': True,
    }, indent=2), encoding='utf-8')
    print('Facade: identical class tokens across core and three product TUs passed', flush=True)


if __name__ == '__main__':
    main()
