#!/usr/bin/python3

import sys
import os
import subprocess
import argparse

all_tests = [
    'vxvol',
    'jpeg_vxvol',
    'hdf5',
]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--use-release', action='store_true')
    args = parser.parse_args()

    voxie_binary = './voxie.sh'
    if args.use_release:
        voxie_binary = 'build/release/voxie'

    voxie_dir = os.path.dirname(__file__)
    os.chdir(voxie_dir)
    os.chdir('../../..')

    for test in all_tests:
        print(f'Running test {test}...', flush=True)
        res = subprocess.run([voxie_binary, '--batch', '--main-window=headless', f'src/Test/Projects/{test}.vxprj.py'])
        if res.returncode != 0:
            print(f'Test {test} failed with returncode {res.returncode}', flush=True)
            sys.exit(1)
        print(f'Test {test} was successful', flush=True)

    print('========= SUCCESS =========', flush=True)

if __name__ == '__main__':
    main()
