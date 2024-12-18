PiecewisePolynomialFunction property type
=========================================

The property type
`de.uni_stuttgart.Voxie.PropertyType.PiecewisePolynomialFunction` describes a
function (or distribution) which is described by a series of polynomial
functions which are separated by breakpoints. The value at the breakpoints
can be infinite, in which case the integral across the breakpoints can be
non-zero.

DBus signature
--------------

The DBus signature of the type is `(a(d(dd))aad)`.

This consists of a list of breakpoints and a list of intervals.

### List of breakpoints

The list of breakpoints (`a(d(dd))`) lists the breakpoints which separate the
intervals. This list can be empty.

Each breakpoint contains:
- The position of the breakpoint
- A struct with:
  - The value of the breakpoint, which can be finite, positive or negative infinity, or NaN
  - The integral across the breakpoint. If the value is finite, the integral must be zero. If the value is negative infinity, the integral must be zero, negative or NaN. If the value is positive infinity, the integral must be zero, positive or NaN. If the value is NaN, the integral can be anything.

### List of intervals

The list of intervals (`aad`) has always one entry more than the list of
breakpoints and therefore cannot be empty. Each entry (`ad`) contains a list of
[Bernstein coefficients][Bernstein] describing a polynomial function in the
interval. The degree of the polynomial function is one less than the number of
entries, and the minimum degree is 0, i.e. the list must not be empty.

If there is only one interval, that interval goes from 0 to 1. Otherwise,
the first interval has a length of 1 and ends at the first breakpoint and the
last interval has a length of 1 and starts at the last breakpoint. All other
intervals go from one breakpoint to the next.

[Bernstein]: https://en.wikipedia.org/wiki/Bernstein_polynomial
