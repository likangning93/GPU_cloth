#pragma once
#include <vector>
#include "mesh.hpp"
#include "cloth.hpp"
#include "rbody.hpp"
#include "glslUtility.hpp"

using namespace std;

class Simulation
{
private:
	GLuint time_query;
	GLuint64 elapsed_time;
	GLuint64 frameCount;
	GLuint64 timeStagesTotal[3]; // ns
	float timeStagesAVG[3]; // ns
	GLuint64 timeStagesMin[3]; // ns
	GLuint64 timeStagesMax[3]; // ns

	void updateStat(int stat);

public:
	Simulation(vector<string> &body_filenames, vector<string> &cloth_filenames);
	~Simulation();

	vector<Rbody*> rigids;
	vector<Cloth*> cloths;
	int numRigids;
	int numCloths;

	int projectTimes = 10;
	float timeStep = 0.016f;
	float currentTime = 0.0f;
	glm::vec3 Gravity = glm::vec3(0.0f, 0.0f, -0.98f);

	float collisionBounceFactor = 0.2f;

	GLuint prog_ppd1_externalForces;
	GLuint prog_ppd2_dampVelocity;
	GLuint prog_ppd3_predictPositions;
	GLuint prog_ppd4_updateInvMass; // updates inverse masses for pinned vertices

	GLuint prog_ppd6_projectClothConstraints;
	GLuint prog_ppd7_updateVelPos;
	GLuint prog_copyBuffer; // TODO: lol
	
	GLuint prog_genCollisionConstraints;
	GLuint prog_projectCollisionConstraints;

	GLuint prog_rigidbodyAnimate;

	void initComputeProgs();
	void genCollisionConstraints(Cloth *cloth, Rbody *rbody);
	void stepSingleCloth(Cloth *cloth);
	void stepSimulation();

	void animateRbody(Rbody *rbody);

	void selectByRaycast(glm::vec3 eye, glm::vec3 dir);

	void retrieveBuffer(GLuint ssbo, int numItems);
};