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

#include <VoxieClient/Optional.hpp>

#include <limits>

// This code is heavily based on
// https://stackoverflow.com/questions/51304323/reliable-overflow-detection-of-floating-point-integer-type-conversion/51323959#51323959

namespace vx {
// Same as static_cast<IntType>(value), but will return vx::nullopt if the cast
// would overflow.

// Broken implementation
template <typename IntType, typename FloatType>
vx::Optional<IntType> castFloatToIntSafeBroken(FloatType value) {
  static_assert(std::numeric_limits<IntType>::is_integer,
                "IntType must be integer type");

  // There are rounding errors during the conversion from IntType to
  // FloatType, so that e.g. for IntType=std::int64_t and FloatType=double,
  // 9223372036854775807 is converted incorrectly.
  if (value < std::numeric_limits<IntType>::min() ||
      value > std::numeric_limits<IntType>::max()) {
    return vx::nullopt;
  }
  return static_cast<IntType>(value);
}

namespace intern {
template <typename IntType, typename FloatType>
class CastFloatToIntSafeLimits {
  static constexpr FloatType getLargestFloatNotLargerThan(IntType x) {
    /*  All references to "digits" in this routine refer to digits in
        base std::numeric_limits<FloatType>::radix.  For example, in base
        3, 77 would have four digits (2212).  Zero is considered to
        have zero digits.

        In this routine, "bigger" and "smaller" refer to magnitude.  (3
        is greater than -4, but -4 is bigger than 3.) */

    //  Abbreviate std::numeric_limits<FloatType>::radix.
    const int Radix = std::numeric_limits<FloatType>::radix;

    //  Determine the sign.
    int s = 0 < x ? +1 : -1;

    //  Count how many digits x has.
    IntType digits = 0;
    for (IntType t = x; t; ++digits) t /= Radix;

    /*  If the FloatType type cannot represent finite numbers this big,
        return the biggest finite number it can hold, with the desired
        sign.
    */
    if (std::numeric_limits<FloatType>::max_exponent < digits)
      return s * std::numeric_limits<FloatType>::max();

    //  Determine whether x is exactly representable in FloatType.
    if (std::numeric_limits<FloatType>::digits < digits) {
      /*  x is not representable, so we will return the next lower
          representable value by removing just as many low digits as
          necessary.  Note that x+s might be representable, but we
          want to return the biggest FloatType less than it, which, in
          this case, is also the biggest FloatType less than x.
      */

      /*  Figure out how many digits we have to remove to leave at
          most std::numeric_limits<FloatType>::digits digits.
      */
      digits = digits - std::numeric_limits<FloatType>::digits;

      //  Calculate Radix to the power of digits.
      IntType t = 1;
      while (digits--) t *= Radix;

      return x / t * t;
    } else {
      /*  x is representable.  To return the biggest FloatType smaller
          than x+s, we will fill the remaining digits with Radix-1.
      */

      //  Figure out how many additional digits FloatType can hold.
      digits = std::numeric_limits<FloatType>::digits - digits;

      /*  Put a 1 in the lowest available digit, then subtract from 1
          to set each digit to Radix-1.  (For example, 1 - .001 =
          .999.)
      */
      FloatType t = 1;
      while (digits--) t /= Radix;
      t = 1 - t;

      //  Return the biggest FloatType smaller than x+s.
      return x + s * t;
    }
  }

 public:
  static FloatType constexpr lowerLimitFun() {
    return getLargestFloatNotLargerThan(std::numeric_limits<IntType>::min());
  }
  static FloatType constexpr upperLimitFun() {
    return getLargestFloatNotLargerThan(std::numeric_limits<IntType>::max());
  }

  // Note: These need a definition before C++17
  static const constexpr FloatType lowerLimit = lowerLimitFun();
  static const constexpr FloatType upperLimit = upperLimitFun();
};
}  // namespace intern

template <typename IntType, typename FloatType>
vx::Optional<IntType> castFloatToIntSafe(FloatType value) {
  static_assert(std::numeric_limits<IntType>::is_integer,
                "IntType must be integer type");

  if (intern::CastFloatToIntSafeLimits<IntType, FloatType>::lowerLimitFun() <=
          value &&
      value <=
          intern::CastFloatToIntSafeLimits<IntType, FloatType>::upperLimitFun())
    return static_cast<IntType>(value);
  else
    return vx::nullopt;
}
}  // namespace vx
