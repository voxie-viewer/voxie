/*
        File: surface.cpp
        Programmed by: David Vega - dvega@uc.edu.ve
        August 2019
        February 2020
        August 2020
*/

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>

#include <ExtFilterMarchingCubes33_by_DVega/marchingcubes33.h>

using namespace std;

MC33_real Surface::getIsovalue() { return isovalue; }

unsigned int Surface::getNumberOfVertices() { return numberOfVertices; }

unsigned int Surface::getNumberOfTriangles() { return numberOfTriangles; }

Surface::Surface() : numberOfVertices(0), numberOfTriangles(0) {}

void Surface::clear() {
  triangles.clear();
  vertices.clear();
  numberOfVertices = numberOfTriangles = 0;
}

void Surface::adjustvectorlenght() {
  triangles.resize(numberOfTriangles);
  vertices.resize(numberOfVertices);
}

const unsigned int* Surface::getTriangle(unsigned int n) {
  return (n < numberOfTriangles ? triangles[n].v : 0);
}

const MC33_real* Surface::getVertex(unsigned int n) {
  return (n < numberOfVertices ? vertices[n].v : 0);
}
