/*
 * Copyright (c) 2016 Steffen Kieß
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

#ifndef HDF5_FORWARD_HPP_INCLUDED
#define HDF5_FORWARD_HPP_INCLUDED

namespace HDF5 {
class Exception;
class ObjectReference;

class IdComponent;

class Attribute;
class File;
class DataSpace;

class Object;
class Group;
class DataSet;

class DataType;
class AtomicType;
class CompoundType;
class OpaqueType;
class ReferenceType;

class PropList;

class MatlabSerializationContext;
class MatlabDeserializationContext;
class MatlabSerializationContextHandle;
template <typename T>
class MatlabDeserializationContextHandle;

template <typename T>
struct MatlabSerializer;
}  // namespace HDF5

#endif  // !HDF5_FORWARD_HPP_INCLUDED
