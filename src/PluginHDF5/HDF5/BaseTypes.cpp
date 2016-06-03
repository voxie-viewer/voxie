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

#include "BaseTypes.hpp"

#include <HDF5/File.hpp>
#include <HDF5/Object.hpp>
#include <HDF5/DataSet.hpp>

namespace HDF5 {
  H5O_type_t ObjectReference::getType (const File& file) const {
    H5O_type_t type;
    ObjectReference cpy = *this;
    Exception::check ("H5Rget_obj_type", H5Rget_obj_type (file.handle (), H5R_OBJECT, &cpy.value (), &type));
    return type;
  }

  std::string ObjectReference::getName (const File& file) const {
    ObjectReference cpy = *this;
    ssize_t size = Exception::check ("H5Rget_name", H5Rget_name (file.handle (), H5R_OBJECT, &cpy.value (), NULL, 0));
    std::vector<char> name (size + 1);
    ssize_t size2 = Exception::check ("H5Rget_name", H5Rget_name (file.handle (), H5R_OBJECT, &cpy.value (), name.data (), size + 1));
    ASSERT (size == size2);
    ASSERT (!name[size]);
    return std::string (name.data (), size);
  }

  Object ObjectReference::dereference (const File& file) const {
    ASSERT (!isNull ());

    //return Object (Exception::check ("H5Rdereference", H5Rdereference (file.handle (), H5R_OBJECT, &value ()))); // doesn't work because H5Rdereference has non-const pointer argument
    ObjectReference cpy = *this;
#if H5_VERSION_GE (1, 10, 0)
    //return Object (Exception::check ("H5Rdereference", H5Rdereference (file.handle (), H5P_DEFAULT, H5R_OBJECT, &cpy.value ())));
    return Object (Exception::check ("H5Rdereference", H5Rdereference (file.handle (), setEFilePrefix ()/*TODO: pass as argument?*/.handle (), H5R_OBJECT, &cpy.value ())));
#else
    /* // Does not work for anonymous datasets
    if (getType (file) == H5O_TYPE_DATASET) {
      return DataSet (Exception::check ("H5Dopen", H5Dopen (file.handle (), getName (file).c_str (), setEFilePrefix ().handle ())));
    } else {
    */
    return Object (Exception::check ("H5Rdereference", H5Rdereference (file.handle (), H5R_OBJECT, &cpy.value ())));
#endif
  }
}
