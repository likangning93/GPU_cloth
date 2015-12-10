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
layout(std430, binding = 2) readonly buffer _pCollisionConstraints { // vec4s of dir and distance 
    vec4 pClothCollisionConstraints[];
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

	//pCloth2[idx].xyz = isx; // test
	//pCloth2[idx].w = -10.0;

	// muller: C = (p - q) dot n
	// isx = q
	// posNewTimestep = p
	// constraint.xyz = n
	// also from muller: dP = s * wi * dC
	// s = C / (dC ^ 2) for all P involved in C

	// idea is: we have an intersection point and a curr point
	// these form a vector going into the surface
	// we get a resolved curr point by projecting back along the surface normal
	// unsigned scalar of this projection is none other than (p - q) dot n.norm
	// we're assuming n is already normalized

	vec3 correction = (dot(isx - posNewTimestep, constraint.xyz) + 0.001 ) * constraint.xyz;
	//pCloth2[idx].xyz += correction;
}