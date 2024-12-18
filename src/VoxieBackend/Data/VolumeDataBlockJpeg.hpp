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

#include <VoxieBackend/Data/VolumeDataBlock.hpp>

namespace vx {
class BlockJpegImplementation;

class VOXIEBACKEND_EXPORT VolumeDataBlockJpeg : public VolumeDataBlock {
  Q_OBJECT
  VX_REFCOUNTEDOBJECT

 public:
  struct VOXIEBACKEND_EXPORT BlockSize {
    using Type = std::uint64_t;

    VX_BUFFER_TYPE_DECLARE
  };

  struct VOXIEBACKEND_EXPORT HuffmanSymbolCounter {
    using Type = std::uint64_t;

    VX_BUFFER_TYPE_DECLARE
  };

 private:
  double valueOffset_;
  double valueScalingFactor_;

  quint32 samplePrecision_;

  QList<QList<quint8>> huffmanTableDC_;
  QList<QList<quint8>> huffmanTableAC_;

  QList<QList<quint16>> quantizationTable_;
  bool quantizationTableIs16bit_;
  std::vector<quint16> quantizationTableZigzag_;

  // TODO: Store in a more efficient way when the data is read-only?
  QMutex dataMutex;
  std::vector<std::vector<uint8_t>> data;

  // Note: This is the value with the offset 1 << (this->samplePrecision() - 1)
  // added, therefore it is unsigned.
  std::uint16_t zeroValueEncoded_;
  double zeroValue_;

  QSharedPointer<BlockJpegImplementation> decoderImplementation;
  QSharedPointer<BlockJpegImplementation> encoderImplementation;

 public:
  VolumeDataBlockJpeg(const vx::Vector<size_t, 3>& arrayShape,
                      const vx::Vector<size_t, 3>& blockShape,
                      // DataType dataType,
                      const vx::Vector<double, 3>& volumeOrigin,
                      const vx::Vector<double, 3>& gridSpacing,
                      double valueOffset, double valueScalingFactor,
                      quint32 samplePrecision,
                      const QList<QList<quint8>>& huffmanTableDC,
                      const QList<QList<quint8>>& huffmanTableAC,
                      const QList<QList<quint16>>& quantizationTable,
                      const QList<QSharedPointer<BlockJpegImplementation>>&
                          possibleImplementations);

  ~VolumeDataBlockJpeg();

  // The value in the volume is:
  // valueOffset +
  // (decoded value - (1 << (samplePrecision - 1))) * valueScalingFactor
  // Substracting (1 << (samplePrecision - 1)) will undo the "Level shift" of
  // JPEG (see JPEG specification A.3.1).
  double valueOffset() { return valueOffset_; }
  double valueScalingFactor() { return valueScalingFactor_; }

  // The value P from the JPEG specification
  // Currently always has to be 8 or 12.
  quint32 samplePrecision() { return samplePrecision_; }

  // https://en.wikipedia.org/wiki/Canonical_Huffman_code#cite_ref-1
  // https://en.wikipedia.org/w/index.php?title=Canonical_Huffman_code&oldid=1179905834
  const QList<QList<quint8>>& huffmanTableDC() { return huffmanTableDC_; }
  const QList<QList<quint8>>& huffmanTableAC() { return huffmanTableAC_; }

  // Always 8x8 elements, outer list is row, inner list is column
  // All entries must be non-zero.
  // Precision is implicit: If all values are below 255, 8 bit is used,
  // otherwise 16 bit is used.
  // If samplePrecision is 8, all values must be below 255.
  const QList<QList<quint16>>& quantizationTable() {
    return quantizationTable_;
  }
  bool quantizationTableIs16bit() { return quantizationTableIs16bit_; }
  const std::vector<quint16> quantizationTableZigzag() {
    return quantizationTableZigzag_;
  }

  QList<QString> supportedDBusInterfaces() override;

  QList<QSharedPointer<SharedMemory>> getSharedMemorySections() override;

  void getVolumeInfo(QList<std::tuple<QString, QString>>& fields) override;

  size_t getCompressedBlockSize(const vx::Vector<size_t, 3>& blockId);
  // Will return the compressed block size.
  // When the returned value is larger than maxLength, no data will be written.
  size_t getCompressedBlockData(const vx::Vector<size_t, 3>& blockId,
                                uint8_t* out, size_t maxLength);

  void setCompressedBlockData(const vx::Vector<size_t, 3>& blockId,
                              const uint8_t* data, size_t length);

  void decodeBlockNoCache(const vx::Vector<size_t, 3>& blockId,
                          DataType dataType,
                          const vx::Array3<void>& data) override;

 protected:
  void encodeBlockImpl(const vx::Vector<size_t, 3>& blockId, DataType dataType,
                       const vx::Array3<const void>& data) override;
};
}  // namespace vx
