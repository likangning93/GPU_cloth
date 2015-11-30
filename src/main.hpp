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

//====================================
// GL Stuff
//====================================

GLuint attr_position = 0;
const char *attributeLocations[] = { "Position" };
GLuint planetVAO = 0;
GLuint planetVBO = 0;
GLuint planetIBO = 0;
GLuint displayImage;
GLuint program[2];

const unsigned int PROG_PLANET = 0; // program for renderign planets. holdover from nbody
const unsigned int PROG_CLOTH = 1; // program for rendering cloth

const float fovy = (float) (PI / 4);
const float zNear = 0.10f;
const float zFar = 10.0f;

glm::mat4 projection;
float theta = 1.22f;
float phi = 0.65f;
float zoom = 2.0f;

//====================================
// Main
//====================================

const char *projectName;

int main(int argc, char* argv[]);

//====================================
// Main loop
//====================================
void mainLoop();
void drawMesh();
void errorCallback(int error, const char *description);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void updateCamera();

//====================================
// Setup/init Stuff
//====================================
bool init(int argc, char **argv);
void initVAO();
void initShaders(GLuint *program);
