in vec2 vUv;
out vec4 FragColor;

uniform sampler2D sdfTexture;
uniform vec2 sceneResolution;
uniform int cascadeLevel;

void main() {
  // 计算像素坐标
  vec2 px = (gl_FragCoord.xy - 0.5);

  CascadeConfig cfg = get_grid_config(cascadeLevel);
  CascadeID gid = get_grid_id(px, cfg);
  Box box = get_box(gid, cfg);

  // 计算当前像素在盒子里的相对位置
  vec2 diff = px - box.min;
  AngIdx local_coords = AngIdx(ivec2(diff));
  
  // 获取角度
  float angle = get_angle_from_coords(local_coords, cfg);

  // 准备射线
  float cx = cos(angle);
  float cy = sin(angle);
  vec2 dir = vec2(cx, cy);
  
  // 转换坐标系
  vec2 origin = box.center * sceneResolution / cascadeResolution;

  // 这里的 SampleRadiance_SDF 已经被重命名为 trace_scene
  vec4 result = trace_scene(sdfTexture, sceneResolution, origin, dir, cfg);

  FragColor = result;
}