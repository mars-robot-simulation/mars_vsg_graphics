void image2DLoad(in vec2 texCoord, out vec4 color) {  
  ivec2 size = imageSize(storeImageMap);
  ivec2 pixelCoord = ivec2(int(size.x*texCoord.x), int(size.y*texCoord.y));
  color = imageLoad(storeImageMap, pixelCoord);
}
