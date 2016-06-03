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
            const float4 areaRect) 
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
        float4 value = read_imagef(volume, samplerNN, coord);
        image[i] = value.x;
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
            const float4 areaRect) 
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
        float4 value = read_imagef(volume, samplerLN, coord);
        image[i] = value.x;
    }
}


// Local Variables:
// mode: c
// tab-width: 4
// c-basic-offset: 4
// End:
