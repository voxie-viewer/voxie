from typing import Tuple
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
from enum import IntEnum
from collections import namedtuple

import numpy as np
from reikna.fft import FFT as _REIKNA_FFT
from reikna.core import Computation, Parameter, Annotation, Type
from reikna.helpers import template_for


class Axis(IntEnum):
    X = 0
    Y = 1
    Z = 2


IntQuadruple = Tuple[int, int, int, int]

BatchShape = namedtuple('BatchShape', 'batch x y z')


class WindowedSum(Computation):

    def __init__(self, axis: Axis, roi_shape: IntQuadruple,
                 kernel_shape: IntQuadruple):
        assert len(roi_shape) == 4
        assert len(kernel_shape) == 4

        # batch dimension of same size!
        assert roi_shape[0] == kernel_shape[0]

        self.axis = axis
        self.roi_shape = roi_shape
        self.filter_size = kernel_shape[1:]
        self.out_shape = self.valid_shape(roi_shape, kernel_shape)

        in_shape = roi_shape
        f32 = np.float32

        Computation.__init__(self, [
            Parameter('roi_batch', Annotation(
                Type(f32, shape=in_shape), 'io')),
        ])

    @staticmethod
    def valid_shape(roi_shape: IntQuadruple,
                    kernel_shape: IntQuadruple) -> IntQuadruple:
        return roi_shape[:1] + tuple(r - k + 1 for r, k
                                     in zip(roi_shape[1:],
                                            kernel_shape[1:]))

    def _build_plan(self, plan_factory, device_params,
                    roi_batch):
        plan = plan_factory()

        template = template_for(__file__)

        full_shape = BatchShape(*self.roi_shape)
        valid_shape = BatchShape(*self.out_shape)
        batch_size = full_shape.batch

        axis = self.axis

        arrays = [roi_batch]

        template = template.get_def(f'sum{axis}')

        if axis is Axis.X:
            global_size = (full_shape.z, full_shape.y, batch_size)
            filter_size = self.filter_size[0]
        elif axis is Axis.Y:
            global_size = (full_shape.z, valid_shape.x, batch_size)
            filter_size = self.filter_size[1]
        elif axis is Axis.Z:
            global_size = (valid_shape.y, valid_shape.x, batch_size)
            filter_size = self.filter_size[2]
        else:
            raise ValueError('Invalid axis argument provided.'
                             'Must be either of {0, 1, 2}.')

        x_end, y_end, z_end = valid_shape[1:]

        plan.kernel_call(
            template,
            arrays,
            global_size=global_size,
            local_size=None,
            render_kwds=dict(
                filter_size=filter_size,
                x_end=x_end,
                y_end=y_end,
                z_end=z_end,
            )
        )

        # filter_size_total = np.prod(self.filter_size)
        # TODO: divide results by filter_size_total right here!

        return plan


def reikna_compile_norm_correlate(cl_thread,
                                  roi_shape: IntQuadruple,
                                  kernel_shape: IntQuadruple):
    dtype = np.complex64
    batched = True

    fft_func = reikna_fft_kernel(cl_thread, dtype,
                                 size=roi_shape,
                                 batched=batched)
    reikna_mean_std = reikna_compile_mean_std(cl_thread,
                                              roi_shape,
                                              kernel_shape)
    # mult_conj = compile_mult_conj(cl_thread)

    def run(roi_batch: np.ndarray, kernel_batch: np.ndarray,
            kernel_shape: IntQuadruple,
            copy_data_to_device=True) -> Tuple[np.ndarray]:
        volume_shape = np.array(roi_batch.shape)
        valid_shape = volume_shape - kernel_shape + 1

        roi_batch_comp = roi_batch.astype(dtype)
        kernel_batch_comp = kernel_batch.astype(dtype)

        volume_fft = _reikna_fft(cl_thread, fft_func, roi_batch_comp,
                                 copy_data_to_device=copy_data_to_device)
        kernel_fft = _reikna_fft(cl_thread, fft_func, kernel_batch_comp,
                                 copy_data_to_device=copy_data_to_device)

        product = volume_fft * kernel_fft.conj()
        corr = _reikna_fft(cl_thread, fft_func, product, inverse=True,
                           copy_data_to_device=False)

        mean, std = reikna_mean_std(roi_batch)  # is blocking
        corr = corr.get()  # is blocking

        corr = corr[:, :valid_shape[1], :valid_shape[2], :valid_shape[3]]
        corr = corr.real.astype('float32')

        return mean, std, corr

    return run


def reikna_compile_mean_std(cl_thread,
                            roi_shape: IntQuadruple,
                            kernel_shape: IntQuadruple):
    kernels = [WindowedSum(axis, roi_shape, kernel_shape)
               .compile(cl_thread)
               for axis in Axis]

    dtype = np.float32
    out_shape = WindowedSum.valid_shape(roi_shape, kernel_shape)
    filter_size_total = np.prod(kernel_shape[1:])

    def run(input_: np.ndarray) -> np.ndarray:
        assert input_.dtype == dtype

        mean_cl = cl_thread.to_device(input_)
        sqrd_cl = cl_thread.to_device(input_ ** 2.0)

        for kernel in kernels:
            e1, = kernel(mean_cl)
            e2, = kernel(sqrd_cl)

            e1.wait()
            e2.wait()

            # e1_start = e1.get_profiling_info(profiling_info.START)
            # e2_start = e2.get_profiling_info(profiling_info.START)
            # e1_end = e1.get_profiling_info(profiling_info.END)
            # e2_end = e2.get_profiling_info(profiling_info.END)

        mean_result = mean_cl[:, :out_shape[1], :out_shape[2], :out_shape[3]]
        mean_result = mean_result.get() / filter_size_total

        sqrd_result = sqrd_cl[:, :out_shape[1], :out_shape[2], :out_shape[3]]
        sqrd_result = sqrd_result.get() / filter_size_total

        std_result = np.sqrt(sqrd_result - mean_result**2)

        return mean_result, std_result

    return run


def reikna_fft_kernel(thread, dtype=np.complex64,
                      size=(32, 32, 32), batched=True):
    arr_like = np.empty(size, dtype=dtype)

    fft = (_REIKNA_FFT(arr_like, axes=(1, 2, 3)) if batched
           else _REIKNA_FFT(arr_like))

    return fft.compile(thread)


def _reikna_fft(thread, reikna_fft_func, data, inverse=False,
                copy_data_to_device=True):
    if copy_data_to_device:
        data = thread.to_device(data)

    reikna_fft_func(data, data, 1 if inverse else 0)

    return data


def reikna_compile_correlate(cl_thread, batch_shape: IntQuadruple,
                             batched=True):
    dtype = np.complex64

    fft_func = reikna_fft_kernel(cl_thread, dtype,
                                 size=batch_shape,
                                 batched=batched)

    def run(roi_batch: np.ndarray, kernel_batch: np.ndarray,
            kernel_shape: IntQuadruple,
            copy_data_to_device=True) -> np.ndarray:
        return _correlate_gpu(cl_thread, fft_func,
                              roi_batch.astype(dtype),
                              kernel_batch.astype(dtype),
                              kernel_shape,
                              batched,
                              copy_data_to_device)

    return run


def _correlate_gpu(cl_thread, reikna_fft_func, volume: np.ndarray,
                   kernel: np.ndarray, kernel_shape: np.ndarray,
                   batched=True,
                   copy_data_to_device=True) -> np.ndarray:
    volume_shape = np.array(volume.shape)
    valid_shape = volume_shape - kernel_shape + 1

    volume_fft = _reikna_fft(cl_thread, reikna_fft_func, volume,
                             copy_data_to_device=copy_data_to_device)
    kernel_fft = _reikna_fft(cl_thread, reikna_fft_func, kernel,
                             copy_data_to_device=copy_data_to_device)

    product = volume_fft * kernel_fft.conj()

    corr = _reikna_fft(cl_thread, reikna_fft_func, product, inverse=True,
                       copy_data_to_device=False)
    corr = corr.get()

    if batched:
        result = corr[:, :valid_shape[1], :valid_shape[2], :valid_shape[3]]
    else:
        result = corr[:valid_shape[0], :valid_shape[1], :valid_shape[2]]

    return result.real.astype('float32')


def _window_mean_std_batched(roi_batch, kernel_shape: tuple):
    # Efficiently calculate mean and standard deviation using sum of squares
    # idea: https://stackoverflow.com/a/18422519

    kernel_shape_1 = kernel_shape[1:]

    origin = (0,) + tuple(int(-x / 2) for x in kernel_shape_1)
    filter_shape = (1,) + kernel_shape_1
    valid_end = tuple(r - k + 1 for r, k in zip(roi_batch.shape[1:],
                                                kernel_shape_1))

    mean = uniform_filter(roi_batch, filter_shape,
                          mode='constant', origin=origin)
    meansqrd = uniform_filter(roi_batch**2,
                              filter_shape,
                              mode='constant',
                              origin=origin)

    std_result = np.sqrt(meansqrd - mean**2)

    mean_result = mean[:, :valid_end[0],
                       :valid_end[1],
                       :valid_end[2]]
    std_result = std_result[:, :valid_end[0],
                            :valid_end[1],
                            :valid_end[2]]

    return mean_result, std_result


def _mse(arr1: np.ndarray, arr2: np.ndarray) -> float:
    return float(np.mean((arr1 - arr2) ** 2))


def main():
    global uniform_filter
    from scipy.ndimage.filters import uniform_filter
    from reikna.cluda import ocl_api

    from time import perf_counter

    roi_shape = (256, 32, 32, 32)
    kernel_shape = (256, 16, 16, 16)

    np.random.seed(11)

    api = ocl_api()
    device = api.get_platforms()[0].get_devices(4)[0]
    thread = api.Thread(device)

    filter_func = reikna_compile_mean_std(
        thread, roi_shape, kernel_shape
    )

    for _ in range(10):
        roi_batch = np.random.random((256, 32, 32, 32)).astype('float32')

        # context = cl.Context(devices=[device])
        # queue = cl.CommandQueue(context, properties=cl.command_queue_properties.PROFILING_ENABLE)

        # sqrd_result = thread.array(out_shape, dtype='float32')

        t = perf_counter()
        mean_result, std_result = filter_func(roi_batch)
        print(perf_counter() - t)

        mean_true, std_true = _window_mean_std_batched(roi_batch, kernel_shape)

        print('mse mean:', _mse(mean_result, mean_true))
        print('mse std:', _mse(std_result, std_true))


if __name__ == '__main__':
    main()
