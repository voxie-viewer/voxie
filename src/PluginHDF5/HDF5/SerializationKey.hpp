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

#ifndef HDF5_SERIALIZATIONKEY_HPP_INCLUDED
#define HDF5_SERIALIZATIONKEY_HPP_INCLUDED

// Serialization key used by Serialization.hpp and Matlab.hpp

#include <HDF5/BaseTypes.hpp>

#include <memory>
#include <memory>
#include <boost/type_traits/remove_cv.hpp>

#include <typeinfo>

#define HDF5_ALLOW_CYCLES_WITHOUT_SHAREDPTR 0
// For HDF5_ALLOW_CYCLES_WITHOUT_SHAREDPTR=0: The cycle detection code works only when the object is passed in as a std::shared_ptr<>
// For HDF5_ALLOW_CYCLES_WITHOUT_SHAREDPTR=1: If an object which gets serialized is destroyed during serialization and another object is created at the same memory address, this class will fail (i.e. it will consider the second object as the same as the first object). Therefore all objects which are serialized should live until the end of the serialization.

namespace HDF5 {
  // Used to identify a serialized object
  // The type is needed to distinguish between a structure and its first field, and for some objects where the serialization is implemented using another serializer
  class SerializationKey {
    const std::type_info* type_;
#if !HDF5_ALLOW_CYCLES_WITHOUT_SHAREDPTR
    std::weak_ptr<const void> weakPtr_;
#else
    const void* ptr_;
#endif

    template <typename T>
    struct GetWeakPointer {
      static std::weak_ptr<const void> get (UNUSED const T& t) {
        return std::weak_ptr<const void> (std::make_shared<int> (0)); // Create a unique weak_ptr
      }
    };
    template <typename T>
    struct GetWeakPointer<std::shared_ptr<T> > {
      static std::weak_ptr<const void> get (const std::shared_ptr<T>& t) {
        return std::weak_ptr<const void> (t);
      }
    };

  public:
    SerializationKey (const std::type_info* type,
#if !HDF5_ALLOW_CYCLES_WITHOUT_SHAREDPTR
                      const std::weak_ptr<const void>& weakPtr
#else
                      const void* ptr
#endif
                      )
      : type_ (type),
#if !HDF5_ALLOW_CYCLES_WITHOUT_SHAREDPTR
        weakPtr_ (weakPtr)
#else
        ptr_ (ptr)
#endif
    {
    }

    template <typename T> static SerializationKey create (T& val) {
      return SerializationKey (&typeid (T),
#if !HDF5_ALLOW_CYCLES_WITHOUT_SHAREDPTR
                               GetWeakPointer<typename boost::remove_cv<T>::type>::get (val)
#else
                               &val
#endif
                               );
    }

    bool operator< (const SerializationKey& other) const {
      if (type_ < other.type_)
        return true;
      else if (other.type_ < type_)
        return false;
#if !HDF5_ALLOW_CYCLES_WITHOUT_SHAREDPTR
      else
        return weakPtr_.owner_before (other.weakPtr_);
#else
      else
        return ptr_ < other.ptr_;
#endif
    }
  };

  class DeserializationKey {
    const std::type_info* type_;
    ObjectReference ref_;

  public:
    DeserializationKey (const std::type_info* type, ObjectReference ref) : type_ (type), ref_ (ref) {}

    template <typename T> static DeserializationKey create (ObjectReference ref) {
      return DeserializationKey (&typeid (T), ref);
    }

    bool operator< (const DeserializationKey& other) const {
      if (type_ < other.type_)
        return true;
      else if (type_ > other.type_)
        return false;
      else
        return ref_ < other.ref_;
    }
  };
}

#endif // !HDF5_SERIALIZATIONKEY_HPP_INCLUDED
