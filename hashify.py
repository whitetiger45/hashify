# -*- coding: utf-8 -*-
#!/bin/python3
# author: bryan kanu
from datetime import datetime, date, time, timezone
from pathlib import Path

import argparse, hashlib, traceback

ALGORITHM = "sha256"
HASHIFIED_HASHES_FNAME = ""
TOTAL_PATHS_FOUND = 0
TOTAL_PATHS_HASHIFIED = 0

get_files = lambda paths : list(filter((lambda path: path.is_file()),paths))
get_timestamp = lambda : datetime.now(timezone.utc).strftime('%m-%d-%y.%H-%M-%S')

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
        root = Path(f"{argv.directory}")
        HASHIFIED_HASHES_FNAME = f"{argv.directory}.hashes.{get_timestamp()}.txt"
        paths = get_files(list(root.rglob("**/*")))
        list(map(hashify,paths))
    except:
        print(f"[x] {traceback.format_exc()}")
    return len(paths)

parser = argparse.ArgumentParser(prog=f"hashify", 
    description="Program to help verify the integrity of files")
parser.add_argument("-d", "--dir", dest="directory", metavar="DIRECTORY",
    required=True, type=Path, help="Root directory to start recursive hashing")
argv = parser.parse_args()

if __name__ == "__main__":
    try:
        print(f"[*] Algorithm: {ALGORITHM}")
        TOTAL_PATHS_FOUND = main()
        print(f"[*] Found {TOTAL_PATHS_FOUND} files")
        print(f"[*] Hashed {TOTAL_PATHS_HASHIFIED} files")
        print(f"[*] Output written to: {HASHIFIED_HASHES_FNAME}")
    except:
        print(f"[x] {traceback.format_exc()}")
