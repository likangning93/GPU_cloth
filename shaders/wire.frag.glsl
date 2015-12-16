#version 430
// ^ Change this to version 130 if you have compatibility issues

out vec4 out_Col;

void main()
{
    // Copy the color; there is no shading.
    out_Col = vec4(1.0);
}
