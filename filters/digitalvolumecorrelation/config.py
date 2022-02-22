from enum import Enum
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
from typing import NamedTuple, Tuple
from multiprocessing import cpu_count

import numpy as np


class SubvoxelMode(Enum):
    OPTIMAL_FILTER = 'OptimalFilter'
    DISABLED = 'Disabled'


class CalculationMode(Enum):
    # AUTOMATIC should no longer be present after init!
    AUTOMATIC = 'Automatic'
    CPU = 'Cpu'
    GPU_REIKNA = 'GpuReikna'


IntTriple = Tuple[int, int, int]


class Config(NamedTuple):
    reference_volume: np.ndarray
    deformed_volume: np.ndarray
    kernel_shape: IntTriple
    roi_shape: IntTriple
    stride_shape: IntTriple
    subvoxel_mode: SubvoxelMode
    calculation_mode: CalculationMode
    workers_per_gpu: int
    gpu_count: int
    interpolation_order: int
    map_displacements: bool = False
    std_threshold: float = 0.0


def init(config: Config):
    """Initialize global fixed-variable configuration accordingly to config
      tuple and receive an updated copy of said tuple.

      Can also be queried by calling: config.get() afterwards.

      These values will always be read-only after init!

      Important: in order for config.get() to return anything,
                   init *has* to be called before!
    """

    global _CONFIG_SINGLETON

    gpu_present = _all_gpu_libraries_nominal()

    # update calculation_mode

    calc_mode = config.calculation_mode

    if calc_mode is CalculationMode.AUTOMATIC:
        calc_mode = (CalculationMode.GPU_REIKNA
                     if gpu_present
                     else CalculationMode.CPU)
        config = config._replace(calculation_mode=calc_mode)
    elif calc_mode is CalculationMode.GPU_REIKNA and not gpu_present:
        print('[Err]: GPU mode was selected, '
              'but failed to import or use libraries.')
        _test_gpu_libraries()

    # update gpu_count
    if calc_mode is CalculationMode.GPU_REIKNA:
        gpu_count = _gpu_count()
    else:
        gpu_count = 0  # stays 0 in case of non-GPU mode

    config = config._replace(gpu_count=gpu_count)

    # update workers_per_gpu
    if config.workers_per_gpu == 0:
        # automatic worker count detected!

        if calc_mode is CalculationMode.GPU_REIKNA:
            workers_per_gpu = cpu_count() // gpu_count
        else:
            workers_per_gpu = cpu_count()

        config = config._replace(workers_per_gpu=workers_per_gpu)

    _CONFIG_SINGLETON = config

    return config


def get() -> Config:
    """See config.init()"""
    if _CONFIG_SINGLETON is None:
        raise RuntimeError('"config.get()" was called before "config.init()"!')

    return _CONFIG_SINGLETON


def _test_gpu_libraries():
    import os
    import sys

    # sys.path.append(os.path.join(os.path.dirname(
    #     __file__), '..', 'lib', 'funcsigs-1.0.2'))
    # sys.path.append(os.path.join(os.path.dirname(
    #     __file__), '..', 'lib', 'reikna-0.7.5'))

    from reikna.cluda import ocl_api
    gpus = ocl_api().get_platforms()[0].get_devices(4)

    if not gpus:
        raise RuntimeError('No gpu was found.')


def _all_gpu_libraries_nominal() -> bool:
    try:
        _test_gpu_libraries()
        return True

    except Exception:
        return False


def _gpu_count() -> int:
    from reikna.cluda import ocl_api
    api = ocl_api()

    gpu_magic_number = 4
    gpus = api.get_platforms()[0].get_devices(gpu_magic_number)

    return len(gpus)


_CONFIG_SINGLETON: Config = None
