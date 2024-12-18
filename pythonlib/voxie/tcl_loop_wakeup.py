import os
import numbers
import signal
import sys
import threading
import select


class FileHandle:
    debug = False

    def __init__(self, fd, *, backend_object=None):
        if fd is not None and not isinstance(fd, numbers.Integral):
            raise Exception('fd must be an integral number or None')
        if fd is not None:
            fd = int(fd)

        self.fd = fd
        self.backend_object = backend_object
        if self.debug:
            print('FileHandle ctor {}'.format(self.fd), flush=True)

    def destroy(self):
        if self.debug and self.fd is not None:
            print('FileHandle destroy {}'.format(self.fd), flush=True)

        fd = self.fd
        self.fd = None
        if fd is not None:
            # os might be None during shutdown, in that case, don't call close()
            if os is not None:  # Handle process shutdown
                os.close(fd)

        if self.backend_object is not None:
            self.backend_object.destroy()

    def __del__(self):
        if self.debug and self.fd is not None:
            print('FileHandle dtor {}'.format(self.fd), flush=True)

        self.destroy()

    def take(self):
        if self.debug:
            print('FileHandle take {}'.format(self.fd), flush=True)

        fd = self.fd
        self.fd = None
        return fd

    def setNonblock(self, nonblock):
        import fcntl

        if nonblock:
            fcntl.fcntl(self.fd, fcntl.F_SETFL, fcntl.fcntl(self.fd, fcntl.F_GETFL) | os.O_NONBLOCK)
        else:
            fcntl.fcntl(self.fd, fcntl.F_SETFL, fcntl.fcntl(self.fd, fcntl.F_GETFL) & ~os.O_NONBLOCK)


# Will call tk.deletefilehandler() when it is deleted
# There must be only one instance of TclFileHandle per FD (because it calls deletefilehandler)
class TclFileHandle:
    debug = False

    def __init__(self, tk, file_handle):
        self.destroyed = False
        self.tk = tk
        self.file_handle = file_handle
        if self.debug:
            print('TclFileHandle ctor {}'.format(self.file_handle.fd), flush=True)

    def destroy(self):
        if self.destroyed:
            return
        self.destroyed = True

        if self.debug:
            print('TclFileHandle destroy {}'.format(self.file_handle.fd), flush=True)
        if self.file_handle is not None:
            self.tk.deletefilehandler(self.file_handle.fd)
        # TODO: Do this?
        # self.file_handle.destroy()

    def __del__(self):
        if self.debug:
            print('TclFileHandle dtor {}'.format(self.file_handle.fd), flush=True)

        self.destroy()

    def setHandler(self, mask, handler):
        # Delete old handlers
        self.tk.deletefilehandler(self.file_handle.fd)
        self.tk.createfilehandler(self.file_handle.fd, mask, handler)


def pipe():
    read_fd, write_fd = os.pipe()
    return FileHandle(read_fd), FileHandle(write_fd)


class SimpleWakeupFdImpl:
    debug = False

    def __init__(self):
        if self.debug:
            print('SimpleWakeupFdImpl ctor {}'.format(hash(self)), flush=True)

        self.destroyed = False
        self.isSet = False
        self.readHandle = None
        self.writeHandle = None
        ok = False
        try:
            self.readHandle, self.writeHandle = pipe()
            self.writeHandle.setNonblock(True)
            oldFd = signal.set_wakeup_fd(self.writeHandle.fd, warn_on_full_buffer=False)
            if oldFd != -1:
                signal.set_wakeup_fd(-1, warn_on_full_buffer=False)
                signal.set_wakeup_fd(oldFd, warn_on_full_buffer=False)
                raise Exception('Signal wakeup fd was alrleady set to {}'.format(oldFd))
            self.isSet = True

            ok = True
        finally:
            if not ok:
                self.destroy()

    def destroy(self):
        if self.destroyed:
            return
        self.destroyed = True

        if self.debug:
            print('SimpleWakeupFdImpl destroy {}'.format(hash(self)), flush=True)

        if self.isSet:
            if signal is not None:  # Handle process shutdown
                signal.set_wakeup_fd(-1, warn_on_full_buffer=False)
        if self.readHandle is not None:
            self.readHandle.destroy()
        if self.writeHandle is not None:
            self.writeHandle.destroy()

    def __del__(self):
        if self.debug:
            print('SimpleWakeupFdImpl dtor {}'.format(hash(self)), flush=True)

        self.destroy()

    def take(self):
        handle = self.readHandle
        self.readHandle = None
        return handle


def createSimpleWakeupFd():
    impl = SimpleWakeupFdImpl()
    readHandle = impl.take()
    readHandle.backend_object = impl
    return readHandle


class WakeupDistributorImpl:
    debug = False

    class DistribtorThreadImpl:
        def __init__(self, upstream):
            self.destroyed = False
            self.upstream = upstream
            self.killReadHandle = None
            self.killWriteHandle = None
            self.poll = None
            self.children = []

            if WakeupDistributorImpl.debug:
                print('WakeupDistributorImpl.DistribtorThreadImpl ctor {}'.format(hash(self)), flush=True)

            self.mutex = threading.Lock()

            ok = False
            try:
                self.killReadHandle, self.killWriteHandle = pipe()

                self.upstream.setNonblock(True)
                self.killReadHandle.setNonblock(True)

                self.poll = select.poll()
                self.poll.register(self.upstream.fd)
                self.poll.register(self.killReadHandle.fd)

                ok = True
            finally:
                if not ok:
                    self.destroy()

        def destroy(self):
            if self.destroyed:
                return
            self.destroyed = True

            if WakeupDistributorImpl is not None and WakeupDistributorImpl.debug:
                print('WakeupDistributorImpl.DistribtorThreadImpl destroy {}'.format(hash(self)), flush=True)

            if self.upstream is not None:
                self.upstream.destroy()
            if self.killReadHandle is not None:
                self.killReadHandle.destroy()
            if self.killWriteHandle is not None:
                self.killWriteHandle.destroy()

        def __del__(self):
            if WakeupDistributorImpl.debug:
                print('WakeupDistributorImpl.DistribtorThreadImpl dtor {}'.format(hash(self)), flush=True)

            self.destroy()

        def run(self):
            if WakeupDistributorImpl.debug:
                print('WakeupDistributorImpl.DistribtorThreadImpl run {}'.format(hash(self)), flush=True)

            while True:
                res = self.poll.poll()
                if WakeupDistributorImpl.debug:
                    print('WakeupDistributorImpl.DistribtorThreadImpl {}: Got {} from poll'.format(hash(self), res), flush=True)

                count = 0
                eofUpstream = False
                while True:
                    # Read from file until it would block
                    try:
                        data = os.read(self.upstream.fd, 1024)
                        if WakeupDistributorImpl.debug:
                            print('WakeupDistributorImpl.DistribtorThreadImpl {}: Got {} bytes from upstream'.format(hash(self), len(data)), flush=True)
                        count += len(data)
                        if len(data) == 0:
                            eofUpstream = True
                            if WakeupDistributorImpl.debug:
                                print('WakeupDistributorImpl.DistribtorThreadImpl {}: Got EOF from upstream'.format(hash(self)), flush=True)
                            break
                    except BlockingIOError:
                        break

                countKill = 0
                eofKill = False
                while True:
                    # Read from file until it would block
                    try:
                        data = os.read(self.killReadHandle.fd, 1024)
                        if WakeupDistributorImpl.debug:
                            print('WakeupDistributorImpl.DistribtorThreadImpl {}: Got {} bytes from kill pipe'.format(hash(self), len(data)), flush=True)
                        countKill += len(data)
                        if len(data) == 0:
                            eofKill = True
                            if WakeupDistributorImpl.debug:
                                print('WakeupDistributorImpl.DistribtorThreadImpl {}: Got EOF from kill pipe'.format(hash(self)), flush=True)
                            break
                    except BlockingIOError:
                        break

                if countKill > 0 or eofKill:
                    return

                if count > 0:
                    if WakeupDistributorImpl.debug:
                        print('WakeupDistributorImpl.DistribtorThreadImpl {}: Waking up children'.format(hash(self)), flush=True)
                    with self.mutex:
                        newChildren = []
                        for child in self.children:
                            try:
                                os.write(child.fd, b'\0')
                                newChildren.append(child)
                            except Exception as e:
                                print('WakeupDistributorImpl.DistribtorThreadImpl {}: Warning: Error writing to wakeup child {}: {}'.format(hash(self), child, e), flush=True, file=sys.stderr)
                                try:
                                    child.destroy()
                                except Exception as e:
                                    print('WakeupDistributorImpl.DistribtorThreadImpl {}: Warning: Error destroying child {}: {}'.format(hash(self), child, e), flush=True, file=sys.stderr)
                        if WakeupDistributorImpl.debug:
                            print('WakeupDistributorImpl.DistribtorThreadImpl {}: Child count: {} -> {}'.format(hash(self), len(self.children), len(newChildren)), flush=True)
                        self.children = newChildren

                if eofUpstream:
                    print('WakeupDistributorImpl.DistribtorThreadImpl {}: Warning: Got EOF from upstream pipe, exiting'.format(hash(self)), flush=True, file=sys.stderr)
                    return

        def take(self):
            handle = self.killWriteHandle
            self.killWriteHandle = None
            return handle

        def addChild(self, child):
            child.setNonblock(True)
            with self.mutex:
                self.children.append(child)

        def removeChild(self, child):
            with self.mutex:
                try:
                    self.children.remove(child)
                except ValueError:
                    pass

    class Child:
        def __init__(self, parent):
            if WakeupDistributorImpl.debug:
                print('WakeupDistributorImpl.Child ctor {}'.format(hash(self)), flush=True)

            self.destroyed = False
            self.parent = parent
            self.readHandle = None
            self.writeHandle = None
            ok = False
            try:
                self.readHandle, self.writeHandle = pipe()
                parent.addChild(self.writeHandle)

                ok = True
            finally:
                if not ok:
                    self.destroy()

        def destroy(self):
            if self.destroyed:
                return
            self.destroyed = True

            if WakeupDistributorImpl.debug:
                print('WakeupDistributorImpl.Child destroy {}'.format(hash(self)), flush=True)

            if self.writeHandle is not None:
                self.parent.removeChild(self.writeHandle)
                self.writeHandle.destroy()
            if self.readHandle is not None:
                self.readHandle.destroy()

        def __del__(self):
            if WakeupDistributorImpl.debug:
                print('WakeupDistributorImpl.Child dtor {}'.format(hash(self)), flush=True)

            self.destroy()

        def take(self):
            handle = self.readHandle
            self.readHandle = None
            return handle

    def __init__(self, upstream=None):
        if self.debug:
            print('WakeupDistributorImpl ctor {}'.format(hash(self)), flush=True)

        self.destroyed = False
        self.upstream = None
        self.threadImpl = None
        self.thread = None
        self.killWriteHandle = None
        ok = False
        try:
            self.upstream = upstream
            if self.upstream is None:
                self.upstream = createSimpleWakeupFd()

            self.threadImpl = WakeupDistributorImpl.DistribtorThreadImpl(self.upstream)
            self.killWriteHandle = self.threadImpl.take()
            self.killWriteHandle.setNonblock(True)
            self.thread = threading.Thread(target=self.threadImpl.run, name='WakeupDistributorThread', daemon=True)
            self.thread.start()

            ok = True
        finally:
            if not ok:
                self.destroy()

    def destroy(self):
        if self.destroyed:
            return
        self.destroyed = True

        if self.debug:
            print('WakeupDistributorImpl destroy {}'.format(hash(self)), flush=True)

        if self.killWriteHandle is not None:
            if self.killWriteHandle.fd is not None:
                # os might be None during shutdown, in that case, don't call write()
                if os is not None:  # Handle process shutdown
                    os.write(self.killWriteHandle.fd, b'\0')
            self.killWriteHandle.destroy()
        if self.thread is not None:
            self.thread.join()
        if self.threadImpl is not None:
            self.threadImpl.destroy()
        if self.upstream is not None:
            self.upstream.destroy()

    def __del__(self):
        if self.debug:
            print('WakeupDistributorImpl dtor {}'.format(hash(self)), flush=True)

        self.destroy()

    def createChild(self):
        child = WakeupDistributorImpl.Child(self.threadImpl)
        readHandle = child.take()
        readHandle.backend_object = child
        return readHandle


wakeupDistributorInstance = None


def getWakeupDistributor():
    global wakeupDistributorInstance
    if wakeupDistributorInstance is None:
        wakeupDistributorInstance = WakeupDistributorImpl()
    return wakeupDistributorInstance


def setWakeupFd(tk):
    import tkinter

    readHandle = getWakeupDistributor().createChild()
    readHandle.setNonblock(True)

    def handler(file, mask):
        while True:
            # print('HANDLER READ', flush=True)
            # Read from file until it would block
            try:
                data = os.read(file, 1024)
                # print('Wakeup FD handler: Got {} bytes'.format(len(data)), flush=True, file=sys.stderr)
                if len(data) == 0:
                    # EOF
                    print('Wakeup FD handler: Got EOF', flush=True, file=sys.stderr)
                    # TODO: Deregister?
                    return
            except BlockingIOError:
                return
    tclReadHandle = TclFileHandle(tk, readHandle)
    tk.register(tclReadHandle)
    # TODO: Also look for errors?
    tclReadHandle.setHandler(tkinter.READABLE, handler)


def createAndSetupTk():
    import tkinter

    root = tkinter.Tk()
    root.withdraw()
    setWakeupFd(root)
    return root


patchTkDebug = False


def _patchTk(do):
    import tkinter

    cls = tkinter.Tk
    current = cls.__init__
    patched = hasattr(current, 'original_function')

    if not do:
        if not patched:
            if patchTkDebug:
                print('patchTk: Not patched', flush=True)
            return
        cls.__init__ = current.original_function
        return

    # Make sure wakeup distribtor is available
    getWakeupDistributor()

    if patched:
        if patchTkDebug:
            print('patchTk: Already patched', flush=True)
        return

    def wrapper(self, *args, **kwargs):
        res = current(self, *args, **kwargs)
        try:
            if patchTkDebug:
                print('Setting wakeup FD for {}'.format(self), flush=True)
            setWakeupFd(self)
        except Exception as e:
            print('Error setting wakeup FD for: {}'.format(e), flush=True, file=sys.stderr)
        return res
    wrapper.original_function = current

    cls.__init__ = wrapper


def patchTk():
    _patchTk(True)


def unpatchTk():
    _patchTk(False)


patchMatplotlibDebug = False


def _patchMatplotlib(do):
    import matplotlib.backends._backend_tk

    cls = matplotlib.backends._backend_tk.FigureManagerTk
    current = cls.create_with_canvas
    patched = hasattr(current, 'original_function')

    if not do:
        if not patched:
            if patchMatplotlibDebug:
                print('patchMatplotlib: Not patched', flush=True)
            return
        cls.create_with_canvas = current.original_function
        return

    # Make sure wakeup distribtor is available
    getWakeupDistributor()

    if patched:
        if patchMatplotlibDebug:
            print('patchMatplotlib: Already patched', flush=True)
        return

    def wrapper(*args, **kwargs):
        res = current(*args, **kwargs)
        try:
            if patchMatplotlibDebug:
                print('Setting wakeup FD for {}'.format(res.window), flush=True)
            setWakeupFd(res.window)
        except Exception as e:
            print('Error setting wakeup FD for: {}'.format(e), flush=True, file=sys.stderr)
        return res
    wrapper.original_function = current

    cls.create_with_canvas = wrapper


def patchMatplotlib():
    _patchMatplotlib(True)


def unpatchMatplotlib():
    _patchMatplotlib(False)


def setDebug(debug):
    if not isinstance(debug, bool):
        raise Exception('Parameter debug is not a bool')
    debug = bool(debug)

    global patchTkDebug
    global patchMatplotlibDebug

    FileHandle.debug = debug
    TclFileHandle.debug = debug
    SimpleWakeupFdImpl.debug = debug
    WakeupDistributorImpl.debug = debug
    patchTkDebug = debug
    patchMatplotlibDebug = debug
