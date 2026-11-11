#!/usr/bin/env python3
"""Export/verify/rebuild a tested development ESP32 WiFi maintainer source bundle.

Not another Arduino product ZIP. Reuses the release builder; never copies build,
Git, hardware fixtures, credentials, SDK caches or unrelated tests. The separately
installed SDK/CLI are prerequisites, not claimed to be bundled or fully pinned.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
import build_arduino_release as release
from arduino_release_profile import TASK_PILOT_ID, apply_profile, load_profile


EXTRAS = (
    'tools/build_arduino_release.py', 'tools/arduino_release_profile.py',
    'tools/build_arduino_handoff.py',
    'tests/arduino/verify_installed_release.py',
    'tests/arduino/CompileTaskWiFi/CompileTaskWiFi.ino',
    'tests/release/test_arduino_handoff.py', 'tests/release/development_profile.json',
    'docs/DEVICE_V2_TASK_PILOT_HANDOFF.md',
)
DEPENDENCY_FILES = ('library.properties', 'library.json', 'keywords.txt', 'LICENSE.txt', 'README.md')
REQUIREMENTS = {
    'scope': 'Development ESP32 manual WiFi candidate; optional explicit task pilot, not the full release matrix',
    'python': '3.11+ (validated with 3.13)',
    'arduino_cli': '1.5.1',
    'arduino_core': 'esp32:esp32@3.3.11',
    'fqbn': 'esp32:esp32:XIAO_ESP32S3:PartitionScheme=default_8MB',
    'libraries': {'PubSubClient': '2.8'},
    'bundled': ['library source', 'release/profile tools', 'installed gate', 'public task consumer', 'PubSubClient source/license'],
    'external': ['Python', 'Arduino CLI', 'ESP32 SDK and its compiler/tools'],
    'runtime': 'Matching App/Gateway/Broker, schema and feature admission are NOT deployed by this bundle',
}


def digest(path: Path) -> str:
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def relative_name(value: str) -> Path:
    if not isinstance(value, str) or not value or '\\' in value or ':' in value:
        raise ValueError('unsafe manifest path')
    if any(part in ('', '.', '..') for part in value.split('/')) or PurePosixPath(value).is_absolute():
        raise ValueError('unsafe manifest path')
    return Path(*value.split('/'))


def inventory(root: Path) -> dict[str, str]:
    result = {}
    if root.is_symlink():
        raise ValueError('symlink is not a portable input')
    for path in sorted(root.rglob('*')):
        if path.is_symlink():
            raise ValueError('symlink is not a portable input')
        if path.is_file():
            result[path.relative_to(root).as_posix()] = digest(path)
    return result


def copy_input(source: Path, destination: Path, boundary: Path) -> None:
    if source.is_symlink() or not source.is_file() or not source.resolve().is_relative_to(boundary.resolve()):
        raise ValueError('missing or non-portable handoff input')
    if any(p.is_symlink() for p in source.parents if p.is_relative_to(boundary)):
        raise ValueError('symlink is not a portable input')
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, destination)


def require_development(profile: Path) -> None:
    parsed = load_profile(profile)
    if parsed.production:
        raise ValueError('handoff admits only development profiles; no production admission')


def export(profile: Path, reference: Path, dependency: Path, destination: Path) -> dict:
    require_development(profile)
    properties, files = release.validate()
    if destination.exists():
        raise ValueError('destination must not exist')
    dependency = dependency.resolve()
    metadata = (dependency / 'library.properties').read_text(encoding='utf-8').splitlines()
    values = dict(line.split('=', 1) for line in metadata if '=' in line and not line.startswith('#'))
    if values.get('name') != 'PubSubClient' or values.get('version') != '2.8':
        raise ValueError('PubSubClient 2.8 source required')
    # Compare before exporting; a previous tested ZIP is evidence, not a name.
    with tempfile.TemporaryDirectory(prefix='blinker-handoff-', dir=destination.parent) as temp:
        stage = Path(temp) / 'stage'
        release.stage(stage, files)
        apply_profile(stage, load_profile(profile))
        candidate = release.archive(stage, Path(temp) / 'artifacts', properties)
        if candidate.read_bytes() != reference.read_bytes():
            raise ValueError('current source/profile differs from reference ZIP')
    destination.mkdir()
    for source in files:
        copy_input(source, destination / 'source' / source.relative_to(release.REPOSITORY), release.REPOSITORY)
    for name in EXTRAS:
        copy_input(release.REPOSITORY / name, destination / 'source' / name, release.REPOSITORY)
    dependency_inputs = [dependency / n for n in DEPENDENCY_FILES]
    if not (dependency / 'src').is_dir():
        raise ValueError('PubSubClient source missing')
    for path in (dependency / 'src').rglob('*'):
        if path.is_symlink():
            raise ValueError('symlink is not a portable input')
        if path.is_file():
            if path.suffix not in ('.h', '.hpp', '.cpp', '.c'):
                raise ValueError('unexpected dependency source artifact')
            dependency_inputs.append(path)
    for path in dependency_inputs:
        copy_input(path, destination / 'dependencies/PubSubClient' / path.relative_to(dependency), dependency)
    shutil.copyfile(profile, destination / 'profile.json')
    (destination / 'requirements.json').write_text(json.dumps(REQUIREMENTS, indent=2) + '\n', encoding='utf-8')
    manifest = {'schema': 1, 'kind': 'blinker-wifi-source',
                'expected_zip': {'name': reference.name, 'sha256': digest(reference)},
                'files': inventory(destination)}
    (destination / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    verify(destination)
    return manifest


def verify(bundle: Path) -> dict:
    manifest = json.loads((bundle / 'manifest.json').read_text(encoding='utf-8'))
    if (set(manifest) != {'schema', 'kind', 'expected_zip', 'files'} or manifest['schema'] != 1
            or manifest['kind'] not in ('blinker-task-pilot-source', 'blinker-wifi-source')):
        raise ValueError('unsupported handoff manifest')
    expected = manifest['expected_zip']
    if set(expected) != {'name', 'sha256'} or len(relative_name(expected['name']).parts) != 1:
        raise ValueError('unsafe expected ZIP')
    hashes = manifest['files']
    if not isinstance(hashes, dict) or not hashes or 'manifest.json' in hashes:
        raise ValueError('invalid handoff inventory')
    for name in hashes:
        relative_name(name)
    for value in (*hashes.values(), expected['sha256']):
        if not isinstance(value, str) or len(value) != 64 or any(c not in '0123456789abcdef' for c in value):
            raise ValueError('invalid sha256')
    actual = inventory(bundle)
    del actual['manifest.json']
    if actual != hashes:
        raise ValueError('handoff bytes missing, modified or unexpected')
    require_development(bundle / 'profile.json')
    if manifest['kind'] == 'blinker-task-pilot-source' and load_profile(bundle / 'profile.json').task_profile != TASK_PILOT_ID:
        raise ValueError('old task handoff requires its explicit task profile')
    return manifest


def rebuild(bundle: Path, work: Path) -> Path:
    manifest = verify(bundle)
    if work.exists() or work.resolve().is_relative_to(bundle.resolve()):
        raise ValueError('work must be a new directory outside the verified bundle')
    work.mkdir(parents=True)
    env = {**os.environ, 'PYTHONDONTWRITEBYTECODE': '1'}
    env.pop('PYTHONPATH', None)
    command = [sys.executable, str(bundle / 'source/tools/build_arduino_release.py'),
               '--profile', str(bundle / 'profile.json'), '--allow-development-profile',
               '--stage-dir', str(work / 'stage'), '--output-dir', str(work / 'artifacts')]
    result = subprocess.run(command, cwd=bundle / 'source', env=env, capture_output=True, text=True, timeout=120)
    (work / 'rebuild.log').write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        raise ValueError('isolated release builder failed; inspect rebuild.log')
    archive = work / 'artifacts' / manifest['expected_zip']['name']
    if digest(archive) != manifest['expected_zip']['sha256']:
        raise ValueError('rebuilt ZIP differs from tested reference')
    verify(bundle)  # Also catch a builder that altered its own inputs.
    return archive


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='mode', required=True)
    create = commands.add_parser('export')
    for name in ('profile', 'reference-zip', 'dependency-dir', 'out'):
        create.add_argument('--' + name, type=Path, required=True)
    check = commands.add_parser('verify')
    check.add_argument('--bundle', type=Path, required=True)
    build = commands.add_parser('rebuild')
    build.add_argument('--bundle', type=Path, required=True)
    build.add_argument('--work-dir', type=Path, required=True)
    args = parser.parse_args()
    if args.mode == 'export':
        manifest = export(args.profile.resolve(), args.reference_zip.resolve(), args.dependency_dir.resolve(), args.out.resolve())
        print(f"Source handoff exported: {len(manifest['files'])} files; no SDK or local hardware fixtures copied")
    elif args.mode == 'verify':
        verify(args.bundle.resolve())
        print('Source handoff bytes verified (integrity, not publisher authenticity)')
    else:
        print('Tested ZIP rebuilt identically: ' + str(rebuild(args.bundle.resolve(), args.work_dir.resolve())))


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        raise SystemExit(f'error: {error}') from error
