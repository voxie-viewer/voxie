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
from numpy.fft import fftn, ifftn
from scipy.signal import correlate
from skimage.util import view_as_windows

import digitalvolumecorrelation.profiling as profiling


PROFILER_INV = profiling.add_profiler('opt_filter_cpu_matrix_inv')
PROFILER_MAT = profiling.add_profiler('opt_filter_cpu_matrix_creation')


def _im2col(arr: np.ndarray, block_size) -> np.ndarray:
    return view_as_windows(arr, block_size).reshape(-1, np.prod(block_size))


def _optimal_filter_3d_patch_direct_corr(ref_patch: np.ndarray,
                                         img_patch: np.ndarray,
                                         filter_size: int) -> np.ndarray:
    assert ref_patch.shape == img_patch.shape

    # x_n = _im2col(ref_patch, (filter_size, filter_size,
    #                           filter_size))
    # least squares

    if filter_size % 2 != 0:
        raise ValueError(f"filter_size={filter_size} isn't even")

    slice_pos = slice(filter_size // 2 - 1, -filter_size // 2)
    vector_img_n = img_patch[slice_pos, slice_pos, slice_pos]

    # non transposed vector_imgn, because (:) (in the original matlab code)

    # ref_patch_valid_shape = np.array(ref_patch.shape) - filter_size + 1

    x_end, y_end, z_end = (s - filter_size + 1 for s in ref_patch.shape)
    # ref_paddings = ((0, filter_size),) * 3

    invertible = np.zeros((filter_size**3,) * 2, dtype='float')
    # ref_patch_freq = fftn(ref_patch)

    for pos, (i, j, k) in enumerate(itertools.product(range(filter_size),
                                                      range(filter_size),
                                                      range(filter_size))):
        ref_slice = ref_patch[i:x_end + i,
                              j:y_end + j,
                              k:z_end + k]
        # ref_slice = np.pad(ref_slice, ref_paddings)
        # ref_slice_freq = fftn(ref_slice)

        # product = ref_patch_freq * ref_slice_freq.conj()
        # corr = ifftn(product)[:ref_patch_valid_shape[0],
        #                       :ref_patch_valid_shape[1],
        #                       :ref_patch_valid_shape[2]]
        corr = correlate(ref_patch, ref_slice, mode='valid', method='direct')
        invertible[pos] = corr.flatten()

    inverse_n = np.linalg.inv(invertible)
    x_ti = correlate(ref_patch, vector_img_n, mode='valid').flatten()
    inverse_n_x_ti = inverse_n @ x_ti

    filter_n = inverse_n_x_ti.reshape((filter_size, filter_size, filter_size))
    filter_n = filter_n / np.sum(filter_n)  # normalize to 1.0

    return filter_n


def _optimal_filter_3d_patch_fft_corr(ref_patch: np.ndarray,
                                      img_patch: np.ndarray,
                                      filter_size: int) -> np.ndarray:
    assert ref_patch.shape == img_patch.shape

    # x_n = _im2col(ref_patch, (filter_size, filter_size,
    #                           filter_size))
    # least squares

    if filter_size % 2 != 0:
        raise ValueError(f"filter_size={filter_size} isn't even")

    slice_pos = slice(filter_size // 2 - 1, -filter_size // 2)
    vector_img_n = img_patch[slice_pos, slice_pos, slice_pos]

    # non transposed vector_imgn, because (:) (in the original matlab code)

    x_end, y_end, z_end = tuple(s - filter_size + 1 for s in ref_patch.shape)

    filter_pow_3 = filter_size**3
    invertible = np.zeros((filter_pow_3,) * 2, dtype='float')
    ref_slices = np.zeros((filter_pow_3,) + ref_patch.shape)

    for pos, (i, j, k) in enumerate(itertools.product(range(filter_size),
                                                      range(filter_size),
                                                      range(filter_size))):
        ref_slice = ref_patch[i:x_end + i,
                              j:y_end + j,
                              k:z_end + k]
        ref_slices[pos, :x_end, :y_end, :z_end] = ref_slice

    ref_patch_freq = fftn(ref_patch)
    ref_slices_freq = fftn(ref_slices)

    product = ref_patch_freq * ref_slices_freq.conj()
    corr = ifftn(product)[:, :filter_size,
                          :filter_size,
                          :filter_size]

    invertible = corr.reshape((filter_pow_3, filter_pow_3))

    inverse_n = np.linalg.inv(invertible)
    x_ti = correlate(ref_patch, vector_img_n, mode='valid').flatten()
    inverse_n_x_ti = inverse_n @ x_ti

    filter_n = inverse_n_x_ti.reshape((filter_size, filter_size, filter_size))
    filter_n = filter_n / np.sum(filter_n)  # normalize to 1.0

    return filter_n


def _optimal_filter_3d_patch_im2col(ref_patch: np.ndarray,
                                    img_patch: np.ndarray,
                                    filter_size: int) -> np.ndarray:
    assert ref_patch.shape == img_patch.shape

    t_mat = PROFILER_MAT.start()

    x_n = _im2col(ref_patch, (filter_size, filter_size,
                              filter_size))
    # least squares

    if filter_size % 2 == 0:
        slice_pos = slice(filter_size // 2 - 1, -filter_size // 2)
    else:
        slice_pos = slice(filter_size // 2, -filter_size // 2 + 1)

    vector_img_n = img_patch[slice_pos, slice_pos, slice_pos]

    # non transposed vector_imgn, because (:) (in the original matlab code)
    # flattens in column major order!
    i_n = vector_img_n.flatten()

    invertible = x_n.T @ x_n
    PROFILER_MAT.stop(t_mat)

    t_inv = PROFILER_INV.start()
    inverse_n = np.linalg.inv(invertible)
    PROFILER_INV.stop(t_inv)

    x_ti = x_n.T @ i_n

    inverse_n_x_ti = inverse_n @ x_ti

    filter_n = inverse_n_x_ti.reshape((filter_size, filter_size, filter_size))
    filter_n = filter_n / np.sum(filter_n)  # normalize to 1.0

    return filter_n


FILTER_VARIANT_FUNC = _optimal_filter_3d_patch_im2col


def optimal_filter_3d_patch_shift(ref_patch: np.ndarray,
                                  img_patch: np.ndarray,
                                  interpolation_order: int,
                                  cl_thread=None,
                                  filter_func=None) -> Tuple[float, float]:
    """Computes subpixel shift of two image patches
     via optimal filter method.

    >>> a = np.random.random((30, 30, 30))
    >>> b = np.zeros_like(a)

    >>> b[:-1, :-1, :-1] = (a[:-1, :-1, :-1] + a[1:, 1:, 1:]) / 2

    >>> x_shift, y_shift, z_shift = optimal_filter_3d_patch_shift(a, b, 4)

    >>> round(x_shift, 3)
    -0.5
    >>> round(y_shift, 3)
    -0.5
    >>> round(z_shift, 3)
    -0.5
    """

    # TODO: allow specification of individual filter_size dimensions;
    #        not all three equal!

    filter_size = interpolation_order + 1

    if filter_size % 2 == 0:
        weight = np.arange(-(filter_size // 2 - 1), filter_size // 2 + 1)
    else:
        weight = np.arange(-(filter_size // 2), filter_size // 2 + 1)

    if img_patch.shape != ref_patch.shape:
        raise ValueError(f'Dimensions of reference ({ref_patch.shape}) '
                         f'and image ({img_patch.shape}) patch must be equal!')

    filter_n = FILTER_VARIANT_FUNC(ref_patch, img_patch, filter_size)

    return (-np.sum(filter_n, axis=(1, 2), dtype='float32') @ weight,
            -np.sum(filter_n, axis=(0, 2), dtype='float32') @ weight,
            -np.sum(filter_n, axis=(0, 1), dtype='float32') @ weight)


def _optimal_filter_2d_patch(ref_patch: np.ndarray,
                             img_patch: np.ndarray,
                             filter_size: int) -> np.ndarray:
    assert ref_patch.shape == img_patch.shape

    x_n = _im2col(ref_patch, (filter_size, filter_size))

    # least squares

    if filter_size % 2 != 0:
        raise ValueError(f"filter_size={filter_size} isn't even")

    slice_pos = slice(filter_size // 2 - 1, -filter_size // 2)
    vector_img_n = img_patch[slice_pos, slice_pos]

    # non transposed vector_imgn, because (:) (in the original matlab code)
    # flattens in column major order!
    i_n = vector_img_n.flatten()

    inverse_n = np.linalg.inv(x_n.T @ x_n)
    x_ti = x_n.T @ i_n
    inverse_n_x_ti = inverse_n @ x_ti

    filter_n = inverse_n_x_ti.reshape((filter_size, filter_size))
    filter_n = filter_n / np.sum(filter_n)  # normalize to 1.0

    return filter_n


def optimal_filter_2d_patch_shift(ref_patch: np.ndarray,
                                  img_patch: np.ndarray,
                                  filter_size: int) -> Tuple[float, float]:
    """Computes subpixel shift of two image patches
     via optimal filter method.

    >>> a = np.random.random((30, 30))
    >>> b = np.zeros_like(a)

    >>> b[:-1, :] = (a[:-1, :] + a[1:, :]) / 2

    >>> y_shift, x_shift = optimal_filter_2d_patch_shift(a, b, 4)

    >>> np.round(y_shift, 3)
    -0.5
    >>> abs(np.round(x_shift, 3))
    0.0
    """
    if filter_size % 2 != 0:
        raise ValueError(f"filter_size={filter_size} isn't even")

    weight = np.arange(-(filter_size // 2 - 1), filter_size // 2 + 1)
    filter_n = _optimal_filter_2d_patch(ref_patch, img_patch, filter_size)

    return (-np.sum(filter_n, axis=1) @ weight,
            -np.sum(filter_n, axis=0) @ weight)
