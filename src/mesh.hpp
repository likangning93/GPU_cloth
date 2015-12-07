#pragma once
#include <glm/glm.hpp>
#include <cstdlib>
#include <cstdio>
#include "glslUtility.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

// holds pointers to everything that can be rendered from a mesh
// - GL buffer for positions
// - index buffer to help with rendering
// - obj loader -> expects quads

using namespace std;

class Mesh
{
public:
  string filename;
  GLuint ssbo_pos; // shader storage buffer object -> holds positions
  GLuint idxbo; // index buffer

  vector<glm::vec4> initPositions;
  vector<glm::ivec4> indicesQuads;
  vector<int> indicesTris;

  Mesh(string filename);
  ~Mesh();

private:
  void buildGeomtery();
  void placeToken(string token, ifstream *myfile);

};