/**
 * @file      main.cpp
 * @brief     Example N-body simulation for CIS 565
 * @authors   Liam Boone, Kai Ninomiya
 * @date      2013-2015
 * @copyright University of Pennsylvania
 */

#include "main.hpp"
#include "nbody.hpp"
#include "checkGLError.hpp"

// ================
// Configuration
// ================

#define VISUALIZE 1

const float DT = 0.2f;

/**
 * C main function.
 */
int main(int argc, char* argv[]) {
    projectName = "565 Compute Shader Intro: N-Body";

    if (init(argc, argv)) {
        mainLoop();
        return 0;
    } else {
        return 1;
    }
}

//-------------------------------
//---------RUNTIME STUFF---------
//-------------------------------

std::string deviceName;
GLFWwindow *window;

/**
 * Initialization of GLFW.
 */
bool init(int argc, char **argv) {
    // Window setup stuff
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        std::cout
            << "Error: Could not initialize GLFW!"
            << " Perhaps OpenGL 3.3 isn't available?"
            << std::endl;
        return false;
    }
    int width = 1280;
    int height = 720;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, deviceName.c_str(), NULL, NULL);
    if (!window) {
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, keyCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        return false;
    }
    glGetError();

    // Create and setup VAO
    glGenVertexArrays(1, &planetVAO);
    glBindVertexArray(planetVAO);
    glEnableVertexAttribArray(attr_position);
    glVertexAttribPointer((GLuint) attr_position, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindVertexArray(0);

    // Initialize n-body simulation
    
	initComputeProgs();

	initSimulation();

	glm::vec3 cameraPosition;
	cameraPosition.x = zoom * sin(phi) * sin(theta);
	cameraPosition.z = zoom * cos(theta);
	cameraPosition.y = zoom * cos(phi) * sin(theta);

    projection = glm::perspective(fovy, float(width) / float(height), zNear, zFar);
    glm::mat4 view = glm::lookAt(cameraPosition, glm::vec3(0), glm::vec3(0, 0, 1));

    projection = projection * view;

    initShaders(program);

    glEnable(GL_DEPTH_TEST);

    return true;
}

void initShaders(GLuint * program) {
    GLint location;

    program[PROG_PLANET] = glslUtility::createProgram(
                     "shaders/planet.vert.glsl",
                     "shaders/planet.geom.glsl",
                     "shaders/planet.frag.glsl", attributeLocations, 1);
    glUseProgram(program[PROG_PLANET]);

    if ((location = glGetUniformLocation(program[PROG_PLANET], "u_projMatrix")) != -1) {
        glUniformMatrix4fv(location, 1, GL_FALSE, &projection[0][0]);
    }
    //if ((location = glGetUniformLocation(program[PROG_PLANET], "u_cameraPos")) != -1) {
    //    glUniform3fv(location, 1, &cameraPosition[0]);
    //}
}

//====================================
// Main loop
//====================================

void mainLoop() {
    double fps = 0;
    double timebase = 0;
    int frame = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        frame++;
        double time = glfwGetTime();

        if (time - timebase > 1.0) {
            fps = frame / (time - timebase);
            timebase = time;
            frame = 0;
        }
        std::ostringstream ss;
        ss << "[";
        ss.precision(1);
        ss << std::fixed << fps;
        ss << " fps] " << deviceName;
        glfwSetWindowTitle(window, ss.str().c_str());

        stepSimulation();

#if VISUALIZE
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPointSize(2.0f);

        glUseProgram(program[PROG_PLANET]);

        glBindVertexArray(planetVAO);

        GLuint ssbo = getSSBOPosition();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

        glDrawArrays(GL_POINTS, 0, N_LENGTH * N_WIDE + 2);

        glPointSize(1.0f);
        glUseProgram(0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        checkGLError("visualize");
#endif
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}


void errorCallback(int error, const char *description) {
    fprintf(stderr, "error %d: %s\n", error, description);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		phi += 0.1f;
		updateCamera();
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		phi -= 0.1f;
		updateCamera();
	}
	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		theta -= 0.1f;
		updateCamera();
	}
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		theta += 0.1f;
		updateCamera();
	}
}

void updateCamera() {
	glm::vec3 cameraPosition;
	cameraPosition.x = zoom * sin(phi) * sin(theta);
	cameraPosition.z = zoom * cos(theta);
	cameraPosition.y = zoom * cos(phi) * sin(theta);

	int width = 1280;
	int height = 720;
	projection = glm::perspective(fovy, float(width) / float(height), zNear, zFar);
	glm::mat4 view = glm::lookAt(cameraPosition, glm::vec3(0), glm::vec3(0, 0, 1));
	projection = projection * view;
	glUseProgram(program[PROG_PLANET]);

	GLint location;
	if ((location = glGetUniformLocation(program[PROG_PLANET], "u_projMatrix")) != -1) {
		glUniformMatrix4fv(location, 1, GL_FALSE, &projection[0][0]);
	}
}