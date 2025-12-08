#version 130

attribute vec3 vertexPosition_modelspace;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

uniform mat4 MVP;
uniform mat4 M;

// TODO: Does flat shading work with 1.10 shaders?

// varying vec4 fragmentColor;
// varying vec3 fragmentPosition;
// varying vec3 fragmentNormal;

flat out vec4 fragmentColor;
// TODO: Should this be flat or smooth? For light sources at infinity it should not matter.
flat out vec3 fragmentPosition;
flat out vec3 fragmentNormal;

void main() {
    gl_Position = MVP * vec4(vertexPosition_modelspace,1);

    fragmentColor = vertexColor;

    // TODO: Is this correct? Shouldn't this use ", 0" because the normal is a
    // direction vector?
    fragmentNormal = (M * vec4(vertexNormal, 1)).xyz;
    fragmentPosition = (M * vec4(vertexPosition_modelspace, 1)).xyz;
}
