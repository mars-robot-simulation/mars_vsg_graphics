void image2DStore(in vec2 texCoord, in vec4 color) {
  ivec2 size = imageSize(storeImageMap);
  ivec2 pixelCoord = ivec2(int(size.x*texCoord.x), int(size.y*texCoord.y));
  imageStore(storeImageMap, pixelCoord, color);
}
