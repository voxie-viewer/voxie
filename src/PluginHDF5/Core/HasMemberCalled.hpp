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

#ifndef CORE_HASMEMBERCALLED_HPP_INCLUDED
#define CORE_HASMEMBERCALLED_HPP_INCLUDED

// CORE_DEFINE_HAS_MEMBER_CALLED(foo) will define a type HasMemberCalled_foo<T> in the current namespace which will derive from boost::mpl::bool_<true> if T contains a member called "foo" and from boost::mpl::bool_<false> otherwise.
#include <boost/mpl/bool.hpp>
#include <boost/type_traits/is_class.hpp>

// Based on boost=1.46.1-5ubuntu3 /usr/include/boost/thread/locks.hpp
#define CORE_DEFINE_HAS_MEMBER_CALLED(MemberName)                       \
  namespace Core_HasMemberCalled_Impl {                                 \
    template<typename T, bool=boost::is_class<T>::value>                \
    struct HasMemberCalledImpl_##MemberName {                           \
      BOOST_STATIC_CONSTANT(bool, value=false);                         \
    };                                                                  \
                                                                        \
    template<typename T>                                                \
    struct HasMemberCalledImpl_##MemberName<T,true> {                   \
      typedef char true_type;                                           \
      struct false_type {                                               \
        true_type dummy[2];                                             \
      };                                                                \
                                                                        \
      struct fallback { int MemberName; };                              \
      struct derived : T, fallback {                                    \
        derived();                                                      \
      };                                                                \
                                                                        \
      template<int fallback::*> struct tester;                          \
                                                                        \
      template<typename U>                                              \
      static false_type has_member(tester<&U::MemberName>*);            \
      template<typename U>                                              \
      static true_type has_member(...);                                 \
                                                                        \
      BOOST_STATIC_CONSTANT(bool, value=sizeof(has_member<derived>(0))==sizeof(true_type)); \
    };                                                                  \
  }                                                                     \
  template <typename T>                                                 \
  struct HasMemberCalled_##MemberName : boost::mpl::bool_<Core_HasMemberCalled_Impl::HasMemberCalledImpl_##MemberName<T>::value> {}


#endif // !CORE_HASMEMBERCALLED_HPP_INCLUDED
