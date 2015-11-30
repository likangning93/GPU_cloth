#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_ACC 16

// spring constant
const float K = 0.9;

const float N = 4.0; // number of times to project

layout(std430, binding = 0) readonly buffer _Pos {
    vec4 Pos[];
};
layout(std430, binding = 1) readonly buffer _pPos1 { // source
    vec4 pPos1[];
};
layout(std430, binding = 2) buffer _pPos2 { // destination
    vec4 pPos2[];
};
layout(std430, binding = 3) buffer _Constraints {
    vec3 Constraints[];
};

layout(local_size_x = WORK_GROUP_SIZE_ACC, local_size_y = 1, local_size_z = 1) in;

void main() {
    // gl_GlobalInvocationID is equal to:
    //     gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID.
    uint idx = gl_GlobalInvocationID.x;

    // compute force contribution from this constraint
    vec3 constraint = Constraints[idx];
    
    int targetIdx = int(constraint.x); // index of "target" -> the position to be modified
    int influenceIdx = int(constraint.y); // index of "influencer" -> the particle doing the pulling

    if (targetIdx == -1 || influenceIdx == -1) { // bogus influence
        return;
    }

    // "prefetch?"
    vec3 targPos = pPos1[targetIdx].xyz;
    vec3 influencePos = pPos1[influenceIdx].xyz;

    if (targetIdx == influenceIdx) { // case of a pin
        pPos2[targetIdx] = Pos[targetIdx];
        return;
    }

    vec3 diff = influencePos - targPos;
    float dist = length(diff);
    float w = 0.5f; // it's w1 / (w1 + w2), but w1 == w2 right?

    vec3 dp1 = w * (dist - constraint.z) * diff / dist; // force is towards influencer

    float k_prime = 1.0 - pow(1.0 - K, 1.0 / N);

    pPos2[targetIdx] += vec4(k_prime * dp1, 1.0);

}
