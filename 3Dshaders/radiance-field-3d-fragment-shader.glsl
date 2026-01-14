out vec4 FragColor;

uniform vec2 iResolution; 
uniform float iTime;
uniform sampler2D iChannel3; 

// 采样上一帧的纹理（即 Shadertoy 中的 TextureCube）
vec4 SampleAtlas(vec2 uv, float lod) {
    vec2 texUV = uv / ATLAS_RES;
    return textureLod(iChannel3, texUV, lod);
}

// 加权采样函数 (对应 Cascades and merging 部分)
vec4 WeightedSample(vec2 luvo, vec2 luvd, vec2 luvp, vec2 uvo, vec3 probePos,
                    vec3 gTan, vec3 gBit, vec3 gPos, float lProbeSize) {
    
    vec3 lastProbePos = gPos + gTan*(luvp.x*lProbeSize/256.) + gBit*(luvp.y*lProbeSize/256.);
    vec3 relVec = probePos - lastProbePos;
    float theta = (lProbeSize*0.5 - 0.5)/(lProbeSize*0.5)*PI*0.5;
    float phi = atan(-dot(relVec, gTan), -dot(relVec, gBit));
    float phiI = floor((phi/PI*0.5 + 0.5)*(4. + 8.*(lProbeSize*0.5 - 1.))) + 0.5;
    
    vec2 phiUV;
    float phiLen = lProbeSize - 1.;
    if (phiI < phiLen) phiUV = vec2(lProbeSize - 0.5, lProbeSize - phiI);
    else if (phiI < phiLen*2.) phiUV = vec2(lProbeSize - (phiI - phiLen), 0.5);
    else if (phiI < phiLen*3.) phiUV = vec2(0.5, phiI - phiLen*2.);
    else phiUV = vec2(phiI - phiLen*3., lProbeSize - 0.5);
    
    float lProbeRayDist = SampleAtlas(luvo + floor(phiUV)*uvo + luvp, 0.).w;
    
    if (lProbeRayDist < -0.5 || length(relVec) < lProbeRayDist*cos(PI*0.5 - theta) + 0.01) {
        vec2 luv = luvo + luvd + clamp(luvp, vec2(0.5), uvo - 0.5);;
        return vec4(SampleAtlas(luv, 0.).xyz + SampleAtlas(luv + vec2(uvo.x, 0.), 0.).xyz +
                    SampleAtlas(luv + vec2(0., uvo.y), 0.).xyz + SampleAtlas(luv + uvo, 0.).xyz, 1.);
    }
    return vec4(0.);
}

void main() {
    // 当前像素坐标 (0.5 ~ 1023.5, 0.5 ~ 6143.5)
    vec2 UV = gl_FragCoord.xy;
    vec4 Output = SampleAtlas(UV, 0.0); // 默认继承上一帧

    // 检查是否在有效区域内
    if (DFBox(UV, vec2(1024., (256.*6.)*2.)) < 0.) {
        Output = vec4(0.); 
        vec2 gRes;
        vec3 gTan, gBit, gNor, gPos;
  
        // 判断属于哪个物体表面
        if (UV.y < 256.*6.) {
            if (UV.x < 256.) { // Floor
                gTan = vec3(1., 0., 0.); gBit = vec3(0., 0., 1.); gNor = vec3(0., 1., 0.);
                gPos = vec3(0., 0., 0.); gRes = vec2(256.);
            } else if (UV.x < 512.) { // Ceiling
                gTan = vec3(1., 0., 0.); gBit = vec3(0., 0., 1.); gNor = vec3(0., -1., 0.);
                gPos = vec3(0., 0.5, 0.); gRes = vec2(256.);
            } else if (UV.x < 640.) { // Wall X
                gTan = vec3(0., 1., 0.); gBit = vec3(0., 0., 1.); gNor = vec3(1., 0., 0.);
                gPos = vec3(0., 0., 0.); gRes = vec2(128., 256.);
            } else if (UV.x < 768.) { // Wall -X
                gTan = vec3(0., 1., 0.); gBit = vec3(0., 0., 1.); gNor = vec3(-1., 0., 0.);
                gPos = vec3(1., 0., 0.); gRes = vec2(128., 256.);
            } else if (UV.x < 896.) { // Wall Z
                gTan = vec3(0., 1., 0.); gBit = vec3(1., 0., 0.); gNor = vec3(0., 0., 1.);
                gPos = vec3(0., 0., 0.); gRes = vec2(128., 256.);
            } else { // Wall -Z
                gTan = vec3(0., 1., 0.); gBit = vec3(1., 0., 0.); gNor = vec3(0., 0., -1.);
                gPos = vec3(0., 0., 1.); gRes = vec2(128., 256.);
            }
        } else { // Interior Walls
            if (UV.x < 128.) {
                gTan = vec3(0., 1., 0.); gBit = vec3(1., 0., 0.); gNor = vec3(0., 0., -1.);
                gPos = vec3(0., 0., 0.47 - 1./256.); gRes = vec2(128., 256.);
            } else if (UV.x < 256.) {
                gTan = vec3(0., 1., 0.); gBit = vec3(1., 0., 0.); gNor = vec3(0., 0., 1.);
                gPos = vec3(0., 0., 0.53 - 1./256.); gRes = vec2(128., 256.);
            }
        }
        
        // 探针
        vec2 modUV = mod(UV, gRes);
        float probeCascade = floor(mod(UV.y, 1536.)/256.);
        float probeSize = pow(2., probeCascade + 1.);
        vec2 probePositions = gRes/probeSize;
        
        // 计算当前像素对应的 3D 空间位置 (Probe Position)
        vec3 probePos = gPos + mod(modUV.x, probePositions.x)*probeSize/256.*gTan +
                               mod(modUV.y, probePositions.y)*probeSize/256.*gBit;
                               
        vec2 probeUV = floor(modUV/probePositions) + 0.5;
        vec2 probeRel = probeUV - probeSize*0.5;
        float probeThetai = max(abs(probeRel.x), abs(probeRel.y));
        float probeTheta = probeThetai/probeSize*PI; 
        float probePhi = 0.;
        
        // 计算射线角度
        if (probeRel.x + 0.5 > probeThetai && probeRel.y - 0.5 > -probeThetai) {
            probePhi = probeRel.x - probeRel.y;
        } else if (probeRel.y - 0.5 < -probeThetai && probeRel.x - 0.5 > -probeThetai) {
            probePhi = probeThetai*2. - probeRel.y - probeRel.x;
        } else if (probeRel.x - 0.5 < -probeThetai && probeRel.y + 0.5 < probeThetai) {
            probePhi = probeThetai*4. - probeRel.x + probeRel.y;
        } else if (probeRel.y + 0.5 > probeThetai && probeRel.x + 0.5 < probeThetai) {
            probePhi = probeThetai*8. - (probeRel.y - probeRel.x);
        }
        probePhi = probePhi*PI*2./(4. + 8.*floor(probeThetai));
        
        // 构建射线方向
        vec3 probeDir = vec3(vec2(sin(probePhi), cos(probePhi))*sin(probeTheta), cos(probeTheta));
        probeDir = probeDir.x*gTan + probeDir.y*gBit + probeDir.z*gNor;
        
        // 光线追踪 (Ray Tracing)
        float tInterval = (1./64.)*probeSize*2.;
        if (probeCascade > 4.5) tInterval = 10000.;
        
        HIT rayHit = TraceRay(probePos + gNor*0.001, probeDir, tInterval, iTime);
        
        if (rayHit.n.x > -1.5) {
            Output.w = rayHit.t; 
            
            if (rayHit.c.x < -1.5) {
                // Reflective 
            } else if (max(rayHit.c.x, max(rayHit.c.y, rayHit.c.z)) > 1.5) {
                // Emissive 
                Output.xyz += rayHit.c;
            } else {
                // Geometry 
                if (dot(rayHit.n, probeDir) < 0.) {
                    vec2 suv = GetAtlasUV(rayHit); 
                    Output.xyz = SampleAtlas(suv, 0.).xyz + 
                                 SampleAtlas(suv + vec2(rayHit.res.x*0.5, 0.), 0.).xyz +
                                 SampleAtlas(suv + vec2(0., rayHit.res.y*0.5), 0.).xyz + 
                                 SampleAtlas(suv + rayHit.res*0.5, 0.).xyz;

                    Output.xyz *= rayHit.c;
                }
            }
        } else {
            Output.w = -1.;
            Output.xyz = vec3(0.);
        }
        
        // 半球归一化和余弦加权
        Output.xyz *= (cos(probeTheta - PI/probeSize) -
                       cos(probeTheta + PI/probeSize))/(4. + 8.*floor(probeThetai));
        Output.xyz *= cos(probeTheta); // Diffuse term
        
        // 级联合并 (Cascades Merging) - 核心滤波步骤
        if (probeCascade < 4.5) {
            float interpMinDist = (1./256.)*probeSize*1.5;
            float interpMaxInterval = interpMinDist;
            if (probeCascade < 0.5) { interpMinDist = 0.; interpMaxInterval *= 2.; }
            
            // 计算混合权重 l
            float l = 1. - clamp((rayHit.t - interpMinDist)/interpMaxInterval, 0., 1.);
            
            vec2 uvo = probePositions*0.5;
            vec2 lPUVOrigin = floor(UV/gRes)*gRes + vec2(0., gRes.y);
            vec2 lPUVDirs = floor(modUV/probePositions)*probePositions;
            vec2 lPUVPos = clamp(mod(modUV, probePositions)*0.5, vec2(0.5), uvo - 0.5);
            vec2 fPUVPos = fract(lPUVPos - 0.5);
            vec2 flPUVPos = floor(lPUVPos - 0.5) + 0.5;
            
            // 采样下一级级联 (Weighted Bilinear)
            vec4 S0 = WeightedSample(lPUVOrigin, lPUVDirs, flPUVPos,
                                     uvo, probePos, gTan, gBit, gPos, probeSize*2.);
            vec4 S1 = WeightedSample(lPUVOrigin, lPUVDirs, flPUVPos + vec2(1., 0.),
                                     uvo, probePos, gTan, gBit, gPos, probeSize*2.);
            vec4 S2 = WeightedSample(lPUVOrigin, lPUVDirs, flPUVPos + vec2(0., 1.),
                                     uvo, probePos, gTan, gBit, gPos, probeSize*2.);
            vec4 S3 = WeightedSample(lPUVOrigin, lPUVDirs, flPUVPos + 1.,
                                     uvo, probePos, gTan, gBit, gPos, probeSize*2.);
                                     
            vec3 lastOutput = mix(mix(S0.xyz, S1.xyz, fPUVPos.x), mix(S2.xyz, S3.xyz, fPUVPos.x), fPUVPos.y)/
                              max(0.01, mix(mix(S0.w, S1.w, fPUVPos.x), mix(S2.w, S3.w, fPUVPos.x), fPUVPos.y));
            
            // 混合当前级联结果与下一级结果
            if (!isnan(lastOutput.x)) 
                Output.xyz = Output.xyz*l + lastOutput*(1. - l);
        }
    }
    
    FragColor = Output;
}