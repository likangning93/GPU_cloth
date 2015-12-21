#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_ACC 32

layout(std430, binding = 0) buffer _Vel { // velocities
    vec4 Vel[];
};
layout(std430, binding = 1) buffer _Pos { // position in the last timestep
    vec4 Pos[];
};
layout(std430, binding = 2) readonly buffer _pPos { // corrected predicted positions
    vec4 pPos[];
};
layout(std430, binding = 3) buffer _colConstraints { // collision constraints from the last timestep
    vec4 colConstraints[]; // at most one per vertex, so parallelizing here by vertex is fine
};

layout(location = 0) uniform float DT;
layout(location = 1) uniform int numVertices;

layout(local_size_x = WORK_GROUP_SIZE_ACC, local_size_y = 1, local_size_z = 1) in;

void main() {
    // gl_GlobalInvocationID is equal to:
    //     gl_WorkGroupID * gl_WorkGroupSize + gl_LocalInvocationID.
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= numVertices) return;

    vec3 predictedPosition = pPos[idx].xyz;
    vec3 predictedVelocity = (predictedPosition - Pos[idx].xyz) / DT;

    // if there was a collision constraint, bounce baby bounce
    vec4 constraint = colConstraints[idx];
    if (constraint.w >= 0.0) {
        //predictedVelocity = vec3(0.0, 0.0, 0.1);
        // from wolfram: reflecting is v' = v - 2 * dot(v, n) * n
        predictedVelocity -= 2 * dot(predictedVelocity, constraint.xyz) * constraint.xyz;
    }

    Vel[idx].xyz = predictedVelocity;
    Pos[idx].xyz = predictedPosition;
    colConstraints[idx] = vec4(-1.0);
}
