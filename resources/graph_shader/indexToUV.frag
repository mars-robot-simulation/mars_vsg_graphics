void indexToUV(in int index, in int textureSize, out vec2 uv) {
     int h = index / textureSize;
     float offset = (1.0/float(textureSize))*0.5;
     uv.x = mod(float(index), float(textureSize))/float(textureSize) + offset;
     uv.y = float(h)/float(textureSize) + offset;
}
