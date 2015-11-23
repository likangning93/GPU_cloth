#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_ACC 16

// Universal gravitation constant.
const float G = 6.67384e-11;
// Mass of the "star" at the center.
const float starMass = 5e10;
// Mass of one "planet."
const float planetMass = 3e8;

layout(location = 0) uniform int numPlanets;

layout(std430, binding = 0) readonly buffer _Pos {
    vec3 Pos[];
};
layout(std430, binding = 1) writeonly buffer _Acc {
    vec3 Acc[];
};

layout(local_size_x = WORK_GROUP_SIZE_ACC, local_size_y = 1, local_size_z = 1) in;

void main() {
    // gl_GlobalInvocationID is equal to:
    //     gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID.
    uint idx = gl_GlobalInvocationID.x;

    // star contribution
    vec3 p = Pos[idx];
    float dist = length(p);
    vec3 acc = -(G * starMass * p) / (dist * dist * dist + 0.0001); // gotta jitter to avoid NaN

    // planet-planet contributions
    for (int i = 0; i < numPlanets; i++) {
        vec3 v = p - Pos[i];
        float d = length(v);
        acc -= (G * planetMass * v) / (d * d * d + 0.0001); // gotta jitter to avoid NaN
    }
    Acc[idx] = acc;
}
