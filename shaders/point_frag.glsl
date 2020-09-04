#version 330 core
// out vec4 frag_Color;
layout(location = 0) out vec4 color;

in vertex_data {
  vec4 colour;
} vertex;

void main() {
  vec2 pt = gl_PointCoord * 2.0 - 1.0;
  float r = dot(pt, pt);
  // if (r > 1.0) {
  //   discard;
  // }
  //Black magic
  float delta = fwidth(r)/3;
  float alpha = 1.0 - smoothstep(1.0 - delta, 1.0 + delta, r);
  color = vec4(vertex.colour.xyz, vertex.colour.w * alpha);
}
