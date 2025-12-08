#version 110

attribute vec3 vertexPosition_modelspace;
attribute vec4 vertexColor;

varying vec4 fragmentColor;

uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(vertexPosition_modelspace,1);
    fragmentColor = vertexColor;
}
