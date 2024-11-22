void applyModel(inout vec2 z)
{ float n = length(z); z = z * atanh(n) / (tau * max(1e-10, n)); }
