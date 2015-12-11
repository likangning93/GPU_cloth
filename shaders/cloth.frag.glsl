#version 430 core

#define amb 0.3

uniform vec3 u_color;

uniform vec3 u_camLight;

// pretend there's an area light straight up

in vec3 normal;
in vec3 worldPos;

out vec4 fragColor;

void main() {
	vec3 up = vec3(0.0, 0.0, 1.0); // we live in a z-up world
	float sun_mag = max(dot(up, normal), amb);
	float cam_mag = dot(normalize(u_camLight - worldPos), normal);

    fragColor = vec4(vec3((cam_mag * 0.8 + sun_mag) * u_color), 1.0);
}
