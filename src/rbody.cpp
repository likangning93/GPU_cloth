#include "rbody.hpp"

Rbody::Rbody(string filename) : Mesh(filename) {
	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

	// set up triangles and triangle buffer
	int numTriangles = indicesTris.size() / 3;
	glGenBuffers(1, &ssbo_triangles);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_triangles);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numTriangles * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);
	glm::vec4 *pos = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, numTriangles * sizeof(glm::vec4), bufMask);
	for (int i = 0; i < numTriangles; i++) {
		glm::vec4 triangle;
		triangle.x = indicesTris.at(i * 3 + 0);
		triangle.y = indicesTris.at(i * 3 + 1);
		triangle.z = indicesTris.at(i * 3 + 2);

		pos[i] = triangle;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	/*
	// set up some bogus weights for now
	int numVertices = this->initPositions.size();
	for (int i = 0; i < numVertices; i++) {
		weights.push_back(1.0f);
	}
	

	// redo the positions buffer to include weights
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pos);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numVertices * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);
	glm::vec4 *pos = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, numVertices * sizeof(glm::vec4), bufMask);
	for (int i = 0; i < numVertices; i++) {
		pos[i].w = weights.at(i);
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	// set up the animated positions buffer
	glGenBuffers(1, &ssbo_animPos);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_animPos);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numVertices * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);
	glm::vec4 *pos = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, numVertices * sizeof(glm::vec4), bufMask);
	for (int i = 0; i < numVertices; i++) {
		pos[i] = initPositions[i];
		pos[i].w = weights.at(i);
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); */
}

Rbody::~Rbody() {

}
