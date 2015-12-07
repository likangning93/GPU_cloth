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

    glShaderSource(cs, 1, &cs_str, &cs_len);

    GLint status;

    glCompileShader(cs);
    glGetShaderiv(cs, GL_COMPILE_STATUS, &status);
    if (!status) {
        printf("Error compiling compute shader: %s\n", path);
        glslUtility::printShaderInfoLog(cs);
		printf(cs_str);
		cout << endl;
        exit(EXIT_FAILURE);
    }
    delete[] cs_str;

    glAttachShader(prog, cs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        printf("Error linking compute shader: %s\n", path);
        glslUtility::printLinkInfoLog(prog);
		printf(cs_str);
		cout << endl;
        exit(EXIT_FAILURE);
    }
	return prog;
}

void Simulation::initComputeProgs() {
	prog_ppd1_externalForces = initComputeProg("../shaders/cloth_ppd1_externalForces.comp.glsl");
	glUseProgram(prog_ppd1_externalForces);
	glUniform3fv(1, 1, &Gravity[0]);
	glUniform1f(0, timeStep);

	prog_ppd2_dampVelocity = initComputeProg("../shaders/cloth_ppd2_dampVelocities.comp.glsl");

	prog_ppd3_predictPositions = initComputeProg("../shaders/cloth_ppd3_predictPositions.comp.glsl");
	glUseProgram(prog_ppd3_predictPositions);
	glUniform1f(0, timeStep);

	prog_ppd4_updateInvMass = initComputeProg("../shaders/cloth_ppd4_updateInverseMasses.comp.glsl");

	prog_ppd6_projectClothConstraints = initComputeProg("../shaders/cloth_ppd6_projectClothConstraints.comp.glsl");
	glUseProgram(prog_ppd6_projectClothConstraints);
	glUniform1f(0, (float) projectTimes);

	prog_ppd7_updateVelPos = initComputeProg("../shaders/cloth_ppd7_updatePositionsVelocities.comp.glsl");
	glUseProgram(prog_ppd7_updateVelPos);
	glUniform1f(0, timeStep);
	
	prog_copyBuffer = initComputeProg("../shaders/copy.comp.glsl");

	prog_genCollisionConstraints = initComputeProg("../shaders/cloth_genCollisions.comp.glsl");

	prog_projectCollisionConstraints = initComputeProg("../shaders/cloth_projectCollisions.comp.glsl");
}

void Simulation::genCollisionConstraints(Cloth *cloth, Rbody *rbody) {
	int numVertices = cloth->initPositions.size();
	glUseProgram(prog_genCollisionConstraints);
	glUniform1i(0, rbody->indicesTris.size() / 3);
	glUniform1i(1, numVertices);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred2);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, rbody->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, rbody->ssbo_triangles);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, cloth->ssbo_collisionConstraints);
	
	int workGroupCount_vertices = (numVertices - 1) / WORK_GROUP_SIZE_ACC + 1;
	glDispatchCompute(workGroupCount_vertices, 1, 1);
}

void Simulation::stepSingleCloth(Cloth *cloth) {
	int numVertices = cloth->initPositions.size();
	int workGroupCount_vertices = (numVertices - 1) / WORK_GROUP_SIZE_ACC + 1;

	/* compute new velocities with external forces */
	glUseProgram(prog_ppd1_externalForces);
	glUniform1i(2, numVertices);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	glDispatchCompute(workGroupCount_vertices, 1, 1);


	/* damp velocities */
	glUseProgram(prog_ppd2_dampVelocity);
	glUniform1i(0, numVertices);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	glDispatchCompute(workGroupCount_vertices, 1, 1);
	
	
	/* predict new positions */
	glUseProgram(prog_ppd3_predictPositions);
	glUniform1i(1, numVertices);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_pos_pred1);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cloth->ssbo_pos_pred2);

	glDispatchCompute(workGroupCount_vertices, 1, 1);
	
	/* update inverse masses */
	int numPinConstraints = cloth->externalConstraints.size();
	int workGroupCountPinConstraints = (numPinConstraints - 1) / WORK_GROUP_SIZE_ACC + 1;
	glUseProgram(prog_ppd4_updateInvMass);
	glUniform1i(0, numPinConstraints);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos_pred1);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred2);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_externalConstraints);
	glDispatchCompute(workGroupCountPinConstraints, 1, 1);

	/* project cloth constraints N times */
	for (int i = 0; i < projectTimes; i++) {
		glUseProgram(prog_ppd6_projectClothConstraints);
		// project each of the 4 internal constraints
		// bind predicted positions input/output
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos_pred1);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred2);

		for (int j = 0; j < 4; j++) {
			// bind inner constraints
			int workGroupCountInnerConstraints = 
				(cloth->internalConstraints[j].size() - 1) / WORK_GROUP_SIZE_ACC + 1;
			glUniform1i(1, cloth->internalConstraints[j].size());

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_internalConstraints[j]);
			// project this set of constraints
			glDispatchCompute(workGroupCountInnerConstraints, 1, 1);
		}

		// project pin constraints
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos); // init positions, not pred 1
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred2);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_externalConstraints);
		glUniform1i(1, numPinConstraints);
		glDispatchCompute(workGroupCountPinConstraints, 1, 1);

		// ffwd pred1 to match pred2
		
		glUseProgram(prog_copyBuffer); // TODO: lol... THIS IS DUMB DO SOMETHING BETTER
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos_pred2);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred1);
		glDispatchCompute(workGroupCount_vertices, 1, 1);
	}
	
	/* generate and resolve collision constraints */
	for (int i = 0; i < numRigids; i++) {
		genCollisionConstraints(cloth, rigids.at(i));
	}
	glUseProgram(prog_projectCollisionConstraints);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred2);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_collisionConstraints);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cloth->ssbo_vel);
	glUniform1i(0, numVertices);
	glDispatchCompute(workGroupCount_vertices, 1, 1);

	/* update positions and velocities */
	glUseProgram(prog_ppd7_updateVelPos);
	glUniform1i(1, numVertices);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_pos_pred2); // no ffwd
	glDispatchCompute(workGroupCount_vertices, 1, 1);

	//retrieveBuffer(cloth->ssbo_internalConstraints[0], 100);
	//retrieveBuffer(cloth->ssbo_internalConstraints[1], 100);
	//retrieveBuffer(cloth->ssbo_internalConstraints[2], 100);
	//retrieveBuffer(cloth->ssbo_internalConstraints[3], 100);

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
		if (int(positions.at(i).x) == int(positions.at(i).y)) {
			cout << "fart" << endl;
		}
		//cout << positions.at(i).x << " " << positions.at(i).y << " " << positions.at(i).z << " " << endl;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void Simulation::stepSimulation() {
	for (int i = 0; i < numCloths; i++) {
		stepSingleCloth(cloths.at(i));
	}
}