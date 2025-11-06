#!/usr/bin/env python3
# filepath: build.py
# SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-SerialStudio-Commercial

"""
Serial Studio Cross-Platform Build Script

Usage:
  python build.py [OPTIONS]

Examples:
  python build.py --platform mac --type release
  python build.py --platform windows --toolchain msvc --type debug
  python build.py --platform linux --clean
  python build.py --help
"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional, List


class Colors:
    """ANSI color codes for terminal output"""
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'


class BuildConfig:
    """Build configuration container"""
    def __init__(self):
        self.platform: str = self._detect_platform()
        self.build_type: str = "Release"
        self.toolchain: Optional[str] = None
        self.jobs: int = os.cpu_count() or 4
        self.clean: bool = False
        self.verbose: bool = False
        self.production: bool = False
        self.gpl_only: bool = True
        self.sanitizer: bool = False
        
        # Platform-specific defaults
        if self.platform == "windows":
            self.toolchain = "mingw"
    
    @staticmethod
    def _detect_platform() -> str:
        """Auto-detect current platform"""
        system = platform.system().lower()
        if system == "darwin":
            return "mac"
        elif system == "windows":
            return "windows"
        elif system == "linux":
            return "linux"
        else:
            raise RuntimeError(f"Unsupported platform: {system}")
    
    @property
    def build_dir(self) -> str:
        """Generate build directory name"""
        parts = ["build"]
        parts.append(self.build_type.lower())
        if self.platform == "windows" and self.toolchain:
            parts.append(self.toolchain)
        return "-".join(parts)
    
    @property
    def qt_root(self) -> Optional[str]:
        """Detect Qt installation path"""
        if self.platform == "mac":
            homebrew_qt = "/opt/homebrew/opt/qt@6"
            if os.path.exists(homebrew_qt):
                return homebrew_qt
            return os.path.expanduser("~/Qt/6.9.2/macos")
        elif self.platform == "windows":
            base = "C:/Qt/6.9.2"
            if self.toolchain == "msvc":
                return f"{base}/msvc2022_64"
            else:
                return f"{base}/mingw_64"
        elif self.platform == "linux":
            return os.path.expanduser("~/Qt/6.9.2/gcc_64")
        return None


class Builder:
    """Cross-platform build orchestrator"""
    
    def __init__(self, config: BuildConfig):
        self.config = config
        self.project_root = Path(__file__).parent.resolve()
    
    def _print(self, message: str, color: str = Colors.OKBLUE):
        """Print colored message"""
        print(f"{color}{message}{Colors.ENDC}")
    
    def _run(self, cmd: List[str], env: Optional[dict] = None) -> int:
        """Execute command and handle errors"""
        cmd_str = " ".join(cmd)
        if self.config.verbose:
            self._print(f"Executing: {cmd_str}", Colors.OKCYAN)
        
        result = subprocess.run(
            cmd,
            cwd=self.project_root,
            env={**os.environ, **(env or {})},
            stdout=None if self.config.verbose else subprocess.PIPE,
            stderr=subprocess.STDOUT
        )
        
        if result.returncode != 0:
            self._print(f"Command failed: {cmd_str}", Colors.FAIL)
            if result.stdout:
                print(result.stdout.decode())
        
        return result.returncode
    
    def clean(self):
        """Remove build directory"""
        build_path = self.project_root / self.config.build_dir
        if build_path.exists():
            self._print(f"Cleaning {build_path}", Colors.WARNING)
            shutil.rmtree(build_path)
    
    def configure(self) -> int:
        """Run CMake configure step"""
        self._print("=== Configuring Serial Studio ===", Colors.HEADER)
        
        build_path = self.project_root / self.config.build_dir
        build_path.mkdir(parents=True, exist_ok=True)
        
        # Build CMake arguments
        cmake_args = [
            "cmake" if self.config.platform != "mac" else "qt-cmake",
            "-S", ".",
            "-B", self.config.build_dir,
            f"-DCMAKE_BUILD_TYPE={self.config.build_type}",
            f"-DBUILD_GPL3={'ON' if self.config.gpl_only else 'OFF'}",
        ]
        
        # Add production optimization for Release builds
        if self.config.build_type == "Release" and self.config.production:
            cmake_args.append("-DPRODUCTION_OPTIMIZATION=ON")
        
        # Add sanitizers for Debug builds
        if self.config.build_type == "Debug" and self.config.sanitizer:
            cmake_args.append("-DDEBUG_SANITIZER=ON")
        
        # Platform-specific configuration
        env = {}
        
        if self.config.platform == "windows":
            if self.config.toolchain == "mingw":
                cmake_args.extend(["-G", "MinGW Makefiles"])
                qt_root = self.config.qt_root
                if qt_root:
                    cmake_args.append(f"-DCMAKE_PREFIX_PATH={qt_root}")
                    # Add MinGW tools to PATH
                    mingw_bin = f"{qt_root}/bin"
                    mingw_tools = "C:/Qt/Tools/mingw1120_64/bin"
                    env["PATH"] = f"{mingw_bin};{mingw_tools};{os.environ['PATH']}"
            
            elif self.config.toolchain == "msvc":
                cmake_args.extend(["-G", "Ninja"])
                qt_root = self.config.qt_root
                if qt_root:
                    cmake_args.append(f"-DCMAKE_PREFIX_PATH={qt_root}")
                # Note: MSVC environment must be initialized externally
                # via vcvars64.bat or Developer Command Prompt
        
        elif self.config.platform == "mac":
            # qt-cmake automatically handles Qt path on macOS
            pass
        
        elif self.config.platform == "linux":
            cmake_args.extend(["-G", "Ninja"])
            qt_root = self.config.qt_root
            if qt_root:
                cmake_args.append(f"-DCMAKE_PREFIX_PATH={qt_root}")
        
        return self._run(cmake_args, env)
    
    def build(self) -> int:
        """Run CMake build step"""
        self._print("=== Building Serial Studio ===", Colors.HEADER)
        
        cmd = [
            "cmake",
            "--build", self.config.build_dir,
            "--parallel", str(self.config.jobs)
        ]
        
        # MSVC requires --config flag
        if self.config.platform == "windows" and self.config.toolchain == "msvc":
            cmd.extend(["--config", self.config.build_type])
        
        if self.config.verbose:
            cmd.append("--verbose")
        
        return self._run(cmd)
    
    def run(self) -> int:
        """Execute built application"""
        self._print("=== Running Serial Studio ===", Colors.HEADER)
        
        build_path = self.project_root / self.config.build_dir
        
        if self.config.platform == "mac":
            app_path = build_path / "app" / "Serial-Studio-GPL3.app"
            cmd = ["open", str(app_path)]
        
        elif self.config.platform == "windows":
            if self.config.toolchain == "msvc":
                exe_path = build_path / "app" / self.config.build_type / "Serial-Studio-GPL3.exe"
            else:
                exe_path = build_path / "app" / "Serial-Studio-GPL3.exe"
            cmd = [str(exe_path)]
        
        elif self.config.platform == "linux":
            exe_path = build_path / "app" / "Serial-Studio-GPL3"
            cmd = [str(exe_path)]
        
        else:
            self._print(f"Run not supported on {self.config.platform}", Colors.FAIL)
            return 1
        
        if not Path(cmd[0] if self.config.platform != "mac" else str(app_path)).exists():
            self._print(f"Executable not found: {cmd[0]}", Colors.FAIL)
            return 1
        
        return self._run(cmd)
    
    def package(self) -> int:
        """Create installer/package"""
        self._print("=== Packaging Serial Studio ===", Colors.HEADER)
        
        cmd = ["cmake", "--build", self.config.build_dir, "--target", "package"]
        return self._run(cmd)
    
    def info(self):
        """Display build configuration"""
        self._print("=== Build Configuration ===", Colors.HEADER)
        print(f"  Platform:        {self.config.platform}")
        print(f"  Build Type:      {self.config.build_type}")
        if self.config.toolchain:
            print(f"  Toolchain:       {self.config.toolchain}")
        print(f"  Build Dir:       {self.config.build_dir}")
        print(f"  Parallel Jobs:   {self.config.jobs}")
        print(f"  Qt Root:         {self.config.qt_root or 'Auto-detected'}")
        print(f"  GPL-only Build:  {self.config.gpl_only}")
        if self.config.build_type == "Release":
            print(f"  Production Opt:  {self.config.production}")
        if self.config.build_type == "Debug":
            print(f"  Sanitizers:      {self.config.sanitizer}")
        print()


def main():
    parser = argparse.ArgumentParser(
        description="Serial Studio Cross-Platform Build Script",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --platform mac --type release --production
  %(prog)s --platform windows --toolchain msvc --type debug
  %(prog)s --platform linux --clean --jobs 16
  %(prog)s --info
        """
    )
    
    config = BuildConfig()
    
    # Platform selection
    parser.add_argument(
        "--platform", "-p",
        choices=["mac", "windows", "linux"],
        default=config.platform,
        help=f"Target platform (default: {config.platform})"
    )
    
    # Build type
    parser.add_argument(
        "--type", "-t",
        choices=["release", "debug", "Release", "Debug"],
        default="release",
        help="Build type (default: release)"
    )
    
    # Toolchain (Windows only)
    parser.add_argument(
        "--toolchain", "-tc",
        choices=["mingw", "msvc"],
        help="Windows toolchain (default: mingw)"
    )
    
    # Build options
    parser.add_argument(
        "--jobs", "-j",
        type=int,
        default=config.jobs,
        help=f"Parallel jobs (default: {config.jobs})"
    )
    
    parser.add_argument(
        "--clean", "-c",
        action="store_true",
        help="Clean build directory before building"
    )
    
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Verbose build output"
    )
    
    parser.add_argument(
        "--production",
        action="store_true",
        help="Enable production optimizations (Release only)"
    )
    
    parser.add_argument(
        "--sanitizer",
        action="store_true",
        help="Enable AddressSanitizer (Debug only)"
    )
    
    parser.add_argument(
        "--commercial",
        action="store_true",
        help="Build with commercial features (requires license)"
    )
    
    # Actions
    parser.add_argument(
        "--run", "-r",
        action="store_true",
        help="Run application after building"
    )
    
    parser.add_argument(
        "--package",
        action="store_true",
        help="Create installer package"
    )
    
    parser.add_argument(
        "--info",
        action="store_true",
        help="Display build configuration and exit"
    )
    
    args = parser.parse_args()
    
    # Update config from arguments
    config.platform = args.platform
    config.build_type = args.type.capitalize()
    config.toolchain = args.toolchain
    config.jobs = args.jobs
    config.clean = args.clean
    config.verbose = args.verbose
    config.production = args.production
    config.gpl_only = not args.commercial
    config.sanitizer = args.sanitizer
    
    # Validate Windows toolchain
    if config.platform == "windows" and not config.toolchain:
        config.toolchain = "mingw"
    
    builder = Builder(config)
    
    # Display info and exit if requested
    if args.info:
        builder.info()
        return 0
    
    builder.info()
    
    # Execute build pipeline
    try:
        if config.clean:
            builder.clean()
        
        if builder.configure() != 0:
            print(f"{Colors.FAIL}Configuration failed{Colors.ENDC}")
            return 1
        
        if builder.build() != 0:
            print(f"{Colors.FAIL}Build failed{Colors.ENDC}")
            return 1
        
        print(f"{Colors.OKGREEN}Build successful!{Colors.ENDC}")
        
        if args.package:
            if builder.package() != 0:
                print(f"{Colors.FAIL}Packaging failed{Colors.ENDC}")
                return 1
        
        if args.run:
            return builder.run()
        
        return 0
    
    except KeyboardInterrupt:
        print(f"\n{Colors.WARNING}Build interrupted by user{Colors.ENDC}")
        return 130
    except Exception as e:
        print(f"{Colors.FAIL}Error: {e}{Colors.ENDC}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
