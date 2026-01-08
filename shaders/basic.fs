#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2DArray screenTexture; 
uniform sampler2D sceneTex;           
uniform float u_base_dist = 8.0;       // 适当调大一点看看
uniform vec2 u_screen_size = vec2(800.0, 600.0); 

void main() {
    // --- 调试步骤 1：直接看 SceneTex (如果有图说明贴图传到了) ---
    // vec4 test = texture(sceneTex, TexCoords);
    // FragColor = vec4(test.rgb, 1.0); return; 

    vec3 total_radiance = vec3(0.0);
    float total_trans = 0.0; // 用于调试轮廓

    // 为了调试，我们只看一个方向，看看能不能射中物体
    for (uint i = 0; i < 64; i++) {
        float angle = (float(i) + 0.5) / 64.0 * 6.2831853;
        vec2 ray_dir = vec2(cos(angle), sin(angle));

        float near_trans = 1.0;
        float near_radiance = 0.0;

        // --- Cascade -1 核心修正 ---
        // 使用 TexCoords * u_screen_size 代替 gl_FragCoord.xy 更加稳妥
        vec2 start_pixel_pos = TexCoords * u_screen_size;

        for(float d = 1.0; d < u_base_dist; d += 1.0) { // 步长设为1像素
            vec2 sample_uv = (start_pixel_pos + ray_dir * d) / u_screen_size;

            // 越界直接跳出
            if(sample_uv.x < 0.0 || sample_uv.x > 1.0 || sample_uv.y < 0.0 || sample_uv.y > 1.0) break;

            vec4 scene = texture(sceneTex, sample_uv);
            
            // RC 逻辑：如果碰到物体 (scene.a > 0)
            if(scene.a > 0.0) {
                near_radiance += scene.rgb * near_trans * scene.a;
                near_trans *= (1.0 - scene.a);
                if(near_trans < 0.01) break;
            }
        }
        
        // 采样 C0 (调试时可以先注释掉，只看 -1)
        /*
        ivec2 atlas_size = textureSize(screenTexture, 0).xy;
        vec2 probe_count = vec2(atlas_size) / 8.0;
        vec2 c0_probe_pos = TexCoords * probe_count - 0.5;
        uvec2 d_idx2 = uvec2(i % 8, i / 8);
        vec2 atlas_uv = (vec2(d_idx2) * probe_count + c0_probe_pos + 0.5) / vec2(atlas_size);
        vec3 far_radiance = texture(screenTexture, vec3(atlas_uv, 0)).rgb;
        */

        total_radiance += near_radiance; // + near_trans * far_radiance;
        total_trans += near_trans;
    }

    // 最终输出：如果射线撞到了物体，屏幕就不应该是黑的
    vec3 result = total_radiance / 64.0;
    
    // 调试辅助：如果你看见灰色的轮廓，说明射线起作用了
    if(length(result) < 0.001) {
        // 如果还是全黑，显示一个淡淡的背景色，判断 Shader 没死
        result = vec3(0.05); 
    }

    FragColor = vec4(result, 1.0);
}