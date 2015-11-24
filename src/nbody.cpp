#include "nbody.hpp"
#include <cstdlib>
#include <cstdio>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "glslUtility.hpp"


// TODO: perform timing experiments, based on work group size.
// You must ALSO change the work group size in the compute shaders.
#define WORK_GROUP_SIZE_ACC 16
#define WORK_GROUP_SIZE_VELPOS 16

GLuint unif_numPoints = 0;

static GLuint ssbo_pos;
static GLuint ssbo_vel;
static GLuint ssbo_force;
static GLuint ssbo_constraints;

static GLuint prog_externalForces;
static GLuint prog_internalForces;
static GLuint prog_velpos;

static GLuint initComputeProg(const char *path) {
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

void initComputeProgs() {
    prog_externalForces = initComputeProg("shaders/cloth_externalForces.comp.glsl");
	prog_internalForces = initComputeProg("shaders/cloth_internalForces.comp.glsl");
    prog_velpos = initComputeProg("shaders/cloth_velpos.comp.glsl");

	glUseProgram(prog_velpos);
	glUniform1i(unif_numPoints, N_WIDE * N_LENGTH + 2);
}

glm::vec3 generateRandomVec3() {
    return glm::vec3(
            rand() * 2.0f / (float) RAND_MAX - 1.0f,
            rand() * 2.0f / (float) RAND_MAX - 1.0f,
            rand() * 2.0f / (float) RAND_MAX - 1.0f);
}

glm::vec3 genConstraint(int idx1, int idx2, glm::vec3 *points) {
	glm::vec3 p1 = points[idx1];
	glm::vec3 p2 = points[idx2];
	return glm::vec3(idx1, idx2, glm::length(p1 - p2));
}

void initSimulation() {
	// go make some gl buffers
    glGenBuffers(1, &ssbo_pos);
    glGenBuffers(1, &ssbo_vel);
	glGenBuffers(1, &ssbo_force);
    glGenBuffers(1, &ssbo_constraints);

    GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

    // build a neat grid of points

	float width = 0.8f;
	float length = 0.8f;

	glm::vec3 corner = glm::vec3(-width / 2.0f, -length / 2.0f, 0.0);
	float dwidth = width / (float)N_WIDE;
	float dlength = length / (float)N_LENGTH;

	int N_FOR_VIS = N_WIDE * N_LENGTH + 2;

    glm::vec3 *hst_pos = new glm::vec3[N_FOR_VIS];
	for (int x = 0; x < N_WIDE; x++) {
		for (int z = 0; z < N_LENGTH; z++) {
			int i = x * N_LENGTH + z;
			hst_pos[i] = corner;
			hst_pos[i].y += z * dlength;
			hst_pos[i].x += x * dwidth;
		}
    }

	// add the "constraint" corners to the end.
	// TODO: add a way to specify constraints
	hst_pos[N_FOR_VIS - 2] = corner;
	hst_pos[N_FOR_VIS - 1] = corner;
	hst_pos[N_FOR_VIS - 1].x += (N_WIDE - 1) * dwidth;

	// generate constraints
	// 1--2--x
	// |\/|\/|
	// |/\|/\|
	// 4--3--x
	// |\/|\/|
	// |/\|/\|
	// x--x--x
	// num constraints generated at each step is 6 * 2

	glm::vec3 *hst_constraints = new glm::vec3[N_CONSTRAINTS];
	int constraintCounter = 0;

	for (int x = 0; x < N_WIDE - 1; x++) {
		for (int z = 0; z < N_LENGTH - 1; z++) {
			int p1 = x * N_LENGTH + z;
			int p2 = x * N_LENGTH + z + 1;
			int p3 = (x + 1) * N_LENGTH + z + 1;
			int p4 = (x + 1) * N_LENGTH + z;

			printf("%i %i %i %i\n", p1, p2, p3, p4);

			// a constraint as a vec3 is index, index, rest length

			// p1--p2: stretch constraint
			hst_constraints[constraintCounter + 0] = genConstraint(p1, p2, hst_pos);
			hst_constraints[constraintCounter + 1] = genConstraint(p2, p1, hst_pos);

			// p2--p3: stretch constraint
			hst_constraints[constraintCounter + 2] = genConstraint(p2, p3, hst_pos);
			hst_constraints[constraintCounter + 3] = genConstraint(p3, p2, hst_pos);

			// p3--p4: stretch constraint
			hst_constraints[constraintCounter + 4] = genConstraint(p3, p4, hst_pos);
			hst_constraints[constraintCounter + 5] = genConstraint(p4, p3, hst_pos);

			// p4--p1: stretch constraint
			hst_constraints[constraintCounter + 6] = genConstraint(p4, p1, hst_pos);
			hst_constraints[constraintCounter + 7] = genConstraint(p1, p4, hst_pos);

			// p1--p3: fake bend constraint
			hst_constraints[constraintCounter + 8] = genConstraint(p1, p3, hst_pos);
			hst_constraints[constraintCounter + 9] = genConstraint(p3, p1, hst_pos);

			// p2--p4: fake bend constraint
			hst_constraints[constraintCounter + 10] = genConstraint(p2, p4, hst_pos);
			hst_constraints[constraintCounter + 11] = genConstraint(p4, p2, hst_pos);

			constraintCounter += 12;
		}
	}
	// add constraints for the 2 attachment points
	int p1 = 0;
	int p2 = N_FOR_VIS - 2;
	int p3 = (N_WIDE - 1) * N_LENGTH;
	int p4 = N_FOR_VIS - 1;

	hst_constraints[constraintCounter + 0] = genConstraint(p1, p2, hst_pos);
	hst_constraints[constraintCounter + 1] = genConstraint(p2, p1, hst_pos);
	hst_constraints[constraintCounter + 2] = genConstraint(p3, p4, hst_pos);
	hst_constraints[constraintCounter + 3] = genConstraint(p4, p3, hst_pos);

	//glm::vec3 peekPoint1 = hst_pos[p3];
	//glm::vec3 peekPoint2 = hst_pos[p1];
	//
	//glm::vec3 peek1 = hst_constraints[constraintCounter + 0];
	//glm::vec3 peek2 = hst_constraints[constraintCounter + 1];
	//glm::vec3 peek3 = hst_constraints[constraintCounter + 2];
	//glm::vec3 peek4 = hst_constraints[constraintCounter + 3];

    // Initialize cloth positions on GPU

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pos);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N_FOR_VIS * sizeof(glm::vec4),
            NULL, GL_STREAM_COPY);
    glm::vec4 *pos = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
            0, N_FOR_VIS * sizeof(glm::vec4), bufMask);
    for (int i = 0; i < N_FOR_VIS; i++) {
        pos[i] = glm::vec4(hst_pos[i], 1.0);
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    // Initialize velocities on GPU

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vel);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N_FOR_VIS * sizeof(glm::vec4),
            NULL, GL_STREAM_COPY);

	// Initialize forces on GPU

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_force);
	glBufferData(GL_SHADER_STORAGE_BUFFER, N_FOR_VIS * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);

    // Allocate constraints on GPU

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_constraints);
	glBufferData(GL_SHADER_STORAGE_BUFFER, N_CONSTRAINTS * sizeof(glm::vec3),
            NULL, GL_STREAM_COPY);

	glm::vec3 *constraints = (glm::vec3 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, N_CONSTRAINTS * sizeof(glm::vec3), bufMask);
	for (int i = 0; i < N_CONSTRAINTS; i++) {
		constraints[i] = hst_constraints[i];
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    delete[] hst_pos;
	delete[] hst_constraints;
}

void stepSimulation() {
	int numVertices = N_WIDE * N_LENGTH + 2;
	int numConstraints = N_CONSTRAINTS;

	/*** compute external forces per vertex ***/

	glUseProgram(prog_externalForces);
	// Bind positions input
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_pos);
	// Bind force output
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_force);
	// Dispatch compute shader
	int workGroupCountExternal = (numVertices - 1) / WORK_GROUP_SIZE_ACC + 1;
	glDispatchCompute(workGroupCountExternal, 1, 1);
	
	/*** compute internal forces per constraint ***/

	glUseProgram(prog_internalForces);
	// Bind positions input
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_pos);
	// Bind force
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_force);
	// Bind constraints
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_constraints);
	// Dispatch compute shader
	int workGroupCountInternal = (numConstraints - 1) / WORK_GROUP_SIZE_ACC + 1;
	glDispatchCompute(workGroupCountInternal, 1, 1);

	// update velocities and positions. damp velocities
	glUseProgram(prog_velpos);
	// Bind force
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_force);
	// Bind positions input
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_pos);
	// Bind velocities
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_vel);

    // Dispatch compute shader
	int workGroupCountVelPos = (numVertices - 1) / WORK_GROUP_SIZE_VELPOS + 1;
    glDispatchCompute(workGroupCountVelPos, 1, 1);
}

GLuint getSSBOPosition() {
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    return ssbo_pos;
}
