#version 450

vec2 positions[3] = {
    {  0.0, -0.5 },
    { +0.5, +0.5 },
    { -0.5, +0.5 }
};

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.1337, 1.0);
}
