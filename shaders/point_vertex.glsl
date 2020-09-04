#version 330 core
layout (location = 0) in vec2 pos;
layout (location = 1) in vec4 colour;

out vertex_data {
  vec4 colour;
} vertex;

uniform int window_w;
uniform int window_h;

void main() {
  vec2 fixed_pos = vec2(pos.x / window_w, pos.y / window_h);
  gl_Position = vec4(fixed_pos, 0, 1);
  vertex.colour = colour;
}
