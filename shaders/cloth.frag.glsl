#version 430 core

// pretend there's an area light straight up

in vec3 normal;
//in vec3 worldPos;

out vec4 fragColor;

void main() {
	vec3 up = vec3(0.0, 0.0, 1.0); // we live in a z-up world
    fragColor = vec4(vec3(dot(up, normal)), 1.0);
}
