#version 330 core
out vec4 frag_Color;

uniform sampler2D fb_texture;

in vertex_data {
  vec2 texture_coord;
} vertex;

uniform float colour_scale;

void main() {
  vec4 sample = texture(fb_texture, vertex.texture_coord);
  frag_Color = max(sample - colour_scale, vec4(0));
}
