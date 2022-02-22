import os
#
# Copyright (c) 2014-2022 The Voxie Authors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
import argparse
import importlib
from pathlib import Path
from timeit import default_timer

import numpy as np


PID = os.getpid()
TEST_FUNCTION_NAME = 'test'
ARMING_FUNCTION_NAME = 'arm'

TIMING_DATA_PATH = Path.cwd() / 'timing_data'
SEP = '\n'

BATCH_SIZE = 64
FFT_SIZE = 32

SAMPLING_RATE = 100
ITERATIONS = 20

RANDOM_SEED = 42


def gen_test_data(shape=(BATCH_SIZE, FFT_SIZE, FFT_SIZE, FFT_SIZE)):
    np.random.seed(RANDOM_SEED)
    return (np.random.random(shape) * 1000.0).astype(np.complex64)


def main():
    argparser = argparse.ArgumentParser()
    argparser.add_argument('test_module_name', type=str)
    argparser.add_argument('batch_size', type=int, default=BATCH_SIZE)
    argparser.add_argument('fft_size', type=int, default=FFT_SIZE)

    args = vars(argparser.parse_args())

    module_to_test_name = args.get('test_module_name')
    module_to_test = importlib.import_module(module_to_test_name)

    batch_size = args.get('batch_size')
    fft_size = args.get('fft_size')

    print('commencing test of', module_to_test_name)
    test_data = gen_test_data(shape=(batch_size, fft_size, fft_size, fft_size))
    test_func = getattr(module_to_test, TEST_FUNCTION_NAME)

    arm_func = getattr(module_to_test, ARMING_FUNCTION_NAME)

    filename = (f'profile_{module_to_test_name}'
                f'_{batch_size}_{fft_size}_{SAMPLING_RATE}sr_native')

    arm_func(test_data)

    # subprocess.Popen(['py-spy', 'record', '--output', filename + '.svg',
    #                   '--pid', str(PID),
    #                   '--rate', str(SAMPLING_RATE),
    #                   '-n'])

    timings = []

    for _ in range(ITERATIONS):
        t1 = default_timer()
        _result = test_func(test_data)
        timings.append(default_timer() - t1)

    data_path = TIMING_DATA_PATH / str(fft_size)

    data_path.mkdir(exist_ok=True, parents=True)
    with (data_path / (filename + '.csv')).open('w') as f:
        np.array(timings).tofile(f, sep=SEP)


if __name__ == '__main__':
    main()
