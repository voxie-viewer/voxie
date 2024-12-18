/*
 * Copyright (c) 2014-2024 The Voxie Authors
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

#include <VoxieClient/Array.hpp>

#include <VoxieBackend/VoxieBackend.hpp>

#include <VoxieBackend/Component/Component.hpp>
#include <VoxieBackend/Component/ComponentTypeInfo.hpp>

#include <memory>

namespace vx {
class BlockDecoder;
class BlockEncoder;

class VOXIEBACKEND_EXPORT BlockJpegImplementation : public vx::Component {
 public:
  // Note: This structure contains references and should not be used after the
  // referenced data is destroyed
  struct ParametersRef {
    quint32 samplePrecision;

    const vx::Vector<size_t, 3>& blockShape;

    const QList<QList<quint8>>& huffmanTableDC;
    const QList<QList<quint8>>& huffmanTableAC;

    const std::vector<quint16>& quantizationTable;

    ParametersRef(quint32 samplePrecision,
                  const vx::Vector<size_t, 3>& blockShape,
                  const QList<QList<quint8>>& huffmanTableDC,
                  const QList<QList<quint8>>& huffmanTableAC,
                  const std::vector<quint16>& quantizationTable)
        : samplePrecision(samplePrecision),
          blockShape(blockShape),
          huffmanTableDC(huffmanTableDC),
          huffmanTableAC(huffmanTableAC),
          quantizationTable(quantizationTable) {}
  };

  BlockJpegImplementation(const QString& name);
  virtual ~BlockJpegImplementation();

  virtual bool hasDecoder() = 0;
  virtual bool supportsParametersDecoder(const ParametersRef& parRef) = 0;
  virtual std::shared_ptr<BlockDecoder> createDecoder(
      const ParametersRef& parRef) = 0;

  virtual bool hasEncoder() = 0;
  virtual bool supportsParametersEncoder(const ParametersRef& parRef) = 0;
  virtual std::shared_ptr<BlockEncoder> createEncoder(
      const ParametersRef& parRef) = 0;

  virtual int priority() = 0;

  QList<QString> supportedComponentDBusInterfaces() override;
};

template <>
struct ComponentTypeInfo<BlockJpegImplementation> {
  static const char* name() {
    return "de.uni_stuttgart.Voxie.ComponentType.BlockJpegImplementation";
  }
};
}  // namespace vx
