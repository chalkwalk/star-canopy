#version 150 core

// One triangle covering the viewport, from gl_VertexID alone, so no vertex
// buffer is needed. Bakes cut it down to a tile with the scissor, which keeps
// gl_FragCoord the face's own texel coordinate.

void main() {
  vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
  gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
