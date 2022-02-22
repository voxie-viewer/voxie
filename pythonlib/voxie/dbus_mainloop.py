import ctypes
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
import ctypes.util
import _dbus_bindings
import select
import atexit
import time
# import dbus.mainloop.glib
# from gi.repository import GLib

# https://dbus.freedesktop.org/doc/api/html/group__DBusConnection.html
# https://docs.python.org/3/library/atexit.html

# See dbus-connection.h

DBUS_WATCH_READABLE = 1 << 0
DBUS_WATCH_WRITABLE = 1 << 1
DBUS_WATCH_ERROR = 1 << 2
DBUS_WATCH_HANGUP = 1 << 3

DBUS_DISPATCH_DATA_REMAINS = 0
DBUS_DISPATCH_COMPLETE = 1
DBUS_DISPATCH_NEED_MEMORY = 2

# See dbus-python.h

# TODO: Implement for Python2

PYDBUS_CAPSULE_NAME = b"_dbus_bindings._C_API"
DBUS_BINDINGS_API_COUNT = 3

ctypes.pythonapi.PyCapsule_GetPointer.argtypes = [
    ctypes.py_object, ctypes.c_char_p]
ctypes.pythonapi.PyCapsule_GetPointer.restype = ctypes.c_void_p

ptr = ctypes.pythonapi.PyCapsule_GetPointer(
    _dbus_bindings._C_API, PYDBUS_CAPSULE_NAME)
if not ptr:
    raise Exception('PyCapsule_GetPointer() returned NULL')

count = ctypes.POINTER(ctypes.c_int).from_address(ptr).contents.value
if count < DBUS_BINDINGS_API_COUNT:
    raise Exception('_dbus_bindings has API version %d but %s needs _dbus_bindings API version at least %d' % (
        count, __name__, DBUS_BINDINGS_API_COUNT))

api = (ctypes.c_void_p * 3).from_address(ptr)
# count == ctypes.c_int.from_address(dbus_mainloop.api[0])

dbus_bool_t = ctypes.c_uint32
DBusConnection_p = ctypes.c_void_p
DBusServer_p = ctypes.c_void_p
DBusWatch_p = ctypes.c_void_p
DBusTimeout_p = ctypes.c_void_p

_dbus_py_conn_setup_func = ctypes.PYFUNCTYPE(
    dbus_bool_t, DBusConnection_p, ctypes.c_void_p)
_dbus_py_srv_setup_func = ctypes.PYFUNCTYPE(
    dbus_bool_t, DBusServer_p, ctypes.c_void_p)
_dbus_py_free_func = ctypes.PYFUNCTYPE(None, ctypes.c_void_p)

DBusPyConnection_BorrowDBusConnection = ctypes.PYFUNCTYPE(
    DBusConnection_p, ctypes.py_object)(api[1])

DBusPyNativeMainLoop_New4 = ctypes.PYFUNCTYPE(
    ctypes.py_object, _dbus_py_conn_setup_func, _dbus_py_srv_setup_func, _dbus_py_free_func, ctypes.c_void_p)(api[2])

DBusFreeFunction = ctypes.PYFUNCTYPE(None, ctypes.c_void_p)

DBusAddWatchFunction = ctypes.PYFUNCTYPE(
    dbus_bool_t, DBusWatch_p, ctypes.c_void_p)
DBusRemoveWatchFunction = ctypes.PYFUNCTYPE(None, DBusWatch_p, ctypes.c_void_p)
DBusWatchToggledFunction = ctypes.PYFUNCTYPE(
    None, DBusWatch_p, ctypes.c_void_p)

DBusAddTimeoutFunction = ctypes.PYFUNCTYPE(
    dbus_bool_t, DBusTimeout_p, ctypes.c_void_p)
DBusRemoveTimeoutFunction = ctypes.PYFUNCTYPE(
    None, DBusTimeout_p, ctypes.c_void_p)
DBusTimeoutToggledFunction = ctypes.PYFUNCTYPE(
    None, DBusTimeout_p, ctypes.c_void_p)

DBusWakeupMainFunction = ctypes.PYFUNCTYPE(None, ctypes.c_void_p)

dbusdll = ctypes.CDLL(ctypes.util.find_library('dbus-1'))

dbusdll.dbus_connection_dispatch.argtypes = [DBusConnection_p]
dbusdll.dbus_connection_dispatch.restype = dbus_bool_t
dbusdll.dbus_connection_get_dispatch_status.argtypes = [DBusConnection_p]
dbusdll.dbus_connection_get_dispatch_status.restype = dbus_bool_t

dbusdll.dbus_connection_set_watch_functions.argtypes = [
    DBusConnection_p, DBusAddWatchFunction, DBusRemoveWatchFunction, DBusWatchToggledFunction, ctypes.c_void_p, DBusFreeFunction]
dbusdll.dbus_connection_set_watch_functions.restype = dbus_bool_t
dbusdll.dbus_watch_get_unix_fd.argtypes = [DBusWatch_p]
dbusdll.dbus_watch_get_unix_fd.restype = ctypes.c_int
dbusdll.dbus_watch_get_socket.argtypes = [DBusWatch_p]
dbusdll.dbus_watch_get_socket.restype = ctypes.c_int
dbusdll.dbus_watch_get_enabled.argtypes = [DBusWatch_p]
dbusdll.dbus_watch_get_enabled.restype = dbus_bool_t
dbusdll.dbus_watch_get_flags.argtypes = [DBusWatch_p]
dbusdll.dbus_watch_get_flags.restype = ctypes.c_int
dbusdll.dbus_watch_handle.argtypes = [DBusWatch_p, ctypes.c_int]
dbusdll.dbus_watch_handle.restype = dbus_bool_t

dbusdll.dbus_connection_set_timeout_functions.argtypes = [
    DBusConnection_p, DBusAddTimeoutFunction, DBusRemoveTimeoutFunction, DBusTimeoutToggledFunction, ctypes.c_void_p, DBusFreeFunction]
dbusdll.dbus_connection_set_timeout_functions.restype = dbus_bool_t
dbusdll.dbus_timeout_get_enabled.argtypes = [DBusTimeout_p]
dbusdll.dbus_timeout_get_enabled.restype = dbus_bool_t
dbusdll.dbus_timeout_get_interval.argtypes = [DBusTimeout_p]
dbusdll.dbus_timeout_get_interval.restype = ctypes.c_int
dbusdll.dbus_timeout_handle.argtypes = [DBusTimeout_p]
dbusdll.dbus_timeout_handle.restype = dbus_bool_t

dbusdll.dbus_connection_set_wakeup_main_function.argtypes = [
    DBusConnection_p, DBusWakeupMainFunction, ctypes.c_void_p, DBusFreeFunction]
dbusdll.dbus_connection_set_wakeup_main_function.restype = dbus_bool_t

watches = []
timeouts = []
pending = []

timeoutMonotonicLast = {}

deregFuncs = {}

enableDebug = False


def debug(*args):
    if enableDebug:
        print(*args)


def add_watch(watch, data):
    debug('add_watch (%s, %s)' % (watch, data))
    debug('%s %s %s %s' % (dbusdll.dbus_watch_get_unix_fd(watch), dbusdll.dbus_watch_get_socket(
        watch), dbusdll.dbus_watch_get_enabled(watch), dbusdll.dbus_watch_get_flags(watch)))
    watches.append(watch)
    return 1


def remove_watch(watch, data):
    debug('remove_watch (%s, %s)' % (watch, data))
    watches.remove(watch)
    pendingcpy = list(pending)
    for ev in pendingcpy:
        if ev[0] == watch:
            pending.remove(ev)


def watch_toggled(watch, data):
    debug('watch_toggled (%s, %s)' % (watch, data))


def free_watch_funcs(data):
    debug('free1 watch (%s)' % (data))
    if data in deregFuncs:
        deregFuncs[data]()


def add_timeout(timeout, data):
    debug('add_timeout (%s, %s)' % (timeout, data))
    timeouts.append(timeout)
    timeoutMonotonicLast[timeout] = 0
    return 1


def remove_timeout(timeout, data):
    debug('remove_timeout (%s, %s)' % (timeout, data))
    timeouts.remove(timeout)
    del timeoutMonotonicLast[timeout]


def timeout_toggled(timeout, data):
    debug('timeout_toggled (%s, %s)' % (timeout, data))
    timeoutMonotonicLast[timeout] = 0


def free_timeout_funcs(data):
    debug('free1 timeout (%s)' % (data))


def wakeup_main(data):
    debug('wakeup_main (%s)' % (data))


def free_wakeup_main_funcs(data):
    debug('free1 wakeup_main (%s)' % (data))


c_add_watch = DBusAddWatchFunction(add_watch)
c_remove_watch = DBusRemoveWatchFunction(remove_watch)
c_watch_toggled = DBusWatchToggledFunction(watch_toggled)
c_free_watch_funcs = DBusFreeFunction(free_watch_funcs)

c_add_watch_0 = DBusAddWatchFunction(0)
c_remove_watch_0 = DBusRemoveWatchFunction(0)
c_watch_toggled_0 = DBusWatchToggledFunction(0)
c_free_watch_funcs_0 = DBusFreeFunction(0)

c_add_timeout = DBusAddTimeoutFunction(add_timeout)
c_remove_timeout = DBusRemoveTimeoutFunction(remove_timeout)
c_timeout_toggled = DBusTimeoutToggledFunction(timeout_toggled)
c_free_timeout_funcs = DBusFreeFunction(free_timeout_funcs)

c_add_timeout_0 = DBusAddTimeoutFunction(0)
c_remove_timeout_0 = DBusRemoveTimeoutFunction(0)
c_timeout_toggled_0 = DBusTimeoutToggledFunction(0)
c_free_timeout_funcs_0 = DBusFreeFunction(0)

c_wakeup_main = DBusWakeupMainFunction(wakeup_main)
c_free_wakeup_main_funcs = DBusFreeFunction(free_wakeup_main_funcs)

c_wakeup_main_0 = DBusWakeupMainFunction(0)
c_free_wakeup_main_funcs_0 = DBusFreeFunction(0)

conns = []


def conn_setup(conn, data):
    debug('conn_setup (%s, %s)' % (conn, data))

    def dereg():  # Will be called either when the connection is closed or on shutdown
        debug('dereg')
        atexit.unregister(deregFuncs[conn])
        del deregFuncs[conn]
        conns.remove(conn)
        dbusdll.dbus_connection_set_watch_functions(
            conn, c_add_watch_0, c_remove_watch_0, c_watch_toggled_0, None, c_free_watch_funcs_0)
        dbusdll.dbus_connection_set_timeout_functions(
            conn, c_add_timeout_0, c_remove_timeout_0, c_timeout_toggled_0, None, c_free_timeout_funcs_0)
        dbusdll.dbus_connection_set_wakeup_main_function(
            conn, c_wakeup_main_0, None, c_free_wakeup_main_funcs_0)
    deregFuncs[conn] = atexit.register(dereg)
    dbusdll.dbus_connection_set_watch_functions(
        conn, c_add_watch, c_remove_watch, c_watch_toggled, ctypes.c_void_p(conn), c_free_watch_funcs)
    dbusdll.dbus_connection_set_timeout_functions(
        conn, c_add_timeout, c_remove_timeout, c_timeout_toggled, ctypes.c_void_p(conn), c_free_timeout_funcs)
    dbusdll.dbus_connection_set_wakeup_main_function(
        conn, c_wakeup_main, None, c_free_wakeup_main_funcs)
    conns.append(conn)
    return 1


def srv_setup(srv, data):
    debug('srv_setup (%s, %s)' % (srv, data))
    return 0


def free(data):
    debug('free2 (%s)' % (data))


c_conn_setup = _dbus_py_conn_setup_func(conn_setup)
c_srv_setup = _dbus_py_srv_setup_func(srv_setup)
# c_free = _dbus_py_free_func (free)
# Some function which will ignore its arguments
c_free = _dbus_py_free_func(ctypes.pythonapi.PySys_HasWarnOptions)

mainloop = DBusPyNativeMainLoop_New4(
    c_conn_setup, c_srv_setup, c_free, ctypes.c_void_p(22))


def iter():
    rlist = []
    wlist = []
    xlist = []
    rlistW = []
    wlistW = []
    xlistW = []
    timeoutS = None
    for conn in conns:
        debug('X %s' % dbusdll.dbus_connection_get_dispatch_status(conn))
        while dbusdll.dbus_connection_dispatch(conn) == DBUS_DISPATCH_DATA_REMAINS:
            pass
    now = time.monotonic()
    for timeout in timeouts:
        if not dbusdll.dbus_timeout_get_enabled(timeout):
            continue
        interval = dbusdll.dbus_timeout_get_interval(timeout)
        remaining = now - timeoutMonotonicLast[timeout] - interval
        remaining = remaining * 0.001
        if timeoutS is None or remaining < timeoutS:
            timeoutS = remaining
    if timeoutS is not None and timeoutS < 0:
        timeoutS = 0
    # if timeoutS is None or timeoutS > 2:
    #    timeoutS = 2
    for watch in watches:
        if not dbusdll.dbus_watch_get_enabled(watch):
            continue
        flags = dbusdll.dbus_watch_get_flags(watch)
        socket = dbusdll.dbus_watch_get_socket(watch)
        if socket == -1:
            socket = dbusdll.dbus_watch_get_unix_fd(watch)
        if flags & DBUS_WATCH_READABLE:
            rlist.append(socket)
            rlistW.append(watch)
        if flags & DBUS_WATCH_WRITABLE:
            wlist.append(socket)
            wlistW.append(watch)
        xlist.append(socket)
        xlistW.append(watch)
    (rres, wres, xres) = select.select(rlist, wlist, xlist, timeoutS)
    for s in rres:
        watch = rlistW[rlist.index(s)]
        pending.append((watch, DBUS_WATCH_READABLE))
    for s in wres:
        watch = wlistW[wlist.index(s)]
        pending.append((watch, DBUS_WATCH_WRITABLE))
    for s in xres:
        watch = xlistW[xlist.index(s)]
        pending.append((watch, DBUS_WATCH_ERROR | DBUS_WATCH_HANGUP))
    while len(pending):
        ev = pending[0]
        pending.remove(ev)
        debug(ev)
        dbusdll.dbus_watch_handle(ev[0], ev[1])
        # dbusdll.dbus_watch_handle (ev[0], 15)
    for timeout in timeouts:
        if not dbusdll.dbus_timeout_get_enabled(timeout):
            continue
        interval = dbusdll.dbus_timeout_get_interval(timeout)
        remaining = now - timeoutMonotonicLast[timeout] - interval
        if remaining <= 0 and timeout in timeouts:
            dbusdll.dbus_timeout_handle(timeout)
    for conn in conns:
        debug('Y %s' % dbusdll.dbus_connection_get_dispatch_status(conn))
        while dbusdll.dbus_connection_dispatch(conn) == DBUS_DISPATCH_DATA_REMAINS:
            pass


def iter_forever():
    while True:
        debug('iter')
        iter()

# import dbus
# #c = dbus.bus.BusConnection('unix:abstract=/tmp/dbus-FCbuyM4bSN', mainloop=mainloop)
# c = dbus.SessionBus(mainloop=mainloop)
# #loop = GLib.MainLoop ()
# #c = dbus.bus.BusConnection('unix:abstract=/tmp/dbus-FCbuyM4bSN', mainloop=dbus.mainloop.glib.DBusGMainLoop ())
# print (c.get_unique_name())
# iter_forever()
# # print ('a')
# # loop.get_context ().iteration (may_block = True)
# # print ('a')
# # loop.get_context ().iteration (may_block = True)
# # print ('a')
# # loop.get_context ().iteration (may_block = True)
# # print ('a')
# # loop.get_context ().iteration (may_block = True)
# # print ('a')
# # loop.get_context ().iteration (may_block = True)
# # print ('a')
