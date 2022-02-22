Command line options
====================

files
-----

Files passed on the command line will be opened by voxie.
Files ending in `.vxprj.py` will be opened as project files.

`--slice`, `--iso`, `--raw`
---------------------------

These options will automatically cause a [slice visualizer] or
[isosurface generation filter] to be opened for each [volume file] specified
on the command line (for `--slice` and `--iso`) or a [raw visualizer] to be
opened for each [raw file] specified on the command line (for `--raw`).


`--import-property`
---------------------------

This option needs an argument of the form `property-name=value-as-json`.
The property `property-name` will be set to the value `value-as-json` when
loading files from the command line. The type of the property is determined
automatically. If the property does not exist, a warning is emitted.


`--no-opengl`
-------------

The `--no-opengl` option disables support for OpenGL. This makes the
[3D visualizer] unusable. This option might be useful if OpenGL is not
available.

Note that currently opening a [3D visualizer] with this option might cause
voxie to crash.


`--no-opencl`
-------------

The `--no-opengl` option disables support for OpenCL. This might be useful
if OpenCL is not available or the OpenCL driver is not working properly.


[3D visualizer]: voxie:///help/prototype/de.uni_stuttgart.Voxie.Visualizer.View3D
[slice visualizer]: voxie:///help/prototype/de.uni_stuttgart.Voxie.Visualizer.Slice
[raw visualizer]: voxie:///help/prototype/de.uni_stuttgart.Voxie.Visualizer.TomographyRawData
[isosurface generation filter]: voxie:///help/prototype/de.uni_stuttgart.Voxie.Filter.CreateSurface
[volume file]: voxie:///help/prototype/de.uni_stuttgart.Voxie.Data.Volume
[raw file]: voxie:///help/prototype/de.uni_stuttgart.Voxie.Data.TomographyRawData


`--main-window`
---------------

Can be set to `--main-window=normal` (default), `--main-window=background`
(main window will be opened in the background) or `--main-window=hidden` (main
window won't be shown at all).


Other options
-------------

See `./voxie --help`.
