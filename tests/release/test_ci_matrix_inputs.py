"""Check workflow compile inputs without installing SDKs or invoking Arduino CLI."""
from pathlib import Path
import re
import shlex
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/arduino'))
import compile_matrix as matrix


class CiMatrixInputs(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.workflow = (ROOT / '.github/workflows/device-v2-ci.yml').read_text(encoding='utf-8')

    def job(self, name):
        body = self.workflow.split(f'\n  {name}:\n', 1)[1]
        return re.split(r'^  [a-z0-9-]+:', body, maxsplit=1, flags=re.MULTILINE)[0]

    def step(self, job, name):
        body = job.split(f'      - name: {name}\n', 1)[1]
        return re.split(r'^      - ', body, maxsplit=1, flags=re.MULTILINE)[0]

    def folded_command(self, step):
        lines = step.split('        run: >-\n', 1)[1].splitlines()
        lines = [line for line in lines if line.strip()]
        self.assertTrue(lines)
        # A more-indented YAML line preserves a newline instead of folding to
        # a space. The shell would then run it as a separate command.
        self.assertEqual([len(line) - len(line.lstrip()) for line in lines],
                         [10] * len(lines), 'folded shell command has mixed indentation')
        return ' '.join(line[10:] for line in lines)

    def compile_targets(self, command):
        words = shlex.split(command)
        self.assertEqual(words[:2], ['python3', 'tests/arduino/compile_matrix.py'])
        arguments = words[2:]
        self.assertEqual(len(arguments) % 2, 0)
        self.assertTrue(arguments)
        targets = []
        for flag, name in zip(arguments[::2], arguments[1::2]):
            if flag == '--group':
                self.assertIn(name, matrix.GROUPS)
                selected = matrix.GROUPS[name]
            else:
                self.assertEqual(flag, '--target')
                selected = (name,)
            for target in selected:
                self.assertTrue(target in matrix.TARGETS, f'unknown CI compile target: {target}')
                targets.append(target)
        return targets

    def test_core_matrix_keeps_every_available_core_and_crypto_target(self):
        job = self.job('arduino-core')
        rows = re.findall(r'^          - target: ([a-z0-9-]+)$', job, flags=re.MULTILINE)
        self.assertCountEqual(rows, matrix.CORE_TARGET_NAMES)
        command = self.folded_command(self.step(job, 'Compile portable Core/BBP2 and crypto fallback'))
        selected = []
        for target in rows:
            with self.subTest(target=target):
                selected.extend(self.compile_targets(command.replace('${{ matrix.target }}', target)))
        self.assertCountEqual(selected, [*matrix.CORE_TARGET_NAMES, *matrix.GROUPS['portable-crypto']])

    def test_cloud_matrix_and_direct_group_inputs_resolve_to_current_targets(self):
        job = self.job('arduino-cloud')
        rows = re.findall(r'^            targets: (.+)$', job, flags=re.MULTILINE)
        self.assertTrue(rows)
        step = self.step(job, 'Compile platform storage, BLE security, HTTP and MQTT Ports')
        command = step.split('        run: ', 1)[1].strip()
        for arguments in rows:
            with self.subTest(arguments=arguments):
                self.compile_targets(command.replace('${{ matrix.targets }}', arguments))
        commands = re.findall(r'^        run: (python3 tests/arduino/compile_matrix\.py .+)$',
                              self.job('arduino-unified'), flags=re.MULTILINE)
        self.assertTrue(commands)
        for command in commands:
            with self.subTest(command=command):
                self.compile_targets(command)

    def test_wio_pinned_core_url_is_an_argument_in_one_folded_shell_command(self):
        command = self.folded_command(self.step(self.job('arduino-unified'),
                                                'Install pinned Wio Terminal core'))
        self.assertEqual(shlex.split(command), [
            'arduino-cli', 'core', 'install', 'Seeeduino:samd@1.8.5',
            '--additional-urls',
            'https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json',
        ])

    def test_host_golden_passes_every_cpp_executable_from_the_sanitized_build(self):
        script = (ROOT / 'tests/golden/run_cross_language_golden.ps1').read_text(encoding='utf-8')
        parameters = script.split('param(\n', 1)[1].split('\n)', 1)[0]
        names = re.findall(r'\[string\]\$([A-Za-z]*CppExecutable)\s*=', parameters)
        self.assertTrue(names)
        resolutions = re.findall(
            r'^\$([A-Za-z]*CppExecutable) = Resolve-GoldenExecutable `\n    \$\1 "([^"]+)"$',
            script, flags=re.MULTILINE)
        self.assertCountEqual([name for name, _ in resolutions], names)
        step = self.step(self.job('host-quality'), 'Cross-language exact golden vectors')
        arguments = re.findall(r'^            -([A-Za-z]*CppExecutable) (.+)$',
                               step, flags=re.MULTILINE)
        self.assertCountEqual([name for name, _ in arguments], names)
        self.assertCountEqual(arguments, [
            (name, f'"$PWD/build/sanitized/{executable}" `')
            for name, executable in resolutions
        ])


if __name__ == '__main__':
    unittest.main()
