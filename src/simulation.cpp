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
	prog_ppd2_dampVelocity = initComputeProg("../shaders/cloth_ppd2_dampVelocities.comp.glsl");
	prog_ppd3_predictPositions = initComputeProg("../shaders/cloth_ppd3_predictPositions.comp.glsl");
	prog_ppd4_projectClothConstraints = initComputeProg("../shaders/cloth_ppd4_projectClothConstraints.comp.glsl");
	prog_ppd6_updateVelPos = initComputeProg("../shaders/cloth_ppd6_updatePositionsVelocities.comp.glsl");
}

void Simulation::stepSingleCloth(Cloth *cloth) {
	int numVertices = cloth->initPositions.size();

	/* compute new velocities with external forces */
	glUseProgram(prog_ppd1_externalForces);
	// bind velocities input/output
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	// Dispatch compute shader
	int workGroupCount_vertices = (numVertices - 1) / WORK_GROUP_SIZE_ACC + 1;
	glDispatchCompute(workGroupCount_vertices, 1, 1);


	/* damp velocities */
	glUseProgram(prog_ppd2_dampVelocity);
	// bind velocities input/output
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	// Dispatch compute shader
	glDispatchCompute(workGroupCount_vertices, 1, 1);


	/* predict new positions */
	glUseProgram(prog_ppd3_predictPositions);
	// bind velocities input
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	// bind positions input and output
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_pos_pred);
	// Dispatch compute shader
	glDispatchCompute(workGroupCount_vertices, 1, 1);


	/* project cloth constraints N times */
	glUseProgram(prog_ppd4_projectClothConstraints);
	for (int i = 0; i < projectTimes; i++) {
		// project each of the 4 internal constraints
		// bind positions input
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos);
		// bind predicted positions input/output
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred);
		for (int j = 0; j < 4; j++) {
			// bind inner constraints
			int workGroupCountInnerConstraints = cloth->internalConstraints[j].size();
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_internalConstraints[j]);
			glDispatchCompute(workGroupCountInnerConstraints, 1, 1);
		}
	}
	// project pin constraints
	int workGroupCountPinConstraints = cloth->externalConstraints.size();
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_externalConstraints);
	glDispatchCompute(workGroupCountPinConstraints, 1, 1);


	/* update positions and velocities */
	glUseProgram(prog_ppd6_updateVelPos);
	// bind velocities input
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	// bind positions input and output
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_pos_pred);
	// Dispatch compute shader
	glDispatchCompute(workGroupCount_vertices, 1, 1);
}

void Simulation::stepSimulation() {
	for (int i = 0; i < numCloths; i++) {
		stepSingleCloth(cloths.at(i));
	}
}