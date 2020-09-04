#version 330 core
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;

out vertex_data {
  vec2 texture_coord;
} vertex;

void main() {
  gl_Position = vec4(pos, 0, 1);
  vertex.texture_coord = uv;
}
