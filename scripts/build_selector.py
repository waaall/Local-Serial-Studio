#!/usr/bin/env python3
"""
High-level build orchestrator for Serial Studio.

This module consolidates the earlier ad-hoc build scripts into a layered,
configurable CLI tool. All external parameters (toolchains, generators, Qt
paths, additional CMake flags, environment overrides) are injected through CLI
options, JSON config files, or environment variables—no repository-specific
paths are hard-coded anymore.
"""
from __future__ import annotations

import argparse
import json
import logging
import os
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence

APP_NAME = "Serial-Studio-GPL3"


class BuildError(RuntimeError):
    """Raised when an underlying build step fails."""


@dataclass
class BuildOptions:
    platform: str
    build_type: str = "Release"
    toolchain: Optional[str] = None
    generator: Optional[str] = None
    build_dir: Path = Path("build")
    jobs: Optional[int] = None
    configure_only: bool = False
    dry_run: bool = False
    production: bool = False
    sanitizer: bool = False
    gpl_only: bool = True
    clean: bool = False
    run_after_build: bool = False
    create_package: bool = False
    info_only: bool = False
    verbose: bool = False
    qt_cmake_binary: Optional[str] = None
    cmake_binary: Optional[str] = None
    qt_root: Optional[Path] = None
    qt_tools_root: Optional[Path] = None
    extra_cmake_args: List[str] = field(default_factory=list)
    env_overrides: Dict[str, str] = field(default_factory=dict)


def detect_platform() -> str:
    system = sys.platform
    if system.startswith("darwin"):
        return "mac"
    if system.startswith("win"):
        return "windows"
    if system.startswith("linux"):
        return "linux"
    raise BuildError(f"Unsupported host platform: {system}")


def configure_logging(verbose: bool) -> logging.Logger:
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(level=level, format="[%(levelname)s] %(message)s")
    return logging.getLogger("serial-studio-build")


class CommandRunner:
    def __init__(self, logger: logging.Logger, dry_run: bool, env: Dict[str, str]):
        self._logger = logger
        self._dry_run = dry_run
        self._base_env = env

    def run(self, args: Sequence[str], cwd: Path, extra_env: Optional[Dict[str, str]] = None) -> None:
        command_str = " ".join(args)
        self._logger.debug("Executing: %s", command_str)
        if self._dry_run:
            return

        env = {**self._base_env, **(extra_env or {})}
        try:
            subprocess.run(args, cwd=cwd, env=env, check=True)
        except subprocess.CalledProcessError as exc:
            raise BuildError(f"Command failed ({exc.returncode}): {command_str}") from exc


class ToolchainStrategy:
    def configure_args(self, options: BuildOptions) -> List[str]:
        raise NotImplementedError

    def configure_env(self, options: BuildOptions) -> Dict[str, str]:
        return {}

    def build_args(self, options: BuildOptions) -> List[str]:
        return []


def generator_available(name: str) -> bool:
    if name == "Ninja":
        return shutil.which("ninja") is not None
    if name == "MinGW Makefiles":
        return shutil.which("mingw32-make") is not None
    if name == "NMake Makefiles":
        return shutil.which("nmake") is not None
    if name.startswith("Visual Studio"):
        return shutil.which("cmake") is not None
    return True


def pick_generator(preferred: Iterable[str]) -> Optional[str]:
    for candidate in preferred:
        if generator_available(candidate):
            return candidate
    return None


class GenericToolchain(ToolchainStrategy):
    def configure_args(self, options: BuildOptions) -> List[str]:
        generator = options.generator or pick_generator(["Ninja", "Unix Makefiles"])
        args: List[str] = []
        if generator:
            args.extend(["-G", generator])
        if options.qt_root:
            args.append(f"-DCMAKE_PREFIX_PATH={options.qt_root}")
        return args


class MinGWToolchain(ToolchainStrategy):
    def configure_args(self, options: BuildOptions) -> List[str]:
        generator = options.generator or pick_generator(["Ninja", "MinGW Makefiles"])
        if not generator:
            raise BuildError("No suitable CMake generator found for MinGW (install Ninja or mingw32-make).")
        args = ["-G", generator]
        if options.qt_root:
            args.append(f"-DCMAKE_PREFIX_PATH={options.qt_root}")
        return args

    def configure_env(self, options: BuildOptions) -> Dict[str, str]:
        extra_path: List[str] = []
        if options.qt_root:
            extra_path.append(str(options.qt_root / "bin"))
        if options.qt_tools_root:
            extra_path.append(str(options.qt_tools_root / "bin"))
        if not extra_path:
            return {}
        joined = os.pathsep.join(extra_path + [os.environ.get("PATH", "")])
        return {"PATH": joined}


class MsvcToolchain(ToolchainStrategy):
    def configure_args(self, options: BuildOptions) -> List[str]:
        preferred = ["Ninja", "NMake Makefiles"]
        generator = options.generator or pick_generator(preferred)
        if not generator:
            raise BuildError("No suitable generator found for MSVC (install Ninja or use Developer Command Prompt for nmake).")
        args = ["-G", generator]
        if options.qt_root:
            args.append(f"-DCMAKE_PREFIX_PATH={options.qt_root}")
        return args

    def build_args(self, options: BuildOptions) -> List[str]:
        return ["--config", options.build_type]


def load_config_file(path: Optional[str]) -> Dict[str, str]:
    if not path:
        return {}
    config_path = Path(path).expanduser()
    if not config_path.exists():
        raise BuildError(f"Config file not found: {config_path}")
    with config_path.open("r", encoding="utf-8") as handle:
        try:
            data = json.load(handle)
        except json.JSONDecodeError as exc:
            raise BuildError(f"Invalid JSON config: {config_path}") from exc
    if not isinstance(data, dict):
        raise BuildError("Config file must contain a JSON object.")
    return {str(k): v for k, v in data.items()}


def parse_env_overrides(values: Sequence[str]) -> Dict[str, str]:
    overrides: Dict[str, str] = {}
    for item in values:
        if "=" not in item:
            raise BuildError(f"Environment override must be KEY=VALUE, got '{item}'.")
        key, value = item.split("=", 1)
        overrides[key] = value
    return overrides


def resolve_command(preferred: Optional[str], fallback: str) -> str:
    if preferred:
        return preferred
    found = shutil.which(fallback)
    if not found:
        raise BuildError(f"Required command '{fallback}' not found in PATH; specify it explicitly with CLI options.")
    return found


def determine_build_directory(options: BuildOptions, project_root: Path) -> Path:
    if options.build_dir.is_absolute():
        return options.build_dir
    segments: List[str] = ["build", options.platform]
    if options.toolchain:
        segments.append(options.toolchain)
    segments.append(options.build_type.lower())
    return project_root / Path(*segments)


def select_toolchain(options: BuildOptions) -> ToolchainStrategy:
    if options.platform == "windows":
        if not options.toolchain:
            raise BuildError("Windows builds require --toolchain (msvc or mingw).")
        if options.toolchain == "msvc":
            return MsvcToolchain()
        if options.toolchain == "mingw":
            return MinGWToolchain()
        raise BuildError(f"Unsupported Windows toolchain: {options.toolchain}")
    if options.toolchain:
        raise BuildError("--toolchain is only valid for Windows builds.")
    return GenericToolchain()


class Builder:
    def __init__(
        self,
        options: BuildOptions,
        project_root: Path,
        logger: logging.Logger,
        runner: CommandRunner,
        toolchain: ToolchainStrategy,
    ):
        self.options = options
        self.project_root = project_root
        self.logger = logger
        self.runner = runner
        self.toolchain = toolchain
        self.build_dir = determine_build_directory(options, project_root)

    def info(self) -> None:
        self.logger.info("Build configuration:")
        self.logger.info("  platform      : %s", self.options.platform)
        self.logger.info("  build type    : %s", self.options.build_type)
        if self.options.toolchain:
            self.logger.info("  toolchain     : %s", self.options.toolchain)
        self.logger.info("  build dir     : %s", self.build_dir)
        if self.options.generator:
            self.logger.info("  generator     : %s", self.options.generator)
        if self.options.jobs:
            self.logger.info("  parallel jobs : %s", self.options.jobs)
        if self.options.qt_root:
            self.logger.info("  Qt root       : %s", self.options.qt_root)
        if self.options.qt_tools_root:
            self.logger.info("  Qt tools root : %s", self.options.qt_tools_root)
        if self.options.extra_cmake_args:
            self.logger.info("  extra args    : %s", " ".join(self.options.extra_cmake_args))
        if self.options.env_overrides:
            self.logger.info("  env overrides : %s", self.options.env_overrides)

    def clean(self) -> None:
        if not self.build_dir.exists():
            return
        self.logger.info("Removing build directory: %s", self.build_dir)
        shutil.rmtree(self.build_dir)

    def configure(self) -> None:
        self.logger.info("Configuring project...")
        self.build_dir.mkdir(parents=True, exist_ok=True)

        try:
            cmake_bin = resolve_command(self.options.qt_cmake_binary, "qt-cmake")
        except BuildError:
            cmake_bin = resolve_command(self.options.cmake_binary, "cmake")

        configure_cmd = [
            cmake_bin,
            "-S",
            str(self.project_root),
            "-B",
            str(self.build_dir),
            f"-DCMAKE_BUILD_TYPE={self.options.build_type}",
            f"-DBUILD_GPL3={'ON' if self.options.gpl_only else 'OFF'}",
        ]

        if self.options.production:
            configure_cmd.append("-DPRODUCTION_OPTIMIZATION=ON")
        if self.options.sanitizer:
            configure_cmd.append("-DDEBUG_SANITIZER=ON")

        configure_cmd.extend(self.toolchain.configure_args(self.options))
        configure_cmd.extend(self.options.extra_cmake_args)

        env = self.options.env_overrides.copy()
        env.update(self.toolchain.configure_env(self.options))

        self.runner.run(configure_cmd, cwd=self.project_root, extra_env=env)

    def build(self) -> None:
        self.logger.info("Building project...")
        cmake_bin = resolve_command(self.options.cmake_binary, "cmake")
        build_cmd = [
            cmake_bin,
            "--build",
            str(self.build_dir),
        ]

        if self.options.jobs and self.options.jobs > 1:
            build_cmd.extend(["--parallel", str(self.options.jobs)])

        build_cmd.extend(self.toolchain.build_args(self.options))
        self.runner.run(build_cmd, cwd=self.project_root, extra_env=self.options.env_overrides)

    def package(self) -> None:
        self.logger.info("Creating package...")
        cmake_bin = resolve_command(self.options.cmake_binary, "cmake")
        package_cmd = [
            cmake_bin,
            "--build",
            str(self.build_dir),
            "--target",
            "package",
        ]
        package_cmd.extend(self.toolchain.build_args(self.options))
        self.runner.run(package_cmd, cwd=self.project_root, extra_env=self.options.env_overrides)

    def run_app(self) -> None:
        self.logger.info("Launching application...")
        app_path: Path
        if self.options.platform == "mac":
            app_dir = self.build_dir / "app" / f"{APP_NAME}.app"
            if not app_dir.exists():
                raise BuildError(f"Application bundle not found: {app_dir}")
            self.runner.run(["open", str(app_dir)], cwd=self.project_root, extra_env=self.options.env_overrides)
            return

        if self.options.platform == "windows":
            exe_name = f"{APP_NAME}.exe"
            if self.options.toolchain == "msvc":
                app_path = self.build_dir / "app" / self.options.build_type / exe_name
            else:
                app_path = self.build_dir / "app" / exe_name
        else:
            app_path = self.build_dir / "app" / APP_NAME

        if not app_path.exists():
            raise BuildError(f"Executable not found: {app_path}")
        self.runner.run([str(app_path)], cwd=self.project_root, extra_env=self.options.env_overrides)


def parse_cli(argv: Optional[Sequence[str]]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Serial Studio build orchestrator",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--config", help="Path to JSON configuration file.")
    parser.add_argument("--platform", choices=["mac", "windows", "linux"], help="Target platform.")
    parser.add_argument("--build-type", choices=["Release", "Debug"], default="Release", help="CMake build type.")
    parser.add_argument("--toolchain", choices=["msvc", "mingw"], help="Windows toolchain.")
    parser.add_argument("--generator", help="Explicit CMake generator.")
    parser.add_argument("--build-dir", help="Output build directory.")
    parser.add_argument("--jobs", type=int, help="Maximum parallel build jobs.")
    parser.add_argument("--clean", action="store_true", help="Delete the build directory before configuring.")
    parser.add_argument("--configure-only", action="store_true", help="Run configure step without building.")
    parser.add_argument("--run", dest="run_app", action="store_true", help="Launch the application after build.")
    parser.add_argument("--package", action="store_true", help="Invoke the CMake 'package' target after build.")
    parser.add_argument("--info", action="store_true", help="Print configuration summary and exit.")
    parser.add_argument("--dry-run", action="store_true", help="Print commands without executing them.")
    parser.add_argument("--production", action="store_true", help="Enable production optimizations.")
    parser.add_argument("--sanitizer", action="store_true", help="Enable sanitizers (Debug builds).")
    parser.add_argument("--commercial", action="store_true", help="Enable commercial build (disables GPL-only flag).")
    parser.add_argument("--verbose", action="store_true", help="Increase logging verbosity.")
    parser.add_argument("--qt-root", help="Path to the Qt installation used for this build.")
    parser.add_argument("--qt-tools-root", help="Path to auxiliary Qt tools (e.g. MinGW toolchain).")
    parser.add_argument("--qt-cmake", dest="qt_cmake_binary", help="Explicit path to qt-cmake.")
    parser.add_argument("--cmake", dest="cmake_binary", help="Explicit path to cmake.")
    parser.add_argument(
        "--env",
        action="append",
        default=[],
        help="Environment override in KEY=VALUE form (can be repeated).",
    )
    parser.add_argument(
        "--cmake-arg",
        action="append",
        dest="extra_cmake_args",
        default=[],
        help="Additional argument passed to the CMake configure step (can be repeated).",
    )
    parser.add_argument("remainder", nargs=argparse.REMAINDER, help="Additional arguments after '--' go to CMake.")
    return parser.parse_args(argv)


def merge_cli_with_config(cli: argparse.Namespace) -> BuildOptions:
    config_values = load_config_file(cli.config)

    def from_sources(key: str, default=None):
        if getattr(cli, key, None) is not None:
            return getattr(cli, key)
        return config_values.get(key, default)

    platform_value = from_sources("platform", detect_platform())
    build_dir_value = from_sources("build_dir")

    build_dir_path = Path(build_dir_value) if build_dir_value else Path("build")

    extra_args = list(cli.extra_cmake_args or [])
    if cli.remainder:
        extra = list(cli.remainder)
        if extra and extra[0] == "--":
            extra = extra[1:]
        extra_args.extend(extra)

    env_overrides = parse_env_overrides(cli.env or [])
    for key, value in config_values.get("env_overrides", {}).items():
        env_overrides.setdefault(key, value)

    qt_root_value = from_sources("qt_root")
    qt_tools_value = from_sources("qt_tools_root")

    options = BuildOptions(
        platform=platform_value,
        build_type=from_sources("build_type", cli.build_type),
        toolchain=from_sources("toolchain"),
        generator=from_sources("generator"),
        build_dir=build_dir_path,
        jobs=from_sources("jobs"),
        configure_only=from_sources("configure_only", cli.configure_only),
        dry_run=from_sources("dry_run", cli.dry_run),
        production=from_sources("production", cli.production),
        sanitizer=from_sources("sanitizer", cli.sanitizer),
        gpl_only=not from_sources("commercial", cli.commercial),
        clean=from_sources("clean", cli.clean),
        run_after_build=from_sources("run_app", cli.run_app),
        create_package=from_sources("package", cli.package),
        info_only=from_sources("info", cli.info),
        verbose=from_sources("verbose", cli.verbose),
        qt_cmake_binary=from_sources("qt_cmake_binary"),
        cmake_binary=from_sources("cmake_binary"),
        qt_root=Path(qt_root_value).expanduser() if qt_root_value else None,
        qt_tools_root=Path(qt_tools_value).expanduser() if qt_tools_value else None,
        extra_cmake_args=extra_args,
        env_overrides=env_overrides,
    )

    if options.jobs is None:
        options.jobs = os.cpu_count() or 1

    if options.build_type not in {"Release", "Debug"}:
        options.build_type = options.build_type.capitalize()

    return options


def validate_options(options: BuildOptions) -> None:
    if options.jobs is not None and options.jobs < 1:
        raise BuildError("--jobs must be a positive integer.")
    if options.platform != "windows" and options.toolchain:
        raise BuildError("--toolchain is only supported on Windows.")
    if options.platform == "windows" and not options.toolchain:
        raise BuildError("Provide --toolchain when targeting Windows (msvc or mingw).")
    if options.sanitizer and options.build_type != "Debug":
        raise BuildError("Sanitizers can only be enabled for Debug builds.")
    if options.production and options.build_type != "Release":
        raise BuildError("Production optimizations only apply to Release builds.")


def main(argv: Optional[Sequence[str]] = None) -> int:
    try:
        cli_args = parse_cli(argv)
        options = merge_cli_with_config(cli_args)
        validate_options(options)

        logger = configure_logging(options.verbose)
        project_root = Path(__file__).resolve().parent.parent

        runner = CommandRunner(logger=logger, dry_run=options.dry_run, env=dict(os.environ))
        toolchain = select_toolchain(options)
        builder = Builder(options=options, project_root=project_root, logger=logger, runner=runner, toolchain=toolchain)

        builder.info()
        if options.info_only:
            return 0

        if options.clean:
            builder.clean()

        builder.configure()
        if options.configure_only:
            return 0

        builder.build()

        if options.create_package:
            builder.package()

        if options.run_after_build:
            builder.run_app()

        logger.info("Build completed successfully.")
        return 0

    except BuildError as error:
        logging.getLogger("serial-studio-build").error(str(error))
        return 1
    except KeyboardInterrupt:
        logging.getLogger("serial-studio-build").warning("Operation interrupted by user.")
        return 130


if __name__ == "__main__":
    sys.exit(main())
