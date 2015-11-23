#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_VELPOS 16

#define DT 0.2

layout(std430, binding = 0) readonly buffer _Force {
    vec3 Force[];
};
layout(std430, binding = 1) buffer _Pos {
    vec3 Pos[];
};
layout(std430, binding = 2) buffer _Vel {
    vec3 Vel[];
};

layout(local_size_x = WORK_GROUP_SIZE_VELPOS, local_size_y = 1, local_size_z = 1) in;

void main() {
    // gl_GlobalInvocationID is equal to:
    //     gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID.
    uint idx = gl_GlobalInvocationID.x;

    vec3 p = Pos[idx];
    vec3 v = Vel[idx];
    vec3 f = Force[idx];

    // explicit symplectic integration:
    // update the velocity and compute position with updated velocity
    v += DT * f;
    p += DT * v * 0.01; // damping factor
    Vel[idx] = v;
    Pos[idx] = p;
}
