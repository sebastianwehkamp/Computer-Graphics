#version 410
// Fragment shader

layout (location = 0) in vec3 vertcoords_camera_fs;
layout (location = 1) in vec3 vertnormal_camera_fs;

uniform float reflectiondensity;

out vec4 fColor;

void main() {

  // Create normal
  vec3 normal = normalize(vertnormal_camera_fs);

  vec3 v = vec3(1.0,0.0,0.0);
  float angle = dot(normal, normalize(vertcoords_camera_fs));
  float theta = mod((angle*180),(reflectiondensity*reflectiondensity));

  if (theta <= reflectiondensity){
      fColor = vec4(0.0,0.0,0.0,1.0);
  } else {
      fColor = vec4(1.0,1.0,1.0,1.0);
  }
}
