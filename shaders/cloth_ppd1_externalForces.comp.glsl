#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_ACC 16

#define DT 0.2

// acceleration due to gravity
const float G = -0.0098;

layout(std430, binding = 0) buffer _Vel {
    vec4 Vel[];
};

layout(local_size_x = WORK_GROUP_SIZE_ACC, local_size_y = 1, local_size_z = 1) in;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    vec4 vel0 = Vel[idx];

    Vel[idx] = vel0 + DT * vec4(0.0, 0.0, G, 0.0);
}
