#!/usr/bin/python3
#
# Copyright (c) 2014-2022 The Voxie Authors
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import numpy as np
import voxie
import time


def format_num(num):
    num_str = '{}'.format(num)
    groups = []
    pos = len(num_str)
    while pos > 0:
        new_pos = pos - 3
        if new_pos < 0:
            new_pos = 0
        groups.insert(0, num_str[new_pos:pos])
        pos = new_pos
    return '\xa0'.join(groups)


def table_to_html(data, line_len):
    result_text = ''
    lines = []
    for count in data:
        if len(lines) == 0 or len(lines[-1]) >= 16:
            lines.append([])
        lines[-1].append(count)
    result_text += '<table>\n'
    for line in lines:
        result_text += '<tr>'
        for count in line:
            result_text += '<td>'
            result_text += format_num(count)
            result_text += '</td>'
        result_text += '</tr>\n'
    result_text += '</table>\n'
    return result_text


# TODO: Align data for monospace font?
def table_to_md(data, line_len, skip=[]):
    skip = set(skip)
    result_text = ''
    lines = []
    for i, count in enumerate(data):
        if len(lines) == 0 or len(lines[-1]) >= 16:
            lines.append([])
        if i in skip:
            lines[-1].append(None)
        else:
            lines[-1].append(count)
    for i in range(line_len):
        result_text += '| {:x} '.format(i)
    result_text += '|\n'
    for i in range(line_len):
        result_text += '| ---: '
    result_text += '|\n'
    for line in lines:
        for count in line:
            if count is None:
                result_text += '| '
            else:
                result_text += '| {} '.format(format_num(count))
        result_text += '|\n'
    result_text += '\n'
    return result_text


def get_huffman_length(table, num):
    lengths = np.zeros(num, dtype=np.uint64)
    for l_min_one, symbols in enumerate(table):
        for symb in symbols:
            if symb >= num:
                raise Exception('symb >= num')
            if lengths[symb] != 0:
                raise Exception('lengths[symb] != 0')
            lengths[symb] = l_min_one + 1
    return lengths


def get_huffman_free_codewords(table):
    overall_codewords = 0
    for l_min_one, symbols in enumerate(table):
        length = l_min_one + 1
        overall_codewords += len(symbols) << (16 - length)
    return 2**16 - overall_codewords


def get_overall_huffman_length(table, counters):
    lengths = {}
    for l_min_one, symbols in enumerate(table):
        for symb in symbols:
            if symb in lengths:
                raise Exception('symb in lengths')
            lengths[symb] = l_min_one + 1

    res = 0
    for symb, count in enumerate(counters):
        if count == 0:
            continue
        if symb not in lengths:
            raise Exception('Unable to find length for symbol {}'.format(symb))
        res += int(count) * lengths[symb]
    return res


def get_overall_value_length(counters):
    res = 0
    for symb, count in enumerate(counters):
        if count == 0:
            continue
        value_len = symb & 0x0f
        res += int(count) * value_len
    return res


args = voxie.parser.parse_args()
context = voxie.VoxieContext(args)
instance = context.createInstance()

if args.voxie_action != 'RunFilter':
    raise Exception('Invalid operation: ' + repr(args.voxie_action))

start_time = time.monotonic()
with context.makeObject(context.bus, context.busName, args.voxie_operation, ['de.uni_stuttgart.Voxie.ExternalOperationRunFilter']).ClaimOperationAndCatch() as op:
    inputData = op.GetInputData('de.uni_stuttgart.Voxie.Input').CastTo('de.uni_stuttgart.Voxie.VolumeDataBlockJpeg')

    output_path = op.Properties['de.uni_stuttgart.Voxie.TextOutput'].getValue('o')

    result_text = ''

    blockIDBT = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.BufferType', 'de.uni_stuttgart.Voxie.VolumeDataBlock.BlockID').CastTo('de.uni_stuttgart.Voxie.BufferType')
    blockSizeBT = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.BufferType', 'de.uni_stuttgart.Voxie.VolumeDataBlockJpeg.BlockSize').CastTo('de.uni_stuttgart.Voxie.BufferType')
    huffmanSymbolCounterBT = instance.Components.GetComponent('de.uni_stuttgart.Voxie.ComponentType.BufferType', 'de.uni_stuttgart.Voxie.VolumeDataBlockJpeg.HuffmanSymbolCounter').CastTo('de.uni_stuttgart.Voxie.BufferType')

    blockCount = inputData.BlockCount
    blockShape = inputData.BlockShape
    arrayShape = inputData.ArrayShape

    sizes = np.zeros(blockCount[0] * blockCount[1] * blockCount[2], dtype=np.uint64)
    bufferSize = blockCount[0] * blockCount[1]
    with instance.CreateBuffer(0, ['array', [bufferSize], [blockIDBT.SizeBytes], blockIDBT.Type]) as blockIDsBuffer, instance.CreateBuffer(0, ['array', [bufferSize], [blockSizeBT.SizeBytes], blockSizeBT.Type]) as blockSizeBuffer:
        with instance.CreateBuffer(0, ['array', [16], [huffmanSymbolCounterBT.SizeBytes], huffmanSymbolCounterBT.Type]) as dcSymbolCount, instance.CreateBuffer(0, ['array', [256], [huffmanSymbolCounterBT.SizeBytes], huffmanSymbolCounterBT.Type]) as acSymbolCount, instance.CreateBuffer(0, ['array', [8], [huffmanSymbolCounterBT.SizeBytes], huffmanSymbolCounterBT.Type]) as paddingBitCount:
            dcSymbolCount[:] = 0
            acSymbolCount[:] = 0
            paddingBitCount[:] = 0
            for z in range(0, blockCount[2]):
                op.ThrowIfCancelled()

                blockIDsPos = np.arange(blockCount[0] * blockCount[1], dtype=np.uint64)
                blockIDsX = blockIDsPos % blockCount[0]
                blockIDsY = blockIDsPos // blockCount[0]
                blockIDsBuffer[:, 0] = blockIDsX
                blockIDsBuffer[:, 1] = blockIDsY
                blockIDsBuffer[:, 2] = z

                inputData.GetCompressedBlockSizes(bufferSize, blockIDsBuffer, 0, blockSizeBuffer, 0)
                inputData.CountHuffmanSymbols(bufferSize, blockIDsBuffer, 0, dcSymbolCount, 0, acSymbolCount, 0, paddingBitCount, 0)
                sizes[z * bufferSize:(z + 1) * bufferSize] = blockSizeBuffer
                op.SetProgress((z + 1) / blockCount[2])

    sizes = np.asarray(sizes)
    minSize = np.min(sizes)
    maxSize = np.max(sizes)
    minNonzeroSize = None
    if maxSize != 0:
        minNonzeroSize = np.min(sizes[sizes != 0])
    meanSize = np.mean(sizes)
    overallSize = np.sum(sizes)
    count = sizes.shape[0]
    zeroBlocks = np.count_nonzero(sizes == 0)
    blockShapeBytes = np.product(blockShape)
    meanBitsPerVoxel = overallSize * 8 / np.product(arrayShape)
    samplePrecision = inputData.SamplePrecision

    result_text += f'''General
-------

- Block shape: {blockShape}
- Block count: {blockCount}
- Zero blocks: {zeroBlocks}
- Number of blocks: {count}
- Smallest block: {minSize} bytes
- Smallest non-zero block: {minNonzeroSize} bytes
- Largest block: {maxSize} bytes
- Average size: {meanSize} bytes
- Uncompressed uint8 size: {blockShapeBytes} bytes, ratio {blockShapeBytes / meanSize:0.2f}
- Uncompressed uint16 size: {2 * blockShapeBytes} bytes, ratio {2 * blockShapeBytes / meanSize:0.2f}
- Uncompressed float size: {4 * blockShapeBytes} bytes, ratio {4 * blockShapeBytes / meanSize:0.2f}
- Overall size: {overallSize} bytes
- Average bits per voxel: {meanBitsPerVoxel:.2f} bits
- Sample precision: {samplePrecision} bits
'''

    tableDC = inputData.HuffmanTableDC
    tableAC = inputData.HuffmanTableAC
    dc_symb_len = get_overall_huffman_length(tableDC, dcSymbolCount)
    dc_value_len = get_overall_value_length(dcSymbolCount)
    ac_symb_len = get_overall_huffman_length(tableAC, acSymbolCount)
    ac_value_len = get_overall_value_length(acSymbolCount)
    overall_len = dc_symb_len + dc_value_len + ac_symb_len + ac_value_len
    padding_len = np.sum(paddingBitCount[:] * range(8))
    overall_len_with_padding = overall_len + padding_len
    if overall_len_with_padding != int(overallSize) * 8:
        raise Exception('overall_len_with_padding != int(overallSize) * 8')
    if count - zeroBlocks != np.sum(paddingBitCount):
        raise Exception('count - zeroBlocks != np.sum(paddingBitCount)')
    avg_padding_len = padding_len / count
    nz_avg_padding_len = None
    if count != zeroBlocks:
        nz_avg_padding_len = padding_len / (count - zeroBlocks)

    result_text += f'''
Huffman statistics
------------------
- DC symbol length: {format_num(dc_symb_len)} bits
- DC value length: {format_num(dc_value_len)} bits
- AC symbol length: {format_num(ac_symb_len)} bits
- AC value length: {format_num(ac_value_len)} bits
- Overall length: {format_num(overall_len)} bits
- Padding length: {format_num(padding_len)} bits
- Average padding length: {avg_padding_len:.5f} bits
- Average non-zero-block padding length: {nz_avg_padding_len:.5f} bits
'''

    invalidDcSymbols = []
    if samplePrecision == 8:
        # Table F.1
        maxDcSymbol = 11
    elif samplePrecision == 12:
        # Table F.6
        maxDcSymbol = 15
    for i in range(maxDcSymbol + 1, len(dcSymbolCount[:])):
        invalidDcSymbols.append(i)
    for i in invalidDcSymbols:
        if dcSymbolCount[i] != 0:
            raise Exception('Got invalid DC symbol {:02x}: {}'.format(i, dcSymbolCount[i]))

    invalidAcSymbols = []
    for i in range(1, 15):
        invalidAcSymbols.append(i << 4)
    if samplePrecision == 8:
        # Table F.2
        maxAcSymbol = 10
    elif samplePrecision == 12:
        # Table F.7
        maxAcSymbol = 14
    for i in range(maxAcSymbol + 1, 16):
        for j in range(0, 16):
            invalidAcSymbols.append(i + (j << 4))
    for i in invalidAcSymbols:
        if acSymbolCount[i] != 0:
            raise Exception('Got invalid AC symbol {:02x}: {}'.format(i, acSymbolCount[i]))

    if False:
        result_text += f'''
Data
----
```
'''
        result_text += f'''dcSymbolCount={repr(dcSymbolCount[:])}\n'''
        result_text += f'''acSymbolCount={repr(acSymbolCount[:])}\n'''
        result_text += '```\n'

    result_text += '\n'
    result_text += '### Padding bit count\n'
    result_text += table_to_md(paddingBitCount, 8)
    result_text += '### DC symbol count\n'
    result_text += table_to_md(dcSymbolCount, 16, invalidDcSymbols)
    result_text += '### AC symbol count\n'
    result_text += table_to_md(acSymbolCount, 16, invalidAcSymbols)

    current_huffman_lengths_dc = get_huffman_length(tableDC, 16)
    free_dc_codewords = get_huffman_free_codewords(tableDC)
    current_huffman_lengths_ac = get_huffman_length(tableAC, 256)
    free_ac_codewords = get_huffman_free_codewords(tableAC)
    result_text += '### Current Huffman Table DC lengths\n'
    result_text += 'Free codewords: {} / 65536\n'.format(free_dc_codewords)
    result_text += table_to_md(current_huffman_lengths_dc, 16, np.nonzero(current_huffman_lengths_dc == 0)[0])
    result_text += '### Current Huffman Table AC lengths\n'
    result_text += 'Free codewords: {} / 65536\n'.format(free_ac_codewords)
    result_text += table_to_md(current_huffman_lengths_ac, 16, np.nonzero(current_huffman_lengths_ac == 0)[0])

    # TODO: Optimize table, print new statistics

    if False:
        result_text += '''
HTML
----
```
<style>
table, th, td {
  border: 1px solid black;
  border-collapse: collapse;
}
</style>
'''
        result_text += table_to_html(dcSymbolCount, 16)
        result_text += '<br>\n'
        result_text += table_to_html(acSymbolCount, 16)
        result_text += '''```
'''

    result_text_bin = (result_text).encode('utf-8')

    with instance.CreateFileDataByteStream('text/markdown; charset=UTF-8; variant=CommonMark', len(result_text_bin)) as output:
        with output.CreateUpdate() as update:
            with output.GetBufferWritable(update) as buffer:
                buffer[:] = bytearray(result_text_bin)

            with update.Finish() as version:
                result = {}
                result[output_path] = {
                    'Data': voxie.Variant('o', output._objectPath),
                    'DataVersion': voxie.Variant('o', version._objectPath),
                }
                op.Finish(result)

    instance._context.client.destroy()
print('Wall clock time: {}, CPU time: {}'.format(time.monotonic() - start_time, time.process_time()))
