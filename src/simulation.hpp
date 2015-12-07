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
	glm::vec3 Gravity = glm::vec3(0.0f, 0.0f, -0.98f);

	GLuint prog_ppd1_externalForces;
	GLuint prog_ppd2_dampVelocity;
	GLuint prog_ppd3_predictPositions;
	GLuint prog_ppd4_updateInvMass; // updates inverse masses for pinned vertices

	GLuint prog_ppd6_projectClothConstraints;
	GLuint prog_ppd7_updateVelPos;
	GLuint prog_copyBuffer; // TODO: lol
	
	GLuint prog_genCollisionConstraints;
	GLuint prog_projectCollisionConstraints;

	void initComputeProgs();
	void genCollisionConstraints(Cloth *cloth, Rbody *rbody);
	void stepSingleCloth(Cloth *cloth);
	void stepSimulation();

	void retrieveBuffer(GLuint ssbo, int numItems);
};