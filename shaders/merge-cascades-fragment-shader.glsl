in vec2 vUv;
out vec4 FragColor;

uniform sampler2D nextCascadeMergedTexture;
uniform sampler2D cascadeRealTexture;
uniform sampler2D sdfTexture;
uniform vec2 sceneResolution;

uniform int numCascadeLevels;
uniform int currentCascadeLevel;

void main() {
  vec2 px = (gl_FragCoord.xy - 0.5);

  // 获取当前配置
  CascadeConfig cfg = get_grid_config(currentCascadeLevel);
  CascadeID gid = get_grid_id(px, cfg);
  Box b = get_box(gid, cfg);

  vec2 rel = px - b.min;
  AngIdx ai = AngIdx(ivec2(rel));
  
  float ang = get_angle_from_coords(ai, cfg);

  // 当前层的数据
  vec4 current_val = texture(cascadeRealTexture, gl_FragCoord.xy / cascadeResolution);

  // 射线信息
  vec2 dir = vec2(cos(ang), sin(ang));
  vec2 ro = b.center * sceneResolution / cascadeResolution;

  int max_lev = numCascadeLevels - 1;
  int next_lev = currentCascadeLevel + 1;

  // 如果还有下一层级，进行混合
  if (next_lev <= max_lev) {
    // 调用改名后的双线性采样函数
    vec4 upper_val = sample_merged_bilinear(nextCascadeMergedTexture, px, ang, next_lev);

    // 经典的混合公式
    current_val.rgb += upper_val.rgb * current_val.a;
    current_val.a *= upper_val.a;
  }

  FragColor = current_val;
}