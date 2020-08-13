#!/usr/bin/env python3
# pylint: disable=C0103
"""

Resolve DLL Dependencies

"""
import argparse
from collections import deque
from enum import Enum
import logging
import os
import os.path
import shutil

from typing import Deque, Dict, List, Optional

import pefile


class LibraryKind(Enum):
    INPUT = 1
    SYSTEM = 2
    THIRDPARTY = 3


class Library:
    """ Library inforamtion """

    def __init__(self, name: str, path: str, kind: LibraryKind, deps: List[str]):
        self.name = name
        self.path = path
        self.kind = kind
        self.deps = deps


class LibraryNotFoundError(RuntimeError):
    pass


class LibraryResolver:
    """ Resolve library paths and dependencies recursively """
    def __init__(self, libdirs: List[str], ignore_missing: bool = False):
        self.libdirs = libdirs
        self.ignore_missing = ignore_missing
        self.kind_paths = self._make_system_paths()
        self.queue: Deque[str] = deque()
        self.found: Dict[str, Library] = dict()

    @staticmethod
    def _make_system_paths() -> List[str]:
        system_root = os.getenv("SystemRoot")
        if system_root:
            system_paths = [
                os.path.join(system_root, "system32"),
                system_root
            ]
            logging.info("Using system path: %s", ", ".join(system_paths))
            return system_paths

        logging.warning("%%SystemRoot%% not set")
        return []

    def process(self, files: List[str]) -> None:
        """ Process input files """
        for f in files:
            self.process_file(f)

        while len(self.queue) > 0:
            file_name = self.queue.popleft()

            if file_name in self.found:
                continue

            try:
                lib_info = self.get_info(file_name)
                self.record(lib_info)
            except LibraryNotFoundError:
                if self.ignore_missing:
                    logging.info("Can't find %s, ignoring", file_name)
                else:
                    raise


    def process_file(self, path: str) -> None:
        """ Process single input file """
        base_name = os.path.basename(path)
        deps = self.scan_dependencies(path)
        info = Library(base_name, path, LibraryKind.INPUT, deps)
        self.record(info)

    def record(self, info: Library) -> None:
        """ Record resolved library path, and queue for scanning its dependencies """
        for dep in info.deps:
            self.queue.append(dep)
        self.found[info.name] = info

    def print_files(self) -> None:
        """ Print full paths for library dependencies """
        for fi in self.found.values():
            if fi.kind == LibraryKind.THIRDPARTY:
                print(fi.path)

    def copy(self, dest: str, no_overwrite: bool) -> None:
        """ Copy found library paths to destination folder """
        for fi in self.found.values():
            if fi.kind == LibraryKind.THIRDPARTY:
                dest_path = os.path.join(dest, fi.name)
                if os.path.exists(dest_path) and no_overwrite:
                    logging.info("%s already exists in %s", fi.name, dest)
                else:
                    logging.info("Copying %s", fi.path)
                    shutil.copy(fi.path, dest_path)

    def get_info(self, file_name, kind: int = LibraryKind.THIRDPARTY) -> Library:
        """
        Fetch information about given library.
        """
        logging.debug("Processing %s", file_name)

        system_lib = self.find_lib(file_name, self.kind_paths)
        if system_lib:
            logging.debug("  Found system library: %s", system_lib)
            return Library(file_name, system_lib, LibraryKind.SYSTEM, [])

        path = self.find_lib(file_name, self.libdirs)
        if path:
            logging.debug("  Found: %s", path)
            deps = self.scan_dependencies(path)
            logging.debug("  Dependencies: %s", " ".join(deps))
            return Library(file_name, path, kind, deps)

        raise LibraryNotFoundError("Cannot find DLL {0}".format(file_name))

    @staticmethod
    def scan_dependencies(file_name: str) -> List[str]:
        """
        Find libraries that given file depends on.
        """
        pe = pefile.PE(file_name)
        return [entry.dll.decode("utf-8") for entry in pe.DIRECTORY_ENTRY_IMPORT]

    @staticmethod
    def find_lib(name: str, dirs: List[str]) -> Optional[str]:
        """
        Find library file in one of provided directories. Return None
        if library cannot be found.
        """
        for d in dirs:
            path = os.path.join(d, name)
            if os.path.exists(path):
                return path
        return None


def main():
    """
    Application entry point
    """
    parser = argparse.ArgumentParser(description="Copy all DLLs")
    parser.add_argument("files", type=str, nargs="+",
                        help="Files to scan")
    parser.add_argument("-L", "--libdir", type=str, action="append",
                        help="Source directory containing library files")
    parser.add_argument("-d", "--dest", type=str,
                        help="Destination directory")
    parser.add_argument("-V", "--verbose", type=str, action="store", nargs="?",
                        default="warning", const="info",
                        help="Verbosity level")
    parser.add_argument("--dry-run", action='store_true',
                        help="Do not copy anything; print files that would be copied")
    parser.add_argument("--no-overwrite", action='store_true',
                        help="Do not overwrite files")
    parser.add_argument("--ignore-missing", action="store_true",
                        help="Ignore missing libraries")
    args = parser.parse_args()
    logging.basicConfig(level=getattr(logging, args.verbose.upper(), None),
                        format="%(levelname)s %(message)s")

    resolver = LibraryResolver(args.libdir or [],
                               ignore_missing=args.ignore_missing)
    resolver.process(args.files)
    if args.dry_run:
        resolver.print_files()
    else:
        resolver.copy(args.dest, args.no_overwrite)


if __name__ == "__main__":
    main()
