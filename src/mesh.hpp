#pragma once
#include <glm/glm.hpp>
#include <cstdlib>
#include <cstdio>
#include "glslUtility.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>

// holds pointers to everything that can be rendered from a mesh
// - GL buffer for positions
// - index buffer to help with rendering
// - obj loader -> expects quads

using namespace std;

class Mesh
{
public:
  string filename;
  GLuint attr_position = 0;
  GLuint drawingVAO;
  GLuint ssbo_pos; // shader storage buffer object -> holds positions
  GLuint idxbo; // index buffer
  //GLuint ssbo_debug; // ssbo for storing debug info

  vector<glm::vec4> initPositions;
  vector<glm::ivec4> indicesQuads;
  vector<int> indicesTris;

  glm::vec3 color;

  Mesh(string filename);
  Mesh(string filename, glm::vec3 jitter);
  ~Mesh();

private:
	glm::vec3 jitter;

  void buildGeometry();
  void placeToken(string token, ifstream *myfile);

};