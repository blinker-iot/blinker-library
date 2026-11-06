"""Original selector source counterexamples; no SDK, build, device or service."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_device_v2 as architecture

ROOT = Path(__file__).resolve().parents[2]
FACADE = 'BlinkerV2/api/BlinkerFacade.h'
CONFIG = 'BlinkerV2/arduino/config/OfficialConfig.h'
CASES = (
    ('BlinkerWiFi.h', 'BLINKER_PRODUCT_WIFI', (
        'BlinkerV2/arduino/platform/esp32/Esp32WiFiProduct.h',
        'BlinkerV2/arduino/platform/uno_r4/UnoR4WiFiProduct.h',
        'BlinkerV2/arduino/platform/wio_terminal/WioTerminalWiFiProduct.h')),
    ('BlinkerBLE.h', 'BLINKER_PRODUCT_BLE', (
        'BlinkerV2/arduino/platform/esp32/Esp32BLEProduct.h',
        'BlinkerV2/arduino/platform/uno_r4/UnoR4BLEProduct.h',
        'BlinkerV2/arduino/platform/wio_terminal/WioTerminalBLEProduct.h')),
    ('BlinkerWiFiBLE.h', 'BLINKER_PRODUCT_WIFI_BLE', (
        'BlinkerV2/arduino/platform/esp32/Esp32WiFiBLEProduct.h',)),
)


class ProductSelectorContract(unittest.TestCase):
    def source(self, case):
        return (ROOT / 'src' / case[0]).read_text(encoding='utf8')

    def check(self, case, text):
        return architecture.product_selector_valid(text, case[1], case[2])

    def test_actual_three_selectors_keep_their_specific_prefixes(self):
        for case in CASES:
            with self.subTest(selector=case[0]):
                self.assertTrue(self.check(case, self.source(case)))

    def test_wifi_ble_requires_profile_before_facade(self):
        case = CASES[2]
        source = self.source(case)
        config = f'#include "{CONFIG}"\n'
        for name, text in (
            ('missing', source.replace(config, '')),
            ('late', source.replace(config, '').replace(
                f'#include "{FACADE}"', f'#include "{FACADE}"\n{config}')),
        ):
            with self.subTest(change=name):
                self.assertFalse(self.check(case, text))

    def test_ordinary_selectors_do_not_gain_early_profile_or_dependencies(self):
        for case in CASES[:2]:
            for extra in (CONFIG, 'unreviewed-early-dependency.h'):
                text = self.source(case).replace(
                    f'#include "{FACADE}"', f'#include "{extra}"\n#include "{FACADE}"')
                with self.subTest(selector=case[0], include=extra):
                    self.assertFalse(self.check(case, text))

    def test_wifi_ble_rejects_other_includes_before_or_inside_the_prefix(self):
        case = CASES[2]
        for before in (CONFIG, FACADE):
            text = self.source(case).replace(
                f'#include "{before}"',
                f'#include "unreviewed-early-dependency.h"\n#include "{before}"')
            with self.subTest(before=before):
                self.assertFalse(self.check(case, text))

    def test_selection_must_be_defined_before_the_first_consumer(self):
        for case in CASES:
            define = f'#define {case[1]} 1\n'
            text = self.source(case).replace(define, '').replace(
                f'#include "{FACADE}"', f'#include "{FACADE}"\n{define}')
            with self.subTest(selector=case[0]):
                self.assertFalse(self.check(case, text))

    def test_all_other_product_selectors_must_remain_conflicting(self):
        for case in CASES:
            for other in {entry[1] for entry in CASES} - {case[1]}:
                text = self.source(case).replace(f'defined({other})', '0')
                with self.subTest(selector=case[0], removed=other):
                    self.assertFalse(self.check(case, text))

    def test_platform_bindings_cannot_disappear_or_precede_the_facade(self):
        for case in CASES:
            product = f'#include "{case[2][0]}"'
            source = self.source(case)
            for name, text in (
                ('missing', source.replace(product, '')),
                ('early', source.replace(product, '').replace(
                    f'#include "{FACADE}"', product + f'\n#include "{FACADE}"')),
            ):
                with self.subTest(selector=case[0], change=name):
                    self.assertFalse(self.check(case, text))

    def test_empty_and_unknown_selectors_are_not_admitted(self):
        self.assertFalse(self.check(CASES[2], ''))
        self.assertFalse(architecture.product_selector_valid(
            self.source(CASES[2]), 'BLINKER_PRODUCT_OTHER', CASES[2][2]))


if __name__ == '__main__':
    unittest.main()
