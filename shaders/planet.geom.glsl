#version 430 core

uniform mat4 u_projMatrix;

layout(points) in;
layout(points) out;
layout(max_vertices = 1) out;



void main() {
    vec3 Position = gl_in[0].gl_Position.xyz / 1e2;
    gl_Position = u_projMatrix * vec4(Position, 1.0);
    EmitVertex();
    EndPrimitive();
}
