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

#include <VoxieClient/Vector.hpp>

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
// This file contains for loops like this (where dim is a template parameter):
// for (size_t i = 0; i < dim; i++)
// On gcc 10.2.1 this will cause -Wtype-limits errors when the function is
// instantiated with dim=0.
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

namespace vx {
namespace intern {

// This base class is needed to create constructors with the correct number of
// arguments
template <typename T, std::size_t columnCount, typename... U>
class MatrixBase {
  // Note: This must not be called 'rowCount' because at least MSVC2017 and
  // MSVC2019 don't like it if the template parameter for Matrix is called the
  // same as the class member here (this would cause msvcTest1() in
  // MatrixTest.cpp to fail with:
  // error C2662: 'unknown-type
  // &vx::Matrix<quint64,3,4>::asNestedArrayRowMajor(void)': cannot convert
  // 'this' pointer from 'const vx::Matrix<quint64,3,4>' to
  // 'vx::Matrix<quint64,3,4> &'
  static constexpr std::size_t rowCount_ = sizeof...(U);

 protected:
  std::array<std::array<T, columnCount>, rowCount_> data_;

 public:
  MatrixBase() = default;
  MatrixBase(const vx::Vector<U, columnCount>&... par)
      : data_{par.asArray()...} {}
};
// Specialization for 0-row matrix
template <typename T, std::size_t columnCount>
class MatrixBase<T, columnCount> {
 protected:
  std::array<std::array<T, columnCount>, 0> data_;

 public:
  MatrixBase() = default;
};
// Specialization for 1-row matrix
template <typename T, std::size_t columnCount>
class MatrixBase<T, columnCount, T> {
 protected:
  std::array<std::array<T, columnCount>, 1> data_;

 public:
  MatrixBase() = default;
  MatrixBase(const vx::Vector<T, columnCount>& par) : data_{{par.asArray()}} {}
};

template <typename T, std::size_t columnCount, std::size_t i, typename... U>
struct MatrixBaseType : MatrixBaseType<T, columnCount, i - 1, T, U...> {};
template <typename T, std::size_t columnCount, typename... U>
struct MatrixBaseType<T, columnCount, 0, U...> {
  using type = MatrixBase<T, columnCount, U...>;
};

}  // namespace intern

template <typename T, std::size_t rowCount, std::size_t columnCount = rowCount>
class Matrix : vx::intern::MatrixBaseType<T, columnCount, rowCount>::type {
  using BaseType =
      typename vx::intern::MatrixBaseType<T, columnCount, rowCount>::type;
  using BaseType::BaseType;

 public:
  const std::array<std::array<T, columnCount>, rowCount>&
  asNestedArrayRowMajor() const {
    return this->data_;
  }
  std::array<std::array<T, columnCount>, rowCount>& asNestedArrayRowMajor() {
    return this->data_;
  }

  const T& operator()(size_t row, size_t column) const {
    return asNestedArrayRowMajor()[row][column];
  }
  T& operator()(size_t row, size_t column) {
    return asNestedArrayRowMajor()[row][column];
  }

  template <std::size_t row, std::size_t column>
  const T& access() const {
    static_assert(row < rowCount, "Row index out of range");
    static_assert(column < columnCount, "Column index out of range");
    return asNestedArrayRowMajor()[row][column];
  }
  template <std::size_t row, std::size_t column>
  T& access() {
    static_assert(row < rowCount, "Row index out of range");
    static_assert(column < columnCount, "Column index out of range");
    return asNestedArrayRowMajor()[row][column];
  }

  friend Matrix operator+(const Matrix& m) { return m; }
  friend Matrix operator-(const Matrix& m) {
    Matrix ret;
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++)
        ret(row, col) = -m(row, col);
    return ret;
  }

  friend Matrix operator+(const Matrix& m1, const Matrix& m2) {
    Matrix ret;
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++)
        ret(row, col) = m1(row, col) + m2(row, col);
    return ret;
  }
  friend Matrix& operator+=(Matrix& m1, const Matrix& m2) {
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++)
        m1(row, col) += m2(row, col);
    return m1;
  }

  friend Matrix operator-(const Matrix& m1, const Matrix& m2) {
    Matrix ret;
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++)
        ret(row, col) = m1(row, col) - m2(row, col);
    return ret;
  }
  friend Matrix& operator-=(Matrix& m1, const Matrix& m2) {
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++)
        m1(row, col) -= m2(row, col);
    return m1;
  }

  friend Matrix operator*(T s, const Matrix& m) {
    Matrix ret;
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++)
        ret(row, col) = s * m(row, col);
    return ret;
  }
  friend Matrix operator*(const Matrix& m, T s) {
    Matrix ret;
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++)
        ret(row, col) = m(row, col) * s;
    return ret;
  }
  friend Matrix& operator*=(Matrix& m, T s) {
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++) m(row, col) *= s;
    return m;
  }

  // This is a template to prevent it being instantiated for non-division-ring
  // types
  template <typename AllowNonDivisionRing = std::false_type>
  friend Matrix operator/(const Matrix& m, T s) {
    // TODO: Should this be allowed?
    static_assert(
        vx::RingTraits<T>::isDivisionRing || AllowNonDivisionRing::value,
        "operator/ cannot be used for vector module over non-division "
        "ring");

    Matrix ret;
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++)
        ret(row, col) = m(row, col) / s;
    return ret;
  }
  // This is a template to prevent it being instantiated for non-division-ring
  // types
  template <typename AllowNonDivisionRing = std::false_type>
  friend Matrix& operator/=(Matrix& m, T s) {
    // TODO: Should this be allowed?
    static_assert(
        vx::RingTraits<T>::isDivisionRing || AllowNonDivisionRing::value,
        "operator/= cannot be used for vector module over non-division "
        "ring");

    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++) m(row, col) /= s;
    return m;
  }

  friend bool operator==(const Matrix& m1, const Matrix& m2) {
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++)
        if (m1(row, col) != m2(row, col)) return false;
    return true;
  }
  friend bool operator!=(const Matrix& m1, const Matrix& m2) {
    return !(m1 == m2);
  }

  // Matrix-vector multiplication
  friend Vector<T, rowCount> operator*(const Matrix& m,
                                       const Vector<T, columnCount>& v) {
    Vector<T, rowCount> ret;
    for (size_t resultRow = 0; resultRow < rowCount; resultRow++) {
      // TODO: Use double instead of T?
      T sum = 0;
      for (size_t pos = 0; pos < columnCount; pos++)
        sum += m(resultRow, pos) * v[pos];
      ret(resultRow) = sum;
    }
    return ret;
  }

  friend std::ostream& operator<<(std::ostream& o, const Matrix& m) {
    o << "(\n";
    for (size_t row = 0; row < rowCount; row++) {
      o << "  ";
      for (size_t col = 0; col < columnCount; col++) {
        if (col != 0) o << " ";
        o << m(row, col);
      }
      o << "\n";
    }
    o << ")";
    return o;
  }

#if VX_VECTOR_ENABLE_QDEBUG
  friend QDebug operator<<(QDebug debug, const Matrix& m) {
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "(\n";
    for (size_t row = 0; row < rowCount; row++) {
      debug << "  ";
      for (size_t col = 0; col < columnCount; col++) {
        if (col != 0) debug << " ";
        debug << m(row, col);
      }
      debug << "\n";
    }
    debug << ")";
    return debug;
  }
#endif

  Matrix<T, columnCount, rowCount> transposed() const {
    Matrix<T, columnCount, rowCount> ret;
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++)
        ret(col, row) = (*this)(row, col);
    return ret;
  }

  static Matrix zero() {
    Matrix ret;
    for (size_t row = 0; row < rowCount; row++)
      for (size_t col = 0; col < columnCount; col++) ret(row, col) = 0;
    return ret;
  }
};

// Matrix multiplication
template <typename T, size_t resRowCount, size_t count, size_t resColCount>
Matrix<T, resRowCount, resColCount> operator*(
    const Matrix<T, resRowCount, count>& m1,
    const Matrix<T, count, resColCount>& m2) {
  Matrix<T, resRowCount, resColCount> ret;
  for (size_t resultRow = 0; resultRow < resRowCount; resultRow++) {
    for (size_t resultCol = 0; resultCol < resColCount; resultCol++) {
      // TODO: Use double instead of T?
      T sum = 0;
      for (size_t pos = 0; pos < count; pos++)
        sum += m1(resultRow, pos) * m2(pos, resultCol);
      ret(resultRow, resultCol) = sum;
    }
  }
  return ret;
}
// Only works for square matrices
template <typename T, size_t count>
Matrix<T, count, count>& operator*=(Matrix<T, count, count>& m,
                                    const Matrix<T, count, count>& m2) {
  auto result = m * m2;
  return m = result;
}

template <typename To, typename From, std::size_t rowCount,
          std::size_t columnCount>
inline std::enable_if_t<IsConvertibleWithoutNarrowing<From, To>::value,
                        Matrix<To, rowCount, columnCount>>
matrixCast(const Matrix<From, rowCount, columnCount>& m) {
  Matrix<To, rowCount, columnCount> ret;
  for (size_t row = 0; row < rowCount; row++)
    for (size_t col = 0; col < columnCount; col++) ret(row, col) = m(row, col);
  return ret;
}

template <typename To, typename From, std::size_t rowCount,
          std::size_t columnCount>
inline std::enable_if_t<std::is_convertible<From, To>::value,
                        Matrix<To, rowCount, columnCount>>
matrixCastNarrow(const Matrix<From, rowCount, columnCount>& m) {
  Matrix<To, rowCount, columnCount> ret;
  for (size_t row = 0; row < rowCount; row++)
    for (size_t col = 0; col < columnCount; col++)
      ret(row, col) = static_cast<To>(m(row, col));
  return ret;
}

template <typename T, std::size_t rowCount, std::size_t columnCount>
inline T squaredNorm(const Matrix<T, rowCount, columnCount>& m) {
  T sum = 0;
  for (size_t row = 0; row < rowCount; row++)
    for (size_t col = 0; col < columnCount; col++)
      sum += m(row, col) * m(row, col);
  return sum;
}

namespace intern {
template <std::size_t... indices>
struct DeterminantIndices {
  static constexpr std::size_t count = sizeof...(indices);
};

template <std::size_t count, std::size_t... indices>
struct BuildDeterminantIndices
    : BuildDeterminantIndices<count - 1, count - 1, indices...> {};
template <std::size_t... indices>
struct BuildDeterminantIndices<0, indices...> {
  using type = DeterminantIndices<indices...>;
};

// Helper for calculating the determinant
template <typename T, std::size_t count, typename RowIndices,
          typename ColumnIndices>
struct CalcDeterminant {
  static_assert(RowIndices::count > 0, "Got empty row indices list");
  static_assert(ColumnIndices::count > 0, "Got empty column indices list");
  // count == 1 should be handled separately (but this code would also work)
  static_assert(RowIndices::count > 1, "Got only 1 element");
  static_assert(
      RowIndices::count == ColumnIndices::count,
      "Got different number of elements in RowIndices and ColumnIndices");

  template <typename DoInvert, std::size_t indRow, std::size_t... tailRow,
            std::size_t... prevCol, std::size_t indCol, std::size_t... tailCol>
  static void add(const Matrix<T, count, count>& m, T& sum, DoInvert,
                  DeterminantIndices<indRow, tailRow...>,
                  DeterminantIndices<prevCol...>,
                  DeterminantIndices<indCol, tailCol...>) {
    T inner =
        m.template access<indRow, indCol>() *
        CalcDeterminant<T, count, DeterminantIndices<tailRow...>,
                        DeterminantIndices<prevCol..., tailCol...>>::calc(m);
    if (DoInvert::value)
      sum -= inner;
    else
      sum += inner;
    add(m, sum, std::integral_constant<bool, !DoInvert::value>(),
        DeterminantIndices<indRow, tailRow...>(),
        DeterminantIndices<prevCol..., indCol>(),
        DeterminantIndices<tailCol...>());
  }
  template <typename DoInvert, std::size_t... row, std::size_t... prevCol>
  static void add(const Matrix<T, count, count>& m, T& sum, DoInvert,
                  DeterminantIndices<row...>, DeterminantIndices<prevCol...>,
                  DeterminantIndices<>) {
    (void)m;
    (void)sum;
  }

  // Will return the determinant of a submatrix of m
  // - Using all row indices in RowIndices
  // - Using all column indices in ColumnIndices
  static T calc(const Matrix<T, count, count>& m) {
    T sum = 0;
    add(m, sum, std::false_type(), RowIndices(), DeterminantIndices<>(),
        ColumnIndices());
    return sum;
  }
};
// Determinant of 1-element matrix
template <typename T, std::size_t count, std::size_t row, std::size_t column>
struct CalcDeterminant<T, count, DeterminantIndices<row>,
                       DeterminantIndices<column>> {
  static T calc(const Matrix<T, count, count>& m) {
    return m.template access<row, column>();
  }
};
// Determinant of empty matrix
template <typename T, std::size_t count>
struct CalcDeterminant<T, count, DeterminantIndices<>, DeterminantIndices<>> {
  static T calc(const Matrix<T, count, count>& m) {
    (void)m;
    return 1;
  }
};
}  // namespace intern

template <typename T, std::size_t count>
T determinant(const Matrix<T, count, count>& m) {
  // TODO: Use higher precision for calculation?
  using Indices = typename vx::intern::BuildDeterminantIndices<count>::type;
  return vx::intern::CalcDeterminant<T, count, Indices, Indices>::calc(m);
}

namespace intern {
template <typename T, std::size_t count>
struct Invert {
  using AllIndices = typename vx::intern::BuildDeterminantIndices<count>::type;

  template <typename DoInvert, std::size_t row, std::size_t... rows,
            std::size_t... prevCol, std::size_t col, std::size_t... tailCol>
  static void calcCols(Matrix<T, count, count>& output,
                       const Matrix<T, count, count>& input, T invDet, DoInvert,
                       DeterminantIndices<row>, DeterminantIndices<rows...>,
                       DeterminantIndices<prevCol...>,
                       DeterminantIndices<col, tailCol...>) {
    // TODO: Is this correct (swapping columns and rows here)?
    T value =
        CalcDeterminant<T, count, DeterminantIndices<prevCol..., tailCol...>,
                        DeterminantIndices<rows...>>::calc(input);
    value *= invDet;
    if (!DoInvert::value)
      output.template access<row, col>() = value;
    else
      output.template access<row, col>() = -value;
    calcCols(output, input, invDet,
             std::integral_constant<bool, !DoInvert::value>(),
             DeterminantIndices<row>(), DeterminantIndices<rows...>(),
             DeterminantIndices<prevCol..., col>(),
             DeterminantIndices<tailCol...>());
  }
  template <typename DoInvert, std::size_t row, std::size_t... rows,
            std::size_t... prevCol>
  static void calcCols(Matrix<T, count, count>& output,
                       const Matrix<T, count, count>& input, T invDet, DoInvert,
                       DeterminantIndices<row>, DeterminantIndices<rows...>,
                       DeterminantIndices<prevCol...>, DeterminantIndices<>) {
    (void)output;
    (void)input;
    (void)invDet;
  }

  template <typename DoInvert, std::size_t... prevRow, std::size_t row,
            std::size_t... tailRow>
  static void calcRows(Matrix<T, count, count>& output,
                       const Matrix<T, count, count>& input, T invDet, DoInvert,
                       DeterminantIndices<prevRow...>,
                       DeterminantIndices<row, tailRow...>) {
    calcCols(output, input, invDet, DoInvert(), DeterminantIndices<row>(),
             DeterminantIndices<prevRow..., tailRow...>(),
             DeterminantIndices<>(), AllIndices());
    calcRows(output, input, invDet,
             std::integral_constant<bool, !DoInvert::value>(),
             DeterminantIndices<prevRow..., row>(),
             DeterminantIndices<tailRow...>());
  }
  template <typename DoInvert, std::size_t... prevRow>
  static void calcRows(Matrix<T, count, count>& output,
                       const Matrix<T, count, count>& input, T invDet, DoInvert,
                       DeterminantIndices<prevRow...>, DeterminantIndices<>) {
    (void)output;
    (void)input;
    (void)invDet;
  }

  static void calc(Matrix<T, count, count>& output,
                   const Matrix<T, count, count>& input, T invDet) {
    calcRows(output, input, invDet, std::false_type(), DeterminantIndices<>(),
             AllIndices());
  }
};
}  // namespace intern

template <typename T, std::size_t count,
          typename AllowNonDivisionRing = std::false_type>
Matrix<T, count, count> inverse(const Matrix<T, count, count>& m,
                                T* determinantPtr = nullptr) {
  // Needed for calculating "1 / det"
  static_assert(
      vx::RingTraits<T>::isDivisionRing || AllowNonDivisionRing::value,
      "inverse() cannot be used for vector module over non-division "
      "ring");

  T det = determinant(m);

  if (determinantPtr) *determinantPtr = det;

  Matrix<T, count, count> ret;
  vx::intern::Invert<T, count>::calc(ret, m, 1 / det);
  return ret;
}

template <typename T, std::size_t count>
static Matrix<T, count, count> identityMatrix() {
  Matrix<T, count, count> ret;
  for (size_t row = 0; row < count; row++)
    for (size_t col = 0; col < count; col++)
      ret(row, col) = ((row == col) ? 1 : 0);
  return ret;
}

// TODO: Allow more than two arguments for concatRows() / concatColumns() (and
// vx::concat(vx::Vector, ...))?

template <typename T, std::size_t rowCount1, std::size_t rowCount2,
          std::size_t columnCount>
static Matrix<T, rowCount1 + rowCount2, columnCount> concatRows(
    const Matrix<T, rowCount1, columnCount>& m1,
    const Matrix<T, rowCount2, columnCount>& m2) {
  Matrix<T, rowCount1 + rowCount2, columnCount> ret;

  for (size_t row = 0; row < rowCount1; row++)
    for (size_t col = 0; col < columnCount; col++) ret(row, col) = m1(row, col);
  for (size_t row = 0; row < rowCount2; row++)
    for (size_t col = 0; col < columnCount; col++)
      ret(rowCount1 + row, col) = m2(row, col);

  return ret;
}

template <typename T, std::size_t rowCount, std::size_t columnCount1,
          std::size_t columnCount2>
static Matrix<T, rowCount, columnCount1 + columnCount2> concatColumns(
    const Matrix<T, rowCount, columnCount1>& m1,
    const Matrix<T, rowCount, columnCount2>& m2) {
  Matrix<T, rowCount, columnCount1 + columnCount2> ret;

  for (size_t row = 0; row < rowCount; row++)
    for (size_t col = 0; col < columnCount1; col++)
      ret(row, col) = m1(row, col);
  for (size_t row = 0; row < rowCount; row++)
    for (size_t col = 0; col < columnCount2; col++)
      ret(row, columnCount1 + col) = m2(row, col);

  return ret;
}

template <typename T, std::size_t rowCount>
static Matrix<T, rowCount, 1> createColumnVector(
    const vx::Vector<T, rowCount>& v) {
  Matrix<T, rowCount, 1> res;
  for (size_t row = 0; row < rowCount; row++) res(row, 0) = v(row);
  return res;
}
}  // namespace vx
