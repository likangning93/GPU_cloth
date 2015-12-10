/**
 * @file      main.cpp
 * @brief     Example N-body simulation for CIS 565
 * @authors   Liam Boone, Kai Ninomiya
 * @date      2013-2015
 * @copyright University of Pennsylvania
 */
#include "main.hpp"
#include "simulation.hpp"
#include "checkGLError.hpp"

// ================
// Configuration
// ================

#define VISUALIZE 1

const float DT = 0.2f;

Simulation *sim = NULL;

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

    // Create and setup VAO for drawing
	glGenVertexArrays(1, &drawingVAO);
	glBindVertexArray(drawingVAO);
    glEnableVertexAttribArray(attr_position);
    glVertexAttribPointer((GLuint) attr_position, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindVertexArray(0);

	glm::vec3 cameraPosition;
	cameraPosition.x = zoom * sin(phi) * sin(theta);
	cameraPosition.z = zoom * cos(theta);
	cameraPosition.y = zoom * cos(phi) * sin(theta);

    projection = glm::perspective(fovy, float(width) / float(height), zNear, zFar);
    glm::mat4 view = glm::lookAt(cameraPosition, glm::vec3(0), glm::vec3(0, 0, 1));

    projection = projection * view;

    initShaders(program);

	// Initialize simulation
	std::vector<string> colliders;
	//colliders.push_back("meshes/floor.obj");
	colliders.push_back("meshes/low_poly_bear.obj");
	//colliders.push_back("meshes/semi_smooth_cube.obj");

	std::vector<string> cloths;
	//cloths.push_back("meshes/floor.obj");
	//cloths.push_back("meshes/20x20cloth.obj");
	//cloths.push_back("meshes/3x3cloth.obj");
	cloths.push_back("meshes/bear_cloth.obj");
	//cloths.push_back("meshes/dress.obj");

	sim = new Simulation(colliders, cloths);
	checkGLError("init sim");

    glEnable(GL_DEPTH_TEST);

    return true;
}

void initShaders(GLuint * program) {
    GLint location;

	program[PROG_CLOTH] = glslUtility::createProgram(
		"shaders/cloth.vert.glsl",
		"shaders/cloth.geom.glsl",
		"shaders/cloth.frag.glsl", attributeLocations, 1);
	glUseProgram(program[PROG_CLOTH]);

	if ((location = glGetUniformLocation(program[PROG_CLOTH], "u_projMatrix")) != -1) {
		glUniformMatrix4fv(location, 1, GL_FALSE, &projection[0][0]);
	}

	checkGLError("init shaders");
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

		if (!pause) {
			sim->stepSimulation();
			checkGLError("sim");
		}

#if VISUALIZE
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.5f, 0.7f, 1.0f, 1.0f);

		// draw all the meshes
		for (int i = 0; i < sim->numRigids; i++) {
			drawMesh(sim->rigids.at(i));
		}
		for (int i = 0; i < sim->numCloths; i++) {
			drawMesh(sim->cloths.at(i));
		}
		glfwSwapBuffers(window);
		if (stepFrames)
			pause = true;
#endif
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}

void drawMesh(Mesh *drawMe) {

  glUseProgram(program[PROG_CLOTH]);
  glBindVertexArray(drawingVAO);

  // upload color uniform
  GLint location;
  if ((location = glGetUniformLocation(program[PROG_CLOTH], "u_color")) != -1) {
	  glUniform3fv(location, 1, &drawMe->color[0]);
  }

  // Tell the GPU where the indices are: in the index buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawMe->idxbo);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, drawMe->ssbo_pos);

  // Draw the elements.

  glDrawElements(GL_TRIANGLES, drawMe->indicesTris.size(), GL_UNSIGNED_INT, 0);

  checkGLError("visualize");
}

void errorCallback(int error, const char *description) {
    fprintf(stderr, "error %d: %s\n", error, description);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
		return;
    }
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		pause = !pause;
		return;
	}
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		phi += 0.1f;
	}
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		phi -= 0.1f;
	}
	if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		theta -= 0.1f;
	}
	if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		theta += 0.1f;
	}
	if (key == GLFW_KEY_W && action == GLFW_PRESS) {
		lookAt.x += 0.5f;
	}
	if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		lookAt.x -= 0.5f;
	}
	if (key == GLFW_KEY_A && action == GLFW_PRESS) {
		lookAt.y += 0.5f;
	}
	if (key == GLFW_KEY_D && action == GLFW_PRESS) {
		lookAt.y -= 0.5f;
	}
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		lookAt.z -= 0.5f;
	}
	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		lookAt.z += 0.5f;
	}
	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		zoom -= 0.5f;
	}
	if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		zoom += 0.5f;
	}
	if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		stepFrames = !stepFrames;
	}
	updateCamera();
}

void updateCamera() {
	glm::vec3 cameraPosition;
	cameraPosition.x = zoom * sin(phi) * sin(theta);
	cameraPosition.z = zoom * cos(theta);
	cameraPosition.y = zoom * cos(phi) * sin(theta);
	cameraPosition += lookAt;

	int width = 1280;
	int height = 720;
	projection = glm::perspective(fovy, float(width) / float(height), zNear, zFar);
	glm::mat4 view = glm::lookAt(cameraPosition, lookAt, glm::vec3(0, 0, 1));
	projection = projection * view;

	GLint location;

	glUseProgram(program[PROG_CLOTH]);
	if ((location = glGetUniformLocation(program[PROG_CLOTH], "u_projMatrix")) != -1) {
		glUniformMatrix4fv(location, 1, GL_FALSE, &projection[0][0]);
	}
}