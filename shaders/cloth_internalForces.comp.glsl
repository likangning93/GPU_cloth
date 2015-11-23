#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_ACC 16

// spring constant
const float K = 0.1;

layout(std430, binding = 0) readonly buffer _Pos {
    vec3 Pos[];
};
layout(std430, binding = 1) buffer _Force {
    vec3 Force[];
};
layout(std430, binding = 2) buffer _Constraints {
    vec3 Constraints[];
};

layout(local_size_x = WORK_GROUP_SIZE_ACC, local_size_y = 1, local_size_z = 1) in;

void main() {
    // gl_GlobalInvocationID is equal to:
    //     gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID.
    uint idx = gl_GlobalInvocationID.x;

    // compute force contribution from this constraint
    vec3 constraint = Constraints[idx];
    
    uint pointIdx = int(constraint.x);

    vec3 diff = Pos[int(constraint.y)] - Pos[pointIdx];
    float dist = length(diff);
    if (dist < 0.0001) return; // avoid singularities
    vec3 forceContrib = K * abs(dist - constraint.z) * (diff / dist);

    // TODO: atomics?
    memoryBarrierBuffer();
    Force[pointIdx] += forceContrib;
}
