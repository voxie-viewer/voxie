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

#include <QCoreApplication>
#include <QtTest>

#include <fstream>
#include <iostream>
#include <set>
#include <stack>

#include <PluginFilter/CreateSurface/IsosurfaceExtractionOperation.hpp>
#include <PluginFilter/CreateSurface/MarchingCubes.hpp>
#include <PluginFilter/CreateSurface/SurfaceExtractor.hpp>

#include <Voxie/IO/RunFilterOperation.hpp>

#include <VoxieBackend/Data/DataType.hpp>
#include <VoxieBackend/Data/SharedMemory.hpp>
#include <VoxieBackend/Data/SharedMemoryArray.hpp>
#include <VoxieBackend/Data/SurfaceData.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>
#include <VoxieBackend/Data/VolumeStructureVoxel.hpp>

#include <VoxieClient/DBusTypeList.hpp>

class MC33Test : public QObject {
  Q_OBJECT

 public:
  MC33Test();
  ~MC33Test();

 private Q_SLOTS:
  void initTestCase();
  void cleanupTestCase();
  void test_case1();

 private:
  void createSurfaceAndCalculateCharacteristics(float[][5][5], int&, int&);
  int eulerCharacteristic(QSharedPointer<vx::SurfaceDataTriangleIndexed>);
  int bettiZero(QSharedPointer<vx::SurfaceDataTriangleIndexed>,
                std::vector<std::set<uint>*>&);
  int bettiOne(int, int, int);
  int bettiTwo(QSharedPointer<vx::SurfaceDataTriangleIndexed>,
               std::vector<std::set<uint>*>&);

  QSharedPointer<vx::io::Operation> operation;
  MarchingCubes* mc;
};

MC33Test::MC33Test() {}

MC33Test::~MC33Test() {}

void MC33Test::initTestCase() {
  this->operation = vx::io::Operation::create();
  this->mc = new MarchingCubes();
}

void MC33Test::cleanupTestCase() { delete mc; }

void MC33Test::test_case1() {
  int numberOfMismatches = 0;

  int chiWanted;
  int beta1Wanted;
  float volumeData[5][5][5];
  std::ifstream volume;
  std::ifstream invariants;

  // Iterating over 10,000 datasets
  for (int datasetId = 9999; datasetId >= 0; datasetId--) {
    char numberAsRawBytes[4];
    char fileName[200] = {};

    // Opening volume file
    sprintf(fileName, "lib/C_MC33/Closed_Surfaces/Grids/%d-scalar_field.raw",
            datasetId);
    volume.open(fileName, std::ios::in | std::ios::binary);

    volume.seekg(0, volume.end);
    int length = volume.tellg();
    volume.seekg(0, volume.beg);

    std::vector<char> data(length);
    volume.read(data.data(), length);

    // Filling volume array with values from file
    for (int i = 4; i >= 0; i--)
      for (int j = 4; j >= 0; j--)
        for (int k = 4; k >= 0; k--) {
          int index = (k + j * 5 + i * 25) * 4;
          // Reversing bytes to convert from big to little endian
          for (int bi = 3; bi >= 0; bi--)
            std::memcpy(numberAsRawBytes + 3 - bi, data.data() + index + bi,
                        sizeof(char));
          // Copying little endian float into volume array
          std::memcpy(&volumeData[i][j][k], numberAsRawBytes, sizeof(float));
        }

    volume.close();

    // Opening expected values file
    sprintf(fileName,
            "lib/C_MC33/Closed_Surfaces/InvariantsGrid/%d-invariant_grid.txt",
            datasetId);
    invariants.open(fileName, std::ios::in | std::ios::binary);

    invariants.getline(numberAsRawBytes, 4);
    chiWanted = std::stoi(numberAsRawBytes);
    invariants.getline(numberAsRawBytes, 4);
    beta1Wanted = std::stoi(numberAsRawBytes);

    invariants.close();

    // Test
    int chi, beta1;
    createSurfaceAndCalculateCharacteristics(volumeData, chi, beta1);

    if (chi != chiWanted || beta1 != beta1Wanted) numberOfMismatches++;
    /*QCOMPARE(chiWanted, chi);
    QCOMPARE(beta1Wanted, beta1);*/

    // Clear out Heap. Nesseccary because no Voxie Main-Loop
    QCoreApplication::instance()->sendPostedEvents();
    QCoreApplication::instance()->sendPostedEvents(nullptr,
                                                   QEvent::DeferredDelete);
    QCoreApplication::instance()->processEvents();
  }

  std::cout << "Number of Mismatches: " << numberOfMismatches << "\n";
  std::cout << "Error Percentile: " << numberOfMismatches / 100 << "%\n";
  QCOMPARE(0, numberOfMismatches);
}

/**
 * @brief Calculates the Euler Characteristic Chi of a given surface.
 *        Chi = Vertices - Edges + Faces
 * @param surface - Surface for which to to calculate Chi.
 * @return Chi
 */
int MC33Test::eulerCharacteristic(
    QSharedPointer<vx::SurfaceDataTriangleIndexed> surface) {
  vx::SharedMemoryArray<vx::SurfaceDataTriangleIndexed::Triangle>& triangles =
      surface->triangles();
  // bool edges[surface->vertices().size()][surface->vertices().size()];
  std::vector<std::vector<bool>> edges(
      surface->vertices().size(),
      std::vector<bool>(surface->vertices().size(), false));
  for (int i = surface->vertices().size() - 1; i >= 0; i--)
    for (int j = surface->vertices().size() - 1; j >= 0; j--)
      edges[i][j] = false;

  // If edge exists sets corresponding entry in edges to true.
  // Only half of the array is filled (diagonally)
  // therefor the number of true entries = number of edges.
  for (int i = triangles.size() - 1; i >= 0; i--) {
    std::array<uint, 3> vertices = *(triangles.data() + i);

    if (vertices[0] > vertices[1])
      edges[vertices[1]][vertices[0]] = true;
    else
      edges[vertices[0]][vertices[1]] = true;

    if (vertices[1] > vertices[2])
      edges[vertices[2]][vertices[1]] = true;
    else
      edges[vertices[1]][vertices[2]] = true;

    if (vertices[2] > vertices[0])
      edges[vertices[0]][vertices[2]] = true;
    else
      edges[vertices[2]][vertices[0]] = true;
  }

  int numberOfEdges = 0;

  // Count number of edges
  for (int i = surface->vertices().size() - 1; i >= 0; i--)
    for (int j = surface->vertices().size() - 1; j >= 0; j--)
      // edges[i][j] is either 0 or 1
      numberOfEdges += edges[i][j];

  return surface->vertices().size() - numberOfEdges + triangles.size();
}

/**
 * @brief Calculates Beta 0 of the Betti Numbers
 *        Beta 0 = Number of individual Isosurfaces
 * @param surface - Surface for which to calculate Beta 0
 * @param connectedComponents - Set with size = #vertecies for use in
 *        bettiTwo()
 * @return Beta 0
 */
int MC33Test::bettiZero(QSharedPointer<vx::SurfaceDataTriangleIndexed> surface,
                        std::vector<std::set<uint>*>& connectedComponents) {
  for (int i = surface->vertices().size() - 1; i >= 0; i--) {
    std::set<uint>* set = new std::set<uint>();
    set->insert(i);
    connectedComponents[i] = set;
  }

  // Testing if triangle is in a set. If triangle is in several sets,
  // union these sets.
  for (int i = surface->triangles().size() - 1; i >= 0; i--) {
    std::array<uint, 3> vertices = *(surface->triangles().data() + i);

    std::set<uint>* firstContainingSet = nullptr;
    for (int j = surface->vertices().size() - 1; j >= 0; j--) {
      // Set was already unioned into another set
      if (connectedComponents[j] == nullptr) continue;
      for (uint vertex : *connectedComponents[j]) {
        if (vertex == vertices[0] || vertex == vertices[1] ||
            vertex == vertices[2]) {
          // Test if this is the first set the triangles is in
          if (firstContainingSet == nullptr) {
            connectedComponents[j]->insert(vertices.begin(), vertices.end());
            firstContainingSet = connectedComponents[j];
          } else {
            firstContainingSet->insert(connectedComponents[j]->begin(),
                                       connectedComponents[j]->end());
            delete connectedComponents[j];  // Clean up
            connectedComponents[j] = nullptr;
          }
          break;
        }
      }
    }
  }

  // Counting the number of individual Isosurfaces. Each set != nullptr
  // is an individual isosurface
  int beta0 = 0;
  for (std::set<uint>* set : connectedComponents)
    if (set != nullptr) beta0++;

  return beta0;
}

/**
 * @brief Calculates Beta 1 of the Betti Numbers
 *        Beta 1 = Beta 0 + Beta 2 - chi
 * @param chi - Value of the Euler Characteristik
 * @param beta0 - Value of the 0th Betti Number
 * @param beta2 - Value of the 2nd Betti Number
 * @return Beta 1
 */
int MC33Test::bettiOne(int chi, int beta0, int beta2) {
  return beta0 + beta2 - chi;
}

/**
 * @brief Calculates Beta 2 of the Betti Numbers
 *        Beta 2 = number of closed Isosurfaces
 * @param surface - Surface for which to calculate Beta 0
 * @param connectedComponents - Set with size = #vertecies from bettiZero()
 * @return Beta 2
 */
int MC33Test::bettiTwo(QSharedPointer<vx::SurfaceDataTriangleIndexed> surface,
                       std::vector<std::set<uint>*>& connectedComponents) {
  int closedRegions = 0;
  for (std::set<uint>* set : connectedComponents) {
    if (set != nullptr) {
      // short edges[surface->vertices().size()][surface->vertices().size()];
      std::vector<std::vector<short>> edges(
          surface->vertices().size(),
          std::vector<short>(surface->vertices().size(), 0));
      for (int i = surface->vertices().size() - 1; i >= 0; i--)
        for (int j = surface->vertices().size() - 1; j >= 0; j--)
          edges[i][j] = 0;

      // If edge exists sets corresponding entry in edges to true.
      // Only half of the array is filled (diagonally)
      // therefor the number of entries greater 0 = number of edges. And the
      // value of the entry is the amount of triangles sharing this edge
      for (int i = surface->triangles().size() - 1; i >= 0; i--) {
        std::array<uint, 3> vertices = *(surface->triangles().data() + i);

        // Counting how often an edge exists
        if (set->find(vertices[0]) != set->end()) {
          if (vertices[0] > vertices[1])
            edges[vertices[1]][vertices[0]] += 1;
          else
            edges[vertices[0]][vertices[1]] += 1;

          if (vertices[1] > vertices[2])
            edges[vertices[2]][vertices[1]] += 1;
          else
            edges[vertices[1]][vertices[2]] += 1;

          if (vertices[2] > vertices[0])
            edges[vertices[0]][vertices[2]] += 1;
          else
            edges[vertices[2]][vertices[0]] += 1;
        }
      }

      bool closedRegion = true;
      for (int i = surface->vertices().size() - 1; i >= 0; i--)
        for (int j = surface->vertices().size() - 1; j >= 0; j--) {
          if (edges[i][j] > 0) {
            // Edge exists only once -> Isosurface not closed
            if (edges[i][j] == 1) closedRegion = false;
            // Edge is part of 3+ triangles -> Algorithm can not be used
            if (edges[i][j] > 2) return -1;
          }
        }

      // closedRegion either 0 or 1
      closedRegions += closedRegion;
    }
  }

  return closedRegions;
}

/**
 * @brief Executes Marching Cubes and test surface characteristics
 * @param volume - The volume data, only 5x5x5
 * @param chi    - Returns the value of chi
 * @param beta1  - Returns the value of the first Betti Number
 */
void MC33Test::createSurfaceAndCalculateCharacteristics(float volume[][5][5],
                                                        int& chi, int& beta1) {
  auto pVolumeDataVoxel =
      vx::VolumeDataVoxel::createVolume(5, 5, 5, vx::DataType::Float32);
  // Write volume to correct memory location. Must be done because pointer is
  // unmutable.
  pVolumeDataVoxel->performInGenericContext(
      [&](const auto& pVolumeDataVoxelInst) {
        void* data = pVolumeDataVoxelInst.getData();
        for (int i = 0; i < 5; i++)
          for (int j = 0; j < 5; j++)
            for (int k = 0; k < 5; k++) {
              *((float*)data + 25 * i + 5 * j + k) = volume[i][j][k];
            }
      });

  // Run Marching Cubes
  QSharedPointer<vx::SurfaceDataTriangleIndexed> surface =
      mc->extract(operation, pVolumeDataVoxel.data(), nullptr, 0.0, false);

  // Calculate Euler characteristic
  chi = eulerCharacteristic(surface);

  // Calculate Betti numbers
  std::vector<std::set<uint>*> connectedComponents(surface->vertices().size());
  int beta0 = bettiZero(surface, connectedComponents);
  int beta2 = bettiTwo(surface, connectedComponents);
  beta1 = bettiOne(chi, beta0, beta2);

  // Clean up
  for (std::set<uint>* set : connectedComponents)
    if (set != nullptr) delete set;
}

QTEST_MAIN(MC33Test)

#include "MC33UnitTest.moc"
