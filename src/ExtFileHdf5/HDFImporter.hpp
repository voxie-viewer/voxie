/*
 * Copyright (c) 2014-2022 The Voxie Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <ExtFileHdf5/CT/DataFiles.hpp>
#include <ExtFileHdf5/CT/rawdatafiles.hpp>
#include <ExtFileHdf5/CT/typefiles.hpp>

#include <VoxieClient/ClaimedOperation.hpp>
#include <VoxieClient/DataTypeExt.hpp>
#include <VoxieClient/Exception.hpp>

#include <hdf5.h>

using namespace vx;

// Can throw arbitrary exceptions
std::tuple<vx::RefObjWrapper<de::uni_stuttgart::Voxie::Data>,
           vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
loadVoxelData(
    vx::DBusClient& dbusClient, const HDF5::File& file,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationImport>&
        op);
// Can throw arbitrary exceptions
std::tuple<vx::RefObjWrapper<de::uni_stuttgart::Voxie::Data>,
           vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
loadRawData(
    vx::DBusClient& dbusClient, const HDF5::File& file,
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationImport>&
        op);

std::tuple<vx::RefObjWrapper<de::uni_stuttgart::Voxie::Data>,
           vx::RefObjWrapper<de::uni_stuttgart::Voxie::DataVersion>>
import(vx::DBusClient& dbusClient,
       vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationImport>&
           op);

vx::DataTypeExt convertHDF5TypeToDataType(HDF5::DataType type);
