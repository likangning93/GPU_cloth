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

const unsigned int PROG_PLANET = 0;

const float fovy = (float) (PI / 4);
const float zNear = 0.10f;
const float zFar = 5.0f;

glm::mat4 projection;
glm::vec3 cameraPosition(1.75, 1.75, 1.35);

//====================================
// Main
//====================================

const char *projectName;

int main(int argc, char* argv[]);

//====================================
// Main loop
//====================================
void mainLoop();
void errorCallback(int error, const char *description);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

//====================================
// Setup/init Stuff
//====================================
bool init(int argc, char **argv);
void initVAO();
void initShaders(GLuint *program);
