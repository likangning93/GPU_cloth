#include "simulation.hpp"
#include "checkGLError.hpp"

// TODO: perform timing experiments, based on work group size.
// You must ALSO change the work group size in the compute shaders.
#define WORK_GROUP_SIZE_ACC 16
#define WORK_GROUP_SIZE_VELPOS 16

Simulation::Simulation(vector<string> &body_filenames,
	vector<string> &cloth_filenames) {
	initComputeProgs();

	numRigids = body_filenames.size();
	for (int i = 0; i < numRigids; i++) {
		Rbody *newCollider = new Rbody(body_filenames.at(i));
		rigids.push_back(newCollider);
	}
	checkGLError("init rbodies");

	numCloths = cloth_filenames.size();
	for (int i = 0; i < numCloths; i++) {
		Cloth *newCloth = new Cloth(cloth_filenames.at(i));
		newCloth->uploadExternalConstraints(); // TODO: test
		cloths.push_back(newCloth);
	}
	checkGLError("init cloths");

}

Simulation::~Simulation() {
	// delete all the meshes and rigidbodies
	for (int i = 0; i < numRigids; i++) {
		delete(&rigids.at(i));
	}
	for (int i = 0; i < numCloths; i++) {
		delete(&cloths.at(i));
	}
}

GLuint initComputeProg(const char *path) {
    GLuint prog = glCreateProgram();
    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);

    int cs_len;
    const char *cs_str;
    cs_str = glslUtility::loadFile(path, cs_len);

	printf(cs_str);
	cout << endl;

    glShaderSource(cs, 1, &cs_str, &cs_len);

    GLint status;

    glCompileShader(cs);
    glGetShaderiv(cs, GL_COMPILE_STATUS, &status);
    if (!status) {
        printf("Error compiling compute shader: %s\n", path);
        glslUtility::printShaderInfoLog(cs);
        exit(EXIT_FAILURE);
    }
    delete[] cs_str;

    glAttachShader(prog, cs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        printf("Error linking compute shader: %s\n", path);
        glslUtility::printLinkInfoLog(prog);
        exit(EXIT_FAILURE);
    }
	return prog;
}

void Simulation::initComputeProgs() {
	prog_ppd1_externalForces = initComputeProg("../shaders/cloth_ppd1_externalForces.comp.glsl");
	glUseProgram(prog_ppd1_externalForces);
	glm::vec3 G = glm::vec3(0.0f, 0.0f, -0.98f);
	glUniform3fv(1, 1, &G[0]);
	glUniform1f(0, 0.016f);

	prog_ppd2_dampVelocity = initComputeProg("../shaders/cloth_ppd2_dampVelocities.comp.glsl");

	prog_ppd3_predictPositions = initComputeProg("../shaders/cloth_ppd3_predictPositions.comp.glsl");
	glUseProgram(prog_ppd3_predictPositions);
	glUniform1f(0, 0.016f);

	prog_ppd4_projectClothConstraints = initComputeProg("../shaders/cloth_ppd4_projectClothConstraints.comp.glsl");
	glUseProgram(prog_ppd4_projectClothConstraints);
	glUniform1f(0, 0.9f);
	glUniform1f(1, 4.0f);

	prog_ppd6_updateVelPos = initComputeProg("../shaders/cloth_ppd6_updatePositionsVelocities.comp.glsl");
	glUseProgram(prog_ppd6_updateVelPos);
	glUniform1f(0, 0.016f);
	
	prog_copyBuffer = initComputeProg("../shaders/copy.comp.glsl");
}

void Simulation::stepSingleCloth(Cloth *cloth) {
	int numVertices = cloth->initPositions.size();
	int workGroupCount_vertices = (numVertices - 1) / WORK_GROUP_SIZE_ACC + 1;

	/* compute new velocities with external forces */
	glUseProgram(prog_ppd1_externalForces);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	glDispatchCompute(workGroupCount_vertices, 1, 1);


	/* damp velocities */
	glUseProgram(prog_ppd2_dampVelocity);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	glDispatchCompute(workGroupCount_vertices, 1, 1);
	
	
	/* predict new positions */
	glUseProgram(prog_ppd3_predictPositions);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_pos_pred1);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cloth->ssbo_pos_pred2);

	glDispatchCompute(workGroupCount_vertices, 1, 1);
	
	
	/* project cloth constraints N times */
	for (int i = 0; i < projectTimes; i++) {
		glUseProgram(prog_ppd4_projectClothConstraints);
		// project each of the 4 internal constraints
		// bind positions input
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos);
		// bind predicted positions input/output
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred1);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_pos_pred2);

		for (int j = 0; j < 4; j++) {
			// bind inner constraints
			int workGroupCountInnerConstraints = 
				(cloth->internalConstraints[j].size() - 1) / WORK_GROUP_SIZE_ACC + 1;
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cloth->ssbo_internalConstraints[j]);
			glDispatchCompute(workGroupCountInnerConstraints, 1, 1);
		}
		// ffwd pred1 to match pred2
		
		glUseProgram(prog_copyBuffer); // TODO: lol... THIS IS DUMB DO SOMETHING BETTER
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos_pred2);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred1);
		glDispatchCompute(workGroupCount_vertices, 1, 1);
	}
	
	// project pin constraints
	glUseProgram(prog_ppd4_projectClothConstraints);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos);
	// bind predicted positions input/output
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred1);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_pos_pred2);
	int workGroupCountPinConstraints = cloth->externalConstraints.size();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cloth->ssbo_externalConstraints);
	glDispatchCompute(workGroupCountPinConstraints, 1, 1);
	
	/* update positions and velocities */
	glUseProgram(prog_ppd6_updateVelPos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_pos_pred2); // no ffwd
	glDispatchCompute(workGroupCount_vertices, 1, 1);

	//retrieveBuffer(cloth->ssbo_pos, 5);
}

void Simulation::retrieveBuffer(GLuint ssbo, int numItems) {
	// test getting something back from the GPU. can we even do this?!
	GLint bufMask = GL_MAP_READ_BIT;
	
	std::vector<glm::vec4> positions;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glm::vec4 *constraintsMapped = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, numItems * sizeof(glm::vec4), bufMask);
	for (int i = 0; i < numItems; i++) {
		positions.push_back(glm::vec4(constraintsMapped[i]));
	}
	checkGLError("backcopy");

	for (int i = 0; i < numItems; i++) {
		cout << positions.at(i).z << " ";
	}
	cout << endl;
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void Simulation::stepSimulation() {
	for (int i = 0; i < numCloths; i++) {
		stepSingleCloth(cloths.at(i));
	}
}