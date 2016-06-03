// float4 read_imagef (image3d_t image, sampler_t sampler, float4 coord);

__constant sampler_t sampler3D = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

typedef struct
{
    float3 position;
    float3 front, up, right;
} Camera;

typedef struct
{
    float3 origin;
    float3 direction;
} Ray;

int intersectBox(Ray ray, float3 boxmin, float3 boxmax, float *tnear, float *tfar);

__kernel void render(
    __write_only image2d_t resultImage,
    __read_only image3d_t sourceImage,
    float3 coordinateTransform,
    float3 cameraPosition,
    float2 voxelRange,
    int numSamples,
    float scale)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    float2 size = (float2)((float)get_global_size(0), (float)get_global_size(1));

    float fx = (float)(x) / size.x;
    float fy = (float)(y) / size.y;
    float aspect = size.x / size.y;

    Camera camera;
    camera.position = cameraPosition;
    camera.front = -normalize(camera.position);
    camera.right = normalize(cross(camera.front, (float3)(0,1,0)));
    camera.up = normalize(cross(camera.front, camera.right));

    Ray ray;
    ray.origin = camera.position;
    ray.direction = normalize(
        1.5f * camera.front +
        aspect * 2.0f * camera.right * (fx - 0.5f) +
        2.0f * camera.up * (fy - 0.5f));


    float tnear, tfar;
    int rayresult = intersectBox(ray, (float3)(-0.5f, -0.5f, -0.5f), (float3)(0.5f, 0.5f, 0.5f), &tnear, &tfar);

    if(rayresult == 0)
    {
        // Didn't hit anything, return
        write_imagef(resultImage, (int2)(x, y), (float4)(0.0f, 0.0f, 0.0f, 0.0f));
        return;
    }

    // Clamp to camera
    if(tnear < 0.0f)
    {
        tnear = 0.0f;
    }
    if(tfar < tnear)
    {
        // We hit the box completly behind us, return red for debug reasons
        write_imagef(resultImage, (int2)(x, y), (float4)(1.0f, 0.0f, 0.0f, 1.0f));
        return;
    }

    float accu = 0.0f;

    for(int i = 0; i < numSamples; i++)
    {
        float delta = (float)i / (float)(numSamples - 1);
        float3 position = ray.origin + (tnear + delta * (tfar - tnear)) * ray.direction;
        position *= coordinateTransform;
        position += 0.5f; // Transform into data space

        float voxel = read_imagef(sourceImage, sampler3D, (float4)(position.x, position.y, position.z, 0.0f)).x;

        accu += clamp((voxel - voxelRange.x) / (voxelRange.y - voxelRange.x), 0.0f, 1.0f);
    }
    // Normalize accumulator
    accu *= (scale / (float)numSamples);

    float4 result = (float4)(accu, accu, accu, accu);

    write_imagef(resultImage, (int2)(x, y), result);
}




int intersectBox(Ray ray, float3 boxmin, float3 boxmax, float *tnear, float *tfar)
{
    // compute intersection of ray with all six bbox planes
    float3 invR = (float3)(1.0f, 1.0f, 1.0f) / ray.direction;
    float3 tbot = invR * (boxmin - ray.origin);
    float3 ttop = invR * (boxmax - ray.origin);

    // re-order intersections to find smallest and largest on each axis
    float3 tmin = min(ttop, tbot);
    float3 tmax = max(ttop, tbot);

    // find the largest tmin and the smallest tmax
    float largest_tmin = max(max(tmin.x, tmin.y), tmin.z);
    float smallest_tmax = min(min(tmax.x, tmax.y), tmax.z);

    *tnear = largest_tmin;
    *tfar = smallest_tmax;

    return smallest_tmax > largest_tmin;
}

// Local Variables:
// mode: c
// tab-width: 4
// c-basic-offset: 4
// End:
