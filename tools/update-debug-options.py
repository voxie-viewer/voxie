#!/usr/bin/python3
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

import sys
import glob
import os
import json
import codecs
import subprocess
import io

import property_types
import codegen_utils

debug_options = {
    'VoxieBackend': {
        'Log.SurfaceBoundingBox': {'Type': 'bool'},
    },
    'Voxie': {
        'Log.View3DUpdates': {'Type': 'bool'},
        # },
        # TODO: This should be in PluginVis3D (but then getting the list of all debug options in Main has to work differently)
        # 'PluginVis3D': {
        'Log.Vis3D.MouseTracking': {'Type': 'bool'},
        'Log.Vis3D.CreateModifiedSurface': {'Type': 'bool'},
    },
    'Main': {
        'Log.QtEvents': {'Type': 'bool'},
        'Log.FocusChanges': {'Type': 'bool'},
        'Log.HelpPageCache': {'Type': 'bool'},
    },
}

os.chdir(os.path.join(os.path.dirname(sys.argv[0]), '..'))


def escapeCppString(str):
    b = bytes(str, 'utf-8')
    s = '"'
    for c in b:
        if c >= 32 and c < 127 and c != '"':
            s += chr(c)
        else:
            s += '\\{0:03o}'.format(c)
    s += '"'
    return s


def escape_name(name):
    return name.replace('.', '_')


def getDebugOptionType(ty):
    if ty == 'bool':
        return 'vx::DebugOptionBool'
    else:
        raise Exception('Unknown type for debug option: {!r}'.format(ty))


for module in sorted(debug_options):
    options = debug_options[module]
    out_prefix = 'src/' + module + '/DebugOptions'

    exportStr = ''
    # TODO: Make this more generic?
    if module == 'VoxieClient':
        exportStr = 'VOXIECLIENT_EXPORT '
    elif module == 'VoxieBackend':
        exportStr = 'VOXIEBACKEND_EXPORT '
    elif module == 'Voxie':
        exportStr = 'VOXIECORESHARED_EXPORT '

    with open(out_prefix + '.hpp.new', 'w') as hpp, open(out_prefix + '.cpp.new', 'w') as cpp:
        for file in [hpp, cpp]:
            file.write(
                '// This file was automatically generated by tools/update-debug-options.py\n')
            file.write('// All changes to this file will be lost\n')
            file.write('\n')
        hpp.write('#pragma once\n')
        hpp.write('\n')
        hpp.write('#include <VoxieClient/DebugOption.hpp>\n')
        hpp.write('\n')
        if exportStr != '':
            hpp.write('#include <{}/{}.hpp>\n'.format(module, module))
        hpp.write('\n')
        cpp.write('#include "DebugOptions.hpp"\n')
        cpp.write('\n')
        hpp.write('namespace vx {\n')
        hpp.write('namespace debug_option {\n')
        cpp.write('namespace vx {\n')
        cpp.write('namespace debug_option_impl {\n')

        option_names = sorted(options)
        for option_name in option_names:
            option = options[option_name]
            esc_name = escape_name(option_name)
            ty = option['Type']
            do_type = getDebugOptionType(ty)

            hpp.write('{}{}* {}();'.format(exportStr, do_type, esc_name))
            cpp.write('{} {}_option({});'.format(do_type, esc_name, escapeCppString(option_name)))

        hpp.write('}\n')
        hpp.write('\n')
        cpp.write('}\n')
        cpp.write('}\n')
        cpp.write('\n')

        for option_name in option_names:
            option = options[option_name]
            esc_name = escape_name(option_name)
            ty = option['Type']

            cpp.write('{}* vx::debug_option::{}() {{'.format(do_type, esc_name))
            cpp.write('return &vx::debug_option_impl::{}_option;'.format(esc_name))
            cpp.write('}\n')

        cpp.write('\n')
        hpp.write('{}QList<vx::DebugOption*> get{}DebugOptions();'.format(exportStr, module))
        cpp.write('QList<vx::DebugOption*> vx::get{}DebugOptions() {{\n'.format(module))
        cpp.write('return {')
        for option_name in option_names:
            option = options[option_name]
            esc_name = escape_name(option_name)
            ty = option['Type']

            cpp.write('vx::debug_option::{}(),'.format(esc_name))
        cpp.write('};\n')
        cpp.write('}\n')

        hpp.write('}\n')
    codegen_utils.formatAndRename(out_prefix + '.hpp')
    codegen_utils.formatAndRename(out_prefix + '.cpp')

all_file = 'src/Main/AllDebugOptions.cpp'
with open(all_file + '.new', 'w') as cpp:
    cpp.write('#include "AllDebugOptions.hpp"\n')
    cpp.write('\n')
    for module in sorted(debug_options):
        cpp.write('#include <{}/DebugOptions.hpp>\n'.format(module))
    cpp.write('\n')
    cpp.write('static QList<vx::DebugOption*> getAllDebugOptions() {\n')
    cpp.write('QList<vx::DebugOption*> result;\n')
    for module in sorted(debug_options):
        cpp.write('result << vx::get{}DebugOptions();\n'.format(module))
    cpp.write('return result;\n')
    cpp.write('}\n')
    cpp.write('\n')
    cpp.write('''QSharedPointer<QList<vx::DebugOption*>> vx::allDebugOptions() {
      static QSharedPointer<QList<vx::DebugOption*>> cache =
          createQSharedPointer<QList<vx::DebugOption*>>(
              std::move(getAllDebugOptions()));
      return cache;
    }\n''')
codegen_utils.formatAndRename(all_file)
