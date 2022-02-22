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
from reikna.fft import FFT
from reikna.cluda import ocl_api


REIKNA_CL_THREAD = None
REIKNA_FFT = None
IN_ARRAY = None


def reikna_compile_fft(thread, dtype=np.complex64,
                       size=(32, 32, 32), batched=True):
    arr_like = np.empty(size, dtype=dtype)
    fft = FFT(arr_like, axes=(1, 2, 3)) if batched else FFT(arr_like)

    return fft.compile(REIKNA_CL_THREAD)


def arm(data=None, batched=True):
    global REIKNA_CL_THREAD
    global REIKNA_FFT
    global IN_ARRAY

    REIKNA_CL_THREAD = ocl_api().Thread.create()

    REIKNA_FFT = reikna_compile_fft(
        REIKNA_CL_THREAD, np.complex64, data.shape, batched)
    IN_ARRAY = REIKNA_CL_THREAD.to_device(data)


def reikna_fft(data, inverse=False, copy_data_to_device=True):
    if copy_data_to_device:
        IN_ARRAY.set(data, REIKNA_CL_THREAD._queue)
        data = IN_ARRAY

    events = REIKNA_FFT(data, data, 1 if inverse else 0)

    return data, events


def _reverse_and_conj(x):
    """
    Reverse array `x` in all dimensions and perform the complex conjugate
    """
    reverse = (slice(None, None, -1),) * x.ndim
    return x[reverse].conj()


def _input_swap_needed(shape1, shape2):
    axes = axes = range(len(shape1))

    ok1 = all(shape1[i] >= shape2[i] for i in axes)
    ok2 = all(shape2[i] >= shape1[i] for i in axes)

    if not (ok1 or ok2):
        raise ValueError("For 'valid' mode, one must be at least "
                         "as large as the other in every dimension")

    return not ok1


def correlate_gpu(volume: np.ndarray, kernel: np.ndarray,
                  kernel_shape: np.ndarray) -> np.ndarray:
    # if _input_swap_needed(volume.shape, kernel.shape):
    #     # TODO: should probably be ValueError in voxie!
    #     volume, kernel = kernel, volume

    # output_shape = np.array(reikna_fft_func.parameter.output.shape)

    volume_shape = np.array(volume.shape)
    full_shape = volume_shape + kernel_shape - 1

    valid_shape = volume_shape - kernel_shape + 1
    start = (full_shape - valid_shape) // 2
    end = start + valid_shape

    volume_fft, _ = reikna_fft(volume)
    kernel_fft, _ = reikna_fft(kernel)

    product = volume_fft * kernel_fft.conjugate()

    corr, _ = reikna_fft(product, inverse=True, copy_data_to_device=False)

    result = corr.get()

    return result[start[0]:end[0], start[1]:end[1],
                  start[2]:end[2]].real.astype('float32')


# def corr_gpu(volume, kernel):
#     if _input_swap_needed(volume.shape, kernel.shape):
#         volume, kernel = kernel, volume

#     kernel = _reverse_and_conj(kernel)

#     output_shape = np.array(REIKNA_FFT.parameter.output.shape)

#     volume_shape, kernel_shape = np.array(volume.shape), np.array(kernel.shape)
#     full_shape = volume_shape + kernel_shape - 1

#     valid_shape = volume_shape - kernel_shape + 1
#     start = (full_shape - valid_shape) // 2
#     end = start + valid_shape

#     pad_kernel = (output_shape-kernel_shape)[0]
#     pad_volume = (output_shape-volume_shape)[0]

#     # TODO: support non-uniform and non even kernel shapes, like (32, 32, 31), etc.
#     kernel = np.pad(kernel, (0, pad_kernel))
#     volume = np.pad(volume, (0, pad_volume))

#     volume_fft, _ = reikna_fft(volume)
#     kernel_fft, _ = reikna_fft(kernel)

#     product = volume_fft * kernel_fft

#     corr, _events = reikna_fft(product, inverse=True, copy_data_to_device=False)

#     result = corr.get()[start[0]:end[0], start[1]:end[1], start[2]:end[2]]

#     return result.real.astype('float32')


def test(data: np.ndarray):
    gpu_data, events = reikna_fft(data)

    for event in events:
        event.wait()

    result = gpu_data.get()
    return result


def main():
    np.random.seed(seed=42)
    test_a = np.random.random((256, 32, 32, 32)).astype('complex64')
    test_b = np.random.random((256, 32, 32, 32)).astype('complex64')
    arm(np.random.random((32, 32, 32)).astype('complex64'), batched=False)

    # from scipy.signal import correlate

    # kernel = np.zeros((256, 32, 32, 32), dtype='complex64')
    # # kernel[:, 16:, 16:, 16:] = test_b

    # corr_res = correlate_gpu(test_a[0], test_b[0], np.array(test_b.shape[1:]))
    # corr_value = correlate(test_a[0], test_b[0], mode="valid", method="fft").real.astype('float32')
    # print(corr_res.shape)
    # print(corr_value.shape)

    # print(corr_res-corr_value)

    res, _ = reikna_fft(test_a[0])
    print(
        np.round(
            reikna_fft(
                res,
                inverse=True,
                copy_data_to_device=False)[0].get() -
            test_a[0],
            4))


if __name__ == '__main__':
    main()
