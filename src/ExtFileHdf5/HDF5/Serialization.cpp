/*
 * Copyright (c) 2010-2012 Steffen Kieß
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

#include "Serialization.hpp"

namespace HDF5 {
SerializationContext::SerializationContext(const HDF5::File& file)
    : file(file), id(0) {}

SerializationContext::~SerializationContext() {}

DeserializationContext::DeserializationContext(const HDF5::File& file)
    : file(file) {}

DeserializationContext::~DeserializationContext() {}

HDF5::DataSet SerializationContextHandle::createDataSet(
    const HDF5::DataType& data_type, const HDF5::DataSpace& data_space,
    const DataSetCreatePropList& dcpl) const {
  std::string name = context().newName();
  ASSERT(context().references.count(key()) == 0);
  HDF5::DataSet ds = HDF5::DataSet::create(context().file, data_type,
                                           data_space, dcpl, setEFilePrefix());
  context().references[key()] = ds.reference();
  context().getRefGroup().link(name, ds);
  return ds;
}
}  // namespace HDF5
