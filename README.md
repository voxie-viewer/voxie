About voxie
===========

Voxie is a voxel viewer with support for plugins and external scripts.
It was initially developed as a student project at the IPVS of the University
of Stuttgart.


Compiling from source on Linux
==============================

First, install the packages required to run Voxie. On Ubuntu 14.04 this can be done using:

    sudo apt-get install build-essential qt5-default qtscript5-dev libqt5opengl5-dev opencl-headers libhdf5-dev libboost-all-dev

If you want to compile the manual, you also have to install LaTeX:

    sudo apt-get install texlive-latex-recommended

For running the python examples, you need python + numpy:

    sudo apt-get install python3-numpy

Then you can either use `qtcreator` for compiling the code or run

    tools/build.sh

(This will use `qmake` to build the project.)
In order to install the code, run:

    tools/build.sh install

This will install Voxie into `/usr/local/lib/voxie` and put a link
into `/usr/local/bin/voxie`.


Compiling from source on Windows
================================

For compiling on Windows you need to install Qt5 + QtCreator, HDF5, an OpenCL
SDK (e.g. the AMD OpenCL SDK) and Boost (for Boost, only the headers or sources
are needed).

* <https://www.qt.io/download-open-source/#section-2>
* <https://www.hdfgroup.org/HDF5/release/obtain5.html>
* <http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/>
* <http://www.boost.org/users/download/>

For running external scripts you also will need DBus for Windows:

* <http://cgit.freedesktop.org/dbus/dbus/snapshot/dbus-1.8.2.zip>

The next step is to create a file `localconfig.pri` in `src/PluginHDF5` with
the content:

    HDF5_PATH = $$quote("C:/Path to HDF5/1.2.3")
    BOOST_PATH = $$quote("C:/Path to Boost")

Then you should be able to compile the project using QtCreator.


Plugins / Scripting
===================

You can extend Voxie by writing plugins or scripts.

* Plugins will be loaded into the same address space as Voxie and can access
  all the internals of Voxie.
* Internal scripts can be written in JavaScript/QScript. There are some examples
  in the directory `scripts`.
* External scripts can be written in an arbitrary language and can communicate
  with Voxie over DBus. The can also access the data directly using shared
  memory. There are some examples in the directory `scripts` and one in
  `src/ScriptGetAverage`.

Additional plugins can be put into `~/.config/voxie/plugins` on Linux or
`C:\Users\...\AppData\Roaming\voxie\plugins` on Windows.
Additional scripts can be put into `~/.config/voxie/scripts` on Linux or
`C:\Users\...\AppData\Roaming\voxie\scripts` on Windows.
