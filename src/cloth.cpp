#include "cloth.hpp"
#include "checkGLError.hpp"

Cloth::Cloth(string filename) : Mesh(filename) {
  glGenBuffers(1, &ssbo_vel);
  generateConstraints();
}

Cloth::~Cloth() {

}

struct constraintVertexIndices {
  int thisIndex = -1;
  int numConstraints = 0;
  int indices[4];
};

void addConstraint(constraintVertexIndices &vert1, constraintVertexIndices &vert2) {
  // assess whether the constraints given already exist here or not
  for (int i = 0; i < 4; i++) {
    if (vert1.indices[i] == vert2.thisIndex) {
      return;
    }
    if (vert2.indices[i] == vert1.thisIndex) {
      return;
    }
  }

  // set up constraints on vert and influencer based on each other
  if (vert1.numConstraints < 4) {
    vert1.indices[vert1.numConstraints] = vert2.thisIndex;
    vert1.numConstraints++;
  }
  if (vert2.numConstraints < 4) {
    vert2.indices[vert2.numConstraints] = vert1.thisIndex;
    vert2.numConstraints++;
  }
}

void Cloth::generateConstraints() {

  /*****************************************************************************
   The standard mass-spring system constraint requires:
   - (1) index of vertex being moved
   - (1) index of vertex "not being moved"
   - (1) rest length
   So each constraint "struct" is unidirectional; real constraints are thus
   made up of 2 constraints each.
   The obj is loaded such that each face is "half-edged," counterclockwise.
   1---2---5
   |   |   |        N
   4---3---6  ->  W 3 E
   |   |   |        S
   44--43--45
   So we need to generate a constraint for all edges specified by indices
   In addition, we need to generate constraints for all indices that don't have
   a matched pair (in this case, 1-2, 2-5, etc.)

   Finally, since each constraint is designed to correct the predicted position
   of the "receiving" vertex and each vertex as such will have at most 4
   constraints (we're ignoring bending for now b/c we're super bad) we must
   use 4 buffers of constraints to avoid race conditions.

   So what we'll do is determine for each vertex just what constraints it should
   have, then distribute each vert's constraints into each of the 4 buffers.
   We'll do this by building 2 constraints for every edge given in a quadface.
   We don't actually NEED the N,S,E,W constraints to be uniform like this.
  *****************************************************************************/
  int numVertices = initPositions.size();
  std::vector<constraintVertexIndices> constraintsPerVertex;
  for (int i = 0; i < numVertices; i++) {
    constraintVertexIndices emptyVert;
    emptyVert.thisIndex = i;
    emptyVert.indices[0] = -1;
    emptyVert.indices[1] = -1;
    emptyVert.indices[2] = -1;
    emptyVert.indices[3] = -1;
    constraintsPerVertex.push_back(emptyVert);
  }
  // Check every face and input the constraints per vertex.
  int numFaces = indicesQuads.size();
  for (int i = 0; i < numFaces; i++) {
    // each face generates 4 constraints
    glm::ivec4 face = indicesQuads.at(i);
    addConstraint(constraintsPerVertex[face[0]], constraintsPerVertex[face[1]]);
    addConstraint(constraintsPerVertex[face[1]], constraintsPerVertex[face[2]]);
    addConstraint(constraintsPerVertex[face[2]], constraintsPerVertex[face[3]]);
    addConstraint(constraintsPerVertex[face[3]], constraintsPerVertex[face[0]]);
  }

  // deploy constraints and buffers

  for (int i = 0; i < numVertices; i++) {
    constraintVertexIndices currSet = constraintsPerVertex.at(i);
    for (int j = 0; j < 4; j++) {
      if (currSet.indices[j] != -1) {
        glm::vec3 shaderConstraint;
        shaderConstraint[0] = currSet.thisIndex;
        shaderConstraint[1] = currSet.indices[j];
        glm::vec3 p1 = initPositions.at(i);
        glm::vec3 p2 = initPositions.at(i);
        shaderConstraint[2] = glm::length(p1 - p2);
        constraints[j].push_back(shaderConstraint);
      }
    }
  }

  GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

  for (int i = 0; i < 4; i++) {
    // gen buffer
    glGenBuffers(1, &ssbo_constraints[i]);
	checkGLError("init cloths");

    // allocate space for constraints on GPU
    int numConstraints = constraints[i].size();
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_constraints[i]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numConstraints * sizeof(glm::vec3),
      NULL, GL_STREAM_COPY);

    // transfer
    glm::vec3 *constraintsMapped = (glm::vec3 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
      0, numConstraints * sizeof(glm::vec3), bufMask);

    for (int j = 0; j < numConstraints; j++) {
      constraintsMapped[j] = constraints[i].at(j);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  }
}
