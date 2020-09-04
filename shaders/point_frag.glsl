#version 330 core
// out vec4 frag_Color;
layout(location = 0) out vec3 color;

in vertex_data {
  vec4 colour;
} vertex;

void main() {
  color = vertex.colour.xyz;
}
