vec2 applyModel(vec2 z)
{ float n = length(z); return z * atanh(n) / (tau * max(1e-10, n)); }
