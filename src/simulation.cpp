#include "simulation.hpp"
#include "checkGLError.hpp"

// TODO: perform timing experiments, based on work group size.
// You must ALSO change the work group size in the compute shaders.
#define WORK_GROUP_SIZE 32

#define DEBUG_VERBOSE 0

#define QUERY_PERFORMANCE 1

#define PROJ_CONSTRAINTS 0
#define GENER_COLLISIONS 1
#define RESOL_COLLISIONS 2

Simulation::Simulation(vector<string> &body_filenames,
	vector<string> &cloth_filenames) {
	initComputeProgs();
	glm::vec3 jitter;
	int iSecret;
	iSecret = rand() % 100 + 1;
	jitter.x = (float)iSecret / 100000.0f;
	iSecret = rand() % 100 + 1;
	jitter.y = (float)iSecret / 100000.0f;
	iSecret = rand() % 100 + 1;
	jitter.z = (float)iSecret / 100000.0f;

	numRigids = body_filenames.size();
	for (int i = 0; i < numRigids; i++) {
		Rbody *newCollider = new Rbody(body_filenames.at(i));
		rigids.push_back(newCollider);
		checkGLError("init rbodies");
	}

	numCloths = cloth_filenames.size();
	for (int i = 0; i < numCloths; i++) {
		Cloth *newCloth = new Cloth(cloth_filenames.at(i), jitter);
		iSecret = rand() % 100 + 1;
		jitter.x = (float)iSecret / 100000.0f;
		iSecret = rand() % 100 + 1;
		jitter.y = (float)iSecret / 100000.0f;
		iSecret = rand() % 100 + 1;
		jitter.z = (float)iSecret / 100000.0f;
		newCloth->uploadExternalConstraints();
		cloths.push_back(newCloth);
		checkGLError("init cloths");
	}

#if QUERY_PERFORMANCE
	glGenQueries(1, &time_query);
	elapsed_time = 0;
	frameCount = 0;
	timeStagesTotal[PROJ_CONSTRAINTS] = 0;
	timeStagesTotal[GENER_COLLISIONS] = 0;
	timeStagesTotal[RESOL_COLLISIONS] = 0;
	timeStagesAVG[PROJ_CONSTRAINTS] = 0.0f;
	timeStagesAVG[GENER_COLLISIONS] = 0.0f;
	timeStagesAVG[RESOL_COLLISIONS] = 0.0f;
	timeStagesMin[PROJ_CONSTRAINTS] = HUGE_VAL;
	timeStagesMin[GENER_COLLISIONS] = HUGE_VAL;
	timeStagesMin[RESOL_COLLISIONS] = HUGE_VAL;
	timeStagesMax[PROJ_CONSTRAINTS] = 0;
	timeStagesMax[GENER_COLLISIONS] = 0;
	timeStagesMax[RESOL_COLLISIONS] = 0;
#endif

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

//http://stackoverflow.com/questions/3418231/replace-part-of-a-string-with-another-string
bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

GLuint initComputeProg(const char *path) {
    GLuint prog = glCreateProgram();
    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);

    int cs_len;
    const char *cs_str;
    cs_str = glslUtility::loadFile(path, cs_len);

	// check and edit the shader so the workgroup size is correct
	string str_shader = string(cs_str);
	str_shader.resize(cs_len);
	replace(str_shader, "WORK_GROUP_SIZE XX", "WORK_GROUP_SIZE " + std::to_string(WORK_GROUP_SIZE));

	cs_len = str_shader.length();

	char *cs_str_edited = new char[str_shader.length() + 1];
	strcpy(cs_str_edited, str_shader.c_str());

	//cs_str = str_shader->c_str();

	glShaderSource(cs, 1, &cs_str_edited, &cs_len);

    GLint status;

    glCompileShader(cs);
    glGetShaderiv(cs, GL_COMPILE_STATUS, &status);
    if (!status) {
        printf("Error compiling compute shader: %s\n", path);
        glslUtility::printShaderInfoLog(cs);
		printf(cs_str_edited);
		cout << endl;
        exit(EXIT_FAILURE);
    }

    glAttachShader(prog, cs);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (!status) {
        printf("Error linking compute shader: %s\n", path);
        glslUtility::printLinkInfoLog(prog);
		printf(cs_str_edited);
		cout << endl;
        exit(EXIT_FAILURE);
    }

	delete[] cs_str;
	delete[] cs_str_edited;

	return prog;
}

void Simulation::updateStat(int stat) {
	glEndQuery(GL_TIME_ELAPSED);

	// wait until the query is available and return.
	// if this takes too long, alert and continue.
	// TODO: see if the double buffering version works with compute shaders.
	int done = 0;
	while (!done) {
		glGetQueryObjectiv(time_query,
			GL_QUERY_RESULT_AVAILABLE,
			&done);
	}
	glGetQueryObjectui64v(time_query, GL_QUERY_RESULT, &elapsed_time);
	timeStagesTotal[stat] += elapsed_time;
	timeStagesAVG[stat] = (float)timeStagesTotal[stat] / (float)frameCount;

	timeStagesMin[stat] = glm::min(timeStagesMin[stat], elapsed_time);
	timeStagesMax[stat] = glm::max(timeStagesMax[stat], elapsed_time);
}

void Simulation::initComputeProgs() {
	prog_ppd1_externalForces = initComputeProg("../shaders/cloth_pbd1_externalForces.comp.glsl");
	glUseProgram(prog_ppd1_externalForces);
	glUniform3fv(1, 1, &Gravity[0]);
	glUniform1f(0, timeStep);

	prog_ppd2_dampVelocity = initComputeProg("../shaders/cloth_pbd2_dampVelocities.comp.glsl");

	prog_ppd3_predictPositions = initComputeProg("../shaders/cloth_pbd3_predictPositions.comp.glsl");
	glUseProgram(prog_ppd3_predictPositions);
	glUniform1f(0, timeStep);

	prog_ppd4_updateInvMass = initComputeProg("../shaders/cloth_pbd4_updateInverseMasses.comp.glsl");

	prog_ppd6_projectClothConstraints = initComputeProg("../shaders/cloth_pbd5_projectClothConstraints.comp.glsl");
	glUseProgram(prog_ppd6_projectClothConstraints);
	glUniform1f(0, (float) projectTimes);

	prog_ppd7_updateVelPos = initComputeProg("../shaders/cloth_pbd6_updatePositionsVelocities.comp.glsl");
	glUseProgram(prog_ppd7_updateVelPos);
	glUniform1f(0, timeStep);

	
	prog_copyBuffer = initComputeProg("../shaders/copy.comp.glsl");

	prog_genCollisionConstraints = initComputeProg("../shaders/cloth_genCollisions.comp.glsl");

	prog_projectCollisionConstraints = initComputeProg("../shaders/cloth_projectCollisions.comp.glsl");
	glUseProgram(prog_projectCollisionConstraints);
	glUniform1f(1, collisionBounceFactor);

	prog_rigidbodyAnimate = initComputeProg("../shaders/rigidbody_animate.comp.glsl");
}

void Simulation::genCollisionConstraints(Cloth *cloth, Rbody *rbody) {
	int numVertices = cloth->initPositions.size();
	glUseProgram(prog_genCollisionConstraints);
	glUniform1i(0, rbody->indicesTris.size() / 3);
	glUniform1i(1, numVertices);
	glUniform1f(2, cloth->default_static_constraint_bounce);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred2);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, rbody->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, rbody->ssbo_triangles);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, cloth->ssbo_collisionConstraints);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, cloth->ssbo_debug);

	int workGroupCount_vertices = (numVertices - 1) / WORK_GROUP_SIZE + 1;
	glDispatchCompute(workGroupCount_vertices, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	//retrieveBuffer(cloth->ssbo_debug, 121);
}

void Simulation::stepSingleCloth(Cloth *cloth) {
	int numVertices = cloth->initPositions.size();
	int workGroupCount_vertices = (numVertices - 1) / WORK_GROUP_SIZE + 1;

	/* compute new velocities with external forces */
	glUseProgram(prog_ppd1_externalForces);
	glUniform1i(2, numVertices);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	glDispatchCompute(workGroupCount_vertices, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // emulate ssbo memory coherence

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
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	/* update inverse masses */
	int numPinConstraints = cloth->externalConstraints.size();
	int workGroupCountPinConstraints = (numPinConstraints - 1) / WORK_GROUP_SIZE + 1;
	glUseProgram(prog_ppd4_updateInvMass);
	glUniform1i(0, numPinConstraints);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos_pred1);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred2);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_externalConstraints);
	glDispatchCompute(workGroupCountPinConstraints, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

#if QUERY_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, time_query);
#endif

	/* project cloth constraints N times */
	for (int i = 0; i < projectTimes; i++) {
		glUseProgram(prog_ppd6_projectClothConstraints);
		// project each of the 4 internal constraints
		// bind predicted positions input/output
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos_pred1);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred2);
		glUniform1f(2, cloth->default_internal_K); // uniform K
		glUniform1i(3, cloth->ssbo_pos_pred1); // send the identity of the influencing SSBO

		for (int j = 0; j < cloth->numInternalConstraintBuffers; j++) {
			// bind inner constraints
			int workGroupCountInnerConstraints = 
				(cloth->internalConstraints[j].size() - 1) / WORK_GROUP_SIZE + 1;
			glUniform1i(1, cloth->internalConstraints[j].size());

			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_internalConstraints[j]);
			// project this set of constraints
			glDispatchCompute(workGroupCountInnerConstraints, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		}

		// project pin constraints
		int numPinnedSSBOs = cloth->pinnedSSBOs.size();
		for (int i = 0; i < numPinnedSSBOs; i++) {
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->pinnedSSBOs.at(i)); // init positions, not pred 1
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred2); // update this
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_externalConstraints);
			glUniform1i(1, cloth->externalConstraints.size());
			glUniform1f(2, cloth->default_pin_K); // uniform K
			glUniform1i(3, (int)cloth->pinnedSSBOs.at(i)); // send the identity of the influencing SSBO
			glDispatchCompute(workGroupCountPinConstraints, 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		// ffwd pred1 to match pred2
		glUseProgram(prog_copyBuffer); // TODO: lol... THIS IS DUMB DO SOMETHING BETTER
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos_pred2);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred1);
		glDispatchCompute(workGroupCount_vertices, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	}

#if QUERY_PERFORMANCE
	updateStat(PROJ_CONSTRAINTS);
#endif

#if QUERY_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, time_query);
#endif

	/* generate and resolve collision constraints */
	for (int i = 0; i < numRigids; i++) {
		genCollisionConstraints(cloth, rigids.at(i));
	}

#if QUERY_PERFORMANCE
	updateStat(GENER_COLLISIONS);
#endif

#if DEBUG_VERBOSE
	cout << "init ";
	retrieveBuffer(cloth->ssbo_pos, 1);
	cout << "pred befor ";
	retrieveBuffer(cloth->ssbo_pos_pred2, 1);
#endif

#if QUERY_PERFORMANCE
	glBeginQuery(GL_TIME_ELAPSED, time_query);
#endif

	glUseProgram(prog_projectCollisionConstraints);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos_pred2);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_collisionConstraints);
	glUniform1i(0, numVertices);
	glDispatchCompute(workGroupCount_vertices, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

#if QUERY_PERFORMANCE
	updateStat(RESOL_COLLISIONS);
#endif

#if DEBUG_VERBOSE
	cout << "pred after ";
	retrieveBuffer(cloth->ssbo_pos_pred2, 1);
	cout << "con ";
	retrieveBuffer(cloth->ssbo_collisionConstraints, 1);
#endif
	//retrieveBuffer(cloth->ssbo_collisionConstraints, 1);
	/* update positions and velocities, reset collision constraints */

	glUseProgram(prog_ppd7_updateVelPos);
	glUniform1i(1, numVertices);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cloth->ssbo_vel);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cloth->ssbo_pos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cloth->ssbo_pos_pred2);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cloth->ssbo_collisionConstraints);
	glDispatchCompute(workGroupCount_vertices, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

#if DEBUG_VERBOSE
	cout << "vel ";
	retrieveBuffer(cloth->ssbo_vel, 1);
	cout << endl;
#endif
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
		if (positions.at(i).w > 0.0001f && ((int)positions.at(i).w % 2 || positions.at(i).w > 2.0f))
			cout << positions.at(i).x << " " << positions.at(i).y << " " << positions.at(i).z << " " << positions.at(i).w << endl;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void Simulation::animateRbody(Rbody *rbody) {
	if (rbody->animated == false) return;
	glm::mat4 tf = rbody->getTransformationAtTime(currentTime);
	int numVertices = rbody->initPositions.size();
	int workGroupCount_vertices = (numVertices - 1) / WORK_GROUP_SIZE + 1;

	glUseProgram(prog_rigidbodyAnimate);
	glUniform1i(0, numVertices);
	glUniformMatrix4fv(1, 1, GL_FALSE, &tf[0][0]);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, rbody->ssbo_initPos);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rbody->ssbo_pos);
	glDispatchCompute(workGroupCount_vertices, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Simulation::stepSimulation() {
	frameCount++;
	for (int i = 0; i < numRigids; i++) {
		animateRbody(rigids.at(i));
	}

	for (int i = 0; i < numCloths; i++) {
		stepSingleCloth(cloths.at(i));
	}
	currentTime += timeStep;

#if QUERY_PERFORMANCE
	// report performance every 360 frames
	if (frameCount % 360 == 0) {
		cout << "performance as of frame " << frameCount << endl;
		cout << "average times (microseconds)" << endl;
		cout << "solving internal constraints: " <<
			(float)timeStagesAVG[PROJ_CONSTRAINTS] / 1000.0f << endl;
		cout << "generating collision constraints: " <<
			(float)timeStagesAVG[GENER_COLLISIONS] / 1000.0f << endl;
		cout << "resolving collision constraints: " <<
			(float)timeStagesAVG[RESOL_COLLISIONS] / 1000.0f << endl;

		cout << "max times (microseconds)" << endl;
		cout << "solving internal constraints: " <<
			(float)timeStagesMax[PROJ_CONSTRAINTS] / 1000.0f << endl;
		cout << "generating collision constraints: " <<
			(float)timeStagesMax[GENER_COLLISIONS] / 1000.0f << endl;
		cout << "resolving collision constraints: " <<
			(float)timeStagesMax[RESOL_COLLISIONS] / 1000.0f << endl;

		cout << "min times (microseconds)" << endl;
		cout << "solving internal constraints: " <<
			(float)timeStagesMin[PROJ_CONSTRAINTS] / 1000.0f << endl;
		cout << "generating collision constraints: " <<
			(float)timeStagesMin[GENER_COLLISIONS] / 1000.0f << endl;
		cout << "resolving collision constraints: " <<
			(float)timeStagesMin[RESOL_COLLISIONS] / 1000.0f << endl;
	}
#endif

}

void Simulation::selectByRaycast(glm::vec3 eye, glm::vec3 dir) {
	cout << dir.x << " " << dir.y << " " << dir.z << endl;
}
