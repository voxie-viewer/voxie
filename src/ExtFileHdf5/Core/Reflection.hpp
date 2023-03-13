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

#ifndef CORE_REFLECTION_HPP_INCLUDED
#define CORE_REFLECTION_HPP_INCLUDED

#include <Core/HasMemberCalled.hpp>
#include <Core/Util.hpp>

#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/list.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>

#include <boost/mpl/map.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/insert.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/at.hpp>

#include <boost/mpl/pop_front.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/empty.hpp>

#include <boost/static_assert.hpp>

namespace Core {
  template <typename T>
  struct Metatype : public T::Core_Reflection_Metatype {};

  template <typename T, typename F, F T::* member>
  struct PointerToMember {
    typedef T DeclaringType;
    typedef F ElementType;
    typedef ElementType DeclaringType::* MemberType;
    static MemberType get () {
      return member;
    }
  };

  namespace Intern {
    struct MemberListEmpty {};

    template <bool value, typename T>
    struct StaticAssertionFailureInfo;
    template <typename T>
    struct StaticAssertionFailureInfo<true, T> {};

    template <std::size_t size>
    struct StaticAssertionInst {};

    template <typename T>
    struct GetMemberMap {
      typedef typename GetMemberMap<typename T::Next>::Type NextMap;
      typedef typename T::Info::Pointer Pointer;
      //BOOST_STATIC_ASSERT (!(boost::mpl::has_key<NextMap, Pointer>::value));
      typedef StaticAssertionInst<sizeof (StaticAssertionFailureInfo<!(boost::mpl::has_key<NextMap, Pointer>::value), Pointer>)> StaticAssert;
      typedef typename boost::mpl::insert<NextMap, boost::mpl::pair<Pointer, T> >::type Type;
    };
    template <>
    struct GetMemberMap<MemberListEmpty> {
      typedef boost::mpl::map<> Type;
    };

    template <typename T>
    struct GetMemberVector {
      typedef typename GetMemberVector<typename T::Next>::Type NextVector;
      typedef typename boost::mpl::push_back<NextVector, typename T::Info>::type Type;
    };
    template <>
    struct GetMemberVector<MemberListEmpty> {
      typedef boost::mpl::vector<> Type;
    };

    CORE_DEFINE_HAS_MEMBER_CALLED (Core_Reflection_Metatype);
  }

  template <typename T>
  struct HasMetatype : Intern::HasMemberCalled_Core_Reflection_Metatype<T> {};
}

// See http://stackoverflow.com/questions/2308243/macro-returning-the-number-of-arguments-it-is-given-in-c
#define CORE_REFLECTION_PP_NARG(...)                                 \
  CORE_REFLECTION_PP_NARG_(__VA_ARGS__,CORE_REFLECTION_PP_RSEQ_N())
#define CORE_REFLECTION_PP_NARG_(...)                \
  CORE_REFLECTION_PP_ARG_N(__VA_ARGS__)
#define CORE_REFLECTION_PP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9,_10,     \
                            _11,_12,_13,_14,_15,_16,_17,_18,_19,_20,    \
                            _21,_22,_23,_24,_25,_26,_27,_28,_29,_30,    \
                            _31,_32,_33,_34,_35,_36,_37,_38,_39,_40,    \
                            _41,_42,_43,_44,_45,_46,_47,_48,_49,_50,    \
                            _51,_52,_53,_54,_55,_56,_57,_58,_59,_60,    \
                            _61,_62,_63,_64,_65,_66,  N, ...) N
#define CORE_REFLECTION_PP_RSEQ_N()                  \
    66,65,64,63,62,61,60,                            \
    59,58,57,56,55,54,53,52,51,50,                   \
    49,48,47,46,45,44,43,42,41,40,                   \
    39,38,37,36,35,34,33,32,31,30,                   \
    29,28,27,26,25,24,23,22,21,20,                   \
    19,18,17,16,15,14,13,12,11,10,                   \
    9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define CORE_REFLECTION_FWDDECL(r, data, i, elem) struct Member_##i;
#define CORE_REFLECTION_DECL(r, data, i, elem)                          \
  BOOST_PP_IF                                                           \
  (i,                                                                   \
   struct Member_##i {                                                  \
    typedef BOOST_PP_CAT(Member_, BOOST_PP_ADD(i, 1)) Next;             \
    struct Info {                                                       \
      static const char* name () { return BOOST_PP_STRINGIZE(elem); }   \
      typedef DECLTYPE(::Core::PointerToMember<OriginalType, DECLTYPE(OriginalType::elem), &OriginalType::elem>()) Pointer; /* The decltype() stuff is needed to prevent the preprocessor from considering the commas as argument separators */ \
    };                                                                  \
   };                                                                   \
   ,                                                                    \
   )


#define CORE_REFLECTION_TUPLE(size, tuple)                              \
  class Core_Reflection_Metatype {                                      \
  public:                                                               \
  typedef BOOST_PP_TUPLE_ELEM(size, 0, tuple) OriginalType;             \
  private:                                                              \
  typedef ::Core::Intern::MemberListEmpty BOOST_PP_CAT(Member_, size);  \
  BOOST_PP_LIST_FOR_EACH_I(CORE_REFLECTION_FWDDECL, -, BOOST_PP_TUPLE_TO_LIST(size, tuple)) \
  BOOST_PP_LIST_FOR_EACH_I(CORE_REFLECTION_DECL, -, BOOST_PP_TUPLE_TO_LIST(size, tuple)) \
  public:                                                               \
  typedef typename ::Core::Intern::GetMemberMap<Member_1>::Type Map;    \
  typedef typename ::Core::Intern::GetMemberVector<Member_1>::Type Members; \
  static const char* name () { return BOOST_PP_STRINGIZE(BOOST_PP_TUPLE_ELEM(size, 0, tuple)); } \
  };

#define CORE_REFLECTION(...) CORE_REFLECTION_TUPLE(CORE_REFLECTION_PP_NARG (__VA_ARGS__), (__VA_ARGS__))

#endif // !CORE_REFLECTION_HPP_INCLUDED
