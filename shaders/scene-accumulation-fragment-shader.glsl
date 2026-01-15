uniform vec2 resolution;

in vec2 vUv;
out vec4 FragColor;

uniform sampler2D sdfTexture;

uniform vec2 brushPos1;
uniform vec2 brushPos2;
uniform float brushRadius;
uniform vec3 brushColour;
uniform bool isDrawing;
uniform bool isErasing; 

void main() {
  vec2 uv = gl_FragCoord.xy / resolution.xy;
  vec2 p = gl_FragCoord.xy - resolution.xy / 2.0;

  vec4 curr_color = texture(sdfTexture, uv);

  if (isDrawing) {
        vec2 start = (brushPos1 - 0.5) * resolution;
        vec2 end = (brushPos2 - 0.5) * resolution;

        float len = distance(start, end);
        float count = max(ceil(len), 1.0);

        // draw line by stepping
        for (float k = 0.0; k <= count; ++k) {
            float t = k / count;
            vec2 pos = mix(start, end, t);
            
            // distance to brush
            float dist = sdfCircle(p - pos, brushRadius * 0.5);

            if (isErasing) {
                // erase
                curr_color.w = max(curr_color.w, -dist);
            } else {
                // draw
                curr_color.w = min(curr_color.w, dist);
                
                float alpha = 1.0 - smoothstep(0.0, 1.0, dist);
                curr_color.xyz = mix(curr_color.xyz, brushColour, alpha);
            }
        }
    }

  FragColor = vec4(curr_color.xyz, curr_color.w);
}