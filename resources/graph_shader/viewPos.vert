void viewPos(in vec3 modelPos, out vec4 viewPos) {
  viewPos = pc.modelView * vec4(modelPos, 1.0);
}