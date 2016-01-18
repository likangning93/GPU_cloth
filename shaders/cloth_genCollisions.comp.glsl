#version 430 core
#extension GL_ARB_compute_shader: enable
#extension GL_ARB_shader_storage_buffer_object: enable

// work group size injected before compilation
#define WORK_GROUP_SIZE XX
#define EPSILON 0.0001

layout(std430, binding = 0) buffer _pCloth1 { // cloth positions in previous timestep
    vec4 pCloth1[];
};
layout(std430, binding = 1) buffer _pCloth2 { // cloth positions in new timestep
    vec4 pCloth2[];
};
layout(std430, binding = 2) readonly buffer _bodyPositions { // influencee "rigidbody"
    vec4 pBody[];
};
layout(std430, binding = 3) readonly buffer _bodyTriangles {
    vec4 bodyTriangles[];
};
layout(std430, binding = 4) buffer _collisionConstraints { // vec4s of normal dir and distance 
    vec4 pClothCollisionConstraints[];
};
layout(std430, binding = 5) buffer _debug { // vec4s of debug data
    vec4 debug[];
};

layout(local_size_x = WORK_GROUP_SIZE, local_size_y = 1, local_size_z = 1) in;

layout(location = 0) uniform int numTriangles;
layout(location = 1) uniform int numPositions;
layout(location = 2) uniform float staticConstraintBounce;

vec3 nearestPointOnTriangle(vec3 P, vec3 A, vec3 B, vec3 C)
{
    vec3 v0 = C - A;
    vec3 v1 = B - A;
    vec3 N_v2 = cross(normalize(v1), normalize(v0));

    // case 1: it's in the triangle
    // project into triangle plane
    // (project an arbitrary vector from P to triangle onto the normal)
    vec3 w = A - P;
    float signedDistance_invDenom = dot(N_v2, w);
    vec3 projP = P + signedDistance_invDenom * N_v2;

    // compute u v coordinates
    // http://www.blackpawn.com/texts/pointinpoly/
    //u = ((v1.v1)(v2.v0) - (v1.v0)(v2.v1)) / ((v0.v0)(v1.v1) - (v0.v1)(v1.v0))
    //v = ((v0.v0)(v2.v1) - (v0.v1)(v2.v0)) / ((v0.v0)(v1.v1) - (v0.v1)(v1.v0))
    N_v2 = projP - A;
    float dot00_tAB = dot(v0, v0);
    float dot01_tBC = dot(v0, v1);
    float dot02_tCA = dot(v0, N_v2);
    float dot11_minDistance = dot(v1, v1);
    float dot12_candidate = dot(v1, N_v2);

    signedDistance_invDenom = 1 / (dot00_tAB * dot11_minDistance - dot01_tBC * dot01_tBC);
    float u_t = (dot11_minDistance * dot02_tCA - dot01_tBC * dot12_candidate) * signedDistance_invDenom;
    float v = (dot00_tAB * dot12_candidate - dot01_tBC * dot02_tCA) * signedDistance_invDenom;

    // if the u v is in bounds, we can return the projected point
    if (u_t >= 0.0 && v >= 0.0 && (u_t + v) <= 1.0) {
      return projP;
      }

      // case 2: it's on an edge
      // http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
      // t = - (x1 - x0) dot (x2 - x1) / len(x2 - x1) ^ 2
      // x1 is the "line origin," x2 is the "towards" point, and x0 is the outlier
      // check A->B edge
      dot00_tAB = -dot(A - P, B - A) / pow(length(B - A), 2.0f);

      // check B->C edge
      dot01_tBC = -dot(B - P, C - B) / pow(length(C - B), 2.0f);

      // check C->A edge
      dot02_tCA = -dot(C - P, A - C) / pow(length(A - C), 2.0f);

    // handle case 3: point is closest to a vertex
    dot00_tAB = clamp(dot00_tAB, 0.0f, 1.0f);
    dot01_tBC = clamp(dot01_tBC, 0.0f, 1.0f);
    dot02_tCA = clamp(dot02_tCA, 0.0f, 1.0f);

      // assess each edge's distance and parametrics
    dot11_minDistance = length(cross(P - A, P - B)) / length(B - A);
    dot12_candidate;
      v0 = A;
      v1 = B;
    u_t = dot00_tAB;

    dot12_candidate = length(cross(P - B, P - C)) / length(B - C);
    if (dot12_candidate < dot11_minDistance) {
      dot11_minDistance = dot12_candidate;
      v0 = B;
      v1 = C;
      u_t = dot01_tBC;
      }

      dot12_candidate = length(cross(P - C, P - A)) / length(C - A);
    if (dot12_candidate < dot11_minDistance) {
      dot11_minDistance = dot12_candidate;
      v0 = C;
      v1 = A;
      u_t = dot02_tCA;
      }
    return (u_t * (v1 - v0) + v0);
}

void generateStaticConstraint(vec3 pos) {
    uint idx = gl_GlobalInvocationID.x;

    // static constraint: generate a "point of entry" approximating the closest
    // point on the mesh to the pos from the last timestep (pos).
    // Move the position in the last timestep based on this "point of entry" and
    // use the normal at this point to generate a constraint that will get
    // the point in this timestep out.

    vec3 triangle = bodyTriangles[0].xyz;
    vec3 v0 = pBody[int(triangle.x)].xyz;
    vec3 v1 = pBody[int(triangle.y)].xyz;
    vec3 v2 = pBody[int(triangle.z)].xyz;
    vec3 nearestPoint = nearestPointOnTriangle(pos, v0, v1, v2);
    vec3 nearestNormal = normalize(cross(v1 - v0, v2 - v0));
    float nearestDistance = length(nearestPoint - pos);
    vec3 candidatePoint;
    float candidateDistance;

    for (int i = 1; i < numTriangles; i++) {

        triangle = bodyTriangles[i].xyz;
        v0 = pBody[int(triangle.x)].xyz;
        v1 = pBody[int(triangle.y)].xyz;
        v2 = pBody[int(triangle.z)].xyz;
        candidatePoint = nearestPointOnTriangle(pos, v0, v1, v2);
        candidateDistance = length(candidatePoint - pos);
        if (candidateDistance < nearestDistance) {
            nearestDistance = candidateDistance;
            nearestPoint = candidatePoint;
            nearestNormal = normalize(cross(v1 - v0, v2 - v0));
        }
    }

    // move the position in the last timestep over to nearestPoint
    pCloth1[idx].xyz = nearestPoint + nearestNormal * staticConstraintBounce;
    pClothCollisionConstraints[idx] = vec4(nearestNormal, 1.0);
    return;
}

float mollerTrumboreIntersectTriangle(vec3 orig, vec3 dir, vec3 v0, vec3 v1, vec3 v2)
{
    
    // adapted from Moller-Trumbore intersection algorithm pseudocode on wikipedia
    vec3 e1, e2; // Edge1, Edge2
    vec3 P, Q, T;
    float det, inv_det, u, v;
    float t;
    
    // vectors for edges sharing V1
    e1 = v1 - v0;
    e2 = v2 - v0;

    // begin calculating determinant - also used to calculate u param
    P = cross(dir, e2);

    // if determinant is near zero, ray lies in plane of triangle
    det = dot(e1, P);
    // NOT culling
    if (det > -EPSILON && det < EPSILON) return -1.0;
    inv_det = 1.0 / det;

    // calculate distance from v0 to ray origin
    T = orig - v0;

    // calculate u parameter and test bound
    u = dot(T, P) * inv_det;
    // the intersection lies outside of the triangle
    if (u < 0.0 || u > 1.0) return -1.0;

    // prepare to test v parameter
    Q = cross(T, e1);

    // calculate v param and test bound
    v = dot(dir, Q) * inv_det;

    // the intersection is outside the triangle?
    if (v < 0.0 || (u + v) > 1.0) return -1.0;

    t = dot(e2, Q) * inv_det;

    if (t > 0.0) {
        return t;
    }

    return -1.0;
}

float altIntersectTriangle(vec3 orig, vec3 dir, vec3 v0, vec3 v1, vec3 v2)
{
    // adapted from http://undernones.blogspot.com/2010/12/gpu-ray-tracing-with-glsl.html
    // buggy? -> in some sims but not others. ditto with method above. wat
    vec3 u, v, n; // triangle vectors
    vec3 w0, w;  // ray float
    float r, a, b; // params to calc ray-plane intersect

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
    if (r <= 0.0)
        return -1.0; // ray goes away from triangle.

    // baycentric coordinates part
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

    return (r >= 0.0) ? r : -1.0;
}

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= numPositions) return;

    // check if there's already a valid constraint. if so, do nothing
    if (pClothCollisionConstraints[idx].w >= 0.0) return; 

    // also, if this is infinite weighted, do nothing
    // inverse mass is the w in the position
    if (pCloth1[idx].w < EPSILON) return;

    vec3 pos = pCloth1[idx].xyz; // prev timestep
    vec3 lookAt = pCloth2[idx].xyz; // next timestep
    float dirScale = length(lookAt - pos);
    vec3 dir = normalize(lookAt - pos);

    vec4 collisionConstraint = vec4(-1.0); // a bogus collisionConstraint

    // if there's an odd number of collisions, we're inside the mesh already
    int numCollisions = 0; // which means we need a static constraint (addtl handling here)

    debug[idx] = vec4(-1.0);
    vec3 debugPos;

    // check against every triangle in the mesh.
    for (int i = 0; i < numTriangles; i++) {

        vec3 triangle = bodyTriangles[i].xyz;
        vec3 v0 = pBody[int(triangle.x)].xyz;
        vec3 v1 = pBody[int(triangle.y)].xyz;
        vec3 v2 = pBody[int(triangle.z)].xyz;
        vec3 norm = normalize(cross(v1 - v0, v2 - v0));

        // b/c intersectTriangle gets us a distance with a normalized dir vector
        // intersectTriangle = realLength * dirScale
        // intersectTriangle / dirScale = realLength
        float collisionT = mollerTrumboreIntersectTriangle(pos, dir, v0, v1, v2);
        // collision out of bounds
        if (collisionT > -EPSILON) {
            numCollisions++;
            debugPos = pos + (collisionT / dirScale) * (lookAt - pos);
        }
        collisionT /= dirScale;
        if (collisionT > 1.0 || collisionT < 0.0) {
            continue;
        }
        //use the nearest collision with distance less than 1
        if (collisionConstraint.w < 0.0 ||
            collisionT < collisionConstraint.w) {
            collisionConstraint.xyz = norm;
            collisionConstraint.w = collisionT;
        }
    }
    debug[idx].xyz = debugPos;
    debug[idx].w = numCollisions;

    // if the number of collisions is odd
    // and no triangle was crossed in the timestep, <- ? seems logical but leads to odd results
    // generate a static constraint instead.
    if (numCollisions % 2 != 0) {//} && collisionConstraint.w < 0.0) {
        //pCloth2[idx].xyz = debugPos;//vec3(0.0, 0.0, -numCollisions); // debug
        generateStaticConstraint(pos);
        return;
    }

    pClothCollisionConstraints[idx] = collisionConstraint;
}
