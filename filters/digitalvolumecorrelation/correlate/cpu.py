import numpy as np
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


def correlate_np(volume: np.ndarray, kernel: np.ndarray,
                 kernel_shape: np.ndarray) -> np.ndarray:

    volume_shape = np.array(volume.shape)
    valid_shape = volume_shape - kernel_shape + 1

    volume_fft = np.fft.fftn(volume, axes=(1, 2, 3))
    kernel_fft = np.fft.fftn(kernel, axes=(1, 2, 3))

    product = volume_fft * kernel_fft.conj()

    corr = np.fft.ifftn(product, axes=(1, 2, 3))

    result = corr[:, :valid_shape[1], :valid_shape[2], :valid_shape[3]]

    return result.real.astype('float32')
