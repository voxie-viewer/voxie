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
#include <Voxie/Data/ContainerData.hpp>
#include <Voxie/Data/LabelViewModel.hpp>
#include <Voxie/Data/TableData.hpp>
#include <Voxie/Node/SegmentationStep.hpp>
#include <VoxieBackend/Data/DataType.hpp>
#include <VoxieBackend/Data/PlaneInfo.hpp>
#include <VoxieBackend/Data/VolumeDataVoxel.hpp>
#include <VoxieBackend/Data/VolumeDataVoxelInst.hpp>
#include <VoxieBackend/IO/Operation.hpp>
#include <VoxieClient/QtUtil.hpp>
#include <set>

namespace vx {

typedef std::tuple<quint32, quint32, quint32> voxel_index;

// constant expressing the bit shift to shift to the MSB of SegmentationType
// e.g. for uint8 numeric_limits::digits is 8 - 1 sets 7 as shift
static constexpr SegmentationType segmentationShift =
    std::numeric_limits<SegmentationType>::digits - 1;

void inline setBit(SegmentationType& data, SegmentationType pos) {
  data |= (1 << pos);
}
SegmentationType inline getBit(SegmentationType data, SegmentationType pos) {
  return data & (1 << pos);
}
void inline clearBit(SegmentationType& data, SegmentationType pos) {
  data &= ~(1 << pos);
}

// color palette used for segmentation label coloring
extern QList<QColor> defaultColorPalette;

/**
 * @brief Calls iterateAllVoxels for the labelVolume. Converts VolumeData to
 * VolumeDataInstance assuming SegmentationType as datatype. Creates inner
 * and outer updates.
 * @param voxelFunc functor of operation that shall be applide to every iterated
 * voxel
 * @param containerData compound of segmentation containing labelTable &
 * labelVolume
 */
template <typename Functor>
void inline iterateAllLabelVolumeVoxels(
    Functor voxelFunc, QSharedPointer<ContainerData> containerData,
    QSharedPointer<vx::io::Operation> op, bool emitFinished = true) {
  // cast output to VolumeDataVoxelInst SegmentationType as we know its type

  QSharedPointer<VolumeDataVoxelInst<SegmentationType>> compoundVoxelDataInst =
      qSharedPointerDynamicCast<VolumeDataVoxelInst<SegmentationType>>(
          containerData->getElement("labelVolume"));

  auto outerUpdate = containerData->createUpdate();
  auto update = compoundVoxelDataInst->createUpdate(
      {{containerData->getPath(), outerUpdate}});

  iterateAllVoxels(voxelFunc, compoundVoxelDataInst, op, emitFinished);

  update->finish({});
  outerUpdate->finish({});
}

/**
 * @brief template inline function to iterate and perform a given function over
 * all passsed voxels (indexes) of the labelVolume. Also
 * converts VolumeData to VolumeDataInstance assuming SegmentationType as
 * datatype
 * @param voxelFunc functor of operation that shall be applide to every iterated
 * voxel
 * @param voxelIndexes indices that should be iterated
 * @param containerData compound of segmentation containing labelTable &
 * labelVolume
 */
template <typename Functor>
void inline iterateAllPassedLabelVolumeVoxels(
    Functor voxelFunc, QList<voxel_index> voxelIndexes,
    QSharedPointer<ContainerData> containerData,
    QSharedPointer<vx::io::Operation> op, bool emitFinished = true) {
  QSharedPointer<VolumeDataVoxelInst<SegmentationType>> compoundVoxelDataInst =
      qSharedPointerDynamicCast<VolumeDataVoxelInst<SegmentationType>>(
          containerData->getElement("labelVolume"));

  auto outerUpdate = containerData->createUpdate();
  auto update = compoundVoxelDataInst->createUpdate(
      {{containerData->getPath(), outerUpdate}});

  iterateAllPassedVoxels(voxelFunc, voxelIndexes, compoundVoxelDataInst, op,
                         emitFinished);

  update->finish({});
  outerUpdate->finish({});
}

/**
 * @brief template inline function to iterate and perform a given function over
 * all passsed voxels (indexes) of a given VolumeData object.
 * @param voxelFunc functor of operation that shall be applide to every iterated
 * voxel
 * @param voxelIndexes indices that should be iterated
 * @param labelDataIn volume data on which the voxel function shall be performed
 */
template <typename Functor>
void inline iterateAllPassedVoxels(
    Functor voxelFunc, QList<voxel_index> voxelIndexes,
    QSharedPointer<VolumeDataVoxelInst<SegmentationType>> labelDataIn,
    QSharedPointer<vx::io::Operation> op, bool emitFinished = true) {
  auto result = createQSharedPointer<vx::io::Operation::ResultSuccess>();

  if (!labelDataIn) {
    qWarning()
        << "Segmentation::iterateAllPassedVoxels(): Null pointer in input";
    return;
  }

  for (auto voxelIndex : voxelIndexes) {
    voxelFunc(std::get<0>(voxelIndex), std::get<1>(voxelIndex),
              std::get<2>(voxelIndex), labelDataIn);
  }
  if (emitFinished) executeOnMainThread([op, result]() { op->finish(result); });
}

/**
 * @brief template inline function to iterate and perform a given function over
 * all voxel values of a given VolumeData element. Also converts VolumeData to
 * VolumeDataInstance assuming SegmentationType as datatype
 * @param voxelFunc functor of operation that shall be applide to every iterated
 * voxel
 * @param labelDataIn volume data on which the voxel function shall be performed
 */
template <typename Functor>
void inline iterateAllVoxels(
    Functor voxelFunc,
    QSharedPointer<VolumeDataVoxelInst<SegmentationType>> labelDataIn,
    QSharedPointer<vx::io::Operation> op, bool emitFinished = true) {
  auto result = createQSharedPointer<vx::io::Operation::ResultSuccess>();
  const vx::VectorSizeT3& labelDim = labelDataIn->getDimensions();

  if (!labelDataIn) {
    qWarning() << "Segmentation::iterateAllVoxels(): Null pointer in input";
    return;
  }

  for (size_t x = 0; x < labelDim.x; x++) {
    for (size_t y = 0; y < labelDim.y; y++) {
      op->throwIfCancelled();
      for (size_t z = 0; z < labelDim.z; z++) {
        voxelFunc(x, y, z, labelDataIn);
      }
    }
    op->updateProgress(1.0f * x / labelDim.x);
  }
  if (emitFinished) executeOnMainThread([op, result]() { op->finish(result); });
}

QList<voxel_index> inline getVoxelListFromSet(std::set<voxel_index> input) {
  QList<voxel_index> returnList;

  for (voxel_index voxel : input) {
    returnList.append(voxel);
  }

  return returnList;
}
/**
 * @brief  inline function to update voxel statistics of the segmentation main
 * view.
 * @param containerData compound of segmentation containing labelTable &
 * labelVolume
 * @param labelVoxelChangeMap Map containing label ID to voxel count numbers
 * that shall be written into the segmentation table view
 * @param outerUpdate Data update of the compound, needed if used inside steps
 * @param isCumulative Voxel update mode, true(default): cumulative adds new to
 * old values, false: new values override existing values in table
 */
void inline updateStatistics(QSharedPointer<ContainerData> containerData,
                             QMap<qint64, qint64>& labelVoxelChangeMap,
                             bool isCumulative = true) {
  auto outerUpdate = containerData->createUpdate();

  auto labelTable = qSharedPointerDynamicCast<TableData>(
      containerData->getElement("labelTable"));

  auto labelVolume = qSharedPointerDynamicCast<VolumeDataVoxel>(
      containerData->getElement("labelVolume"));

  QList<TableRow> tableRows = labelTable->getRowsByIndex();
  auto volumeSize = labelVolume->getSize();

  auto update =
      labelTable->createUpdate({{containerData->getPath(), outerUpdate}});

  for (const TableRow& row : tableRows) {
    int labelID =
        row.data().at(labelTable->getColumnIndexByName("LabelID")).value<int>();

    // Update Voxel Number
    qint64 newNrVoxels = labelVoxelChangeMap[labelID];
    if (isCumulative) {
      qint64 oldNrVoxels = row.data()
                               .at(labelTable->getColumnIndexByName("Voxels"))
                               .value<int>();
      newNrVoxels += oldNrVoxels;
    }
    labelTable->modifyRowEntry(update, row.rowID(),
                               labelTable->getColumnIndexByName("Voxels"),
                               newNrVoxels);

    // Update Voxel Percents
    labelTable->modifyRowEntry(update, row.rowID(),
                               labelTable->getColumnIndexByName("Percent"),
                               100 * ((double)newNrVoxels / volumeSize));
  }
  update->finish({});
  outerUpdate->finish({});
}

/**
 * @brief Return an empty labelVoxelChangeMap
 */
inline QMap<qint64, qint64> initLabelVoxelChangeMap(
    QSharedPointer<TableData> labelTable) {
  QMap<qint64, qint64> labelVoxelChangeMap;
  QList<TableRow> tableRows = labelTable->getRowsByIndex();
  for (const TableRow& row : tableRows) {
    labelVoxelChangeMap[row.data()
                            .at(labelTable->getColumnIndexByName("LabelID"))
                            .value<int>()] = 0;
  }
  return labelVoxelChangeMap;
}

/**
 * @brief Casts all the elements of a QList.
 */
template <typename From, typename To>
inline QList<To> castQList(QList<From> inList) {
  QList<To> outList;

  for (auto element : inList) {
    outList.append((To)element);
  }
  return outList;
}

/**
 * @brief The abstract IndexCalculator provides functionality to calculate
 * the voxel indices by region growing with the flatfield-algorithm
 */
class IndexCalculator {
 public:
  IndexCalculator(QSharedPointer<VolumeDataVoxel> originalVolume,
                  QVector3D planeOrigin, QQuaternion planeOrientation);
  virtual ~IndexCalculator();

 protected:
  /**
   * If a voxel corner does hava a smaller distance [m] to the plane as
   * distanceToPlaneThreshold, it is selected even if the voxel does not
   * intersect with the plane: distanceToPlaneThreshold =
   * voxel_diagonal*distanceScaleFactor
   */
  float distanceToPlaneThreshold;
  /**
   *  Scale factor --> Multiplied with voxel-diagonal
   */
  float distanceScaleFactor = 0.1;

  // Volume properties
  VectorSizeT3 volumeSize;

  // VoxelSize [m]
  QVector3D voxelSize;

  // Diagonal size of a voxel [m]
  float voxelDiagonal;

  // current slice information
  PlaneInfo plane;

  // Transformation Matrices
  QMatrix4x4 voxelToPlaneTrafo;
  QMatrix4x4 PlaneToVoxelTrafo;
  QMatrix4x4 threeDToPlaneTrafo;

  // Projection Matrices
  QMatrix4x4 voxelToPlaneProjection;

  // Transformations single coordinate
  QVector4D voxelToPlaneZTrafo;

  // Projections single coordinates
  QVector4D voxelToPlaneXProjection;
  QVector4D voxelToPlaneYProjection;

  // data structures for the flatfield algorithm
  QList<voxel_index> foundVoxels = {};
  QVector<voxel_index> visitNext = {};
  std::set<voxel_index> visitedVoxels = {};

  /**
   * @brief Checks if certain criteria are met
   * @param voxelIndex indices of voxel to check
   * @return Criteria met: true, Criteria not met: false
   */
  bool virtual areCriteriaMet(voxel_index& voxelIndex) = 0;

  /**
   * @brief Checks if a voxel intersects with the plane or is really close to it
   * @param voxelIndex indices of voxel to check
   * @return voxel-intersects/ is close: true, voxel does not intersect/ not
   * close: false
   */
  bool doesVoxelIntersectWithPlane(voxel_index& voxelIndex);

  /**
   * @brief Checks if a point is inside the
   * @param point point in voxel coordinates
   */
  bool isPointInVolume(QVector3D point) {
    // center point in voxel index dimension but not yet rounded to indices
    if (point.x() < 0 || point.y() < 0 || point.z() < 0 ||
        point.x() > this->volumeSize.x || point.y() > this->volumeSize.y ||
        point.z() > this->volumeSize.z) {
      return false;
    } else {
      return true;
    }
  }

  void addNeighboursToVisitNext(voxel_index middleVoxel) {
    quint32 x = std::get<0>(middleVoxel);
    quint32 y = std::get<1>(middleVoxel);
    quint32 z = std::get<2>(middleVoxel);

    for (quint32 i = x == 0 ? 0 : x - 1;
         i <= (x >= this->volumeSize.x - 1 ? this->volumeSize.x - 1 : x + 1);
         i++) {
      for (quint32 j = y == 0 ? 0 : y - 1;
           j <= (y >= this->volumeSize.y - 1 ? this->volumeSize.y - 1 : y + 1);
           j++) {
        for (quint32 k = z == 0 ? 0 : z - 1;
             k <=
             (z >= this->volumeSize.z - 1 ? this->volumeSize.z - 1 : z + 1);
             k++) {
          voxel_index neighbourVoxel = voxel_index(i, j, k);
          if (!this->visitedVoxels.count(neighbourVoxel)) {
            this->visitNext.append(neighbourVoxel);
          }
        }
      }
    }
  }

 public:
  /**
   * @brief Executes the flatfield algorithm to find all voxels that do meet the
   * criteria
   */
  void growRegion();
  /**
   * @brief Returns the found voxels, growRegion has to be executed before
   * othervise the list will be empty.
   * @return List of voxelIndices that do meet the criteria
   */
  QList<voxel_index> getFoundVoxels();
};

}  // namespace vx
