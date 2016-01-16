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

int width = 1280;
int height = 720;

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
    
    // run some tests and unit tests
    runTests();

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
	glfwSetMouseButtonCallback(window, clickCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        return false;
    }
    glGetError();

	glm::vec3 cameraPosition;
	cameraPosition.x = zoom * sin(phi) * sin(theta);
	cameraPosition.z = zoom * cos(theta);
	cameraPosition.y = zoom * cos(phi) * sin(theta);

    projection = glm::perspective(fovy, float(width) / float(height), zNear, zFar);
    glm::mat4 view = glm::lookAt(cameraPosition, glm::vec3(0), glm::vec3(0, 0, 1));

    projection = projection * view;

    initShaders(program);
	
	//loadDancingBear();
	//loadStaticCollDetectDebug();
	loadStaticCollResolveDebug();

    glEnable(GL_DEPTH_TEST);

    return true;
}

void loadDancingBear() {
	std::vector<string> colliders;
	std::vector<string> cloths;
	// Initialize a fun simulation
	colliders.push_back("meshes/low_poly_bear.obj");
	colliders.push_back("meshes/floor.obj");

	cloths.push_back("meshes/cape.obj");
	cloths.push_back("meshes/dress.obj");
	cloths.push_back("meshes/bear_cloth.obj");

	sim = new Simulation(colliders, cloths);
	checkGLError("init sim");

	// let's generate some clothespins!
	int bearLeftShoulder = 779;
	int bearRightShoulder = 1578;
	int bearSSBO = sim->rigids.at(0)->ssbo_pos;
	sim->rigids.at(0)->color = glm::vec3(1.0f);
	sim->rigids.at(0)->animated = true;

	// SSBOs for cape
	int capeLeftShoulder = 1;
	int capeRightShoulder = 0;
	sim->cloths.at(0)->addPinConstraint(capeLeftShoulder, bearLeftShoulder, bearSSBO);
	sim->cloths.at(0)->addPinConstraint(capeRightShoulder, bearRightShoulder, bearSSBO);
	sim->cloths.at(0)->color = glm::vec3(0.5f, 1.0f, 1.0f);

	// SSBOs for dress
	int dressLeftShoulder = 14;
	int dressRightShoulder = 13;
	sim->cloths.at(1)->addPinConstraint(dressLeftShoulder, bearLeftShoulder, bearSSBO);
	sim->cloths.at(1)->addPinConstraint(dressRightShoulder, bearRightShoulder, bearSSBO);
	sim->cloths.at(1)->color = glm::vec3(1.0f, 0.5f, 0.5f);
}

void loadStaticCollDetectDebug() {
	std::vector<string> colliders;
	std::vector<string> cloths;
	cloths.push_back("meshes/perf/cloth_121.obj");
	colliders.push_back("meshes/perf/ball_98.obj");
	zoom = 10.0f;
	updateCamera();
	stepFrames = true;
	sim = new Simulation(colliders, cloths);
	checkGLError("init sim");
}

void loadStaticCollResolveDebug() {
	std::vector<string> colliders;
	std::vector<string> cloths;
	colliders.push_back("meshes/low_poly_bear.obj");
	colliders.push_back("meshes/floor.obj");

	cloths.push_back("meshes/bear_cloth.obj");

	sim = new Simulation(colliders, cloths);
	checkGLError("init sim");

	sim->rigids.at(0)->animated = false;
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

	program[PROG_WIRE] = glslUtility::createProgram(
		"shaders/wire.vert.glsl",
		"shaders/wire.geom.glsl",
		"shaders/wire.frag.glsl", attributeLocations, 1);
	glUseProgram(program[PROG_WIRE]);

	if ((location = glGetUniformLocation(program[PROG_WIRE], "u_projMatrix")) != -1) {
		glUniformMatrix4fv(location, 1, GL_FALSE, &projection[0][0]);
	}
	updateCamera();
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
  glBindVertexArray(drawMe->drawingVAO);

  // upload color uniform
  GLint location;
  if ((location = glGetUniformLocation(program[PROG_CLOTH], "u_color")) != -1) {
	  glUniform3fv(location, 1, &drawMe->color[0]);
  }

  // Tell the GPU where the positions are. haven't figured out how to bind to the VAO yet
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, drawMe->ssbo_pos);

  // Draw the elements.
  glDrawElements(GL_TRIANGLES, drawMe->indicesTris.size(), GL_UNSIGNED_INT, 0);

  //checkGLError("visualize");
}

void errorCallback(int error, const char *description) {
    fprintf(stderr, "error %d: %s\n", error, description);
}

void clickCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		//printf("%f %f\n", xpos, ypos);
		//sim->selectByRaycast(cameraPosition, rayCast(xpos, ypos, width, height));
	}
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
	//drawRaycast();
}

void updateCamera() {
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
	if ((location = glGetUniformLocation(program[PROG_CLOTH], "u_camLight")) != -1) {
		glUniform3fv(location, 1, &cameraPosition[0]);
	}
	glUseProgram(program[PROG_WIRE]);
	if ((location = glGetUniformLocation(program[PROG_WIRE], "u_projMatrix")) != -1) {
		glUniformMatrix4fv(location, 1, GL_FALSE, &projection[0][0]);
	}
}
/*
glm::vec3 rayCast(int x, int y, int width, int height) {
	updateCamera();
	float sx = (2.0f * (float)x / (float)width) - 1.0f;
	float sy = 1.0f - (2.0f * (float)y / (float)height);
	glm::vec3 up = glm::vec3(0.0, 0.0, 1.0);
	glm::vec3 F = glm::normalize(cameraPosition - lookAt);
	glm::vec3 R = glm::normalize(cross(up, F));
	glm::vec3 U = glm::normalize(cross(F, R));

	float aspect = (float)x / (float)width;

	float len = glm::length(lookAt - cameraPosition);
	float tanAlpha = tan(fovy * (PI / 180.0f) / 2.0f);
	glm::vec3 V = U * len * tanAlpha;
	glm::vec3 H = R * len * aspect * tanAlpha;
	glm::vec3 p = lookAt + sx * H + sy * V;

	// upload and draw the raycast
	GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, raycastSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(glm::vec4),
		NULL, GL_STREAM_COPY);

	glm::vec4 *pos = (glm::vec4 *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
		0, 2 * sizeof(glm::vec4), bufMask);
	pos[0] = glm::vec4(cameraPosition, 1.0);
	pos[1] = glm::vec4(cameraPosition + lookAt * 10000.0f, 1.0f);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	//drawRaycast();

	return glm::normalize(p - cameraPosition);
}

void drawRaycast() {
	glUseProgram(program[PROG_WIRE]);
	glBindVertexArray(wireVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, raycastIDXBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, raycastSSBO);

	glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, 0);
	//glDrawArrays(GL_LINES, 0, 2);
	checkGLError("raycast draw");
}
*/

glm::vec3 closestPointOnTriangle(glm::vec3 A, glm::vec3 B, glm::vec3 C, glm::vec3 P) {
	glm::vec3 v0 = C - A;
	glm::vec3 v1 = B - A;
	glm::vec3 N_v2 = glm::cross(glm::normalize(v1), glm::normalize(v0));

	// case 1: it's in the triangle
	// project into triangle plane
	// (project an arbitrary vector from P to triangle onto the normal)
	glm::vec3 w = A - P;
  float signedDistance_invDenom = glm::dot(N_v2, w);
  glm::vec3 projP = P + signedDistance_invDenom * N_v2;

	// compute u v coordinates
	// http://www.blackpawn.com/texts/pointinpoly/
	//u = ((v1.v1)(v2.v0) - (v1.v0)(v2.v1)) / ((v0.v0)(v1.v1) - (v0.v1)(v1.v0))
	//v = ((v0.v0)(v2.v1) - (v0.v1)(v2.v0)) / ((v0.v0)(v1.v1) - (v0.v1)(v1.v0))
	N_v2 = projP - A;
	float dot00_tAB = glm::dot(v0, v0);
	float dot01_tBC = glm::dot(v0, v1);
	float dot02_tCA = glm::dot(v0, N_v2);
	float dot11_minDistance = glm::dot(v1, v1);
	float dot12_candidate = glm::dot(v1, N_v2);

  signedDistance_invDenom = 1 / (dot00_tAB * dot11_minDistance - dot01_tBC * dot01_tBC);
  float u_t = (dot11_minDistance * dot02_tCA - dot01_tBC * dot12_candidate) * signedDistance_invDenom;
  float v = (dot00_tAB * dot12_candidate - dot01_tBC * dot02_tCA) * signedDistance_invDenom;

  // if the u v is in bounds, we can return the projected point
  if (u_t >= 0.0 && v >= 0.0 && (u_t + v) <= 1.0) {
    return projP;
	}

	// case 2: it's on an edge
	// http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html
	// t = - (x1 - x0) dot (x2 - x1) / len(x2 - x1) ^ 2
	// x1 is the "line origin," x2 is the "towards" point, and x0 is the outlier
	// check A->B edge
	dot00_tAB = -glm::dot(A - P, B - A) / pow(glm::length(B - A), 2.0f);

	// check B->C edge
	dot01_tBC = -glm::dot(B - P, C - B) / pow(glm::length(C - B), 2.0f);

	// check C->A edge
	dot02_tCA = -glm::dot(C - P, A - C) / pow(glm::length(A - C), 2.0f);

  // handle case 3: point is closest to a vertex
  dot00_tAB = glm::clamp(dot00_tAB, 0.0f, 1.0f);
  dot01_tBC = glm::clamp(dot01_tBC, 0.0f, 1.0f);
  dot02_tCA = glm::clamp(dot02_tCA, 0.0f, 1.0f);

	// assess each edge's distance and parametrics
  dot11_minDistance = glm::length(glm::cross(P - A, P - B)) / length(B - A);
  dot12_candidate;
	v0 = A;
	v1 = B;
  u_t = dot00_tAB;

  dot12_candidate = glm::length(glm::cross(P - B, P - C)) / length(B - C);
  if (dot12_candidate < dot11_minDistance) {
    dot11_minDistance = dot12_candidate;
    v0 = B;
    v1 = C;
    u_t = dot01_tBC;
	}

	dot12_candidate = glm::length(glm::cross(P - C, P - A)) / length(C - A);
  if (dot12_candidate < dot11_minDistance) {
    dot11_minDistance = dot12_candidate;
    v0 = C;
    v1 = A;
    u_t = dot02_tCA;
	}
  return (u_t * (v1 - v0) + v0);
}

void runTests() {
  cout << "running some tests..." << endl;
  // tests for nearest point on triangle

  cout << "testing nearest point on triangle algorithm" << endl;
  glm::vec3 A, B, C, P, nearest;

  A = glm::vec3(-1.0f, -1.0f, 0.0f);
  B = glm::vec3(3.0f, -1.0f, 0.0f);
  C = glm::vec3(-1.0f, 3.0f, 0.0f);

  // case 1: closest ISX is in triangle
  /********************************************
  C
  | P
  A---B
  ********************************************/
  P = glm::vec3(0.1f, 0.1f, 2.0f);
  nearest = closestPointOnTriangle(A, B, C, P);
  cout << "testing case 1: closest point is in triangle." << endl;
  cout << "expected: 0.1 0.1 0.0 actual: " << nearest.x << " " << nearest.y << " " << nearest.z << endl;
  
  // case 2: closest ISX is on an edge
  /********************************************
  C
  |
  A---B

  P
  ********************************************/
  P = glm::vec3(1.0f, -2.0f, 0.0f);
  nearest = closestPointOnTriangle(A, B, C, P);
  cout << "testing case 2: closest point is on an edge." << endl;
  cout << "expected: 1 -1 0 actual: " << nearest.x << " " << nearest.y << " " << nearest.z << endl;


  // case 3: closest ISX is a vertex
  /********************************************
      C
      |
      A---B

  P
  ********************************************/
  P = glm::vec3(-3.0f, -3.0f, 0.0f);
  nearest = closestPointOnTriangle(A, B, C, P);
  cout << "testing case 2: closest point is on an edge." << endl;
  cout << "expected: -1 -1 0 actual: " << nearest.x << " " << nearest.y << " " << nearest.z << endl;

  
  cout << "done tests!" << endl;
}