# -*- coding: utf-8 -*-
#!/bin/python3
# author: bryan kanu
# Release v1.0
from datetime import datetime, date, time, timezone
from pathlib import Path

import argparse, hashlib, sys, traceback

ALGORITHM = "sha256"
HASHIFIED_HASHES_FNAME = ""
TOTAL_PATHS_FOUND = 0
TOTAL_PATHS_HASHIFIED = 0

file_exists = lambda fd: (fd.exists() and fd.is_file()) == True
get_files = lambda paths : list(filter((lambda path: path.is_file()),paths))
get_timestamp = lambda : datetime.now(timezone.utc).strftime('%m-%d-%y.%H-%M-%S')
tstamp = get_timestamp()

def ascii_walker(f1,f2):
    ret_1 = ""; ret_2 = ""
    i = 0; j = 0
    while i < len(f1)-1 or j < len(f2)-1:
        if i < len(f1)-1 and j < len(f2)-1:
            if f1[i] != f2[j]:
                if f2[j] not in f1:
                    ret_1 += f"[1->2 line {j}]: {f2[j]}\n"
                if f1[i] not in f2:
                    ret_2 += f"[2->1 line {i}]: {f1[i]}\n"
        elif i >= len(f1)-1 and j < len(f2)-1: 
            if f2[j] not in f1:
                ret_1 += f"[1->2 line {j}]: {f2[j]}\n"
        elif i < len(f1)-1 and j >= len(f2)-1: 
            if f1[i] not in f2:
                ret_2 += f"[2->1 line {i}]: {f1[i]}\n"
        i += 1; j += 1
    return ret_1, ret_2

def hashify(path):
    global TOTAL_PATHS_HASHIFIED
    try:
        with open(HASHIFIED_HASHES_FNAME,"a") as fd:
            m_hash = hashlib.sha256(); m_hash.update(path.read_bytes())
            fd.write(f"{path.absolute()} {m_hash.hexdigest()}\n")
            TOTAL_PATHS_HASHIFIED += 1
    except:
        print(f"[x] {traceback.format_exc()}")

def main():
    global HASHIFIED_HASHES_FNAME
    paths = []
    try:
        root = Path(f"{argv['directory']}")
        HASHIFIED_HASHES_FNAME = f"{argv['directory']}.hashes.{tstamp}.txt"
        paths = get_files(list(root.rglob("**/*")))
        list(map(hashify,paths))
    except:
        print(f"[x] {traceback.format_exc()}")
    return len(paths)

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

if __name__ == "__main__":
    try:        
        if "directory" in argv:
            print(f"[*] Algorithm: {ALGORITHM}")
            TOTAL_PATHS_FOUND = main()
            print(f"[*] Found {TOTAL_PATHS_FOUND} files")
            print(f"[*] Hashed {TOTAL_PATHS_HASHIFIED} files")
            print(f"[*] Output written to: {HASHIFIED_HASHES_FNAME}")
        elif "file_1" in argv and "file_2" in argv:            
            output_dir = Path(tstamp)
            output_dir.mkdir()
            if file_exists(argv["file_1"]) and file_exists(argv["file_2"]):
                file_1 = argv["file_1"].read_text().split("\n")
                file_2 = argv["file_2"].read_text().split("\n")
                print(f"[*] Diffing {argv['file_1']} and {argv['file_2']}")
                diff_1,diff_2 = ascii_walker(file_1,file_2)
                output_fd_1 = Path(f"{output_dir}/diff_1.txt")
                output_fd_2 = Path(f"{output_dir}/diff_2.txt")
                output_fd_1.write_text(f"[1: {argv['file_1']}]\n[2: {argv['file_2']}]\n\n{diff_1}")
                output_fd_2.write_text(f"[1: {argv['file_1']}]\n[2: {argv['file_2']}]\n\n{diff_2}")
                print(f"[*] Diffs written to {tstamp}/")
            else:
                print(f"[x] Check file paths!")
                print(f"[!] Found {argv['file_1']}: {file_exists(argv['file_1'])}")
                print(f"[!] Found {argv['file_2']}: {file_exists(argv['file_2'])}")
        else:
            parser.print_help()
    except:
        print(f"[x] {traceback.format_exc()}")
