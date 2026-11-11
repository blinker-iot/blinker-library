"""Finite Wio link-graph regressions; fixtures are not hardware ownership proof."""
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "arduino"))
import verify_installed_release as gate
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "architecture"))
import check_device_v2 as architecture

OWNER = "20001000 00000024 b blinker::wioTerminalQspiOwner()::owner\n"
GUARD = "20001024 00000004 b guard variable for blinker::wioTerminalQspiOwner()::owner\n"
TASK4 = ("00001000 W blinker::WioTerminalTaskSnapshotBlobStore<4u>::load()\n"
         "00001001 T blinker::TaskEngine::poll()\n"
         "00001002 T blinker::TaskSnapshotStore::load()\n"
         "00001003 W blinker::TaskCloudTransport<blinker::ManagedMqttTransport>::poll()\n"
         "00001004 T Ed25519::verify(unsigned char const*)\n")
LOCK4 = ("00001010 T blinker_wio_newlib_lock_anchor\n"
         "00001011 T __malloc_lock\n"
         "00001012 T __malloc_unlock\n")


class WioTaskAdmission(unittest.TestCase):
    def check(self, symbols=OWNER + TASK4 + LOCK4, capacity=4, reserved=True):
        gate.verify_wio_qspi_closure(symbols, task_capacity=capacity, region_reserved=reserved)

    def test_default_and_explicit_task4_graphs(self):
        self.check(OWNER + GUARD, 0, False)
        self.check(OWNER + GUARD + "00001004 T Ed25519::verify(unsigned char const*)\n", 0, False)
        self.check(OWNER + GUARD + TASK4 + LOCK4)

    def test_default_rejects_each_task_only_graph_marker(self):
        for line in TASK4.splitlines(keepends=True)[:-1]:
            with self.subTest(line=line), self.assertRaisesRegex(SystemExit, "default link"):
                self.check(OWNER + line, 0, False)

    def test_missing_duplicate_or_function_only_owner_rejected(self):
        for owner in ("", GUARD, OWNER * 2, "00001000 T blinker::wioTerminalQspiOwner()::owner\n"):
            with self.subTest(owner=owner), self.assertRaisesRegex(SystemExit, "exactly one"):
                self.check(owner + TASK4 + LOCK4)

    def test_capacity_and_reservation_fail_closed(self):
        for capacity in (True, -1, 1, 3, 5, 10, "4"):
            with self.subTest(capacity=capacity), self.assertRaisesRegex(SystemExit, "capacity"):
                self.check(capacity=capacity)
        for reserved in (False, 1, "yes", None):
            with self.subTest(reserved=reserved), self.assertRaisesRegex(SystemExit, "attestation"):
                self.check(reserved=reserved)

    def test_internal_product_keeps_exact_guards_and_plain_manual_scope(self):
        product = (architecture.PRODUCT_WIO_TERMINAL / "WioTerminalWiFiProduct.h").read_text(encoding="utf-8")
        self.assertEqual(architecture.wio_task_product_failures(product), [])
        for token in ("BLINKER_INTERNAL_WIO_TASKS != 1", "BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED != 1",
                      "BLINKER_TASK_CAPACITY != 4", "BLINKER_PRODUCT_BLE", "BLINKER_PRODUCT_WIFI_BLE",
                      "BLINKER_INTERNAL_LOCAL_ACCESS_CONTROL", "BLINKER_INTERNAL_LAN_WEBSOCKET",
                      "BLINKER_OFFICIAL_CLOUD_SESSION_TLS", "BLINKER_OFFICIAL_MQTT_TLS",
                      "BLINKER_RESOURCE_PROFILE == BLINKER_RESOURCE_PROFILE_SMALL",
                      "BLINKER_DEVICE_FRAME_SIZE == 256", "kDeviceKeyMqttPacketBufferSize == 361U",
                      "MQTT_MAX_PACKET_SIZE == kDeviceKeyMqttPacketBufferSize"):
            self.assertIn(token, product)
            changed = product.replace(token, "REMOVED_GUARD")
            with self.subTest(token=token):
                self.assertTrue(architecture.wio_task_product_failures(changed))
                self.assertTrue(architecture.wio_task_product_failures(changed + "\n// " + token))

    def test_allocator_wiring_retains_internal_scope_cold_and_irq_guards(self):
        port = architecture.PORT_ROOT / "wio_terminal"
        sources = [(architecture.IMPLEMENTATION_ROOT / "BlinkerWioTerminal.cpp").read_text(encoding="utf-8"),
                   (port / "WioTerminalNewlibLock.h").read_text(encoding="utf-8"),
                   (port / "WioTerminalNewlibLock.inc").read_text(encoding="utf-8"),
                   (architecture.PRODUCT_WIO_TERMINAL / "WioTerminalWiFiTaskExtension.h").read_text(encoding="utf-8")]
        self.assertEqual(architecture.wio_allocator_wiring_failures(*sources), [])
        for index, token in (
            (0, "WioTerminalNewlibLock.inc"), (1, 'extern "C" void blinker_wio_newlib_lock_anchor();'),
            (2, "BLINKER_ENABLE_TASKS != 1"), (2, "BLINKER_INTERNAL_WIO_TASKS != 1"),
            (2, "BLINKER_TASK_CAPACITY != 4"), (2, "BLINKER_INTERNAL_WIO_TASK_REGION_RESERVED != 1"),
            (2, "INCLUDE_xTaskGetSchedulerState != 1 && configUSE_TIMERS != 1"),
            (2, "if (__get_IPSR() != 0U) wioAllocatorContextFault();"),
            (2, "if (state == taskSCHEDULER_NOT_STARTED) return false;"),
            (2, "if (wioAllocatorSchedulerStarted()) vTaskSuspendAll();"),
            (2, "if (wioAllocatorSchedulerStarted()) (void)xTaskResumeAll();"),
            (3, "blinker_wio_newlib_lock_anchor();"),
        ):
            self.assertIn(token, sources[index])
            changed = list(sources); changed[index] = changed[index].replace(token, "REMOVED_GUARD")
            with self.subTest(index=index, token=token):
                self.assertTrue(architecture.wio_allocator_wiring_failures(*changed))

    def test_default_does_not_substitute_for_task4_validation(self):
        with self.assertRaisesRegex(SystemExit, "default link"):
            self.check(capacity=0, reserved=False)
        for line in TASK4.splitlines(keepends=True):
            with self.subTest(line=line), self.assertRaisesRegex(SystemExit, "original store"):
                self.check(OWNER + TASK4.replace(line, "") + LOCK4)
        for task in (TASK4.replace("<4u>", "<3u>"), TASK4 + TASK4.replace("<4u>", "<10ul>")):
            with self.subTest(task=task), self.assertRaisesRegex(SystemExit, "original store"):
                self.check(OWNER + task + LOCK4)

    def test_known_flash_drivers_rejected_without_banning_fs_or_sd(self):
        for symbol in ("sfud_read", "SFUD::begin", "Adafruit_SPIFlash::begin", "Arduino_QSPI::begin",
                       "QSPIFlash::read", "Seeed_FS::beginFlash", "Seeed_FS::formatFlash"):
            with self.subTest(symbol=symbol), self.assertRaisesRegex(SystemExit, "conflicting"):
                self.check(OWNER + TASK4 + LOCK4 + "00001005 T " + symbol)
        self.check(OWNER + TASK4 + LOCK4 + "00001005 T fs::FS::open\n00001006 T SDClass::begin\n")

    def test_default_rejects_only_defined_exact_internal_anchor(self):
        for line in ("00001010 T blinker_wio_newlib_lock_anchor\n",
                     "00001010 00000004 T blinker_wio_newlib_lock_anchor\n",
                     "00001010 D blinker_wio_newlib_lock_anchor\n"):
            with self.subTest(line=line), self.assertRaisesRegex(SystemExit, "internal allocator anchor"):
                self.check(OWNER + line, 0, False)
        for line in ("         U blinker_wio_newlib_lock_anchor\n",
                     "00001010 U blinker_wio_newlib_lock_anchor\n",
                     "00001010 T blinker_wio_newlib_lock_anchor_extra\n",
                     "00001010 T prefix_blinker_wio_newlib_lock_anchor\n",
                     "00001010 T blinker_wio_newlib_lock_anchor()\n",
                     "00001011 T __malloc_lock\n00001012 T __malloc_unlock\n",
                     "00001011 W __malloc_lock\n00001012 W __malloc_unlock\n"):
            with self.subTest(line=line):
                self.check(OWNER + line, 0, False)

    def test_task4_requires_unique_strong_defined_allocator_closure(self):
        for line in LOCK4.splitlines(keepends=True):
            for lock in (LOCK4.replace(line, ""), LOCK4 + line,
                         LOCK4.replace(line, line.replace(" T ", " U ")),
                         LOCK4.replace(line, line.replace(" T ", " W ")),
                         LOCK4.replace(line, line.replace(" T ", " D ")),
                         LOCK4.replace(line, line.rstrip() + "_extra\n")):
                with self.subTest(line=line, lock=lock), self.assertRaisesRegex(SystemExit, "strong allocator"):
                    self.check(OWNER + TASK4 + lock)
        self.check(OWNER + TASK4 + LOCK4.replace(" T ", " 00000004 T "))
        self.check(OWNER + TASK4 + LOCK4 + "         U blinker_wio_newlib_lock_anchor\n"
                   "00001014 T blinker_wio_newlib_lock_anchor_extra\n")

    def test_regular_wio_link_gate_uses_the_same_check(self):
        target = next(t for t in gate.TARGETS if t.name == "wio-terminal-wifi")
        with tempfile.TemporaryDirectory() as folder:
            link_map = Path(folder) / "fixture.map"
            link_map.write_text("BlinkerDeviceV2.a(BlinkerApi.cpp.o)\n"
                                "BlinkerDeviceV2.a(BlinkerTransport.cpp.o)\n")
            symbols = "00001000 T blinker::MqttFrameTransport::poll()\n"
            with self.assertRaisesRegex(SystemExit, "exactly one"):
                gate.verify_link_closure(target, link_map, symbols)
            gate.verify_link_closure(target, link_map, symbols + OWNER + GUARD)
            ble_target = gate.Target("wio-terminal-ble", target.fqbn, "BleQuickStart", frozenset())
            gate.verify_link_closure(ble_target, link_map,
                                    "00001000 T blinker::BleFrameTransport::poll()\n" + OWNER
                                    + "00001004 T Ed25519::verify(unsigned char const*)\n")


if __name__ == "__main__":
    unittest.main()
