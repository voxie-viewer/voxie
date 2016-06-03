TEMPLATE = subdirs

FOO = bar

CONFIG += ordered

SUBDIRS += \
	Voxie \
	Main \
	PluginVisSlice \
	PluginVisDiff \
	PluginVis3D \
	PluginFilter \
	PluginHDF5 \
	PluginExample \
        ScriptGetAverage \
	extra
