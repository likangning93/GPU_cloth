#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_VELPOS 16

layout(std430, binding = 0) readonly buffer _pCloth1 { // cloth positions in previous timestep
    vec4 pCloth1[];
};
layout(std430, binding = 1) buffer _pCloth2 { // cloth positions in new timestep. correct these.
    vec4 pCloth2[];
};
layout(std430, binding = 2) buffer _collisionConstraints { // vec4s of dir and distance 
    vec4 pClothCollisionConstraints[];
};
layout(std430, binding = 3) buffer _velocities { // vec4s of dir and distance 
    vec4 velocities[];
};

layout(local_size_x = WORK_GROUP_SIZE_VELPOS, local_size_y = 1, local_size_z = 1) in;

layout(location = 0) uniform int numPositions;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= numPositions) return;

	vec4 constraint = pClothCollisionConstraints[idx];
	if (constraint.w < 0.0) { // no correction
		return;
	}

    vec3 posOldTimestep = pCloth1[idx].xyz;
	vec3 posNewTimestep = pCloth2[idx].xyz;
	vec3 dir = posNewTimestep - posOldTimestep;
	vec3 isx = dir * constraint.w + posOldTimestep;
	float C = dot((posNewTimestep - isx), constraint.xyz);

	pCloth2[idx].xyz -= C * (-dir) / length(dir);
	//pCloth2[idx].xyz = posOldTimestep - dir * constraint.w;
	velocities[idx].xyz = vec3(0.0);
	pClothCollisionConstraints[idx] = vec4(-1.0, -1.0, -1.0, -1.0); // reset constraint
}