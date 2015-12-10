#version 430 core

#define amb 0.3

uniform vec3 u_color;

// pretend there's an area light straight up

in vec3 normal;
//in vec3 worldPos;

out vec4 fragColor;

void main() {
	vec3 up = vec3(0.0, 0.0, 1.0); // we live in a z-up world
	float mag = max(dot(up, normal), amb);
    fragColor = vec4(vec3(mag * u_color), 1.0);
}
