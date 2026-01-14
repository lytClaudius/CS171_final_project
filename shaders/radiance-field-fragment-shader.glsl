in vec2 vUv;
out vec4 FragColor;

uniform sampler2D mergedCascade0Texture;

// 辅助函数：获取特定 Probe 的平均 Radiance
vec4 GetProbeRadiance(ivec2 probeCoords, CascadeInfo info) {
  vec2 probeBasePixels = vec2(probeCoords) * float(info.dimensions);
  
  vec4 sum = vec4(0.0);
  for (int i = 0; i < info.dimensions; ++i) {
    for (int j = 0; j < info.dimensions; ++j) {
      vec2 samplePos = probeBasePixels + vec2(i, j) + 0.5;
      sum += texture(mergedCascade0Texture, samplePos / cascadeResolution);
    }
  }
  return sum / float(info.dimensions * info.dimensions);
}

void main() {
  CascadeInfo ci0 = Cascade_GetInfo(0);
  
  // 1. 计算当前像素在 Probe 网格中的位置
  float spacing = float(ci0.dimensions);
  vec2 currentPosInProbeGrid = gl_FragCoord.xy / spacing - 0.5;
  
  // 2. 找到相邻的四个 Probe 索引
  ivec2 p00 = ivec2(floor(currentPosInProbeGrid));
  ivec2 p10 = p00 + ivec2(1, 0);
  ivec2 p01 = p00 + ivec2(0, 1);
  ivec2 p11 = p00 + ivec2(1, 1);
  
  // 3. 获取四个 Probe 的平均光照
  vec4 r00 = GetProbeRadiance(p00, ci0);
  vec4 r10 = GetProbeRadiance(p10, ci0);
  vec4 r01 = GetProbeRadiance(p01, ci0);
  vec4 r11 = GetProbeRadiance(p11, ci0);
  
  // 4. 双线性插值
  vec2 f = fract(currentPosInProbeGrid);
  vec4 r0 = mix(r00, r10, f.x);
  vec4 r1 = mix(r01, r11, f.x);
  vec4 finalRadiance = mix(r0, r1, f.y);

  FragColor = finalRadiance;
}