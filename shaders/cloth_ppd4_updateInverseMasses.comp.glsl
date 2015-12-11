#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_VELPOS 32

layout(std430, binding = 0) buffer _pPos1 { // predicted position
    vec4 pPos1[];
};
layout(std430, binding = 1) buffer _pPos2 { // predicted position
    vec4 pPos2[];
};
layout(std430, binding = 2) readonly buffer _Constraints {
    vec4 constraints[];
};

layout(location = 0) uniform int numConstraints;

layout(local_size_x = WORK_GROUP_SIZE_VELPOS, local_size_y = 1, local_size_z = 1) in;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= numConstraints) return;
    // compute force contribution from this constraint
    vec4 constraint = constraints[idx];    
    highp int targetIdx = int(constraint.x); // index of "target" -> the position to be modified

    if (constraint.z < 0.0) { // this position will be pinned
        pPos1[targetIdx].w = 0.0;
        pPos2[targetIdx].w = 0.0;
    }
}
