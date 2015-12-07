#pragma once
#include <vector>
#include "mesh.hpp"
#include "cloth.hpp"
#include "rbody.hpp"
#include "glslUtility.hpp"

using namespace std;

class Simulation
{
public:
	Simulation(vector<string> &body_filenames, vector<string> &cloth_filenames);
	~Simulation();

	vector<Rbody*> rigids;
	vector<Cloth*> cloths;
	int numRigids;
	int numCloths;

	int projectTimes = 10;
	float timeStep = 0.016;
	float K = 0.9f;
	glm::vec3 Gravity = glm::vec3(0.0f, 0.0f, -0.98f);

	GLuint unif_numPoints;
	GLuint ssbo_addtl_constraints;

	GLuint prog_ppd1_externalForces;
	GLuint prog_ppd2_dampVelocity;
	GLuint prog_ppd3_predictPositions;
	GLuint prog_ppd4_projectClothConstraints;
	GLuint prog_ppd6_updateVelPos;
	GLuint prog_copyBuffer; // TODO: lol

	void initComputeProgs();
	void stepSingleCloth(Cloth *cloth);
	void stepSimulation();

	void retrieveBuffer(GLuint ssbo, int numItems);
};