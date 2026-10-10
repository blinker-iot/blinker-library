import unittest
from task_nvs_gate import fixture_event, validate


class TaskNvsDriverTests(unittest.TestCase):
    def test_filters_unrecognized_or_extra_data(self):
        for line in (b'password=secret', b'{"event":"other"}', b'[]',
                     b'{"event":"write","bytes":1,"us":2,"code":0,"secret":"x"}',
                     b'{"event":"done","command":"erase","code":0}', b'x' * 1025):
            self.assertIsNone(fixture_event(line))
        self.assertEqual(fixture_event(b'{"event":"write","bytes":76,"us":10,"code":0}')["bytes"], 76)

    def test_requires_matching_completion_and_no_error(self):
        with self.assertRaises(ValueError):
            validate("run", [], None, None, None)
        with self.assertRaises(ValueError):
            validate("run", [{"event":"done", "command":"run", "code":22}], None, None, None)
        validate("run", [{"event":"done", "command":"run", "code":0}], None, None, None)

    def test_cut_is_not_a_completed_execution(self):
        cut = [{"event":"cut", "phase":"claimed", "callbacks":0}]
        validate("claim", cut, None, None, None)
        with self.assertRaises(ValueError):
            validate("run", cut, None, None, None)

    def test_recovery_expectations(self):
        events = [{"event":"boot", "code":0, "ready":1, "resetReason":3},
                  {"event":"status", "label":"boot", "count":1, "callbacks":0},
                  {"event":"record", "index":0, "code":0, "progress":2}]
        validate("reset", events, 1, 2, 0)
        with self.assertRaises(ValueError):
            validate("reset", events, 1, 3, 0)
        with self.assertRaises(ValueError):
            validate("reset", events[1:], 1, 2, 0)
        with self.assertRaises(ValueError):
            validate("clear", events, 1, 2, 0)
        with self.assertRaises(ValueError):
            validate("reset", events[:-1], 1, None, 0)


if __name__ == "__main__":
    unittest.main()
