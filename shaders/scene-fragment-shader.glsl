uniform vec2 resolution;

in vec2 vUv;

out vec4 FragColor;

uniform float time;
uniform sampler2D sdfTexture;

uniform vec2 brushPos;    
uniform float brushRadius;
uniform vec3 brushColour;


void main() {
  vec2 uv = gl_FragCoord.xy / resolution.xy;
  vec2 pixelCoords = gl_FragCoord.xy - resolution.xy / 2.0;

  vec4 texel = texture2D(sdfTexture, uv);

  // 添加固定障碍物
  // 障碍物1：左上角的盒子
  vec3 obstacle1Color = vec3(0.);
  float obstacle1Dist = sdfBox(pixelCoords - vec2(-500.0, 400.0), vec2(60.0, 60.0));
  texel.xyz = mix(texel.xyz, obstacle1Color, smoothstep(1.0, 0.0, obstacle1Dist));
  texel.w = min(texel.w, obstacle1Dist);
  
  // 障碍物2：右上角的圆形
  vec3 obstacle2Color = vec3(0.);
  float obstacle2Dist = sdfCircle(pixelCoords - vec2(500.0, 400.0), 50.0);
  texel.xyz = mix(texel.xyz, obstacle2Color, smoothstep(1.0, 0.0, obstacle2Dist));
  texel.w = min(texel.w, obstacle2Dist);
  
  // 障碍物3：左下角的盒子
  vec3 obstacle3Color = vec3(0.);
  float obstacle3Dist = sdfBox(pixelCoords - vec2(-500.0, -400.0), vec2(60.0, 60.0));
  texel.xyz = mix(texel.xyz, obstacle3Color, smoothstep(1.0, 0.0, obstacle3Dist));
  texel.w = min(texel.w, obstacle3Dist);

  // Draw temporary brush
  vec2 brushCoords = (brushPos - 0.5) * resolution;
  float brushDist = sdfCircle((pixelCoords - brushCoords), brushRadius * 0.5);

  texel.xyz = mix(texel.xyz, brushColour, smoothstep(1.0, 0.0, brushDist));
  texel.w = min(texel.w, brushDist);

  FragColor = vec4(texel.xyz, texel.w);
}