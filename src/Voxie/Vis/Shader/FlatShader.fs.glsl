#version 130

// varying vec4 fragmentColor;
// varying vec3 fragmentPosition;
// varying vec3 fragmentNormal;

flat in vec4 fragmentColor;
flat in vec3 fragmentPosition;
flat in vec3 fragmentNormal;

uniform vec4 lightPosition;

void main(){
  vec3 cameraFragDirection = vec3(
    lightPosition.x - fragmentPosition.x * lightPosition.w,
    lightPosition.y - fragmentPosition.y * lightPosition.w,
    lightPosition.z - fragmentPosition.z * lightPosition.w);
  float lighting = max(0.0, dot(fragmentNormal * (gl_FrontFacing ? 1.0 : -1.0),
                                normalize(cameraFragDirection)));

  // TODO: Allow specifying a different back color?
  // vec4 color = gl_FrontFacing ? fragmentColor : fragmentColorBack;
  // vec4 color = gl_FrontFacing ? fragmentColor : vec4(1, 0, 0, 1);
  //vec4 color = fragmentColor;
  vec4 color = gl_FrontFacing ? fragmentColor : vec4(0.9, 0.1, 0.1, 1.0);
  vec3 diffuse =
      vec3(1.0 - (1.0 - lighting) * (0.25 + (0.25 * (1.0 - color.a))));
  color *= vec4(diffuse, 1.0);

  gl_FragColor = color;
}
