from pathlib import Path
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

import numpy as np
import matplotlib
import matplotlib.pyplot as plt

FFT_SIZE = 32
SEP = '\n'
PATH_TEST = Path.cwd() / 'filters/digitalvolumecorrelation/perftest/fftgputest'
PATH_TIMING_DATA = PATH_TEST / f'timing_data/{FFT_SIZE}'
PATH_FIGURE = PATH_TEST / f'fft_runtime_plot_{FFT_SIZE}.eps'

COLORS = ('#7570b3', '#1b9e77', '#d95f02')
MODULE_NAMES = ('gpu_reikna', 'gpu_gpyfft', 'cpu')


def main():
    matplotlib.rcParams['figure.figsize'] = [8, 8]

    datasets = []

    for module_name in MODULE_NAMES:

        test_name = module_name + '_test_'

        filepaths = sorted(PATH_TIMING_DATA.glob(f'profile_{module_name}*'),
                           key=lambda p: p.name.split(test_name)[1]
                           .split('_')[0].zfill(4))
        data = [np.fromfile(str(p), sep=SEP) for p in filepaths]
        datasets.append(data)

    # means = np.mean(datasets, axis=2)
    # for i in range(means.shape[1]):
    #     reikna = means[0, i]
    #     cpu = means[2, i]

    #     print(2**i, cpu/reikna)

    axes = plt.subplot()

    sizes = [p.name.split(test_name)[1].split('_')[0]
             for p in filepaths]

    bp1 = axes.boxplot(datasets[0],
                       positions=np.arange(len(datasets[0])) * 2.0 - 0.6,
                       widths=0.6, sym='',
                       showbox=False, showcaps=False, showfliers=False)
    bp2 = axes.boxplot(datasets[1],
                       positions=np.arange(len(datasets[1])) * 2.0,
                       widths=0.6, sym='',
                       showbox=False, showcaps=False, showfliers=False)
    bp3 = axes.boxplot(datasets[2],
                       positions=np.arange(len(datasets[2])) * 2.0 + 0.6,
                       widths=0.6, sym='',
                       showbox=False, showcaps=False, showfliers=False)

    def color_box_plot(bp, color):
        plt.setp(bp['boxes'], color=color)
        plt.setp(bp['whiskers'], color=color)
        plt.setp(bp['medians'], color=color)
        plt.setp(bp['caps'], color=color)

    for bp, color in zip((bp1, bp2, bp3), COLORS):
        color_box_plot(bp, color)

    axes.set_yscale('log')
    axes.set_yticks([0.00001, 0.001, 0.01, 0.1, 1, 10, 10])
    # axes.get_yaxis().set_major_formatter(matplotlib.ticker.ScalarFormatter())
    axes.get_yaxis().set_major_formatter(
        matplotlib.ticker.StrMethodFormatter('{x:,.3f}'))
    axes.set_ylim([0.0001, 100.0])
    axes.set_ylabel('runtime in s')
    axes.set_xlabel('batch-size')
    axes.set_xticklabels(sizes)
    axes.set_xticks(range(0, len(sizes) * 2, 2))
    # axes.set_title(f'Batched 3D FFT of size {FFT_SIZE} runtime comparison')

    for module, color in zip(MODULE_NAMES, COLORS):
        axes.plot([], c=color, label=module.replace('_', '-'))

    axes.legend()

    plt.tight_layout()
    plt.savefig(str(PATH_FIGURE), transparent=True)
    plt.show()


if __name__ == '__main__':
    main()
