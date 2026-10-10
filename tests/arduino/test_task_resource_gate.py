"""No compiler or device required: fail-closed parsing of resource evidence."""
import unittest

from task_resource_gate import PRODUCT_METHODS, REQUIRED_METHODS, check_symbols, resource_summary


class ResourceEvidenceTests(unittest.TestCase):
    def test_summary_requires_both_measurements(self):
        self.assertEqual(resource_summary("Sketch uses 100 bytes\nGlobal variables use 20 bytes"),
                         {"flash_bytes": 100, "static_ram_bytes": 20})
        for log in ("", "Sketch uses 100 bytes", "Global variables use 20 bytes"):
            with self.assertRaises(ValueError):
                resource_summary(log)

    def test_disabled_rejects_any_task_graph(self):
        self.assertEqual(check_symbols("00100000 00000010 T blinker::Client::poll()", 0), 0)
        for name in REQUIRED_METHODS:
            with self.assertRaises(ValueError):
                check_symbols(name, 0)
        with self.assertRaises(ValueError):
            check_symbols("blinker::TaskCloudTransport<blinker::ManagedMqttTransport>::poll(unsigned long)", 0)

    def test_enabled_requires_all_code_and_actual_module_storage(self):
        symbols = "\n".join(REQUIRED_METHODS) + "\n3fc90000 000005e8 b tasks\n"
        self.assertEqual(check_symbols(symbols, 4), 1512)
        for name in REQUIRED_METHODS:
            with self.assertRaises(ValueError):
                check_symbols(symbols.replace(name, "missing"), 4)
        with self.assertRaises(ValueError):
            check_symbols("\n".join(REQUIRED_METHODS), 4)
        with self.assertRaises(ValueError):
            check_symbols(symbols + "3fc91000 000005e8 b tasks\n", 4)

    def test_product_requires_complete_carrier_and_unique_storage(self):
        instance = "3fc90000 00001000 b blinker::integration::official_detail::esp32WifiProduct()::instance\n"
        symbols = "\n".join(PRODUCT_METHODS) + "\n" + instance
        self.assertEqual(check_symbols(symbols, 4, True), 4096)
        self.assertEqual(check_symbols(instance, 0, True), 4096)
        self.assertEqual(check_symbols(instance.replace(" b ", " V "), 0, True), 4096)
        for name in PRODUCT_METHODS:
            with self.assertRaises(ValueError):
                check_symbols(symbols.replace(name, "missing"), 4, True)
        for invalid in (symbols + instance, "\n".join(PRODUCT_METHODS)):
            with self.assertRaises(ValueError):
                check_symbols(invalid, 4, True)
        with self.assertRaises(ValueError):
            check_symbols(symbols, 0, True)


if __name__ == "__main__":
    unittest.main()
