import subprocess
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
from pathlib import Path


TEST_DRIVER_PATH = Path.cwd(
) / 'filters/digitalvolumecorrelation/perftest/fftgputest/test_driver.py'
BATCHES_TO_TEST = [2**i for i in range(0, 11)]
MODULES_TO_TEST = [
    # 'cpu_test',
    'gpu_gpyfft_test',
    'gpu_reikna_test'
]
FFT_SIZES_TO_TEST = [64, 32]


def main():
    for module in MODULES_TO_TEST:
        for fft_size in FFT_SIZES_TO_TEST:
            for batch_size in BATCHES_TO_TEST:
                print(module, batch_size, fft_size)
                subprocess.run(
                    ['python', str(TEST_DRIVER_PATH),
                     module, str(batch_size), str(fft_size)])


if __name__ == '__main__':
    main()
