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
import pyopencl as cl
import pyopencl.array as cla
from gpyfft.fft import FFT as _FFT

CONTEXT = None
QUEUE = None
FFT_FUNC = None
IN_ARRAY = None


def arm(data=None):
    global CONTEXT
    global QUEUE
    global FFT_FUNC
    global IN_ARRAY

    CONTEXT = cl.create_some_context()
    QUEUE = cl.CommandQueue(CONTEXT)
    IN_ARRAY = cla.to_device(QUEUE, data)
    FFT_FUNC = FFT(IN_ARRAY, axes=(1, 2, 3))


def FFT(in_array, out_array=None, axes=None, real=False, callbacks=None):
    return _FFT(CONTEXT, QUEUE, in_array, out_array, axes, False,
                real, callbacks)


def fft_gpu(in_out_array: np.ndarray, inverse=False):
    IN_ARRAY.set(in_out_array, queue=QUEUE)

    event, = FFT_FUNC.enqueue(forward=not inverse)

    return event, IN_ARRAY


def test(data: np.ndarray):
    event, data = fft_gpu(data)
    event.wait()
    result = data.get()
    return result
