#!/usr/bin/python3
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

"""dvc.py: An implementation of Digital Volume Correlation."""

__author__ = "Robin Kindler,Pouya Shahidi,Alexander Zeising"
__copyright__ = "Copyright 2021, Germany"

import os
import queue
import threading
import collections
import functools
import itertools
from typing import List, Tuple, Generator, NewType, Union, Optional

import numpy as np
from scipy.ndimage import sobel
from scipy.ndimage.filters import uniform_filter

import digitalvolumecorrelation.profiling as profiling
import digitalvolumecorrelation.config as dvc_config
from .config import SubvoxelMode, CalculationMode
from .correlate.cpu import correlate_np
from .subpixel_optimal_filter.cpu import optimal_filter_3d_patch_shift

DVCTask = NewType('DVCTask', List[Tuple[int, int, int]])
FinishedTask = NewType('FinishedTask', Union[int, Exception])
Progress = NewType('Progress', float)


BATCH_SIZE = 64
GPU_COUNT_OVERRIDE = None


PROFILER_OPT_FILTER = profiling.add_profiler('opt_filter_total')
PROFILER_DVC = profiling.add_profiler('dvc_total', 1 / BATCH_SIZE)
PROFILER_CORR = profiling.add_profiler('dvc_corr', 1 / BATCH_SIZE)
PROFILER_MEAN_STD = profiling.add_profiler('dvc_mean_std', 1 / BATCH_SIZE)


def import_ocl():
    global ocl_api
    global reikna_compile_norm_correlate
    global reikna_compile_optimal_filter

    from reikna.cluda import ocl_api

    from .correlate.gpu import reikna_compile_norm_correlate
    from .subpixel_optimal_filter.gpu import reikna_compile_optimal_filter


def window_mean_std_batched(roi_batch, kernel_shape: tuple):
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


def fast_norm_cross_corr(reikna_norm_correlate: Optional[callable],
                         kernel_batch: np.ndarray,
                         roi_batch: np.ndarray,
                         kernel_shape: Tuple[int, int, int, int]):

    sl = (slice(None),
          slice(0, kernel_shape[1]),
          slice(0, kernel_shape[2]),
          slice(0, kernel_shape[3]))

    # TODO: eliminate temporary copy (used for contiguous memory requirement)?
    kernel_batch = kernel_batch.copy()
    temp_slice = kernel_batch[sl]

    template_mean = np.mean(temp_slice, axis=(1, 2, 3))
    template_std = np.std(temp_slice, axis=(1, 2, 3))

    kernel_batch[sl] = (temp_slice
                        - template_mean[:,
                                        None, None, None])
    kernel_batch[sl] = (temp_slice
                        / template_std[:,
                                       None, None, None])

    template_sum = np.sum(kernel_batch, axis=(1, 2, 3))[:,
                                                        None, None, None]

    # calculate mean and standard deviation

    if reikna_norm_correlate is not None:
        t_mean_std = PROFILER_MEAN_STD.start()

        mean, std, corr = reikna_norm_correlate(roi_batch,
                                                kernel_batch,
                                                kernel_shape)
        PROFILER_MEAN_STD.stop(PROFILER_MEAN_STD.start())
        PROFILER_CORR.stop(t_mean_std)
    else:
        t_mean_std = PROFILER_MEAN_STD.start()
        mean, std = window_mean_std_batched(roi_batch, kernel_shape)
        PROFILER_MEAN_STD.stop(t_mean_std)

        t_corr = PROFILER_CORR.start()
        corr = correlate_np(roi_batch, kernel_batch, kernel_shape)
        PROFILER_CORR.stop(t_corr)

    # normalize the result using mean and standard deviation
    corr = corr / std

    # return cv2.scaleAdd(mean, -template_sum, corr)
    return corr - template_sum * mean


class _DVCWorker:
    def __init__(self, dvc_pool: 'DVCPool', gpu_mapping: 'GPUMapping'):
        self.pool = dvc_pool

        (self.reikna_cl_thread,
         self.reikna_norm_correlate,
         self.reikna_optimal_filter) = gpu_mapping

        self.thread = threading.Thread(target=self._work,
                                       daemon=True)
        self.thread.start()

    def _prepare_batch(self, batch_indices: List[DVCTask],
                       batch_kernel: np.ndarray, batch_roi: np.ndarray,
                       indices: np.ndarray):
        pool = self.pool

        reference_volume = pool.reference_volume
        deformed_volume = pool.deformed_volume

        kernel_x, kernel_y, kernel_z = kernel_shape = pool.kernel_shape

        roi_shape = pool.roi_shape

        batch_pos = 0

        strides = pool.stride_shape
        roi_x, roi_y, roi_z = pool.roi_shape

        x_off, y_off, z_off = ((roi - kernel) // 2
                               for roi, kernel in zip(roi_shape,
                                                      kernel_shape))

        for i, j, k in batch_indices:
            x, y, z = [int(ind * stride)
                       for ind, stride in zip((i, j, k), strides)]

            x_lo, y_lo, z_lo = x + x_off, y + y_off, z + z_off

            subvolume = reference_volume[x_lo:x_lo + kernel_x,
                                         y_lo:y_lo + kernel_y,
                                         z_lo:z_lo + kernel_z]
            vol_shape = subvolume.shape

            # build ROI out of second volume
            # ROI is centered in subvolume and gets cut off in the border areas

            volume_roi = deformed_volume[x:x + roi_x, y:y + roi_y, z:z + roi_z]

            batch_kernel[batch_pos] = 0
            batch_kernel[batch_pos,
                         :vol_shape[0],
                         :vol_shape[1],
                         :vol_shape[2]] = subvolume

            v2_shape = volume_roi.shape

            batch_roi[batch_pos] = 0
            batch_roi[batch_pos,
                      :v2_shape[0],
                      :v2_shape[1],
                      :v2_shape[2]] = volume_roi

            indices[batch_pos] = (((i, j, k),
                                   (x, y, z),
                                   (x_lo, y_lo, z_lo)))

            batch_pos += 1

    def _work(self):
        pool = self.pool
        out_q = pool.output_queue

        try:
            reikna_norm_correlate = self.reikna_norm_correlate
            reikna_optimal_filter = self.reikna_optimal_filter

            q = pool.queue

            batch_range = np.arange(pool.batch_size)

            subv_size = pool.roi_shape

            batch_kernel = np.zeros((pool.batch_size,)
                                    + subv_size,
                                    dtype='float32')
            batch_roi = np.zeros((pool.batch_size,)
                                 + pool.roi_shape,
                                 dtype='float32')
            indices = np.zeros((pool.batch_size,) + (3, 3),
                               dtype='uint16')

            batch_indices: Optional[DVCTask] = None

            std_threshold = pool.std_threshold

            x_stride, y_stride, z_stride = pool.kernel_shape
            kernel_shape = ((pool.batch_size,)
                            + pool.kernel_shape)

            while True:
                batch_indices = q.get()
                batch_len: int = len(batch_indices)

                # loads next batch into the previously created numpy arrays
                self._prepare_batch(batch_indices, batch_kernel,
                                    batch_roi, indices)

                # 3D correlation of actual subvolume of v1 and actual ROI of v2

                t_corr_start = PROFILER_DVC.start()
                corr = fast_norm_cross_corr(reikna_norm_correlate,
                                            batch_kernel, batch_roi,
                                            kernel_shape)
                PROFILER_DVC.stop(t_corr_start)

                # find the index of the max. correlation value.
                # This is the position within the subvolume
                maxima = corr.reshape(corr.shape[0], -1).argmax(1)
                maxima = np.unravel_index(maxima, corr.shape[1:])

                corr_max = corr[(batch_range,) + maxima]

                results = np.column_stack(maxima)
                results = results.astype('float32')

                # set every individual result with std below threshold to NaN
                # only AFTER everything has been calculated,
                #  to avoid possible unforeseen side-effects
                if std_threshold:
                    std_kernel = np.std(batch_kernel, axis=(1, 2, 3))
                    below = std_kernel <= std_threshold
                    corr_max[below] = np.nan
                    results[below] = np.nan

                # TODO: implement batched OptimalFilter
                if pool.subvoxel_mode is SubvoxelMode.OPTIMAL_FILTER:
                    for pos in range(pool.batch_size):

                        if std_threshold:
                            if below[pos]:
                                continue

                        res = results[pos]

                        x_lo = int(res[0])
                        x_hi = x_lo + x_stride

                        y_lo = int(res[1])
                        y_hi = y_lo + y_stride

                        z_lo = int(res[2])
                        z_hi = z_lo + z_stride

                        roi_crop = batch_roi[pos,
                                             x_lo:x_hi,
                                             y_lo:y_hi,
                                             z_lo:z_hi]
                        r_shape = roi_crop.shape

                        kernel = batch_kernel[pos, :r_shape[0],
                                              :r_shape[1],
                                              :r_shape[2]]

                        t_opt = PROFILER_OPT_FILTER.start()
                        if pool.calculation_mode is CalculationMode.CPU:
                            subvoxel_offsets = optimal_filter_3d_patch_shift(
                                kernel, roi_crop, pool.interpolation_order
                            )
                        else:
                            subvoxel_offsets = reikna_optimal_filter(
                                kernel,
                                roi_crop
                            )
                        PROFILER_OPT_FILTER.stop(t_opt)

                        results[pos] += subvoxel_offsets

                subvolume_indices, split_indices, offsets = (indices[:, 0],
                                                             indices[:, 1],
                                                             indices[:, 2])

                # displacements = results + global positions
                displacements = results - offsets + split_indices

                d_vec = np.column_stack(
                    (displacements, corr_max, split_indices)
                )

                # Finally, assign the results!
                # (batch_len of the last chunk of work is usually
                #  shorter than BATCH_SIZE; hence the indexing for this case)

                pool.vector_list[subvolume_indices[:batch_len, 0],
                                 subvolume_indices[:batch_len, 1],
                                 subvolume_indices[:batch_len, 2],
                                 :] = d_vec[:batch_len]

                out_q.put(FinishedTask(batch_len))

        except Exception as e:
            out_q.put(FinishedTask(e))


class DVCPool:
    """A worker pool/controller of gpu accelerated DVC calculating workers,
        holding global state for each worker.
    """

    def __init__(self, batch_size: int,
                 config: dvc_config.Config):
        self.batch_size = batch_size

        self.reference_volume = config.reference_volume
        self.deformed_volume = config.deformed_volume

        self.kernel_shape = config.kernel_shape
        self.roi_shape = config.roi_shape
        self.stride_shape = config.stride_shape
        self.subvoxel_mode = config.subvoxel_mode
        self.calculation_mode = config.calculation_mode
        self.std_threshold = config.std_threshold
        self.interpolation_order = config.interpolation_order

        self.workers: List[_DVCWorker] = []
        self.worker_count: int = 0
        self._slice_counter: int = 0

        self.queue: "Queue[DVCTask]" = queue.Queue()
        self.output_queue: "Queue[FinishedTask]" = queue.Queue()

        # contains reference to reikna.cluda.ocl, in case it could be loaded!
        self.ocl_api = None

        if self.calculation_mode is CalculationMode.GPU_REIKNA:
            # maps gpu devices to cl threads/queues/fft_funcs/etc.
            import_ocl()
            self.gpu_devices: list = self._list_gpus()
            self._gpu_cycle_it = itertools.cycle(self.gpu_devices)
        else:
            self.gpu_devices: list = []
            self._gpu_cycle_it = None

        v_shape = self.v_shape = np.array(config.reference_volume.shape)
        strides = np.array(config.stride_shape)

        self.amount_of_subvolumes = (np.ceil((v_shape - config.roi_shape + 1)
                                             / strides).astype('uint32'))

        self.total_slices = int(np.prod(self.amount_of_subvolumes))

        # Create a dummy list to be filled with the displacement vectors
        # 4th axis will contain:
        #   3 elements for displacement vector, correlation max,
        #   3 elements for split index

        self.vector_list = np.zeros(tuple(self.amount_of_subvolumes.tolist())
                                    + (7,))

    def add_task(self, batch_indices: DVCTask):
        self.queue.put_nowait(batch_indices)

    def _dvc_task_generator(self) -> Generator[DVCTask, None, None]:
        index_iter = itertools.product(*(range(r)
                                         for r in self.amount_of_subvolumes))

        for index_chunk in chunked(index_iter, self.batch_size):
            yield DVCTask(index_chunk)

    def run_with_progress(self) -> Generator[Progress, None, None]:
        if self.worker_count == 0:
            raise RuntimeError('run_with_progress called'
                               ' with worker_count == 0')

        def fill_task_queue():
            for dvc_task in self._dvc_task_generator():
                self.add_task(dvc_task)

        thread = threading.Thread(target=fill_task_queue)
        thread.daemon = True
        thread.start()

        # print('total_slices:', self.total_slices, flush=True)

        while True:
            result: FinishedTask = self.output_queue.get()

            if isinstance(result, int):
                self._slice_counter += result
            else:
                raise result

            if self.is_done():
                return self.progress()

            yield self.progress()

    def progress(self) -> Progress:
        return Progress(self._slice_counter / self.total_slices)

    def result(self) -> np.ndarray:
        if not self.is_done():
            raise RuntimeError("DVCPool.result() got called "
                               "while the workers weren't done working.")

        return self.vector_list

    def is_done(self) -> bool:
        return self._slice_counter >= self.total_slices

    def create_worker(self, device=None) -> _DVCWorker:
        GPUMapping = collections.namedtuple('GPUMapping',
                                            ['cl_thread',
                                             'norm_correlate_kernel',
                                             'subvoxel_kernel'])

        # workers need to be created before letting the first one start!
        self.worker_count += 1

        if self.calculation_mode is CalculationMode.GPU_REIKNA:
            if device is None:
                device = next(self._gpu_cycle_it)

            roi_batch_shape = (self.batch_size,) + self.roi_shape
            kernel_batch_shape = (self.batch_size,) + self.kernel_shape

            # TODO: create thread and compile INSIDE
            # target worker thread to alleviate possible congestion/scaling
            # problem
            reikna_cl_thread = self.ocl_api.Thread(device)

            reikna_norm_correlate = reikna_compile_norm_correlate(
                reikna_cl_thread,
                roi_batch_shape,
                kernel_batch_shape
            )

            if self.subvoxel_mode is SubvoxelMode.OPTIMAL_FILTER:
                reikna_optimal_filter = reikna_compile_optimal_filter(
                    reikna_cl_thread,
                    self.kernel_shape,
                    self.interpolation_order)
            else:
                reikna_optimal_filter = None

            mapping = GPUMapping(reikna_cl_thread,
                                 reikna_norm_correlate,
                                 reikna_optimal_filter)

        else:
            mapping = (None,) * 3

        w = _DVCWorker(self, mapping)
        self.workers.append(w)

        return w

    def _increment_batch_counter(self, batch_size: int):
        self._slice_counter += batch_size

    def _list_gpus(self) -> list:
        GPU = 4

        self.ocl_api = api = ocl_api()

        # TODO: maybe check all platforms for unique devices?
        gpus = api.get_platforms()[0].get_devices(GPU)
        return gpus


def take(n: int, iterable):
    return list(itertools.islice(iterable, n))


def chunked(iterable, n: int):
    return iter(functools.partial(take, n, iterable), [])


def correlate(op, config: dvc_config.Config):
    """
    Correlares a split voxel volume with an un-split voxel volume via
        Fourier analysis and returns an array of displacement vectors.
        Internally: Calculates a related Region of Interest (ROI)
        for each element of the subvolume List and normalizes
        the ROI and the element. Second step is the correlation.

    :param volume: Reference voxel volume.
    :param volume2: Modified voxel volume as volume data array.
    :param roi_shape: Shape (Size) of Region of interest.
        The lower the higher the performance. Big ROIs can cause
        false correlations (especially on symmetrical
         or otherwise similar objects).
    :return: 3D Numpy Array of displacement vector objects
        (objects containing displacement vector and
        position of the split index of the related subvolume)
    """

    assert config.reference_volume.shape == config.deformed_volume.shape

    dvc_pool = DVCPool(batch_size=BATCH_SIZE, config=config)

    gpu_count = config.gpu_count
    if GPU_COUNT_OVERRIDE:
        gpu_count = GPU_COUNT_OVERRIDE

    if dvc_pool.calculation_mode is CalculationMode.GPU_REIKNA:
        for gpu_id in dvc_pool.gpu_devices[:gpu_count]:
            for _ in range(config.workers_per_gpu):
                _w = dvc_pool.create_worker(gpu_id)
    else:
        for _ in range(config.workers_per_gpu):
            _w = dvc_pool.create_worker()

    for progress in dvc_pool.run_with_progress():
        op.ThrowIfCancelled()
        op.SetProgress(progress)

    # Return the np.ndarray of displacement vector objects.
    return dvc_pool.result()


def map_displacement(subvolume_disp_arr, v1, kernel_shape):
    """
    Rebuilds one big voxel volume out of voxel (sub)volume array.
    :param subvolume_disp_arr: the array with subvolumes' displacement objects
    :param v1: voxel volume to map the displacementArray on
    :param kernel_shape: tuple containing the shape of
                            the correlation window (x,y,z)
    """
    x, y, z = kernel_shape

    # size of voxel volume data
    amount_x = subvolume_disp_arr.shape[0]
    amount_y = subvolume_disp_arr.shape[1]
    amount_z = subvolume_disp_arr.shape[2]

    # prototype of displacement array
    disp_array = np.empty(v1.shape + (4,))

    for i in range(amount_x):
        for j in range(amount_y):
            for k in range(amount_z):
                s = subvolume_disp_arr[i, j, k, 4:]
                value = subvolume_disp_arr[i, j, k, 0:4]
                disp_array[s[0]:s[0] + x, s[1]:s[1] + y, s[2]:s[2] + z] = value

    return disp_array


def dvc(op, config: dvc_config.Config):
    """
    Performs the Digital Volume Correlation for two voxel volumes.

    :param reference_volume
    :param deformed_volume
    :param kernel_shape: Shape tuple of correlation window (x,y,z)
    :param roi_shape: Shape tuple of region of interest
    :param op: This voxie operation
    :param stride_shape: tupel containing the stride sizes (x,y,z)
    """
    # Handle some errors
    if config.reference_volume.shape != config.deformed_volume.shape:
        raise ValueError(
            "DVC error: Voxel volumes V1 and V2 must have the same shape.")

    # DVC step one: Preprocessing
    #############################

    py_spy_path = os.environ.get('DVC_PYSPY_PATH', None)
    if py_spy_path and os.path.isfile(py_spy_path):
        import subprocess
        pid = os.getpid()

        filename = 'profile_dvc.svg'

        subprocess.Popen([py_spy_path, 'record',
                          '--output', filename,
                          '--pid', str(pid),
                          '--rate', '100'])

    # DVC step two: Fourier based correlation
    ########################################

    # Calculate ROI, normalize values
    # and correlate each subvolume of v1 with ROI of v2.
    # Returns the resulting displacement vectors it in a numpy 3d array.

    local_displacement_array_and_split_indeces = correlate(op, config)

    # DVC step three: Mapping of displacementArray
    ###########################################

    # TODO: add strain_tensor results back in

    if config.map_displacements:
        result = map_displacement(
            local_displacement_array_and_split_indeces,
            config.reference_volume,
            config.kernel_shape)
    else:
        result = local_displacement_array_and_split_indeces[:, :, :, :4]

    return result


def strain_tensor(correlation_result):
    x = correlation_result[:, :, :, 0]
    y = correlation_result[:, :, :, 1]
    z = correlation_result[:, :, :, 2]

    dud_x = sobel(x, axis=0, mode='constant')
    dvd_x = sobel(x, axis=1, mode='constant')
    dwd_x = sobel(x, axis=2, mode='constant')

    dud_y = sobel(y, axis=0, mode='constant')
    dvd_y = sobel(y, axis=1, mode='constant')
    dwd_y = sobel(y, axis=2, mode='constant')

    dud_z = sobel(z, axis=0, mode='constant')
    dvd_z = sobel(z, axis=1, mode='constant')
    dwd_z = sobel(z, axis=2, mode='constant')

    exx = dud_x + (dud_x ** 2 + dvd_x**2 + dwd_x ** 2) / 2
    eyy = dvd_y + (dud_y ** 2 + dvd_y**2 + dwd_y ** 2) / 2
    ezz = dwd_z + (dud_z ** 2 + dvd_z**2 + dwd_z ** 2) / 2

    exy = (dud_y + dvd_x) / 2 + (dud_x * dud_y
                                 + dvd_x * dvd_y
                                 + dwd_x * dwd_y) / 2
    exz = (dud_z + dwd_x) / 2 + (dud_x * dud_z
                                 + dvd_x * dvd_z
                                 + dwd_x * dwd_z) / 2
    eyz = (dwd_y + dvd_z) / 2 + (dud_z * dud_y
                                 + dvd_z * dvd_y
                                 + dwd_z * dwd_y) / 2

    return exx, eyy, ezz, exy, exz, eyz
