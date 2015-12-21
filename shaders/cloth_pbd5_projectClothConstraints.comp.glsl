// cloth constraints are vec4s as follows:
// x: index of position to be projected onto
// y: index of position influencing x
// z: rest length. a rest length of -1 indicates a pin constraint.
// w: SSBO_ID of influencer


#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_ACC 32

layout(std430, binding = 0) readonly buffer _influencerPos { // influencer
    vec4 pInfluencer[];
};
layout(std430, binding = 1) buffer _modifyPos { // influencee
    vec4 pModify[];
};
layout(std430, binding = 2) readonly buffer _Constraints {
    vec4 Constraints[];
};

// spring constant
layout(location = 0) uniform float N; // number of times to project

layout(location = 1) uniform int numConstraints;

layout(location = 2) uniform float K; // PBD spring constant

layout(location = 3) uniform int SSBO_ID; // the ID of the SSBO providing pModify

layout(local_size_x = WORK_GROUP_SIZE_ACC, local_size_y = 1, local_size_z = 1) in;

void main() {
    // gl_GlobalInvocationID is equal to:
    //     gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID.
    uint idx = gl_GlobalInvocationID.x;

    if (idx >= numConstraints) return;

    // compute force contribution from this constraint
    vec4 constraint = Constraints[idx];
    
    highp int targetIdx = int(constraint.x); // index of "target" -> the position to be modified
    highp int influenceIdx = int(constraint.y); // index of "influencer" -> the particle doing the pulling
    highp int constraintSSBO = int(constraint.w); // ID of the ssbo this constraint is intended for
    if (constraintSSBO != SSBO_ID) return; // do not evaluate, this is not the right SSBO

    if (constraint.x < 0.0 || constraint.y < 0.0) { // bogus influence
        return;
    }

    // "prefetch?"
    vec4 target = pModify[targetIdx];
    float k_prime = 1.0 - pow(1.0 - K, 1.0 / N);

    vec4 influencer = pInfluencer[influenceIdx];

    if (constraint.z < 0.0) { // case of a stationary pin
        pModify[targetIdx].xyz = influencer.xyz;
        return;
    }

    vec3 diff = influencer.xyz - target.xyz;
    float dist = length(diff);
    float w = target.w / (influencer.w + target.w);

    vec3 dp1 = w * (dist - constraint.z) * diff / dist; // force is towards influencer

    pModify[targetIdx].xyz += k_prime * dp1;

}
