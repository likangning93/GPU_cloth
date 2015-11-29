#pragma once
#include "mesh.hpp"

// holds pointers to everything for a Cloth object:
// - (2) GL buffer for predicted positions
// - (1) GL buffer for velocities
// - (4) GL buffers for internal forces


class Cloth : public Mesh
{
public:
  GLuint ssbo_pos_p1; // predicted positions buffer 1
  GLuint ssbo_pos_p2; // predicted positions buffer 2
  GLuint ssbo_vel; // shader storage buffer object -> holds velocities

  GLuint ssbo_constraints[4];

  std::vector<glm::vec3> constraints[4];

  Cloth(string filename);
  ~Cloth();

private:
  void generateConstraints();
};