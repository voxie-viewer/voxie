import itertools
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
from typing import Tuple

import numpy as np
from scipy.signal import correlate
from reikna.cluda import ocl_api
from reikna.core import Computation, Parameter, Annotation, Type
from reikna.helpers import template_for

import digitalvolumecorrelation.profiling as profiling
from digitalvolumecorrelation.correlate.gpu import reikna_compile_correlate

PROFILER_INV = profiling.add_profiler('opt_filter_matrix_inv')
PROFILER_MAT = profiling.add_profiler('opt_filter_gpu_matrix_creation')


OPT_FILTER_USE_GPU = False


class AutocorrMatrix(Computation):

    def __init__(self, image_shape: Tuple[int, int, int], filter_size: int):
        assert len(image_shape) == 3

        self.filter_size = filter_size

        out_shape = (filter_size**3, filter_size**3)
        f32 = np.float32

        Computation.__init__(self, [
            Parameter('output', Annotation(Type(f32, shape=out_shape), 'o')),
            Parameter('image', Annotation(Type(f32, shape=image_shape), 'i')),
        ])

    @staticmethod
    def _create_indice_lookup_arr(filter_size: int) -> np.ndarray:
        arr = list(itertools.product(*(range(filter_size),) * 3))
        return np.array(arr, dtype=np.uint8)

    def _build_plan(self, plan_factory, device_params,
                    output, image):
        plan = plan_factory()

        template = template_for(__file__)

        x_end, y_end, z_end = (s - self.filter_size + 1 for s in image.shape)

        indices = plan.constant_array(
            self._create_indice_lookup_arr(self.filter_size)
        )

        plan.kernel_call(
            template.get_def('autocorr'),
            [output, image, indices],
            global_size=output.shape,
            local_size=None,
            render_kwds=dict(
                x_end=x_end,
                y_end=y_end,
                z_end=z_end
            )
        )

        return plan


def reikna_compile_optimal_filter(cl_thread, image_shape: tuple,
                                  interpolation_order: int):
    autocorr_kernel = AutocorrMatrix(image_shape, interpolation_order + 1)
    autocorr_kernel = autocorr_kernel.compile(cl_thread)

    correlate_func = None

    if OPT_FILTER_USE_GPU:
        correlate_func = reikna_compile_correlate(cl_thread, image_shape,
                                                  batched=False)

    def run(kernel_patch: np.ndarray, roi_patch: np.ndarray) -> np.ndarray:
        return _optimal_filter_3d_patch_shift(
            kernel_patch,
            roi_patch,
            interpolation_order,
            cl_thread,
            autocorr_kernel,
            correlate_func)

    return run


def _optimal_filter_3d_patch_opencl(ref_patch: np.ndarray,
                                    img_patch: np.ndarray,
                                    filter_size: int,
                                    cl_thread,
                                    filter_func: callable,
                                    correlate_func: callable) -> np.ndarray:
    assert ref_patch.shape == img_patch.shape

    if filter_size % 2 == 0:
        slice_pos = slice(filter_size // 2 - 1, -filter_size // 2)
    else:
        slice_pos = slice(filter_size // 2, -filter_size // 2 + 1)

    vector_img_n = img_patch[slice_pos, slice_pos, slice_pos]
    img_n_shape = vector_img_n.shape

    # non transposed vector_imgn, because (:) (in the original matlab code)

    t_mat = PROFILER_MAT.start()
    invertible = cl_thread.array((filter_size**3,) * 2, dtype='float32')
    ref_patch_cl = cl_thread.to_device(np.copy(ref_patch, order='C'))

    assert ref_patch_cl.dtype == np.float32
    event, = filter_func(invertible, ref_patch_cl)

    if OPT_FILTER_USE_GPU:
        vector_img_n_padded = np.zeros_like(ref_patch)
        vector_img_n_padded[:img_n_shape[0],
                            :img_n_shape[1],
                            :img_n_shape[2]] = vector_img_n

        # will block until data gets copied back from gpu
        x_ti = correlate_func(ref_patch, vector_img_n_padded, img_n_shape)
    else:

        x_ti = correlate(ref_patch, vector_img_n, mode='valid', method='fft')
    x_ti = x_ti.flatten()

    # thus we wait for the other filter function to finish after
    event.wait()
    invertible = invertible.get()
    PROFILER_MAT.stop(t_mat)

    t_inv = PROFILER_INV.start()
    # inverse_n = np.linalg.inv(invertible)
    inverse_n_x_ti = np.linalg.solve(invertible, x_ti)
    PROFILER_INV.stop(t_inv)

    filter_n = inverse_n_x_ti.reshape((filter_size, filter_size, filter_size))
    filter_n = filter_n / np.sum(filter_n)  # normalize to 1.0

    return filter_n


def _optimal_filter_3d_patch_shift(ref_patch: np.ndarray,
                                   img_patch: np.ndarray,
                                   interpolation_order: int,
                                   cl_thread,
                                   filter_func: callable,
                                   correlate_func: callable) -> Tuple[float,
                                                                      float]:
    """Computes subpixel shift of two image patches
     via optimal filter method.

    >>> a = np.random.random((30, 30, 30))
    >>> b = np.zeros_like(a)

    >>> b[:-1, :-1, :-1] = (a[:-1, :-1, :-1] + a[1:, 1:, 1:]) / 2

    >>> x_shift, y_shift, z_shift = _optimal_filter_3d_patch_shift(a, b, 4)

    >>> round(x_shift, 3)
    -0.5
    >>> round(y_shift, 3)
    -0.5
    >>> round(z_shift, 3)
    -0.5
    """

    filter_size = interpolation_order + 1

    if filter_size % 2 == 0:
        weight = np.arange(-(filter_size // 2 - 1), filter_size // 2 + 1)
    else:
        weight = np.arange(-(filter_size // 2), filter_size // 2 + 1)

    if img_patch.shape != ref_patch.shape:
        raise ValueError(f'Dimensions of reference ({ref_patch.shape}) '
                         f'and image ({img_patch.shape}) patch must be equal!')

    filter_n = _optimal_filter_3d_patch_opencl(
        ref_patch, img_patch, filter_size, cl_thread,
        filter_func, correlate_func
    )

    return (-np.sum(filter_n, axis=(1, 2), dtype='float32') @ weight,
            -np.sum(filter_n, axis=(0, 2), dtype='float32') @ weight,
            -np.sum(filter_n, axis=(0, 1), dtype='float32') @ weight)


def main():
    np.random.seed(7)
    a = np.random.random((16, 16, 16)).astype('float32')
    b = np.zeros_like(a)

    b[:-1, :-1, :-1] = (a[:-1, :-1, :-1] + a[1:, 1:, 1:]) / 2

    filter_size = 4

    api = ocl_api()
    device = api.get_platforms()[0].get_devices(4)[0]
    thread = api.Thread(device)

    filter_func = reikna_compile_optimal_filter(
        thread, (16, 16, 16), filter_size)

    shifts = _optimal_filter_3d_patch_shift(
        a, b, filter_size, thread, filter_func, None)

    print(shifts)


if __name__ == '__main__':
    main()
