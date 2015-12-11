#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_VELPOS 32

layout(std430, binding = 0) readonly buffer _Vel {
    vec4 Vel[];
};
layout(std430, binding = 1) readonly buffer _Pos {
    vec4 Pos[];
};
layout(std430, binding = 2) buffer _pPos1 { // predicted position
    vec4 pPos1[];
};
layout(std430, binding = 3) buffer _pPos2 { // predicted position
    vec4 pPos2[];
};

layout(location = 0) uniform float DT;

layout(location = 1) uniform int numVertices;

layout(local_size_x = WORK_GROUP_SIZE_VELPOS, local_size_y = 1, local_size_z = 1) in;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= numVertices) return;
    vec4 vertexData = Pos[idx];
    vec3 prediction = vertexData.xyz + Vel[idx].xyz * DT;
    pPos1[idx] = vec4(prediction, vertexData.w);
    pPos2[idx] = vec4(prediction, vertexData.w);
}
