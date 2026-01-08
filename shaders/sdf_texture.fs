uniform vec2 resolution; // 屏幕分辨率 (WIDTH, HEIGHT)

out vec4 FragColor;


void main() {
    // 1. 当前像素的坐标 (从 0,0 到 WIDTH,HEIGHT)
    vec2 p = gl_FragCoord.xy;
    
    // 抗锯齿参数
    float pixelSize = 2.0 / min(resolution.x, resolution.y);
    float aa = pixelSize * 1.5;

    // 初始化场景
    vec3 finalCol = vec3(0.0);
    float finalDist = 10000.0;

    // --- 光源 1: 暖黄色光 (左上) ---
    vec2 light1Pos = resolution * vec2(0.25, 0.7);
    float light1Radius = 25.0;
    float dLight1 = sdfCircle(p - light1Pos, light1Radius);
    vec3 colLight1 = vec3(10.0, 8.0, 3.0);
    float light1Mask = smoothstep(aa, -aa, dLight1);
    finalCol += colLight1 * light1Mask;
    finalDist = min(finalDist, dLight1);

    // --- 光源 2: 冷蓝色光 (右上) ---
    vec2 light2Pos = resolution * vec2(0.75, 0.7);
    float light2Radius = 28.0;
    float dLight2 = sdfCircle(p - light2Pos, light2Radius);
    vec3 colLight2 = vec3(2.0, 6.0, 12.0);
    float light2Mask = smoothstep(aa, -aa, dLight2);
    finalCol = mix(finalCol, colLight2, light2Mask);
    finalDist = min(finalDist, dLight2);

    // --- 光源 3: 绿色光 (左下) ---
    vec2 light3Pos = resolution * vec2(0.2, 0.3);
    float light3Radius = 20.0;
    float dLight3 = sdfCircle(p - light3Pos, light3Radius);
    vec3 colLight3 = vec3(3.0, 10.0, 4.0);
    float light3Mask = smoothstep(aa, -aa, dLight3);
    finalCol = mix(finalCol, colLight3, light3Mask);
    finalDist = min(finalDist, dLight3);

    // --- 光源 4: 洋红色光 (右下) ---
    vec2 light4Pos = resolution * vec2(0.8, 0.25);
    float light4Radius = 22.0;
    float dLight4 = sdfCircle(p - light4Pos, light4Radius);
    vec3 colLight4 = vec3(12.0, 2.0, 8.0);
    float light4Mask = smoothstep(aa, -aa, dLight4);
    finalCol = mix(finalCol, colLight4, light4Mask);
    finalDist = min(finalDist, dLight4);

    // --- 光源 5: 橙色光 (中心偏上) ---
    vec2 light5Pos = resolution * vec2(0.5, 0.6);
    float light5Radius = 18.0;
    float dLight5 = sdfCircle(p - light5Pos, light5Radius);
    vec3 colLight5 = vec3(15.0, 6.0, 1.0);
    float light5Mask = smoothstep(aa, -aa, dLight5);
    finalCol = mix(finalCol, colLight5, light5Mask);
    finalDist = min(finalDist, dLight5);

    // --- 遮挡物 1: 中央圆形 ---
    vec2 blocker1Pos = resolution * vec2(0.5, 0.4);
    float blocker1Radius = 60.0;
    float dBlocker1 = sdfCircle(p - blocker1Pos, blocker1Radius);
    vec3 colBlocker1 = vec3(0.0);
    float blocker1Mask = smoothstep(aa, -aa, dBlocker1);
    finalCol = mix(finalCol, colBlocker1, blocker1Mask);
    finalDist = min(finalDist, dBlocker1);

    // --- 遮挡物 2: 方形遮挡物 (右边) ---
    vec2 blocker2Pos = resolution * vec2(0.7, 0.45);
    float dBlocker2 = sdfBox(p - blocker2Pos, vec2(40.0, 80.0));
    vec3 colBlocker2 = vec3(0.0);
    float blocker2Mask = smoothstep(aa, -aa, dBlocker2);
    finalCol = mix(finalCol, colBlocker2, blocker2Mask);
    finalDist = min(finalDist, dBlocker2);

    // --- 输出结果 ---
    FragColor = vec4(finalCol, finalDist);
}