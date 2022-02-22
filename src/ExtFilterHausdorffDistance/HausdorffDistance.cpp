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

#include "HausdorffDistance.hpp"
#include "Cell.hpp"

#define SAMPLING_DENSITY 75

HausdorffDistance::HausdorffDistance(
    vx::Array2<const uint32_t> triangles_nominal,
    vx::Array2<const float> vertices_nominal,
    vx::Array2<const uint32_t> triangles_actual,
    vx::Array2<const float> vertices_actual, vx::Array2<float> outputDistances)
    : triangles_nominal(triangles_nominal),
      vertices_nominal(vertices_nominal),
      triangles_actual(triangles_actual),
      vertices_actual(vertices_actual),
      outputDistances(outputDistances) {}

void HausdorffDistance::run(
    vx::ClaimedOperation<de::uni_stuttgart::Voxie::ExternalOperationRunFilter>&
        op) {
  auto vertexCount_nominal = vertices_nominal.size<0>();
  auto trianglesCount_nominal = triangles_nominal.size<0>();
  auto vertexCount_actual = vertices_actual.size<0>();
  auto trianglesCount_actual = triangles_actual.size<0>();

  // get min and max values for coordinates of bounding box
  float min_x = vertices_nominal(0, 0);
  float max_x = vertices_nominal(0, 0);
  float min_y = vertices_nominal(0, 1);
  float max_y = vertices_nominal(0, 1);
  float min_z = vertices_nominal(0, 2);
  float max_z = vertices_nominal(0, 2);

  for (size_t i = 1; i < vertexCount_nominal; i++) {
    if (min_x > vertices_nominal(i, 0)) {
      min_x = vertices_nominal(i, 0);
    }
    if (max_x < vertices_nominal(i, 0)) {
      max_x = vertices_nominal(i, 0);
    }
    if (min_y > vertices_nominal(i, 1)) {
      min_y = vertices_nominal(i, 1);
    }
    if (max_y < vertices_nominal(i, 1)) {
      max_y = vertices_nominal(i, 1);
    }
    if (min_z > vertices_nominal(i, 2)) {
      min_z = vertices_nominal(i, 2);
    }
    if (max_z < vertices_nominal(i, 2)) {
      max_z = vertices_nominal(i, 2);
    }
  }

  for (size_t i = 0; i < vertexCount_actual; i++) {
    if (min_x > vertices_actual(i, 0)) {
      min_x = vertices_actual(i, 0);
    }
    if (max_x < vertices_actual(i, 0)) {
      max_x = vertices_actual(i, 0);
    }
    if (min_y > vertices_actual(i, 1)) {
      min_y = vertices_actual(i, 1);
    }
    if (max_y < vertices_actual(i, 1)) {
      max_y = vertices_actual(i, 1);
    }
    if (min_z > vertices_actual(i, 2)) {
      min_z = vertices_actual(i, 2);
    }
    if (max_z < vertices_actual(i, 2)) {
      max_z = vertices_actual(i, 2);
    }
  }

  QVector3D minCoordinates(min_x, min_y, min_z);
  QVector3D maxCoordinates(max_x, max_y, max_z);

  std::cout << "min x: " + std::to_string(min_x) << std::endl;
  std::cout << "min y: " + std::to_string(min_y) << std::endl;
  std::cout << "min z: " + std::to_string(min_z) << std::endl;
  std::cout << "max x: " + std::to_string(max_x) << std::endl;
  std::cout << "max y: " + std::to_string(max_y) << std::endl;
  std::cout << "max z: " + std::to_string(max_z) << std::endl;

  // Triange Sampling
  std::vector<QVector3D> samplingCoordinates;

  for (size_t i = 0; i < trianglesCount_actual; i++) {
    auto vertexIdA = triangles_actual(i, 0);
    auto vertexIdB = triangles_actual(i, 1);
    auto vertexIdC = triangles_actual(i, 2);

    QVector3D vertexA(vertices_actual(vertexIdA, 0),
                      vertices_actual(vertexIdA, 1),
                      vertices_actual(vertexIdA, 2));
    QVector3D vertexB(vertices_actual(vertexIdB, 0),
                      vertices_actual(vertexIdB, 1),
                      vertices_actual(vertexIdB, 2));
    QVector3D vertexC(vertices_actual(vertexIdC, 0),
                      vertices_actual(vertexIdC, 1),
                      vertices_actual(vertexIdC, 2));

    // Calculates area of triangles with crossproduct
    double area = calculateTriangleArea(vertexA, vertexB, vertexC);

    // TODO: Hier Parameter für die Sampling density einbauen
    int32_t sampleFrequency = calculateSampleFrequency(area, SAMPLING_DENSITY);

    /*---- Debug Code Begin
    std::cout << "Sample Frequency: " << sampleFrequency
              << " for Trinagle: " << i << std::endl;
    //---- Debug Code End*/

    std::vector<QVector3D> sample =
        triangleSampling(vertexA, vertexB, vertexC, sampleFrequency);

    /*---- Debug Code Begin
    for (QVector3D vec : sample) {
      std::cout << " X: " << vec.x() << " , Y: " << vec.y()
                << " , Z: " << vec.z() << std::endl;
    }
    //---- Debug Code End*/

    // concatenate sample coordinates
    samplingCoordinates.insert(samplingCoordinates.end(),
                               std::make_move_iterator(sample.begin()),
                               std::make_move_iterator(sample.end()));
  }

  // add vertices of actual surface to sample coordinates
  for (size_t index = 0; index < vertices_actual.size<0>(); index++) {
    samplingCoordinates.push_back(QVector3D(vertices_actual(index, 0),
                                            vertices_actual(index, 1),
                                            vertices_actual(index, 2)));
  }

  // calculate dimensions of cells
  const int cellCount = 10;  // change if higher cellCount is needed

  float xLength = max_x - min_x;
  float yLength = max_y - min_y;
  float zLength = max_z - min_z;

  float cellDimension = std::max({xLength, yLength, zLength}) / cellCount;

  std::cout << "CellDimension: " + std::to_string(cellDimension) << std::endl;

  // create 3D-Array with cells
  vx::Array3<Cell> cellArray({cellCount, cellCount, cellCount});

  for (size_t i = 0; i < cellCount; i++) {
    for (size_t j = 0; j < cellCount; j++) {
      for (size_t k = 0; k < cellCount; k++) {
        Cell cell;
        cell.setValues(
            (i * cellDimension) + min_x, ((i + 1) * cellDimension) + min_x,
            (j * cellDimension) + min_y, ((j + 1) * cellDimension) + min_y,
            (k * cellDimension) + min_z, ((k + 1) * cellDimension) + min_z);

        if (i == cellCount - 1) {
          cell.setEndX(max_x);
        }
        if (j == cellCount - 1) {
          cell.setEndY(max_y);
        }
        if (k == cellCount - 1) {
          cell.setEndZ(max_z);
        }
        cellArray(i, j, k) = cell;
      }
    }
  }

  // allocate each point to its respective cell
  for (size_t i = 0; i < samplingCoordinates.size(); i++) {
    QVector3D cellNumber = calculateCell(samplingCoordinates.at(i), cellArray);

    cellArray(static_cast<size_t>(cellNumber.x()),
              static_cast<size_t>(cellNumber.y()),
              static_cast<size_t>(cellNumber.z()))
        .addVertexToCell(samplingCoordinates.at(i));
  }

  /* DEBUG-CODE
  for (size_t i = 0; i < cellCount; i++) {
    for (size_t j = 0; j < cellCount; j++) {
      for (size_t k = 0; k < cellCount; k++) {
        std::vector<QVector3D> vertices = cellArray(i, j, k).getVertices();

        for (size_t index = 0; index < vertices.size(); index++) {
          if (vertices.at(index).x() < cellArray(i, j, k).getStartX() ||
              vertices.at(index).x() > cellArray(i, j, k).getEndX()) {
            std::cout << "ERROR x-Coordinate: " +
  std::to_string(vertices.at(index).x()) << std::endl; std::cout << "Cell:
  (" + std::to_string(i) + "," + std::to_string(j) + "," + std::to_string(k)
  + ")" << std::endl; std::cout << "Cell Start X: " +
  std::to_string(cellArray(i, j, k).getStartX()) << std::endl; std::cout <<
  "Cell End X: " + std::to_string(cellArray(i, j, k).getEndX()) <<
  std::endl;
          }

          if (vertices.at(index).y() < cellArray(i, j, k).getStartY() ||
              vertices.at(index).y() > cellArray(i, j, k).getEndY()) {
            std::cout << "ERROR y-Coordinate: " +
  std::to_string(vertices.at(index).y()) << std::endl; std::cout << "Cell:
  (" + std::to_string(i) + "," + std::to_string(j) + "," + std::to_string(k)
  << std::endl; std::cout << "Cell Start Y: " + std::to_string(cellArray(i,
  j, k).getStartY()) << std::endl; std::cout << "Cell End Y: " +
  std::to_string(cellArray(i, j, k).getEndY()) << std::endl;
          }

          if (vertices.at(index).z() < cellArray(i, j, k).getStartZ() ||
              vertices.at(index).z() > cellArray(i, j, k).getEndZ()) {
            std::cout << "ERROR z-Coordinate: " +
  std::to_string(vertices.at(index).z()) << std::endl; std::cout << "Cell:
  (" + std::to_string(i) + "," + std::to_string(j) + "," + std::to_string(k)
  + ")" << std::endl; std::cout << "Cell Start Z: " +
  std::to_string(cellArray(i, j, k).getStartZ()) << std::endl; std::cout <<
  "Cell End Z: " + std::to_string(cellArray(i, j, k).getEndZ()) <<
  std::endl;
          }
        }
      }
    }
  }
  */

  // calculate Hausdorff metric
  vx::Array1<double> distances({vertices_nominal.size<0>()});

  for (size_t index = 0; index < vertices_nominal.size<0>(); index++) {
    distances(index) = minimalDistance(
        QVector3D(vertices_nominal(index, 0), vertices_nominal(index, 1),
                  vertices_nominal(index, 2)),
        cellArray);

    // progressbar
    HANDLEDBUSPENDINGREPLY(op.opGen().SetProgress(
        static_cast<double>(index) /
            static_cast<double>(vertices_nominal.size<0>()),
        vx::emptyOptions()));
  }

  // TODO: convert per-vertex-distance to per-triangle-distance;   REMOVE AFTER
  // IMPLEMENTATION OF VERTEX COLORING
  vx::Array1<double> distancesTriangles({trianglesCount_nominal});
  for (size_t index = 0; index < trianglesCount_nominal; index++) {
    distancesTriangles(index) = (distances(triangles_nominal(index, 0)) +
                                 distances(triangles_nominal(index, 1)) +
                                 distances(triangles_nominal(index, 2))) /
                                3;
  }

  for (size_t index = 0; index < distancesTriangles.size<0>();
       index++) {  // change distancesTriangles to distances for vertex coloring
    outputDistances(index, 0) = static_cast<float>(distancesTriangles(index));

    // std::cout << std::to_string(outputDistances(index,0)) << std::endl;
  }
}

/**
 * @brief calculateTriangleArea calculates the area of an triangle.
 * @param vertexA coordinates of vertex A
 * @param vertexB coordinates of vertex B
 * @param vertexC coordinates of vertex C
 * @return the area of a triangle as a double
 */
double HausdorffDistance::calculateTriangleArea(const QVector3D vertexA,
                                                const QVector3D vertexB,
                                                const QVector3D vertexC) {
  QVector3D u = vertexB - vertexA;
  QVector3D v = vertexC - vertexA;

  QVector3D crossProduct = QVector3D::crossProduct(u, v);

  double area = 0.5 * crossProduct.lengthSquared();
  return area;
}

/**
 * @brief calculateSampleFrequency
 * Returns the integer sample frequency for a triangle of area triangleArea, so
 * that the sample density (number of samples per unit area) is samplingDensity
 * (statistically speaking). The returned sample frequency is the number of
 * samples to take on each side. A random variable is used so that the
 * resulting sampling density is samplingDensity in average.
 * We use a random variable so that the expected (i.e. statistical
 * average) number of samples is n_samples=triangleArea/samplingDensity. Given a
 * sampling freq. n the number of samples is n*(n+1)/2. Given the target
 * number of samples n_samples we obtain the maximum sampling freq. n that
 * gives no more than n_samples. The we choose n with probability p, or n+1
 * with probability 1-p, so that p*n*(n+1)/2+(1-p)*(n+1)*(n+2)/2=n_samples,
 * that is the expected value is n_samples.
 * @param triangleArea the area of a triangle
 * @param samplingDensity sampledensity in percent i.e. 75 for 75%
 * @return the sample frequency
 */
int HausdorffDistance::calculateSampleFrequency(const double triangleArea,
                                                const double samplingDensity) {
  /* rand variable in [0,1) interval */
  double random = rand() / (RAND_MAX + 1.0);
  double n_samples = triangleArea * samplingDensity;

  int n = static_cast<int>(floor(sqrt(0.25 + 2 * n_samples) - 0.5));
  double p = (n + 2) * 0.5 - n_samples / (n + 1);
  return (random < p) ? n : n + 1;
}

/**
 * @brief triangleSampling
 * Samples a triangle using n samples in each direction.
 * The sample points are returned in a vector. The total number of samples is
 * n*(n+1)/2. As a special case, if n equals 1, the triangle middle point is
 * used as the sample.
 * @param vertexA coordinates of vertex A
 * @param vertexB coordinates of vertex B
 * @param vertexC coordinates of vertex C
 * @param sampleFrequency the scalculated sample frequency
 * @return a list with coordinates of sampling points
 */
std::vector<QVector3D> HausdorffDistance::triangleSampling(
    const QVector3D vertexA, const QVector3D vertexB, const QVector3D vertexC,
    int32_t sampleFrequency) {
  double numberOfSamples = sampleFrequency * (sampleFrequency + 1) / 2;
  std::vector<QVector3D> samples;

  if (numberOfSamples == 0) return samples;

  if (numberOfSamples == 1) {
    // Use triangle middlepoint
    float x =
        static_cast<float>(1 / 3.0 * (vertexA.x() + vertexB.x() + vertexC.x()));
    float y =
        static_cast<float>(1 / 3.0 * (vertexA.y() + vertexB.y() + vertexC.y()));
    float z =
        static_cast<float>(1 / 3.0 * (vertexA.z() + vertexB.z() + vertexC.z()));
    QVector3D sample(x, y, z);
    samples.push_back(sample);
  } else {
    // sample triangle
    QVector3D u =
        (vertexB - vertexA) * 1 / static_cast<float>((numberOfSamples - 1));
    QVector3D v =
        (vertexC - vertexA) * 1 / static_cast<float>((numberOfSamples - 1));
    for (int i = 0; i < numberOfSamples; ++i) {
      for (int j = 0; j < numberOfSamples - i; ++j) {
        float x = vertexA.x() + i * u.x() + j * v.x();
        float y = vertexA.y() + i * u.y() + j * v.y();
        float z = vertexA.z() + i * u.z() + j * v.z();
        QVector3D sample(x, y, z);
        samples.push_back(sample);
      }
    }
  }
  return samples;
}

// TODO: kommentar
// TODO: PArameter als Shared Pointer auf vertices Array übergeben
std::vector<std::vector<QVector3D>> HausdorffDistance::getSamplingPoints(
    vx::Array2<const float>& actualVertices, const double samplingDensity) {
  std::vector<std::vector<QVector3D>> samplingCoordinates;

  for (size_t i = 0; i < actualVertices.size<0>(); i++) {
    auto vertexIdA = actualVertices(i, 0);
    auto vertexIdB = actualVertices(i, 1);
    auto vertexIdC = actualVertices(i, 2);

    QVector3D vertexA(actualVertices(static_cast<size_t>(vertexIdA), 0),
                      actualVertices(static_cast<size_t>(vertexIdA), 1),
                      actualVertices(static_cast<size_t>(vertexIdA), 2));
    QVector3D vertexB(actualVertices(static_cast<size_t>(vertexIdB), 0),
                      actualVertices(static_cast<size_t>(vertexIdB), 1),
                      actualVertices(static_cast<size_t>(vertexIdB), 2));
    QVector3D vertexC(actualVertices(static_cast<size_t>(vertexIdC), 0),
                      actualVertices(static_cast<size_t>(vertexIdC), 1),
                      actualVertices(static_cast<size_t>(vertexIdC), 2));

    // Calculates area of triangles with crossproduct
    double area = calculateTriangleArea(vertexA, vertexB, vertexC);

    // TODO: Hier Parameter für die Sampling density einbauen
    int32_t sampleFrequency = calculateSampleFrequency(area, samplingDensity);

    //---- Debug Code Begin
    std::cout << "Sample Frequency: " << sampleFrequency
              << " for Trinagle: " << i << std::endl;
    //---- Debug Code End

    std::vector<QVector3D> sample =
        triangleSampling(vertexA, vertexB, vertexC, sampleFrequency);

    //---- Debug Code Begin
    for (QVector3D vec : sample) {
      std::cout << " X: " << vec.x() << " , Y: " << vec.y()
                << " , Z: " << vec.z() << std::endl;
    }

    samplingCoordinates.push_back(sample);
  }

  return samplingCoordinates;
}

/**
 * @brief calculate the cell to a point's coordinates and returns a QVector3D
 * with the respective cell numbers
 * @param point the point to be assigned
 * @param cellArray needed for calculation info
 * @return the respective cell-number
 */
QVector3D HausdorffDistance::calculateCell(QVector3D point,
                                           vx::Array3<Cell> cellArray) {
  // create helping variables
  size_t cellArrayMaxIndex = cellArray.size<0>() - 1;
  QVector3D minCoordinates(cellArray(0, 0, 0).getStartX(),
                           cellArray(0, 0, 0).getStartY(),
                           cellArray(0, 0, 0).getStartZ());
  QVector3D maxCoordinates(
      cellArray(cellArrayMaxIndex, cellArrayMaxIndex, cellArrayMaxIndex)
          .getEndX(),
      cellArray(cellArrayMaxIndex, cellArrayMaxIndex, cellArrayMaxIndex)
          .getEndY(),
      cellArray(cellArrayMaxIndex, cellArrayMaxIndex, cellArrayMaxIndex)
          .getEndZ());

  float xCoordinate = point.x();
  float yCoordinate = point.y();
  float zCoordinate = point.z();
  QVector3D vertex(point.x(), point.y(), point.z());

  // calculate cell on x-axis
  size_t cellNumberX = 0;
  while (xCoordinate >= cellArray(0, 0, 0).getStartX()) {
    xCoordinate -= cellArray(0, 0, 0).getEdgeLength();
    cellNumberX++;
  }

  // calculate cell on y-axis
  size_t cellNumberY = 0;
  while (yCoordinate >= cellArray(0, 0, 0).getStartY()) {
    yCoordinate -= cellArray(0, 0, 0).getEdgeLength();
    cellNumberY++;
  }

  // calculate cell on z-axis
  size_t cellNumberZ = 0;
  while (zCoordinate >= cellArray(0, 0, 0).getStartZ()) {
    zCoordinate -= cellArray(0, 0, 0).getEdgeLength();
    cellNumberZ++;
  }

  // fix off-by-one error for min/max values
  if (abs(static_cast<double>(vertex.x() - minCoordinates.x())) < 0.000001 &&
      cellNumberX == 0) {
    cellNumberX++;
  }
  if (abs(static_cast<double>(vertex.y() - minCoordinates.y())) < 0.000001 &&
      cellNumberY == 0) {
    cellNumberY++;
  }
  if (abs(static_cast<double>(vertex.z() - minCoordinates.z())) < 0.000001 &&
      cellNumberZ == 0) {
    cellNumberZ++;
  }

  if (abs(static_cast<double>(vertex.x() - maxCoordinates.x())) < 0.000001 &&
      cellNumberX == 11) {
    cellNumberX--;
  }
  if (abs(static_cast<double>(vertex.y() - maxCoordinates.y())) < 0.000001 &&
      cellNumberY == 11) {
    cellNumberY--;
  }
  if (abs(static_cast<double>(vertex.z() - maxCoordinates.z())) < 0.000001 &&
      cellNumberZ == 11) {
    cellNumberZ--;
  }

  // check if point is outside uniform grid
  if (cellNumberX > cellArray.size<0>() || cellNumberY > cellArray.size<0>() ||
      cellNumberZ > cellArray.size<0>() || cellNumberX == 0 ||
      cellNumberY == 0 || cellNumberZ == 0) {
    std::cout << "ERROR: Point outside uniform grid" << std::endl;

    std::cout << "x coord: " + std::to_string(vertex.x()) << std::endl;
    std::cout << "y coord: " + std::to_string(vertex.y()) << std::endl;
    std::cout << "z coord: " + std::to_string(vertex.z()) << std::endl;

    std::cout << "cell x: " + std::to_string(cellNumberX) << std::endl;
    std::cout << "cell y: " + std::to_string(cellNumberY) << std::endl;
    std::cout << "cell z: " + std::to_string(cellNumberZ) << std::endl;

    return QVector3D(0, 0, 0);
  } else {
    return QVector3D(cellNumberX - 1, cellNumberY - 1, cellNumberZ - 1);
  }
}

/**
 * @brief calculate Distance between two points of type QVector3D
 * @param point1 first point
 * @param point2 second point
 * @return the distance
 */
double HausdorffDistance::calculateDistance(QVector3D point1,
                                            QVector3D point2) {
  return sqrt((pow(point1.x() - point2.x(), 2)) +
              (pow(point1.y() - point2.y(), 2)) +
              (pow(point1.z() - point2.z(), 2)));
}

/**
 * @brief calculate the minimal distance of a point to all points inside the
 * given cell
 * @param point the starting point
 * @param cell the cell to use
 * @return minimal distance to all point inside cell
 */
double HausdorffDistance::minimalDistanceInsideCell(QVector3D point,
                                                    Cell cell) {
  double minimalDistance = std::numeric_limits<double>::max();
  for (QVector3D cellpoint : cell.getVertices()) {
    minimalDistance =
        std::min({calculateDistance(point, cellpoint), minimalDistance});
  }
  return minimalDistance;
}

/**
 * @brief calculate minimal distance of a point to all points surrounding cells,
 * growing in distance, until minimal distance is final
 * @param point the starting point
 * @param cellArray the list of cells with points inside
 * @return the minimal distance
 */
double HausdorffDistance::minimalDistance(const QVector3D point,
                                          const vx::Array3<Cell> cellArray) {
  bool proceed = true;
  double minimalDistance = std::numeric_limits<double>::max();
  QVector3D cellOfPoint = calculateCell(point, cellArray);

  for (QVector3D cellpoint : cellArray(static_cast<size_t>(cellOfPoint.x()),
                                       static_cast<size_t>(cellOfPoint.y()),
                                       static_cast<size_t>(cellOfPoint.z()))
                                 .getVertices()) {
    minimalDistance =
        std::min({calculateDistance(point, cellpoint), minimalDistance});
  }
  size_t level = 0;

  size_t xPlus = (static_cast<size_t>(cellArray.size<0>()) - 1) -
                 static_cast<size_t>(cellOfPoint.x());
  size_t xMinus = static_cast<size_t>(cellOfPoint.x());
  size_t yPlus = (static_cast<size_t>(cellArray.size<0>()) - 1) -
                 static_cast<size_t>(cellOfPoint.y());
  size_t yMinus = static_cast<size_t>(cellOfPoint.y());
  size_t zPlus = (static_cast<size_t>(cellArray.size<0>()) - 1) -
                 static_cast<size_t>(cellOfPoint.z());
  size_t zMinus = static_cast<size_t>(cellOfPoint.z());

  // check minimal distance to nearest neighbour-cell on each axis
  double xPlusDistance =
      static_cast<double>(cellArray(static_cast<size_t>(cellOfPoint.x()),
                                    static_cast<size_t>(cellOfPoint.y()),
                                    static_cast<size_t>(cellOfPoint.z()))
                              .getEndX() -
                          point.x());
  double xMinusDistance = static_cast<double>(
      point.x() - cellArray(static_cast<size_t>(cellOfPoint.x()),
                            static_cast<size_t>(cellOfPoint.y()),
                            static_cast<size_t>(cellOfPoint.z()))
                      .getStartX());
  double yPlusDistance =
      static_cast<double>(cellArray(static_cast<size_t>(cellOfPoint.x()),
                                    static_cast<size_t>(cellOfPoint.y()),
                                    static_cast<size_t>(cellOfPoint.z()))
                              .getEndY() -
                          point.y());
  double yMinusDistance = static_cast<double>(
      point.y() - cellArray(static_cast<size_t>(cellOfPoint.x()),
                            static_cast<size_t>(cellOfPoint.y()),
                            static_cast<size_t>(cellOfPoint.z()))
                      .getStartY());
  double zPlusDistance =
      static_cast<double>(cellArray(static_cast<size_t>(cellOfPoint.x()),
                                    static_cast<size_t>(cellOfPoint.y()),
                                    static_cast<size_t>(cellOfPoint.z()))
                              .getEndZ() -
                          point.z());
  double zMinusDistance = static_cast<double>(
      point.z() - cellArray(static_cast<size_t>(cellOfPoint.x()),
                            static_cast<size_t>(cellOfPoint.y()),
                            static_cast<size_t>(cellOfPoint.z()))
                      .getStartZ());

  // search surrounding cells for closer points
  while (proceed) {
    level++;

    // update cell distances for multiple iterations iteration
    if (level > 1) {
      xPlusDistance += static_cast<double>(cellArray(0, 0, 0).getEdgeLength());
      xMinusDistance += static_cast<double>(cellArray(0, 0, 0).getEdgeLength());
      yPlusDistance += static_cast<double>(cellArray(0, 0, 0).getEdgeLength());
      yMinusDistance += static_cast<double>(cellArray(0, 0, 0).getEdgeLength());
      zPlusDistance += static_cast<double>(cellArray(0, 0, 0).getEdgeLength());
      zMinusDistance += static_cast<double>(cellArray(0, 0, 0).getEdgeLength());
    }

    // iterate through cells of level-distance on all axis:
    // x-Axis
    if (minimalDistance > xPlusDistance && level <= xPlus) {
      for (size_t yPlusLevel = 0; yPlusLevel <= yPlus && yPlusLevel <= level;
           yPlusLevel++) {
        for (size_t zPlusLevel = 0; zPlusLevel <= zPlus && zPlusLevel <= level;
             zPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) + level,
                       static_cast<size_t>(cellOfPoint.y()) + yPlusLevel,
                       static_cast<size_t>(cellOfPoint.z()) + zPlusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + level) + ","
                       + std::to_string(cellOfPoint.y() + yPlusLevel) + ","
                       + std::to_string(cellOfPoint.z() + zPlusLevel) + ")" <<
             std::endl;*/
        }

        for (size_t zMinusLevel = 1;
             zMinusLevel <= zMinus && zMinusLevel <= level; zMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) + level,
                       static_cast<size_t>(cellOfPoint.y()) + yPlusLevel,
                       static_cast<size_t>(cellOfPoint.z()) - zMinusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + level) + ","
                       + std::to_string(cellOfPoint.y() + yPlusLevel) + ","
                       + std::to_string(cellOfPoint.z() - zMinusLevel) + ")" <<
             std::endl;*/
        }
      }
      for (size_t yMinusLevel = 1;
           yMinusLevel <= yMinus && yMinusLevel <= level; yMinusLevel++) {
        for (size_t zPlusLevel = 0; zPlusLevel <= zPlus && zPlusLevel <= level;
             zPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) + level,
                       static_cast<size_t>(cellOfPoint.y()) - yMinusLevel,
                       static_cast<size_t>(cellOfPoint.z()) + zPlusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + level) + ","
                       + std::to_string(cellOfPoint.y() - yMinusLevel) + ","
                       + std::to_string(cellOfPoint.z() + zPlusLevel) + ")" <<
             std::endl;*/
        }
        for (size_t zMinusLevel = 1;
             zMinusLevel <= zMinus && zMinusLevel <= level; zMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) + level,
                       static_cast<size_t>(cellOfPoint.y()) - yMinusLevel,
                       static_cast<size_t>(cellOfPoint.z()) - zMinusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + level) + ","
                       + std::to_string(cellOfPoint.y() - yMinusLevel) + ","
                       + std::to_string(cellOfPoint.z() - zMinusLevel) + ")" <<
             std::endl;*/
        }
      }
    }
    if (minimalDistance > xMinusDistance && level <= xMinus) {
      for (size_t yPlusLevel = 0; yPlusLevel <= yPlus && yPlusLevel <= level;
           yPlusLevel++) {
        for (size_t zPlusLevel = 0; zPlusLevel <= zPlus && zPlusLevel <= level;
             zPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) - level,
                       static_cast<size_t>(cellOfPoint.y()) + yPlusLevel,
                       static_cast<size_t>(cellOfPoint.z()) + zPlusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - level) + ","
                       + std::to_string(cellOfPoint.y() + yPlusLevel) + ","
                       + std::to_string(cellOfPoint.z() + zPlusLevel) + ")" <<
             std::endl;*/
        }
        for (size_t zMinusLevel = 1;
             zMinusLevel <= zMinus && zMinusLevel <= level; zMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) - level,
                       static_cast<size_t>(cellOfPoint.y()) + yPlusLevel,
                       static_cast<size_t>(cellOfPoint.z()) - zMinusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - level) + ","
                       + std::to_string(cellOfPoint.y() + yPlusLevel) + ","
                       + std::to_string(cellOfPoint.z() - zMinusLevel) + ")" <<
             std::endl;*/
        }
      }
      for (size_t yMinusLevel = 1;
           yMinusLevel <= yMinus && yMinusLevel <= level; yMinusLevel++) {
        for (size_t zPlusLevel = 0; zPlusLevel <= zPlus; zPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) - level,
                       static_cast<size_t>(cellOfPoint.y()) - yMinusLevel,
                       static_cast<size_t>(cellOfPoint.z()) + zPlusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - level) + ","
                       + std::to_string(cellOfPoint.y() - yMinusLevel) + ","
                       + std::to_string(cellOfPoint.z() + zPlusLevel) + ")" <<
             std::endl;*/
        }
        for (size_t zMinusLevel = 1;
             zMinusLevel <= zMinus && zMinusLevel <= level; zMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) - level,
                       static_cast<size_t>(cellOfPoint.y()) - yMinusLevel,
                       static_cast<size_t>(cellOfPoint.z()) - zMinusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - level) + ","
                       + std::to_string(cellOfPoint.y() - yMinusLevel) + ","
                       + std::to_string(cellOfPoint.z() - zMinusLevel) + ")" <<
             std::endl;*/
        }
      }
    }
    // y-Axis
    if (minimalDistance > yPlusDistance && level <= yPlus) {
      for (size_t xPlusLevel = 0; xPlusLevel <= xPlus && xPlusLevel <= level;
           xPlusLevel++) {
        for (size_t zPlusLevel = 0; zPlusLevel <= zPlus && zPlusLevel <= level;
             zPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) + xPlusLevel,
                       static_cast<size_t>(cellOfPoint.y()) + level,
                       static_cast<size_t>(cellOfPoint.z()) + zPlusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + xPlusLevel) + ","
                       + std::to_string(cellOfPoint.y() + level) + ","
                       + std::to_string(cellOfPoint.z() + zPlusLevel) + ")" <<
             std::endl;*/
        }

        for (size_t zMinusLevel = 1;
             zMinusLevel <= zMinus && zMinusLevel <= level; zMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) + xPlusLevel,
                       static_cast<size_t>(cellOfPoint.y()) + level,
                       static_cast<size_t>(cellOfPoint.z()) - zMinusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + xPlusLevel) + ","
                       + std::to_string(cellOfPoint.y() + level) + ","
                       + std::to_string(cellOfPoint.z() - zMinusLevel) + ")" <<
             std::endl;*/
        }
      }
      for (size_t xMinusLevel = 1;
           xMinusLevel <= xMinus && xMinusLevel <= level; xMinusLevel++) {
        for (size_t zPlusLevel = 0; zPlusLevel <= zPlus && zPlusLevel <= level;
             zPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) - xMinusLevel,
                       static_cast<size_t>(cellOfPoint.y()) + level,
                       static_cast<size_t>(cellOfPoint.z()) + zPlusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - xMinusLevel) + ","
                       + std::to_string(cellOfPoint.y() + level) + ","
                       + std::to_string(cellOfPoint.z() + zPlusLevel) + ")" <<
             std::endl;*/
        }
        for (size_t zMinusLevel = 1;
             zMinusLevel <= zMinus && zMinusLevel <= level; zMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) - xMinusLevel,
                       static_cast<size_t>(cellOfPoint.y()) + level,
                       static_cast<size_t>(cellOfPoint.z()) - zMinusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - xMinusLevel) + ","
                       + std::to_string(cellOfPoint.y() + level) + ","
                       + std::to_string(cellOfPoint.z() - zMinusLevel) + ")" <<
             std::endl;*/
        }
      }
    }
    if (minimalDistance > yMinusDistance && level <= yMinus) {
      for (size_t xPlusLevel = 0; xPlusLevel <= xPlus && xPlusLevel <= level;
           xPlusLevel++) {
        for (size_t zPlusLevel = 0; zPlusLevel <= zPlus && zPlusLevel <= level;
             zPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) + xPlusLevel,
                       static_cast<size_t>(cellOfPoint.y()) - level,
                       static_cast<size_t>(cellOfPoint.z()) + zPlusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + xPlusLevel) + ","
                       + std::to_string(cellOfPoint.y() - level) + ","
                       + std::to_string(cellOfPoint.z() + zPlusLevel) + ")" <<
             std::endl;*/
        }

        for (size_t zMinusLevel = 1;
             zMinusLevel <= zMinus && zMinusLevel <= level; zMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) + xPlusLevel,
                       static_cast<size_t>(cellOfPoint.y()) - level,
                       static_cast<size_t>(cellOfPoint.z()) - zMinusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + xPlusLevel) + ","
                       + std::to_string(cellOfPoint.y() - level) + ","
                       + std::to_string(cellOfPoint.z() - zMinusLevel) + ")" <<
             std::endl;*/
        }
      }
      for (size_t xMinusLevel = 1;
           xMinusLevel <= xMinus && xMinusLevel <= level; xMinusLevel++) {
        for (size_t zPlusLevel = 0; zPlusLevel <= zPlus && zPlusLevel <= level;
             zPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) - xMinusLevel,
                       static_cast<size_t>(cellOfPoint.y()) - level,
                       static_cast<size_t>(cellOfPoint.z()) + zPlusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - xMinusLevel) + ","
                       + std::to_string(cellOfPoint.y() - level) + ","
                       + std::to_string(cellOfPoint.z() + zPlusLevel) + ")" <<
             std::endl;*/
        }
        for (size_t zMinusLevel = 1;
             zMinusLevel <= zMinus && zMinusLevel <= level; zMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(
                       static_cast<size_t>(cellOfPoint.x()) - xMinusLevel,
                       static_cast<size_t>(cellOfPoint.y()) - level,
                       static_cast<size_t>(cellOfPoint.z()) - zMinusLevel))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - xMinusLevel) + ","
                       + std::to_string(cellOfPoint.y() - level) + ","
                       + std::to_string(cellOfPoint.z() - zMinusLevel) + ")" <<
             std::endl;*/
        }
      }
    }
    // z-Axis
    if (minimalDistance > zPlusDistance && level <= zPlus) {
      for (size_t xPlusLevel = 0; xPlusLevel <= xPlus && xPlusLevel <= level;
           xPlusLevel++) {
        for (size_t yPlusLevel = 0; yPlusLevel <= yPlus && yPlusLevel <= level;
             yPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(static_cast<size_t>(cellOfPoint.x()) + xPlusLevel,
                             static_cast<size_t>(cellOfPoint.y()) + yPlusLevel,
                             static_cast<size_t>(cellOfPoint.z()) + level))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + xPlusLevel) + ","
                       + std::to_string(cellOfPoint.y() + yPlusLevel) + ","
                       + std::to_string(cellOfPoint.z() + level) + ")" <<
             std::endl;*/
        }

        for (size_t yMinusLevel = 1;
             yMinusLevel <= yMinus && yMinusLevel <= level; yMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(static_cast<size_t>(cellOfPoint.x()) + xPlusLevel,
                             static_cast<size_t>(cellOfPoint.y()) - yMinusLevel,
                             static_cast<size_t>(cellOfPoint.z()) + level))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + xPlusLevel) + ","
                       + std::to_string(cellOfPoint.y() - yMinusLevel) + ","
                       + std::to_string(cellOfPoint.z() + level) + ")" <<
             std::endl;*/
        }
      }
      for (size_t xMinusLevel = 1;
           xMinusLevel <= xMinus && xMinusLevel <= level; xMinusLevel++) {
        for (size_t yPlusLevel = 0; yPlusLevel <= yPlus && yPlusLevel <= level;
             yPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(static_cast<size_t>(cellOfPoint.x()) - xMinusLevel,
                             static_cast<size_t>(cellOfPoint.y()) + yPlusLevel,
                             static_cast<size_t>(cellOfPoint.z()) + level))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - xMinusLevel) + ","
                       + std::to_string(cellOfPoint.y() + yPlusLevel) + ","
                       + std::to_string(cellOfPoint.z() + level) + ")" <<
             std::endl;*/
        }
        for (size_t yMinusLevel = 1;
             yMinusLevel <= yMinus && yMinusLevel <= level; yMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(static_cast<size_t>(cellOfPoint.x()) - xMinusLevel,
                             static_cast<size_t>(cellOfPoint.y()) - yMinusLevel,
                             static_cast<size_t>(cellOfPoint.z()) + level))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - xMinusLevel) + ","
                       + std::to_string(cellOfPoint.y() - yMinusLevel) + ","
                       + std::to_string(cellOfPoint.z() + level) + ")" <<
             std::endl;*/
        }
      }
    }
    if (minimalDistance > zMinusDistance && level <= zMinus) {
      for (size_t xPlusLevel = 0; xPlusLevel <= xPlus && xPlusLevel <= level;
           xPlusLevel++) {
        for (size_t yPlusLevel = 0; yPlusLevel <= yPlus && yPlusLevel <= level;
             yPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(static_cast<size_t>(cellOfPoint.x()) + xPlusLevel,
                             static_cast<size_t>(cellOfPoint.y()) + yPlusLevel,
                             static_cast<size_t>(cellOfPoint.z()) - level))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + xPlusLevel) + ","
                       + std::to_string(cellOfPoint.y() + yPlusLevel) + ","
                       + std::to_string(cellOfPoint.z() - level) + ")" <<
             std::endl;*/
        }

        for (size_t yMinusLevel = 1;
             yMinusLevel <= yMinus && yMinusLevel <= level; yMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(static_cast<size_t>(cellOfPoint.x()) + xPlusLevel,
                             static_cast<size_t>(cellOfPoint.y()) - yMinusLevel,
                             static_cast<size_t>(cellOfPoint.z()) - level))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() + xPlusLevel) + ","
                       + std::to_string(cellOfPoint.y() - yMinusLevel) + ","
                       + std::to_string(cellOfPoint.z() - level) + ")" <<
             std::endl;*/
        }
      }
      for (size_t xMinusLevel = 1;
           xMinusLevel <= xMinus && xMinusLevel <= level; xMinusLevel++) {
        for (size_t yPlusLevel = 0; yPlusLevel <= yPlus && yPlusLevel <= level;
             yPlusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(static_cast<size_t>(cellOfPoint.x()) - xMinusLevel,
                             static_cast<size_t>(cellOfPoint.y()) + yPlusLevel,
                             static_cast<size_t>(cellOfPoint.z()) - level))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - xMinusLevel) + ","
                       + std::to_string(cellOfPoint.y() + yPlusLevel) + ","
                       + std::to_string(cellOfPoint.z() - level) + ")" <<
             std::endl;*/
        }
        for (size_t yMinusLevel = 1;
             yMinusLevel <= yMinus && yMinusLevel <= level; yMinusLevel++) {
          minimalDistance = std::min(
              {minimalDistance,
               minimalDistanceInsideCell(
                   point,
                   cellArray(static_cast<size_t>(cellOfPoint.x()) - xMinusLevel,
                             static_cast<size_t>(cellOfPoint.y()) - yMinusLevel,
                             static_cast<size_t>(cellOfPoint.z()) - level))});
          /*std::cout << "Checked Cell: ("
                       + std::to_string(cellOfPoint.x() - xMinusLevel) + ","
                       + std::to_string(cellOfPoint.y() - yMinusLevel) + ","
                       + std::to_string(cellOfPoint.z() - level) + ")" <<
             std::endl;*/
        }
      }
    }

    if (level == cellArray.size<0>() - 1 ||
        minimalDistance <
            std::min({xPlusDistance, xMinusDistance, yPlusDistance,
                      yMinusDistance, zPlusDistance, zMinusDistance})) {
      /*
      std::cout << "final min Dist: " + std::to_string(minimalDistance) <<
      std::endl; std::cout << "" << std::endl;
      */
      proceed = false;
    }
  }
  return minimalDistance;
}
