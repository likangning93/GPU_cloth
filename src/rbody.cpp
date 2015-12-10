#include "rbody.hpp"

Rbody::Rbody(string filename) : Mesh(filename) {
	// animation state
	translation = glm::vec3(0.0);
	scale = glm::vec3(1.0);
	eulerRotation = glm::vec3(0.0);

	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

	// set up triangles and triangle buffer
	int numTriangles = indicesTris.size() / 3;
	glGenBuffers(1, &ssbo_triangles);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_triangles);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numTriangles * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);
	glm::vec4 *tri = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, numTriangles * sizeof(glm::vec4), bufMask);
	for (int i = 0; i < numTriangles; i++) {
		glm::vec4 triangle;
		triangle.x = indicesTris.at(i * 3 + 0);
		triangle.y = indicesTris.at(i * 3 + 1);
		triangle.z = indicesTris.at(i * 3 + 2);

		tri[i] = triangle;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	int numVertices = this->initPositions.size();

	// set up the animated positions buffer
	glGenBuffers(1, &ssbo_initPos);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_initPos);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numVertices * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);
	glm::vec4 *pos = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, numVertices * sizeof(glm::vec4), bufMask);
	for (int i = 0; i < numVertices; i++) {
		pos[i] = initPositions[i];
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

Rbody::~Rbody() {

}

glm::mat4 Rbody::getTransformationAtTime(float t) {
	return sineHop(t) * twirl(t);
	//return sineHop(t);
}

glm::mat4 Rbody::twirl(float t) {
	glm::vec3 eulerRotationT = eulerRotation;
	eulerRotationT.z = t * 2.0f;
	return glm::eulerAngleZ(eulerRotationT.z);
}

glm::mat4 Rbody::sineHop(float t) {
	glm::vec3 translationT = translation;
	translationT.x = t * 0.5f;
	translationT.z = abs(sin(t)) * 0.5f;
	return glm::translate(translationT);
}