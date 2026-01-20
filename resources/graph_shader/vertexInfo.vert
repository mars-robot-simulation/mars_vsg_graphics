void vertexInfo(out vec3 normal) {
  normal = normalize((pc.modelView * vec4(vsg_Normal, 0.0)).xyz);
  //normal = normalize(wt.viewInverse * pc.modelView * vec4(vsg_Normal, 0.0)).xyz;
}