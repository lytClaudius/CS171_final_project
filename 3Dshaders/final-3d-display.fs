
out vec4 FragColor;

uniform vec2 iResolution;
uniform float iTime;
uniform vec3 uCamPos;   
uniform vec3 uCamFront; 

// 刚刚烘焙好的 Atlas 纹理
uniform sampler2D radianceAtlas;

// 采样 Atlas 的辅助函数
vec4 SampleAtlas(vec2 uv) {
    vec2 texUV = uv / ATLAS_RES;
    return textureLod(radianceAtlas, texUV, 0.0);
}

void main() {
    vec3 Color = vec3(0.);
    vec3 pPos = uCamPos; 
    vec3 pEye = normalize(uCamFront); 
    
    // 计算射线方向
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 uv = fragCoord / iResolution.xy;
    vec2 p = uv * 2.0 - 1.0;
    p.x *= iResolution.x / iResolution.y; 
    
    vec3 pDir = normalize(vec3(p, 1.)*TBN(pEye));
    

    HIT rayHit = TraceRay(pPos, pDir, 1000000., iTime);
    
    if (rayHit.n.x > -15.) { // 击中物体
        if (rayHit.c.x < -1.5) {
            // Reflective (镜面反射)
            vec3 rDir = reflect(pDir, rayHit.n);
            HIT rayHit2 = TraceRay(pPos + pDir*rayHit.t + rayHit.n*0.001, rDir, 1000000., iTime);
            
            if (rayHit2.n.x > -15.) {
                if (rayHit.c.x > 1.) {
                    Color += rayHit.c; // Emissive
                } else if (dot(rayHit2.n, rDir) < 0.) {
                    // 查表获取间接光
                    vec2 suv = GetAtlasUV(rayHit2);
                    Color = SampleAtlas(suv).xyz + 
                            SampleAtlas(suv + vec2(rayHit2.res.x*0.5, 0.)).xyz +
                            SampleAtlas(suv + vec2(0., rayHit2.res.y*0.5)).xyz + 
                            SampleAtlas(suv + rayHit2.res*0.5).xyz;
                    
                    Color *= rayHit2.c;
                }
            } else {
                Color = vec3(0.);
            }
        } else if (max(rayHit.c.x, max(rayHit.c.y, rayHit.c.z)) > 1.5) {
            // Emissive
            Color += rayHit.c;
        } else {
            // Diffuse Object
            if (dot(rayHit.n, pDir) < 0.) {
                // 核心：查询光照图集 (Texture Atlas Lookup)
                vec2 suv = GetAtlasUV(rayHit);
                
                Color = SampleAtlas(suv).xyz + 
                        SampleAtlas(suv + vec2(rayHit.res.x*0.5, 0.)).xyz +
                        SampleAtlas(suv + vec2(0., rayHit.res.y*0.5)).xyz + 
                        SampleAtlas(suv + rayHit.res*0.5).xyz;
            

                Color *= rayHit.c;
            }
        }
    } else {
        // Sky
        Color = vec3(0.);
    }
    
    // Tone mapping and Gamma correction
    FragColor = vec4(pow(AcesFilm(max(vec3(0.), Color)), vec3(0.45)), 1.);
}