#version 320 es
/* Rotating green triangle; rotation angle comes via push constant. */
layout(push_constant) uniform PC {
    float angle;
    float pad0; float pad1; float pad2;
} pc;

void main() {
    vec2 pos[3];
    pos[0] = vec2( 0.00,  0.62);
    pos[1] = vec2(-0.56, -0.50);
    pos[2] = vec2( 0.56, -0.50);
    vec2 p = pos[gl_VertexIndex];
    float c = cos(pc.angle);
    float s = sin(pc.angle);
    vec2 r = vec2(p.x * c - p.y * s, p.x * s + p.y * c);
    gl_Position = vec4(r, 0.0, 1.0);
}
