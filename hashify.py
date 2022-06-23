# -*- coding: utf-8 -*-
#!/bin/python3
# author: bryan kanu
import hashlib, sys, traceback
from pathlib import Path

ALGORITHM = "sha256"
ACTUAL_COUNT = 0; TOTAL_COUNT = 0
getFiles = lambda paths : list(filter((lambda path: path.is_file()),paths))

def hashify(path):
    global ACTUAL_COUNT
    try:
        with open("hashes.tmp.txt","a") as fd:
            m_hash = hashlib.sha256(); m_hash.update(path.read_bytes())
            fd.write(f"[*] file: {path.absolute()} ({m_hash.hexdigest()})\n")
            ACTUAL_COUNT += 1
    except:
        print(f"[x] {traceback.format_exc()}")

def main(argv):
    global TOTAL_COUNT
    try:
        root = Path(f"{sys.argv[1]}"); paths = getFiles(list(root.rglob("**/*")))
        TOTAL_COUNT = len(paths); list(map(hashify,paths))
    except:
        print(f"[x] {traceback.format_exc()}")

if __name__ == "__main__":
    try:
        print("[*] Running.")
        main(sys.argv)
        print(f"[*] Finished. Found {TOTAL_COUNT} files. Hashed {ACTUAL_COUNT} files using {ALGORITHM}.")
    except:
        print(f"[x] {traceback.format_exc()}")
