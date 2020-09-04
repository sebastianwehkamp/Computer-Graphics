#version 410
// Tessellation control shader. Only sets the tessellation levels
layout (vertices = 16) out;

layout (location = 0) in vec3 vertcoords_camera_vs[];

layout (location = 0) out vec3 vertcoords_camera_te[];

void main()
{
  vertcoords_camera_te[gl_InvocationID] = vertcoords_camera_vs[gl_InvocationID];

  if (gl_InvocationID == 0)
  {
      gl_TessLevelInner[0] = 4.0;
      gl_TessLevelInner[1] = 4.0;

      gl_TessLevelOuter[0] = 4.0;
      gl_TessLevelOuter[1] = 4.0;
      gl_TessLevelOuter[2] = 4.0;
      gl_TessLevelOuter[3] = 4.0;
    }
}
