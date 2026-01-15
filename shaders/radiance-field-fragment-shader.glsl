in vec2 vUv;
out vec4 FragColor;

uniform sampler2D mergedCascade0Texture;

// Helper to average the probe pixels
vec4 avg_probe(ivec2 p, CascadeConfig c) {
  float size = float(c.n);
  vec2 base = vec2(p) * size;
  
  vec4 total = vec4(0.0);
  // double loop to sum up pixels
  for (int x = 0; x < c.n; ++x) {
    for (int y = 0; y < c.n; ++y) {
      vec2 off = vec2(x, y) + 0.5;
      vec2 pos = base + off;
      total += texture(mergedCascade0Texture, pos / cascadeResolution);
    }
  }
  return total / float(c.n * c.n);
}

void main() {
  CascadeConfig c0 = get_grid_config(0);
  
  float step = float(c0.n);
  vec2 grid_pos = gl_FragCoord.xy / step - 0.5;
  
  // 4 corners
  ivec2 idx00 = ivec2(floor(grid_pos));
  ivec2 idx10 = idx00 + ivec2(1, 0);
  ivec2 idx01 = idx00 + ivec2(0, 1);
  ivec2 idx11 = idx00 + ivec2(1, 1);
  
  vec4 val00 = avg_probe(idx00, c0);
  vec4 val10 = avg_probe(idx10, c0);
  vec4 val01 = avg_probe(idx01, c0);
  vec4 val11 = avg_probe(idx11, c0);
  
  // bilinear interpolation manually
  vec2 t = fract(grid_pos);
  
  vec4 temp1 = mix(val00, val10, t.x);
  vec4 temp2 = mix(val01, val11, t.x);
  
  vec4 res = mix(temp1, temp2, t.y);

  FragColor = res;
}