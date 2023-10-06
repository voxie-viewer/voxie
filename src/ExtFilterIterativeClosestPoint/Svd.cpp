/*
MIT License

Copyright (c) 2017 Greg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

* This routine is adapted from
* https://github.com/Gregjksmith/Iterative-Closest-Point/tree/master/src/svd.cpp
*/

/*
 * svdcomp - SVD decomposition routine.
 * Takes an mxn matrix a and decomposes it into udv, where u,v are
 * left and right orthogonal transformation matrices, and d is a
 * diagonal matrix of singular values.
 *
 * Input to dsvd is as follows:
 *   a = mxn matrix to be decomposed, gets overwritten with u
 *   m = row dimension of a
 *   n = column dimension of a
 *   w = returns the vector of singular values of a
 *   v = returns the right orthogonal transformation matrix
 */

#include "Svd.hpp"

double pythag(double a, double b) {
  double at = std::abs(a), bt = std::abs(b), ct, result;

  if (at > bt) {
    ct = bt / at;
    result = at * std::sqrt(1.0 + ct * ct);
  } else if (bt > 0.0) {
    ct = at / bt;
    result = bt * std::sqrt(1.0 + ct * ct);
  } else
    result = 0.0;
  return (result);
}

int dsvd(float** a, int m, int n, float* w, float** v) {
  /*
          Matrix a: [1..m][1..n]
                  replaced by U.
          vector w: singular values
          matrix v: matrix V [1..n][1..n]
  */
  int flag, i, its, j, jj, k, l, nm;
  nm = 0; // Prevent "warning: ‘nm’ may be used uninitialized"
  double c, f, h, s, x, y, z;
  double anorm = 0.0, g = 0.0, scale = 0.0;

  l = 0;  // Avoid -Wmaybe-uninitialized warning

  if (m < n) {
    fprintf(stderr, "#rows must be > #cols \n");
    return (0);
  }

  double* rv1 = new double[(unsigned int)n * sizeof(double)];

  /* Householder reduction to bidiagonal form */
  for (i = 0; i < n; i++) {
    /* left-hand reduction */
    l = i + 1;
    rv1[i] = scale * g;
    g = s = scale = 0.0;
    if (i < m) {
      for (k = i; k < m; k++) scale += std::abs((double)a[k][i]);
      if (scale) {
        for (k = i; k < m; k++) {
          a[k][i] = (float)((double)a[k][i] / scale);
          s += ((double)a[k][i] * (double)a[k][i]);
        }
        f = (double)a[i][i];
        int signF = (f >= 0) ? 1 : -1;
        g = -std::sqrt(s) * signF;
        h = f * g - s;
        a[i][i] = (float)(f - g);
        if (i != n - 1) {
          for (j = l; j < n; j++) {
            for (s = 0.0, k = i; k < m; k++)
              s += ((double)a[k][i] * (double)a[k][j]);
            f = s / h;
            for (k = i; k < m; k++) a[k][j] += (float)(f * (double)a[k][i]);
          }
        }
        for (k = i; k < m; k++) a[k][i] = (float)((double)a[k][i] * scale);
      }
    }
    w[i] = (float)(scale * g);

    /* right-hand reduction */
    g = s = scale = 0.0;
    if (i < m && i != n - 1) {
      for (k = l; k < n; k++) scale += std::abs((double)a[i][k]);
      if (scale) {
        for (k = l; k < n; k++) {
          a[i][k] = (float)((double)a[i][k] / scale);
          s += ((double)a[i][k] * (double)a[i][k]);
        }
        f = (double)a[i][l];
        int signF = (f >= 0) ? 1 : -1;
        g = -std::sqrt(s) * signF;
        h = f * g - s;
        a[i][l] = (float)(f - g);
        for (k = l; k < n; k++) rv1[k] = (double)a[i][k] / h;
        if (i != m - 1) {
          for (j = l; j < m; j++) {
            for (s = 0.0, k = l; k < n; k++)
              s += ((double)a[j][k] * (double)a[i][k]);
            for (k = l; k < n; k++) a[j][k] += (float)(s * rv1[k]);
          }
        }
        for (k = l; k < n; k++) a[i][k] = (float)((double)a[i][k] * scale);
      }
    }
    anorm = std::max(anorm, (std::abs((double)w[i]) + std::abs(rv1[i])));
  }

  /* accumulate the right-hand transformation */
  for (i = n - 1; i >= 0; i--) {
    if (i < n - 1) {
      if (g) {
        for (j = l; j < n; j++)
          v[j][i] = (float)(((double)a[i][j] / (double)a[i][l]) / g);
        /* double division to avoid underflow */
        for (j = l; j < n; j++) {
          for (s = 0.0, k = l; k < n; k++)
            s += ((double)a[i][k] * (double)v[k][j]);
          for (k = l; k < n; k++) v[k][j] += (float)(s * (double)v[k][i]);
        }
      }
      for (j = l; j < n; j++) v[i][j] = v[j][i] = 0.0;
    }
    v[i][i] = 1.0;
    g = rv1[i];
    l = i;
  }

  /* accumulate the left-hand transformation */
  for (i = n - 1; i >= 0; i--) {
    l = i + 1;
    g = (double)w[i];
    if (i < n - 1)
      for (j = l; j < n; j++) a[i][j] = 0.0;
    if (g) {
      g = 1.0 / g;
      if (i != n - 1) {
        for (j = l; j < n; j++) {
          for (s = 0.0, k = l; k < m; k++)
            s += ((double)a[k][i] * (double)a[k][j]);
          f = (s / (double)a[i][i]) * g;
          for (k = i; k < m; k++) a[k][j] += (float)(f * (double)a[k][i]);
        }
      }
      for (j = i; j < m; j++) a[j][i] = (float)((double)a[j][i] * g);
    } else {
      for (j = i; j < m; j++) a[j][i] = 0.0;
    }
    ++a[i][i];
  }

  /* diagonalize the bidiagonal form */
  for (k = n - 1; k >= 0; k--) {     /* loop over singular values */
    for (its = 0; its < 30; its++) { /* loop over allowed iterations */
      flag = 1;
      for (l = k; l >= 0; l--) { /* test for splitting */
        nm = l - 1;
        if (std::abs(rv1[l]) + anorm == anorm) {
          flag = 0;
          break;
        }
        if (std::abs((double)w[nm]) + anorm == anorm) break;
      }
      if (flag) {
        c = 0.0;
        s = 1.0;
        for (i = l; i <= k; i++) {
          f = s * rv1[i];
          if (fabs(f) + anorm != anorm) {
            g = (double)w[i];
            h = pythag(f, g);
            w[i] = (float)h;
            h = 1.0 / h;
            c = g * h;
            s = (-f * h);
            for (j = 0; j < m; j++) {
              y = (double)a[j][nm];
              z = (double)a[j][i];
              a[j][nm] = (float)(y * c + z * s);
              a[j][i] = (float)(z * c - y * s);
            }
          }
        }
      }
      z = (double)w[k];
      if (l == k) {    /* convergence */
        if (z < 0.0) { /* make singular value nonnegative */
          w[k] = (float)(-z);
          for (j = 0; j < n; j++) v[j][k] = (-v[j][k]);
        }
        break;
      }
      if (its >= 30) {
        delete[] rv1;
        fprintf(stderr, "No convergence after 30,000! iterations \n");
        return (0);
      }

      /* shift from bottom 2 x 2 minor */
      x = (double)w[l];
      nm = k - 1;
      y = (double)w[nm];
      g = rv1[nm];
      h = rv1[k];
      f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2.0 * h * y);
      g = pythag(f, 1.0);
      int signF = (f >= 0) ? 1 : -1;
      f = ((x - z) * (x + z) + h * ((y / (f + g * signF)) - h)) / x;

      /* next QR transformation */
      c = s = 1.0;
      for (j = l; j <= nm; j++) {
        i = j + 1;
        g = rv1[i];
        y = (double)w[i];
        h = s * g;
        g = c * g;
        z = pythag(f, h);
        rv1[j] = z;
        c = f / z;
        s = h / z;
        f = x * c + g * s;
        g = g * c - x * s;
        h = y * s;
        y = y * c;
        for (jj = 0; jj < n; jj++) {
          x = (double)v[jj][j];
          z = (double)v[jj][i];
          v[jj][j] = (float)(x * c + z * s);
          v[jj][i] = (float)(z * c - x * s);
        }
        z = pythag(f, h);
        w[j] = (float)z;
        if (z) {
          z = 1.0 / z;
          c = f * z;
          s = h * z;
        }
        f = (c * g) + (s * y);
        x = (c * y) - (s * g);
        for (jj = 0; jj < m; jj++) {
          y = (double)a[jj][j];
          z = (double)a[jj][i];
          a[jj][j] = (float)(y * c + z * s);
          a[jj][i] = (float)(z * c - y * s);
        }
      }
      rv1[l] = 0.0;
      rv1[k] = f;
      w[k] = (float)x;
    }
  }
  delete[] rv1;
  return (1);
}
