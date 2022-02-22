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

// Kernel extracts a slice image from a volume

float4 metricToNormal(float4 point, float4 volSize)
{
    return ((float4)( point.x/volSize.x, point.y/volSize.y, point.z/volSize.z, 0 ));
}

float4 pixelToVolume(float relX, float relY, float4 area, float4 planeOrigin, float4 xAxis, float4 yAxis)
{
    // map point to plane
    float aX = (relX * area.z) + area.x;
    float aY = (relY * area.w) + area.y;
    // map point to volume
    return planeOrigin + (aX * xAxis) + (aY * yAxis);
}

const sampler_t samplerNN = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void extract_slice_nearest(
            __read_only image3d_t volume,
            __global float* image,
            const float4 volSize_imgWidth,
            const uint stride,
            const float4 origin,
            const float4 xAxis,
            const float4 yAxis,
            const float4 areaRect,
            const uint useInt)
{
/* - - - - - - - - - - - - - - - - - */
    int i = get_global_id(0);
    int imgWidth = (int) volSize_imgWidth.w;
    int x = i % imgWidth;
    int y = i / imgWidth;
    float imgW = volSize_imgWidth.w;
    float imgH = (get_global_size(0)/imgWidth);

    float4 coord = metricToNormal( pixelToVolume(x/imgW , y/imgH , areaRect, origin, xAxis, yAxis) , volSize_imgWidth );
    if(coord.x <= 0 || coord.y <= 0 || coord.z <= 0 || coord.x >= 1 || coord.y >= 1 || coord.z >= 1){
        image[i] = (float) NAN;
    } else {
        if(useInt>0){
                int4 value = read_imagei(volume, samplerNN, coord);
                image[i] = value.x;
        }else{
                float4 value = read_imagef(volume, samplerNN, coord);
                image[i] = value.x;
        }
    }
}


const sampler_t samplerLN = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

__kernel void extract_slice_linear(
            __read_only image3d_t volume,
            __global float* image,
            const float4 volSize_imgWidth,
            const uint stride,
            const float4 origin,
            const float4 xAxis,
            const float4 yAxis,
            const float4 areaRect,
            const uint useInt)
{
/* - - - - - - - - - - - - - - - - - */
    int i = get_global_id(0);
    int imgWidth = (int) volSize_imgWidth.w;
    int x = i % imgWidth;
    int y = i / imgWidth;
    float imgW = volSize_imgWidth.w;
    float imgH = (get_global_size(0)/imgWidth);

    float4 coord = metricToNormal( pixelToVolume(x/imgW , y/imgH , areaRect, origin, xAxis, yAxis) , volSize_imgWidth );
    if(coord.x <= 0 || coord.y <= 0 || coord.z <= 0 || coord.x >= 1 || coord.y >= 1 || coord.z >= 1){
        image[i] = (float) NAN;
    } else {
        if(useInt>0){
                int4 value = read_imagei(volume, samplerLN, coord);
                image[i] = value.x;
        }else{
                float4 value = read_imagef(volume, samplerLN, coord);
                image[i] = value.x;
        }
    }
}


// Local Variables:
// mode: c
// tab-width: 4
// c-basic-offset: 4
// End:
