#version 430 core

uniform mat4 u_projMatrix;

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

in vec3 worldPos0[3];

// http://stackoverflow.com/questions/19346019/calculate-normals-geometry-shader

out vec3 normal;
out vec3 worldPos;

void main() {

	// compute triangle normal
    vec3 n = cross(normalize(worldPos0[1].xyz - worldPos0[0].xyz),
    	normalize(worldPos0[2].xyz - worldPos0[0].xyz));
	for(int i = 0; i < gl_in.length(); i++)
        {
            vec3 Position = gl_in[i].gl_Position.xyz;
   			gl_Position = u_projMatrix * vec4(Position, 1.0);
            normal = n;
            worldPos = worldPos0[i];
            EmitVertex();
        }
}
