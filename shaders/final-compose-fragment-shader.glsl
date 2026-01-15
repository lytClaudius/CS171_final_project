in vec2 vUv;
out vec4 FragColor;

uniform sampler2D radianceTexture;
uniform sampler2D sdfTexture;
uniform vec2 resolution;

// Random function copied from online
float my_random(vec2 p) {
    vec2 temp = p * 0.3183099 + vec2(0.71, 0.113);
    p = 50.0 * fract(temp);
    return -1.0 + 2.0 * fract(p.x * p.y * (p.x + p.y));
}

vec3 get_bg_color() {
  return vec3(1.0, 1.0, 1.0); // White
}

vec3 draw_grid(vec2 px) {
  // checkerboard pattern
  vec2 cell = floor(px / 100.0);
  float check = mod(cell.x + cell.y, 2.0);
  vec3 cb_col = vec3(check);

  vec3 col = get_bg_color();
  col = mix(col, cb_col, 0.05);

  // add noise
  float n = my_random(px);
  col = (vec3(0.95) + n * 0.01) * col;

  return col;
}

void main() {
  vec2 uv = vUv;
  vec2 px = (uv - 0.5) * resolution;
  
  vec4 light = texture(radianceTexture, uv);
  vec4 scene = texture2D(sdfTexture, uv);
  
  vec3 bg = draw_grid(px);

  float dist = scene.w;
  // antialias
  float w = fwidth(dist);
  float alpha = smoothstep(w, -w, dist);

  vec3 bg_lit = bg * light.xyz;
  vec3 obj_col = scene.xyz;

  // mix final color
  vec3 final_col = mix(bg_lit, obj_col, alpha);

  #if defined(USE_OKLAB)
  final_col = oklabToRGB(final_col);
  #endif

  // tone mapping
  final_col = aces_tonemap(final_col);
  
  FragColor = vec4(final_col, 1.0);
}