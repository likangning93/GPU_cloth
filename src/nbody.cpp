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

// Universal gravitation constant.
const float G = 6.67384e-11;
// Mass of the "star" at the center.
const float starMass = 5e10;

/*! Size of the starting area in simulation space. */
const float scene_scale = 1e2;

GLuint unif_numPlanets = 0;

static GLuint prog_nbody_acc;
static GLuint prog_nbody_velpos;
static GLuint ssbo_pos;
static GLuint ssbo_vel;
static GLuint ssbo_acc;


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
    prog_nbody_acc    = initComputeProg("shaders/nbody_acc.comp.glsl");
    prog_nbody_velpos = initComputeProg("shaders/nbody_velpos.comp.glsl");
	
    glUseProgram(prog_nbody_acc);
    glUniform1i(unif_numPlanets, N_FOR_VIS);

    glUseProgram(prog_nbody_velpos);
    glUniform1i(unif_numPlanets, N_FOR_VIS);
}

glm::vec3 generateRandomVec3() {
    return glm::vec3(
            rand() * 2.0f / (float) RAND_MAX - 1.0f,
            rand() * 2.0f / (float) RAND_MAX - 1.0f,
            rand() * 2.0f / (float) RAND_MAX - 1.0f);
}

void initSimulation() {
    glGenBuffers(1, &ssbo_pos);
    glGenBuffers(1, &ssbo_vel);
    glGenBuffers(1, &ssbo_acc);

    GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

    // Randomize planet positions

    glm::vec3 *hst_pos = new glm::vec3[N_FOR_VIS];
    for (int i = 0; i < N_FOR_VIS; i++) {
        glm::vec3 r = generateRandomVec3();
        hst_pos[i].x = scene_scale * r.x;
        hst_pos[i].y = scene_scale * r.y;
        hst_pos[i].z = scene_scale * r.z * sqrt(r.x * r.x + r.y * r.y) * 0.1;
    }

    // Initialize planet positions on GPU

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pos);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N_FOR_VIS * sizeof(glm::vec3),
            NULL, GL_STREAM_COPY);
    glm::vec3 *pos = (glm::vec3 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
            0, N_FOR_VIS * sizeof(glm::vec3), bufMask);
    for (int i = 0; i < N_FOR_VIS; i++) {
        pos[i] = hst_pos[i];
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    // Initialize planet velocities on GPU

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vel);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N_FOR_VIS * sizeof(glm::vec3),
            NULL, GL_STREAM_COPY);

    glm::vec3 *vel = (glm::vec3 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
            0, N_FOR_VIS * sizeof(glm::vec3), bufMask);
    for (int i = 0; i < N_FOR_VIS; i++) {
        glm::vec3 R = hst_pos[i];

        float r = glm::length(R) + 0.00001;
        float s = sqrt(G * starMass / r);
        glm::vec3 D = glm::normalize(glm::cross(R / r, glm::vec3(0, 0, 1)));
        vel[i] = s * D;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    // Allocate planet accelerations on GPU

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_acc);
    glBufferData(GL_SHADER_STORAGE_BUFFER, N_FOR_VIS * sizeof(glm::vec3),
            NULL, GL_STREAM_COPY);

    delete[] hst_pos;
}

void stepSimulation() {
    // Acceleration step

    glUseProgram(prog_nbody_acc);
    // Bind nbody-acc input
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_pos);
    // Bind nbody-acc output
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_acc);

    // Dispatch compute shader
    int workGroupCountAcc = (N_FOR_VIS - 1) / WORK_GROUP_SIZE_ACC + 1;
    glDispatchCompute(workGroupCountAcc, 1, 1);

    // Velocity/position update step

    glUseProgram(prog_nbody_velpos);
    // Bind nbody-velpos input
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_acc);
    // Bind nbody-velpos input/outputs
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_pos);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_vel);

    // Dispatch compute shader
    int workGroupCountVelPos = (N_FOR_VIS - 1) / WORK_GROUP_SIZE_VELPOS + 1;
    glDispatchCompute(workGroupCountVelPos, 1, 1);
}

GLuint getSSBOPosition() {
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    return ssbo_pos;
}
