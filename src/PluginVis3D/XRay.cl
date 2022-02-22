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


// float4 read_imagef (image3d_t image, sampler_t sampler, float4 coord);

__constant sampler_t sampler3D =
    CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

typedef struct {
  float3 position;
  float3 front, up, right;
} Camera;

typedef struct {
  float3 origin;
  float3 direction;
} Ray;

typedef struct {
  float4 position;
  float4 colorF;
} lightSource;

int intersectBox(Ray ray, float3 boxmin, float3 boxmax, float* tnear,
                 float* tfar);

__kernel void render( /*0*/ __write_only image2d_t resultImage,
                      /*1*/ __read_only image3d_t sourceImage,
                      /*2*/ float3 spacing,
                      /*3*/ float16 invViewProjection,
                      /*4*/ float2 voxelRange,
                      /*5*/ int numSamples,
                      /*6*/ float raytraceScale,
                      /*7*/ int useInt,
                      /*8*/ int useAntiAliazing,
                      /*9*/ float3 firstVoxelPosition,
                      /*10*/ float3 dimensionsMetric,
                      /*11*/ float3 ambientColor,
                      /*12*/ float ambientScale,
                      /*13*/ float diffuseScale,
                      /*14*/ __constant const lightSource* lightSourceList,
                      /*15*/ int lightSourceListLength,
                      /*16*/ int useAbsDiffuseShadingValue,
                      /*17*/ __global const float* randomValues,
                      /*18*/ int imageWidth ) {

  int x = get_global_id(0);  // current x-value
  int y = get_global_id(1);  // current y-value

  // get_global_size(0) = max. value in x-direction (image width)
  // get_global_size(1) = max. value in y-direction (image height)

  float fx = ((float)x + 0.5f) / get_global_size(0) * 2 - 1;
  float fy = 1 - ((float)y + 0.5f) / get_global_size(1) * 2;

  float4 temp0 = {fx, fy, 0, 1};
  float4 temp1 = {fx, fy, 1, 1};
  float4 Mtemp0, Mtemp1;

  Mtemp0.x = dot(temp0, (invViewProjection.s0123));
  Mtemp0.y = dot(temp0, (invViewProjection.s4567));
  Mtemp0.z = dot(temp0, (invViewProjection.s89ab));
  Mtemp0.w = dot(temp0, (invViewProjection.scdef));

  Mtemp1.x = dot(temp1, (invViewProjection.s0123));
  Mtemp1.y = dot(temp1, (invViewProjection.s4567));
  Mtemp1.z = dot(temp1, (invViewProjection.s89ab));
  Mtemp1.w = dot(temp1, (invViewProjection.scdef));

  Ray ray;
  ray.origin = invViewProjection.s26a / invViewProjection.se;
  ray.direction =
      normalize((float3){Mtemp0.w * Mtemp1.x - Mtemp1.w * Mtemp0.x,
                         Mtemp0.w * Mtemp1.y - Mtemp1.w * Mtemp0.y,
                         Mtemp0.w * Mtemp1.z - Mtemp1.w * Mtemp0.z});

  float tnear, tfar;
  int rayresult =
      intersectBox(ray, firstVoxelPosition,
                   firstVoxelPosition + dimensionsMetric, &tnear, &tfar);

  if (rayresult == 0) {
    // Didn't hit anything, return
    write_imagef(resultImage, (int2)(x, y), (float4)(0.0f, 0.0f, 0.0f, 1.0f));
    return;
  }

  // Clamp to camera
  if (tnear < 0.0f) {
    tnear = 0.0f;
  }
  if (tfar < tnear) {
    // We hit the box completly behind us, return red for debug reasons
    write_imagef(resultImage, (int2)(x, y), (float4)(1.0f, 0.0f, 0.0f, 1.0f));
    return;
  }

  float accu = 0.0f;
  float randomNumber = 0.0f;

  float3 diffuseResultColor = (float3)(0, 0, 0);
  float3 ambientResultColor = (float3)(0, 0, 0);

  for (int i = 0; i < numSamples; i++) {
    float iRandom = (float)i;

    if (useAntiAliazing != 0) {
    iRandom += randomValues[y*imageWidth+x]; // [0][1] -> [0][1][2][3]
                                             // [2][3];
    } else {
          iRandom += 0.5f;
    }

    float delta = iRandom / (float)(numSamples);

    // conversion from meter coordinate system to pixel coordinate system
    float3 position =
        ray.origin + (tnear + delta * (tfar - tnear)) * ray.direction;
    position -= firstVoxelPosition;
    position /= spacing;

    if (useInt != 0) {
      int voxel =
          read_imagei(sourceImage, sampler3D,
                      (float4)(position.x, position.y, position.z, 0.0f))
              .x;

      if(isnan( (float)voxel))
        voxel = 0;
      accu += clamp((voxel - voxelRange.x) / (voxelRange.y - voxelRange.x),
                    0.0f, 1.0f);
    } else {
      float voxel =
          read_imagef(sourceImage, sampler3D,
                      (float4)(position.x, position.y, position.z, 0.0f))
              .x;

      if(isnan( (float)voxel))
        voxel = 0.0f;
      accu += clamp((voxel - voxelRange.x) / (voxelRange.y - voxelRange.x),
                    0.0f, 1.0f);
    }

    // #### Shading ####

    float x1 = read_imagef(sourceImage, sampler3D,
    (float4)(position.x + 1, position.y,
      position.z, 0.0f)).x;

    float x2 =
    read_imagef(sourceImage, sampler3D,
      (float4)(position.x - 1, position.y,
      position.z, 0.0f)).x;

    float x_Gradient = (x1 - x2) / 2.0f;

    float y1 =
    read_imagef(sourceImage, sampler3D,
      (float4)(position.x, position.y + 1,
      position.z, 0.0f)).x;

    float y2 =
    read_imagef(sourceImage, sampler3D,
      (float4)(position.x, position.y - 1,
      position.z, 0.0f)).x;

    float y_Gradient = (y1 - y2) / 2.0f;

    float z1 =
    read_imagef(sourceImage, sampler3D,
      (float4)(position.x, position.y,
      position.z + 1, 0.0f)).x;

    float z2 =
    read_imagef(sourceImage, sampler3D,
      (float4)(position.x, position.y,
      position.z - 1, 0.0f)).x;

    float z_Gradient = (z1 - z2) / 2.0f;

    float3 normalVec = (float3)(x_Gradient, y_Gradient, z_Gradient);

    //### Ambient-Shading ###
    float normalVecLength = length(normalVec);
    if (normalVecLength > 0) {
      ambientResultColor += ambientColor * normalVecLength;
    }

    // ### Diffuse-Shading ###
    if (lightSourceList) {
      for (int i = 0; i < lightSourceListLength; i++) {
        float3 lightVec = (float3)(0,0,0);

        float4 lightPos = lightSourceList[i].position;

        float w2 = 1.0f;  // since position is only a float3, its w = 1.
        lightVec =  normalize((float3)
            (lightPos.x * w2 - position.x * lightPos.w,
            lightPos.y * w2 - position.y * lightPos.w,
            lightPos.z * w2 - position.z * lightPos.w));

        float shadingValue = dot(lightVec, normalVec);

        if (useAbsDiffuseShadingValue) {
          shadingValue = fabs(shadingValue);
        } else {
          if (shadingValue <= 0) {
            continue;
          }
        }

        float3 lightSourceColor = (float3)(lightSourceList[i].colorF.x, lightSourceList[i].colorF.y, lightSourceList[i].colorF.z);
        diffuseResultColor += lightSourceColor * shadingValue;

      }  // lightSourceList for-loop END
    }
  }  // samples for-loop END


  //Scaling Constants
  const float ambientLightFactor = 1.0f / 11.85f;
  const float diffuseLightFactor = 1.0f / 5.0f;
  const float grayValueFactor = 0.25f;

  // Scale accumulator
  accu *= (raytraceScale * grayValueFactor) / (tfar -tnear)/numSamples;

  float3 grayValue = (float3)(accu, accu, accu);

  // Scale Colors
  ambientResultColor *= (ambientScale * ambientLightFactor) /(tfar -tnear)/numSamples;
  diffuseResultColor *= (diffuseScale * diffuseLightFactor) / (tfar -tnear)/numSamples;

  float3 resultColorF = (float3)(0.0f, 0.0f, 0.0f);

  resultColorF = grayValue + ambientResultColor + diffuseResultColor;

  clamp(resultColorF, 0.0f, 1.0f);

  float4 result = (float4)(resultColorF, 1.0f);

  write_imagef(resultImage, (int2)(x, y), result);
}

int intersectBox(Ray ray, float3 boxmin, float3 boxmax, float* tnear,
                 float* tfar) {
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
