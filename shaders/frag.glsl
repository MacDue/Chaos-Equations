#version 330 core
out vec4 frag_Color;

in vertex_data {
  vec4 colour;
} vertex;

void main() {
  frag_Color = vertex.colour;
}
