#include <ExtFilterMarchingCubes33_by_DVega/marchingcubes33.h>

#ifndef compiling_libMC33

float invSqrt(float x) {
  union {
    float x;
    int i;
  } u;
  u.x = x;
  x *= 0.5f;
  u.i = 0x5f375a86 - (u.i >> 1);
  /* The next line can be repeated any number of times to increase accuracy */
  u.x = u.x * (1.5f - x * u.x * u.x);
  return u.x;
}

#ifndef GRD_orthogonal
// c = Ab, A is a 3x3 upper triangular matrix. If t != 0, A is transposed.
void _multTSA_bf(const double (*A)[3], MC33_real* b, MC33_real* c, int t) {
  if (t) {
    c[2] = A[0][2] * b[0] + A[1][2] * b[1] + A[2][2] * b[2];
    c[1] = A[0][1] * b[0] + A[1][1] * b[1];
    c[0] = A[0][0] * b[0];
  } else {
    c[0] = A[0][0] * b[0] + A[0][1] * b[1] + A[0][2] * b[2];
    c[1] = A[1][1] * b[1] + A[1][2] * b[2];
    c[2] = A[2][2] * b[2];
  }
}
// Performs the multiplication of the matrix A and the vector b: c = Ab. If t !=
// 0, A is transposed.
void _multA_bf(const double (*A)[3], MC33_real* b, MC33_real* c, int t) {
  double u, v;
  if (t) {
    u = A[0][0] * b[0] + A[1][0] * b[1] + A[2][0] * b[2];
    v = A[0][1] * b[0] + A[1][1] * b[1] + A[2][1] * b[2];
    c[2] = A[0][2] * b[0] + A[1][2] * b[1] + A[2][2] * b[2];
  } else {
    u = A[0][0] * b[0] + A[0][1] * b[1] + A[0][2] * b[2];
    v = A[1][0] * b[0] + A[1][1] * b[1] + A[1][2] * b[2];
    c[2] = A[2][0] * b[0] + A[2][1] * b[1] + A[2][2] * b[2];
  }
  c[0] = u;
  c[1] = v;
}
#endif  // GRD_orthogonal
#endif  // compiling_libMC33
