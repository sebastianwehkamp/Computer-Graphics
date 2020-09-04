#version 410
// Vertex shader

layout (location = 0) in vec3 vertcoords_world_vs;
layout (location = 1) in vec3 in_colour;

uniform mat4 modelviewmatrix;
uniform mat4 projectionmatrix;

layout (location = 0) out vec3 vertcoords_camera_vs;
layout (location = 1) out vec3 out_colour;

void main() {
  gl_Position = projectionmatrix * modelviewmatrix * vec4(vertcoords_world_vs, 1.0);

  vertcoords_camera_vs = vec3(modelviewmatrix * vec4(vertcoords_world_vs, 1.0));
  out_colour = in_colour;
}
