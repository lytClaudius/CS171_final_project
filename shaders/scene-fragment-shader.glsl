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

  // vec3 lightColour = vec3(0.0, 0.0, 1.0);
  // float lightDist = sdfBox(pixelCoords - vec2(0.0), vec2(20.0));
  // texel.xyz = mix(texel.xyz, lightColour, smoothstep(1.0, 0.0, lightDist));
  // texel.w = min(texel.w, lightDist);

  // Draw temporary brush
  vec2 brushCoords = (brushPos - 0.5) * resolution;
  float brushDist = sdfCircle((pixelCoords - brushCoords), brushRadius * 0.5);

  texel.xyz = mix(texel.xyz, brushColour, smoothstep(1.0, 0.0, brushDist));
  texel.w = min(texel.w, brushDist);

  FragColor = vec4(texel.xyz, texel.w);
}