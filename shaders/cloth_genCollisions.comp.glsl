#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// TODO: change work group size here and in nbody.cpp
#define WORK_GROUP_SIZE_VELPOS 16

layout(std430, binding = 0) readonly buffer _pCloth1 { // cloth positions in previous timestep
    vec4 pCloth1[];
};
layout(std430, binding = 1) readonly buffer _pCloth2 { // cloth positions in new timestep
    vec4 pCloth2[];
};
layout(std430, binding = 2) readonly buffer _bodyPositions { // influencee
    vec4 pBody[];
};
layout(std430, binding = 3) readonly buffer _bodyTriangles {
    vec4 bodyTriangles[];
};
layout(std430, binding = 4) buffer _collisionConstraints { // vec4s of dir and distance 
    vec4 pClothCollisionConstraints[];
};

layout(local_size_x = WORK_GROUP_SIZE_VELPOS, local_size_y = 1, local_size_z = 1) in;

layout(location = 0) uniform int numTriangles;
layout(location = 1) uniform int numPositions;

// adapted from http://undernones.blogspot.com/2010/12/gpu-ray-tracing-with-glsl.html
float intersectTriangle(vec3 orig, vec3 lookat, vec3 v0, vec3 v1, vec3 v2)
{
    vec3 u, v, n; // triangle vectors
    vec3 w0, w;  // ray float
    float r, a, b; // params to calc ray-plane intersect

    vec3 dir = normalize(lookat - orig);

    // get triangle edge vectors and plane normal
    u = v1 - v0;
    v = v2 - v0;
    n = cross(u, v);

    w0 = orig - v0;
    a = -dot(n, w0);
    b = dot(n, dir);
    if (abs(b) < 1e-5)
    {
        // ray is parallel to triangle plane, and thus can never intersect.
        return -1.0;
    }

    // get intersect point of ray with triangle plane
    r = a / b;
    if (r < 0.0)
        return -1.0; // ray goes away from triangle.

    vec3 I = orig + r * dir;
    float uu, uv, vv, wu, wv, D;
    uu = dot(u, u);
    uv = dot(u, v);
    vv = dot(v, v);
    w = I - v0;
    wu = dot(w, u);
    wv = dot(w, v);
    D = uv * uv - uu * vv;

    // get and test parametric coords
    float s, t;
    s = (uv * wv - vv * wu) / D;
    if (s < 0.0 || s > 1.0)
        return -1.0;
    t = (uv * wu - uu * wv) / D;
    if (t < 0.0 || (s + t) > 1.0)
        return -1.0;

    return (r > 1e-5) ? r : -1.0;
}

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= numPositions) return;

    vec3 pos = pCloth1[idx].xyz;
	vec3 lookAt = pCloth2[idx].xyz;

    vec4 collisionConstraint = vec4(0.0, 0.0, 0.0, -1.0); // a bogus collisionConstraint

    // check against every triangle in the mesh. use the nearest collision
    for (int i = 0; i < numTriangles; i++) {

    	vec3 triangle = bodyTriangles[i].xyz;
    	vec3 v0 = pBody[int(triangle.x)].xyz;
    	vec3 v1 = pBody[int(triangle.y)].xyz;
    	vec3 v2 = pBody[int(triangle.z)].xyz;

    	float collisionT = intersectTriangle(pos, lookAt, v0, v1, v2);
    	vec3 norm = cross(v2 - v0, v1 - v0);
    	if (collisionConstraint.w < 0.0 && collisionT >= 0.0) { // first contact
    		collisionConstraint = vec4(norm, collisionT);
    	}
    	else if (collisionT >= 0.0 && collisionT < collisionConstraint.w) {
    		collisionConstraint = vec4(norm, collisionT);
    	}
    }
    pClothCollisionConstraints[idx] = collisionConstraint;
}
