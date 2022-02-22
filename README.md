About Voxie
===========

Voxie is a volume viewer with support for plugins and external scripts.
It was initially developed as a student project at the Computational Imaging
Systems (CIS) department of the University of Stuttgart.

The main author of Voxie is
[Steffen Kieß](mailto:steffen.kiess@cis.iti.uni-stuttgart.de), other
contributors are mostly students at the University of Stuttgart. The full
list of authors can be found in [doc/software.json](doc/software.json).

Compiling from source on Linux
==============================

First, install the packages required to run Voxie. On Ubuntu 20.04 this can be done using:

    sudo apt-get install build-essential meson ninja-build pkg-config qtscript5-dev libqt5opengl5-dev libqt5x11extras5-dev qtwebengine5-dev opencl-headers libhdf5-dev libboost-all-dev liblapacke-dev python3-pycodestyle python3-numpy python3-scipy python3-pyqt5
    
If you want to compile the old manual, you also have to install LaTeX:

    sudo apt-get install texlive-latex-extra

Then you can either use `qtcreator` (with meson) for compiling the code or run

    tools/build.sh

(If the meson version is too old, you also can run `tools/build.sh --unpack-meson` to download and use a newer meson version.)

After compiling, you can run voxie with:

    ./voxie.sh

Compiling from source on Windows
================================

For compiling on Windows you need to install Qt5 + QtCreator, HDF5 (binarys
needed) and Boost (for Boost, only the headers or sources are needed).

* <https://www.qt.io/download-open-source/#section-2>
* <https://www.hdfgroup.org/ftp/HDF5/releases/hdf5-1.10/hdf5-1.10.0-patch1/bin/windows/extra/hdf5-1.10.0-patch1-win64-vs2015-shared.zip>
* <http://www.boost.org/users/download/>

For running external python filters or scripts you also will need DBus for
Windows:

* <http://cgit.freedesktop.org/dbus/dbus/snapshot/dbus-1.8.2.zip>

IDE Setup
=========

To find out how to setup IDEs for development check out */doc/IDE_Setup.md*.

Style Guide
===========

## c++

All c++ files (cpp, hpp, ...) must follow the *CLANG format* with the [*Google Style*](https://google.github.io/styleguide/cppguide.html).
When you are using QtCreator enable the *Beautifier* plugin and configure it in such as way, that saved files are automatically formatted.
To do that enable Beautifier under Help->Plugins in QtCreator. Download Pre-Built Binaries for Clang-fomat from LLVM Website and install it.
Under Extras->Properties->Beautifier select the Clang Fromat tap and choose clang-format in LLVM install dir. Under Style choose File.

If you prefer other editors you can call `clang-format` directly:

    clang-format -i -style=file path/to/file.cpp

One could use this command (or something similar) to format all files changed and not merged into master:

    git diff master --name-only | grep -i '.hpp\|.cpp' | xargs clang-format -i -style=file

## Python

Python files must obey [PEP8](https://www.python.org/dev/peps/pep-0008/). To make sure your code is compliant to PEP8 run:

    autopep8 -i -a -a --global-config /foo path-to-python-file

Make sure to use the default PEP8, namely use `--global-config` and let it point to a *non-existing file*. Note that `-a -a` is required to force some line breaks.

One could again use something like this to ensure compliance with PEP8:

    git diff master --name-only | grep -i .py | xargs autopep8 -i -a -a --global-config /foo

Downloading binaries for Linux or Windows
=========================================

You can also download a binary version of Voxie for Linux or Windows (64-bit)
from <https://github.com/voxie-viewer/voxie/releases>.

The Linux binaries in `voxie-...-lin64.tar.gz` require Qt5, the OpenCL ICD and
DBus to be available. To install Qt5 and the OpenCL ICD under Ubuntu, type:

    sudo apt-get install libqt5script5 libqt5opengl5 libqt5x11extras5 ocl-icd-libopencl1

The Windows binaries in `voxie-...-win64.zip` have all required libraries
bundled.

The MacOS binaries in `voxie-...-mac64.dmg` are alpha quality and might not
work.

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
