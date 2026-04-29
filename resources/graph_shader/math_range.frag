void math_range(in float start, in float end, in float value, out float result) {
    result = 1.0;
    if(value < start) result = 0.0;
    if(value > end) result = 0.0;
}