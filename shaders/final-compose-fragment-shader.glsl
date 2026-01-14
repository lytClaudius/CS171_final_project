in vec2 vUv;
out vec4 FragColor;

uniform sampler2D radianceTexture;
uniform sampler2D sdfTexture;
uniform vec2 resolution;

float hash(vec2 p)  // replace this by something better
{
    p  = 50.0*fract( p*0.3183099 + vec2(0.71,0.113));
    return -1.0+2.0*fract( p.x*p.y*(p.x+p.y) );
}

float hash(vec3 p)  // replace this by something better
{
    p  = 50.0*fract( p*0.3183099 + vec3(0.71, 0.113, 0.5231));
    return -1.0+2.0*fract( p.x*p.y*p.z*(p.x+p.y+p.z) );
}

vec3 BackgroundColour() {
  return vec3(1.0);
}


vec3 drawGraphBackground_Ex(vec2 pixelCoords, float scale) {
  float pixelSize = 1.0 / scale;
  vec2 cellPosition = floor(pixelCoords / vec2(100.0));
  vec2 cellID = vec2(floor(cellPosition.x), floor(cellPosition.y));
  vec3 checkerboard = vec3(mod(cellID.x + cellID.y, 2.0));

  vec3 colour = BackgroundColour();
  colour = mix(colour, checkerboard, 0.05);

  colour = (vec3(0.95) + hash(pixelCoords) * 0.01) * colour;

  return colour;
}

vec3 drawGraphBackground(vec2 pixelCoords) {
  return drawGraphBackground_Ex(pixelCoords, 1.0);
}

void main() {
  vec2 pixelCoords = (vUv - 0.5) * resolution;
  vec4 radiance = texture(radianceTexture, vUv);
  vec4 sdf = texture2D(sdfTexture, vUv);
  vec3 bg = drawGraphBackground(pixelCoords);

  // 计算物体的遮罩 (抗锯齿)
  float edge = fwidth(sdf.w);
  float mask = smoothstep(edge, -edge, sdf.w);

  // 1. 背景受光照影响
  vec3 litBackground = bg * radiance.xyz;
  
  // 2. 物体本身不被自己的光再次相乘（它本身就是光源）
  vec3 colour = mix(litBackground, sdf.xyz, mask);
  #if defined(USE_OKLAB)
  colour = oklabToRGB(colour);
  #endif
  // vec3 colour = litBackground * (1.0 - mask) + sdf.xyz * mask;

  colour = aces_tonemap(colour);
  FragColor = vec4(colour, 1.0);
}