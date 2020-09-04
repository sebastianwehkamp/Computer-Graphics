#version 410
layout (lines_adjacency) in;
layout (line_strip, max_vertices = 4) out;

layout (location = 1) out vec3 out_color;

float sq(float t){
  return t * t;
}

vec3 toColour(float col) {
    if (col < 0.33){
        return vec3(3 * col, 0, 1 - 3 * col);
    } else if (col < 0.66) {
        col -= 0.33;
        return vec3(1 - 3 * col, 3 * col, 0);
    } else {
        col -= 0.66;
        return vec3(0, 1 - 3 * col, 3 * col);
    }
}

vec3 calcColour(vec4 pos0, vec4 pos1, vec4 pos2){
    vec4 a = pos2-pos1;
    vec4 b = pos0-pos1;
    vec4 c = pos2-pos0;

     // Calc info required for curvature
     float alpha = dot(normalize(a), normalize(b));
     float angle = acos(alpha);
     //curvature
     float k = 2 * sin(angle) / sqrt(dot(c,c));

     float col = k / 8.0; // Apply scaling
     col = sqrt(col);

     // Convert to actual color
     return out_color = toColour(col);
}

void main() {
    out_color = calcColour(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position);

    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    out_color = calcColour(gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);

    gl_Position = gl_in[2].gl_Position;
    EmitVertex();
    EndPrimitive();

}
