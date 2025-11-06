#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Configure and build Serial Studio with Qt toolchains."
    )
    parser.add_argument(
        "--platform",
        choices=["mac", "windows", "linux"],
        required=True,
        help="Target platform for the build."
    )
    parser.add_argument(
        "--build-type",
        choices=["Release", "Debug"],
        default="Release",
        help="CMake build type (default: Release)."
    )
    parser.add_argument(
        "--toolchain",
        choices=["msvc", "mingw"],
        help="Windows toolchain: MSVC or MinGW."
    )
    parser.add_argument(
        "--build-dir",
        help="Optional build directory (defaults to build-<platform>-<toolchain>-<type>)."
    )
    parser.add_argument(
        "--generator",
        help="Override CMake generator (e.g. Ninja, MinGW Makefiles)."
    )
    parser.add_argument(
        "--jobs",
        type=int,
        help="Maximum parallel build jobs."
    )
    parser.add_argument(
        "--configure-only",
        action="store_true",
        help="Run configuration step only."
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print commands without executing them."
    )
    parser.add_argument(
        "extra_args",
        nargs=argparse.REMAINDER,
        help="Additional arguments passed to qt-cmake after a '--'."
    )
    return parser.parse_args()


def determine_build_dir(args: argparse.Namespace) -> Path:
    if args.build_dir:
        return Path(args.build_dir)
    parts = ["build", args.platform]
    if args.platform == "windows":
        parts.append(args.toolchain or "msvc")
    parts.append(args.build_type.lower())
    return Path("-".join(parts))


def pick_generator(args: argparse.Namespace) -> str:
    if args.generator:
        return args.generator

    if shutil.which("ninja"):
        return "Ninja"

    if args.platform == "windows":
        return "MinGW Makefiles" if args.toolchain == "mingw" else "NMake Makefiles"

    return "Unix Makefiles"


def is_multi_config(generator: str) -> bool:
    multi_keywords = ("Multi-Config", "Visual Studio", "Xcode")
    return any(keyword in generator for keyword in multi_keywords)


def ensure_tool(name: str) -> None:
    if shutil.which(name):
        return
    sys.stderr.write(f"error: '{name}' not found in PATH.\n")
    sys.exit(1)


def run_command(cmd: list[str], dry_run: bool) -> None:
    print(" ".join(cmd))
    if dry_run:
        return
    result = subprocess.run(cmd, check=False)
    if result.returncode != 0:
        sys.exit(result.returncode)


def main() -> None:
    args = parse_args()
    if args.platform != "windows" and args.toolchain:
        sys.stderr.write("error: --toolchain only applies to windows builds.\n")
        sys.exit(1)

    if args.platform == "windows" and not args.toolchain:
        args.toolchain = "msvc"

    ensure_tool("qt-cmake")
    ensure_tool("cmake")

    build_dir = determine_build_dir(args)
    build_dir.parent.mkdir(parents=True, exist_ok=True)

    generator = pick_generator(args)
    multi_config = is_multi_config(generator)
    cmake_args = []
    if args.extra_args:
        if args.extra_args[0] == "--":
            cmake_args = args.extra_args[1:]
        else:
            cmake_args = args.extra_args

    configure_cmd = [
        "qt-cmake",
        "-S",
        ".",
        "-B",
        str(build_dir),
        f"-DCMAKE_BUILD_TYPE={args.build_type}"
    ]

    configure_cmd += ["-G", generator]

    run_command(configure_cmd + cmake_args, args.dry_run)
    if args.configure-only:
        return

    build_cmd = [
        "cmake",
        "--build",
        str(build_dir)
    ]

    if multi_config:
        build_cmd += ["--config", args.build_type]

    jobs = args.jobs or os.cpu_count()
    if jobs and jobs > 1:
        build_cmd += ["--parallel", str(jobs)]

    run_command(build_cmd, args.dry_run)


if __name__ == "__main__":
    main()
