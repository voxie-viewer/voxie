/*
        File: grid3d.cpp
        Programmed by: David Vega: dvega@uc.edu.ve
        August 2019
        August 2020
*/

#ifdef _MSC_VER
#pragma warning(disable : 4244)
#endif

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

#include <ExtFilterMarchingCubes33_by_DVega/marchingcubes33.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

#ifndef GRD_orthogonal
static void setIdentMat3x3d(double (*A)[3]) {
  for (double* d = A[0] + 8; --d != A[0];) d[0] = 0.0;
  for (int i = 0; i != 3; ++i) A[i][i] = 1.0;
}
#endif

//******************************************************************
const unsigned int* grid3d::get_N() { return N; }
const float* grid3d::get_L() { return L; }
const float* grid3d::get_Ang() { return Ang; }
const double* grid3d::get_r0() { return origin; }
const double* grid3d::get_d() { return spacing; }
GRD_data_type grid3d::get_grid_value(unsigned int i, unsigned int j,
                                     unsigned int k) {
  if (data && i <= N[0] && j <= N[1] && k <= N[2]) return data[k][j][i];
  return 0;
}
#ifndef GRD_orthogonal
const double (*grid3d::get__A())[3] { return _A; }
const double (*grid3d::get_A_())[3] { return A_; }
int grid3d::isnotorthogonal() { return nonortho; }
#endif
void grid3d::setRatioAspect(double rx, double ry, double rz) {
  if (data) {
    spacing[0] = rx;
    spacing[1] = ry;
    spacing[2] = rz;
    for (int i = 0; i != 3; ++i) L[i] = N[i] * spacing[i];
  }
}

void grid3d::setOrigin(double x, double y, double z) {
  origin[0] = x;
  origin[1] = y;
  origin[2] = z;
}

void grid3d::set_Ang(float angle_bc, float angle_ca, float angle_ab) {
  Ang[0] = angle_bc;
  Ang[1] = angle_ca;
  Ang[2] = angle_ab;
#ifndef GRD_orthogonal
  if (Ang[0] != 90 || Ang[1] != 90 || Ang[2] != 90) {
    nonortho = 1;
    double ca = cos(Ang[0] * (M_PI / 180.0));
    double cb = cos(Ang[1] * (M_PI / 180.0));
    double aux1 = Ang[2] * (M_PI / 180.0);
    double sg = sin(aux1);
    double cg = cos(aux1);
    aux1 = ca - cb * cg;
    double aux2 = sqrt(sg * sg + 2 * ca * cb * cg - ca * ca - cb * cb);
    _A[0][0] = A_[0][0] = 1.0;
    _A[0][1] = cg;
    _A[0][2] = cb;
    _A[1][1] = sg;
    A_[1][1] = cb = 1.0 / sg;
    A_[0][1] = -cg * cb;
    _A[1][2] = aux1 * cb;
    _A[2][2] = aux2 * cb;
    aux2 = 1.0 / aux2;
    A_[0][2] = (cg * aux1 - ca * sg * sg) * cb * aux2;
    A_[1][2] = -aux1 * cb * aux2;
    A_[2][2] = sg * aux2;
    _A[1][0] = _A[2][0] = _A[2][1] = 0.0;
    A_[1][0] = A_[2][0] = A_[2][1] = 0.0;
  } else {
    nonortho = 0;
    setIdentMat3x3d(_A);
    setIdentMat3x3d(A_);
  }
#endif
}

void grid3d::setGridValue(unsigned int i, unsigned int j, unsigned int k,
                          GRD_data_type value) {
  if (internal_data && data && i <= N[0] && j <= N[1] && k <= N[2])
    data[k][j][i] = value;
}
//******************************************************************
void grid3d::free_data() {
  if (data) {
    for (unsigned int k = 0; k <= N[2]; ++k) {
      if (data[k] && internal_data) {
        for (unsigned int j = 0; j <= N[1]; ++j) free(data[k][j]);
      } else {
        k = N[2];
        break;
      }
      free(data[k]);
    }
    free(data);
    data = 0;
  }
}

grid3d::grid3d() : data(0) {}

grid3d::~grid3d() { free_data(); }

int grid3d::alloc_data() {
  unsigned int j, k;
  data = reinterpret_cast<GRD_data_type***>(
      malloc((N[2] + 1) * sizeof(GRD_data_type**)));
  if (!data) return -1;
  for (k = 0; k <= N[2]; ++k) {
    data[k] = reinterpret_cast<GRD_data_type**>(
        malloc((N[1] + 1) * sizeof(GRD_data_type*)));
    if (!data[k]) return -1;
    for (j = 0; j <= N[1]; ++j) {
      data[k][j] = reinterpret_cast<GRD_data_type*>(
          malloc((N[0] + 1) * sizeof(GRD_data_type)));
      if (!data[k][j]) {
        while (j) free(data[k][--j]);
        free(data[k]);
        data[k] = 0;
        return -1;
      }
    }
  }
  internal_data = 1;
  return 0;
}

int grid3d::setGridDimensions(unsigned int Nx, unsigned int Ny,
                              unsigned int Nz) {
  free_data();
  N[0] = Nx - 1;
  N[1] = Ny - 1;
  N[2] = Nz - 1;
  if (alloc_data()) {
    free_data();
    return -1;
  }
  for (int i = 0; i != 3; ++i) {
    L[i] = N[i];
    spacing[i] = 1.0;
    origin[i] = 0.0;
#ifndef GRD_orthogonal
    Ang[i] = 90.0f;
#endif
  }
#ifndef GRD_orthogonal
  nonortho = 0;
  setIdentMat3x3d(_A);
  setIdentMat3x3d(A_);
#endif
  return 0;
}

int grid3d::setDataPointer(unsigned int Nx, unsigned int Ny, unsigned int Nz,
                           GRD_data_type* data) {
  free_data();
  this->data =
      reinterpret_cast<GRD_data_type***>(malloc(Nz * sizeof(GRD_data_type**)));
  if (!this->data) return -1;
  N[0] = Nx - 1;
  N[1] = Ny - 1;
  N[2] = Nz - 1;
  for (unsigned int k = 0; k < Nz; ++k) {
    this->data[k] =
        reinterpret_cast<GRD_data_type**>(malloc(Ny * sizeof(GRD_data_type*)));
    if (!this->data[k]) {
      while (k) free(this->data[--k]);
      free(data);
      data = 0;
      return -1;
    }
    for (unsigned int j = 0; j < Ny; ++j) this->data[k][j] = data + j * Nx;
    data += Ny * Nx;
  }
  for (int i = 0; i != 3; ++i) {
    L[i] = N[i];
    spacing[i] = 1.0;
    origin[i] = 0.0;
#ifndef GRD_orthogonal
    Ang[i] = 90.0f;
#endif
  }
#ifndef GRD_orthogonal
  nonortho = 0;
  setIdentMat3x3d(_A);
  setIdentMat3x3d(A_);
#endif
  internal_data = 0;
  return 0;
}
