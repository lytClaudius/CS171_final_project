
const float PI = 3.141592653;
const float ICYCLETIME = 1./5.;
const float CYCLETIME_OFFSET = 1.;
const float I256 = 1./256.;
const float I512 = 1./512.;
const float I1024 = 1./1024.;

const vec2 ATLAS_RES = vec2(1024.0, 6144.0); 


struct HIT { 
    float t;    // 射线击中距离
    vec2 uv;    // 击中点在当前平面内的局部UV (0-1范围)
    vec2 uvo;   // 当前平面在Atlas中的UV偏移原点 (像素单位)
    vec2 res;   // 当前平面在Atlas中的分辨率 (像素单位)
    vec3 n;     // 法线
    vec3 c;     // 材质颜色/属性 (x<-1:反射, x>1:自发光)
};

vec2 Rotate2(vec2 p, float ang) {
    float c = cos(ang);
    float s = sin(ang);
    return vec2(p.x*c - p.y*s, p.x*s + p.y*c);
}

vec2 Repeat2(vec2 p, float n) {
    float ang = 2.*PI/n;
    float sector = floor(atan(p.x, p.y)/ang + 0.5);
    return Rotate2(p, sector*ang);
}

// Tone mapping
vec3 AcesFilm(vec3 x) {
    return clamp((x*(2.51*x + 0.03))/(x*(2.43*x + 0.59) + 0.14), 0., 1.);
}

// TBN Matrix construction
mat3 TBN(vec3 N) {
    vec3 Nb, Nt;
    if (abs(N.y) > 0.999) {
        Nb = vec3(1., 0., 0.);
        Nt = vec3(0., 0., 1.);
    } else {
    	Nb = normalize(cross(N, vec3(0., 1., 0.)));
    	Nt = normalize(cross(Nb, N));
    }
    return mat3(Nb.x, Nt.x, N.x, Nb.y, Nt.y, N.y, Nb.z, Nt.z, N.z);
}

float DFBox(vec2 p, vec2 b) {
    vec2 d = abs(p - b*0.5) - b*0.5;
    return min(max(d.x, d.y), 0.) + length(max(d, 0.));
}

float DFBox(vec3 p, vec3 b) {
    vec3 d = abs(p - b*0.5) - b*0.5;
    return min(max(d.x, max(d.y, d.z)), 0.) + length(max(d, 0.));
}


// INTERSECTION LOGIC (Hardcoded Scene)
bool InteriorIntersection(vec3 p) {
    if (length(p.xy - vec2(0.5, 0.)) < 0.25) return true;
    if (length(p.xy - vec2(0.87, 0.25)) < 0.12) return true;
    return false;
}
// Quad Intersection
vec3 AQuad(vec3 p, vec3 d, vec3 vTan, vec3 vBit, vec3 vNor, vec2 pSize) {
    float norDot = dot(vNor, d);
    float pDot = dot(vNor, p);
    if (sign(norDot*pDot) < -0.5) {
        float t = -pDot/norDot;
        vec2 hit2 = vec2(dot(p + d*t, vTan), dot(p + d*t, vBit));
        if (DFBox(hit2, pSize) <= 0.) return vec3(hit2, t);
    }
    return vec3(-1.);
}

// Box Intersection (Normal)
vec2 ABoxNormal(vec3 origin, vec3 idir, vec3 bmin, vec3 bmax, out vec3 N) {
    vec3 tMin = (bmin - origin)*idir;
    vec3 tMax = (bmax - origin)*idir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    vec3 signdir = -(max(vec3(0.), sign(idir))*2. - 1.);
    if (t1.x > max(t1.y, t1.z)) N = vec3(signdir.x, 0., 0.);
    else if (t1.y > t1.z) N = vec3(0., signdir.y, 0.);
    else N = vec3(0., 0., signdir.z);
    return vec2(max(max(t1.x, t1.y), t1.z), min(min(t2.x, t2.y), t2.z));
}

// Sphere Intersection
float ASphere(vec3 p, vec3 d, float r) {
    float a = dot(p, p) - r*r;
    float b = 2.*dot(p, d);
    float re = b*b*0.25 - a;
    if (dot(p, d) < 0. && re > 0.) {
        return -b*0.5 - sqrt(re);
    }
    return -1.;
}

// Cylinder Intersection (Z-axis)
float ACylZ(vec3 p, vec3 d, float r) {
    float a = (dot(p.xy, p.xy) - r*r)/dot(d.xy, d.xy);
    float b = 2.*dot(p.xy, d.xy)/dot(d.xy, d.xy);
    float re = b*b*0.25 - a;
    if (re > 0.) {
        return -b*0.5 + sqrt(re);
    }
    return -1.;
}

HIT TraceRay(vec3 p, vec3 d, float maxt, float time) {
    // 初始化 HIT，材质颜色默认为 -1 (黑色/无属性)
    HIT info = HIT(maxt, vec2(-1.), vec2(-1.), vec2(-1.), vec3(-20.), vec3(-1.));
    vec3 uvt, sp;
    vec2 bb;
    float st;
    
    // ---------------------------------------------------------
    // 1. Room Shell 
    // ---------------------------------------------------------

    // Floor (地板)
    uvt = AQuad(p, d, vec3(1., 0., 0.), vec3(0., 0., 1.), vec3(0., 1., 0.), vec2(1., 1.));
    if (uvt.z > -0.5 && uvt.z < info.t)
        info = HIT(uvt.z, uvt.xy, vec2(0., 0.), vec2(256.), vec3(0., 1., 0.), vec3(0.9));
    
    // Ceiling (天花板)
    uvt = AQuad(p - vec3(0., 0.5, 0.), d, vec3(1., 0., 0.), vec3(0., 0., 1.), vec3(0., 1., 0.), vec2(1., 1.));
    if (uvt.z > -0.5 && uvt.z < info.t)
        info = HIT(uvt.z, uvt.xy, vec2(256., 0.), vec2(256.), vec3(0., -1., 0.), vec3(0.9));
    
    // Walls X
    uvt = AQuad(p, d, vec3(0., 1., 0.), vec3(0., 0., 1.), vec3(1., 0., 0.), vec2(0.5, 1.));
    if (uvt.z > -0.5 && uvt.z < info.t)
        info = HIT(uvt.z, uvt.xy, vec2(512., 0.), vec2(128., 256.), vec3(1., 0., 0.), vec3(0.9, 0.1, 0.1));
        
    uvt = AQuad(p - vec3(1., 0., 0.), d, vec3(0., 1., 0.), vec3(0., 0., 1.), vec3(-1., 0., 0.), vec2(0.5, 1.));
    if (uvt.z > -0.5 && uvt.z < info.t)
        info = HIT(uvt.z, uvt.xy, vec2(640., 0.), vec2(128., 256.), vec3(-1., 0., 0.), vec3(0.05, 0.95, 0.1));
    
    // Walls Z
    uvt = AQuad(p, d, vec3(0., 1., 0.), vec3(1., 0., 0.), vec3(0., 0., -1.), vec2(0.5, 1.));
    if (uvt.z > -0.5 && uvt.z < info.t)
        info = HIT(uvt.z, uvt.xy, vec2(768., 0.), vec2(128., 256.), vec3(0., 0., 1.), vec3(0.9));
        
    uvt = AQuad(p - vec3(0., 0., 1.), d, vec3(0., 1., 0.), vec3(1., 0., 0.), vec3(0., 0., -1.), vec2(0.5, 1.));
    if (uvt.z > -0.5 && uvt.z < info.t)
        info = HIT(uvt.z, uvt.xy, vec2(896., 0.), vec2(128., 256.), vec3(0., 0., -1.), vec3(0.9));

    //Interior wall
    uvt = AQuad(p - vec3(0., 0., 0.47 - 1./256.), d, vec3(0., 1., 0.), vec3(1., 0., 0.), vec3(0., 0., -1.), vec2(0.5, 1.));
    if (uvt.z > -0.5 && uvt.z < info.t && !InteriorIntersection(p + d*uvt.z))
        info = HIT(uvt.z, uvt.xy, vec2(0., 1536), vec2(128., 256.), vec3(0., 0., -1.), vec3(0.99));
    uvt = AQuad(p - vec3(0., 0., 0.53 - 1./256.), d, vec3(0., 1., 0.), vec3(1., 0., 0.), vec3(0., 0., -1.), vec2(0.5, 1.));
    if (uvt.z > -0.5 && uvt.z < info.t && !InteriorIntersection(p + d*uvt.z))
        info = HIT(uvt.z, uvt.xy, vec2(128., 1536.), vec2(128., 256.), vec3(0., 0., 1.), vec3(0.99));
    sp = p - vec3(0.5, 0., 0.);
    st = ACylZ(sp, d, 0.25);
    if (st > 0. && st < info.t && sp.z + d.z*st >= 0.47 - 1./256. && sp.z + d.z*st <= 0.53 - 1./256.)
        info = HIT(st, vec2(1.), vec2(-1.), vec2(-1.), vec3(-normalize(sp.xy + d.xy*st), 0.), vec3(0.));
    sp = p - vec3(0.87, 0.25, 0.);
    st = ACylZ(sp, d, 0.12);
    if (st > 0. && st < info.t && sp.z + d.z*st >= 0.47 - 1./256. && sp.z + d.z*st <= 0.53 - 1./256.)
        info = HIT(st, vec2(1.), vec2(-1.), vec2(-1.), vec3(-normalize(sp.xy + d.xy*st), 0.), vec3(0.));

    // 地面球形光源 1 (Red)
    sp = p - vec3(0.25, 0.05, 0.25);
    st = ASphere(sp, d, 0.05); // 半径 0.05
    if (st > 0. && st < info.t) 
        // 红色高光
        info = HIT(st, vec2(0.), vec2(0.), vec2(0.), normalize(sp + d*st), vec3(10.0, 0.1, 0.1));

    // 地面球形光源 2 (Green)
    sp = p - vec3(0.75, 0.05, 0.25);
    st = ASphere(sp, d, 0.05);
    if (st > 0. && st < info.t) 
        // 绿色高光
        info = HIT(st, vec2(0.), vec2(0.), vec2(0.), normalize(sp + d*st), vec3(0.1, 10.0, 0.1));

    // 地面球形光源 3 (Blue)
    sp = p - vec3(0.5, 0.05, 0.75);
    st = ASphere(sp, d, 0.05);
    if (st > 0. && st < info.t) 
        // 蓝色高光
        info = HIT(st, vec2(0.), vec2(0.), vec2(0.), normalize(sp + d*st), vec3(0.1, 0.1, 10.0));
    
    // Mirror box (Reflective)
    vec3 sn;
    vec3 sd = normalize(d); 
    sp = p - vec3(0.86, 0.14, 0.86);
    bb = ABoxNormal(sp, 1./sd, vec3(-0.08), vec3(0.08), sn);
    if (bb.x > 0. && bb.y > bb.x && bb.x < info.t) {
        info = HIT(bb.x, vec2(1.), vec2(-1.), vec2(-1.), normalize(sn), vec3(-2.));
    }
    
    return info;
}

// =========================================================================
// MAPPING HELPERS
// =========================================================================

// Helper to convert a Ray Hit on a surface to the Radiance Atlas Texture Coordinates
// This is critical for the "Final Display" shader to look up the baked lighting
vec2 GetAtlasUV(HIT hit) {
    // hit.uv is normalized 0..1 within the quad
    // hit.res is the resolution in pixels of that quad area in the atlas
    // hit.uvo is the pixel offset in the atlas
    
    // Shadertoy logic: clamp(rayHit.uv*128., vec2(0.5), rayHit.res*0.5 - 0.5) + rayHit.uvo;
    // Note: The factor 128. comes from the fact that UVs in TraceRay AQuad are often scaled 
    // based on 0.5 size. Let's stick to the reference implementation logic:
    
    // In AQuad, uv returned is dot product with Tangent/Bitangent.
    // For a 1x1 quad centered at 0, range is -0.5 to 0.5.
    // So usually we map [-0.5, 0.5] -> [0, 1] for texture lookup.
    
    // However, looking at TraceRay, it returns raw projected coordinates.
    // And mainImage does: clamp(rayHit.uv*128., ...)
    
    // Let's implement exactly what mainImage does:
    return clamp(hit.uv * 128.0, vec2(0.5), hit.res * 0.5 - 0.5) + hit.uvo;
}