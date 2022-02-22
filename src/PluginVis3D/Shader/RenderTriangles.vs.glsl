#version 130

#define HAS_ATTRIBUTE_COLOR_V $hasVertexAttribute(de.uni_stuttgart.Voxie.SurfaceAttribute.Color)
#define HAS_ATTRIBUTE_COLORBACKSIDE_V $hasVertexAttribute(de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside)
#define HAS_ATTRIBUTE_COLOR_T $hasTriangleAttribute(de.uni_stuttgart.Voxie.SurfaceAttribute.Color)
#define HAS_ATTRIBUTE_COLORBACKSIDE_T $hasTriangleAttribute(de.uni_stuttgart.Voxie.SurfaceAttribute.ColorBackside)
#define MAX_CLIP_DISTANCES $maxClipDistances

/*
 * TODO: Implement more shading techniques, e.g. Gouraud and Phong shading.
 *
 * (The current smooth shading uses per-triangle normals, not per-vertex ones.)
 */
#if $useSmoothShading
#define INTERPOLATION smooth
#else
#define INTERPOLATION flat
#endif

// TODO: Update shader version to OpenGL Version

in vec3 vertexPosition_modelspace;
in vec3 vertexNormal;

uniform vec4 defaultFrontColor;
uniform vec4 defaultBackColor;

#if HAS_ATTRIBUTE_COLOR_V ||  HAS_ATTRIBUTE_COLOR_T
in vec4 de_uni_stuttgart_Voxie_SurfaceAttribute_Color;
#define color de_uni_stuttgart_Voxie_SurfaceAttribute_Color
#else
#define color defaultFrontColor
#endif

#if HAS_ATTRIBUTE_COLORBACKSIDE_V||  HAS_ATTRIBUTE_COLORBACKSIDE_T
in vec4 de_uni_stuttgart_Voxie_SurfaceAttribute_ColorBackside;
#define colorBackside de_uni_stuttgart_Voxie_SurfaceAttribute_ColorBackside
#else
#define colorBackside defaultBackColor
#endif

uniform mat4 MVP;
uniform mat4 M;
uniform int invertColor;
uniform int numClipDistances;
uniform int cuttingLimit;
uniform int cuttingMode;

uniform vec4 clippingPlanes[MAX_CLIP_DISTANCES];
uniform int clippingDirections[MAX_CLIP_DISTANCES];
const vec3 orangeColor = vec3(1, 1, 0);

flat out uint vertexID;
INTERPOLATION out vec4 fragmentColor;
INTERPOLATION out vec4 fragmentColorBack;
flat out vec3 fragmentNormal;
INTERPOLATION out vec3 fragmentPosition;
out float gl_ClipDistance[MAX_CLIP_DISTANCES];

void setClipDistances(vec4 vertexPosition);

void main() {
  vertexID = uint(gl_VertexID);
  vec4 vertPosition = vec4(vertexPosition_modelspace, 1);
  gl_Position = MVP * vertPosition;

  if (numClipDistances != 0) setClipDistances(vertPosition);

  if (invertColor == 1){
    fragmentColor = vec4(1 - color.x, 1 - color.y, 1 - color.z, color.w);
    fragmentColorBack = vec4(1 - colorBackside.x, 1 - colorBackside.y, 1 - colorBackside.z, colorBackside.w);
  }
  else{
    fragmentColor = color;
    fragmentColorBack = colorBackside;
  }

  fragmentNormal = (M * vec4(vertexNormal, 1)).xyz;
  fragmentPosition = (M * vec4(vertexPosition_modelspace, 1)).xyz;
}

void setClipDistances(vec4 vertexPosition) {
  // Counts the number of planes that cut the point away
  int cuttingCounter = 0;

  for (int i = 0; i < numClipDistances; i++) {
    gl_ClipDistance[i] =
        dot(vertexPosition, clippingPlanes[i]) * clippingDirections[i];
    if (gl_ClipDistance[i] < 0) cuttingCounter++;
  }

  bool clip = false;
  if (cuttingMode == 0) {
    // AtLeast mode cuts away if the minimum cutting limit is reached
    if (cuttingCounter >= cuttingLimit) clip = true;
  } else if (cuttingMode == 1 &&
             cuttingCounter == numClipDistances - cuttingLimit) {
    clip = true;
  }

  if (!clip) {
    // Flip the sign of all clipped distances. This leads to unsharp edges.
    // TODO: Assign distance to closest real cutting plane?
    for (int i = 0; i < numClipDistances; i++) {
      if (gl_ClipDistance[i] < 0) gl_ClipDistance[i] *= -1;
    }
  }
}
