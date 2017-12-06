#version 400

in vec3 vertTex;
uniform mat4 M, P, V;
out vec3 texcoords;

void main() {

  texcoords = vertTex;

  gl_Position = P * V * M * vec4(vertTex, 1.0);
}