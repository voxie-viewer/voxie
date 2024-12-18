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

#include "BlockDecoderImpl.hpp"

#include <VoxieBackend/Data/BlockDecoder.hpp>

#include <PluginJpegLibjpeg/Common.hpp>
#include <PluginJpegLibjpeg/Libjpeg.hpp>

// TODO: Do not ignore libjpeg warnings

namespace vx {
namespace libjpeg {
namespace IMPL_NAMESPACE {
static const size_t inputBufferSize =
    1024 * 1024;  // TODO: How large should this be?

struct JpegDecompressContext {
  jpeg_decompress_struct cinfo;
  jpeg_error_mgr jerr;

  JpegDecompressContext() {
    memset(&cinfo, 0, sizeof(cinfo));
    memset(&jerr, 0, sizeof(jerr));

    cinfo.err = jpeg_std_error(&jerr);
    cinfo.err->error_exit = handlerErrorExit;
    cinfo.err->emit_message = handlerEmitMessage;
    jpeg_create_decompress(&cinfo);
  }

  ~JpegDecompressContext() { jpeg_destroy_decompress(&cinfo); }
};

#define ASSERT_MEMORYSOURCEMANAGER(x)                                    \
  do {                                                                   \
    if (!(x))                                                            \
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",        \
                          "MemorySourceManager: Assertion failed: " #x); \
                                                                         \
  } while (0)
#define ASSERT_MEMORYSOURCEMANAGER_MSG(x, msg)                                \
  do {                                                                        \
    if (!(x))                                                                 \
      throw vx::Exception(                                                    \
          "de.uni_stuttgart.Voxie.InternalError",                             \
          QString("MemorySourceManager: Assertion failed: " #x ":") + (msg)); \
                                                                              \
  } while (0)
#define ABORT_MEMORYSOURCEMANAGER_MSG(msg)                         \
  do {                                                             \
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",    \
                        QString("MemorySourceManager: ") + (msg)); \
                                                                   \
  } while (0)

class MemorySourceManager : public jpeg_source_mgr {
  Q_DISABLE_COPY(MemorySourceManager)

  uint8_t* ptr;
  size_t size;
  size_t maxSize;

  static void init_source_impl(j_decompress_ptr cinfo) {
    auto src = (MemorySourceManager*)cinfo->src;

    src->next_input_byte = (JOCTET*)src->ptr;
    src->bytes_in_buffer = src->size;
  }

  static boolean fill_input_buffer_impl(j_decompress_ptr cinfo) {
    auto src = (MemorySourceManager*)cinfo->src;

    (void)src;
    ABORT_MEMORYSOURCEMANAGER_MSG("Reached EOF while reading data");
  }

  static void skip_input_data_impl(j_decompress_ptr cinfo, long num_bytes) {
    auto src = (MemorySourceManager*)cinfo->src;

    (void)src;
    (void)num_bytes;
    ABORT_MEMORYSOURCEMANAGER_MSG(
        "skip_input_data() not implemented for memory buffer");
  }

  static boolean resync_to_restart_impl(j_decompress_ptr cinfo, int desired) {
    auto src = (MemorySourceManager*)cinfo->src;

    (void)src;
    (void)desired;
    ABORT_MEMORYSOURCEMANAGER_MSG(
        "resync_to_restart() not implemented for memory buffer");
  }

  static void term_source_impl(j_decompress_ptr cinfo) {
    auto src = (MemorySourceManager*)cinfo->src;

    ASSERT_MEMORYSOURCEMANAGER_MSG(
        src->bytes_in_buffer == 0,
        "Got " + QString::number(src->bytes_in_buffer) + " leftover bytes");
  }

 public:
  MemorySourceManager(uint8_t* ptr, size_t maxSize) {
    memset(this, 0, sizeof(jpeg_source_mgr));

    this->ptr = ptr;
    this->maxSize = maxSize;
    this->size = 0;

    this->init_source = init_source_impl;
    this->fill_input_buffer = fill_input_buffer_impl;
    this->skip_input_data = skip_input_data_impl;
    this->resync_to_restart = resync_to_restart_impl;
    this->term_source = term_source_impl;

    this->next_input_byte = nullptr;
    this->bytes_in_buffer = 0;
  }

  void setSize(size_t size) {
    ASSERT_MEMORYSOURCEMANAGER(size < this->maxSize);
    this->size = size;
  }
};

template <size_t bits>
class BlockDecoderImpl : public BlockDecoder {
  Q_DISABLE_COPY(BlockDecoderImpl)

  using Libjpeg = LibJpegImpl<bits>;
  using jsample = typename Libjpeg::jsample;

  vx::Vector<size_t, 3> blockShape_;
  const vx::Vector<size_t, 3>& blockShape() const { return blockShape_; }

  JpegDecompressContext context;

  std::vector<uint8_t> expectedHeader;
  std::vector<uint8_t> expectedFooter;

  std::vector<uint8_t> inputData;
  std::shared_ptr<MemorySourceManager> source;

  vx::Array3<jsample> outputData;
  std::vector<jsample*> scanlines;
  // vx::Array3<jsample, 3> outputDataReordered;
  // TODO: Should this be allocated by the caller or by BlockDecoderImpl?
  // TODO: Avoid this by returning a polymorphic array reference?
  // TODO: Avoid this when jsample is uint16_t?
  vx::Array3<uint16_t> outputDataConverted;

  static size_t stuff(uint8_t* output, size_t outputLength,
                      const uint8_t* input, size_t inputLength);

 public:
  BlockDecoderImpl(const BlockJpegImplementation::ParametersRef& parRef);
  virtual ~BlockDecoderImpl();

  vx::Array3<const uint16_t> decode(const void* ptr, size_t size) override;
};

#define ASSERT_BLOCK_DECODER(x)                                       \
  do {                                                                \
    if (!(x))                                                         \
      throw vx::Exception("de.uni_stuttgart.Voxie.InternalError",     \
                          "BlockDecoderImpl: Assertion failed: " #x); \
                                                                      \
  } while (0)
#define ASSERT_BLOCK_DECODER_MSG(x, msg)                                   \
  do {                                                                     \
    if (!(x))                                                              \
      throw vx::Exception(                                                 \
          "de.uni_stuttgart.Voxie.InternalError",                          \
          QString("BlockDecoderImpl: Assertion failed: " #x ":") + (msg)); \
                                                                           \
  } while (0)
#define ABORT_BLOCK_DECODER_MSG(msg)                            \
  do {                                                          \
    throw vx::Exception("de.uni_stuttgart.Voxie.InternalError", \
                        QString("BlockDecoderImpl: ") + (msg)); \
                                                                \
  } while (0)

// TODO: Move some methods to a common file for encoder and decoder?
static void setHuffmanTable(j_common_ptr cinfo, JHUFF_TBL** tbl,
                            const QList<QList<quint8>>& input) {
  ASSERT_BLOCK_DECODER(input.size() == 16);

  if (!*tbl) *tbl = jpeg_alloc_huff_table(cinfo);
  ASSERT_BLOCK_DECODER(*tbl);

  (*tbl)->bits[0] = 0;
  int pos = 0;
  for (int i = 0; i <= 15; i++) {
    (*tbl)->bits[i + 1] = input[i].size();
    for (int j = 0; j < input[i].size(); j++) {
      ASSERT_BLOCK_DECODER(pos < 256);
      (*tbl)->huffval[pos] = input[i][j];
      pos++;
    }
  }
  while (pos < 256) {
    (*tbl)->huffval[pos] = 0;
    pos++;
  }
}

template <size_t bits>
BlockDecoderImpl<bits>::BlockDecoderImpl(
    const BlockJpegImplementation::ParametersRef& parRef)
    : blockShape_(parRef.blockShape),
      context(),
      inputData(inputBufferSize),
      // TODO: Block size
      outputData(this->blockShape().asArray()),
      // outputDataReordered(Math::reorderDimensions(outputData.view(), {1, 3,
      // 2})),
      outputDataConverted(this->blockShape().asArray()) {
  for (size_t dim = 0; dim < 3; dim++) {
    ASSERT_BLOCK_DECODER(this->blockShape()[dim] > 0);
    ASSERT_BLOCK_DECODER(this->blockShape()[dim] % 8 == 0);
  }

  ASSERT_BLOCK_DECODER(this->outputData.template strideBytes<0>() ==
                       sizeof(jsample));
  ASSERT_BLOCK_DECODER(this->outputData.template strideBytes<1>() ==
                       (ptrdiff_t)(this->blockShape()[0] * sizeof(jsample)));
  ASSERT_BLOCK_DECODER(this->outputData.template strideBytes<2>() ==
                       (ptrdiff_t)(this->blockShape()[0] *
                                   this->blockShape()[1] * sizeof(jsample)));
  // TODO: Or use this->blockShape()[2] lines? imageWidth and imageHeight
  // would also have to be changed
  this->scanlines.resize(this->blockShape()[1] * this->blockShape()[2]);
  for (size_t i = 0; i < this->scanlines.size(); i++)
    this->scanlines[i] = &this->outputData(0, i % this->blockShape()[1],
                                           i / this->blockShape()[1]);
  uint16_t imageWidth = this->blockShape()[0];
  uint16_t imageHeight = this->blockShape()[1] * this->blockShape()[2];

  // TODO: Duplicated from BlockEncoder.cpp

  expectedHeader.insert(expectedHeader.end(), {0xff, 0xd8});  // SOI marker
  uint8_t sofMarker = parRef.samplePrecision == 8 ? 0xc0 : 0xc1;
  expectedHeader.insert(
      expectedHeader.end(),
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
  expectedHeader.insert(
      expectedHeader.end(),
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

  expectedFooter.insert(expectedFooter.end(), {0xff, 0xd9});  // EOI marker

#if VX_LIBJPEG_HAVE_RUNTIME
  this->context.cinfo.data_precision = parRef.samplePrecision;
#endif

  source =
      std::make_shared<MemorySourceManager>(inputData.data(), inputData.size());
  context.cinfo.src = source.get();

  // Set quantization matrix
  ASSERT_BLOCK_DECODER(parRef.quantizationTable.size() == 64);
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

  ASSERT_BLOCK_DECODER(expectedHeader.size() <= inputData.size());
  memcpy(inputData.data(), expectedHeader.data(), expectedHeader.size());
}
template <size_t bits>
BlockDecoderImpl<bits>::~BlockDecoderImpl() {}

template <size_t bits>
size_t BlockDecoderImpl<bits>::stuff(uint8_t* output, size_t outputLength,
                                     const uint8_t* input, size_t inputLength) {
  ASSERT_BLOCK_DECODER(outputLength >= 2 * inputLength);

  size_t outputPos = 0;
  for (size_t pos = 0; pos < inputLength; pos++) {
    auto val = input[pos];
    output[outputPos] = val;
    outputPos++;
    if (val == 0xff) {
      output[outputPos] = 0x00;
      outputPos++;
    }
  }

  return outputPos;
}

template <size_t bits>
vx::Array3<const uint16_t> BlockDecoderImpl<bits>::decode(const void* ptr,
                                                          size_t size) {
  auto pos =
      expectedHeader.size() + stuff(inputData.data() + expectedHeader.size(),
                                    inputData.size() - expectedHeader.size(),
                                    (const uint8_t*)ptr, size);
  ASSERT_BLOCK_DECODER(pos + expectedFooter.size() <= inputData.size());
  memcpy(inputData.data() + pos, expectedFooter.data(), expectedFooter.size());
  pos += expectedFooter.size();
  source->setSize(pos);

  // Core::writeFile(Core::OStream::open("/tmp/x2"), std::string((const
  // char*)inputData.data(), pos));

  jpeg_read_header(&context.cinfo, TRUE);

  jpeg_start_decompress(&context.cinfo);

  size_t spos = 0;
  while (spos < scanlines.size()) {
    auto count = Libjpeg::read_scanlines(
        &context.cinfo, scanlines.data() + spos,
        vx::checked_cast<JDIMENSION>(scanlines.size() - spos));
    ASSERT_BLOCK_DECODER(count > 0);
    spos += count;
    ASSERT_BLOCK_DECODER(spos <= scanlines.size());
  }

  jpeg_finish_decompress(&context.cinfo);

  // return outputDataReordered;
  for (size_t z = 0; z < outputDataConverted.size<2>(); z++)
    for (size_t y = 0; y < outputDataConverted.size<1>(); y++)
      for (size_t x = 0; x < outputDataConverted.size<0>(); x++)
        this->outputDataConverted(x, y, z) = this->outputData(x, y, z);

  // TODO: Allow this (implicitly add const):
  // return outputDataConverted;
  return Array3<const uint16_t>(Array3<const void>(outputDataConverted));
}

std::shared_ptr<BlockDecoder> createBlockDecoder(
    const BlockJpegImplementation::ParametersRef& parRef) {
#if VX_LIBJPEG_HAVE_8BIT
  if (parRef.samplePrecision == 8)
    return std::make_shared<BlockDecoderImpl<8>>(parRef);
#endif
#if VX_LIBJPEG_HAVE_12BIT
  if (parRef.samplePrecision == 12)
    return std::make_shared<BlockDecoderImpl<12>>(parRef);
#endif
  throw vx::Exception("de.uni_stuttgart.Voxie.InvalidArgument",
                      "JPEG Decoder does not support sample precision " +
                          QString::number(parRef.samplePrecision));
}
}  // namespace IMPL_NAMESPACE
}  // namespace libjpeg
}  // namespace vx
