"""A module for simple deterministic, profiling."""
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
import pprint
from typing import Optional
from time import perf_counter

from threading import Lock

_PROFILERS = dict()


class Profiler:

    def __init__(self, averaging_factor=1.0):
        self.averaging_factor: float = averaging_factor

        self.elapsed_time: float = 0.0
        self.times_called: int = 0
        self._lock = Lock()

    __slots__ = 'elapsed_time', 'times_called', 'averaging_factor', '_lock'

    def start(self):
        return perf_counter()

    def stop(self, start_time: float):
        now = perf_counter()

        with self._lock:
            self.elapsed_time += now - start_time
            self.times_called += 1

    def average_time(self, averaging_factor: float = None) -> Optional[float]:
        if self.times_called == 0:
            return None

        if averaging_factor is None:
            averaging_factor = self.averaging_factor

        return (self.elapsed_time / self.times_called) * averaging_factor


def add_profiler(profiler_name: str, averaging_factor=1.0) -> Profiler:
    if profiler_name in _PROFILERS:
        raise ValueError(f'Profiler with name "{profiler_name}" '
                         f'already in use!')

    profiler = Profiler(averaging_factor)
    _PROFILERS[profiler_name] = profiler

    return profiler


def summarize() -> dict:
    return {name: dict(avg_time=profiler.average_time(),
                       total_elapsed_time=profiler.elapsed_time,
                       times_called=profiler.times_called,
                       avg_factor=profiler.averaging_factor)
            for name, profiler in _PROFILERS.items()}


def pprint_summary():
    print('Profiling summary:', flush=True)
    pp = pprint.PrettyPrinter(indent=4)
    pp.pprint(summarize())
