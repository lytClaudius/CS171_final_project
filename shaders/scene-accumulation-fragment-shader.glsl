uniform vec2 resolution;

in vec2 vUv;

out vec4 FragColor;

uniform sampler2D sdfTexture;

uniform vec2 brushPos1;    // 上一帧鼠标归一化位置 (0.0 - 1.0)
uniform vec2 brushPos2;    // 当前帧鼠标归一化位置 (0.0 - 1.0)
uniform float brushRadius;
uniform vec3 brushColour;
uniform bool isDrawing;   

void main() {
  vec2 uv = gl_FragCoord.xy / resolution.xy;
  vec2 pixelCoords = gl_FragCoord.xy - resolution.xy / 2.0;

  vec4 texel = texture2D(sdfTexture, uv);

  if (isDrawing) {
        vec2 b1 = (brushPos1 - 0.5) * resolution;
        vec2 b2 = (brushPos2 - 0.5) * resolution;

        float distBetween = distance(b1, b2);
        float steps = max(ceil(distBetween), 1.0);

        for (float i = 0.0; i <= steps; ++i) {
            vec2 currentB = mix(b1, b2, i / steps);
            float d = sdfCircle(pixelCoords - currentB, brushRadius * 0.5);

            texel.w = min(texel.w, d);
            texel.xyz = mix(texel.xyz, brushColour, smoothstep(1.0, 0.0, d));
        }
    }


  FragColor = vec4(texel.xyz, texel.w);
}