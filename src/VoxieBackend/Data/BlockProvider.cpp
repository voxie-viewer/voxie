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

#include "BlockProvider.hpp"

vx::DecodedBlock::DecodedBlock(const QSharedPointer<BlockProviderInfo>& info,
                               vx::DataType dataType,
                               const vx::Array3<void>& array)
    : info_(info), dataType_(dataType), array_(array) {}

vx::DecodedBlockReferenceBase::DecodedBlockReferenceBase(
    const DecodedBlockReferenceBase& o)
    : block_(o.block()) {}
vx::DecodedBlockReferenceBase::DecodedBlockReferenceBase(
    DecodedBlockReferenceBase&& o)
    : block_(std::move(o.block_)) {}
vx::DecodedBlockReferenceBase::DecodedBlockReferenceBase(
    const QSharedPointer<DecodedBlock>& block)
    : block_(block) {}
vx::DecodedBlockReferenceBase::DecodedBlockReferenceBase(
    QSharedPointer<DecodedBlock>&& block)
    : block_(std::move(block)) {}
vx::DecodedBlockReferenceBase::~DecodedBlockReferenceBase() {}

Q_NORETURN void
vx::DecodedBlockReferenceBase::raiseMismatchingDataTypeException(
    vx::DataType expected) {
  if (!block())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.Error",
        "Attempting to create DecodedBlockReference for null DecodedBlock");
  if (expected == block()->dataType())
    throw vx::Exception(
        "de.uni_stuttgart.Voxie.InternalError",
        "Got unexpected type in "
        "DecodedBlockReferenceBase::raiseMismatchingDataTypeException()");
  throw vx::Exception("de.uni_stuttgart.Voxie.Error",
                      "Type mismatch for DecodedBlockReference: Expected " +
                          getDataTypeString(expected) + ", got " +
                          getDataTypeString(block()->dataType()));
}

vx::BlockProviderOrCache::~BlockProviderOrCache() {}

vx::BlockProvider::~BlockProvider() {}
