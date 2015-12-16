#version 430 core

uniform mat4 u_projMatrix;

layout(lines) in;
layout(line_strip, max_vertices=2) out;

in vec3 worldPos0[2];

// http://stackoverflow.com/questions/19346019/calculate-normals-geometry-shader

void main() {
	for(int i = 0; i < gl_in.length(); i++)
        {
            vec3 Position = gl_in[i].gl_Position.xyz;
   			gl_Position = u_projMatrix * vec4(Position, 1.0);
            EmitVertex();
        }
}
