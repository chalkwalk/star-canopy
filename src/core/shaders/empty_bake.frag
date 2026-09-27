#version 150 core

// The empty sky: black everywhere. It exists to prove the context, the tiling
// and the readback before the model arrives.

out vec4 fragColor;

void main() {
  fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
