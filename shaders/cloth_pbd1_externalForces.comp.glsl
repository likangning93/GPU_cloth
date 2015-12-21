#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_ACC 32

layout(std430, binding = 0) buffer _Vel {
    vec4 Vel[];
};

// acceleration due to gravity
layout(location = 0) uniform float DT;
layout(location = 1) uniform vec3 F;
layout(location = 2) uniform int numVertices;

layout(local_size_x = WORK_GROUP_SIZE_ACC, local_size_y = 1, local_size_z = 1) in;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= numVertices) return;

    vec4 vel0 = Vel[idx];
    vel0.xyz += F * DT;

    Vel[idx] = vel0;
}
