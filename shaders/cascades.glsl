#define PI 3.14159265359

uniform int cascade0_Dims;
uniform float cascade0_Range;
uniform vec2 cascadeResolution;

struct CascadeConfig {
  int n;          
  int total;      
  int lev;        
  vec2 range;     
};

struct CascadeID {
  ivec2 id;
};

struct Box {
  vec2 min;
  vec2 max;
  vec2 center;
};

vec2 calc_range_helper(int l) {
  float f = 4.0;
  float num1 = 1.0 - pow(f, float(l));
  float num2 = 1.0 - pow(f, float(l) + 1.0);
  float den = 1.0 - f;
  
  float start = cascade0_Range * num1 / den;
  float end = cascade0_Range * num2 / den;

  return vec2(start, end);
}

CascadeConfig get_grid_config(int l) {
  int size = cascade0_Dims * (1 << l); 
  vec2 r = calc_range_helper(l);
  return CascadeConfig(size, size * size, l, r);
}

// 获取网格索引
CascadeID get_grid_id(vec2 px, CascadeConfig cfg) {
  float size = float(cfg.n);
  ivec2 idx = ivec2(floor(px / size));
  return CascadeID(idx);
}

// 创建包围盒
Box get_box(CascadeID gid, CascadeConfig cfg) {
  float size = float(cfg.n);
  vec2 base = size * vec2(gid.id);
  vec2 corner = base + size - 1.0;
  vec2 mid = base + 0.5 * (size - 1.0);
  
  return Box(base, corner, mid);
}

float get_half_step(CascadeConfig cfg) {
  float step = 2.0 * PI / float(cfg.total);
  return step * 0.5;
}

int angle_to_idx_raw(float ang, CascadeConfig cfg) {
  float norm = ang / (2.0 * PI);
  int raw = int(floor(norm * float(cfg.total)));
  return raw % int(cfg.total);
}

struct AngIdx {
  ivec2 xy;
};

AngIdx idx_to_coords(int idx, CascadeConfig cfg) {
  int safe_idx = idx % cfg.total;
  int x = safe_idx % cfg.n;
  int y = safe_idx / cfg.n;
  return AngIdx(ivec2(x, y));
}

// 查找附近的4个角度索引
AngIdx[4] find_neighbors(float ang, CascadeConfig cfg) {
  int center = angle_to_idx_raw(ang + get_half_step(cfg), cfg);

  AngIdx a1 = idx_to_coords(center - 1, cfg);
  AngIdx a2 = idx_to_coords(center, cfg);
  AngIdx a3 = idx_to_coords(center + 1, cfg);
  AngIdx a4 = idx_to_coords(center + 2, cfg);

  return AngIdx[4](a1, a2, a3, a4);
}

int coords_to_int(AngIdx ai, CascadeConfig cfg) {
  return ai.xy.x + ai.xy.y * cfg.n;
}

float idx_to_angle(int i, CascadeConfig cfg) {
  float step = 2.0 * PI / float(cfg.total);
  return float(i) * step;
}

// 从坐标反推角度
float get_angle_from_coords(AngIdx ai, CascadeConfig cfg) {
  int idx = coords_to_int(ai, cfg);
  float ang = idx_to_angle(idx, cfg) - get_half_step(cfg);
  return mod(ang, (2.0 * PI));
}

vec2 get_uv(Box b, AngIdx ai, vec2 res) {
  vec2 pixel_pos = b.min + vec2(ai.xy) + 0.5;
  return pixel_pos / res;
}

struct Weights {
  vec2 p1, p2, p3, p4; // 4个角的位置
  vec2 w;              // 权重
};

// 双线性插值准备
Weights setup_bilinear(vec2 px, CascadeConfig cfg) {
  float size = float(cfg.n);
  vec2 local_pos = px - size * 0.5;

  CascadeID gid = get_grid_id(local_pos, cfg);
  Box box = get_box(gid, cfg);
  vec2 center = box.center;

  vec2 diff = (px - center) / size;
  vec2 frac_part = fract(diff);

  vec2 bl = center;
  vec2 br = center + vec2(size, 0.0);
  vec2 tl = center + vec2(0.0, size);
  vec2 tr = center + vec2(size, size);

  return Weights(bl, br, tl, tr, frac_part);
}

// 射线追踪SDF
vec4 trace_scene(sampler2D tex, vec2 res, vec2 ro, vec2 rd, CascadeConfig cfg) {
  float t = cfg.range.x;
  float max_t = cfg.range.y;
  float current_t = t;

  // 最大迭代64次
  for (int k = 0; k < 64; ++k) {
    if (current_t > max_t) break;

    vec2 p = ro + current_t * rd;

    // 边界检查
    if (p.x < 0.0 || p.x > res.x - 1.0 || p.y < 0.0 || p.y > res.y - 1.0) {
      break;
    }

    vec2 uv = (p + 0.5) / res;
    vec4 samp = texture2D(tex, uv);

    float d = samp.w;
    vec3 col = samp.xyz;

    if (d > 0.1) {
      current_t += d;
      continue;
    }

    return vec4(col, 0.0); // Hit
  }

  return vec4(vec3(0.0), 1.0); // Miss
}

// 在特定方向采样
vec4 sample_dir(sampler2D tex, CascadeID gid, int lev, float rad) {
  CascadeConfig cfg = get_grid_config(lev);
  Box b = get_box(gid, cfg);
  vec2 center = b.center;

  // check bounds
  if (center.x < 0.0 || center.x >= cascadeResolution.x ||
      center.y < 0.0 || center.y >= cascadeResolution.y) {
    return vec4(0.0);
  }

  AngIdx neighbors[4] = find_neighbors(rad, cfg);
  vec4 sum = vec4(0.0);
  
  // 累加四个临近角度
  for (int i = 0; i < 4; i++) {
    vec2 uv = get_uv(b, neighbors[i], cascadeResolution);
    sum += texture(tex, uv);
  }
  
  return sum * 0.25;
}

// 双线性合并采样
vec4 sample_merged_bilinear(sampler2D tex, vec2 px, float rad, int lev) {
  CascadeConfig curr = get_grid_config(lev);
  CascadeConfig prev = get_grid_config(lev - 1);
  
  // 找到上一级的位置
  CascadeID prev_id = get_grid_id(px, prev);
  Box prev_box = get_box(prev_id, prev);
  
  Weights w = setup_bilinear(prev_box.center, curr);

  CascadeID id1 = get_grid_id(w.p1, curr);
  CascadeID id2 = get_grid_id(w.p2, curr);
  CascadeID id3 = get_grid_id(w.p3, curr);
  CascadeID id4 = get_grid_id(w.p4, curr);

  vec4 val1 = sample_dir(tex, id1, lev, rad);
  vec4 val2 = sample_dir(tex, id2, lev, rad);
  vec4 val3 = sample_dir(tex, id3, lev, rad);
  vec4 val4 = sample_dir(tex, id4, lev, rad);

  vec4 bottom = mix(val1, val2, w.w.x);
  vec4 top = mix(val3, val4, w.w.x);
  
  return mix(bottom, top, w.w.y);
}