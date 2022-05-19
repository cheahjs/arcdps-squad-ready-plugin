#!/usr/bin/env python3

import os
import sys
import re

def main():
    if len(sys.argv) < 4:
        print("Usage: stamp-version.py <tag> <run_id> <git_hash>")
        exit(1)
    version = sys.argv[1]
    run_id = sys.argv[2]
    git_hash = sys.argv[3]
    binary_version = f"{version.replace('.', ',')},{run_id}"
    string_version = f"{version}+{run_id}-{git_hash}"
    print(f"Stamping '{binary_version}' and '{string_version}'")

    __location__ = os.path.realpath(os.path.join(os.getcwd(), os.path.dirname(__file__)))
    new_file = ''
    rc_path = os.path.join(__location__, '..', 'squad_ready', 'arcdps-squad-ready-plugin.rc')
    with open(rc_path, 'r') as f:
        new_file = f.read()
    new_file = re.sub(r'(.*(?:(?:FILEVERSION)|(?:PRODUCTVERSION))).*', f'\\1 {binary_version}', new_file)
    new_file = re.sub(r'(.*(?:(?:FileVersion)|(?:ProductVersion))").*', f'\\1, "{string_version}"', new_file)
    with open(rc_path, 'w') as f:
        f.write(new_file)


if __name__ == "__main__":
    main()
