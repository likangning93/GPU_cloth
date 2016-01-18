#pragma once

#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <fstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "utilityCore.hpp"
#include "glslUtility.hpp"
#include "mesh.hpp"

//====================================
// GL Stuff
//====================================

GLuint attr_position = 0;
const char *attributeLocations[] = { "Position" };
GLuint displayImage;
GLuint program[2];

const unsigned int PROG_CLOTH = 0; // program for rendering cloth
const unsigned int PROG_WIRE = 1; // program for rendering wireframes, like for raycasting

const float fovy = (float) (PI / 4);
const float zNear = 0.10f;
const float zFar = 100.0f;

glm::mat4 projection;
float theta = 1.22f;
float phi = -0.65f;
float zoom = 5.0f;
glm::vec3 lookAt = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraPosition;

bool pause = true;
bool stepFrames = false;

//====================================
// Main
//====================================

const char *projectName;

int main(int argc, char* argv[]);

//====================================
// Main loop
//====================================
void mainLoop();
void drawMesh(Mesh *drawMe);
void errorCallback(int error, const char *description);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void clickCallback(GLFWwindow* window, int button, int action, int mods);
void updateCamera();
//glm::vec3 rayCast(int x, int y, int width, int height);
//void drawRaycast();

//====================================
// Setup/init Stuff
//====================================
bool init(int argc, char **argv);
void initVAO();
void initShaders(GLuint *program);

//====================================
// room for different simualtions and
// gl unit tests
//====================================
glm::vec3 closestPointOnTriangle(glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 P);
void runTests(); // for trying gl stuff

void loadPerformanceTests();
void loadDancingBear(); // the default sim
void loadStaticCollDetectDebug(); // ball and cloth. debugging.
void loadStaticCollResolveDebug(); // floor and cloth. debugging.