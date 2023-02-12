#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# author: bryan kanu
# Release v1.0
import argparse
import hashlib
import traceback
from datetime import datetime, timezone
from pathlib import Path
from typing import List, Iterable, Sequence, TextIO

ALGORITHM: str = 'sha256'
EXECUTION_TIME: str = datetime.now(timezone.utc).strftime('%m-%d-%y.%H-%M-%S')


def file_exists(path: Path) -> bool:
    """Comfort function checking whether the given path points to an existing file.

    :parameter path: The path to check.
    :returns: Whether the path exists and is a file.
    """

    return path.exists() and path.is_file()


def filter_existing_files(paths: Iterable[Path]) -> List[Path]:
    """Keep only paths referring to files.

    :parameter paths: A list of paths to filter.
    :returns: The filtered paths.
    """
    return [path for path in paths if path.is_file()]


def collect_differences(lines1: Sequence[str], lines2: Sequence[str]):
    diff1: str = ''
    """Lines in lines2 that don't exist in lines1."""

    diff2 = ''
    """Lines in lines1 that don't exist in lines2."""

    index1: int = 0
    len1: int = len(lines1)
    index2: int = 0
    len2: int = len(lines2)

    while index1 < len1 or index2 < len2:
        if index1 == len1:
            if lines2[index2] not in lines1[index1:]:
                diff1 += f'[1->2 line {index2}]: {lines2[index2]}\n'
            index2 += 1
        elif index2 == len2:
            if lines1[index1] not in lines2[index2:]:
                diff2 += f'[2->1 line {index1}]: {lines1[index1]}\n'
            index1 += 1
        else:
            if lines1[index1] != lines2[index2]:
                if lines2[index2] not in lines1[index1:]:
                    diff1 += f'[1->2 line {index2}]: {lines2[index2]}\n'
                if lines1[index1] not in lines2[index2:]:
                    diff2 += f'[2->1 line {index1}]: {lines1[index1]}\n'
            index1 += 1
            index2 += 1
    return diff1, diff2


class Hashifier:
    total_paths_hashified: int

    def __init__(self):
        self.total_paths_hashified = 0

    def hashify(self, path: Path, target_file: TextIO):
        sha = hashlib.new(name=ALGORITHM)
        sha.update(path.read_bytes())
        target_file.write(f'{path.absolute()} {sha.hexdigest()}\n')
        target_file.flush()
        self.total_paths_hashified += 1


def directory_main():
    paths = []
    hashes_fname = f'{argv["directory"]}.hashes.{EXECUTION_TIME}.txt'
    hashifier = Hashifier()
    try:
        root = Path(f'{argv["directory"]}')
        paths = filter_existing_files(root.rglob("**/*"))
        with open(hashes_fname, 'wt') as target_file:
            for path in paths:
                hashifier.hashify(path, target_file)
    except IOError:
        print(f"[x] {traceback.format_exc()}")
    return len(paths), hashifier.total_paths_hashified, hashes_fname


parser = argparse.ArgumentParser(prog=f"hashify",
                                 description="Program to help verify the integrity of files")
subparsers = parser.add_subparsers(help='sub-command help')
hashes_parser = subparsers.add_parser("hashes", help="Generate file hashes starting in DIRECTORY")
hashes_parser.add_argument(dest="directory", metavar="DIRECTORY",
                           type=Path, help="Root directory to start recursive hashing")
diff_files_parser = subparsers.add_parser("diff", help="Find out what changes were made to a file")
diff_files_parser.add_argument(dest="file_1", metavar="FILE_1",
                               type=Path, help="The name or path of FILE_1")
diff_files_parser.add_argument(dest="file_2", metavar="FILE_2",
                               type=Path, help="The name or path of FILE_2")
argv = vars(parser.parse_args())


def main():
    try:
        if "directory" in argv:
            print(f"[*] Algorithm: {ALGORITHM}")
            total_paths_found, total_paths_hashified, hashified_hashes_fname = directory_main()
            print(f"[*] Found {total_paths_found} files")
            print(f"[*] Hashed {total_paths_hashified} files")
            print(f"[*] Output written to: {hashified_hashes_fname}")
        elif "file_1" in argv and "file_2" in argv:
            output_dir = Path(EXECUTION_TIME)
            output_dir.mkdir()
            if file_exists(argv["file_1"]) and file_exists(argv["file_2"]):
                file_1 = argv["file_1"].read_text().split("\n")
                file_2 = argv["file_2"].read_text().split("\n")
                print(f"[*] Diffing {argv['file_1']} and {argv['file_2']}")
                diff_1, diff_2 = collect_differences(file_1, file_2)
                output_fd_1 = Path(f"{output_dir}/diff_1.txt")
                output_fd_2 = Path(f"{output_dir}/diff_2.txt")
                output_fd_1.write_text(f"[1: {argv['file_1']}]\n[2: {argv['file_2']}]\n\n{diff_1}")
                output_fd_2.write_text(f"[1: {argv['file_1']}]\n[2: {argv['file_2']}]\n\n{diff_2}")
                print(f"[*] Diffs written to {EXECUTION_TIME}/")
            else:
                print(f"[x] Check file paths!")
                print(f"[!] Found {argv['file_1']}: {file_exists(argv['file_1'])}")
                print(f"[!] Found {argv['file_2']}: {file_exists(argv['file_2'])}")
        else:
            parser.print_help()
    except:
        print(f"[x] {traceback.format_exc()}")


if __name__ == "__main__":
    main()
