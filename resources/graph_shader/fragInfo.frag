void fragInfo(out vec4 ambient, out vec4 diffuse, out vec4 specular, out vec4 emission, out vec2 texCoord) {
  texCoord = texCoord0;
  //*texScale;
  //texCoord = vec2(0.0, 0.0);
  // todo: check how to deal with ambient
  ambient = pbr.baseColorFactor;
  diffuse = pbr.diffuseFactor;
  specular = pbr.specularFactor;
  emission = pbr.emissiveFactor;
}