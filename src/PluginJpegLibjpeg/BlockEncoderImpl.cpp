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

#include "BlockEncoderImpl.hpp"

#include <VoxieClient/Format.hpp>

#include <VoxieBackend/Data/BlockEncoder.hpp>

#include <PluginJpegLibjpeg/Common.hpp>
#include <PluginJpegLibjpeg/Libjpeg.hpp>

#include <cstring>

#include <iostream>

// #include <QtCore/QFile>

namespace vx {
namespace libjpeg {
namespace IMPL_NAMESPACE {
static const size_t outputBufferSize =
    1024 * 1024;  // TODO: How large should this be?

struct JpegCompressContext {
  jpeg_compress_struct cinfo;
  jpeg_error_mgr jerr;

  JpegCompressContext() {
    memset(&cinfo, 0, sizeof(cinfo));
    memset(&jerr, 0, sizeof(jerr));

    cinfo.err = jpeg_std_error(&jerr);
    cinfo.err->error_exit = handlerErrorExit;
    cinfo.err->emit_message = handlerEmitMessage;
    jpeg_create_compress(&cinfo);
  }

  ~JpegCompressContext() { jpeg_destroy_compress(&cinfo); }
};

#define ASSERT_BLOCK_ENCODER(x)                                       \
  do {                                                                \
    if (!(x))                                                         \
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",     \
                          "BlockEncoderImpl: Assertion failed: " #x); \
                                                                      \
  } while (0)
#define ASSERT_BLOCK_ENCODER_MSG(x, msg)                               \
  do {                                                                 \
    if (!(x))                                                          \
      throw vx::Exception(                                             \
          "de.uni_stuttgart.Voxie.InternalError",                      \
          QString("BlockEncoder: Assertion failed: " #x ":") + (msg)); \
                                                                       \
  } while (0)

// TODO: Move some methods to a common file for encoder and decoder?
static void setHuffmanTable(j_common_ptr cinfo, JHUFF_TBL** tbl,
                            const QList<QList<quint8>>& input) {
  ASSERT_BLOCK_ENCODER(input.size() == 16);

  if (!*tbl) *tbl = jpeg_alloc_huff_table(cinfo);
  ASSERT_BLOCK_ENCODER(*tbl);

  (*tbl)->bits[0] = 0;
  int pos = 0;
  for (int i = 0; i <= 15; i++) {
    (*tbl)->bits[i + 1] = input[i].size();
    for (int j = 0; j < input[i].size(); j++) {
      ASSERT_BLOCK_ENCODER(pos < 256);
      (*tbl)->huffval[pos] = input[i][j];
      pos++;
    }
  }
  while (pos < 256) {
    (*tbl)->huffval[pos] = 0;
    pos++;
  }
}

class MemoryDestinationManager : public jpeg_destination_mgr {
  Q_DISABLE_COPY(MemoryDestinationManager)

  uint8_t* ptr;
  size_t size;

  static void init_destination_impl(j_compress_ptr cinfo) {
    auto dest = (MemoryDestinationManager*)cinfo->dest;

    dest->next_output_byte = (JOCTET*)dest->ptr;
    dest->free_in_buffer = dest->size;
  }

  static boolean empty_output_buffer_impl(j_compress_ptr cinfo) {
    auto dest = (MemoryDestinationManager*)cinfo->dest;

    (void)dest;
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "empty_output_buffer() not implemented for memory buffer");
  }

  static void term_destination_impl(j_compress_ptr cinfo) {
    auto dest = (MemoryDestinationManager*)cinfo->dest;

    // Do nothing
    (void)dest;
  }

 public:
  MemoryDestinationManager(uint8_t* ptr, size_t size) {
    memset(this, 0, sizeof(jpeg_destination_mgr));

    this->ptr = ptr;
    this->size = size;

    this->init_destination = init_destination_impl;
    this->empty_output_buffer = empty_output_buffer_impl;
    this->term_destination = term_destination_impl;

    this->next_output_byte = nullptr;
    this->free_in_buffer = 0;
  }

  size_t usedBytes() {
    if (!this->next_output_byte)
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",
                          "MemoryDestinationManager: !this->next_output_byte");
    if (this->free_in_buffer > this->size)
      throw vx::Exception(
          "de.uni_stuttgart.Voxie.InternalError",
          "MemoryDestinationManager: this->free_in_buffer > this->size");
    return this->size - this->free_in_buffer;
  }
};

template <size_t bits>
class BlockEncoderImpl : public BlockEncoder {
  Q_DISABLE_COPY(BlockEncoderImpl)

  using Libjpeg = LibJpegImpl<bits>;
  using jsample = typename Libjpeg::jsample;

  vx::Vector<size_t, 3> blockShape_;
  const vx::Vector<size_t, 3>& blockShape() const { return blockShape_; }

  JpegCompressContext context;

  std::vector<uint8_t> expectedHeader;
  std::vector<uint8_t> expectedFooter;

  vx::Array3<jsample> inputData;
  std::vector<jsample*> scanlines;
  // vx::Array3<jsample> inputDataReordered;

  std::vector<uint8_t> outputData;
  std::shared_ptr<MemoryDestinationManager> dest;

  std::vector<uint8_t> outputData2;

  std::shared_ptr<std::vector<uint16_t>> quantizationTable_;

 public:
  BlockEncoderImpl(const BlockJpegImplementation::ParametersRef& parRef);
  ~BlockEncoderImpl() override;

  std::tuple<void*, size_t> encode(const Array3<const uint16_t>& data) override;
};

template <size_t bits>
BlockEncoderImpl<bits>::BlockEncoderImpl(
    const BlockJpegImplementation::ParametersRef& parRef)
    : blockShape_(parRef.blockShape),
      context(),
      // TODO: Block size
      inputData(this->blockShape().asArray()),
      // inputDataReordered(
      //     Math::reorderDimensions(inputData.view(), {1, 3, 2})),
      outputData(outputBufferSize),
      outputData2(outputBufferSize) {
  for (size_t dim = 0; dim < 3; dim++) {
    ASSERT_BLOCK_ENCODER(this->blockShape()[dim] > 0);
    ASSERT_BLOCK_ENCODER(this->blockShape()[dim] % 8 == 0);
  }

  ASSERT_BLOCK_ENCODER(this->inputData.template strideBytes<0>() ==
                       sizeof(jsample));
  ASSERT_BLOCK_ENCODER(this->inputData.template strideBytes<1>() ==
                       (ptrdiff_t)(this->blockShape()[0] * sizeof(jsample)));
  ASSERT_BLOCK_ENCODER(this->inputData.template strideBytes<2>() ==
                       (ptrdiff_t)(this->blockShape()[0] *
                                   this->blockShape()[1] * sizeof(jsample)));
  // TODO: Or use this->blockShape()[2] lines? imageWidth and imageHeight
  // would also have to be changed
  this->scanlines.resize(this->blockShape()[1] * this->blockShape()[2]);
  for (size_t i = 0; i < this->scanlines.size(); i++)
    this->scanlines[i] = &this->inputData(0, i % this->blockShape()[1],
                                          i / this->blockShape()[1]);
  uint16_t imageWidth = this->blockShape()[0];
  uint16_t imageHeight = this->blockShape()[1] * this->blockShape()[2];

  this->expectedHeader.insert(this->expectedHeader.end(),
                              {0xff, 0xd8});  // SOI marker
  uint8_t sofMarker = parRef.samplePrecision == 8 ? 0xc0 : 0xc1;
  this->expectedHeader.insert(
      this->expectedHeader.end(),
      {
          0xff, sofMarker,  // SOF0/SOF1 marker
          0x00, 0x0b,       // Frame header length
          (uint8_t)
              parRef.samplePrecision,  // Sample precision (bits per sample)
          (uint8_t)(imageHeight / 256),
          (uint8_t)(imageHeight % 256),  // Number of lines
          (uint8_t)(imageWidth / 256),
          (uint8_t)(imageWidth % 256),  // Number of columns
          0x01,                         // Number of components
          0x01,                         // Component 1 identifier
          0x11,                         // Component 1 H and V sampling factors
          0x00,  // Component 1 Quantization table destination selector
      });
  this->expectedHeader.insert(
      this->expectedHeader.end(),
      {
          0xff, 0xda,  // SOS marker
          0x00, 0x08,  // Scan header length
          0x01,        // Number of components in scan
          0x01,        // First scan component identifier (same as Component 1
                       // identifier in frame header)
          0x00,        // First scan component DC and AC entropy coding table
                       // selectors
          0x00,        // Start of spectral selection
          0x3f,        // End of spectral selection
          0x00,        // Successive approximation bit position high and low
      });

  this->expectedFooter.insert(this->expectedFooter.end(),
                              {0xff, 0xd9});  // EOI marker

#if VX_LIBJPEG_HAVE_RUNTIME
  this->context.cinfo.data_precision = parRef.samplePrecision;
#endif

  this->dest = std::make_shared<MemoryDestinationManager>(
      this->outputData.data(), this->outputData.size());
  this->context.cinfo.dest = this->dest.get();

  this->context.cinfo.image_width = imageWidth;
  this->context.cinfo.image_height = imageHeight;
  this->context.cinfo.input_components = 1;
  this->context.cinfo.in_color_space = JCS_GRAYSCALE;
  jpeg_set_defaults(&this->context.cinfo);

  // Disable optimal huffman tables even with 12-bit JPEG
  // Note: This is currently broken with libjpeg-turbo version 3, as
  // libjpeg-turbo will reenable optimize_coding in jinit_c_master_control()
  // See https://github.com/libjpeg-turbo/libjpeg-turbo/issues/751
  this->context.cinfo.optimize_coding = FALSE;

  // jpeg_set_colorspace(&this->context.cinfo, JCS_GRAYSCALE);

  // Set quantization matrix
  ASSERT_BLOCK_ENCODER(parRef.quantizationTable.size() == 64);
  if (!this->context.cinfo.quant_tbl_ptrs[0])
    this->context.cinfo.quant_tbl_ptrs[0] =
        jpeg_alloc_quant_table((j_common_ptr)(&this->context.cinfo));
  for (int i = 0; i < 64; i++)
    this->context.cinfo.quant_tbl_ptrs[0]->quantval[i] =
        parRef.quantizationTable[i];
  // TODO: Does quant_tbl_ptrs[1] have to be set?

  // Set huffman tables
  setHuffmanTable((j_common_ptr)(&this->context.cinfo),
                  &this->context.cinfo.dc_huff_tbl_ptrs[0],
                  parRef.huffmanTableDC);
  setHuffmanTable((j_common_ptr)(&this->context.cinfo),
                  &this->context.cinfo.ac_huff_tbl_ptrs[0],
                  parRef.huffmanTableAC);
  // TODO: Do HuffmanTable_DC_1/HuffmanTable_AC_1 have to be set?

  jpeg_suppress_tables(&this->context.cinfo, TRUE);
  this->context.cinfo.write_JFIF_header = FALSE;
  this->context.cinfo.write_Adobe_marker = FALSE;
}
template <size_t bits>
BlockEncoderImpl<bits>::~BlockEncoderImpl() {}

static size_t unstuff(std::vector<uint8_t>& output, const uint8_t* input,
                      size_t inputLength) {
  ASSERT_BLOCK_ENCODER(output.size() >= inputLength);

  size_t outputPos = 0;
  size_t pos = 0;
  while (pos < inputLength) {
    auto val = input[pos];
    pos++;
    if (val == 0xff) {
      ASSERT_BLOCK_ENCODER_MSG(pos < inputLength, "Got 0xff at end of data");
      auto val2 = input[pos];
      pos++;
      ASSERT_BLOCK_ENCODER_MSG(val2 == 0x00,
                               QString("Got unexpected marker: %1")
                                   .arg(val2, 2, 16, QLatin1Char('0')));
    }
    output[outputPos] = val;
    outputPos++;
  }

  return outputPos;
}

template <size_t bits>
std::tuple<void*, size_t> BlockEncoderImpl<bits>::encode(
    const Array3<const uint16_t>& data) {
  // this->inputDataReordered.assign(data);
  // this->inputData.assign(data);
  ASSERT_BLOCK_ENCODER(data.size<0>() == this->inputData.template size<0>());
  ASSERT_BLOCK_ENCODER(data.size<1>() == this->inputData.template size<1>());
  ASSERT_BLOCK_ENCODER(data.size<2>() == this->inputData.template size<2>());
  for (size_t z = 0; z < data.size<2>(); z++)
    for (size_t y = 0; y < data.size<1>(); y++)
      for (size_t x = 0; x < data.size<0>(); x++)
        this->inputData(x, y, z) = data(x, y, z);

  jpeg_start_compress(&this->context.cinfo, FALSE);
  this->context.cinfo.optimize_coding = FALSE;

  Libjpeg::write_scanlines(&this->context.cinfo, this->scanlines.data(),
                           this->scanlines.size());

  jpeg_finish_compress(&this->context.cinfo);

#if 0
  // TODO: Remove
  if (false) {
    for (int j = 0; j < 2; j++) {
      auto table = this->context.cinfo.quant_tbl_ptrs[j];
      ASSERT_BLOCK_ENCODER(table);
      std::cout << "const uint16_t QuantizationTable_" << j << "[64] = {\n";
      for (int i = 0; i < 64; i++) {
        std::cout << table->quantval[i] << ",";
        if (i % 8 == 7)
          std::cout << "//\n";
        else
          std::cout << " ";
      }
      std::cout << "};\n\n";
    }
    ASSERT_BLOCK_ENCODER(!this->context.cinfo.quant_tbl_ptrs[2]);

    for (int j = 0; j < 2; j++) {
      auto table = this->context.cinfo.dc_huff_tbl_ptrs[j];
      ASSERT_BLOCK_ENCODER(table);
      dumpHuffmanTable(Core::OStream::getStdout(),
                       Core::sprintf("HuffmanTable_DC_%d", j),
                       getHuffmanTable(table));
    }
    ASSERT_BLOCK_ENCODER(!this->context.cinfo.dc_huff_tbl_ptrs[2]);

    for (int j = 0; j < 2; j++) {
      auto table = this->context.cinfo.ac_huff_tbl_ptrs[j];
      ASSERT_BLOCK_ENCODER(table);
      dumpHuffmanTable(Core::OStream::getStdout(),
                       Core::sprintf("HuffmanTable_AC_%d", j),
                       getHuffmanTable(table));
    }
    ASSERT_BLOCK_ENCODER(!this->context.cinfo.ac_huff_tbl_ptrs[2]);

    std::cout << std::flush;
    exit(0);
  }
#endif

  auto ptr = this->outputData.data();
  auto size = this->dest->usedBytes();

  /*
  QFile file("/tmp/foo.jpeg");
  file.open(QIODevice::WriteOnly);
  file.write(QByteArray((const char*)ptr, size));
  file.close();
  */

  ASSERT_BLOCK_ENCODER(size >= this->expectedHeader.size() +
                                   this->expectedFooter.size());
  if (memcmp(this->expectedHeader.data(), ptr, this->expectedHeader.size()) !=
      0) {
    // TODO: print nicer hexdump?
    for (size_t i = 0; i < this->expectedHeader.size(); i++) {
      qDebug() << vx::format("{:03x} exp={:02x} act={:02x}{}", i,
                             this->expectedHeader[i], ptr[i],
                             this->expectedHeader[i] != ptr[i] ? " !!!" : "");
    }
    ASSERT_BLOCK_ENCODER_MSG(0, "Header mismatch");
  }
  if (memcmp(this->expectedFooter.data(),
             ptr + (size - this->expectedFooter.size()),
             this->expectedFooter.size()) != 0) {
    // TODO: print differences?
    ASSERT_BLOCK_ENCODER_MSG(0, "Footer mismatch");
  }
  ptr += this->expectedHeader.size();
  size -= this->expectedHeader.size() + this->expectedFooter.size();

  size = unstuff(this->outputData2, ptr, size);
  ptr = this->outputData2.data();

  return std::make_tuple(ptr, size);
}

std::shared_ptr<BlockEncoder> createBlockEncoder(
    const BlockJpegImplementation::ParametersRef& parRef) {
#if VX_LIBJPEG_HAVE_8BIT
  if (parRef.samplePrecision == 8)
    return std::make_shared<BlockEncoderImpl<8>>(parRef);
#endif
#if VX_LIBJPEG_HAVE_12BIT
  if (parRef.samplePrecision == 12)
    return std::make_shared<BlockEncoderImpl<12>>(parRef);
#endif
  throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                      "JPEG Encoder does not support sample precision " +
                          QString::number(parRef.samplePrecision));
}
}  // namespace IMPL_NAMESPACE
}  // namespace libjpeg
}  // namespace vx
