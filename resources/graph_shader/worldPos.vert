void worldPos(in vec4 viewPos, out vec4 worldPos) {
  worldPos = wt.viewInverse * viewPos;
  positionVarying = worldPos;
}