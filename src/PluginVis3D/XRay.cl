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
    float16 invViewProjection,
    float2 voxelRange,
    int numSamples,
    float scale)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    float fx = ((float) x + 0.5f) / get_global_size(0) * 2 - 1;
    float fy = 1 - ((float) y + 0.5f) / get_global_size(1) * 2;

    Ray ray;
    // TODO: handle orthogonal projection?
    // ray.origin = toVector3 (invViewProjection * (0, 0, -1, 0))
    ray.origin = invViewProjection.s26a / invViewProjection.se;
    //float4 temp = { fx, fy, 0, 1 };
    float4 temp0 = { fx, fy, 0, 1 };
    float4 temp1 = { fx, fy, 1, 1 };
    float4 Mtemp0, Mtemp1;
    Mtemp0.x = dot(temp0, (invViewProjection.s0123));
    Mtemp0.y = dot(temp0, (invViewProjection.s4567));
    Mtemp0.z = dot(temp0, (invViewProjection.s89ab));
    Mtemp0.w = dot(temp0, (invViewProjection.scdef));
    Mtemp1.x = dot(temp1, (invViewProjection.s0123));
    Mtemp1.y = dot(temp1, (invViewProjection.s4567));
    Mtemp1.z = dot(temp1, (invViewProjection.s89ab));
    Mtemp1.w = dot(temp1, (invViewProjection.scdef));
    ray.direction = normalize ((float3) {Mtemp0.w * Mtemp1.x - Mtemp1.w * Mtemp0.x, Mtemp0.w * Mtemp1.y - Mtemp1.w * Mtemp0.y, Mtemp0.w * Mtemp1.z - Mtemp1.w * Mtemp0.z});


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
