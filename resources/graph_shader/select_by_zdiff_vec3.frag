void select_by_zdiff_vec3(in vec3 vec_a, in vec3 vec_b, in float th, out vec3 vector) {
     float d = abs(vec_a.z-vec_b.z);
     if(d > th) vector = vec_b;
     else vector = vec_a;
}
