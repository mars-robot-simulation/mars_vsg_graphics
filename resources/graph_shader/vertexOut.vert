void vertexOut(in vec4 viewPos, in vec3 modelPos, in vec3 normalV) {
  gl_Position = pc.projection *viewPos;
  //gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;
  normalVarying = normalV;
  //normalVarying = vsg_Normal;
  //normalVarying = (pc.modelView*vec4(0.0, 0.0, 1.0, 0.0)).xyz;
  modelVertex = vec4(modelPos, 1);
}