#version 430

layout(std430, binding = 0) buffer _Pos {
    vec3 Pos[];
};

out vec4 worldPos0;

void main() {
	vec3 position = Pos[gl_VertexID];
    gl_Position = vec4(position, 1.0);
    worldPos0 = vec4(position, 1.0);
}
