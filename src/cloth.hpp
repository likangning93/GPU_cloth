#pragma once
#include "mesh.hpp"

#define NUM_INT_CON_BUFFERS 8 // number of internal constraint buffers

// holds pointers to everything for a Cloth object:
// - (2) GL buffer for predicted positions
// - (1) GL buffer for velocities
// - (4) GL buffers for internal forces


class Cloth : public Mesh
{
public:
	int numInternalConstraintBuffers = NUM_INT_CON_BUFFERS;

	// positions will be vec4s: x, y, z, mass
	// predicted positions will also be vec4s: x, y, z, invMass

  GLuint ssbo_pos_pred1; // predicted positions buffer
  GLuint ssbo_pos_pred2; // predicted positions buffer

  GLuint ssbo_vel; // shader storage buffer object -> holds velocities
  GLuint ssbo_debug;

  // all constraints in these buffers are vec4s:
  // index of pos to modify, index of influencer, rest length, stiffness K
  // a negative rest length will indicate a "pin" constraint
  GLuint ssbo_internalConstraints[NUM_INT_CON_BUFFERS];
  GLuint ssbo_externalConstraints;

  GLuint ssbo_collisionConstraints;

  float default_internal_K = 0.9f;
  float default_pin_K = 1.0f;
  float default_inv_mass = 441.0f;
  float default_static_constraint_bounce = 0.1f;

  // these vec3 constraints are index, index, rest length
  std::vector<glm::vec4> internalConstraints[NUM_INT_CON_BUFFERS];
  std::vector<glm::vec4> externalConstraints; // pin

  std::vector<GLuint> pinnedSSBOs; // SSBOs that this is pinned to

  Cloth(string filename, glm::vec3 jitter);
  ~Cloth();
  void addPinConstraint(int thisIdx, int otherIdx, GLuint SSBO_ID);
  void uploadExternalConstraints(); // upload all determined constraints.

private:
  void generateConstraints();
};