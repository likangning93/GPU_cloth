#include "mesh.hpp"
#include "checkGLError.hpp"

Mesh::Mesh(string filename) {
  this->filename = filename;

  // compute a randomized, tiny jitter
  /* initialize random seed: */
  srand(time(NULL));

  /* generate secret number between 1 and 100: */
  jitter = glm::vec3(0.0f);

  // load things up
  buildGeometry();

  color = glm::vec3(0.6f);

  glGenVertexArrays(1, &drawingVAO);

  // build all the gl buffers and stuff
  glGenBuffers(1, &ssbo_pos);
  glGenBuffers(1, &idxbo);

  // upload indices
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxbo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesTris.size() * sizeof(GLuint),
	  &indicesTris[0], GL_STATIC_DRAW);

  // Initialize cloth positions on GPU

  GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
  int positionCount = initPositions.size();

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pos);
  glBufferData(GL_SHADER_STORAGE_BUFFER, positionCount * sizeof(glm::vec4),
    &initPositions[0], GL_STREAM_COPY);


//  // debug buffer
//  glGenBuffers(1, &ssbo_debug);
//  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_debug);
//  glBufferData(GL_SHADER_STORAGE_BUFFER, positionCount * sizeof(glm::vec4),
//	  NULL, GL_STREAM_COPY);

  //glm::vec4 *pos = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
  //  0, positionCount * sizeof(glm::vec4), bufMask);
  //for (int i = 0; i < positionCount; i++) {
  //  pos[i] = initPositions[i];
  //}
  //glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

  // bind indices to the VAO.
  glBindVertexArray(drawingVAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxbo);

  // try to bind the ssbo to the VAO. this doesn't work yet, see drawMesh in main
  glEnableVertexAttribArray(attr_position);
  glBindBuffer(GL_ARRAY_BUFFER, ssbo_pos);
  glVertexAttribPointer((GLuint)attr_position, 4, GL_FLOAT, GL_FALSE, 0, 0);

  // shut off the VAO
  glBindVertexArray(0);

  checkGLError("init mesh");
}

Mesh::Mesh(string filename, glm::vec3 jitter) {
	this->filename = filename;

	// compute a randomized, tiny jitter
	/* initialize random seed: */
	srand(time(NULL));

	/* generate secret number between 1 and 100: */
	this->jitter = jitter;

	// load things up
	buildGeometry();

	color = glm::vec3(0.6f);

	glGenVertexArrays(1, &drawingVAO);

	// build all the gl buffers and stuff
	glGenBuffers(1, &ssbo_pos);
	glGenBuffers(1, &idxbo);

	// upload indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesTris.size() * sizeof(GLuint),
		&indicesTris[0], GL_STATIC_DRAW);

	// Initialize cloth positions on GPU

	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	int positionCount = initPositions.size();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pos);
	glBufferData(GL_SHADER_STORAGE_BUFFER, positionCount * sizeof(glm::vec4),
		&initPositions[0], GL_STREAM_COPY);

	// bind indices to the VAO.
	glBindVertexArray(drawingVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxbo);

	// try to bind the ssbo to the VAO. this doesn't work yet, see drawMesh in main
	glEnableVertexAttribArray(attr_position);
	glBindBuffer(GL_ARRAY_BUFFER, ssbo_pos);
	glVertexAttribPointer((GLuint)attr_position, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// shut off the VAO
	glBindVertexArray(0);

	checkGLError("init mesh");
}

Mesh::~Mesh() {

}

void Mesh::buildGeometry()
{
  // load the file up
  ifstream myfile(filename);
  string line;
  if (myfile.is_open())
  {
    while (getline(myfile, line))
    {
      placeToken(line, &myfile);
    }
    myfile.close();
  }
  return;
}

glm::vec3 parseOneVec3(string token) {
  float x, y, z;
  sscanf_s(token.c_str(), "%f %f %f\n", &x, &y, &z);
  return glm::vec3(x, y, z);
}

glm::ivec4 parseOneivec4(string token) {
  int x, y, z, w;
  sscanf_s(token.c_str(), "%i %i %i %i\n", &x, &y, &z, &w);
  return glm::ivec4(x, y, z, w);
}

void Mesh::placeToken(string token, ifstream *myfile) {
    if (token.length() == 0)
    {
      ////std::cout << "newline maybe" << std::endl;
      return;
    }

    // case of a vertex
    if (token.compare(0, 1, "v") == 0)
    {
      token.erase(0, 2); // we'll assume v x y z, so erase v and whitespace space
	  initPositions.push_back(glm::vec4(parseOneVec3(token) + jitter, 1.0));
      return;
    }

    // case of a face index. expects quad faces
    if (token.compare(0, 1, "f") == 0)
    {
      token.erase(0, 2); // we'll assume f x y z, so erase f and whitespace space
        glm::ivec4 face_indices = parseOneivec4(token);
		// slight jitter to avoid extreme edge cases

        // objs are 1 indexed
        face_indices -= glm::ivec4(1);
        indicesQuads.push_back(face_indices);

        // compute triangular indices. quad indices assumed to be cclockwise
        // 0 3
        // 1 2
        glm::ivec3 tri1 = glm::ivec3(face_indices[0], face_indices[1], face_indices[2]);
        glm::ivec3 tri2 = glm::ivec3(face_indices[0], face_indices[2], face_indices[3]);
        indicesTris.push_back(tri1.x);
        indicesTris.push_back(tri1.y);
        indicesTris.push_back(tri1.z);
        indicesTris.push_back(tri2.x);
        indicesTris.push_back(tri2.y);
        indicesTris.push_back(tri2.z);
      return;
    }
    return;
}


