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

#include "VolumeDataBlockJpeg.hpp"

#include <VoxieClient/DBusAdaptors.hpp>
#include <VoxieClient/Format.hpp>

#include <VoxieBackend/DebugOptions.hpp>

#include <VoxieBackend/Data/BlockDecoder.hpp>
#include <VoxieBackend/Data/BlockEncoder.hpp>
#include <VoxieBackend/Data/BlockJpegImplementation.hpp>
#include <VoxieBackend/Data/Buffer.hpp>
#include <VoxieBackend/Data/BufferTypeInst.hpp>

#include <VoxieBackend/Jpeg/HuffmanDecoder.hpp>
#include <VoxieBackend/Jpeg/HuffmanTable.hpp>

#include <VoxieClient/ObjectExport/DBusCallUtil.hpp>

template <class T>
static void toZigzag(const QList<QList<T>> input, std::vector<T>& output) {
  if (input.size() != 8)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "input.size() != 8");
  for (const auto& row : input)
    if (row.size() != 8)
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "row.size() != 8");
  if (output.size() != 64)
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                        "output.size() != 64");

  int x = 0;
  int y = 0;
  bool upwards = true;

  for (int i = 0; i < 64; i++) {
    output[i] = input[y][x];

    if (upwards) {
      if (x < 7) {
        x++;
        if (y > 0)
          y--;
        else
          upwards = false;
      } else {
        y++;
        upwards = false;
      }
    } else {
      if (y < 7) {
        y++;
        if (x > 0)
          x--;
        else
          upwards = true;
      } else {
        x++;
        upwards = true;
      }
    }
  }
}

namespace vx {
namespace internal {
class VolumeDataBlockJpegAdaptorImpl : public VolumeDataBlockJpegAdaptor {
  VolumeDataBlockJpeg* object;

 public:
  VolumeDataBlockJpegAdaptorImpl(VolumeDataBlockJpeg* object)
      : VolumeDataBlockJpegAdaptor(object), object(object) {}
  virtual ~VolumeDataBlockJpegAdaptorImpl() {}

  QList<QByteArray> huffmanTableAC() const override {
    try {
      return toByteArray(object->huffmanTableAC());
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  QList<QByteArray> huffmanTableDC() const override {
    try {
      return toByteArray(object->huffmanTableDC());
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  QList<QList<quint16>> quantizationTable() const override {
    try {
      return object->quantizationTable();
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  bool quantizationTableIs16bit() const override {
    try {
      return object->quantizationTableIs16bit();
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  uint samplePrecision() const override {
    try {
      return object->samplePrecision();
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  double valueOffset() const override {
    try {
      return object->valueOffset();
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  double valueScalingFactor() const override {
    try {
      return object->valueScalingFactor();
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  void GetCompressedBlockSizes(
      quint64 count, const QDBusObjectPath& blockIDsBuffer,
      quint64 blockIDsOffset, const QDBusObjectPath& sizesBytesBufferOut,
      quint64 sizesBytesOffset,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      auto blockIDsBufferObj = vx::Buffer::lookup(blockIDsBuffer);
      auto blockIDs =
          blockIDsBufferObj->asArray1<const VolumeDataBlock::BlockID>();

      auto sizesBytesBufferOutObj = vx::Buffer::lookup(sizesBytesBufferOut);
      auto sizesBytes =
          sizesBytesBufferOutObj->asArray1<VolumeDataBlockJpeg::BlockSize>();

      if (count > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "count parameter too large");
      size_t countS = count;
      if (blockIDsOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blockIDsOffset parameter too large");
      size_t blockIDsOffsetS = blockIDsOffset;
      if (sizesBytesOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "sizesBytesOffset parameter too large");
      size_t sizesBytesOffsetS = sizesBytesOffset;

      if (blockIDsOffsetS + countS < blockIDsOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blockIDsOffset / count parameters too large");
      if (sizesBytesOffsetS + countS < sizesBytesOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "sizesBytesOffset / count parameters too large");

      if (blockIDsOffsetS + countS > blockIDs.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "blockIDs array too small");
      if (sizesBytesOffsetS + countS > sizesBytes.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "sizesBytes array too small");

      handleDBusCallOnBackgroundThreadVoid(
          object, [self = object->thisShared(), count, blockIDs,
                   blockIDsOffsetS, sizesBytes, sizesBytesOffsetS] {
            // Parallelizing this will probably only increase thread contention
            for (size_t i = 0; i < count; i++) {
              // TODO: Do these have to be volatile reads or something like that
              // to prevent the compiler from assuming that the data in
              // blockIDs(i) does not change?
              auto blockID = blockIDs(blockIDsOffsetS + i);

              if (blockID.access<0>() > std::numeric_limits<size_t>::max() ||
                  blockID.access<1>() > std::numeric_limits<size_t>::max() ||
                  blockID.access<2>() > std::numeric_limits<size_t>::max())
                throw Exception("de.uni_stuttgart.Voxie.Overflow",
                                "BlockID parameter too large");
              auto blockIDVec = vectorCastNarrow<size_t>(blockID);

              sizesBytes(sizesBytesOffsetS + i) =
                  self->getCompressedBlockSize(blockIDVec);
            }
          });
    } catch (vx::Exception& e) {
      e.handle(object);
    }
  }

  quint64 GetCompressedData(quint64 count,
                            const QDBusObjectPath& blockIDsBuffer,
                            quint64 blockIDsOffset,
                            const QDBusObjectPath& dataBuffer,
                            quint64 dataOffset, quint64 dataSize,
                            const QDBusObjectPath& sizesBytesBuffer,
                            quint64 sizesBytesOffset,
                            const QMap<QString, QDBusVariant>& options,
                            quint64& actualBytes) override {
    try {
      vx::ExportedObject::checkOptions(options);

      auto blockIDsBufferObj = vx::Buffer::lookup(blockIDsBuffer);
      auto blockIDs =
          blockIDsBufferObj->asArray1<const VolumeDataBlock::BlockID>();

      auto dataBufferObj = vx::Buffer::lookup(dataBuffer);
      auto data = dataBufferObj->asArray1<ByteBuffer>();

      auto sizesBytesBufferObj = vx::Buffer::lookup(sizesBytesBuffer);
      auto sizesBytes =
          sizesBytesBufferObj->asArray1<VolumeDataBlockJpeg::BlockSize>();

      if (count > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "count parameter too large");
      size_t countS = count;
      if (blockIDsOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blockIDsOffset parameter too large");
      size_t blockIDsOffsetS = blockIDsOffset;
      if (dataOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "dataOffset parameter too large");
      size_t dataOffsetS = dataOffset;
      if (dataSize > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "dataSize parameter too large");
      size_t dataSizeS = dataSize;
      if (sizesBytesOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "sizesBytesOffset parameter too large");
      size_t sizesBytesOffsetS = sizesBytesOffset;

      if (blockIDsOffsetS + countS < blockIDsOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blockIDsOffset / count parameters too large");
      if (dataOffsetS + dataSizeS < dataOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "dataOffset / dataSize parameters too large");
      if (sizesBytesOffsetS + countS < sizesBytesOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "sizesBytesOffset / count parameters too large");

      if (blockIDsOffsetS + countS > blockIDs.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "blockIDs array too small");
      if (dataOffsetS + dataSizeS > data.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "data array too small");
      if (sizesBytesOffsetS + countS > sizesBytes.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "sizesBytes array too small");

      return handleDBusCallOnBackgroundThreadTwo<quint64, quint64>(
          actualBytes, object,
          [self = object->thisShared(), countS, blockIDs, blockIDsOffsetS, data,
           dataOffsetS, dataSizeS, sizesBytes, sizesBytesOffsetS] {
            uint8_t* dataStart = data.data() + dataOffsetS;

            // This cannot be parallelized because the position for writing into
            // data is unknown until the previous blocks have been processed.
            size_t pos = 0;
            for (size_t i = 0; i < countS; i++) {
              // TODO: Do these have to be volatile reads or something like that
              // to prevent the compiler from assuming that the data in
              // blocks(i) does not change?
              auto blockID = blockIDs(blockIDsOffsetS + i);

              if (blockID.access<0>() > std::numeric_limits<size_t>::max() ||
                  blockID.access<1>() > std::numeric_limits<size_t>::max() ||
                  blockID.access<2>() > std::numeric_limits<size_t>::max())
                throw Exception("de.uni_stuttgart.Voxie.Overflow",
                                "BlockID parameter too large");
              auto blockIDVec = vectorCastNarrow<size_t>(blockID);

              if (pos > dataSizeS)
                throw Exception("de.uni_stuttgart.Voxie.InternalError",
                                "pos > dataSizeS");
              size_t remaining = dataSizeS - pos;

              size_t len = self->getCompressedBlockData(
                  blockIDVec, dataStart + pos, remaining);
              // Write length even if no data was written
              sizesBytes(sizesBytesOffsetS + i) = len;
              if (len > remaining) {
                // No data was written for this block
                return std::make_tuple(i, pos);
              }
              pos += len;
            }

            // All blocks were written
            return std::make_tuple(countS, pos);
          });
    } catch (vx::Exception& e) {
      return e.handle(object);
    }
  }

  void SetCompressedData(quint64 count, const QDBusObjectPath& blockIDsBuffer,
                         quint64 blockIDsOffset,
                         const QDBusObjectPath& dataBuffer, quint64 dataOffset,
                         quint64 dataSize,
                         const QDBusObjectPath& sizesBytesBuffer,
                         quint64 sizesBytesOffset,
                         const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      auto blockIDsBufferObj = vx::Buffer::lookup(blockIDsBuffer);
      auto blockIDs =
          blockIDsBufferObj->asArray1<const VolumeDataBlock::BlockID>();

      auto dataBufferObj = vx::Buffer::lookup(dataBuffer);
      auto data = dataBufferObj->asArray1<const ByteBuffer>();

      auto sizesBytesBufferObj = vx::Buffer::lookup(sizesBytesBuffer);
      auto sizesBytes =
          sizesBytesBufferObj->asArray1<const VolumeDataBlockJpeg::BlockSize>();

      if (count > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "count parameter too large");
      size_t countS = count;
      if (blockIDsOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blockIDsOffset parameter too large");
      size_t blockIDsOffsetS = blockIDsOffset;
      if (dataOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "dataOffset parameter too large");
      size_t dataOffsetS = dataOffset;
      if (dataSize > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "dataSize parameter too large");
      size_t dataSizeS = dataSize;
      if (sizesBytesOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "sizesBytesOffset parameter too large");
      size_t sizesBytesOffsetS = sizesBytesOffset;

      if (blockIDsOffsetS + countS < blockIDsOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blockIDsOffset / count parameters too large");
      if (dataOffsetS + dataSizeS < dataOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "dataOffset / dataSize parameters too large");
      if (sizesBytesOffsetS + countS < sizesBytesOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "sizesBytesOffset / count parameters too large");

      if (blockIDsOffsetS + countS > blockIDs.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "blockIDs array too small");
      if (dataOffsetS + dataSizeS > data.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "data array too small");
      if (sizesBytesOffsetS + countS > sizesBytes.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "sizesBytes array too small");

      return handleDBusCallOnBackgroundThreadVoid(
          object,
          [self = object->thisShared(), countS, blockIDs, blockIDsOffsetS, data,
           dataOffsetS, dataSizeS, sizesBytes, sizesBytesOffsetS] {
            const uint8_t* dataStart = data.data() + dataOffsetS;

            // This cannot be parallelized because the position for reading from
            // data is unknown until the previous blocks have been processed.
            size_t pos = 0;
            for (size_t i = 0; i < countS; i++) {
              // TODO: Do these have to be volatile reads or something like that
              // to prevent the compiler from assuming that the data in
              // blocks(i) does not change?
              auto blockID = blockIDs(blockIDsOffsetS + i);
              auto len = sizesBytes(sizesBytesOffsetS + i);

              if (blockID.access<0>() > std::numeric_limits<size_t>::max() ||
                  blockID.access<1>() > std::numeric_limits<size_t>::max() ||
                  blockID.access<2>() > std::numeric_limits<size_t>::max())
                throw Exception("de.uni_stuttgart.Voxie.Overflow",
                                "BlockID parameter too large");
              auto blockIDVec = vectorCastNarrow<size_t>(blockID);

              if (len > std::numeric_limits<size_t>::max())
                throw Exception("de.uni_stuttgart.Voxie.Overflow",
                                "sizeBytes parameter too large");
              size_t lenS = len;

              if (pos + lenS < pos)
                throw Exception("de.uni_stuttgart.Voxie.Overflow",
                                "pos+sizeBytes too large");
              if (pos + lenS > dataSizeS)
                throw Exception(
                    "de.uni_stuttgart.Voxie.InvalidArgument",
                    "sum of sizeBytes values exceeds dataSize parameter");

              self->setCompressedBlockData(blockIDVec, dataStart + pos, len);

              pos += len;
            }
          });
    } catch (vx::Exception& e) {
      e.handle(object);
    }
  }

  virtual void CountHuffmanSymbols(
      quint64 count, const QDBusObjectPath& blockIDsBuffer,
      quint64 blockIDsOffset, const QDBusObjectPath& dcSymbolCount,
      quint64 dcSymbolCountOffset, const QDBusObjectPath& acSymbolCount,
      quint64 acSymbolCountOffset, const QDBusObjectPath& paddingBitCount,
      quint64 paddingBitCountOffset,
      const QMap<QString, QDBusVariant>& options) override {
    try {
      vx::ExportedObject::checkOptions(options);

      auto blockIDsBufferObj = vx::Buffer::lookup(blockIDsBuffer);
      auto blockIDs =
          blockIDsBufferObj->asArray1<const VolumeDataBlock::BlockID>();

      auto dcSymbolCountObj = vx::Buffer::lookup(dcSymbolCount);
      auto dcSymbolCountArray =
          dcSymbolCountObj
              ->asArray1<VolumeDataBlockJpeg::HuffmanSymbolCounter>();

      auto acSymbolCountObj = vx::Buffer::lookup(acSymbolCount);
      auto acSymbolCountArray =
          acSymbolCountObj
              ->asArray1<VolumeDataBlockJpeg::HuffmanSymbolCounter>();

      auto paddingBitCountObj = vx::Buffer::lookup(paddingBitCount);
      auto paddingBitCountArray =
          paddingBitCountObj
              ->asArray1<VolumeDataBlockJpeg::HuffmanSymbolCounter>();

      // TODO: Clean up all these checks

      if (count > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "count parameter too large");
      size_t countS = count;
      if (blockIDsOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blockIDsOffset parameter too large");
      size_t blockIDsOffsetS = blockIDsOffset;
      if (dcSymbolCountOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "dcSymbolCountOffset parameter too large");
      size_t dcSymbolCountOffsetS = dcSymbolCountOffset;
      if (acSymbolCountOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "acSymbolCountOffset parameter too large");
      size_t acSymbolCountOffsetS = acSymbolCountOffset;
      if (paddingBitCountOffset > std::numeric_limits<size_t>::max())
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "paddingBitCountOffset parameter too large");
      size_t paddingBitCountOffsetS = paddingBitCountOffset;

      if (blockIDsOffsetS + countS < blockIDsOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "blockIDsOffset / count parameters too large");
      /*
      if (dcSymbolCountOffsetS + ... < dcSymbolCountOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "dcSymbolCountOffset / ... parameters too large");
      if (acSymbolCountOffsetS + ... < acSymbolCountOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "acSymbolCountOffset / ... parameters too large");
      */
      if (paddingBitCountOffsetS + 8 < paddingBitCountOffsetS)
        throw Exception("de.uni_stuttgart.Voxie.Overflow",
                        "paddingBitCountOffset parameter too large");

      if (blockIDsOffsetS + countS > blockIDs.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "blockIDs array too small");
      // Note: This only checks that the offset is at most the size of the array
      if (dcSymbolCountOffsetS > dcSymbolCountArray.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "dcSymbolCount array too small");
      if (acSymbolCountOffsetS > acSymbolCountArray.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "acSymbolCount array too small");
      // Note: This actually checks that there is enough space
      if (paddingBitCountOffsetS + 8 > paddingBitCountArray.size<0>())
        throw Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "paddingBitCount array too small");

      handleDBusCallOnBackgroundThreadVoid(
          object,
          [self = object->thisShared(), count, blockIDs, blockIDsOffsetS,
           dcSymbolCountArray, dcSymbolCountOffsetS, acSymbolCountArray,
           acSymbolCountOffsetS, paddingBitCountArray, paddingBitCountOffsetS] {
            vx::jpeg::HuffmanTable tableDC(self->huffmanTableDC());
            vx::jpeg::HuffmanTable tableAC(self->huffmanTableAC());
            size_t jpegBlockCount = (self->blockShape()[0] / 8) *
                                    (self->blockShape()[1] / 8) *
                                    self->blockShape()[2];

            // TODO: Parallelize this (and sum up the results at the end)
            std::vector<uint8_t> buffer(0);
            for (size_t i = 0; i < count; i++) {
              // TODO: Do these have to be volatile reads or something like that
              // to prevent the compiler from assuming that the data in
              // blockIDs(i) does not change?
              auto blockID = blockIDs(blockIDsOffsetS + i);

              if (blockID.access<0>() > std::numeric_limits<size_t>::max() ||
                  blockID.access<1>() > std::numeric_limits<size_t>::max() ||
                  blockID.access<2>() > std::numeric_limits<size_t>::max())
                throw Exception("de.uni_stuttgart.Voxie.Overflow",
                                "BlockID parameter too large");
              auto blockIDVec = vectorCastNarrow<size_t>(blockID);

              size_t len;
              for (;;) {
                len = self->getCompressedBlockData(blockIDVec, buffer.data(),
                                                   buffer.size());
                if (len <= buffer.size()) break;
                buffer.resize(std::max(buffer.size() * 2, len));
              }

              // Ignore zero blocks
              if (len > 0) {
                vx::jpeg::HuffmanDecoder decoder(buffer.data(), len);

                for (size_t jpegBlock = 0; jpegBlock < jpegBlockCount;
                     jpegBlock++) {
                  decoder.countCodewordsOneBlock(
                      &tableDC, &tableAC,
                      dcSymbolCountArray.data() + dcSymbolCountOffsetS,
                      dcSymbolCountArray.size<0>() - dcSymbolCountOffsetS,
                      acSymbolCountArray.data() + acSymbolCountOffsetS,
                      acSymbolCountArray.size<0>() - acSymbolCountOffsetS);
                }
                uint64_t paddingBits = decoder.remainingBits();
                decoder.assertAtEnd();
                if (paddingBits >= 8)
                  throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                                      "paddingBits >= 8");
                uint64_t& counter =
                    paddingBitCountArray(paddingBitCountOffsetS + paddingBits);
                if (counter + 1 == 0)
                  throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                                      "Counter overflow");
                counter += 1;
              }
            }
          });
    } catch (vx::Exception& e) {
      e.handle(object);
    }
  }
};
}  // namespace internal
}  // namespace vx

static void checkHuffmanTable(const QList<QList<quint8>>& huffmanTable,
                              const QString& name) {
  // Check huffman table entries

  if (huffmanTable.size() != 16)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Error in " + name + " huffman table: huffmanTable.size() != 16");

  bool found[256];
  for (size_t i = 0; i < 256; i++) found[i] = false;

  quint8 len = 1;
  quint32 used = 0;
  for (const auto& values : huffmanTable) {
    for (const auto& val : values) {
      if (found[val])
        throw vx::Exception(
            "de.uni_stuttgart.Voxie.InvalidArgument",
            "Error in " + name +
                " huffman table: Duplicate huffman table entry " +
                QString::number(val));
      found[val] = true;
      used += 1 << (16 - len);
    }
    len += 1;
  }
  // TODO: Should this be >= or >?
  // With >=: The all-1 codeword cannot be used
  // Annex C of the JPEG standard says:
  // "In addition, the codes shall be generated such that the all-1-bits code
  // word of any length is reserved as a prefix for longer code words."
  // which seems to say that an all-1 codeword is not allowed, even if there are
  // no longer codewords.
  if (used >= (1 << 16)) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Error in " + name +
            " huffman table: Huffman table entries are too short");
  }
}

vx::VolumeDataBlockJpeg::VolumeDataBlockJpeg(
    const vx::Vector<size_t, 3>& arrayShape,
    const vx::Vector<size_t, 3>& blockShape,
    // DataType dataType,
    const vx::Vector<double, 3>& volumeOrigin,
    const vx::Vector<double, 3>& gridSpacing, double valueOffset,
    double valueScalingFactor, quint32 samplePrecision,
    const QList<QList<quint8>>& huffmanTableDC,
    const QList<QList<quint8>>& huffmanTableAC,
    const QList<QList<quint16>>& quantizationTable,
    const QList<QSharedPointer<BlockJpegImplementation>>&
        possibleImplementations)
    : VolumeDataBlock(arrayShape, blockShape,
                      // dataType,
                      volumeOrigin, gridSpacing),
      valueOffset_(valueOffset),
      valueScalingFactor_(valueScalingFactor),
      samplePrecision_(samplePrecision),
      huffmanTableDC_(huffmanTableDC),
      huffmanTableAC_(huffmanTableAC),
      quantizationTable_(quantizationTable) {
  new vx::internal::VolumeDataBlockJpegAdaptorImpl(this);

  // Check size parameters

  if (this->blockShape()[0] % 8 != 0)
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "block size x is not a multiple of 8");
  if (this->blockShape()[1] % 8 != 0)
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "block size y is not a multiple of 8");

  // Check JPEG parameters

  if (this->samplePrecision() != 8 && this->samplePrecision() != 12)
    throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                        "Got invalid sample precision (neither 8 nor 12)");

  checkHuffmanTable(this->huffmanTableDC(), "DC");
  checkHuffmanTable(this->huffmanTableAC(), "AC");

  if (this->quantizationTable().size() != 8)
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Error in quantization table: Got invalid number of row entries");
  this->quantizationTableIs16bit_ = false;
  for (const auto& row : this->quantizationTable()) {
    if (row.size() != 8)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InvalidArgument",
          "Error in quantization table: Got invalid number of column entries");
    for (const auto& entry : row) {
      if (entry == 0)
        throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                            "Error in quantization table: Got 0 entry");
      if (entry > 255) this->quantizationTableIs16bit_ = true;
    }
  }
  this->quantizationTableZigzag_.resize(64);
  toZigzag(this->quantizationTable(), this->quantizationTableZigzag_);

  if (this->samplePrecision() == 8 && this->quantizationTableIs16bit())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Got 16-bit entries in quantization table for 8-bit sample precision");

  int decoderPriority = 0;
  decoderImplementation = QSharedPointer<BlockJpegImplementation>();
  for (const auto& impl : possibleImplementations) {
    if (!impl->hasDecoder()) continue;

    if (!impl->supportsParametersDecoder(
            vx::BlockJpegImplementation::ParametersRef(
                this->samplePrecision(), this->blockShape(),
                this->huffmanTableDC(), this->huffmanTableAC(),
                this->quantizationTableZigzag())))
      continue;

    int implPrio = impl->priority();
    if (decoderImplementation && decoderPriority >= implPrio) continue;

    decoderPriority = implPrio;
    decoderImplementation = impl;
  }
  if (!decoderImplementation) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.NotImplemented",
        vx::format("Unable to find a JPEG decoder for {} bit JPEG, maybe "
                   "disabled at compile time",
                   this->samplePrecision()));
  }

  int encoderPriority = 0;
  encoderImplementation = QSharedPointer<BlockJpegImplementation>();
  for (const auto& impl : possibleImplementations) {
    if (!impl->hasEncoder()) continue;

    if (!impl->supportsParametersEncoder(
            vx::BlockJpegImplementation::ParametersRef(
                this->samplePrecision(), this->blockShape(),
                this->huffmanTableDC(), this->huffmanTableAC(),
                this->quantizationTableZigzag())))
      continue;

    int implPrio = impl->priority();
    if (encoderImplementation && encoderPriority >= implPrio) continue;

    encoderPriority = implPrio;
    encoderImplementation = impl;
  }
  if (!encoderImplementation) {
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.NotImplemented",
        vx::format("Unable to find a JPEG encoder for {} bit JPEG, maybe "
                   "disabled at compile time",
                   this->samplePrecision()));
  }

  // Get encoded value for 0
  {
    double val = 0;
    val = (val - this->valueOffset()) / this->valueScalingFactor();
    val += 1 << (this->samplePrecision() - 1);
    val = std::round(val);
    if (val < 0) val = 0;
    if (val >= 1 << this->samplePrecision())
      val = (1 << this->samplePrecision()) - 1;
    zeroValueEncoded_ = (uint16_t)val;
    // TODO: Should this value be stored when the volume is saved to avoid
    // problems when the value passed to the rounding operation is very close to
    // xx.5?
  }
  // Decode the 0 value
  {
    double val = zeroValueEncoded_;
    val -= 1 << (this->samplePrecision() - 1);
    val = this->valueOffset() + this->valueScalingFactor() * val;
    zeroValue_ = val;
  }
  // qDebug() << "Zero value" << zeroValueEncoded_ << zeroValue_;

  // Allocate data
  this->data.resize(this->overallBlockCount());
}

vx::VolumeDataBlockJpeg::~VolumeDataBlockJpeg() {}

QList<QString> vx::VolumeDataBlockJpeg::supportedDBusInterfaces() {
  return {
      "de.uni_stuttgart.Voxie.VolumeDataBlockJpeg",
      "de.uni_stuttgart.Voxie.VolumeDataBlock",
      "de.uni_stuttgart.Voxie.VolumeData",
  };
}

QList<QSharedPointer<vx::SharedMemory>>
vx::VolumeDataBlockJpeg::getSharedMemorySections() {
  // TODO: Actually implement this?
  return {};
}

void vx::VolumeDataBlockJpeg::getVolumeInfo(
    QList<std::tuple<QString, QString>>& fields) {
  // int precision = 6; // Takes too much space
  int precision = 5;

  fields << std::make_tuple("Dimension",
                            QString::number(this->arrayShape()[0]) + " x " +
                                QString::number(this->arrayShape()[1]) + " x " +
                                QString::number(this->arrayShape()[2]));

  fields << std::make_tuple(
      "Spacing",
      QString::number(this->gridSpacing()[0], 'g', precision) + " x " +
          QString::number(this->gridSpacing()[1], 'g', precision) + " x " +
          QString::number(this->gridSpacing()[2], 'g', precision));

  /*
  fields << std::make_tuple("Data Type",
                            DataTypeNames.value(this->getDataType()));
  */

  fields << std::make_tuple("Block size",
                            QString::number(this->blockShape()[0]) + " x " +
                                QString::number(this->blockShape()[1]) + " x " +
                                QString::number(this->blockShape()[2]));

  fields << std::make_tuple("Block count",
                            QString::number(this->blockCount()[0]) + " x " +
                                QString::number(this->blockCount()[1]) + " x " +
                                QString::number(this->blockCount()[2]));

  fields << std::make_tuple("Sample precision",
                            QString::number(this->samplePrecision()));

  fields << std::make_tuple(
      "Quantization table bits",
      QString::number(this->quantizationTableIs16bit() ? 16 : 8));

  // TODO: Information about memory usage?
}

size_t vx::VolumeDataBlockJpeg::getCompressedBlockSize(
    const vx::Vector<size_t, 3>& blockId) {
  for (std::size_t i = 0; i < 3; i++) {
    if (blockId[i] >= blockCount()[i])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got invalid block ID");
  }
  quint64 pos = blockId[0] +
                blockCount()[0] * (blockId[1] + blockCount()[1] * blockId[2]);

  {
    QMutexLocker locker(&this->dataMutex);
    if (pos >= this->data.size())
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "pos >= this->data.size()");
    return this->data[pos].size();
  }
}

size_t vx::VolumeDataBlockJpeg::getCompressedBlockData(
    const vx::Vector<size_t, 3>& blockId, uint8_t* out, size_t maxLength) {
  for (std::size_t i = 0; i < 3; i++) {
    if (blockId[i] >= blockCount()[i])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got invalid block ID");
  }
  quint64 pos = blockId[0] +
                blockCount()[0] * (blockId[1] + blockCount()[1] * blockId[2]);

  {
    QMutexLocker locker(&this->dataMutex);
    if (pos >= this->data.size())
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "pos >= this->data.size()");
    const std::vector<uint8_t>& entry = this->data[pos];

    size_t size = entry.size();
    if (size > maxLength) {
      // Don't write anything
      return size;
    }

    memcpy(out, entry.data(), size);
    return size;
  }
}

void vx::VolumeDataBlockJpeg::setCompressedBlockData(
    const vx::Vector<size_t, 3>& blockId, const uint8_t* data, size_t length) {
  for (std::size_t i = 0; i < 3; i++) {
    if (blockId[i] >= blockCount()[i])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got invalid block ID");
  }
  quint64 pos = blockId[0] +
                blockCount()[0] * (blockId[1] + blockCount()[1] * blockId[2]);

  {
    QMutexLocker locker(&this->dataMutex);
    if (pos >= this->data.size())
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "pos >= this->data.size()");
    std::vector<uint8_t>& entry = this->data[pos];

    entry.resize(length);
    memcpy(entry.data(), data, length);
  }
}

void vx::VolumeDataBlockJpeg::decodeBlockNoCache(
    const vx::Vector<size_t, 3>& blockId, DataType dataType,
    const vx::Array3<void>& data) {
  if (dataType != vx::DataTypeTraitsByType<float>::getDataType())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.NotImplemented",
        "Not implemented: decodeBlockImpl() currently only supports float");
  vx::Array3<float> dataFloat(data);

  auto expectedBlockShape = this->getBlockShape(blockId);
  if (data.size<0>() != expectedBlockShape[0] ||
      data.size<1>() != expectedBlockShape[1] ||
      data.size<2>() != expectedBlockShape[2])
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Got invalid data shape in VolumeDataBlockJpeg::decodeBlockImpl()");

  for (std::size_t i = 0; i < 3; i++) {
    if (blockId[i] >= blockCount()[i])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got invalid block ID");
  }
  quint64 pos = blockId[0] +
                blockCount()[0] * (blockId[1] + blockCount()[1] * blockId[2]);

  // TODO: Avoid making a copy of data? (Needed because otherwise the lock
  // cannot be released)
  std::vector<uint8_t> compressedData;
  {
    QMutexLocker locker(&this->dataMutex);
    if (pos >= this->data.size())
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "pos >= this->data.size()");
    compressedData = this->data[pos];
  }

  if (compressedData.size()) {
    // TODO: Cache decoder instances
    auto decoder = decoderImplementation->createDecoder(
        vx::BlockJpegImplementation::ParametersRef(
            this->samplePrecision(), this->blockShape(), this->huffmanTableDC(),
            this->huffmanTableAC(), this->quantizationTableZigzag()));

    auto decoded =
        decoder->decode(compressedData.data(), compressedData.size());

    for (size_t z = 0; z < data.size<2>(); z++) {
      for (size_t y = 0; y < data.size<1>(); y++) {
        for (size_t x = 0; x < data.size<0>(); x++) {
          float val = decoded(x, y, z);
          val -= 1 << (this->samplePrecision() - 1);
          val = this->valueOffset() + this->valueScalingFactor() * val;
          dataFloat(x, y, z) = val;
        }
      }
    }
  } else {
    // 0-byte block, volume is just 0s
    for (size_t z = 0; z < data.size<2>(); z++) {
      for (size_t y = 0; y < data.size<1>(); y++) {
        for (size_t x = 0; x < data.size<0>(); x++) {
          // Note: Don't use 0 here, use the value which would be the result of
          // decoding an encoded 0 value.
          dataFloat(x, y, z) = zeroValue_;
        }
      }
    }
  }
}

void vx::VolumeDataBlockJpeg::encodeBlockImpl(
    const vx::Vector<size_t, 3>& blockId, DataType dataType,
    const vx::Array3<const void>& data) {
  if (dataType != vx::DataTypeTraitsByType<float>::getDataType())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.NotImplemented",
        "Not implemented: encodeBlockImpl() currently only supports float");
  vx::Array3<const float> dataFloat(data);

  auto expectedBlockShape = this->getBlockShape(blockId);
  if (data.size<0>() != expectedBlockShape[0] ||
      data.size<1>() != expectedBlockShape[1] ||
      data.size<2>() != expectedBlockShape[2])
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InvalidArgument",
        "Got invalid data shape in VolumeDataBlockJpeg::encodeBlockImpl()");

  for (std::size_t i = 0; i < 3; i++) {
    if (blockId[i] >= blockCount()[i])
      throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                          "Got invalid block ID");
  }
  quint64 pos = blockId[0] +
                blockCount()[0] * (blockId[1] + blockCount()[1] * blockId[2]);

  Array3<uint16_t> dataInt(this->blockShape().asArray());
  bool foundNonZero = false;
  for (size_t z = 0; z < this->blockShape()[2]; z++) {
    for (size_t y = 0; y < this->blockShape()[1]; y++) {
      for (size_t x = 0; x < this->blockShape()[0]; x++) {
        size_t x2 = x, y2 = y, z2 = z;
        // TODO: What kind of padding should be used?
        if (x2 >= expectedBlockShape[0]) x2 = expectedBlockShape[0] - 1;
        if (y2 >= expectedBlockShape[1]) y2 = expectedBlockShape[1] - 1;
        if (z2 >= expectedBlockShape[2]) z2 = expectedBlockShape[2] - 1;

        float val = dataFloat(x2, y2, z2);
        val = (val - this->valueOffset()) / this->valueScalingFactor();
        val += 1 << (this->samplePrecision() - 1);
        val = std::round(val);
        if (val < 0) val = 0;
        if (val >= 1 << this->samplePrecision())
          val = (1 << this->samplePrecision()) - 1;
        dataInt(x, y, z) = (uint16_t)val;
        if ((uint16_t)val != zeroValueEncoded_) foundNonZero = true;
      }
    }
  }

  std::shared_ptr<vx::BlockEncoder> encoder;
  std::tuple<void*, size_t> encoded;
  if (foundNonZero) {
    // TODO: Cache encoder instances
    encoder = encoderImplementation->createEncoder(
        vx::BlockJpegImplementation::ParametersRef(
            this->samplePrecision(), this->blockShape(), this->huffmanTableDC(),
            this->huffmanTableAC(), this->quantizationTableZigzag()));

    // TODO: Allow this (implicitly add const):
    // auto encoded = encoder->encode(dataInt);
    encoded =
        encoder->encode(Array3<const uint16_t>(Array3<const void>(dataInt)));
  } else {
    encoded = std::make_tuple(nullptr, 0);
    // qDebug() << "Got zero block";
  }
  auto encodedPtr = std::get<0>(encoded);
  auto encodedSize = std::get<1>(encoded);
  if (vx::debug_option::Log_BlockJpeg()->get())
    qDebug() << "Got" << encodedSize << "bytes after encoding";

  {
    QMutexLocker locker(&this->dataMutex);
    if (pos >= this->data.size())
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "pos >= this->data.size()");
    std::vector<uint8_t>& vec = this->data[pos];
    vec.resize(encodedSize);
    memcpy(vec.data(), encodedPtr, encodedSize);
  }
}

VX_BUFFER_TYPE_DEFINE(vx::VolumeDataBlockJpeg::BlockSize,
                      "de.uni_stuttgart.Voxie.VolumeDataBlockJpeg.BlockSize")

VX_BUFFER_TYPE_DEFINE(
    vx::VolumeDataBlockJpeg::HuffmanSymbolCounter,
    "de.uni_stuttgart.Voxie.VolumeDataBlockJpeg.HuffmanSymbolCounter")
