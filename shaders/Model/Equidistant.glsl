#define PI 3.1415926538

void applyModel(inout vec2 z)
{ z = z * atanh(length(z)) / (2.0 * PI * max(1e-10, length(z))); }
