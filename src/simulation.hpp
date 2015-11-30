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

	GLuint unif_numPoints;
	GLuint ssbo_addtl_constraints;

	GLuint prog_externalForces;
	GLuint prog_internalForces;
	GLuint prog_velpos;

	void initComputeProgs();
	void stepSimulation();
};