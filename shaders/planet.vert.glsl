#version 430

layout(std430, binding = 0) buffer _Pos {
    vec3 Pos[];
};

void main() {
    gl_Position = vec4(Pos[gl_VertexID], 1.0);
}
