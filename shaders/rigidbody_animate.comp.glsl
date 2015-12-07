#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_VELPOS 16

layout(std430, binding = 0) readonly buffer _initPos { // initial positions
    vec4 initPos[];
};
layout(std430, binding = 1) buffer _animPos { // transformed position
    vec4 animPos[];
};

// TODO: figure out how to pass in matrices and weights

layout(local_size_x = WORK_GROUP_SIZE_VELPOS, local_size_y = 1, local_size_z = 1) in;

void main() {
    uint idx = gl_GlobalInvocationID.x;
}
