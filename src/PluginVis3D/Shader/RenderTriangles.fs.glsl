#version 130

#if $useSmoothShading
#define INTERPOLATION smooth
#else
#define INTERPOLATION flat
#endif

INTERPOLATION in vec4 fragmentColor;
INTERPOLATION in vec4 fragmentColorBack;
INTERPOLATION in vec3 fragmentPosition;
flat in vec3 fragmentNormal;
flat in uint vertexID;

uniform vec4 lightPosition;
uniform int highlightedTriangle;

const mat4 thresholdMatrix = mat4(
  1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
  13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
  4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
  16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0
);

void main() {
  vec3 cameraFragDirection = vec3(
    lightPosition.x - fragmentPosition.x * lightPosition.w,
    lightPosition.y - fragmentPosition.y * lightPosition.w,
    lightPosition.z - fragmentPosition.z * lightPosition.w);

  float lighting = max(0.0, dot(fragmentNormal * (gl_FrontFacing ? 1 : -1),
                                normalize(cameraFragDirection)));
  
  vec4 color = gl_FrontFacing ? fragmentColor : fragmentColorBack;

  if(color.a - thresholdMatrix[int(gl_FragCoord.x) % 4][int(gl_FragCoord.y) % 4] < 0)
    discard;

  // TODO: Use gl_PrimitiveID when it is available (and use the unmodified
  // surface)
  if (uint(highlightedTriangle) == vertexID /*gl_PrimitiveID*/ + 1u)
    color = vec4(1 - color.x, 1 - color.y, 1 - color.z, color.w);

  vec3 diffuse = vec3(1 - (1 - lighting) * (0.25f + (0.25f * (1 - color.a))));
  color *= vec4(diffuse, 1.0);
    
  gl_FragColor = color;
}
