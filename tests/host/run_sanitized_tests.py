#!/usr/bin/env python3
"""Build and run the Device V2 host gates with platform sanitizers."""

from __future__ import annotations

import argparse
import glob
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def find_cmake(explicit: str | None) -> Path:
    if explicit:
        candidate = Path(explicit)
        if candidate.is_file():
            return candidate
        raise SystemExit(f"cmake executable not found: {candidate}")

    discovered = shutil.which("cmake")
    if discovered:
        return Path(discovered)

    if os.name == "nt":
        program_files_x86 = os.environ.get("ProgramFiles(x86)", "")
        pattern = str(
            Path(program_files_x86)
            / "Microsoft Visual Studio"
            / "2022"
            / "*"
            / "Common7"
            / "IDE"
            / "CommonExtensions"
            / "Microsoft"
            / "CMake"
            / "CMake"
            / "bin"
            / "cmake.exe"
        )
        matches = sorted(glob.glob(pattern))
        if matches:
            return Path(matches[-1])

    raise SystemExit("cmake was not found; pass --cmake with an absolute path")


def add_msvc_asan_runtime(environment: dict[str, str]) -> None:
    if os.name != "nt":
        return
    program_files_x86 = os.environ.get("ProgramFiles(x86)", "")
    pattern = str(
        Path(program_files_x86)
        / "Microsoft Visual Studio"
        / "2022"
        / "*"
        / "VC"
        / "Tools"
        / "MSVC"
        / "*"
        / "bin"
        / "Hostx64"
        / "x64"
        / "clang_rt.asan_dynamic-x86_64.dll"
    )
    matches = sorted(glob.glob(pattern))
    if matches:
        runtime_directory = str(Path(matches[-1]).parent)
        environment["PATH"] = runtime_directory + os.pathsep + environment["PATH"]


def run(command: list[str], environment: dict[str, str]) -> None:
    print("+", subprocess.list2cmdline(command), flush=True)
    subprocess.run(command, check=True, env=environment)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cmake", help="absolute cmake executable path")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument(
        "--mode",
        choices=("auto", "asan", "asan-ubsan"),
        default="auto",
    )
    parser.add_argument("--iterations", type=int, default=200_000)
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.iterations <= 0:
        raise SystemExit("--iterations must be positive")

    mode = arguments.mode
    if mode == "auto":
        mode = "asan" if os.name == "nt" else "asan-ubsan"
    if os.name == "nt" and mode == "asan-ubsan":
        raise SystemExit("MSVC has no UBSan; use Linux GCC/Clang for asan-ubsan")

    source_directory = Path(__file__).resolve().parent
    build_directory = (
        arguments.build_dir.resolve()
        if arguments.build_dir
        else Path(tempfile.mkdtemp(prefix=f"blinker-host-{mode}-"))
    )
    cmake = find_cmake(arguments.cmake)
    ctest = cmake.with_name("ctest.exe" if os.name == "nt" else "ctest")
    if not ctest.is_file():
        discovered_ctest = shutil.which("ctest")
        if not discovered_ctest:
            raise SystemExit(f"ctest was not found beside {cmake}")
        ctest = Path(discovered_ctest)

    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = (
        "halt_on_error=1:detect_leaks=0"
        if os.name == "nt"
        else "halt_on_error=1:detect_leaks=1"
    )
    if mode == "asan-ubsan":
        environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    add_msvc_asan_runtime(environment)

    configure = [
        str(cmake),
        "-S",
        str(source_directory),
        "-B",
        str(build_directory),
        "-DBLINKER_ENABLE_ASAN=ON",
        "-DBLINKER_BUILD_FUZZ_SMOKE=ON",
    ]
    if os.name != "nt":
        configure.append("-DCMAKE_BUILD_TYPE=RelWithDebInfo")
    if mode == "asan-ubsan":
        configure.append("-DBLINKER_ENABLE_UBSAN=ON")
    run(configure, environment)
    run(
        [
            str(cmake),
            "--build",
            str(build_directory),
            "--config",
            "RelWithDebInfo",
            "--parallel",
        ],
        environment,
    )
    run(
        [
            str(ctest),
            "--test-dir",
            str(build_directory),
            "-C",
            "RelWithDebInfo",
            "--output-on-failure",
        ],
        environment,
    )

    executable_suffix = ".exe" if os.name == "nt" else ""
    executable_name = "blinker_device_v2_protocol_fuzz"
    if os.name == "nt":
        fuzz_executable = (
            build_directory / "RelWithDebInfo" / f"{executable_name}.exe"
        )
    else:
        fuzz_executable = build_directory / executable_name
    run(
        [str(fuzz_executable), "--iterations", str(arguments.iterations)],
        environment,
    )
    print(f"sanitized host gates passed; build directory: {build_directory}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
