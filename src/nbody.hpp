#pragma once
#include <vector>
#include "mesh.hpp"
#include "cloth.hpp"
#include "rbody.hpp"

//std::vector<Rbody*> rigidBodies;
//std::vector<Cloth*> cloths;

const int N_WIDE = 20;
const int N_LENGTH = 20;

const int N_CONSTRAINTS = (20 - 1) * (20 - 1) * 6 * 2 + 4;

void initComputeProgs();
void initSimulation();
void stepSimulation();
unsigned int getSSBOPosition();
