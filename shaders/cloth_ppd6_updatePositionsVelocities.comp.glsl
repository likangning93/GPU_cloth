#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_ACC 16

#define DT 0.02

// spring constant

layout(std430, binding = 0) buffer _Vel { // velocities
    vec4 Vel[];
};
layout(std430, binding = 1) buffer _Pos { // position in the last timestep
    vec4 Pos[];
};
layout(std430, binding = 2) readonly buffer _pPos { // corrected predicted positions
    vec4 pPos[];
};

layout(local_size_x = WORK_GROUP_SIZE_ACC, local_size_y = 1, local_size_z = 1) in;

void main() {
    // gl_GlobalInvocationID is equal to:
    //     gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID.
    uint idx = gl_GlobalInvocationID.x;

    Vel[idx] = (pPos[idx] - Pos[idx]) / DT;
    Pos[idx] = pPos[idx];
}
