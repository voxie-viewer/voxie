#!/usr/bin/python3

import os
import sys
import json
import numbers
import io
import signal
import tkinter
import tkinter.scrolledtext

import numpy as np
from matplotlib import pyplot as plt
import matplotlib.widgets

sys.path.append(os.path.dirname(__file__) + '/../pythonlib')

if True:
    import voxie
    import voxie.tcl_loop_wakeup


def get_readonly_copy(a, *, dtype=None):
    copy = np.array(a)
    copy.flags.writeable = False
    return copy


class BernsteinPolynomial:
    def __init__(self, dtype, coefficients):
        self.dtype = np.dtype(dtype)

        self.coefficients = get_readonly_copy(coefficients, dtype=self.dtype)

        self.degree = self.coefficients.shape[0] - 1
        if self.degree < 0:
            raise Exception('self.degree < 0')

        def expect(name, shape):
            if getattr(self, name).shape != shape:
                raise Exception('Unexpected shape for {}: Expected {}, got {}'.format(name, shape, getattr(self, name).shape))
        expect('coefficients', (self.degree + 1,))

    @staticmethod
    def zero(degree=0, dtype=np.dtype('f8')):
        coefficients = np.zeros((degree + 1), dtype=dtype)

        return BernsteinPolynomial(dtype=dtype, coefficients=coefficients)

    def __call__(self, values, /):
        t = values
        not_t = 1 - t

        # https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm
        entries = np.array(self.coefficients)
        size = self.degree + 1
        while size > 1:
            size -= 1
            for i in range(size):
                entries[i] = entries[i] * not_t + entries[i + 1] * t
        return entries[0]

    def with_coefficient(self, point, value):
        new_coefficients = np.array(self.coefficients)
        new_coefficients[point] = value
        return BernsteinPolynomial(dtype=self.dtype, coefficients=new_coefficients)

    def increase_degree(self):
        new_coefficients = np.zeros(self.coefficients.shape[0] + 1, dtype=self.dtype)
        for i in range(new_coefficients.shape[0]):
            if i == 0:
                new_coefficients[i] = self.coefficients[i]
            elif i == self.coefficients.shape[0]:
                new_coefficients[i] = self.coefficients[i - 1]
            else:
                t = i / self.coefficients.shape[0]
                new_coefficients[i] = (1 - t) * self.coefficients[i] + t * self.coefficients[i - 1]
        return BernsteinPolynomial(self.dtype, new_coefficients)

    # TODO: Clean up
    def decrease_degree(self):
        new_coefficients = np.zeros(self.coefficients.shape[0] - 1, dtype=self.dtype)

        # This will attempt to make sure that when incDegree() is called again the coefficients stay the same, except in the very middle

        for i in range((new_coefficients.shape[0] + 1) // 2):
            # Set coefficients left=i and right=max-i
            left = i
            right = new_coefficients.shape[0] - 1 - i
            # print(left, right)

            if i == 0:
                left_val = self.coefficients[0]
                right_val = self.coefficients[-1]
            else:
                t = 1 / (new_coefficients.shape[0] - 1)
                left_val = (1 + t) * self.coefficients[i] - t * new_coefficients[i - 1]
                right_val = (1 + t) * self.coefficients[self.coefficients.shape[0] - 1 - i] - t * new_coefficients[i + 1]
                # print(i, t, left_val, right_val)

            if left == right:
                avg = 0.5 * (left_val + right_val)
                new_coefficients[left] = avg
            else:
                new_coefficients[left] = left_val
                new_coefficients[right] = right_val

        return BernsteinPolynomial(self.dtype, new_coefficients)

    def split(self, t):
        not_t = 1 - t

        # https://en.wikipedia.org/wiki/De_Casteljau%27s_algorithm
        entries = list(self.coefficients)
        size = self.degree + 1
        coeff0 = []
        coeff1 = []
        coeff0.append(entries[0])
        coeff1.insert(0, entries[size - 1])
        while len(entries) > 1:
            size -= 1
            new_entries = []
            for index in range(len(entries) - 1):
                new_entries.append(entries[index] * not_t + entries[index + 1] * t)
            entries = new_entries
            coeff0.append(entries[0])
            coeff1.insert(0, entries[size - 1])

        print(coeff0, coeff1)
        print(entries[0])
        return BernsteinPolynomial(dtype=self.dtype, coefficients=coeff0), entries[0], BernsteinPolynomial(dtype=self.dtype, coefficients=coeff1),


class PiecewisePolynomialFunction:
    def __init__(self, dtype, breakpoints, breakpointValues, breakpointIntegrals, polynomials):
        self.dtype = np.dtype(dtype)

        self.breakpoints = get_readonly_copy(breakpoints, dtype=self.dtype)
        self.breakpointValues = get_readonly_copy(breakpointValues, dtype=self.dtype)
        self.breakpointIntegrals = get_readonly_copy(breakpointIntegrals, dtype=self.dtype)

        self.polynomials = tuple(polynomials)

        self.breakpointCount = self.breakpoints.shape[0]

        def expect(name, shape):
            if getattr(self, name).shape != shape:
                raise Exception('Unexpected shape for {}: Expected {}, got {}'.format(name, shape, getattr(self, name).shape))
        expect('breakpoints', (len(self.polynomials) - 1,))
        expect('breakpointValues', (self.breakpointCount,))
        expect('breakpointIntegrals', (self.breakpointCount,))

    @staticmethod
    def zero(degree=0, dtype=np.dtype('f8')):
        breakpoints = np.array([], dtype=dtype)
        breakpointValues = np.array([], dtype=dtype)
        breakpointIntegrals = np.array([], dtype=dtype)

        polynomials = [BernsteinPolynomial.zero(degree=degree, dtype=dtype)]

        return PiecewisePolynomialFunction(dtype=dtype, breakpoints=breakpoints, breakpointValues=breakpointValues, breakpointIntegrals=breakpointIntegrals, polynomials=polynomials)

    def intervalStart(self, i, /):
        if not isinstance(i, numbers.Integral):
            raise Exception('Index must be an integral number')
        i = int(i)

        if i < 0 or i > len(self.breakpoints):
            raise Exception('Index out of range')

        if len(self.breakpoints) == 0:
            return 0
        if i == 0:
            return self.breakpoints[0] - 1
        return self.breakpoints[i - 1]

    def intervalEnd(self, i, /):
        if not isinstance(i, numbers.Integral):
            raise Exception('Index must be an integral number')
        i = int(i)

        if i < 0 or i > len(self.breakpoints):
            raise Exception('Index out of range')

        if len(self.breakpoints) == 0:
            return 1
        if i == len(self.breakpoints):
            return self.breakpoints[-1] + 1
        return self.breakpoints[i]

    def intervalLen(self, i, /):
        return self.intervalEnd(i) - self.intervalStart(i)

    def findInterval(self, pos):
        pos = self.dtype.type(pos)

        intervalIndex = 0
        while intervalIndex < len(self.breakpoints) and pos >= self.breakpoints[intervalIndex]:
            if pos == self.breakpoints[intervalIndex]:
                return self
            intervalIndex += 1

        return intervalIndex

    def __call__(self, values, /):
        return np.vectorize(self.call_once)(values)

    def call_once(self, pos, /):
        # TODO: Performance
        # TODO: Allow multiple values at once?

        i = 0
        while i < len(self.breakpoints) and pos >= self.breakpoints[i]:
            if pos == self.breakpoints[i]:
                return self.breakpointValues[i]
            i += 1

        start = self.intervalStart(i)
        end = self.intervalEnd(i)
        t = (pos - start) / (end - start)
        return self.polynomials[i](t)

    def with_polynomials(self, polynomials):
        return PiecewisePolynomialFunction(self.dtype, self.breakpoints, self.breakpointValues, self.breakpointIntegrals, polynomials)

    def with_polynomial(self, interval, polynomial):
        polynomials = list(self.polynomials)
        polynomials[interval] = polynomial
        return self.with_polynomials(polynomials)

    def with_interval_coefficient(self, interval, point, value):
        return self.with_polynomial(interval, self.polynomials[interval].with_coefficient(point, value))

    # TODO: When the first, last or only interval is split, the coefficients of the new outer interval(s) have to be adjusted
    def addBreakpoint(self, pos):
        pos = self.dtype.type(pos)

        intervalIndex = 0
        while intervalIndex < len(self.breakpoints) and pos >= self.breakpoints[intervalIndex]:
            if pos == self.breakpoints[intervalIndex]:
                return self
            intervalIndex += 1

        start = self.intervalStart(intervalIndex)
        end = self.intervalEnd(intervalIndex)
        t = (pos - start) / (end - start)
        poly1, val_at_split, poly2 = self.polynomials[intervalIndex].split(t)

        newBreakpoint = pos
        newBreakpointValue = val_at_split
        newBreakpointIntegral = 0

        print([intervalIndex, self.breakpoints[:intervalIndex], [newBreakpoint], self.breakpoints[intervalIndex:]])
        newBreakpoints = np.concatenate([self.breakpoints[:intervalIndex], [newBreakpoint], self.breakpoints[intervalIndex:]])
        newBreakpointValues = np.concatenate([self.breakpointValues[:intervalIndex], [newBreakpointValue], self.breakpointValues[intervalIndex:]])
        newBreakpointIntegrals = np.concatenate([self.breakpointIntegrals[:intervalIndex], [newBreakpointIntegral], self.breakpointIntegrals[intervalIndex:]])

        new_polynomials = self.polynomials[:intervalIndex] + (poly1, poly2) + self.polynomials[intervalIndex + 1:]

        return PiecewisePolynomialFunction(self.dtype, newBreakpoints, newBreakpointValues, newBreakpointIntegrals, new_polynomials)

    # TODO: inf/nan
    def asJson(self):
        bp = []
        for i in range(self.breakpointCount):
            bp.append([self.breakpoints[i], [self.breakpointValues[i], self.breakpointIntegrals[i]]])
        intervals = []
        for i in range(self.breakpointCount + 1):
            intervals.append(list(self.polynomials[i].coefficients))
        return [bp, intervals]

    # TODO: inf/nan
    @staticmethod
    def fromJson(json):
        dtype = np.dtype('f8')

        bp, intervals = voxie.json_util.expect_array_with_size(json, 2)
        bp = voxie.json_util.expect_array(bp)

        breakpoints = []
        breakpointValues = []
        breakpointIntegrals = []
        for entry in bp:
            pos, vals = voxie.json_util.expect_array_with_size(entry, 2)
            pos = voxie.json_util.expect_float(pos)
            value, integral = voxie.json_util.expect_array_with_size(vals, 2)
            value = voxie.json_util.expect_float(value)
            integral = voxie.json_util.expect_float(integral)

            breakpoints.append(pos)
            breakpointValues.append(value)
            breakpointIntegrals.append(integral)

        intervals = voxie.json_util.expect_array_with_size(intervals, len(breakpoints) + 1)

        polynomials = []
        for entry in intervals:
            data = voxie.json_util.expect_array(entry)
            for val in data:
                voxie.json_util.expect_float(val)
            polynomials.append(BernsteinPolynomial(dtype=dtype, coefficients=data))

        return PiecewisePolynomialFunction(dtype=dtype, breakpoints=breakpoints, breakpointValues=breakpointValues, breakpointIntegrals=breakpointIntegrals, polynomials=polynomials)


def tryParseFloat(s):
    try:
        return float(s)
    except Exception:
        return None


# See https://stackoverflow.com/questions/50439506/dragging-points-in-matplotlib-interactive-plot/50477532#50477532
class GUI:
    def update(self, *, updateJsonTextField=True, updateCurrentIntervalEntry=True, updateCoeffEntries=True):
        f = io.StringIO()
        json.dump(self.func.asJson(), f, allow_nan=False, sort_keys=True, ensure_ascii=False, indent=2)
        # print(f.getvalue())
        if updateJsonTextField:
            # Hack to set content of widget without changing position of scroll bar
            lines = f.getvalue().split('\n')
            self.scrolledText.insert('end', '\n' * len(lines))
            for nr, line in enumerate(lines):
                lpos = '{}.0'.format(nr + 1)
                lpose = '{}.end'.format(nr + 1)
                self.scrolledText.delete(lpos, lpose)
                self.scrolledText.insert(lpos, line)
            posend = '{}.0'.format(len(lines) + 1)
            self.scrolledText.delete(posend, 'end')
            # old = (0.6363636363636364, 1.0)
            # self.scrolledText.vbar.set(*old)

        if updateCurrentIntervalEntry:
            self.currentIntervalEntryStringVar.set('{}'.format(self.currentInterval))

        if updateCoeffEntries:
            for i in range(self.maxDegree + 1):
                if i > self.func.polynomials[self.currentInterval].degree:
                    if self.coeffEntries[i].get() == '':
                        continue
                    self.coeffEntries[i].progChangeRunning = True
                    self.coeffEntries[i].set('')
                    self.coeffEntries[i].progChangeRunning = False
                else:
                    if tryParseFloat(self.coeffEntries[i].get()) == self.func.polynomials[self.currentInterval].coefficients[i]:
                        continue
                    self.coeffEntries[i].progChangeRunning = True
                    self.coeffEntries[i].set('{}'.format(self.func.polynomials[self.currentInterval].coefficients[i]))
                    self.coeffEntries[i].progChangeRunning = False

        self.intervalCountLabel['text'] = '{}'.format(self.func.breakpointCount + 1)

        x, y = self.get_points(self.func)

        # Pad to self.maxDegree + 1, repeat last element (will not be visible)
        x = np.pad(x, (0, self.maxDegree + 1 - x.shape[0],), mode='edge')
        y = np.pad(y, (0, self.maxDegree + 1 - y.shape[0],), mode='edge')

        self.l.set_xdata(x)
        self.l.set_ydata(y)

        self.m.set_ydata(self.func(self.X))

        # redraw canvas while idle
        self.fig.canvas.draw_idle()

    def button_press_callback(self, event):
        'whenever a mouse button is pressed'
        if event.inaxes is None:
            return

        if event.button == 3:
            t = self.ax1.transData.inverted()
            tinv = self.ax1.transData
            x, y = t.transform([event.x, event.y])
            print(x, y)
            y_actual = self.func(x)
            # print(x, y, y - y_actual)
            if y - y_actual < 0.2:
                self.func = self.func.addBreakpoint(x)
                self.update()
            return

        if event.button != 1:
            return

        # print(self.pind)
        self.pind = self.get_ind_under_point(event)
        if self.pind is not None and self.pind > self.func.polynomials[self.currentInterval].degree:
            # TODO: This is currently needed for degree = 0
            self.pind = self.func.polynomials[self.currentInterval].degree
        if self.pind is None:
            t = self.ax1.transData.inverted()
            tinv = self.ax1.transData
            x, y = t.transform([event.x, event.y])
            self.currentInterval = self.func.findInterval(x)
            self.update(updateJsonTextField=False)

    def button_release_callback(self, event):
        'whenever a mouse button is released'
        if event.button != 1:
            return
        self.pind = None

    # TODO: Should this be stored in the class?
    def get_points(self, func):
        interval = self.currentInterval
        if func.polynomials[interval].degree == 0:
            y = np.array(func.polynomials[interval].coefficients[np.array([0, -1])])
            x = np.array([func.intervalStart(interval), func.intervalEnd(interval)])
        else:
            y = np.array(func.polynomials[interval].coefficients)
            x = np.linspace(func.intervalStart(interval), func.intervalEnd(interval), func.polynomials[interval].degree + 1)
        return x, y

    def get_ind_under_point(self, event):
        'get the index of the vertex under point if within epsilon tolerance'

        # display coords
        # print('display x is: {0}; display y is: {1}'.format(event.x, event.y))
        t = self.ax1.transData.inverted()
        tinv = self.ax1.transData
        xy = t.transform([event.x, event.y])
        # print('data x is: {0}; data y is: {1}'.format(xy[0],xy[1]))

        x, y = self.get_points(self.func)
        xr = np.reshape(x, (np.shape(x)[0], 1))
        yr = np.reshape(y, (np.shape(y)[0], 1))

        xy_vals = np.append(xr, yr, 1)
        xyt = tinv.transform(xy_vals)
        xt, yt = xyt[:, 0], xyt[:, 1]
        d = np.hypot(xt - event.x, yt - event.y)
        indseq, = np.nonzero(d == d.min())
        ind = indseq[0]

        # print(d[ind])
        if d[ind] >= self.epsilon:
            ind = None

        # print(ind)
        return ind

    def motion_notify_callback(self, event):
        'on mouse movement'
        if self.pind is None:
            return
        if event.inaxes is None:
            return
        if event.button != 1:
            return

        if self.pind > self.func.polynomials[self.currentInterval].degree:
            # TODO: ?
            return

        # update values
        # print('motion x: {0}; y: {1}'.format(event.xdata,event.ydata))
        # print(self.pind, event.ydata)
        self.func = self.func.with_interval_coefficient(self.currentInterval, self.pind, event.ydata)

        # self.fig.canvas.draw_idle()
        self.update()

    def init_fun(self):
        self.currentInterval = 0
        self.func = PiecewisePolynomialFunction.zero()

    def reset(self):
        print('reset')
        self.init_fun()
        self.update()

    def run(self):
        root = tkinter.Tk()
        root.wm_title("Piecewise polynomial function editor")

        self.init_fun()

        self.maxDegree = 3
        xmin = -5
        xmax = 5
        ymin = -10
        ymax = 10

        # figure.subplot.right
        # matplotlib.rcParams['figure.subplot.right'] = 0.8

        # set up a plot
        self.fig = plt.figure(figsize=(9.0, 8.0))
        axes = self.fig.subplots(1, 1, sharex=True)
        self.ax1 = axes

        canvas = matplotlib.backends.backend_tkagg.FigureCanvasTkAgg(self.fig, master=root)
        canvas.draw()

        # Make sure Ctrl+C works
        # voxie.tcl_loop_wakeup.setWakeupFd(self.fig.canvas.manager.window)
        voxie.tcl_loop_wakeup.setWakeupFd(root)

        self.pind = None  # active point
        # self.epsilon = 5  # max pixel distance
        self.epsilon = 10  # max pixel distance

        self.l, = self.ax1.plot([0] * (self.maxDegree + 1), [0] * (self.maxDegree + 1), color='k', linestyle='none', marker='o', markersize=8)

        self.X = np.arange(xmin, xmax, 0.01)
        self.m, = self.ax1.plot(self.X, self.func(self.X), 'r-')

        self.ax1.set_yscale('linear')
        self.ax1.set_xlim(xmin, xmax)
        self.ax1.set_ylim(ymin, ymax)
        self.ax1.set_xlabel('x')
        self.ax1.set_ylabel('y')
        self.ax1.grid(True)
        self.ax1.yaxis.grid(True, which='minor', linestyle='--')
        # self.ax1.legend(loc=2, prop={'size': 22})

        self.fig.canvas.mpl_connect('button_press_event', self.button_press_callback)
        self.fig.canvas.mpl_connect('button_release_event', self.button_release_callback)
        self.fig.canvas.mpl_connect('motion_notify_event', self.motion_notify_callback)

        # # pack_toolbar=False will make it easier to use a layout manager later on.
        # toolbar = matplotlib.backends.backend_tkagg.NavigationToolbar2Tk(canvas, root, pack_toolbar=False)
        # toolbar.update()
        # toolbar.pack(side=tkinter.BOTTOM, fill=tkinter.X)

        canvas.get_tk_widget().grid(row=0, column=0, columnspan=6, sticky='NSWE')
        root.rowconfigure(0, weight=1)
        root.columnconfigure(5, weight=1)

        tkinter.Label(root, text='Number of intervals').grid(row=1, column=1)
        self.intervalCountLabel = tkinter.Label(root, text='')
        self.intervalCountLabel.grid(row=2, column=1)

        def uintValidate(P):
            for c in P:
                if str.isdigit(c):
                    continue
                return False
            return True
        uintValidateCmd = root.register(uintValidate)

        self.currentIntervalEntryStringVar = tkinter.StringVar()

        def currentIntervalEntryModified(name, index, mode):
            # print(name, index, mode)
            val = self.currentIntervalEntryStringVar.get()
            try:
                pos = int(val)
            except Exception:
                return
            if pos >= 0 and pos <= self.func.breakpointCount:
                if self.currentInterval != pos:
                    self.currentInterval = pos
                    self.update(updateJsonTextField=False, updateCurrentIntervalEntry=False)
        tkinter.Label(root, text='Current interval').grid(row=3, column=1)
        self.currentIntervalEntry = tkinter.Entry(root, validate='all', validatecommand=(uintValidateCmd, '%P'), textvariable=self.currentIntervalEntryStringVar)
        self.currentIntervalEntryStringVar.trace_add("write", currentIntervalEntryModified)
        self.currentIntervalEntry.grid(row=4, column=1)

        def prev_fun():
            # print(self.currentInterval, self.func.breakpointCount)
            if self.currentInterval > 0:
                self.currentInterval -= 1
                self.update()
        prevButton = tkinter.Button(root, text='Prev', command=prev_fun)
        prevButton.grid(row=5, column=1)

        def next_fun():
            # print(self.currentInterval, self.func.breakpointCount)
            if self.currentInterval < self.func.breakpointCount:
                self.currentInterval += 1
                self.update()
        nextButton = tkinter.Button(root, text='Next', command=next_fun)
        nextButton.grid(row=6, column=1)

        def floatValidate(P):
            for c in P:
                if str.isdigit(c) or c in ['.', '-', '+', 'e', 'E']:
                    continue
                return False
            return True
        floatValidateCmd = root.register(floatValidate)

        tkinter.Label(root, text='Coefficients').grid(row=1, column=2)
        self.coeffEntries = []
        for i in range(self.maxDegree + 1):
            def coeffModified(name, index, mode, i=i):
                # print(name, index, mode)
                if self.coeffEntries[i].progChangeRunning:
                    return
                if i > self.func.polynomials[self.currentInterval].degree:
                    return
                s = self.coeffEntries[i].get()
                # print(s)
                val = tryParseFloat(s)
                if val is None:
                    return
                print(self.func.polynomials[self.currentInterval].coefficients[i], val)
                if self.func.polynomials[self.currentInterval].coefficients[i] != val:
                    self.func.polynomials[self.currentInterval].coefficients[i] = val
                    self.update(updateCoeffEntries=False)
            sv = tkinter.StringVar()
            sv.progChangeRunning = False
            sv.trace_add('write', coeffModified)
            entry = tkinter.Entry(root, validate='all', validatecommand=(floatValidateCmd, '%P'), textvariable=sv)
            entry.grid(row=2 + i, column=2)
            self.coeffEntries.append(sv)

        def set_const_fun():
            # print(self.currentInterval, self.func.breakpointCount)
            coeff = self.func.polynomials[self.currentInterval].coefficients
            val = np.mean(coeff)
            coeff = np.zeros_like(coeff)
            coeff[:] = val
            self.func = self.func.with_polynomial(self.currentInterval, BernsteinPolynomial(self.func.dtype, coeff))
            self.update()
        setConstButton = tkinter.Button(root, text='Set const', command=set_const_fun)
        setConstButton.grid(row=6, column=2)

        # TODO: Make more generic (left right, degree which to match, how many coefficients on each side)
        def leftCont():
            cur = self.currentInterval
            polys = list(self.func.polynomials)
            if cur == 0:
                return
            coeff_cur = np.array(polys[cur].coefficients)
            coeff_cur[0] = polys[cur - 1].coefficients[-1]
            polys[cur] = BernsteinPolynomial(dtype=self.func.dtype, coefficients=coeff_cur)
            self.func = self.func.with_polynomials(polys)
            self.update()

        def leftDiff():
            cur = self.currentInterval
            polys = list(self.func.polynomials)
            if cur == 0:
                return
            coeff_cur = np.array(polys[cur].coefficients)
            coeff_cur[0] = polys[cur - 1].coefficients[-1]
            diff = polys[cur - 1].coefficients[-1] - polys[cur - 1].coefficients[-2]
            diff /= self.func.intervalLen(cur - 1)
            diff *= self.func.intervalLen(cur)
            coeff_cur[1] = coeff_cur[0] + diff
            polys[cur] = BernsteinPolynomial(dtype=self.func.dtype, coefficients=coeff_cur)
            self.func = self.func.with_polynomials(polys)
            self.update()

        def rightCont():
            cur = self.currentInterval
            polys = list(self.func.polynomials)
            if cur == self.func.breakpointCount:
                return
            coeff_cur = np.array(polys[cur].coefficients)
            coeff_cur[-1] = polys[cur + 1].coefficients[0]
            polys[cur] = BernsteinPolynomial(dtype=self.func.dtype, coefficients=coeff_cur)
            self.func = self.func.with_polynomials(polys)
            self.update()

        def rightDiff():
            cur = self.currentInterval
            polys = list(self.func.polynomials)
            if cur == self.func.breakpointCount:
                return
            coeff_cur = np.array(polys[cur].coefficients)
            coeff_cur[-1] = polys[cur + 1].coefficients[0]
            diff = polys[cur + 1].coefficients[1] - polys[cur + 1].coefficients[0]
            diff /= self.func.intervalLen(cur + 1)
            diff *= self.func.intervalLen(cur)
            coeff_cur[-2] = coeff_cur[-1] - diff
            polys[cur] = BernsteinPolynomial(dtype=self.func.dtype, coefficients=coeff_cur)
            self.func = self.func.with_polynomials(polys)
            self.update()

        def leftDiff2():
            cur = self.currentInterval
            polys = list(self.func.polynomials)
            if cur == self.func.breakpointCount:
                return
            coeff_prev = np.array(polys[cur - 1].coefficients)
            coeff_cur = np.array(polys[cur].coefficients)
            l1 = self.func.intervalLen(cur - 1)
            l2 = self.func.intervalLen(cur)
            diff = (coeff_cur[1] - polys[cur - 1].coefficients[-2]) / (l1 + l2)
            val = polys[cur - 1].coefficients[-2] + l1 * diff
            coeff_prev[-1] = val
            coeff_cur[0] = val
            polys[cur - 1] = BernsteinPolynomial(dtype=self.func.dtype, coefficients=coeff_prev)
            polys[cur] = BernsteinPolynomial(dtype=self.func.dtype, coefficients=coeff_cur)
            self.func = self.func.with_polynomials(polys)
            self.update()

        def rightDiff2():
            cur = self.currentInterval
            polys = list(self.func.polynomials)
            if cur == self.func.breakpointCount:
                return
            coeff_cur = np.array(polys[cur].coefficients)
            coeff_next = np.array(polys[cur + 1].coefficients)
            l1 = self.func.intervalLen(cur)
            l2 = self.func.intervalLen(cur + 1)
            diff = (polys[cur + 1].coefficients[1] - coeff_cur[-2]) / (l1 + l2)
            val = coeff_cur[-2] + l1 * diff
            coeff_cur[-1] = val
            coeff_next[0] = val
            polys[cur] = BernsteinPolynomial(dtype=self.func.dtype, coefficients=coeff_cur)
            polys[cur + 1] = BernsteinPolynomial(dtype=self.func.dtype, coefficients=coeff_next)
            self.func = self.func.with_polynomials(polys)
            self.update()
        matchButton = tkinter.Button(root, text='Make Left Diff (2)', command=leftDiff2)
        matchButton.grid(row=1, column=3)
        matchButton = tkinter.Button(root, text='Make Left Diff', command=leftDiff)
        matchButton.grid(row=2, column=3)
        matchButton = tkinter.Button(root, text='Make Left Cont', command=leftCont)
        matchButton.grid(row=3, column=3)
        matchButton = tkinter.Button(root, text='Make Right Cont', command=rightCont)
        matchButton.grid(row=4, column=3)
        matchButton = tkinter.Button(root, text='Make Right Diff', command=rightDiff)
        matchButton.grid(row=5, column=3)
        matchButton = tkinter.Button(root, text='Make Right Diff (2)', command=rightDiff2)
        matchButton.grid(row=6, column=3)

        resetButton = tkinter.Button(root, text='Reset', command=self.reset)
        resetButton.grid(row=1, column=4)

        def copy():
            # TODO: Copy from scrolledText?
            f = io.StringIO()
            json.dump(self.func.asJson(), f, allow_nan=False, sort_keys=True, ensure_ascii=False, indent=2)
            root.clipboard_clear()
            root.clipboard_append(f.getvalue() + '\n')

        def paste():
            # TODO: Paste to scrolledText?
            text = root.clipboard_get()
            try:
                obj = json.load(io.StringIO(text))
                self.func = PiecewisePolynomialFunction.fromJson(obj)
                self.update()
            except Exception as e:
                print('Error while parsing clipboard JSON: {}'.format(e), flush=True)
        button = tkinter.Button(root, text='Copy', command=copy)
        button.grid(row=2, column=4)
        button = tkinter.Button(root, text='Paste', command=paste)
        button.grid(row=3, column=4)

        def info():
            out = sys.stderr
            for i in range(self.func.breakpointCount + 1):
                if i == 0:
                    print('Implicit start at {}'.format(self.func.intervalStart(i)), file=out)
                # print('Interval: Bernstein polynomial with degree {}'.format(self.func.polynomials[self.currentInterval].degree), file=out)
                for j, coeff in enumerate(self.func.polynomials[i].coefficients):
                    print('  Coefficient {}: {}'.format(j, coeff), file=out)
                if i == self.func.breakpointCount:
                    print('Implicit end at {}'.format(self.func.intervalEnd(i)), file=out)
                else:
                    if self.func.breakpointIntegrals[i] == 0:
                        print('Breakpoint at {} with value {}'.format(self.func.breakpoints[i], self.func.breakpointValues[i]), file=out)
                    else:
                        print('Breakpoint at {} with integral {}'.format(self.func.breakpoints[i], self.func.breakpointIntegrals[i]), file=out)
        button = tkinter.Button(root, text='Info', command=info)
        button.grid(row=4, column=4)

        def dec_degree():
            if self.func.polynomials[self.currentInterval].degree == 0:
                return
            self.func = self.func.with_polynomial(self.currentInterval, self.func.polynomials[self.currentInterval].decrease_degree())
            self.update()

        def inc_degree():
            if self.func.polynomials[self.currentInterval].degree == self.maxDegree:
                return
            self.func = self.func.with_polynomial(self.currentInterval, self.func.polynomials[self.currentInterval].increase_degree())
            self.update()
        button = tkinter.Button(root, text='Degree -', command=dec_degree)
        button.grid(row=5, column=4)
        button = tkinter.Button(root, text='Degree +', command=inc_degree)
        button.grid(row=6, column=4)

        self.scrolledText = tkinter.scrolledtext.ScrolledText(root, height=8, width=50)
        self.scrolledText.grid(row=1, rowspan=6, column=0)

        def modified(event):
            # TODO: Avoid triggering this if the JSON is modified by the code

            value = self.scrolledText.edit('modified')
            # print('Modified {} -> {}'.format(event, value), flush=True)
            if not value:
                return

            text = self.scrolledText.get('1.0', 'end')

            try:
                obj = json.load(io.StringIO(text))
                self.func = PiecewisePolynomialFunction.fromJson(obj)
                self.update(updateJsonTextField=False)
            except Exception as e:
                print('Error while parsing JSON: {}'.format(e), flush=True)

            self.scrolledText.edit('modified', '0')
        self.scrolledText.bind('<<Modified>>', modified)

        self.update()

        root.protocol("WM_DELETE_WINDOW", root.quit)

        # plt.show()
        root.mainloop()


def main():
    gui = GUI()
    gui.run()


if __name__ == '__main__':
    main()
