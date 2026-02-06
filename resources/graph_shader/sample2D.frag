void sample2D(in vec2 texCoord, in sampler2D texture_, out vec4 color) {
  color = texture(texture_, texCoord);
}